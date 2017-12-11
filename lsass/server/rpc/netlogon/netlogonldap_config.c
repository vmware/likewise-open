/*
 * Copyright (C) VMware. All rights reserved.
 */

#include "includes.h"

static
DWORD
VmAfdGetDCInfo(
    HANDLE hConnection,
    HKEY   hKeyRoot,
    PSTR*  ppszDomainName,
    PSTR*  ppszDCName
    );

static
DWORD
NetlogonRegReadString(
    HANDLE hConnection,
    HKEY   hKey,
    PCSTR  pszKey,
    PSTR*  ppszValue
    );

static
DWORD
NetlogonRegReadInt32(
    HANDLE hConnection,
    HKEY   hKey,
    PCSTR  pszName,
    PDWORD pdwValue
    );

static
VOID
NetlogonFreeBindInfo(
    PNETLOGON_LDAP_BIND_INFO pBindInfo
    );

DWORD
NetlogonGetBindProtocol(
    NETLOGON_LDAP_BIND_PROTOCOL* pBindProtocol
    )
{
    DWORD dwError = 0;
    HANDLE hConnection = NULL;
    HKEY   hKeyRoot = NULL;
    HKEY   hKeyNetlogonProvider = NULL;
    PSTR   pszBindProtocol = NULL;
    NETLOGON_LDAP_BIND_PROTOCOL protocol = NETLOGON_LDAP_BIND_PROTOCOL_SRP;
    
    dwError = RegOpenServer(&hConnection);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = RegOpenKeyExA(
                    hConnection,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &hKeyRoot);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = RegOpenKeyExA(
                    hConnection,
                    hKeyRoot,
                    VMDIR_AUTH_PROVIDER_KEY,
                    0,
                    KEY_READ,
                    &hKeyNetlogonProvider);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    dwError = NetlogonRegReadString(
                   hConnection,
                   hKeyNetlogonProvider,
                   NETLOGON_REG_KEY_BIND_PROTOCOL,
                   &pszBindProtocol);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);
    
    if (!strcasecmp(pszBindProtocol, NETLOGON_LDAP_BIND_PROTOCOL_KERBEROS_STR))
    {
        protocol = NETLOGON_LDAP_BIND_PROTOCOL_KERBEROS;
    }
    else if (!strcasecmp(pszBindProtocol, NETLOGON_LDAP_BIND_PROTOCOL_SRP_STR))
    {
        protocol = NETLOGON_LDAP_BIND_PROTOCOL_SRP;
    }
    
error:
    
    *pBindProtocol = protocol; /* on error, return default protocol */
    
    if (hKeyNetlogonProvider)
    {
        RegCloseKey(hConnection, hKeyNetlogonProvider);
    }
    if (hKeyRoot)
    {
        RegCloseKey(hConnection, hKeyRoot);
    }
    if (hConnection)
    {
        RegCloseServer(hConnection);
    }
    
    LW_SAFE_FREE_STRING(pszBindProtocol);

    return ERROR_SUCCESS;
}

DWORD
NetlogonCreateBindInfo(
    PNETLOGON_LDAP_BIND_INFO* ppBindInfo
    )
{
    DWORD dwError = 0;
    PNETLOGON_LDAP_BIND_INFO pBindInfo = NULL;
    HANDLE hConnection = NULL;
    HKEY   hKeyRoot = NULL;
    HKEY   hKeyNetlogon = NULL;
    PCSTR  pszKey_account  = NETLOGON_REG_KEY_BIND_INFO_ACCOUNT;
    PCSTR  pszKey_binddn   = NETLOGON_REG_KEY_BIND_INFO_BIND_DN;
    PCSTR  pszKey_password = NETLOGON_REG_KEY_BIND_INFO_PASSWORD;
    PSTR   pszDCName = NULL;
    PSTR   pszDomainName = NULL;
    PSTR   pszBindDN = NULL;
    PSTR   pszAccount = NULL;

    dwError = RegOpenServer(&hConnection);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &hKeyRoot);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = VmAfdGetDCInfo(
                    hConnection,
                    hKeyRoot,
                    &pszDomainName,
                    &pszDCName);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    hKeyRoot,
                    NETLOGON_REG_KEY,
                    0,
                    KEY_READ,
                    &hKeyNetlogon);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(NETLOGON_LDAP_BIND_INFO), (PVOID*)&pBindInfo);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    pBindInfo->refCount = 1;

    pBindInfo->pszDomainFqdn = pszDomainName;
    pszDomainName = NULL;

    dwError = LwAllocateStringPrintf(
                    &pBindInfo->pszURI,
                    "ldap://%s",
                    pszDCName);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = NetlogonRegReadString(
                    hConnection,
                    hKeyNetlogon,
                    pszKey_account,
                    &pszAccount);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = NetlogonRegReadString(
                    hConnection,
                    hKeyNetlogon,
                    pszKey_binddn,
                    &pszBindDN);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pBindInfo->pszUPN,
                    "%s@%s",
                    pszAccount,
                    pBindInfo->pszDomainFqdn);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = NetlogonRegReadString(
                    hConnection,
                    hKeyNetlogon,
                    pszKey_password,
                    &pBindInfo->pszPassword);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = LwAllocateString(
                    pBindInfo->pszDomainFqdn,
                    &pBindInfo->pszDomainShort);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = NetlogonGetDefaultSearchBase(
                    pszBindDN,
                    &pBindInfo->pszSearchBase);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    *ppBindInfo = pBindInfo;

