/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        join.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Join to Active Directory
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

#define LSA_JOIN_OU_PREFIX "OU="
#define LSA_JOIN_CN_PREFIX "CN="
#define LSA_JOIN_DC_PREFIX "DC="

#define LSA_JOIN_MAX_ALLOWED_CLOCK_DRIFT_SECONDS 60


static
VOID
LsaGenerateMachinePassword(
    PWSTR  pwszPassword,
    size_t sPasswordLen
    );


static
VOID
LsaGenerateRandomString(
    PWSTR   pwszBuffer,
    size_t  sBufferLen
    );


static
DWORD
LsaCharacterClassesInPassword(
    const wchar16_t* password,
    size_t len
    );


static
DWORD
LsaGetAccountName(
    const wchar16_t *machname,
    const wchar16_t *domain_controller_name,
    const wchar16_t *dns_domain_name,
    wchar16_t       **account_name
    );


static
UINT32
NetWc16sHash(
    const wchar16_t *str
    );


static
wchar16_t *
NetHashToWc16s(
    UINT32    hash
    );


static
NTSTATUS
LsaCreateMachineAccount(
    PWSTR          pwszDCName,
    LW_PIO_CREDS   pCreds,
    PWSTR          pwszMachineAccountName,
    PWSTR          pwszMachinePassword,
    DWORD          dwJoinFlags,
    PWSTR         *ppwszDomainName,
    PSID          *ppDomainSid
    );


static
NTSTATUS
LsaEncryptPasswordBufferEx(
    PBYTE  pPasswordBuffer,
    DWORD  dwPasswordBufferSize,
    PWSTR  pwszPassword,
    DWORD  dwPasswordLen,
    PBYTE  pSessionKey,
    DWORD  dwSessionKeyLen
    );


static
NTSTATUS
LsaEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pBlob,
    IN  DWORD   dwBlobSize
    );


static
DWORD
LsaSaveMachinePassword(
    PCWSTR  pwszMachineName,
    PCWSTR  pwszMachineAccountName,
    PCWSTR  pwszMachineDnsDomain,
    PCWSTR  pwszDomainName,
    PCWSTR  pwszDnsDomainName,
    PCWSTR  pwszDCName,
    PCWSTR  pwszSidStr,
    PCWSTR  pwszPassword
    );


static
DWORD
LsaSavePrincipalKey(
    PCWSTR  pwszName,
    PCWSTR  pwszPassword,
    DWORD   dwPasswordLen,
    PCWSTR  pwszRealm,
    PCWSTR  pwszSalt,
    PCWSTR  pwszDCName,
    DWORD   dwKvno
    );


static
DWORD
LsaDirectoryConnect(
    const wchar16_t *domain,
    LDAP **ldconn,
    wchar16_t **dn_context
    );


static
DWORD
LsaDirectoryDisconnect(
    LDAP *ldconn
    );


static
DWORD
LsaMachAcctCreate(
    LDAP *ld,
    const wchar16_t *machine_name,
    const wchar16_t *machacct_name,
    const wchar16_t *ou,
    int rejoin
    );


static
DWORD
LsaMachDnsNameSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    const wchar16_t *dns_domain_name,
    wchar16_t **samacct
    );


static
DWORD
LsaMachAcctSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    wchar16_t **dn
    );


static
DWORD
LsaMachAcctSetAttribute(
    LDAP *ldconn,
    const wchar16_t *dn,
    const wchar16_t *attr_name,
    const wchar16_t **attr_val,
    int new
    );


static
DWORD
LsaGetNtPasswordHash(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pNtHash,
    IN  DWORD   dwNtHashSize
    );


static
DWORD
LsaEncryptNtHashVerifier(
    IN  PBYTE    pNewNtHash,
    IN  DWORD    dwNewNtHashLen,
    IN  PBYTE    pOldNtHash,
    IN  DWORD    dwOldNtHashLen,
    OUT PBYTE    pNtVerifier,
    IN  DWORD    dwNtVerifierSize
    );


static
DWORD
LsaPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    );


