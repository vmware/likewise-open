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
VmDirRegReadString(
    HANDLE hConnection,
    HKEY   hKey,
    PCSTR  pszKey,
    PSTR*  ppszValue
    );

static
DWORD
VmDirRegReadInt32(
    HANDLE hConnection,
    HKEY   hKey,
    PCSTR  pszName,
    PDWORD pdwValue
    );

static
VOID
VmDirFreeBindInfo(
    PVMDIR_BIND_INFO pBindInfo
    );

DWORD
VmDirGetBindProtocol(
    VMDIR_BIND_PROTOCOL* pBindProtocol
    )
{
    DWORD dwError = 0;
    HANDLE hConnection = NULL;
    HKEY   hKeyRoot = NULL;
    HKEY   hKeyVmDirProvider = NULL;
    PSTR   pszBindProtocol = NULL;
    VMDIR_BIND_PROTOCOL protocol = VMDIR_BIND_PROTOCOL_SRP;
    
    dwError = RegOpenServer(&hConnection);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &hKeyRoot);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    hKeyRoot,
                    VMDIR_AUTH_PROVIDER_KEY,
                    0,
                    KEY_READ,
                    &hKeyVmDirProvider);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirRegReadString(
                   hConnection,
                   hKeyVmDirProvider,
                   VMDIR_REG_KEY_BIND_PROTOCOL,
                   &pszBindProtocol);
    BAIL_ON_VMDIR_ERROR(dwError);

    if (!strcasecmp(pszBindProtocol, VMDIR_BIND_PROTOCOL_KERBEROS_STR))
    {
        protocol = VMDIR_BIND_PROTOCOL_KERBEROS;
    }
    else if (!strcasecmp(pszBindProtocol, VMDIR_BIND_PROTOCOL_SRP_STR))
    {
        protocol = VMDIR_BIND_PROTOCOL_SRP;
    }

error:

    *pBindProtocol = protocol; /* on error, return default protocol */

    if (hKeyVmDirProvider)
    {
        RegCloseKey(hConnection, hKeyVmDirProvider);
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
VmDirCreateBindInfo(
    PVMDIR_BIND_INFO* ppBindInfo
    )
{
    DWORD dwError = 0;
    PVMDIR_BIND_INFO pBindInfo = NULL;
    HANDLE hConnection = NULL;
    HKEY   hKeyRoot = NULL;
    HKEY   hKeyVmDir = NULL;
    PCSTR  pszKey_account  = VMDIR_REG_KEY_BIND_INFO_ACCOUNT;
    PCSTR  pszKey_binddn   = VMDIR_REG_KEY_BIND_INFO_BIND_DN;
    PSTR   pszDCName = NULL;
    PSTR   pszDomainName = NULL;
    PSTR   pszBindDN = NULL;
    PSTR   pszAccount = NULL;

    dwError = RegOpenServer(&hConnection);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &hKeyRoot);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmAfdGetDCInfo(
                    hConnection,
                    hKeyRoot,
                    &pszDomainName,
                    &pszDCName);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    hKeyRoot,
                    VMDIR_REG_KEY,
                    0,
                    KEY_READ,
                    &hKeyVmDir);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(VMDIR_BIND_INFO), (PVOID*)&pBindInfo);
    BAIL_ON_VMDIR_ERROR(dwError);

    pBindInfo->refCount = 1;

    pBindInfo->pszDomainFqdn = pszDomainName;
    pszDomainName = NULL;

    dwError = LwAllocateStringPrintf(
                    &pBindInfo->pszURI,
                    "ldap://%s",
                    pszDCName);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirRegReadString(
                    hConnection,
                    hKeyVmDir,
                    pszKey_account,
                    &pszAccount);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirRegReadString(
                    hConnection,
                    hKeyVmDir,
                    pszKey_binddn,
                    &pszBindDN);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                    &pBindInfo->pszUPN,
                    "%s@%s",
                    pszAccount,
                    pBindInfo->pszDomainFqdn);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateString(
                    pBindInfo->pszDomainFqdn,
                    &pBindInfo->pszDomainShort);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirGetDefaultSearchBase(
                    pszBindDN,
                    &pBindInfo->pszSearchBase);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppBindInfo = pBindInfo;

cleanup:

    if (hKeyVmDir)
    {
        RegCloseKey(hConnection, hKeyVmDir);
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
        VmDirFreeBindInfo(pBindInfo);
    }

    goto cleanup;
}

