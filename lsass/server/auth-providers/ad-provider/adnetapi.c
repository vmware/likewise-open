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

DWORD
AD_NetLookupObjectSidByName(
    IN PCSTR pszHostname,
    IN PCSTR pszLoginName,
    OUT PSTR* ppszObjectSid,
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
    DWORD dwNum_names = 1;
    DWORD dwLevel;
    DWORD dwCount = 0;
    PWSTR pwcNames[2];   
    RefDomainList* pDomains = NULL;
    TranslatedSid2* pSids = NULL;
    DomSid* pUsr_sid = NULL;
    DomSid* pDom_sid = NULL;
    DWORD dwSid_index = 0;          
    PWSTR pwcSid = NULL;
    PSTR pszObjectSid = NULL;
    BOOLEAN bIsNetworkError = FALSE;
           
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
    
    dwError = LsaMbsToWc16s(
                  pszLoginName,
                  &pwcNames[0]);
    BAIL_ON_LSA_ERROR(dwError);

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
                   dwNum_names, 
                   pwcNames,
                   &pDomains, 
                   &pSids,
                   dwLevel,
                   &dwCount);
    if (status != 0)
    {
        LSA_LOG_DEBUG("LsaLookupNames2() failed with %d (0x%08x)", status, status);
        dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_FAILED;
        if (IsDceRpcConnError(status))
        {
            bIsNetworkError = TRUE;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }    
    
    if (dwCount == 0)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwCount > 1)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (!pSids)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_FOUND_DUPLICATES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwSid_index = pSids[0].index;
    
    if (dwSid_index >= pDomains->count)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPNAME2_FAILED;
        BAIL_ON_LSA_ERROR(dwError);
    }        
    
    pDom_sid = pDomains->domains[dwSid_index].sid;
    
    SidAllocateResizedCopy(&pUsr_sid,
                           pDom_sid->subauth_count + 1,
                           pDom_sid);
    pUsr_sid->subauth[pUsr_sid->subauth_count - 1] = pSids[0].rid;     
   
    dwError = SidToString(pUsr_sid,
                          &pwcSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaWc16sToMbs(pwcSid,
                            &pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszObjectSid = pszObjectSid;
    
cleanup:

    LSA_SAFE_FREE_MEMORY(pwcHost);
    LSA_SAFE_FREE_MEMORY(pwcNames[0]);    
    
    if (pDomains)
    {
        LsaRpcFreeMemory((void*)pDomains);
    }
    
    if (pSids)
    {
        LsaRpcFreeMemory((void*)(pSids));
    }
    
    if (pUsr_sid){
          SidFree(pUsr_sid);
    }
    
    if (pwcSid)
    {
        SidFreeString(pwcSid);
    }

    status = LsaClose(lsa_binding, &lsa_policy);
    if (status != 0 && dwError == 0){
        dwError = LSA_ERROR_RPC_CLOSEPOLICY_FAILED;        
    }
    
    if (lsa_binding)
    {
        FreeLsaBinding(&lsa_binding);
    }

    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }

    return dwError;
   
error:

    *ppszObjectSid = NULL;

    LSA_SAFE_FREE_STRING(pszObjectSid);
    
    goto cleanup;
}

DWORD
AD_NetLookupObjectNameBySid(
    IN PCSTR     pszHostname,
    IN PCSTR     pszObjectSid,
    OUT PSTR*    ppszNetbiosName,
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
    SidPtr   sid_holder = {0};
    DWORD dwLevel = 1;
    DWORD dwCount = 0;   
    RefDomainList* pDomains = NULL;
    PSID pObjectSID = NULL;
    TranslatedName* name_array = NULL;
    PSTR pszDomainName = NULL;
    PSTR pszUsername = NULL;
    PSTR pszNetbiosName = NULL;
    BOOLEAN bIsNetworkError = FALSE;
           
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
   
    status = ParseSidString(
                    &pObjectSID,
                    pszObjectSid);
    if (status != 0)
    {
        dwError = LSA_ERROR_RPC_PARSE_SID_STRING;
        if (STATUS_UNHANDLED_EXCEPTION == status)
        {
            bIsNetworkError = TRUE;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    sid_holder.sid = pObjectSID;
    sid_array.sids = &sid_holder;
    sid_array.num_sids = 1;

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
                   &dwCount);
    if (status != 0)
    {
        LSA_LOG_DEBUG("LsaLookupSids() failed with %d (0x%08x)", status, status);
        dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_FAILED;
        if (IsDceRpcConnError(status))
        {
            bIsNetworkError = TRUE;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }    
    
    if (dwCount == 0)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwCount > 1)
    {
        dwError = LSA_ERORR_RPC_LSA_LOOKUPSIDS_FOUND_DUPLICATES;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (!name_array)
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        wchar16_t *domainname = NULL, *username = NULL;
        uint32 sid_index = name_array[0].sid_index;

        if (pDomains->domains[sid_index].name.size > 0)
        {
            domainname = pDomains->domains[sid_index].name.string;
            domainname[pDomains->domains[sid_index].name.len / 2] = 0;
            
            dwError = LsaWc16sToMbs(
                            domainname,
                            &pszDomainName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (name_array[0].name.size > 0)
        {
            username = name_array[0].name.string;
            username[name_array[0].name.len / 2] = 0;
            
            dwError = LsaWc16sToMbs(
                            username,
                            &pszUsername);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    
    if (IsNullOrEmptyString(pszDomainName) ||
        IsNullOrEmptyString(pszUsername))
    {
        dwError = LSA_ERROR_RPC_LSA_LOOKUPSIDS_NOT_FOUND;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaAllocateStringPrintf(
                    &pszNetbiosName,
                    "%s\\%s",
                    pszDomainName,
                    pszUsername);
    BAIL_ON_LSA_ERROR(dwError);

    if (pbIsNetworkError)
    {
        *pbIsNetworkError = bIsNetworkError;
    }
    
    *ppszNetbiosName = pszNetbiosName;
    
cleanup:

    LSA_SAFE_FREE_STRING(pszDomainName);
    LSA_SAFE_FREE_STRING(pszUsername);
    
    LSA_SAFE_FREE_MEMORY(pwcHost);   
    
    if (pDomains)
    {
        LsaRpcFreeMemory((void*)pDomains);
    }
    
    if (name_array)
    {
        LsaRpcFreeMemory((void*)(name_array));
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

    *ppszNetbiosName = NULL;

    LSA_SAFE_FREE_STRING(pszNetbiosName);
    
    LSA_LOG_ERROR("Failed to find user or group. [Error code: %d]", dwError);
    
    dwError = LSA_ERROR_NO_SUCH_USER_OR_GROUP;
    
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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