DWORD
LsaJoinDomain(
    PCSTR pszHostname,
    PCSTR pszHostDnsDomain,
    PCSTR pszDomain,
    PCSTR pszOU,
    PCSTR pszUsername,
    PCSTR pszPassword,
    PCSTR pszOSName,
    PCSTR pszOSVersion,
    PCSTR pszOSServicePack,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;
    PSTR  pszOU_DN = NULL;
    PWSTR pwszHostname = NULL;
    PWSTR pwszHostDnsDomain = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszOU = NULL;
    PWSTR pwszOSName = NULL;
    PWSTR pwszOSVersion = NULL;
    PWSTR pwszOSServicePack = NULL;
    DWORD dwOptions = (LSAJOIN_JOIN_DOMAIN |
                       LSAJOIN_ACCT_CREATE |
                       LSAJOIN_DOMAIN_JOIN_IF_JOINED);
    PLSA_CREDS_FREE_INFO pAccessInfo = NULL;

    BAIL_ON_INVALID_STRING(pszHostname);
    BAIL_ON_INVALID_STRING(pszDomain);
    BAIL_ON_INVALID_STRING(pszUsername);

    if (geteuid() != 0) {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ( !(dwFlags & LSA_NET_JOIN_DOMAIN_NOTIMESYNC) )
    {
        dwError = LsaSyncTimeToDC(pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSetSMBCreds(
                pszDomain,
                pszUsername,
                pszPassword,
                TRUE,
                &pAccessInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(
                    pszHostname,
                    &pwszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszHostDnsDomain))
    {
        dwError = LwMbsToWc16s(
                        pszHostDnsDomain,
                        &pwszHostDnsDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwMbsToWc16s(
                    pszDomain,
                    &pwszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (!LW_IS_NULL_OR_EMPTY_STR(pszOU)) {

        dwError = LsaBuildOrgUnitDN(
                    pszDomain,
                    pszOU,
                    &pszOU_DN);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwMbsToWc16s(
                    pszOU_DN,
                    &pwszOU);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszOSName)) {
        dwError = LwMbsToWc16s(
                    pszOSName,
                    &pwszOSName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszOSVersion)) {
        dwError = LwMbsToWc16s(
                    pszOSVersion,
                    &pwszOSVersion);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!LW_IS_NULL_OR_EMPTY_STR(pszOSServicePack)) {
        dwError = LwMbsToWc16s(
                    pszOSServicePack,
                    &pwszOSServicePack);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaJoinDomainInternal(
            pwszHostname,
            pwszHostDnsDomain,
            pwszDomain,
            pwszOU,
            NULL,
            NULL,
            dwOptions,
            pwszOSName,
            pwszOSVersion,
            pwszOSServicePack);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaFreeSMBCreds(&pAccessInfo);

    LW_SAFE_FREE_STRING(pszOU_DN);
    LW_SAFE_FREE_MEMORY(pwszHostname);
    LW_SAFE_FREE_MEMORY(pwszHostDnsDomain);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszOU);
    LW_SAFE_FREE_MEMORY(pwszOSName);
    LW_SAFE_FREE_MEMORY(pwszOSVersion);
    LW_SAFE_FREE_MEMORY(pwszOSServicePack);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaBuildOrgUnitDN(
    PCSTR pszDomain,
    PCSTR pszOU,
    PSTR* ppszOU_DN
    )
{
    DWORD dwError = 0;
    PSTR  pszOuDN = NULL;
    // Do not free
    PSTR  pszOutputPos = NULL;
    PCSTR pszInputPos = NULL;
    PCSTR pszInputSectionEnd = NULL;
    size_t sOutputDnLen = 0;
    size_t sSectionLen = 0;
    DWORD nDomainParts = 0;

    BAIL_ON_INVALID_STRING(pszDomain);
    BAIL_ON_INVALID_STRING(pszOU);

    // Figure out the length required to write the OU DN
    pszInputPos = pszOU;

    // skip leading slashes
    sSectionLen = strspn(pszInputPos, "/");
    pszInputPos += sSectionLen;

    while ((sSectionLen = strcspn(pszInputPos, "/")) != 0) {
        sOutputDnLen += sizeof(LSA_JOIN_OU_PREFIX) - 1;
        sOutputDnLen += sSectionLen;
        // For the separating comma
        sOutputDnLen++;

        pszInputPos += sSectionLen;

        sSectionLen = strspn(pszInputPos, "/");
        pszInputPos += sSectionLen;
    }

    // Figure out the length required to write the Domain DN
    pszInputPos = pszDomain;
    while ((sSectionLen = strcspn(pszInputPos, ".")) != 0) {
        sOutputDnLen += sizeof(LSA_JOIN_DC_PREFIX) - 1;
        sOutputDnLen += sSectionLen;
        nDomainParts++;

        pszInputPos += sSectionLen;

        sSectionLen = strspn(pszInputPos, ".");
        pszInputPos += sSectionLen;
    }

    // Add in space for the separating commas
    if (nDomainParts > 1)
    {
        sOutputDnLen += nDomainParts - 1;
    }

    dwError = LwAllocateMemory(
                    sizeof(CHAR) * (sOutputDnLen + 1),
                    (PVOID*)&pszOuDN);
    BAIL_ON_LSA_ERROR(dwError);

    pszOutputPos = pszOuDN;
    // Iterate through pszOU backwards and write to pszOuDN forwards
    pszInputPos = pszOU + strlen(pszOU) - 1;

    while(TRUE)
    {
        // strip trailing slashes
        while (pszInputPos >= pszOU && *pszInputPos == '/')
        {
            pszInputPos--;
        }

        if (pszInputPos < pszOU)
        {
            break;
        }

        // Find the end of this section (so that we can copy it to
        // the output string in forward order).
        pszInputSectionEnd = pszInputPos;
        while (pszInputPos >= pszOU && *pszInputPos != '/')
        {
            pszInputPos--;
        }
        sSectionLen = pszInputSectionEnd - pszInputPos;

        // Only "Computers" as the first element is a CN.
        if ((pszOutputPos ==  pszOuDN) &&
            (sSectionLen == sizeof("Computers") - 1) &&
            !strncasecmp(pszInputPos + 1, "Computers", sizeof("Computers") - 1))
        {
            // Add CN=<name>,
            memcpy(pszOutputPos, LSA_JOIN_CN_PREFIX,
                    sizeof(LSA_JOIN_CN_PREFIX) - 1);
            pszOutputPos += sizeof(LSA_JOIN_CN_PREFIX) - 1;
        }
        else
        {
            // Add OU=<name>,
            memcpy(pszOutputPos, LSA_JOIN_OU_PREFIX,
                    sizeof(LSA_JOIN_OU_PREFIX) - 1);
            pszOutputPos += sizeof(LSA_JOIN_OU_PREFIX) - 1;
        }

        memcpy(pszOutputPos,
                pszInputPos + 1,
                sSectionLen);
        pszOutputPos += sSectionLen;

        *pszOutputPos++ = ',';
    }

    // Make sure to overwrite any initial "CN=Computers" as "OU=Computers".
    // Note that it is safe to always set "OU=" as the start of the DN
    // unless the DN so far is exacly "CN=Computers,".
    if (strcasecmp(pszOuDN, LSA_JOIN_CN_PREFIX "Computers,"))
    {
        memcpy(pszOuDN, LSA_JOIN_OU_PREFIX, sizeof(LSA_JOIN_OU_PREFIX) - 1);
    }

    // Read the domain name foward in sections and write it back out
    // forward.
    pszInputPos = pszDomain;
    while (TRUE)
    {
        sSectionLen = strcspn(pszInputPos, ".");

        memcpy(pszOutputPos,
                LSA_JOIN_DC_PREFIX,
                sizeof(LSA_JOIN_DC_PREFIX) - 1);
        pszOutputPos += sizeof(LSA_JOIN_DC_PREFIX) - 1;

        memcpy(pszOutputPos, pszInputPos, sSectionLen);
        pszOutputPos += sSectionLen;

        pszInputPos += sSectionLen;

        sSectionLen = strspn(pszInputPos, ".");
        pszInputPos += sSectionLen;

        if (*pszInputPos != 0)
        {
            // Add a comma for the next entry
            *pszOutputPos++ = ',';
        }
        else
            break;

    }

    assert(pszOutputPos == pszOuDN + sizeof(CHAR) * (sOutputDnLen));
    *pszOutputPos = 0;

    *ppszOU_DN = pszOuDN;

cleanup:

    return dwError;

error:

    *ppszOU_DN = NULL;

    LW_SAFE_FREE_STRING(pszOuDN);

    goto cleanup;
}

DWORD
LsaNetTestJoinDomain(
    PBOOLEAN pbIsJoined
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsJoined = FALSE;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PSTR pszHostname = NULL;

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwpsOpenPasswordStore(
                LWPS_PASSWORD_STORE_DEFAULT,
                &hStore);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwpsGetPasswordByHostName(
                hStore,
                pszHostname,
                &pPassInfo);

    switch(dwError)
    {
        case LWPS_ERROR_INVALID_ACCOUNT:
            bIsJoined = FALSE;
            dwError = 0;
            break;
        case 0:
            bIsJoined = TRUE;
            break;
        default:
            BAIL_ON_LSA_ERROR(dwError);
    }

    *pbIsJoined = bIsJoined;

cleanup:

    if (pPassInfo) {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }

    if (hStore != (HANDLE)NULL) {
        LwpsClosePasswordStore(hStore);
    }

    LW_SAFE_FREE_STRING(pszHostname);

    return dwError;

error:

    *pbIsJoined = FALSE;

    goto cleanup;
}


DWORD
LsaSyncTimeToDC(
    PCSTR  pszDomain
    )
{
    DWORD dwError = 0;
    LWNET_UNIX_TIME_T dcTime = 0;
    time_t ttDCTime = 0;

    dwError = LWNetGetDCTime(
                    pszDomain,
                    &dcTime);
    BAIL_ON_LSA_ERROR(dwError);

    ttDCTime = (time_t) dcTime;

    if (labs(ttDCTime - time(NULL)) > LSA_JOIN_MAX_ALLOWED_CLOCK_DRIFT_SECONDS) {
        dwError = LwSetSystemTime(ttDCTime);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaBuildMachineAccountInfo(
    PLWPS_PASSWORD_INFO pInfo,
    PLSA_MACHINE_ACCT_INFO* ppAcctInfo
    )
{
    DWORD dwError = 0;
    PLSA_MACHINE_ACCT_INFO pAcctInfo = NULL;

    dwError = LwAllocateMemory(
                    sizeof(LSA_MACHINE_ACCT_INFO),
                    (PVOID*)&pAcctInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pInfo->pwszDnsDomainName,
                    &pAcctInfo->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pInfo->pwszDomainName,
                    &pAcctInfo->pszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pInfo->pwszHostname,
                    &pAcctInfo->pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pInfo->pwszMachineAccount,
                    &pAcctInfo->pszMachineAccount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pInfo->pwszMachinePassword,
                    &pAcctInfo->pszMachinePassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToMbs(
                    pInfo->pwszSID,
                    &pAcctInfo->pszSID);
    BAIL_ON_LSA_ERROR(dwError);

    pAcctInfo->dwSchannelType = pInfo->dwSchannelType;
    pAcctInfo->last_change_time = pInfo->last_change_time;

    *ppAcctInfo = pAcctInfo;

cleanup:

    return dwError;

error:

    *ppAcctInfo = NULL;

    if (pAcctInfo) {
        LsaFreeMachineAccountInfo(pAcctInfo);
    }

    goto cleanup;
}

VOID
LsaFreeMachineAccountInfo(
    PLSA_MACHINE_ACCT_INFO pAcctInfo
    )
{
    LW_SAFE_FREE_STRING(pAcctInfo->pszDnsDomainName);
    LW_SAFE_FREE_STRING(pAcctInfo->pszDomainName);
    LW_SAFE_FREE_STRING(pAcctInfo->pszHostname);
    LW_SAFE_FREE_STRING(pAcctInfo->pszMachineAccount);
    LW_SAFE_FREE_STRING(pAcctInfo->pszMachinePassword);
    LW_SAFE_FREE_STRING(pAcctInfo->pszSID);
    LwFreeMemory(pAcctInfo);
}


#if !defined(MACHPASS_LEN)
#define MACHPASS_LEN  (16)
#endif


DWORD
LsaJoinDomainInternal(
    PWSTR  pwszHostname,
    PWSTR  pwszDnsDomain,
    PWSTR  pwszDomain,
    PWSTR  pwszAccountOu,
    PWSTR  pwszAccount,
    PWSTR  pwszPassword,
    DWORD  dwJoinFlags,
    PWSTR  pwszOsName,
    PWSTR  pwszOsVersion,
    PWSTR  pwszOsServicePack
    )
{
    const DWORD dwLsaAccess = LSA_ACCESS_LOOKUP_NAMES_SIDS |
                              LSA_ACCESS_VIEW_POLICY_INFO;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    LSA_BINDING hLsaBinding = NULL;
    POLICY_HANDLE hLsaPolicy = NULL;
    LsaPolicyInformation *pLsaPolicyInfo = NULL;
    PWSTR pwszMachineName = NULL;
    PWSTR pwszMachineAcctName = NULL;
    PWSTR pwszMachinePassword[MACHPASS_LEN+1] = {0};
    PWSTR pwszDomainName = NULL;
    PSID pDomainSid = NULL;
    PWSTR pwszDnsDomainName = NULL;
    PWSTR pwszDCName = NULL;
    LDAP *pLdap = NULL;
    PWSTR pwszMachineNameLc = NULL;    /* lower cased machine name */
    PWSTR pwszBaseDn = NULL;
    PWSTR pwszDn = NULL;
    PWSTR pwszDnsAttrName = NULL;
    PWSTR pwszDnsAttrVal[2] = {0};
    PWSTR pwszSpnAttrName = NULL;
    PWSTR pwszSpnAttrVal[3] = {0};
    PWSTR pwszDescAttrName = NULL;
    PWSTR pwszDescAttrVal[2] = {0};
    PWSTR pwszOSNameAttrName = NULL;
    PWSTR pwszOSNameAttrVal[2] = {0};
    PWSTR pwszOSVersionAttrName = NULL;
    PWSTR pwszOSVersionAttrVal[2] = {0};
    PWSTR pwszOSServicePackAttrName = NULL;
    PWSTR pwszOSServicePackAttrVal[2] = {0};
    PWSTR pwszSidStr = NULL;
    LW_PIO_CREDS pCreds = NULL;

    dwError = LwAllocateWc16String(&pwszMachineName,
                                   pwszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sToUpper(pwszMachineName);
    BAIL_ON_LSA_ERROR(dwError)

    dwError = LsaGetRwDcName(pwszDomain,
                             FALSE,
                             &pwszDCName);
    if (dwError)
    {
        dwError = LsaGetRwDcName(pwszDomain,
                                 TRUE,
                                 &pwszDCName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pwszAccount && pwszPassword)
    {
        ntStatus = LwIoCreatePlainCredsW(pwszAccount,
                                         pwszDomain,
                                         pwszPassword,
                                         &pCreds);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = LwIoGetActiveCreds(NULL,
                                      &pCreds);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LsaInitBindingDefault(&hLsaBinding,
                                     pwszDCName,
                                     pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaOpenPolicy2(hLsaBinding,
                              pwszDCName,
                              NULL,
                              dwLsaAccess,
                              &hLsaPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaQueryInfoPolicy2(hLsaBinding,
                                   hLsaPolicy,
                                   LSA_POLICY_INFO_DNS,
                                   &pLsaPolicyInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwAllocateWc16StringFromUnicodeString(
                                  &pwszDnsDomainName,
                                  &pLsaPolicyInfo->dns.dns_domain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetAccountName(
                             pwszMachineName,
                             pwszDCName,
                             pwszDnsDomain ? pwszDnsDomain : pwszDnsDomainName,
                             &pwszMachineAcctName);
    BAIL_ON_LSA_ERROR(dwError);

    /* If account_ou is specified pre-create disabled machine
       account object in given branch of directory. It will
       be reset afterwards by means of rpc calls */
    if (pwszAccountOu)
    {
        dwError = LsaDirectoryConnect(pwszDCName, &pLdap, &pwszBaseDn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaMachAcctCreate(pLdap, pwszMachineName, pwszMachineAcctName, pwszAccountOu,
                                    (dwJoinFlags & LSAJOIN_DOMAIN_JOIN_IF_JOINED));
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDirectoryDisconnect(pLdap);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaGenerateMachinePassword(
               (PWSTR)pwszMachinePassword,
               sizeof(pwszMachinePassword)/sizeof(pwszMachinePassword[0]));

    if (pwszMachinePassword[0] == '\0')
    {
        BAIL_ON_NT_STATUS(STATUS_INTERNAL_ERROR);
    }

    ntStatus = LsaCreateMachineAccount(pwszDCName,
                                       pCreds,
                                       pwszMachineAcctName,
                                       (PWSTR)pwszMachinePassword,
                                       dwJoinFlags,
                                       &pwszDomainName,
                                       &pDomainSid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RtlAllocateWC16StringFromSid(&pwszSidStr,
                                            pDomainSid);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LsaSaveMachinePassword(
              pwszMachineName,
              pwszMachineAcctName,
              pwszDnsDomain ? pwszDnsDomain : pwszDnsDomainName,
              pwszDomainName,
              pwszDnsDomainName,
              pwszDCName,
              pwszSidStr,
              (PWSTR)pwszMachinePassword);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Open connection to directory server if it's going to be needed
     */
    if (!(dwJoinFlags & LSAJOIN_DEFER_SPN_SET) ||
        pwszOsName || pwszOsVersion || pwszOsServicePack)
    {

        dwError = LsaDirectoryConnect(pwszDCName, &pLdap, &pwszBaseDn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaMachAcctSearch(pLdap, pwszMachineAcctName, pwszBaseDn, &pwszDn);
        BAIL_ON_LSA_ERROR(dwError);

        /*
         * Set SPN and dnsHostName attributes unless this part is to be deferred
         */
        if (!(dwJoinFlags & LSAJOIN_DEFER_SPN_SET))
        {
            PWSTR pwszDnsHostName = NULL;

            dwError = LwAllocateWc16String(&pwszMachineNameLc,
                                           pwszMachineName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwWc16sToLower(pwszMachineNameLc);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwMbsToWc16s("dNSHostName", &pwszDnsAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            pwszDnsAttrVal[0] = LdapAttrValDnsHostName(pwszMachineNameLc,
                                  pwszDnsDomain ? pwszDnsDomain : pwszDnsDomainName);
            pwszDnsAttrVal[1] = NULL;
            pwszDnsHostName = pwszDnsAttrVal[0];

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszDnsAttrName,
                                       (const wchar16_t**)pwszDnsAttrVal, 0);
            if (dwError == ERROR_DS_CONSTRAINT_VIOLATION)
            {
                dwError = ERROR_DS_NAME_ERROR_NO_MAPPING;
            }
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwMbsToWc16s("servicePrincipalName",
                                   &pwszSpnAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            pwszSpnAttrVal[0] = LdapAttrValSvcPrincipalName(pwszDnsHostName);
            pwszSpnAttrVal[1] = LdapAttrValSvcPrincipalName(pwszHostname);
            pwszSpnAttrVal[2] = NULL;

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn, pwszSpnAttrName,
                                              (const wchar16_t**)pwszSpnAttrVal, 0);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (wc16scmp(pwszMachineName, pwszMachineAcctName))
        {
            dwError = LwMbsToWc16s("description",
                                   &pwszDescAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateWc16String(&pwszDescAttrVal[0],
                                           pwszMachineName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(
                      pLdap,
                      pwszDn,
                      pwszDescAttrName,
                      (const wchar16_t**)pwszDescAttrVal,
                      0);
            BAIL_ON_LSA_ERROR(dwError);
        }

        /*
         * Set operating system name and version attributes if specified
         */
        if (pwszOsName)
        {
            dwError = LwMbsToWc16s("operatingSystem",
                                   &pwszOSNameAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateWc16String(&pwszOSNameAttrVal[0],
                                           pwszOsName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszOSNameAttrName,
                                       (const wchar16_t**)pwszOSNameAttrVal,
                                       0);
            if (dwError == ERROR_ACCESS_DENIED)
            {
                /* The user must be a non-admin. In this case, we cannot
                 * set the attribute.
                 */
                dwError = ERROR_SUCCESS;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        if (pwszOsVersion)
        {
            dwError = LwMbsToWc16s("operatingSystemVersion",
                                   &pwszOSVersionAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateWc16String(&pwszOSVersionAttrVal[0],
                                           pwszOsVersion);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszOSVersionAttrName,
                                       (const wchar16_t**)pwszOSVersionAttrVal,
                                       0);
            if (dwError == ERROR_ACCESS_DENIED)
            {
                dwError = ERROR_SUCCESS;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        if (pwszOsServicePack)
        {
            dwError = LwMbsToWc16s("operatingSystemServicePack",
                                   &pwszOSServicePackAttrName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateWc16String(&pwszOSServicePackAttrVal[0],
                                           pwszOsServicePack);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaMachAcctSetAttribute(pLdap, pwszDn,
                                       pwszOSServicePackAttrName,
                                       (const wchar16_t**)pwszOSServicePackAttrVal,
                                       0);
            if (dwError == ERROR_ACCESS_DENIED)
            {
                dwError = ERROR_SUCCESS;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);
            }
        }

        dwError = LsaDirectoryDisconnect(pLdap);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (hLsaBinding && hLsaPolicy)
    {
        LsaClose(hLsaBinding, hLsaPolicy);

        LsaFreeBinding(&hLsaBinding);
    }

    if (pLsaPolicyInfo)
    {
        LsaRpcFreeMemory(pLsaPolicyInfo);
    }

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    LW_SAFE_FREE_MEMORY(pwszDomainName);
    RTL_FREE(&pDomainSid);
    RTL_FREE(&pwszSidStr);
    LW_SAFE_FREE_MEMORY(pwszDnsDomainName);
    LW_SAFE_FREE_MEMORY(pwszMachineName);
    LW_SAFE_FREE_MEMORY(pwszMachineAcctName);
    LW_SAFE_FREE_MEMORY(pwszMachineNameLc);
    LW_SAFE_FREE_MEMORY(pwszBaseDn);
    LW_SAFE_FREE_MEMORY(pwszDn);
    LW_SAFE_FREE_MEMORY(pwszDnsAttrName);
    LW_SAFE_FREE_MEMORY(pwszDnsAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszSpnAttrName);
    LW_SAFE_FREE_MEMORY(pwszSpnAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszSpnAttrVal[1]);
    LW_SAFE_FREE_MEMORY(pwszDescAttrName);
    LW_SAFE_FREE_MEMORY(pwszDescAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszOSNameAttrName);
    LW_SAFE_FREE_MEMORY(pwszOSNameAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszOSVersionAttrName);
    LW_SAFE_FREE_MEMORY(pwszOSVersionAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszOSServicePackAttrName);
    LW_SAFE_FREE_MEMORY(pwszOSServicePackAttrVal[0]);
    LW_SAFE_FREE_MEMORY(pwszDCName);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaGetAccountName(
    const wchar16_t *machname,
    const wchar16_t *domain_controller_name,
    const wchar16_t *dns_domain_name,
    wchar16_t       **account_name
    )
{
    int err = ERROR_SUCCESS;
    LDAP *ld = NULL;
    wchar16_t *base_dn = NULL;
    wchar16_t *dn = NULL;
    wchar16_t *machname_lc = NULL;
    wchar16_t *samname = NULL;     /* short name valid for SAM account */
    wchar16_t *hashstr = NULL;
    wchar16_t *samacctname = NULL; /* account name (with trailing '$') */
    UINT32    hash = 0;
    UINT32    offset = 0;
    wchar16_t newname[16];
    wchar16_t searchname[17];
    size_t    hashstrlen = 0;
    size_t    samacctname_len = 0;

    memset(newname, 0, sizeof(newname));

    /* the host name is short enough to use as is */
    if (wc16slen(machname) < 16)
    {
        err = LwAllocateWc16String(&samname, machname);
        BAIL_ON_LSA_ERROR(err);
    }

    /* look for an existing account using the dns_host_name attribute */
    if (!samname)
    {
        err = LwAllocateWc16String(&machname_lc, machname);
        BAIL_ON_LSA_ERROR(err);

        wc16slower(machname_lc);

        err = LsaDirectoryConnect(domain_controller_name, &ld, &base_dn);
        BAIL_ON_LSA_ERROR(err);

        err = LsaMachDnsNameSearch(ld, machname_lc, base_dn, dns_domain_name, &samname);
        if (err == ERROR_SUCCESS)
        {
            samname[wc16slen(samname) - 1] = 0;
        }
        else
        {
            err = ERROR_SUCCESS;
        }
    }

     /*
      * No account was found and the name is too long so hash the host
      * name and combine with as much of the existing name as will fit
      * in the available space.  Search for an existing account with
      * that name and if a collision is detected, increment the hash
      * and try again.
      */
    if (!samname)
    {
        hash = NetWc16sHash(machname_lc);

        for (offset = 0 ; offset < 100 ; offset++)
        {
            hashstr = NetHashToWc16s(hash + offset);
            if (hashstr == NULL)
            {
                err = ERROR_OUTOFMEMORY;
                BAIL_ON_LSA_ERROR(err);
            }
            hashstrlen = wc16slen(hashstr);

            wc16sncpy(newname, machname, 15 - hashstrlen);
            wc16sncpy(newname + 15 - hashstrlen, hashstr, hashstrlen);

            LW_SAFE_FREE_MEMORY(hashstr);

            if (sw16printfw(searchname,
                            sizeof(searchname)/sizeof(wchar16_t),
                            L"%ws$",
                            newname) < 0)
            {
                err = ErrnoToWin32Error(errno);
                BAIL_ON_LSA_ERROR(err);
            }

            err = LsaMachAcctSearch( ld, searchname, base_dn, &dn );
            if ( err != ERROR_SUCCESS )
            {
                err = ERROR_SUCCESS;

                err = LwAllocateWc16String(&samname, newname);
                BAIL_ON_LSA_ERROR(err);

                break;
            }
            LW_SAFE_FREE_MEMORY(dn);
        }
        if (offset == 10)
        {
            err = ERROR_DUP_NAME;
            goto error;
        }
    }

    err = LwWc16sLen(samname, &samacctname_len);
    BAIL_ON_LSA_ERROR(err);

    samacctname_len += 2;

    err = LwAllocateMemory(sizeof(samacctname[0]) * samacctname_len,
                           OUT_PPVOID(&samacctname));
    BAIL_ON_LSA_ERROR(err);

    if (sw16printfw(samacctname,
                    samacctname_len,
                    L"%ws$",
                    samname) < 0)
    {
        err = ErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(err);
    }

    *account_name = samacctname;

cleanup:
    if (ld)
    {
        LsaDirectoryDisconnect(ld);
    }

    LW_SAFE_FREE_MEMORY(machname_lc);
    LW_SAFE_FREE_MEMORY(hashstr);
    LW_SAFE_FREE_MEMORY(dn);
    LW_SAFE_FREE_MEMORY(samname);
    LW_SAFE_FREE_MEMORY(base_dn);

    return err;

error:
    LW_SAFE_FREE_MEMORY(samacctname);
    *account_name = NULL;

    goto cleanup;
}


static
UINT32
NetWc16sHash(
    const wchar16_t *str
    )
{
    DWORD  dwLen = 0;
    DWORD  dwPos = 0;
    UINT32 result = 0;
    char   *data = (char *)str;

    dwLen = wc16slen(str) * 2;

    for (dwPos = 0 ; dwPos < dwLen ; dwPos++)
    {
        if ( data[dwPos] )
        {
            // rotate result to the left 3 bits with wrap around
            result = (result << 3) | (result >> (sizeof(UINT32)*8 - 3));
            result += data[dwPos];
        }
    }

    return result;
}


static
wchar16_t *
NetHashToWc16s(
    UINT32    hash
    )
{
    char *pszValidChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    DWORD dwNumValid = strlen(pszValidChars);
    char pszHashStr[16];
    UINT32 hash_local = hash;
    DWORD dwPos = 0;
    DWORD new_char = 0;
    wchar16_t *result = NULL;

    memset(pszHashStr, 0, sizeof(pszHashStr));

    pszHashStr[dwPos++] = '-';

    while( hash_local )
    {
        new_char = hash_local % dwNumValid;
        pszHashStr[dwPos++] = pszValidChars[new_char];
        hash_local /= dwNumValid;
    }

    result = ambstowc16s(pszHashStr);

    return result;
}


static
VOID
LsaGenerateMachinePassword(
    PWSTR  pwszPassword,
    size_t sPasswordLen
    )
{
    const DWORD dwMaxGenerationAttempts = 1000;
    DWORD dwGenerationAttempts = 0;

    pwszPassword[0] = '\0';
    do
    {
        LsaGenerateRandomString(pwszPassword, sPasswordLen);

        dwGenerationAttempts++;

    } while (dwGenerationAttempts <= dwMaxGenerationAttempts &&
             LsaCharacterClassesInPassword(pwszPassword, sPasswordLen) < 3);

    if (!(dwGenerationAttempts <= dwMaxGenerationAttempts))
    {
        abort();
    }
}


static
const CHAR
RandomCharsSet[] = "abcdefghijklmnopqrstuvwxyz"
                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                   "-+/*,.;:!<=>%'&()0123456789";

static
VOID
LsaGenerateRandomString(
    PWSTR   pwszBuffer,
    size_t  sBufferLen
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PBYTE pBuffer = NULL;
    DWORD i = 0;

    dwError = LwAllocateMemory(sizeof(pBuffer[0]) * sBufferLen,
                               OUT_PPVOID(&pBuffer));
    BAIL_ON_LSA_ERROR(dwError);

    if (!RAND_bytes((unsigned char*)pBuffer, (int)sBufferLen))
    {
        dwError = ERROR_ENCRYPTION_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    for (i = 0; i < sBufferLen - 1; i++)
    {
        DWORD iChar = pBuffer[i] % (sizeof(RandomCharsSet) - 2);
        pwszBuffer[i] = (WCHAR)RandomCharsSet[iChar];
    }

    pwszBuffer[sBufferLen - 1] = (WCHAR)'\0';

cleanup:
    LW_SAFE_FREE_MEMORY(pBuffer);

    return;

error:
    memset(pwszBuffer, 0, sizeof(pwszBuffer[0] * sBufferLen));

    goto cleanup;
}


static
DWORD
LsaCharacterClassesInPassword(
    const wchar16_t* password,
    size_t len
    )
{
    DWORD dwClassesSeen = 0;
    BOOLEAN bHasUpperCase = FALSE;
    BOOLEAN bHasLowerCase = FALSE;
    BOOLEAN bHasDigit = FALSE;
    BOOLEAN bHasNonAlphaNumeric = FALSE;
    size_t i = 0;

    for (i = 0; i < len; i++)
    {
        if ('A' <= password[i] && password[i] <= 'Z')
        {
            bHasUpperCase = TRUE;
        }
        else if ('a' <= password[i] && password[i] <= 'z')
        {
            bHasLowerCase = TRUE;
        }
        else if ('0' <= password[i] && password[i] <= '9')
        {
            bHasDigit = TRUE;
        }
        else if (strchr( "-+/*,.;:!<=>%'&()", password[i]) != NULL)
        {
            // This may be a better list to check against:
            //       `~!@#$%^&*()_+-={}|[]\:";'<>?,./
            bHasNonAlphaNumeric = TRUE;
        }
    }
    if (bHasUpperCase)
        dwClassesSeen++;
    if (bHasLowerCase)
        dwClassesSeen++;
    if (bHasDigit)
        dwClassesSeen++;
    if (bHasNonAlphaNumeric)
        dwClassesSeen++;

    return dwClassesSeen;
}


static
NTSTATUS
LsaCreateMachineAccount(
    PWSTR          pwszDCName,
    LW_PIO_CREDS   pCreds,
    PWSTR          pwszMachineAccountName,
    PWSTR          pwszMachinePassword,
    DWORD          dwJoinFlags,
    PWSTR         *ppwszDomainName,
    PSID          *ppDomainSid
    )
{
    const DWORD dwConnAccess = SAMR_ACCESS_OPEN_DOMAIN |
                               SAMR_ACCESS_ENUM_DOMAINS;

    const DWORD dwDomainAccess = DOMAIN_ACCESS_ENUM_ACCOUNTS |
                                 DOMAIN_ACCESS_OPEN_ACCOUNT |
                                 DOMAIN_ACCESS_LOOKUP_INFO_2 |
                                 DOMAIN_ACCESS_CREATE_USER;

    const DWORD dwUserAccess = USER_ACCESS_GET_ATTRIBUTES |
                               USER_ACCESS_SET_ATTRIBUTES |
                               USER_ACCESS_SET_PASSWORD;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    unsigned32 rpcStatus = 0;
    SAMR_BINDING hSamrBinding = NULL;
    CONNECT_HANDLE hConnect = NULL;
    rpc_transport_info_handle_t hTransportInfo = NULL;
    DWORD dwProtSeq = 0;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLen = 0;
    unsigned16 sessionKeyLen = 0;
    PSID pBuiltinSid = NULL;
    DWORD dwResume = 0;
    DWORD dwSize = 256;
    PWSTR *ppwszDomainNames = NULL;
    DWORD i = 0;
    PWSTR pwszDomainName = NULL;
    DWORD dwNumEntries = 0;
    PSID pSid = NULL;
    PSID pDomainSid = NULL;
    DOMAIN_HANDLE hDomain = NULL;
    BOOLEAN bNewAccount = FALSE;
    PDWORD pdwRids = NULL;
    PDWORD pdwTypes = NULL;
    ACCOUNT_HANDLE hUser = NULL;
    DWORD dwUserAccessGranted = 0;
    DWORD dwRid = 0;
    DWORD dwLevel = 0;
    UserInfo *pInfo = NULL;
    DWORD dwFlagsEnable = 0;
    DWORD dwFlagsDisable = 0;
    UserInfo Info;
    size_t sMachinePasswordLen = 0;
    UserInfo PassInfo;
    PWSTR pwszMachineName = NULL;
    PUNICODE_STRING pFullName = NULL;

    memset(&Info, 0, sizeof(Info));
    memset(&PassInfo, 0, sizeof(PassInfo));

    ntStatus = SamrInitBindingDefault(&hSamrBinding,
                                      pwszDCName,
                                      pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SamrConnect2(hSamrBinding,
                            pwszDCName,
                            dwConnAccess,
                            &hConnect);
    BAIL_ON_NT_STATUS(ntStatus);

    rpc_binding_inq_transport_info(hSamrBinding,
                                   &hTransportInfo,
                                   &rpcStatus);
    if (rpcStatus)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    rpc_binding_inq_prot_seq(hSamrBinding,
                             (unsigned32*)&dwProtSeq,
                             &rpcStatus);
    if (rpcStatus)
    {
        ntStatus = LwRpcStatusToNtStatus(rpcStatus);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (dwProtSeq == rpc_c_protseq_id_ncacn_np)
    {
        rpc_smb_transport_info_inq_session_key(
                                   hTransportInfo,
                                   (unsigned char**)&pSessionKey,
                                   &sessionKeyLen);
        dwSessionKeyLen = (DWORD)sessionKeyLen;
    }
    else
    {
        ntStatus = STATUS_INVALID_CONNECTION;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    dwError = LwCreateWellKnownSid(WinBuiltinDomainSid,
                                   NULL,
                                   &pBuiltinSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        ntStatus = SamrEnumDomains(hSamrBinding,
                                   hConnect,
                                   &dwResume,
                                   dwSize,
                                   &ppwszDomainNames,
                                   &dwNumEntries);
        BAIL_ON_NT_STATUS(ntStatus);

        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != STATUS_MORE_ENTRIES)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }

        for (i = 0; pDomainSid == NULL && i < dwNumEntries; i++)
        {
            ntStatus = SamrLookupDomain(hSamrBinding,
                                        hConnect,
                                        ppwszDomainNames[i],
                                        &pSid);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!RtlEqualSid(pSid, pBuiltinSid))
            {
                dwError = LwAllocateWc16String(&pwszDomainName,
                                               ppwszDomainNames[i]);
                BAIL_ON_LSA_ERROR(dwError);

                ntStatus = RtlDuplicateSid(&pDomainSid, pSid);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            if (pSid)
            {
                SamrFreeMemory(pSid);
                pSid = NULL;
            }
        }

        if (ppwszDomainNames)
        {
            SamrFreeMemory(ppwszDomainNames);
            ppwszDomainNames = NULL;
        }
    }
    while (ntStatus == STATUS_MORE_ENTRIES);

    ntStatus = SamrOpenDomain(hSamrBinding,
                              hConnect,
                              dwDomainAccess,
                              pDomainSid,
                              &hDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    /* for start, let's assume the account already exists */
    bNewAccount = FALSE;

    ntStatus = SamrLookupNames(hSamrBinding,
                               hDomain,
                               1,
                               &pwszMachineAccountName,
                               &pdwRids,
                               &pdwTypes,
                               NULL);
    if (ntStatus == STATUS_NONE_MAPPED)
    {
        if (!(dwJoinFlags & LSAJOIN_ACCT_CREATE)) goto error;

        ntStatus = SamrCreateUser2(hSamrBinding,
                                   hDomain,
                                   pwszMachineAccountName,
                                   ACB_WSTRUST,
                                   dwUserAccess,
                                   &hUser,
                                   &dwUserAccessGranted,
                                   &dwRid);
        BAIL_ON_NT_STATUS(ntStatus);

        bNewAccount = TRUE;

    }
    else if (ntStatus == STATUS_SUCCESS &&
             !(dwJoinFlags & LSAJOIN_DOMAIN_JOIN_IF_JOINED))
    {
        BAIL_ON_LSA_ERROR(NERR_SetupAlreadyJoined);
    }
    else
    {
        ntStatus = SamrOpenUser(hSamrBinding,
                                hDomain,
                                dwUserAccess,
                                pdwRids[0],
                                &hUser);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /*
     * Flip ACB_DISABLED flag - this way password timeout counter
     * gets restarted
     */

    dwLevel = 16;

    ntStatus = SamrQueryUserInfo(hSamrBinding,
                                 hUser,
                                 dwLevel,
                                 &pInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    dwFlagsEnable = pInfo->info16.account_flags & (~ACB_DISABLED);
    dwFlagsDisable = pInfo->info16.account_flags | ACB_DISABLED;

    Info.info16.account_flags = dwFlagsEnable;
    ntStatus = SamrSetUserInfo2(hSamrBinding,
                                hUser,
                                dwLevel,
                                &Info);
    BAIL_ON_NT_STATUS(ntStatus);

    Info.info16.account_flags = dwFlagsDisable;
    ntStatus = SamrSetUserInfo2(hSamrBinding,
                                hUser,
                                dwLevel,
                                &Info);
    BAIL_ON_NT_STATUS(ntStatus);

    Info.info16.account_flags = dwFlagsEnable;
    ntStatus = SamrSetUserInfo2(hSamrBinding,
                                hUser,
                                dwLevel,
                                &Info);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwWc16sLen(pwszMachinePassword,
                         &sMachinePasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    if (bNewAccount)
    {
        UserInfo25 *pInfo25 = &PassInfo.info25;

        ntStatus = LsaEncryptPasswordBufferEx(pInfo25->password.data,
                                              sizeof(pInfo25->password.data),
                                              pwszMachinePassword,
                                              sMachinePasswordLen,
                                              pSessionKey,
                                              dwSessionKeyLen);
        BAIL_ON_NT_STATUS(ntStatus);

        dwError = LwAllocateWc16String(&pwszMachineName,
                                       pwszMachineAccountName);
        BAIL_ON_LSA_ERROR(dwError);

        pwszMachineName[sMachinePasswordLen - 1] = '\0';

        pInfo25->info.account_flags = ACB_WSTRUST;

        pFullName = &pInfo25->info.full_name;
        dwError = LwAllocateUnicodeStringFromWc16String(
                                      pFullName,
                                      pwszMachineName);
        BAIL_ON_LSA_ERROR(dwError);

        pInfo25->info.fields_present = SAMR_FIELD_FULL_NAME |
                                       SAMR_FIELD_ACCT_FLAGS |
                                       SAMR_FIELD_PASSWORD;
        dwLevel = 25;
    }
    else
    {
        UserInfo26 *pInfo26 = &PassInfo.info26;

        ntStatus = LsaEncryptPasswordBufferEx(pInfo26->password.data,
                                              sizeof(pInfo26->password.data),
                                              pwszMachinePassword,
                                              sMachinePasswordLen,
                                              pSessionKey,
                                              dwSessionKeyLen);
        BAIL_ON_NT_STATUS(ntStatus);

        pInfo26->password_len = sMachinePasswordLen;

        dwLevel = 26;
    }

    ntStatus = SamrSetUserInfo2(hSamrBinding,
                                hUser,
                                dwLevel,
                                &PassInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszDomainName = pwszDomainName;
    *ppDomainSid     = pDomainSid;

cleanup:
    if (hSamrBinding && hUser)
    {
        SamrClose(hSamrBinding, hUser);
    }

    if (hSamrBinding && hDomain)
    {
        SamrClose(hSamrBinding, hDomain);
    }

    if (hSamrBinding && hConnect)
    {
        SamrClose(hSamrBinding, hConnect);
    }

    if (hSamrBinding)
    {
        SamrFreeBinding(&hSamrBinding);
    }

    if (pFullName)
    {
        LwFreeUnicodeString(pFullName);
    }

    if (pInfo)
    {
        SamrFreeMemory(pInfo);
    }

    if (pdwRids)
    {
        SamrFreeMemory(pdwRids);
    }

    if (pdwTypes)
    {
        SamrFreeMemory(pdwTypes);
    }

    if (ppwszDomainNames)
    {
        SamrFreeMemory(ppwszDomainNames);
    }

    LW_SAFE_FREE_MEMORY(pBuiltinSid);

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    LW_SAFE_FREE_MEMORY(pwszDomainName);
    RTL_FREE(&pDomainSid);

    *ppwszDomainName = NULL;
    *ppDomainSid     = NULL;

    goto cleanup;
}


static
NTSTATUS
LsaEncryptPasswordBufferEx(
    PBYTE  pPasswordBuffer,
    DWORD  dwPasswordBufferSize,
    PWSTR  pwszPassword,
    DWORD  dwPasswordLen,
    PBYTE  pSessionKey,
    DWORD  dwSessionKeyLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    MD5_CTX ctx;
    RC4_KEY rc4_key;
    BYTE InitValue[16] = {0};
    BYTE DigestedSessKey[16] = {0};
    BYTE PasswordBuffer[532] = {0};

    BAIL_ON_INVALID_POINTER(pPasswordBuffer);
    BAIL_ON_INVALID_POINTER(pwszPassword);
    BAIL_ON_INVALID_POINTER(pSessionKey);

    if (dwPasswordBufferSize < sizeof(PasswordBuffer))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memset(&ctx, 0, sizeof(ctx));
    memset(&rc4_key, 0, sizeof(rc4_key));

    ntStatus = LsaEncodePasswordBuffer(pwszPassword,
                                       PasswordBuffer,
                                       sizeof(PasswordBuffer));
    BAIL_ON_LSA_ERROR(dwError);

    if (!RAND_bytes((unsigned char*)InitValue, sizeof(InitValue)))
    {
        dwError = ERROR_ENCRYPTION_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    MD5_Init(&ctx);
    MD5_Update(&ctx, InitValue, 16);
    MD5_Update(&ctx, pSessionKey, dwSessionKeyLen);
    MD5_Final(DigestedSessKey, &ctx);

    RC4_set_key(&rc4_key, 16, (unsigned char*)DigestedSessKey);
    RC4(&rc4_key, 516, PasswordBuffer, PasswordBuffer);

    memcpy((PVOID)&PasswordBuffer[516], InitValue, 16);

    memcpy(pPasswordBuffer, PasswordBuffer, sizeof(PasswordBuffer));

cleanup:
    memset(PasswordBuffer, 0, sizeof(PasswordBuffer));

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaEncodePasswordBuffer(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pBlob,
    IN  DWORD   dwBlobSize
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    DWORD dwPasswordSize = 0;
    PWSTR pwszPasswordLE = NULL;
    BYTE PasswordBlob[516] = {0};
    BYTE BlobInit[512] = {0};
    DWORD iByte = 0;

    BAIL_ON_INVALID_POINTER(pwszPassword);
    BAIL_ON_INVALID_POINTER(pBlob);

    if (dwBlobSize < sizeof(PasswordBlob))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Sanity check - password cannot be longer than the buffer size
     */
    if ((sPasswordLen * sizeof(pwszPassword[0])) >
        (sizeof(PasswordBlob) - sizeof(dwPasswordSize)))
    {
        dwError = ERROR_INVALID_PASSWORD;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* size doesn't include terminating zero here */
    dwPasswordSize = sPasswordLen * sizeof(pwszPassword[0]);

    /*
     * Make sure encoded password is 2-byte little-endian
     */
    dwError = LwAllocateMemory(dwPasswordSize + sizeof(pwszPassword[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_LSA_ERROR(dwError);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);

    /*
     * Encode the password length (in bytes) in the last 4 bytes
     * as little-endian number
     */
    iByte = sizeof(PasswordBlob);
    PasswordBlob[--iByte] = (BYTE)((dwPasswordSize >> 24) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((dwPasswordSize >> 16) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((dwPasswordSize >> 8) & 0xff);
    PasswordBlob[--iByte] = (BYTE)((dwPasswordSize) & 0xff);

    /*
     * Copy the password and the initial random bytes
     */
    iByte -= dwPasswordSize;
    memcpy(&(PasswordBlob[iByte]), pwszPasswordLE, dwPasswordSize);

    /*
     * Fill the rest of the buffer with (pseudo) random mess
     * to increase security.
     */
    if (!RAND_bytes((unsigned char*)BlobInit, iByte))
    {
        dwError = ERROR_ENCRYPTION_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memcpy(PasswordBlob, BlobInit, iByte);

    memcpy(pBlob, PasswordBlob, sizeof(PasswordBlob));

cleanup:
    memset(PasswordBlob, 0, sizeof(PasswordBlob));

    if (pwszPasswordLE)
    {
        memset(pwszPasswordLE, 0, dwPasswordSize);
        LW_SAFE_FREE_MEMORY(pwszPasswordLE);
    }

    if (ntStatus == STATUS_SUCCESS &&
        dwError != ERROR_SUCCESS)
    {
        ntStatus = LwWin32ErrorToNtStatus(dwError);
    }

    return ntStatus;

error:
    if (pBlob)
    {
        memset(pBlob, 0, dwBlobSize);
    }

    goto cleanup;
}


static
DWORD
LsaSaveMachinePassword(
    PCWSTR  pwszMachineName,
    PCWSTR  pwszMachineAccountName,
    PCWSTR  pwszMachineDnsDomain,
    PCWSTR  pwszDomainName,
    PCWSTR  pwszDnsDomainName,
    PCWSTR  pwszDCName,
    PCWSTR  pwszSidStr,
    PCWSTR  pwszPassword
    )
{
    wchar_t wszHostFqdnFmt[] = L"host/%ws.%ws";
    wchar_t wszHostFmt[] = L"host/%ws";
    wchar_t wszCifsFqdnFmt[] = L"cifs/%ws.%ws";

    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszAccount = NULL;
    PWSTR pwszDomain = NULL;
    PWSTR pwszAdDnsDomainNameLc = NULL;
    PWSTR pwszAdDnsDomainNameUc = NULL;
    PWSTR pwszMachDnsDomainNameUc = NULL;
    PWSTR pwszMachDnsDomainNameLc = NULL;
    size_t sMachDnsDomainNameLcLen = 0;
    PWSTR pwszSid = NULL;
    PWSTR pwszHostnameUc = NULL;
    PWSTR pwszHostnameLc = NULL;
    size_t sHostnameLcLen = 0;
    PWSTR pwszPass = NULL;
    LWPS_PASSWORD_INFO pi = {0};
    size_t sPassLen = 0;
    DWORD dwKvno = 0;
    PWSTR pwszBaseDn = NULL;
    PWSTR pwszSalt = NULL;
    /* various forms of principal name for keytab */
    PWSTR pwszHostMachineUc = NULL;
    PWSTR pwszHostMachineLc = NULL;
    PWSTR pwszHostMachineFqdn = NULL;
    size_t sHostMachineFqdnSize = 0;
    PWSTR pwszCifsMachineFqdn = NULL;
    size_t sCifsMachineFqdnSize = 0;
    PWSTR pwszPrincipal = NULL;

    dwError = LwAllocateWc16String(&pwszAccount,
                                   pwszMachineAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszDomain,
                                   pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszAdDnsDomainNameLc,
                                   pwszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToLower(pwszAdDnsDomainNameLc);

    dwError = LwAllocateWc16String(&pwszAdDnsDomainNameUc,
                                   pwszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pwszAdDnsDomainNameUc);

    dwError = LwAllocateWc16String(&pwszMachDnsDomainNameUc,
                                   pwszMachineDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pwszMachDnsDomainNameUc);

    dwError = LwAllocateWc16String(&pwszMachDnsDomainNameLc,
                                   pwszMachineDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToLower(pwszMachDnsDomainNameLc);

    dwError = LwAllocateWc16String(&pwszSid,
                                   pwszSidStr);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pwszHostnameUc,
                                   pwszMachineName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToUpper(pwszHostnameUc);

    dwError = LwAllocateWc16String(&pwszHostnameLc,
                                   pwszMachineName);
    BAIL_ON_LSA_ERROR(dwError);

    LwWc16sToLower(pwszHostnameLc);

    dwError = LwAllocateWc16String(&pwszPass,
                                   pwszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Store the machine password first
     */

    pi.pwszDomainName      = pwszDomain;
    pi.pwszDnsDomainName   = pwszAdDnsDomainNameLc;
    pi.pwszSID             = pwszSid;
    pi.pwszHostname        = pwszHostnameUc;
    pi.pwszHostDnsDomain   = pwszMachDnsDomainNameLc;
    pi.pwszMachineAccount  = pwszAccount;
    pi.pwszMachinePassword = pwszPass;
    pi.last_change_time    = time(NULL);
    pi.dwSchannelType      = SCHANNEL_WKSTA;

    ntStatus = LwpsWritePasswordToAllStores(&pi);
    if (ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszPass, &sPassLen);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Find the current key version number for machine account
     */

    dwError = KtKrb5FormatPrincipalW(pwszAccount,
                                     pwszAdDnsDomainNameUc,
                                     &pwszPrincipal);
    BAIL_ON_LSA_ERROR(dwError);

    /* Get the directory base naming context first */
    dwError = KtLdapGetBaseDnW(pwszDCName, &pwszBaseDn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = KtLdapGetKeyVersionW(pwszDCName,
                                   pwszBaseDn,
                                   pwszPrincipal,
                                   &dwKvno);
    if (dwError == ERROR_FILE_NOT_FOUND)
    {
        /*
         * This is probably win2k DC we're talking to, because it doesn't
         * store kvno in directory. In such case return default key version
         */
        dwKvno = 0;
        dwError = ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = KtKrb5GetSaltingPrincipalW(pwszMachineName,
                                         pwszAccount,
                                         pwszMachineDnsDomain,
                                         pwszAdDnsDomainNameUc,
                                         pwszDCName,
                                         pwszBaseDn,
                                         &pwszSalt);
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszSalt == NULL)
    {
        dwError = LwAllocateWc16String(&pwszSalt, pwszPrincipal);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /*
     * Update keytab records with various forms of machine principal
     */

    /*
     * MACHINE$@DOMAIN.NET
     */
    dwError = LsaSavePrincipalKey(pwszAccount,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * host/MACHINE@DOMAIN.NET
     */

    dwError = LwAllocateWc16sPrintfW(&pwszHostMachineUc,
                                     wszHostFmt,
                                     pwszHostnameUc);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSavePrincipalKey(pwszHostMachineUc,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszHostnameLc,
                         &sHostnameLcLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszMachDnsDomainNameLc,
                         &sMachDnsDomainNameLcLen);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * host/machine.domain.net@DOMAIN.NET
     */
    sHostMachineFqdnSize = sHostnameLcLen + sMachDnsDomainNameLcLen +
                           (sizeof(wszHostFqdnFmt)/sizeof(wszHostFqdnFmt[0]));

    dwError = LwAllocateMemory(
                        sizeof(pwszHostMachineFqdn[0]) * sHostMachineFqdnSize,
                        OUT_PPVOID(&pwszHostMachineFqdn));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(
                pwszHostMachineFqdn,
                sHostMachineFqdnSize,
                wszHostFqdnFmt,
                pwszHostnameLc,
                pwszMachDnsDomainNameLc) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSavePrincipalKey(pwszHostMachineFqdn,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * host/MACHINE.DOMAIN.NET@DOMAIN.NET
     */
    if (sw16printfw(
                pwszHostMachineFqdn,
                sHostMachineFqdnSize,
                wszHostFqdnFmt,
                pwszHostnameUc,
                pwszMachDnsDomainNameUc) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSavePrincipalKey(pwszHostMachineFqdn,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * host/MACHINE.domain.net@DOMAIN.NET
     */
    if (sw16printfw(
                pwszHostMachineFqdn,
                sHostMachineFqdnSize,
                wszHostFqdnFmt,
                pwszHostnameUc,
                pwszMachDnsDomainNameLc) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSavePrincipalKey(pwszHostMachineFqdn,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * host/machine.DOMAIN.NET@DOMAIN.NET
     */
    if (sw16printfw(
                pwszHostMachineFqdn,
                sHostMachineFqdnSize,
                wszHostFqdnFmt,
                pwszHostnameLc,
                pwszMachDnsDomainNameUc) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSavePrincipalKey(pwszHostMachineFqdn,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * host/machine@DOMAIN.NET
     */
    dwError = LwAllocateWc16sPrintfW(&pwszHostMachineLc,
                                     wszHostFmt,
                                     pwszHostnameLc);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSavePrincipalKey(pwszHostMachineLc,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    sCifsMachineFqdnSize = sHostnameLcLen + sMachDnsDomainNameLcLen +
                           (sizeof(wszCifsFqdnFmt)/sizeof(wszCifsFqdnFmt[0]));

    /*
     * cifs/machine.domain.net@DOMAIN.NET
     */
    dwError = LwAllocateMemory(
                       sizeof(pwszCifsMachineFqdn[0]) * sCifsMachineFqdnSize,
                       OUT_PPVOID(&pwszCifsMachineFqdn));
    BAIL_ON_LSA_ERROR(dwError);

    if (sw16printfw(
                pwszCifsMachineFqdn,
                sCifsMachineFqdnSize,
                wszCifsFqdnFmt,
                pwszHostnameLc,
                pwszMachDnsDomainNameLc) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSavePrincipalKey(pwszCifsMachineFqdn,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * cifs/MACHINE.DOMAIN.NET@DOMAIN.NET
     */
    if (sw16printfw(
                pwszCifsMachineFqdn,
                sCifsMachineFqdnSize,
                wszCifsFqdnFmt,
                pwszHostnameUc,
                pwszMachDnsDomainNameUc) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSavePrincipalKey(pwszCifsMachineFqdn,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * cifs/MACHINE.domain.net@DOMAIN.NET
     */
    if (sw16printfw(
                pwszCifsMachineFqdn,
                sCifsMachineFqdnSize,
                wszCifsFqdnFmt,
                pwszHostnameUc,
                pwszMachDnsDomainNameLc) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSavePrincipalKey(pwszCifsMachineFqdn,
                                  pwszPass,
                                  sPassLen,
                                  pwszAdDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * cifs/machine.DOMAIN.NET@DOMAIN.NET
     */
    if (sw16printfw(
                pwszCifsMachineFqdn,
                sCifsMachineFqdnSize,
                wszCifsFqdnFmt,
                pwszHostnameLc,
                pwszMachDnsDomainNameUc) < 0)
    {
        dwError = LwErrnoToWin32Error(errno);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSavePrincipalKey(pwszCifsMachineFqdn,
                                  pwszPass,
                                  sPassLen,
                                  pwszMachDnsDomainNameUc,
                                  pwszSalt,
                                  pwszDCName,
                                  dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszBaseDn);
    LW_SAFE_FREE_MEMORY(pwszSalt);
    LW_SAFE_FREE_MEMORY(pwszDomain);
    LW_SAFE_FREE_MEMORY(pwszAdDnsDomainNameLc);
    LW_SAFE_FREE_MEMORY(pwszAdDnsDomainNameUc);
    LW_SAFE_FREE_MEMORY(pwszMachDnsDomainNameLc);
    LW_SAFE_FREE_MEMORY(pwszMachDnsDomainNameUc);
    LW_SAFE_FREE_MEMORY(pwszSid);
    LW_SAFE_FREE_MEMORY(pwszHostnameLc);
    LW_SAFE_FREE_MEMORY(pwszHostnameUc);
    LW_SAFE_FREE_MEMORY(pwszPass);
    LW_SAFE_FREE_MEMORY(pwszAccount);
    LW_SAFE_FREE_MEMORY(pwszHostMachineUc);
    LW_SAFE_FREE_MEMORY(pwszHostMachineLc);
    LW_SAFE_FREE_MEMORY(pwszHostMachineFqdn);
    LW_SAFE_FREE_MEMORY(pwszCifsMachineFqdn);
    LW_SAFE_FREE_MEMORY(pwszPrincipal);

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaSavePrincipalKey(
    PCWSTR  pwszName,
    PCWSTR  pwszPassword,
    DWORD   dwPasswordLen,
    PCWSTR  pwszRealm,
    PCWSTR  pwszSalt,
    PCWSTR  pwszDCName,
    DWORD   dwKvno
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PWSTR pwszPrincipal = NULL;

    BAIL_ON_INVALID_POINTER(pwszName);
    BAIL_ON_INVALID_POINTER(pwszPassword);
    BAIL_ON_INVALID_POINTER(pwszDCName);

    dwError = KtKrb5FormatPrincipalW(pwszName,
                                     pwszRealm,
                                     &pwszPrincipal);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = KtKrb5AddKeyW(pwszPrincipal,
                            (PVOID)pwszPassword,
                            dwPasswordLen,
                            NULL,
                            pwszSalt,
                            pwszDCName,
                            dwKvno);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszPrincipal);

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaDirectoryConnect(
    const wchar16_t *domain,
    LDAP **ldconn,
    wchar16_t **dn_context
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    int close_lderr = LDAP_SUCCESS;
    LDAP *ld = NULL;
    LDAPMessage *info = NULL;
    LDAPMessage *res = NULL;
    wchar16_t *dn_context_name = NULL;
    wchar16_t **dn_context_val = NULL;

    BAIL_ON_INVALID_POINTER(domain);
    BAIL_ON_INVALID_POINTER(ldconn);
    BAIL_ON_INVALID_POINTER(dn_context);

    *ldconn     = NULL;
    *dn_context = NULL;

    lderr = LdapInitConnection(&ld, domain, GSS_C_INTEG_FLAG);
    BAIL_ON_LDAP_ERROR(lderr);

    lderr = LdapGetDirectoryInfo(&info, &res, ld);
    BAIL_ON_LDAP_ERROR(lderr);

    dwError = LwMbsToWc16s("defaultNamingContext",
                           &dn_context_name);
    BAIL_ON_LSA_ERROR(dwError);

    dn_context_val = LdapAttributeGet(ld, info, dn_context_name, NULL);
    if (dn_context_val == NULL) {
        /* TODO: find more descriptive error code */
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        BAIL_ON_LDAP_ERROR(lderr);

    }

    dwError = LwAllocateWc16String(dn_context,dn_context_val[0]);
    BAIL_ON_LSA_ERROR(dwError);

    *ldconn = ld;

cleanup:
    LW_SAFE_FREE_MEMORY(dn_context_name);

    if (dn_context_val) {
        LdapAttributeValueFree(dn_context_val);
    }

    if (res) {
        LdapMessageFree(res);
    }

    if (dwError == ERROR_SUCCESS &&
        lderr != 0)
    {
        dwError = LdapErrToWin32Error(lderr);
    }

    return dwError;

error:
    if (ld) {
        close_lderr = LdapCloseConnection(ld);
        if (lderr == LDAP_SUCCESS &&
            close_lderr != STATUS_SUCCESS) {
            lderr = close_lderr;
        }
    }

    *dn_context = NULL;
    *ldconn     = NULL;
    goto cleanup;
}


static
DWORD
LsaDirectoryDisconnect(
    LDAP *ldconn
    )
{
    int lderr = LdapCloseConnection(ldconn);
    return LdapErrToWin32Error(lderr);
}


static
DWORD
LsaMachAcctCreate(
    LDAP *ld,
    const wchar16_t *machine_name,
    const wchar16_t *machacct_name,
    const wchar16_t *ou,
    int rejoin
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *machacct = NULL;
    LDAPMessage *res = NULL;
    LDAPMessage *info = NULL;
    wchar16_t *dn_context_name = NULL;
    wchar16_t **dn_context_val = NULL;
    wchar16_t *dn_name = NULL;
    wchar16_t **dn_val = NULL;

    BAIL_ON_INVALID_POINTER(ld);
    BAIL_ON_INVALID_POINTER(machine_name);
    BAIL_ON_INVALID_POINTER(machacct_name);
    BAIL_ON_INVALID_POINTER(ou);

    lderr = LdapMachAcctCreate(ld, machine_name, machacct_name, ou);
    if (lderr == LDAP_ALREADY_EXISTS && rejoin) {
        lderr = LdapGetDirectoryInfo(&info, &res, ld);
        BAIL_ON_LDAP_ERROR(lderr);

        dwError = LwMbsToWc16s("defaultNamingContext",
                               &dn_context_name);
        BAIL_ON_LSA_ERROR(dwError);

        dn_context_val = LdapAttributeGet(ld, info, dn_context_name, NULL);
        if (dn_context_val == NULL) {
            /* TODO: find more descriptive error code */
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            goto error;
        }

        lderr = LdapMachAcctSearch(&machacct, ld, machacct_name,
                                   dn_context_val[0]);
        BAIL_ON_LDAP_ERROR(lderr);

        dwError = LwMbsToWc16s("distinguishedName",
                               &dn_name);
        BAIL_ON_LSA_ERROR(dwError);

        dn_val = LdapAttributeGet(ld, machacct, dn_name, NULL);
        if (dn_val == NULL) {
            /* TODO: find more descriptive error code */
            lderr = LDAP_NO_SUCH_ATTRIBUTE;
            goto error;
        }

        lderr = LdapMachAcctMove(ld, dn_val[0], machine_name, ou);
        BAIL_ON_LDAP_ERROR(lderr);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(dn_context_name);
    LW_SAFE_FREE_MEMORY(dn_name);

    if (dn_context_val) {
        LdapAttributeValueFree(dn_context_val);
    }

    if (dn_val) {
        LdapAttributeValueFree(dn_val);
    }

    if (dwError == ERROR_SUCCESS &&
        lderr != 0)
    {
        dwError = LdapErrToWin32Error(lderr);
    }

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaMachDnsNameSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    const wchar16_t *dns_domain_name,
    wchar16_t **samacct
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL;
    wchar16_t *samacct_attr_name = NULL;
    wchar16_t **samacct_attr_val = NULL;

    BAIL_ON_INVALID_POINTER(ldconn);
    BAIL_ON_INVALID_POINTER(name);
    BAIL_ON_INVALID_POINTER(dn_context);
    BAIL_ON_INVALID_POINTER(dns_domain_name);
    BAIL_ON_INVALID_POINTER(samacct);

    *samacct = NULL;

    lderr = LdapMachDnsNameSearch(
                &res,
                ldconn,
                name,
                dns_domain_name,
                dn_context);
    BAIL_ON_LDAP_ERROR(lderr);

    dwError = LwMbsToWc16s("sAMAccountName",
                           &samacct_attr_name);
    BAIL_ON_LSA_ERROR(dwError);

    samacct_attr_val = LdapAttributeGet(ldconn, res, samacct_attr_name, NULL);
    if (!samacct_attr_val) {
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        BAIL_ON_LDAP_ERROR(lderr);
    }

    dwError = LwAllocateWc16String(samacct, samacct_attr_val[0]);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(samacct_attr_name);
    LdapAttributeValueFree(samacct_attr_val);

    if (res)
    {
        LdapMessageFree(res);
    }

    if (dwError == ERROR_SUCCESS &&
        lderr != 0)
    {
        dwError = LwLdapErrToWin32Error(lderr);
    }

    return dwError;

error:

    *samacct = NULL;
    goto cleanup;
}


static
DWORD
LsaMachAcctSearch(
    LDAP *ldconn,
    const wchar16_t *name,
    const wchar16_t *dn_context,
    wchar16_t **dn
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int lderr = LDAP_SUCCESS;
    LDAPMessage *res = NULL;
    wchar16_t *dn_attr_name = NULL;
    wchar16_t **dn_attr_val = NULL;

    BAIL_ON_INVALID_POINTER(ldconn);
    BAIL_ON_INVALID_POINTER(name);
    BAIL_ON_INVALID_POINTER(dn_context);
    BAIL_ON_INVALID_POINTER(dn);

    *dn = NULL;

    lderr = LdapMachAcctSearch(&res, ldconn, name, dn_context);
    BAIL_ON_LDAP_ERROR(lderr);

    dwError = LwMbsToWc16s("distinguishedName", &dn_attr_name);
    BAIL_ON_LSA_ERROR(dwError);

    dn_attr_val = LdapAttributeGet(ldconn, res, dn_attr_name, NULL);
    if (!dn_attr_val) {
        lderr = LDAP_NO_SUCH_ATTRIBUTE;
        BAIL_ON_LDAP_ERROR(lderr);
    }

    dwError = LwAllocateWc16String(dn, dn_attr_val[0]);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(dn_attr_name);
    LdapAttributeValueFree(dn_attr_val);

    if (res) {
        LdapMessageFree(res);
    }

    if (dwError == ERROR_SUCCESS &&
        lderr != 0)
    {
        dwError = LwLdapErrToWin32Error(lderr);
    }

    return dwError;

error:
    *dn = NULL;
    goto cleanup;
}


static
DWORD
LsaMachAcctSetAttribute(
    LDAP *ldconn,
    const wchar16_t *dn,
    const wchar16_t *attr_name,
    const wchar16_t **attr_val,
    int new
    )
{
    int lderr = LDAP_SUCCESS;

    lderr = LdapMachAcctSetAttribute(ldconn, dn, attr_name, attr_val, new);
    return LdapErrToWin32Error(lderr);
}


DWORD
LsaGetRwDcName(
    const wchar16_t *DnsDomainName,
    BOOLEAN Force,
    wchar16_t** DomainControllerName
    )
{
    DWORD dwError = 0;
    wchar16_t *domain_controller_name = NULL;
    char *dns_domain_name_mbs = NULL;
    DWORD get_dc_name_flags = DS_WRITABLE_REQUIRED;
    PLWNET_DC_INFO pDC = NULL;

    if (Force)
    {
        get_dc_name_flags |= DS_FORCE_REDISCOVERY;
    }

    dwError = LwWc16sToMbs(DnsDomainName, &dns_domain_name_mbs);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LWNetGetDCName(NULL, dns_domain_name_mbs, NULL, get_dc_name_flags, &pDC);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMbsToWc16s(pDC->pszDomainControllerName,
                           &domain_controller_name);
    BAIL_ON_LSA_ERROR(dwError)

cleanup:
    LW_SAFE_FREE_MEMORY(dns_domain_name_mbs);
    LWNET_SAFE_FREE_DC_INFO(pDC);
    if (dwError)
    {
        LW_SAFE_FREE_MEMORY(domain_controller_name);
    }

    *DomainControllerName = domain_controller_name;

    // ISSUE-2008/07/14-dalmeida -- Need to do error code conversion

    return dwError;

error:
    goto cleanup;
}


DWORD
LsaEnableDomainGroupMembership(
    PCSTR pszDomainName
    )
{
    return LsaChangeDomainGroupMembership(pszDomainName,
					  TRUE);
}


DWORD
LsaChangeDomainGroupMembership(
    IN  PCSTR    pszDomainName,
    IN  BOOLEAN  bEnable
    )
{
    DWORD dwError = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LSA_BINDING hLsaBinding = NULL;
    WCHAR wszLocalSystem[] = { '\\', '\\', '\0' };
    PWSTR pwszSystem = wszLocalSystem;
    DWORD dwLocalPolicyAccessMask = LSA_ACCESS_VIEW_POLICY_INFO;
    POLICY_HANDLE hLocalPolicy = NULL;
    LsaPolicyInformation *pInfo = NULL;
    PSID pDomainSid = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    DWORD dwLocalSamrAccessMask = SAMR_ACCESS_ENUM_DOMAINS |
                                  SAMR_ACCESS_OPEN_DOMAIN;
    SamrConnectInfo ConnReq;
    DWORD dwConnReqLevel = 0;
    SamrConnectInfo ConnInfo;
    DWORD dwConnLevel = 0;
    CONNECT_HANDLE hSamrConn = NULL;
    DWORD dwBuiltinDomainAccessMask = DOMAIN_ACCESS_OPEN_ACCOUNT;
    PSID pBuiltinDomainSid = NULL;
    DOMAIN_HANDLE hBuiltinDomain = NULL;
    DWORD dwAliasAccessMask = ALIAS_ACCESS_ADD_MEMBER |
                              ALIAS_ACCESS_REMOVE_MEMBER;
    ACCOUNT_HANDLE hAlias = NULL;
    PSID pDomainAdminsSid = NULL;
    PSID pDomainUsersSid = NULL;
    DWORD iGroup = 0;
    DWORD iMember = 0;

    PSID *pppAdminMembers[] = { &pDomainAdminsSid, NULL };
    PSID *pppUsersMembers[] = { &pDomainUsersSid, NULL };

    struct _LOCAL_GROUP_MEMBERS
    {
        DWORD   dwLocalGroupRid;
        PSID  **pppMembers;
    }
    Memberships[] =
    {
        { DOMAIN_ALIAS_RID_ADMINS,
          pppAdminMembers
        },
        { DOMAIN_ALIAS_RID_USERS,
          pppUsersMembers
        }
    };

    memset(&ConnReq, 0, sizeof(ConnReq));
    memset(&ConnInfo, 0, sizeof(ConnInfo));

    /*
     * Connect local lsa rpc server and get basic
     * domain information
     */
    ntStatus = LsaInitBindingDefault(&hLsaBinding,
                                      NULL,
                                      NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaOpenPolicy2(hLsaBinding,
                              pwszSystem,
                              NULL,
                              dwLocalPolicyAccessMask,
                              &hLocalPolicy);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LsaQueryInfoPolicy2(hLsaBinding,
                                   hLocalPolicy,
                                   LSA_POLICY_INFO_DOMAIN,
                                   &pInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    pDomainSid = pInfo->domain.sid;

    dwError = LwCreateWellKnownSid(WinAccountDomainAdminsSid,
                                   pDomainSid,
                                   &pDomainAdminsSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwCreateWellKnownSid(WinAccountDomainUsersSid,
                                   pDomainSid,
                                   &pDomainUsersSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Connect local samr rpc server and open BUILTIN domain
     */
    ntStatus = SamrInitBindingDefault(&hSamrBinding,
                                      NULL,
                                      NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ConnReq.info1.client_version = SAMR_CONNECT_POST_WIN2K;
    dwConnReqLevel = 1;

    ntStatus = SamrConnect5(hSamrBinding,
                            pwszSystem,
                            dwLocalSamrAccessMask,
                            dwConnReqLevel,
                            &ConnReq,
                            &dwConnLevel,
                            &ConnInfo,
                            &hSamrConn);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwCreateWellKnownSid(WinBuiltinDomainSid,
                                   NULL,
                                   &pBuiltinDomainSid,
                                   NULL);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrOpenDomain(hSamrBinding,
                              hSamrConn,
                              dwBuiltinDomainAccessMask,
                              pBuiltinDomainSid,
                              &hBuiltinDomain);
    BAIL_ON_NT_STATUS(ntStatus);

    /*
     * Add requested domain groups or users to their
     * corresponding local groups
     */
    for (iGroup = 0;
         iGroup < sizeof(Memberships)/sizeof(Memberships[0]);
         iGroup++)
    {
        DWORD dwRid = Memberships[iGroup].dwLocalGroupRid;

        ntStatus = SamrOpenAlias(hSamrBinding,
                                 hBuiltinDomain,
                                 dwAliasAccessMask,
                                 dwRid,
                                 &hAlias);
        BAIL_ON_NT_STATUS(ntStatus);

        for (iMember = 0;
             Memberships[iGroup].pppMembers[iMember] != NULL;
             iMember++)
        {
            PSID *ppSid = Memberships[iGroup].pppMembers[iMember];

            if (bEnable)
            {
                ntStatus = SamrAddAliasMember(hSamrBinding,
                                              hAlias,
                                              (*ppSid));
                if (ntStatus == STATUS_MEMBER_IN_ALIAS)
                {
                    ntStatus = STATUS_SUCCESS;
                }
            }
            else
            {
                // This should not cause the join to fail even if we cannot
                // remove the group members

                ntStatus = SamrDeleteAliasMember(hSamrBinding,
                                                 hAlias,
                                                 (*ppSid));
                if ((ntStatus != STATUS_SUCCESS) &&
                    (ntStatus != STATUS_NO_SUCH_MEMBER))
                {
                    // Perhaps log an error here
                    ;
                }
                ntStatus = STATUS_SUCCESS;
            }
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SamrClose(hSamrBinding, hAlias);
        BAIL_ON_NT_STATUS(ntStatus);

        hAlias = NULL;
    }

cleanup:
    if (hSamrBinding && hAlias)
    {
        SamrClose(hSamrBinding, hAlias);
    }

    if (hSamrBinding && hBuiltinDomain)
    {
        SamrClose(hSamrBinding, hBuiltinDomain);
    }

    if (hSamrBinding && hSamrConn)
    {
        SamrClose(hSamrBinding, hSamrConn);
    }

    if (pInfo)
    {
        LsaRpcFreeMemory(pInfo);
    }

    if (hLsaBinding && hLocalPolicy)
    {
        LsaClose(hLsaBinding, hLocalPolicy);
    }

    SamrFreeBinding(&hSamrBinding);
    LsaFreeBinding(&hLsaBinding);

    LW_SAFE_FREE_MEMORY(pBuiltinDomainSid);
    LW_SAFE_FREE_MEMORY(pDomainAdminsSid);
    LW_SAFE_FREE_MEMORY(pDomainUsersSid);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = LwNtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


DWORD
LsaMachineChangePassword(
    VOID
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD conn_err = ERROR_SUCCESS;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszLocalname = NULL;
    PWSTR pwszDCName = NULL;
    size_t sDCNameLen = 0;
    HANDLE hStore = (HANDLE)NULL;
    PLWPS_PASSWORD_INFO pPassInfo = NULL;
    PWSTR pwszUserName = NULL;
    PWSTR pwszOldPassword = NULL;
    WCHAR wszNewPassword[MACHPASS_LEN+1];

    memset(wszNewPassword, 0, sizeof(wszNewPassword));

    dwError = LsaGetHostInfo(&pszLocalname);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT, &hStore);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwpsGetPasswordByHostName(hStore, pszLocalname, &pPassInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pPassInfo)
    {
        dwError = ERROR_INTERNAL_ERROR;
        goto error;
    }

    dwError = LsaGetRwDcName(pPassInfo->pwszDnsDomainName, FALSE,
                            &pwszDCName);
    BAIL_ON_LSA_ERROR(dwError);

    pwszUserName     = pPassInfo->pwszMachineAccount;
    pwszOldPassword  = pPassInfo->pwszMachinePassword;

    LsaGenerateMachinePassword(
                  wszNewPassword,
                  sizeof(wszNewPassword)/sizeof(wszNewPassword[0]));

    dwError = LwWc16sLen(pwszDCName, &sDCNameLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUserChangePassword(pwszDCName,
                                    pwszUserName,
                                    pwszOldPassword,
                                    (PWSTR)wszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSaveMachinePassword(
              pPassInfo->pwszHostname,
              pPassInfo->pwszMachineAccount,
              pPassInfo->pwszHostDnsDomain ? pPassInfo->pwszHostDnsDomain
                                           : pPassInfo->pwszDnsDomainName,
              pPassInfo->pwszDomainName,
              pPassInfo->pwszDnsDomainName,
              pwszDCName,
              pPassInfo->pwszSID,
              wszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

    if (pPassInfo)
    {
        LwpsFreePasswordInfo(hStore, pPassInfo);
        pPassInfo = NULL;
    }

    if (hStore != (HANDLE)NULL)
    {
        ntStatus = LwpsClosePasswordStore(hStore);
        hStore = NULL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pszLocalname);
    LW_SAFE_FREE_MEMORY(pwszDCName);

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    if (pPassInfo)
    {
        LwpsFreePasswordInfo(hStore, pPassInfo);
    }

    if (hStore != (HANDLE)NULL)
    {
        LwpsClosePasswordStore(hStore);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus == STATUS_SUCCESS &&
        conn_err != ERROR_SUCCESS) {
        /* overwrite error code with connection error code only if everything
           else was fine so far */
        dwError = conn_err;
    }

    goto cleanup;
}


DWORD
LsaUserChangePassword(
    PWSTR  pwszDCName,
    PWSTR  pwszUserName,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSTR pszHostname = NULL;
    SAMR_BINDING hSamrBinding = NULL;
    size_t sOldPasswordLen = 0;
    size_t sNewPasswordLen = 0;
    BYTE OldNtHash[16] = {0};
    BYTE NewNtHash[16] = {0};
    BYTE NtPasswordBuffer[516] = {0};
    BYTE NtVerHash[16] = {0};
    RC4_KEY RC4Key;
    PIO_CREDS pCreds = NULL;

    ntStatus = LwIoGetActiveCreds(NULL, &pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwWc16sToMbs(pwszDCName, &pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrInitBindingDefault(&hSamrBinding, pwszDCName, pCreds);
    BAIL_ON_NT_STATUS(ntStatus);

    dwError = LwWc16sLen(pwszOldPassword, &sOldPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwWc16sLen(pwszNewPassword, &sNewPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /* prepare NT password hashes */
    dwError = LsaGetNtPasswordHash(pwszOldPassword,
                                   OldNtHash,
                                   sizeof(OldNtHash));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetNtPasswordHash(pwszNewPassword,
                                   NewNtHash,
                                   sizeof(NewNtHash));
    BAIL_ON_LSA_ERROR(dwError);

    /* encode password buffer */
    dwError = LsaEncodePasswordBuffer(pwszNewPassword,
                                      NtPasswordBuffer,
                                      sizeof(NtPasswordBuffer));
    BAIL_ON_LSA_ERROR(dwError);

    RC4_set_key(&RC4Key, 16, (unsigned char*)OldNtHash);
    RC4(&RC4Key, sizeof(NtPasswordBuffer), NtPasswordBuffer, NtPasswordBuffer);

    /* encode NT verifier */
    dwError = LsaEncryptNtHashVerifier(NewNtHash, sizeof(NewNtHash),
                                       OldNtHash, sizeof(OldNtHash),
                                       NtVerHash, sizeof(NtVerHash));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = SamrChangePasswordUser2(hSamrBinding,
                                       pwszDCName,
                                       pwszUserName,
                                       NtPasswordBuffer,
                                       NtVerHash,
                                       0,
                                       NULL,
                                       NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    if (hSamrBinding)
    {
        SamrFreeBinding(&hSamrBinding);
    }

    memset(OldNtHash, 0, sizeof(OldNtHash));
    memset(NewNtHash, 0, sizeof(NewNtHash));
    memset(NtPasswordBuffer, 0, sizeof(NtPasswordBuffer));

    if (pCreds)
    {
        LwIoDeleteCreds(pCreds);
    }

    if (dwError == ERROR_SUCCESS &&
        ntStatus != STATUS_SUCCESS)
    {
        dwError = NtStatusToWin32Error(ntStatus);
    }

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaGetNtPasswordHash(
    IN  PCWSTR  pwszPassword,
    OUT PBYTE   pNtHash,
    IN  DWORD   dwNtHashSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    size_t sPasswordLen = 0;
    PWSTR pwszPasswordLE = NULL;
    BYTE Hash[16] = {0};

    BAIL_ON_INVALID_POINTER(pwszPassword);
    BAIL_ON_INVALID_POINTER(pNtHash);

    if (dwNtHashSize < sizeof(Hash))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwWc16sLen(pwszPassword, &sPasswordLen);
    BAIL_ON_LSA_ERROR(dwError);

    /*
     * Make sure the password is 2-byte little-endian
     */
    dwError = LwAllocateMemory((sPasswordLen + 1) * sizeof(pwszPasswordLE[0]),
                               OUT_PPVOID(&pwszPasswordLE));
    BAIL_ON_LSA_ERROR(dwError);

    wc16stowc16les(pwszPasswordLE, pwszPassword, sPasswordLen);

    MD4((PBYTE)pwszPasswordLE,
        sPasswordLen * sizeof(pwszPasswordLE[0]),
        Hash);

    memcpy(pNtHash, Hash, sizeof(Hash));

cleanup:
    if (pwszPasswordLE)
    {
        memset(pwszPasswordLE, 0, sPasswordLen * sizeof(pwszPasswordLE[0]));
        LW_SAFE_FREE_MEMORY(pwszPasswordLE);
    }

    memset(Hash, 0, sizeof(Hash));

    return dwError;

error:
    memset(pNtHash, 0, dwNtHashSize);

    goto cleanup;
}


static
DWORD
LsaEncryptNtHashVerifier(
    IN  PBYTE    pNewNtHash,
    IN  DWORD    dwNewNtHashLen,
    IN  PBYTE    pOldNtHash,
    IN  DWORD    dwOldNtHashLen,
    OUT PBYTE    pNtVerifier,
    IN  DWORD    dwNtVerifierSize
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DES_cblock KeyBlockLo;
    DES_cblock KeyBlockHi;
    DES_key_schedule KeyLo;
    DES_key_schedule KeyHi;
    BYTE Verifier[16] = {0};

    BAIL_ON_INVALID_POINTER(pNewNtHash);
    BAIL_ON_INVALID_POINTER(pOldNtHash);
    BAIL_ON_INVALID_POINTER(pNtVerifier);

    if (dwNtVerifierSize < sizeof(Verifier))
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    dwError = LsaPrepareDesKey(&pNewNtHash[0],
			       (PBYTE)KeyBlockLo);
    BAIL_ON_LSA_ERROR(dwError);

    DES_set_odd_parity(&KeyBlockLo);
    DES_set_key_unchecked(&KeyBlockLo, &KeyLo);

    dwError = LsaPrepareDesKey(&pNewNtHash[7],
			       (PBYTE)KeyBlockHi);
    BAIL_ON_LSA_ERROR(dwError);

    DES_set_odd_parity(&KeyBlockHi);
    DES_set_key_unchecked(&KeyBlockHi, &KeyHi);

    DES_ecb_encrypt((DES_cblock*)&pOldNtHash[0],
                    (DES_cblock*)&Verifier[0],
                    &KeyLo,
                    DES_ENCRYPT);
    DES_ecb_encrypt((DES_cblock*)&pOldNtHash[8],
                    (DES_cblock*)&Verifier[8],
                    &KeyHi,
                    DES_ENCRYPT);

    memcpy(pNtVerifier, Verifier, sizeof(Verifier));

cleanup:
    memset(&KeyBlockLo, 0, sizeof(KeyBlockLo));
    memset(&KeyBlockHi, 0, sizeof(KeyBlockHi));
    memset(&KeyLo, 0, sizeof(KeyLo));
    memset(&KeyHi, 0, sizeof(KeyHi));

    return dwError;

error:
    goto cleanup;
}


static
DWORD
LsaPrepareDesKey(
    IN  PBYTE  pInput,
    OUT PBYTE  pOutput
    )
{
    DWORD dwError = ERROR_SUCCESS;
    DWORD i = 0;

    BAIL_ON_INVALID_POINTER(pInput);
    BAIL_ON_INVALID_POINTER(pOutput);

    /*
     * Expand the input 7x8 bits so that each 7 bits are
     * appended with 1 bit space for parity bit and yield
     * 8x8 bits ready to become a DES key
     */
    pOutput[0] = pInput[0] >> 1;
    pOutput[1] = ((pInput[0]&0x01) << 6) | (pInput[1] >> 2);
    pOutput[2] = ((pInput[1]&0x03) << 5) | (pInput[2] >> 3);
    pOutput[3] = ((pInput[2]&0x07) << 4) | (pInput[3] >> 4);
    pOutput[4] = ((pInput[3]&0x0F) << 3) | (pInput[4] >> 5);
    pOutput[5] = ((pInput[4]&0x1F) << 2) | (pInput[5] >> 6);
    pOutput[6] = ((pInput[5]&0x3F) << 1) | (pInput[6] >> 7);
    pOutput[7] = pInput[6]&0x7F;

    for (i = 0; i < 8; i++)
    {
        pOutput[i] = pOutput[i] << 1;
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}





/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
