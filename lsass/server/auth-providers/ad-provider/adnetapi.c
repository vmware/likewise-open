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

error:

    return dwError;
}

DWORD
AD_NetShutdownMemory(
    VOID
    )
{
    DWORD dwError = 0;

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
    PCSTR pszLoginId,
    PCSTR pszOldPassword,
    PCSTR pszNewPassword
    )
{
    DWORD dwError = 0;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszLoginId = NULL;
    PWSTR pwszOldPassword = NULL;
    PWSTR pwszNewPassword = NULL;

    BAIL_ON_INVALID_STRING(pszDomainName);
    BAIL_ON_INVALID_STRING(pszLoginId);

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
        dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(ppTranslatedSids[0]->pszNT4NameOrSid,
                                &pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszObjectSid = pszObjectSid;

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
    LSA_LOG_ERROR("Failed to find user or group. [Error code: %d]", dwError);
    dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;

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
    DomSid* pObject_sid = NULL;
    PWSTR pwcObjectSid = NULL;
    BOOLEAN bIsNetworkError = FALSE;
    DWORD i = 0;

    BAIL_ON_INVALID_STRING(pszHostname);
    dwError = LsaMbsToWc16s(
                  pszHostname,
                  &pwcHost);
    BAIL_ON_LSA_ERROR(dwError);

    rpcStatus = InitLsaBindingDefault(&lsa_binding, (const PBYTE)pszHostname);
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
        // Do not free pDom_sid, reference to pDomains
        DomSid* pDom_sid = NULL;


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

        pDom_sid = pDomains->domains[dwDomainSid_index].sid;

        if (pObject_sid)
        {
              SidFree(pObject_sid);
              pObject_sid = NULL;
        }
        SidAllocateResizedCopy(&pObject_sid,
                               pDom_sid->subauth_count + 1,
                               pDom_sid);
        pObject_sid->subauth[pObject_sid->subauth_count - 1] = pSids[i].rid;

        if (pwcObjectSid)
        {
            SidFreeString(pwcObjectSid);
            pwcObjectSid = NULL;
        }
        dwError = SidToString(pObject_sid,
                              &pwcObjectSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaWc16sToMbs(pwcObjectSid,
                                &ppTranslatedSids[i]->pszNT4NameOrSid);
        BAIL_ON_LSA_ERROR(dwError);
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
    if (pObject_sid)
    {
          SidFree(pObject_sid);
    }
    if (pwcObjectSid)
    {
        SidFreeString(pwcObjectSid);
    }
    status = LsaClose(lsa_binding, &lsa_policy);
    if (status != 0 && dwError == 0)
    {
        dwError = LSA_ERROR_RPC_CLOSEPOLICY_FAILED;
    }
    if (lsa_binding)
    {
        FreeLsaBinding(&lsa_binding);
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
    IN PCSTR     pszHostname,
    IN PCSTR     pszObjectSid,
    OUT PSTR*    ppszNT4Name,
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
        dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(ppTranslatedNames[0]->pszNT4NameOrSid,
                                &pszNT4Name);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszNT4Name = pszNT4Name;

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

    LSA_LOG_ERROR("Failed to find user or group. [Error code: %d]", dwError);

    dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;

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

    BAIL_ON_INVALID_STRING(pszHostname);

    dwError = LsaMbsToWc16s(
                  pszHostname,
                  &pwcHost);
    BAIL_ON_LSA_ERROR(dwError);

    rpcStatus = InitLsaBindingDefault(&lsa_binding, (const PBYTE)pszHostname);
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
        status = ParseSidString(
                        &pObjectSID,
                        ppszObjectSids[i]);
        if (status != 0)
        {
            dwError = LSA_ERROR_RPC_PARSE_SID_STRING;
            BAIL_ON_LSA_ERROR(dwError);
        }

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

        ObjectType = GetObjectType(name_array[i].type);

        if (ObjectType == AccountType_NotFound)
        {
            continue;
        }

        dwError = LsaAllocateMemory(
                            sizeof(*ppTranslatedNames[i]),
                            (PVOID*)&ppTranslatedNames[i]);
        BAIL_ON_LSA_ERROR(dwError);

        ppTranslatedNames[i]->ObjectType = ObjectType;

        if (name_array[i].name.len > 0)
        {
            dwError = LsaWc16snToMbs(
                           name_array[i].name.string,
                           &pszUsername,
                           name_array[i].name.len / 2);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (IsNullOrEmptyString(ppszDomainNames[name_array[i].sid_index]) ||
            IsNullOrEmptyString(pszUsername))
        {
            dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = ADGetDomainQualifiedString(
                    ppszDomainNames[name_array[i].sid_index],
                    pszUsername,
                    &ppTranslatedNames[i]->pszNT4NameOrSid);
        BAIL_ON_LSA_ERROR(dwError);

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

    if (pObjectSID){
        SidFree(pObjectSID);
    }

    status = LsaClose(lsa_binding, &lsa_policy);
    if (status != 0 && dwError == 0){
        dwError = LSA_ERROR_RPC_CLOSEPOLICY_FAILED;
    }

    if (lsa_binding)
    {
        FreeLsaBinding(&lsa_binding);
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

    dwError = LsaMbsToWc16s(pszDomainControllerName, &pwcDomainControllerName);
    BAIL_ON_LSA_ERROR(dwError);

    status = InitNetlogonBindingDefault(&netr_b,
                                        (PUCHAR)pszDomainControllerName);
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
AD_SidToString(
    PSID  pSid,
    PSTR* ppszSid
    )
{
    DWORD dwError = 0;
    PSTR  pszSid = NULL;
    wchar16_t* pwszSid = NULL;

    dwError = SidToString(pSid, &pwszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWc16sToMbs(
                  pwszSid,
                  &pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszSid = pszSid;

cleanup:

    if (pwszSid)
    {
        SidFreeString(pwszSid);
    }

    return dwError;

error:

    *ppszSid = NULL;

    LSA_SAFE_FREE_STRING(pszSid);

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

DWORD
AD_NetlogonAuthenticationUserEx(
    IN PSTR pszDomainController,
    IN PLSA_AUTH_USER_PARAMS pUserParams,
    OUT PLSA_AUTH_USER_INFO *ppUserInfo,
    OUT PBOOLEAN pbIsNetworkError
    )
{
    return LSA_ERROR_NOT_HANDLED;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