DWORD
VmDirCreateBindInfoPassword(
    PSTR *ppszPassword
    )
{
    DWORD  dwError = 0;
    PSTR   pszPassword = NULL;
    HANDLE hConnection = NULL;
    HKEY   hKeyRoot = NULL;
    HKEY   hKeyVmDir = NULL;
    PCSTR  pszKey_password = VMDIR_REG_KEY_BIND_INFO_PASSWORD;

    dwError = RegOpenServer(&hConnection);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &hKeyRoot);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    hKeyRoot,
                    VMDIR_REG_KEY,
                    0,
                    KEY_READ,
                    &hKeyVmDir);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirRegReadString(
                    hConnection,
                    hKeyVmDir,
                    pszKey_password,
                    &pszPassword);
    BAIL_ON_VMDIR_ERROR(dwError);

    *ppszPassword = pszPassword;

cleanup:

    if (hKeyVmDir)
    {
        RegCloseKey(hConnection, hKeyVmDir);
    }

    if (hKeyRoot)
    {
        RegCloseKey(hConnection, hKeyRoot);
    }

    if (hConnection)
    {
        RegCloseServer(hConnection);
    }

    return dwError;

error:

    LW_SECURE_FREE_STRING(pszPassword);

    goto cleanup;
}

PVMDIR_BIND_INFO
VmDirAcquireBindInfo(
    PVMDIR_BIND_INFO pBindInfo
    )
{
    if (pBindInfo)
    {
        LwInterlockedIncrement(&pBindInfo->refCount);
    }

    return pBindInfo;
}

VOID
VmDirReleaseBindInfo(
    PVMDIR_BIND_INFO pBindInfo
    )
{
    if (pBindInfo && (0 == LwInterlockedDecrement(&pBindInfo->refCount)))
    {
        VmDirFreeBindInfo(pBindInfo);
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
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirRegReadInt32(
                    hConnection,
                    hKeyVmAfd,
                    pszKey_DomainState,
                    &dwDomainState);
    BAIL_ON_VMDIR_ERROR(dwError);

    switch (dwDomainState)
    {
        case 1: // domain controller
        case 2: // joined client
            break;
        default:
            dwError = ERROR_NOT_JOINED;
            break;
    }
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirRegReadString(
                    hConnection,
                    hKeyVmAfd,
                    pszKey_DomainName,
                    &pszDomainName);
    BAIL_ON_VMDIR_ERROR(dwError);

    LwStrToUpper(pszDomainName);

    dwError = VmDirRegReadString(
                    hConnection,
                    hKeyVmAfd,
                    pszKey_DCName,
                    &pszDCName);
    BAIL_ON_VMDIR_ERROR(dwError);

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
VmDirRegReadString(
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
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = LwAllocateString((dwSize > 0 ? szValue : ""), &pszValue);
    BAIL_ON_VMDIR_ERROR(dwError);

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
VmDirRegReadInt32(
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
    BAIL_ON_VMDIR_ERROR(dwError);

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
VmDirFreeBindInfo(
    PVMDIR_BIND_INFO pBindInfo
    )
{
    if (pBindInfo)
    {
        LW_SECURE_FREE_STRING(pBindInfo->pszURI);
        LW_SECURE_FREE_STRING(pBindInfo->pszUPN);
        LW_SAFE_FREE_STRING(pBindInfo->pszDomainFqdn);
        LW_SAFE_FREE_STRING(pBindInfo->pszDomainShort);
        LW_SAFE_FREE_STRING(pBindInfo->pszSearchBase);

        LwFreeMemory(pBindInfo);
    }
}

DWORD
VmDirGetCacheEntryExpiry(
    DWORD *pdwCacheEntryExpiry
    )
{
    DWORD dwError = 0;
    HANDLE hConnection = NULL;
    HKEY   hKeyRoot = NULL;
    HKEY   hKeyVmDirProvider = NULL;
    DWORD  dwCacheEntryExpiry = VMDIR_CACHE_ENTRY_EXPIRY_DEFAULT_SECS;

    dwError = RegOpenServer(&hConnection);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &hKeyRoot);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = RegOpenKeyExA(
                    hConnection,
                    hKeyRoot,
                    VMDIR_AUTH_PROVIDER_KEY,
                    0,
                    KEY_READ,
                    &hKeyVmDirProvider);
    BAIL_ON_VMDIR_ERROR(dwError);

    dwError = VmDirRegReadInt32(
                   hConnection,
                   hKeyVmDirProvider,
                   VMDIR_REG_KEY_CACHE_ENTRY_EXPIRY,
                   &dwCacheEntryExpiry);
    BAIL_ON_VMDIR_ERROR(dwError);

error:

    *pdwCacheEntryExpiry = dwCacheEntryExpiry; /* on error, return default value */

    if (hKeyVmDirProvider)
    {
        RegCloseKey(hConnection, hKeyVmDirProvider);
    }
    if (hKeyRoot)
    {
        RegCloseKey(hConnection, hKeyRoot);
    }
    if (hConnection)
    {
        RegCloseServer(hConnection);
    }

    return ERROR_SUCCESS;
}
