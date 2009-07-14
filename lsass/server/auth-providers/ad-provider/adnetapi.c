/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        adnetapi.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Wrappers for calls to NETAPI
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 */
#include "adprovider.h"
#include "adnetapi.h"

static NetrCredentials gSchannelCreds = { 0 };
static NetrCredentials* gpSchannelCreds = NULL;
static NETRESOURCE gSchannelRes = { 0 };
static NETRESOURCE* gpSchannelRes = NULL;
static handle_t ghSchannelBinding = NULL;
static pthread_mutex_t gSchannelLock = PTHREAD_MUTEX_INITIALIZER;

static
DWORD
AD_GetSystemAccessToken(
    LW_PIO_ACCESS_TOKEN* ppAccessToken
    )
{
    LW_PIO_ACCESS_TOKEN pAccessToken = NULL;
    DWORD dwError = 0;
    PSTR pszUsername = NULL;
    PSTR pszPassword = NULL;
    PSTR pszDomainDnsName = NULL;
    PSTR pszHostDnsDomain = NULL;
    PSTR pszHostname = NULL;
    PSTR pszMachPrincipal = NULL;
    PSTR pszKrb5CcPath = NULL;

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszHostname);

    dwError = LwKrb5GetMachineCreds(
                    pszHostname,
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszMachPrincipal,
                    "%s@%s",
                    pszUsername,
                    pszDomainDnsName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwKrb5GetSystemCachePath(
                    KRB5_File_Cache,
                    &pszKrb5CcPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoCreateKrb5AccessTokenA(
                    pszMachPrincipal,
                    pszKrb5CcPath,
                    &pAccessToken);
    BAIL_ON_LSA_ERROR(dwError);

    *ppAccessToken = pAccessToken;

cleanup:
    LSA_SAFE_FREE_STRING(pszUsername);
    LSA_SAFE_FREE_STRING(pszPassword);
    LSA_SAFE_FREE_STRING(pszDomainDnsName);
    LSA_SAFE_FREE_STRING(pszHostDnsDomain);
    LSA_SAFE_FREE_STRING(pszHostname);
    LSA_SAFE_FREE_STRING(pszMachPrincipal);
    LSA_SAFE_FREE_STRING(pszKrb5CcPath);

    return dwError;

error:
    *ppAccessToken = NULL;
    if (pAccessToken != NULL)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    goto cleanup;
}

static
DWORD
AD_SetSystemAccess(
    LW_PIO_ACCESS_TOKEN* ppOldToken
    )
{
    LW_PIO_ACCESS_TOKEN pOldToken = NULL;
    LW_PIO_ACCESS_TOKEN pSystemToken = NULL;
    DWORD dwError = 0;

    dwError = AD_GetSystemAccessToken(&pSystemToken);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoGetThreadAccessToken(&pOldToken);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwIoSetThreadAccessToken(pSystemToken);
    BAIL_ON_LSA_ERROR(dwError);

    *ppOldToken = pOldToken;

cleanup:
    if (pSystemToken != NULL)
    {
        LwIoDeleteAccessToken(pSystemToken);
    }
    return dwError;

error:
    if (pOldToken != NULL)
    {
        LwIoDeleteAccessToken(pOldToken);
    }
    *ppOldToken = NULL;

    goto cleanup;
}

static
VOID
AD_ClearSchannelState(
    VOID
    );

DWORD
AD_NetInitMemory(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = LsaRpcInitMemory();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NetrInitMemory();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrInitMemory();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NetInitMemory();
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
AD_NetShutdownMemory(
    VOID
    )
{
    DWORD dwError = 0;

    AD_ClearSchannelState();

    dwError = NetDestroyMemory();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrDestroyMemory();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NetrDestroyMemory();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaRpcDestroyMemory();
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

DWORD
AD_NetUserChangePassword(
    PCSTR pszDomainName,
    BOOLEAN bIsInOneWayTrustedDomain,
    PCSTR pszLoginId,
    PCSTR pszUserPrincipalName,
    PCSTR pszOldPassword,
    PCSTR pszNewPassword
    )
{
    DWORD dwError = 0;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszLoginId = NULL;
    PWSTR pwszOldPassword = NULL;
    PWSTR pwszNewPassword = NULL;
    PLSA_ACCESS_TOKEN_FREE_INFO pFreeInfo = NULL;
    LW_PIO_ACCESS_TOKEN pOldAccessToken = NULL;
    BOOLEAN bChangedToken = FALSE;

    BAIL_ON_INVALID_STRING(pszDomainName);
    BAIL_ON_INVALID_STRING(pszLoginId);

    if (bIsInOneWayTrustedDomain)
    {
        dwError = LsaSetSMBAccessToken(
                        pszDomainName,
                        pszUserPrincipalName,
                        pszOldPassword,
                        FALSE,
                        &pFreeInfo,
                        &pOldAccessToken);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = AD_SetSystemAccess(&pOldAccessToken);
        BAIL_ON_LSA_ERROR(dwError);
    }
    bChangedToken = TRUE;

    dwError = LsaMbsToWc16s(
                    pszDomainName,
                    &pwszDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(
                    pszLoginId,
                    &pwszLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszOldPassword)) {

        dwError = LsaMbsToWc16s(
                    pszOldPassword,
                    &pwszOldPassword);
        BAIL_ON_LSA_ERROR(dwError);

    }

    if (!IsNullOrEmptyString(pszNewPassword)) {

        dwError = LsaMbsToWc16s(
                    pszNewPassword,
                    &pwszNewPassword);
        BAIL_ON_LSA_ERROR(dwError);

    }

    dwError = NetUserChangePassword(
                    pwszDomainName,
                    pwszLoginId,
                    pwszOldPassword,
                    pwszNewPassword);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_MEMORY(pwszDomainName);
    LSA_SAFE_FREE_MEMORY(pwszLoginId);
    LSA_SAFE_FREE_MEMORY(pwszOldPassword);
    LSA_SAFE_FREE_MEMORY(pwszNewPassword);
    LsaFreeSMBAccessToken(&pFreeInfo);
    if (bChangedToken)
    {
        LwIoSetThreadAccessToken(pOldAccessToken);
    }
    if (pOldAccessToken != NULL)
    {
        LwIoDeleteAccessToken(pOldAccessToken);
    }

    return AD_MapNetApiError(dwError);

error:

    goto cleanup;
}

// ISSUE-2008/08/25-dalmeida -- This needs to be pulled in by header
// with LsaOpenPolicy2 and LsaLookupNames2 functions.
#ifndef STATUS_UNHANDLED_EXCEPTION
#define STATUS_UNHANDLED_EXCEPTION 0xc0000144
#endif

#ifndef STATUS_SOME_UNMAPPED
#define STATUS_SOME_UNMAPPED 0x00000107
#endif

static
ADAccountType
GetObjectType(
    IN LsaSidType Type
    )
{
    ADAccountType ObjectType = AccountType_NotFound;

    switch(Type)
    {
        case SID_TYPE_USER:
            ObjectType = AccountType_User;
            break;

        case SID_TYPE_DOM_GRP:
        case SID_TYPE_ALIAS:
        case SID_TYPE_WKN_GRP:
            ObjectType = AccountType_Group;
            break;
        case SID_TYPE_DOMAIN:
            ObjectType = AccountType_Domain;
            break;

        default:
            ObjectType = AccountType_NotFound;
    }

    return ObjectType;
}

DWORD
AD_NetLookupObjectSidByName(
    IN PCSTR pszHostname,
    IN PCSTR pszObjectName,
    OUT PSTR* ppszObjectSid,
    OUT ADAccountType* pObjectType,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedSids = NULL;
    PSTR pszObjectSid = NULL;
    BOOLEAN bIsNetworkError = FALSE;

    dwError = AD_NetLookupObjectSidsByNames(
                 pszHostname,
                 1,
                 (PSTR*)&pszObjectName,
                 &ppTranslatedSids,
                 NULL,
                 &bIsNetworkError);
    BAIL_ON_LSA_ERROR(dwError);

    // In case of NOT found, the above function bails out with dwError == LSA_ERROR_RPC_LSA_LOOKUPNAMES_FAILED
    // Double check here again
    if (!ppTranslatedSids || !ppTranslatedSids[0])
    {
        dwError = LSA_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(ppTranslatedSids[0]->pszNT4NameOrSid,
                                &pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszObjectSid = pszObjectSid;
    *pObjectType = ppTranslatedSids[0]->ObjectType;

cleanup:
    *pbIsNetworkError = bIsNetworkError;
    if (ppTranslatedSids)
    {
       LsaFreeTranslatedNameList(ppTranslatedSids, 1);
    }
    return dwError;

error:
    *ppszObjectSid = NULL;
    LSA_SAFE_FREE_STRING(pszObjectSid);
    *pObjectType = AccountType_NotFound;
    LSA_LOG_ERROR("Failed to find user or group. [Error code: %d]", dwError);
    dwError = LSA_ERROR_NO_SUCH_OBJECT;

    goto cleanup;
}

DWORD
AD_NetLookupObjectSidsByNames(
    IN PCSTR pszHostname,
    IN DWORD dwNamesCount,
    IN PSTR* ppszNames,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedSids,
    OUT OPTIONAL PDWORD pdwFoundSidsCount,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PWSTR pwcHost = NULL;
    RPCSTATUS rpcStatus;
    NTSTATUS status = 0;
    handle_t lsa_binding = (HANDLE)NULL;
    DWORD dwAccess_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    PolicyHandle lsa_policy = {0};
    DWORD dwLevel;
    DWORD dwFoundSidsCount = 0;
    PWSTR* ppwcNames = NULL;
    RefDomainList* pDomains = NULL;
    TranslatedSid2* pSids = NULL;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedSids = NULL;
    PSID pObject_sid = NULL;
    BOOLEAN bIsNetworkError = FALSE;
    DWORD i = 0;
    LW_PIO_ACCESS_TOKEN pAccessToken = NULL;
    LW_PIO_ACCESS_TOKEN pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;

    BAIL_ON_INVALID_STRING(pszHostname);
    dwError = LsaMbsToWc16s(
                  pszHostname,
                  &pwcHost);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SetSystemAccess(&pOldToken);
    BAIL_ON_LSA_ERROR(dwError);
    bChangedToken = TRUE;

	status = LwIoGetThreadAccessToken(&pAccessToken);
	dwError = LwNtStatusToUnixErrno(status);
    BAIL_ON_LSA_ERROR(dwError);

    rpcStatus = InitLsaBindingDefault(&lsa_binding, pszHostname, pAccessToken);
    if (rpcStatus != 0)
    {
       dwError = LSA_ERROR_RPC_LSABINDING_FAILED;
       bIsNetworkError = TRUE;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if (lsa_binding == NULL)
    {
        dwError = LSA_ERROR_RPC_LSABINDING_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Convert ppszNames to ppwcNames
    dwError = LsaAllocateMemory(
                    sizeof(*ppwcNames)*dwNamesCount,
                    (PVOID*)&ppwcNames);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwNamesCount; i++)
    {
        dwError = LsaMbsToWc16s(
                      ppszNames[i],
                      &ppwcNames[i]);
        BAIL_ON_LSA_ERROR(dwError);
    }

    status = LsaOpenPolicy2(lsa_binding,
                            pwcHost,
                            NULL,
                            dwAccess_rights,
                            &lsa_policy);
    if (status != 0)
    {
        LSA_LOG_DEBUG("LsaOpenPolicy2() failed with %d (0x%08x)", status, status);
        dwError = LSA_ERROR_RPC_OPENPOLICY_FAILED;
        if (IsDceRpcConnError(status))
        {
            bIsNetworkError = TRUE;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Lookup name to sid */
    dwLevel = 1;
    status = LsaLookupNames2(
                   lsa_binding,
                   &lsa_policy,
                   dwNamesCount,
                   ppwcNames,
                   &pDomains,
                   &pSids,
                   dwLevel,
                   &dwFoundSidsCount);
    if (status != 0)
    {
        if (STATUS_SOME_UNMAPPED == status)
        {
            dwError = 0;
            LSA_LOG_DEBUG("LsaLookupNames2() succeeded incomplete results with %d (0x%08x) -- Partial results returned (got %u, expected %u)",
                           status, status, dwFoundSidsCount, dwNamesCount);
        }
        else
        {
            LSA_LOG_DEBUG("LsaLookupNames2() failed with %d (0x%08x)", status, status);
            dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_FAILED;
            if (IsDceRpcConnError(status))
            {
                bIsNetworkError = TRUE;
            }
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwFoundSidsCount == 0)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwFoundSidsCount > dwNamesCount)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!pSids || !pDomains)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // For incomplete results (STATUS_SOME_UNMAPPED == status), leave ppTranslatedSids[i] as NULL for those NOT found
    // to maintain ppszNames[i] -> ppTranslatedSids[i]
    dwError = LsaAllocateMemory(
                    sizeof(*ppTranslatedSids)*dwNamesCount,
                    (PVOID*)&ppTranslatedSids);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwNamesCount; i++)
    {
        ADAccountType ObjectType = AccountType_NotFound;
        DWORD dwDomainSid_index = 0;

        ObjectType = GetObjectType(pSids[i].type);
        if (ObjectType == AccountType_NotFound)
        {
            continue;
        }

        dwError = LsaAllocateMemory(
                            sizeof(*ppTranslatedSids[i]),
                            (PVOID*)&ppTranslatedSids[i]);
        BAIL_ON_LSA_ERROR(dwError);

        ppTranslatedSids[i]->ObjectType = ObjectType;

        dwDomainSid_index = pSids[i].index;

        if (dwDomainSid_index >= pDomains->count)
        {
            dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_FAILED;
            BAIL_ON_LSA_ERROR(dwError);
        }

        LSA_SAFE_FREE_MEMORY(pObject_sid);

        if (AccountType_Domain == ObjectType)
        {
            dwError = LsaAllocateCStringFromSid(
                            &ppTranslatedSids[i]->pszNT4NameOrSid,
                            pDomains->domains[dwDomainSid_index].sid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LsaAllocateSidAppendRid(
                            &pObject_sid,
                            pDomains->domains[dwDomainSid_index].sid,
                            pSids[i].rid);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocateCStringFromSid(
                            &ppTranslatedSids[i]->pszNT4NameOrSid,
                            pObject_sid);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pppTranslatedSids = ppTranslatedSids;
    if (pdwFoundSidsCount)
    {
        *pdwFoundSidsCount = dwFoundSidsCount;
    }

cleanup:
    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }

    LSA_SAFE_FREE_MEMORY(pwcHost);
    if (ppwcNames)
    {
        for (i = 0; i < dwNamesCount; i++)
        {
            LSA_SAFE_FREE_MEMORY(ppwcNames[i]);
        }
        LsaFreeMemory(ppwcNames);
    }
    if (pDomains)
    {
        LsaRpcFreeMemory(pDomains);
    }
    if (pSids)
    {
        LsaRpcFreeMemory(pSids);
    }
    LSA_SAFE_FREE_MEMORY(pObject_sid);
    status = LsaClose(lsa_binding, &lsa_policy);
    if (status != 0 && dwError == 0)
    {
        dwError = LSA_ERROR_RPC_CLOSEPOLICY_FAILED;
    }
    if (lsa_binding)
    {
        FreeLsaBinding(&lsa_binding);
    }
    if (bChangedToken)
    {
        LwIoSetThreadAccessToken(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteAccessToken(pOldToken);
    }

    if (pAccessToken)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    return dwError;

error:
   *pppTranslatedSids = NULL;
   if (pdwFoundSidsCount)
   {
       *pdwFoundSidsCount = 0;
   }
   if (ppTranslatedSids)
   {
       LsaFreeTranslatedNameList(ppTranslatedSids, dwNamesCount);
   }

   goto cleanup;
}

DWORD
AD_NetLookupObjectNameBySid(
    IN PCSTR pszHostname,
    IN PCSTR pszObjectSid,
    OUT PSTR* ppszNT4Name,
    OUT ADAccountType* pObjectType,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedNames = NULL;
    PSTR pszNT4Name = NULL;
    BOOLEAN bIsNetworkError = FALSE;

    dwError = AD_NetLookupObjectNamesBySids(
                 pszHostname,
                 1,
                 (PSTR*)&pszObjectSid,
                 &ppTranslatedNames,
                 NULL,
                 &bIsNetworkError);
    BAIL_ON_LSA_ERROR(dwError);

    // In case of NOT found, the above function bails out with dwError == LSA_ERROR_RPC_LSA_LOOKUPSIDS_FAILED
    // Double check here again
    if (!ppTranslatedNames || !ppTranslatedNames[0])
    {
        dwError = LSA_ERROR_NO_SUCH_OBJECT;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(ppTranslatedNames[0]->pszNT4NameOrSid,
                                &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;
    *pObjectType = ppTranslatedNames[0]->ObjectType;

cleanup:
    *pbIsNetworkError = bIsNetworkError;

    if (ppTranslatedNames)
    {
       LsaFreeTranslatedNameList(ppTranslatedNames, 1);
    }
    return dwError;

error:
    *ppszNT4Name = NULL;
    LSA_SAFE_FREE_STRING(pszNT4Name);
    *pObjectType = AccountType_NotFound;

    LSA_LOG_ERROR("Failed to find user or group. [Error code: %d]", dwError);

    dwError = LSA_ERROR_NO_SUCH_OBJECT;

    goto cleanup;
}

DWORD
AD_NetLookupObjectNamesBySids(
    IN PCSTR pszHostname,
    IN DWORD dwSidsCount,
    IN PSTR* ppszObjectSids,
    OUT PLSA_TRANSLATED_NAME_OR_SID** pppTranslatedNames,
    OUT OPTIONAL PDWORD pdwFoundNamesCount,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PWSTR pwcHost = NULL;
    RPCSTATUS rpcStatus;
    NTSTATUS status = 0;
    handle_t lsa_binding = (HANDLE)NULL;
    DWORD dwAccess_rights = LSA_ACCESS_LOOKUP_NAMES_SIDS;
    PolicyHandle lsa_policy = {0};
    SidArray sid_array  = {0};
    DWORD dwLevel = 1;
    DWORD dwFoundNamesCount = 0;
    RefDomainList* pDomains = NULL;
    PSTR* ppszDomainNames = NULL;
    PSID pObjectSID = NULL;
    TranslatedName* name_array = NULL;
    PSTR pszUsername = NULL;
    PLSA_TRANSLATED_NAME_OR_SID* ppTranslatedNames = NULL;
    BOOLEAN bIsNetworkError = FALSE;
    DWORD i = 0;
    PIO_ACCESS_TOKEN pAccessToken = NULL;
    LW_PIO_ACCESS_TOKEN pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;

    BAIL_ON_INVALID_STRING(pszHostname);

    dwError = LsaMbsToWc16s(
                  pszHostname,
                  &pwcHost);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SetSystemAccess(&pOldToken);
    BAIL_ON_LSA_ERROR(dwError);
    bChangedToken = TRUE;

    status = LwIoGetThreadAccessToken(&pAccessToken);
    dwError = LwNtStatusToUnixErrno(status);
    BAIL_ON_LSA_ERROR(dwError);

    rpcStatus = InitLsaBindingDefault(&lsa_binding, pszHostname, pAccessToken);

    if (rpcStatus != 0)
    {
       dwError = LSA_ERROR_RPC_LSABINDING_FAILED;
       bIsNetworkError = TRUE;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if (lsa_binding == NULL)
    {
        dwError = LSA_ERROR_RPC_LSABINDING_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // Convert ppszObjectSids to sid_array
    sid_array.num_sids = dwSidsCount;
    dwError = LsaAllocateMemory(
                    sizeof(*sid_array.sids)*sid_array.num_sids,
                    (PVOID*)&sid_array.sids);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < sid_array.num_sids; i++)
    {
        dwError = LsaAllocateSidFromCString(
                        &pObjectSID,
                        ppszObjectSids[i]);
        BAIL_ON_LSA_ERROR(dwError);

        sid_array.sids[i].sid = pObjectSID;
        pObjectSID = NULL;
    }

    status = LsaOpenPolicy2(lsa_binding,
                            pwcHost,
                            NULL,
                            dwAccess_rights,
                            &lsa_policy);
    if (status != 0)
    {
        LSA_LOG_DEBUG("LsaOpenPolicy2() failed with %d (0x%08x)", status, status);
        dwError = LSA_ERROR_RPC_OPENPOLICY_FAILED;
        if (IsDceRpcConnError(status))
        {
            bIsNetworkError = TRUE;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Lookup sid to name */
    status = LsaLookupSids(
                   lsa_binding,
                   &lsa_policy,
                   &sid_array,
                   &pDomains,
                   &name_array,
                   dwLevel,
                   &dwFoundNamesCount);
    if (status != 0)
    {
        if (STATUS_SOME_UNMAPPED == status)
        {
            dwError = 0;
            LSA_LOG_DEBUG("LsaLookupSids() succeeded incomplete results with %d (0x%08x) -- Partial results returned (got %u, expected %u)",
                           status, status, dwFoundNamesCount, dwSidsCount);
        }
        else
        {
            LSA_LOG_DEBUG("LsaLookupSids() failed with %d (0x%08x)", status, status);

            dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_FAILED;
            if (IsDceRpcConnError(status))
            {
                bIsNetworkError = TRUE;
            }
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwFoundNamesCount == 0)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwFoundNamesCount > dwSidsCount)
    {
        dwError = LSA_ERORR_RPC_LSA_LOOKUPSIDS_FOUND_DUPLICATES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (!name_array || !pDomains)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                     sizeof(*ppszDomainNames)*pDomains->count,
                     (PVOID*)&ppszDomainNames);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < pDomains->count; i++)
    {
        if (pDomains->domains[i].name.len > 0)
        {
            dwError = LsaWc16snToMbs(
                          pDomains->domains[i].name.string,
                          &ppszDomainNames[i],
                          pDomains->domains[i].name.len / 2);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    // For incomplete results (STATUS_SOME_UNMAPPED == status), leave ppTranslatedNames[i] as NULL for those NOT found
    // to maintain ppszObjectSids[i] -> ppTranslatedNames[i]
    dwError = LsaAllocateMemory(
                    sizeof(*ppTranslatedNames)*dwSidsCount,
                    (PVOID*)&ppTranslatedNames);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; i < dwSidsCount; i++)
    {
        ADAccountType ObjectType = AccountType_NotFound;
        PCSTR pszDomainName = NULL;

        // Check for invalid domain indexing
        if (name_array[i].sid_index >= pDomains->count)
        {
            dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
            BAIL_ON_LSA_ERROR(dwError);
        }

        ObjectType = GetObjectType(name_array[i].type);
        if (ObjectType == AccountType_NotFound)
        {
            continue;
        }

        pszDomainName = ppszDomainNames[name_array[i].sid_index];

        if (name_array[i].name.len > 0)
        {
            dwError = LsaWc16snToMbs(
                           name_array[i].name.string,
                           &pszUsername,
                           name_array[i].name.len / 2);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (IsNullOrEmptyString(pszDomainName) ||
            ((ObjectType != AccountType_Domain) && IsNullOrEmptyString(pszUsername)) ||
            ((ObjectType == AccountType_Domain) && !IsNullOrEmptyString(pszUsername)))
        {
            dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                        sizeof(*ppTranslatedNames[i]),
                        (PVOID*)&ppTranslatedNames[i]);
        BAIL_ON_LSA_ERROR(dwError);

        ppTranslatedNames[i]->ObjectType = ObjectType;

        if (ObjectType != AccountType_Domain)
        {
            dwError = ADGetDomainQualifiedString(
                            pszDomainName,
                            pszUsername,
                            &ppTranslatedNames[i]->pszNT4NameOrSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LsaAllocateString(
                            pszDomainName,
                            &ppTranslatedNames[i]->pszNT4NameOrSid);
            BAIL_ON_LSA_ERROR(dwError);

            LsaStrToUpper(ppTranslatedNames[i]->pszNT4NameOrSid);
        }

        LSA_SAFE_FREE_STRING(pszUsername);
    }

    *pppTranslatedNames = ppTranslatedNames;

cleanup:

    if (pdwFoundNamesCount)
    {
        *pdwFoundNamesCount = dwFoundNamesCount;
    }

    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }

    if (pDomains)
    {
        LsaFreeStringArray(ppszDomainNames,pDomains->count);
        LsaRpcFreeMemory(pDomains);
    }

    LSA_SAFE_FREE_STRING(pszUsername);
    LSA_SAFE_FREE_MEMORY(pwcHost);


    if (name_array)
    {
        LsaRpcFreeMemory(name_array);
    }

    if (sid_array.sids)
    {
        for (i = 0; i < sid_array.num_sids; i++)
        {
            LSA_SAFE_FREE_MEMORY(sid_array.sids[i].sid);
        }
        LSA_SAFE_FREE_MEMORY(sid_array.sids);
    }

    LSA_SAFE_FREE_MEMORY(pObjectSID);

    status = LsaClose(lsa_binding, &lsa_policy);
    if (status != 0 && dwError == 0){
        dwError = LSA_ERROR_RPC_CLOSEPOLICY_FAILED;
    }

    if (lsa_binding)
    {
        FreeLsaBinding(&lsa_binding);
    }
    if (bChangedToken)
    {
        LwIoSetThreadAccessToken(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteAccessToken(pOldToken);
    }

    if (pAccessToken)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    return dwError;

error:

    *pppTranslatedNames = NULL;

    if (ppTranslatedNames)
    {
        LsaFreeTranslatedNameList(ppTranslatedNames, dwSidsCount);
    }

    goto cleanup;
}

DWORD
AD_DsEnumerateDomainTrusts(
    IN PCSTR pszDomainControllerName,
    IN DWORD dwFlags,
    OUT NetrDomainTrust** ppTrusts,
    OUT PDWORD pdwCount,
    OUT OPTIONAL PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PWSTR pwcDomainControllerName = NULL;
    RPCSTATUS status = 0;
    handle_t netr_b = NULL;
    WINERR winError = 0;
    NetrDomainTrust* pTrusts = NULL;
    DWORD dwCount = 0;
    BOOLEAN bIsNetworkError = FALSE;
    LW_PIO_ACCESS_TOKEN pAccessToken = NULL;
    LW_PIO_ACCESS_TOKEN pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;

    dwError = LsaMbsToWc16s(pszDomainControllerName, &pwcDomainControllerName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SetSystemAccess(&pOldToken);
    BAIL_ON_LSA_ERROR(dwError);
    bChangedToken = TRUE;

    status = LwIoGetThreadAccessToken(&pAccessToken);
    dwError = LwNtStatusToUnixErrno(status);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitNetlogonBindingDefault(&netr_b,
                                        pszDomainControllerName,
                                        pAccessToken,
                                        FALSE);
    if (status != 0)
    {
        LSA_LOG_DEBUG("Failed to bind to %s (error %d)",
                      pszDomainControllerName, status);
        dwError = LSA_ERROR_RPC_NETLOGON_FAILED;
        bIsNetworkError = TRUE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    winError = DsrEnumerateDomainTrusts(netr_b,
                                        pwcDomainControllerName,
                                        dwFlags,
                                        &pTrusts,
                                        &dwCount);
    if (winError)
    {
        LSA_LOG_DEBUG("Failed to enumerate trusts at %s (error %d)",
                      pszDomainControllerName, winError);
        dwError = LSA_ERROR_ENUM_DOMAIN_TRUSTS_FAILED;
        // ISSUE-2008/08/25-dalmeida -- Bad error propagation.
        if (IsDceRpcConnError(winError))
        {
            bIsNetworkError = TRUE;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    if (netr_b)
    {
        FreeNetlogonBinding(&netr_b);
        netr_b = NULL;
    }
    LSA_SAFE_FREE_MEMORY(pwcDomainControllerName);
    if (bChangedToken)
    {
        LwIoSetThreadAccessToken(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteAccessToken(pOldToken);
    }
    if (pAccessToken != NULL)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }
    *ppTrusts = pTrusts;
    *pdwCount = dwCount;
    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }
    return dwError;

error:
    dwCount = 0;
    if (pTrusts)
    {
        NetrFreeMemory(pTrusts);
        pTrusts = NULL;
    }
    goto cleanup;
}

VOID
AD_FreeDomainTrusts(
    IN OUT NetrDomainTrust** ppTrusts
    )
{
    if (ppTrusts && *ppTrusts)
    {
        NetrFreeMemory(*ppTrusts);
        *ppTrusts = NULL;
    }
}

DWORD
AD_DsGetDcName(
    IN PCSTR pszServerName,
    IN PCSTR pszDomainName,
    IN BOOLEAN bReturnDnsName,
    OUT PSTR* ppszDomainDnsOrFlatName,
    OUT PSTR* ppszDomainForestDnsName,
    OUT OPTIONAL PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = 0;
    PWSTR pwcServerName = NULL;
    PWSTR pwcDomainName = NULL;
    RPCSTATUS status = 0;
    handle_t netr_b = NULL;
    WINERR winError = 0;
    BOOLEAN bIsNetworkError = FALSE;
    LW_PIO_ACCESS_TOKEN pAccessToken = NULL;
    LW_PIO_ACCESS_TOKEN pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;
    const uint32 dwGetDcNameflags = bReturnDnsName ? DS_RETURN_DNS_NAME : DS_RETURN_FLAT_NAME;
    DsrDcNameInfo* pDcNameInfo = NULL;
    PSTR pszDomainDnsOrFlatName = NULL;
    PSTR pszDomainForestDnsName = NULL;

    dwError = LsaMbsToWc16s(pszServerName, &pwcServerName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SetSystemAccess(&pOldToken);
    BAIL_ON_LSA_ERROR(dwError);
    bChangedToken = TRUE;

    status = LwIoGetThreadAccessToken(&pAccessToken);
    dwError = LwNtStatusToUnixErrno(status);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitNetlogonBindingDefault(&netr_b,
                                        pszServerName,
                                        pAccessToken,
                                        FALSE);
    if (status != 0)
    {
        LSA_LOG_DEBUG("Failed to bind to %s (error %d)",
                       pszServerName, status);
        dwError = LSA_ERROR_RPC_NETLOGON_FAILED;
        bIsNetworkError = TRUE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s(pszDomainName, &pwcDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    winError = DsrGetDcName(netr_b,
                            pwcServerName,
                            pwcDomainName,
                            NULL,
                            NULL,
                            dwGetDcNameflags,
                            &pDcNameInfo);
    if (winError)
    {
        LSA_LOG_DEBUG("Failed to get dc name information for %s at %s (error %d)",
                      pszDomainName,
                      pszServerName,
                      winError);
        dwError = LSA_ERROR_GET_DC_NAME_FAILED;
        if (IsDceRpcConnError(winError))
        {
            bIsNetworkError = TRUE;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaWc16sToMbs(pDcNameInfo->domain_name, &pszDomainDnsOrFlatName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWc16sToMbs(pDcNameInfo->forest_name, &pszDomainForestDnsName);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (netr_b)
    {
        FreeNetlogonBinding(&netr_b);
        netr_b = NULL;
    }
    LSA_SAFE_FREE_MEMORY(pwcServerName);
    LSA_SAFE_FREE_MEMORY(pwcDomainName);
    if (bChangedToken)
    {
        LwIoSetThreadAccessToken(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteAccessToken(pOldToken);
    }
    if (pAccessToken != NULL)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }
    NetrFreeMemory((void*)pDcNameInfo);

    *ppszDomainDnsOrFlatName = pszDomainDnsOrFlatName;
    *ppszDomainForestDnsName = pszDomainForestDnsName;

    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }

    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszDomainDnsOrFlatName);
    LSA_SAFE_FREE_STRING(pszDomainForestDnsName);

    goto cleanup;
}

DWORD
AD_MapNetApiError(
    DWORD dwADError
    )
{
    DWORD dwError = 0;

    switch(dwADError)
    {
        case 1325:
            // There is no RPC error code for this at present.

            dwError = LSA_ERROR_PASSWORD_RESTRICTION;
            break;
        default:
            dwError = dwADError;
            break;
    }

    return dwError;
}

void
LsaFreeTranslatedNameInfo(
    IN OUT PLSA_TRANSLATED_NAME_OR_SID pNameInfo
    )
{
    LSA_SAFE_FREE_STRING(pNameInfo->pszNT4NameOrSid);
    LsaFreeMemory(pNameInfo);
}

void
LsaFreeTranslatedNameList(
    IN OUT PLSA_TRANSLATED_NAME_OR_SID* pNameList,
    IN DWORD dwNumNames
    )
{
    DWORD iName = 0;

    for (iName = 0; iName < dwNumNames; iName++)
    {
        PLSA_TRANSLATED_NAME_OR_SID pNameInfo = pNameList[iName];
        if (pNameInfo)
        {
            LsaFreeTranslatedNameInfo(pNameInfo);
        }
    }
    LsaFreeMemory(pNameList);
}

INT64
WinTimeToInt64(
    WinNtTime WinTime
    )
{
    INT64 Int64Time = 0;

    Int64Time = WinTime.high;
    Int64Time = Int64Time << 32;
    Int64Time |= WinTime.low;

    return Int64Time;
}

static DWORD
LsaCopyNetrUserInfo3(
    OUT PLSA_AUTH_USER_INFO pUserInfo,
    IN NetrValidationInfo *pNetrUserInfo3
    )
{
    DWORD dwError = LSA_ERROR_INTERNAL;
    NetrSamBaseInfo *pBase = NULL;
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_POINTER(pUserInfo);
    BAIL_ON_INVALID_POINTER(pNetrUserInfo3);

    pBase = &pNetrUserInfo3->sam3->base;

    pUserInfo->dwUserFlags = pBase->user_flags;

    pUserInfo->LogonTime          = WinTimeToInt64(pBase->last_logon);
    pUserInfo->LogoffTime         = WinTimeToInt64(pBase->last_logoff);
    pUserInfo->KickoffTime        = WinTimeToInt64(pBase->acct_expiry);
    pUserInfo->LastPasswordChange = WinTimeToInt64(pBase->last_password_change);
    pUserInfo->CanChangePassword  = WinTimeToInt64(pBase->allow_password_change);
    pUserInfo->MustChangePassword = WinTimeToInt64(pBase->force_password_change);

    pUserInfo->LogonCount       = pBase->logon_count;
    pUserInfo->BadPasswordCount = pBase->bad_password_count;

    pUserInfo->dwAcctFlags = pBase->acct_flags;

    dwError = LsaDataBlobStore(&pUserInfo->pSessionKey,
                               sizeof(pBase->key.key),
                               pBase->key.key);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDataBlobStore(&pUserInfo->pLmSessionKey,
                               sizeof(pBase->lmkey.key),
                               pBase->lmkey.key);
    BAIL_ON_LSA_ERROR(dwError);

    pUserInfo->pszUserPrincipalName = NULL;
    pUserInfo->pszDnsDomain = NULL;

    if (pBase->account_name.len)
    {
        dwError = LsaWc16sToMbs(pBase->account_name.string, &pUserInfo->pszAccount);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->full_name.len)
    {
        dwError = LsaWc16sToMbs(pBase->full_name.string, &pUserInfo->pszFullName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->domain.len)
    {
        dwError = LsaWc16sToMbs(pBase->domain.string, &pUserInfo->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->logon_server.len)
    {
        dwError = LsaWc16sToMbs(pBase->logon_server.string, &pUserInfo->pszLogonServer);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->logon_script.len)
    {
        dwError = LsaWc16sToMbs(pBase->logon_script.string, &pUserInfo->pszLogonScript);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->home_directory.len)
    {
        dwError = LsaWc16sToMbs(pBase->home_directory.string, &pUserInfo->pszHomeDirectory);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->home_drive.len)
    {
        dwError = LsaWc16sToMbs(pBase->home_drive.string, &pUserInfo->pszHomeDrive);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pBase->profile_path.len)
    {
        dwError = LsaWc16sToMbs(pBase->profile_path.string, &pUserInfo->pszProfilePath);
        BAIL_ON_LSA_ERROR(dwError);
    }

    ntError = RtlAllocateCStringFromSid(&pUserInfo->pszDomainSid, pBase->domain_sid);
    BAIL_ON_NT_STATUS(dwError);

    pUserInfo->dwUserRid         = pBase->rid;
    pUserInfo->dwPrimaryGroupRid = pBase->primary_gid;

    pUserInfo->dwNumRids = pBase->groups.count;
    if (pUserInfo->dwNumRids != 0)
    {
        int i = 0;

        dwError = LsaAllocateMemory(sizeof(LSA_RID_ATTRIB)*(pUserInfo->dwNumRids),
                                    (PVOID*)&pUserInfo->pRidAttribList);
        BAIL_ON_LSA_ERROR(dwError);

        for (i=0; i<pUserInfo->dwNumRids; i++)
        {
            pUserInfo->pRidAttribList[i].Rid      = pBase->groups.rids[i].rid;
            pUserInfo->pRidAttribList[i].dwAttrib = pBase->groups.rids[i].attributes;
        }

    }

    pUserInfo->dwNumSids = pNetrUserInfo3->sam3->sidcount;
    if (pUserInfo->dwNumSids != 0)
    {
        int i = 0;

        dwError = LsaAllocateMemory(sizeof(LSA_SID_ATTRIB)*(pUserInfo->dwNumSids),
                                    (PVOID*)&pUserInfo->pSidAttribList);
        BAIL_ON_LSA_ERROR(dwError);

        for (i=0; i<pUserInfo->dwNumSids; i++)
        {
            PLSA_SID_ATTRIB pSidAttrib = &(pUserInfo->pSidAttribList[i]);

            pSidAttrib->dwAttrib = pNetrUserInfo3->sam3->sids[i].attribute;

            ntError = RtlAllocateCStringFromSid(&pSidAttrib->pszSid,
                                                pNetrUserInfo3->sam3->sids[i].sid);
            BAIL_ON_NT_STATUS(dwError);
        }
    }

    /* All done */

    dwError = LSA_ERROR_SUCCESS;

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
AD_NetlogonAuthenticationUserEx(
    IN PSTR pszDomainController,
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    OUT PLSA_AUTH_USER_INFO *ppUserInfo,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    DWORD dwError = LSA_ERROR_INTERNAL;
    PWSTR pwszDomainController = NULL;
    PWSTR pwszServerName = NULL;
    PWSTR pwszShortDomain = NULL;
    PWSTR pwszPrimaryShortDomain = NULL;
    PWSTR pwszUsername = NULL;
    PSTR pszHostname = NULL;
    HANDLE hPwdDb = (HANDLE)NULL;
    RPCSTATUS status = 0;
    handle_t netr_b = NULL;
    PLWPS_PASSWORD_INFO pMachAcctInfo = NULL;
    BOOLEAN bIsNetworkError = FALSE;
    NTSTATUS nt_status = STATUS_UNHANDLED_EXCEPTION;
    NetrValidationInfo  *pValidationInfo = NULL;
    UINT8 dwAuthoritative = 0;
    PSTR pszServerName;
    DWORD dwDCNameLen = 0;
    PBYTE pChal = NULL;
    PBYTE pLMResp = NULL;
    DWORD LMRespLen = 0;
    PBYTE pNTResp = NULL;
    DWORD NTRespLen = 0;
    LW_PIO_ACCESS_TOKEN pAccessToken = NULL;
    LW_PIO_ACCESS_TOKEN pOldToken = NULL;
    BOOLEAN bChangedToken = FALSE;

    pthread_mutex_lock(&gSchannelLock);

    /* Grab the machine password and account info */

    dwError = LwpsOpenPasswordStore(LWPS_PASSWORD_STORE_DEFAULT,
                                    &hPwdDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwpsGetPasswordByHostName(hPwdDb,
                                        pszHostname,
                                        &pMachAcctInfo);
    BAIL_ON_LSA_ERROR(dwError);

    /* Gather other Schannel params */

    /* Allocate space for the servername.  Include room for the terminating
       NULL and \\ */

    dwDCNameLen = strlen(pszDomainController) + 3;
    dwError = LsaAllocateMemory(dwDCNameLen, (PVOID*)&pszServerName);
    BAIL_ON_LSA_ERROR(dwError);

    snprintf(pszServerName, dwDCNameLen, "\\\\%s", pszDomainController);
    dwError = LsaMbsToWc16s(pszServerName, &pwszServerName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMbsToWc16s(pUserParams->pszDomain, &pwszShortDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (!ghSchannelBinding)
    {
        dwError = LsaMbsToWc16s(pszDomainController, &pwszDomainController);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaMbsToWc16s(gpADProviderData->szShortDomain,
                                &pwszPrimaryShortDomain);
        BAIL_ON_LSA_ERROR(dwError);

        /* Establish the initial bind to \NETLOGON */

        dwError = AD_SetSystemAccess(&pOldToken);
        BAIL_ON_LSA_ERROR(dwError);
        bChangedToken = TRUE;

        status = LwIoGetThreadAccessToken(&pAccessToken);
        dwError = LwNtStatusToUnixErrno(status);
        BAIL_ON_LSA_ERROR(dwError);

        status = InitNetlogonBindingDefault(&netr_b,(const char*)pszDomainController, pAccessToken, FALSE);
        if (status != 0)
        {
            LSA_LOG_DEBUG("Failed to bind to %s (error %d)",
                          pszDomainController, status);
            dwError = LSA_ERROR_RPC_NETLOGON_FAILED;
            bIsNetworkError = TRUE;
            BAIL_ON_LSA_ERROR(dwError);
        }

        /* Now setup the Schannel session */

        nt_status = NetrOpenSchannel(netr_b,
                                     pMachAcctInfo->pwszMachineAccount,
                                     pwszDomainController,
                                     pwszServerName,
                                     pwszPrimaryShortDomain,
                                     pMachAcctInfo->pwszHostname,
                                     pMachAcctInfo->pwszMachinePassword,
                                     &gSchannelCreds,
                                     &ghSchannelBinding);

        if (nt_status != STATUS_SUCCESS)
        {
            dwError = LSA_ERROR_RPC_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        gpSchannelCreds = &gSchannelCreds;
        gpSchannelRes   = &gSchannelRes;
    }

    /* Time to do the authentication */

    dwError = LsaMbsToWc16s(pUserParams->pszAccountName, &pwszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    /* Get the data blob buffers */

    if (pUserParams->pass.chap.pChallenge)
        pChal = LsaDataBlobBuffer(pUserParams->pass.chap.pChallenge);

    if (pUserParams->pass.chap.pLM_resp) {
        pLMResp = LsaDataBlobBuffer(pUserParams->pass.chap.pLM_resp);
        LMRespLen = LsaDataBlobLength(pUserParams->pass.chap.pLM_resp);
    }

    if (pUserParams->pass.chap.pNT_resp) {
        pNTResp = LsaDataBlobBuffer(pUserParams->pass.chap.pNT_resp);
        NTRespLen = LsaDataBlobLength(pUserParams->pass.chap.pNT_resp);
    }

    nt_status = NetrSamLogonNetwork(ghSchannelBinding,
                                    &gSchannelCreds,
                                    pwszServerName,
                                    pwszShortDomain,
                                    pMachAcctInfo->pwszHostname,
                                    pwszUsername,
                                    pChal,
                                    pLMResp, LMRespLen,
                                    pNTResp, NTRespLen,
                                    2,                /* Network login */
                                    3,                /* Return NetSamInfo3 */
                                    &pValidationInfo,
                                    &dwAuthoritative);

    if (nt_status != STATUS_SUCCESS)
    {
        dwError = LSA_ERROR_RPC_NETLOGON_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* Translate the returned NetrValidationInfo to the
       LSA_AUTH_USER_INFO out param */

    dwError = LsaAllocateMemory(sizeof(LSA_AUTH_USER_INFO), (PVOID*)ppUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaCopyNetrUserInfo3(*ppUserInfo, pValidationInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    if (hPwdDb)
    {
        if (pMachAcctInfo) {
            LwpsFreePasswordInfo(hPwdDb, pMachAcctInfo);
        }

        LwpsClosePasswordStore(hPwdDb);
        hPwdDb = (HANDLE)NULL;
    }

    if (netr_b)
    {
        FreeNetlogonBinding(&netr_b);
        netr_b = NULL;
    }

    if (bChangedToken)
    {
        LwIoSetThreadAccessToken(pOldToken);
    }
    if (pOldToken != NULL)
    {
        LwIoDeleteAccessToken(pOldToken);
    }
    if (pAccessToken != NULL)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    if (pValidationInfo) {
        NetrFreeMemory((void*)pValidationInfo);
    }

    LSA_SAFE_FREE_MEMORY(pszHostname);
    LSA_SAFE_FREE_MEMORY(pszServerName);

    LSA_SAFE_FREE_MEMORY(pwszUsername);
    LSA_SAFE_FREE_MEMORY(pwszDomainController);
    LSA_SAFE_FREE_MEMORY(pwszServerName);
    LSA_SAFE_FREE_MEMORY(pwszShortDomain);
    LSA_SAFE_FREE_MEMORY(pwszPrimaryShortDomain);

    pthread_mutex_unlock(&gSchannelLock);

    return dwError;

error:
    LsaFreeAuthUserInfo(ppUserInfo);

    goto cleanup;
}

static
VOID
AD_ClearSchannelState(
    VOID
    )
{
    pthread_mutex_lock(&gSchannelLock);

    if (ghSchannelBinding)
    {
        NetrCloseSchannel(ghSchannelBinding);

        ghSchannelBinding = NULL;

        memset(&gSchannelCreds, 0, sizeof(gSchannelCreds));
        gpSchannelCreds = NULL;

        memset(&gSchannelRes, 0, sizeof(gSchannelRes));
        gpSchannelRes = NULL;
    }

    pthread_mutex_unlock(&gSchannelLock);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