cleanup:

    if (hKeyNetlogon)
    {
        RegCloseKey(hConnection, hKeyNetlogon);
    }

    if (hKeyRoot)
    {
        RegCloseKey(hConnection, hKeyRoot);
    }

    if (hConnection)
    {
        RegCloseServer(hConnection);
    }

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszDCName);
    LW_SAFE_FREE_STRING(pszAccount);
    LW_SAFE_FREE_STRING(pszBindDN);

    return dwError;

error:

    *ppBindInfo = NULL;

    if (pBindInfo)
    {
        NetlogonFreeBindInfo(pBindInfo);
    }

    goto cleanup;
}

VOID
NetlogonReleaseBindInfo(
    PNETLOGON_LDAP_BIND_INFO pBindInfo
    )
{
    if (pBindInfo && (0 == LwInterlockedDecrement(&pBindInfo->refCount)))
    {
        NetlogonFreeBindInfo(pBindInfo);
    }
}

static
DWORD
VmAfdGetDCInfo(
    HANDLE hConnection,
    HKEY   hKeyRoot,
    PSTR*  ppszDomainName,
    PSTR*  ppszDCName
    )
{
    DWORD dwError = 0;
    HKEY  hKeyVmAfd = NULL;
    PCSTR pszKey_DomainName  = VMAFD_REG_KEY_DOMAIN_NAME;
    PCSTR pszKey_DomainState = VMAFD_REG_KEY_DOMAIN_STATE;
    PCSTR pszKey_DCName = VMAFD_REG_KEY_DC_NAME;
    DWORD dwDomainState = 0;
    PSTR  pszDomainName = NULL;
    PSTR  pszDCName = NULL;

    dwError = RegOpenKeyExA(
                    hConnection,
                    hKeyRoot,
                    VMAFD_REG_KEY,
                    0,
                    KEY_READ,
                    &hKeyVmAfd);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = NetlogonRegReadInt32(
                    hConnection,
                    hKeyVmAfd,
                    pszKey_DomainState,
                    &dwDomainState);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    switch (dwDomainState)
    {
        case 1: // domain controller
        case 2: // joined client
            break;
        default:
            dwError = ERROR_NOT_JOINED;
            break;
    }
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = NetlogonRegReadString(
                    hConnection,
                    hKeyVmAfd,
                    pszKey_DomainName,
                    &pszDomainName);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    LwStrToUpper(pszDomainName);

    dwError = NetlogonRegReadString(
                    hConnection,
                    hKeyVmAfd,
                    pszKey_DCName,
                    &pszDCName);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    *ppszDomainName = pszDomainName;
    *ppszDCName = pszDCName;

cleanup:

    if (hKeyVmAfd)
    {
         RegCloseKey(hConnection, hKeyVmAfd);
    }

    return dwError;

error:

    if (dwError == LWREG_ERROR_NO_SUCH_KEY_OR_VALUE)
    {
        dwError = ERROR_NOT_JOINED;
    }

    if (ppszDCName)
    {
        *ppszDCName = NULL;
    }
    if (ppszDomainName)
    {
        *ppszDomainName = NULL;
    }

    LW_SAFE_FREE_STRING(pszDomainName);
    LW_SAFE_FREE_STRING(pszDCName);

    goto cleanup;
}

static
DWORD
NetlogonRegReadString(
    HANDLE hConnection,
    HKEY   hKey,
    PCSTR  pszName,
    PSTR*  ppszValue
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwType = 0;
    CHAR  szValue[MAX_VALUE_LENGTH];
    DWORD dwSize = sizeof(szValue);
    PSTR  pszValue = NULL;

    memset(&szValue[0], 0, dwSize);

    dwError = RegGetValueA(
                hConnection,
                hKey,
                NULL,
                pszName,
                RRF_RT_REG_SZ,
                &dwType,
                szValue,
                &dwSize);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    dwError = LwAllocateString((dwSize > 0 ? szValue : ""), &pszValue);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    *ppszValue = pszValue;

cleanup:

    return dwError;

error:

    if (ppszValue)
    {
        *ppszValue = NULL;
    }

    LW_SAFE_FREE_STRING(pszValue);

    goto cleanup;
}

static
DWORD
NetlogonRegReadInt32(
    HANDLE hConnection,
    HKEY   hKey,
    PCSTR  pszName,
    PDWORD pdwValue
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD dwType = 0;
    DWORD dwValue = 0;
    DWORD dwSize = sizeof(dwValue);

    dwError = RegGetValueA(
                hConnection,
                hKey,
                NULL,
                pszName,
                RRF_RT_REG_DWORD,
                &dwType,
                (PVOID)&dwValue,
                &dwSize);
    BAIL_ON_NETLOGON_LDAP_ERROR(dwError);

    *pdwValue = dwValue;

cleanup:

    return dwError;

error:

    if (pdwValue)
    {
        *pdwValue = 0;
    }

    goto cleanup;
}

static
VOID
NetlogonFreeBindInfo(
    PNETLOGON_LDAP_BIND_INFO pBindInfo
    )
{
    if (pBindInfo)
    {
        LW_SECURE_FREE_STRING(pBindInfo->pszURI);
        LW_SECURE_FREE_STRING(pBindInfo->pszUPN);
        LW_SECURE_FREE_STRING(pBindInfo->pszPassword);
        LW_SAFE_FREE_STRING(pBindInfo->pszDomainFqdn);
        LW_SAFE_FREE_STRING(pBindInfo->pszDomainShort);
        LW_SAFE_FREE_STRING(pBindInfo->pszSearchBase);

        LwFreeMemory(pBindInfo);
    }
}
