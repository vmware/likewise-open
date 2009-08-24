/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        lsa_lookupsids2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupSids2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvLookupSids2(
    handle_t hBinding,
    POLICY_HANDLE hPolicy,
    SidArray *pSids,
    RefDomainList **ppDomains,
    TranslatedNameArray2 *pNamesArray,
    uint16 level,
    uint32 *pdwCount,
    uint32 unknown1,
    uint32 unknown2    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    RPCSTATUS rpcstatus = 0;
    PPOLICY_CONTEXT pPolCtx = NULL;
    handle_t hLsaBinding = NULL;
    LW_PIO_ACCESS_TOKEN pAccessToken = NULL;
    PolicyHandle hDcPolicy;
    PSAM_DOMAIN_ENTRY pLocalDomain = NULL;
    PSAM_DOMAIN_ENTRY pBuiltinDomain = NULL;
    ACCOUNT_SIDS DomainAccounts = {0};
    ACCOUNT_SIDS LocalAccounts = {0};
    ACCOUNT_SIDS BuiltinAccounts = {0};
    PSTR pszDomainFqdn = NULL;
    PSTR pszDcName = NULL;
    PWSTR pwszDcName = NULL;
    DWORD dwSidsNum = 0;
    SidArray SidsArray = {0};
    DWORD i = 0;
    RefDomainList *pRemoteDomains = NULL;
    TranslatedName *pRemoteNames =  NULL;
    DWORD dwRemoteNamesCount = 0;
    DWORD dwDomIndex = 0;
    DWORD dwLocalDomIndex = 0;
    DWORD dwBuiltinDomIndex = 0;
    LsaDomainInfo *pLocalDomainInfo = NULL;
    LsaDomainInfo *pBuiltinDomainInfo = NULL;
    PDWORD pdwLocalRids = NULL;
    PDWORD pdwBuiltinRids = NULL;
    PDWORD pdwLocalTypes = NULL;
    PWSTR *ppwszLocalNames = NULL;
    PDWORD pdwBuiltinTypes = NULL;
    PWSTR *ppwszBuiltinNames = NULL;
    DWORD dwUnknownSidsNum = 0;
    RefDomainList *pDomains = NULL;
    TranslatedNameArray2 NamesArray = {0};

    pPolCtx = (PPOLICY_CONTEXT)hPolicy;

    if (pPolCtx == NULL || pPolCtx->Type != LsaContextPolicy) {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    dwSidsNum = pSids->num_sids;

    ntStatus = LsaSrvAllocateMemory(OUT_PPVOID(&(NamesArray.names)),
                                    sizeof(NamesArray.names[0]) * dwSidsNum);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(OUT_PPVOID(&pDomains),
                                    sizeof(*pDomains));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(OUT_PPVOID(&pDomains->domains),
                                    sizeof(*pDomains->domains) * (dwSidsNum + 2));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvSelectAccountsByDomainSid(pPolCtx,
                                               pSids,
                                               pSids->num_sids,
                                               &DomainAccounts,
                                               &LocalAccounts,
                                               &BuiltinAccounts);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Check remote (resolving to DOMAIN\name) SIDs first.
     * This means asking the DC.
     */
    if (DomainAccounts.dwCount)
    {
        dwError = LWNetGetCurrentDomain(&pszDomainFqdn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LWNetGetDomainController(pszDomainFqdn,
                                           &pszDcName);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = LwIoGetThreadAccessToken(&pAccessToken);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        rpcstatus = InitLsaBindingDefault(&hLsaBinding,
                                          pszDcName,
                                          pAccessToken);
        if (rpcstatus) {
            dwError = LW_ERROR_RPC_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaMbsToWc16s(pszDcName, &pwszDcName);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = LsaOpenPolicy2(hLsaBinding,
                                  pwszDcName,
                                  NULL,
                                  dwPolicyAccessMask,
                                  &hDcPolicy);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        SidsArray.num_sids = DomainAccounts.dwCount;

        dwError = LwAllocateMemory(
                           sizeof(SidsArray.sids[0]) * SidsArray.num_sids,
                           OUT_PPVOID(&SidsArray.sids));
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; i < SidsArray.num_sids; i++)
        {
            SidsArray.sids[i].sid = DomainAccounts.ppSids[i];
        }

        ntStatus = LsaLookupSids(hLsaBinding,
                                 &hDcPolicy,
                                 &SidsArray,
                                 &pRemoteDomains,
                                 &pRemoteNames,
                                 level,
                                 &dwRemoteNamesCount);
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
            ntStatus != STATUS_NONE_MAPPED) {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        for (i = 0; i < pRemoteDomains->count; i++) {
            LsaDomainInfo *pSrcDomInfo = NULL;
            LsaDomainInfo *pDstDomInfo = NULL;

            dwDomIndex  = pDomains->count;
            pSrcDomInfo = &(pRemoteDomains->domains[i]);
            pDstDomInfo = &(pDomains->domains[dwDomIndex]);

            ntStatus = LsaSrvDuplicateUnicodeStringEx(&pDstDomInfo->name,
                                                      &pSrcDomInfo->name);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            ntStatus = LsaSrvDuplicateSid(&pDstDomInfo->sid,
                                          pSrcDomInfo->sid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pDomains->count  = (++dwDomIndex);
        }

        for (i = 0; i < dwRemoteNamesCount ; i++) {
            DWORD iTransName = DomainAccounts.pdwIndices[i];
            TranslatedName *pRemoteName = &(pRemoteNames[i]);
            TranslatedName2 *pName = &(NamesArray.names[iTransName]);

            ntStatus = LsaSrvDuplicateUnicodeString(&pName->name,
                                                    &pRemoteName->name);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pName->type      = pRemoteName->type;
            pName->sid_index = pRemoteName->sid_index;
            pName->unknown1  = 0;
        }

        NamesArray.count += dwRemoteNamesCount;

        ntStatus = LsaClose(hLsaBinding, &hDcPolicy);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    /*
     * Check local (resolving to MACHINE\name) SIDs.
     * Call our local \samr server to lookup in MACHINE domain.
     */
    if (LocalAccounts.dwCount)
    {
        dwLocalDomIndex  = pDomains->count;
        pLocalDomainInfo = &(pDomains->domains[dwLocalDomIndex]);

        ntStatus = LsaSrvGetLocalSamDomain(pPolCtx,
                                           FALSE,    /* !BUILTIN */
                                           &pLocalDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvInitUnicodeStringEx(&pLocalDomainInfo->name,
                                             pLocalDomain->pwszName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvDuplicateSid(&pLocalDomainInfo->sid,
                                      pLocalDomain->pSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwAllocateMemory(
                              sizeof(pdwLocalRids[0]) * LocalAccounts.dwCount,
                              OUT_PPVOID(&pdwLocalRids));
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; i < LocalAccounts.dwCount; i++)
        {
            PSID pSid = LocalAccounts.ppSids[i];
            pdwLocalRids[i] = pSid->SubAuthority[pSid->SubAuthorityCount - 1];
        }

        ntStatus = SamrLookupRids(pPolCtx->hSamrBinding,
                                  &pLocalDomain->hDomain,
                                  LocalAccounts.dwCount,
                                  pdwLocalRids,
                                  &ppwszLocalNames,
                                  &pdwLocalTypes);
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
            ntStatus != STATUS_NONE_MAPPED) {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        for (i = 0; i < LocalAccounts.dwCount; i++)
        {
            DWORD iTransName = LocalAccounts.pdwIndices[i];
            TranslatedName2 *pName = &(NamesArray.names[iTransName]);

            ntStatus = LsaSrvInitUnicodeString(&pName->name,
                                               ppwszLocalNames[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pName->type      = pdwLocalTypes[i];
            pName->sid_index = dwLocalDomIndex;
            pName->unknown1  = 0;
        }

        pDomains->count  = dwLocalDomIndex + 1;
        NamesArray.count += LocalAccounts.dwCount;
    }

    if (BuiltinAccounts.dwCount)
    {
        dwBuiltinDomIndex  = pDomains->count;
        pBuiltinDomainInfo = &(pDomains->domains[dwBuiltinDomIndex]);

        ntStatus = LsaSrvGetLocalSamDomain(pPolCtx,
                                           TRUE,    /* BUILTIN */
                                           &pBuiltinDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvInitUnicodeStringEx(&pLocalDomainInfo->name,
                                             pBuiltinDomain->pwszName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvDuplicateSid(&pLocalDomainInfo->sid,
                                      pBuiltinDomain->pSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwError = LwAllocateMemory(
                            sizeof(pdwBuiltinRids[0]) * BuiltinAccounts.dwCount,
                            OUT_PPVOID(&pdwBuiltinRids));
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; i < BuiltinAccounts.dwCount; i++)
        {
            PSID pSid = BuiltinAccounts.ppSids[i];
            pdwBuiltinRids[i] = pSid->SubAuthority[pSid->SubAuthorityCount - 1];
        }

        ntStatus = SamrLookupRids(pPolCtx->hSamrBinding,
                                  &pBuiltinDomain->hDomain,
                                  BuiltinAccounts.dwCount,
                                  pdwBuiltinRids,
                                  &ppwszBuiltinNames,
                                  &pdwBuiltinTypes);
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
            ntStatus != STATUS_NONE_MAPPED) {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        for (i = 0; i < BuiltinAccounts.dwCount; i++)
        {
            DWORD iTransName = BuiltinAccounts.pdwIndices[i];
            TranslatedName2 *pName = &(NamesArray.names[iTransName]);

            ntStatus = LsaSrvInitUnicodeString(&pName->name,
                                               ppwszBuiltinNames[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pName->type      = pdwBuiltinTypes[i];
            pName->sid_index = dwBuiltinDomIndex;
            pName->unknown1  = 0;
        }

        pDomains->count  = dwBuiltinDomIndex + 1;
        NamesArray.count += BuiltinAccounts.dwCount;
    }

    /* Check if all SIDs have been mapped to decide about
       returned status */
    for (i = 0; i < NamesArray.count; i++)
    {
        if (NamesArray.names[i].type == SID_TYPE_UNKNOWN)
        {
             dwUnknownSidsNum++;
        }
    }

    if (dwUnknownSidsNum > 0)
    {
        if (dwUnknownSidsNum < NamesArray.count)
        {
            ntStatus = LW_STATUS_SOME_NOT_MAPPED;
        }
        else
        {
            ntStatus = STATUS_NONE_MAPPED;
        }
    }

    /* windows seems to set max_size to multiple of 32 */
    pDomains->max_size = ((pDomains->count / 32) + 1) * 32;

    *ppDomains         = pDomains;
    pNamesArray->count = NamesArray.count;
    pNamesArray->names = NamesArray.names;
    *pdwCount          = NamesArray.count;

cleanup:
    if (hLsaBinding)
    {
        FreeLsaBinding(&hLsaBinding);
    }

    if (pszDomainFqdn)
    {
        LWNetFreeString(pszDomainFqdn);
    }

    if (pszDcName)
    {
        LWNetFreeString(pszDcName);
    }

    LsaSrvFreeAccountSids(&DomainAccounts);
    LsaSrvFreeAccountSids(&LocalAccounts);
    LsaSrvFreeAccountSids(&BuiltinAccounts);

    if (pRemoteDomains)
    {
        LsaRpcFreeMemory(pRemoteDomains);
    }

    if (pRemoteNames)
    {
        LsaRpcFreeMemory(pRemoteNames);
    }

    LsaSrvSamDomainEntryFree(&pLocalDomain);
    LsaSrvSamDomainEntryFree(&pBuiltinDomain);

    if (ppwszLocalNames)
    {
        SamrFreeMemory(ppwszLocalNames);
    }

    if (pdwLocalTypes)
    {
        SamrFreeMemory(pdwLocalTypes);
    }

    if (ppwszBuiltinNames)
    {
        SamrFreeMemory(ppwszBuiltinNames);
    }

    if (pdwLocalTypes)
    {
        SamrFreeMemory(pdwBuiltinTypes);
    }

    LW_SAFE_FREE_MEMORY(pdwLocalRids);
    LW_SAFE_FREE_MEMORY(pdwBuiltinRids);

    return ntStatus;

error:
    *ppDomains         = NULL;
    pNamesArray->count = 0;
    pNamesArray->names = NULL;
    *pdwCount          = 0;
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
