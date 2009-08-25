/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software    2004-2009
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
 *        lsa_lookupnames3.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupNames3 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvLookupNames3(
    handle_t b,
    POLICY_HANDLE hPolicy,
    uint32 num_names,
    UnicodeStringEx *names,
    RefDomainList **domains,
    TranslatedSidArray3 *sids,
    uint16 level,
    uint32 *count,
    uint32 unknown1,
    uint32 unknown2
    )
{
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    RPCSTATUS rpcstatus = 0;
    PPOLICY_CONTEXT pPolCtx = NULL;
    handle_t hLsaBinding = NULL;
    PIO_ACCESS_TOKEN pAccessToken = NULL;
    PolicyHandle hDcPolicy;
    PSAM_DOMAIN_ENTRY pDomain = NULL;
    PSAM_DOMAIN_ENTRY pLocalDomain = NULL;
    PSAM_DOMAIN_ENTRY pBuiltinDomain = NULL;
    PWSTR pwszSystemName = NULL;
    ACCOUNT_NAMES DomainAccounts = {0};
    ACCOUNT_NAMES LocalAccounts = {0};
    ACCOUNT_NAMES BuiltinAccounts = {0};
    ACCOUNT_NAMES OtherAccounts = {0};
    PSTR pszDomainFqdn = NULL;
    PSTR pszDcName = NULL;
    PWSTR pwszDcName = NULL;
    RefDomainList *pRemoteDomains = NULL;
    TranslatedSid3 *pRemoteSids = NULL;
    DWORD dwRemoteSidsCount = 0;
    DWORD *dwRids = NULL;
    DWORD *dwTypes = NULL;
    DWORD dwCount = 0;
    DWORD *dwLocalRids = NULL;
    DWORD *dwLocalTypes = NULL;
    DWORD *dwBuiltinRids = NULL;
    DWORD *dwBuiltinTypes = NULL;
    DWORD dwUnknownNamesNum = 0;
    DWORD i = 0;
    DWORD dwDomIndex = 0;
    DWORD dwLocalDomIndex = 0;
    DWORD dwBuiltinDomIndex = 0;
    LsaDomainInfo *pLocalDomainInfo = NULL;
    LsaDomainInfo *pBuiltinDomainInfo = NULL;
    TranslatedSidArray3 SidArray = {0};
    RefDomainList *pDomains = NULL;

    pPolCtx = (PPOLICY_CONTEXT)hPolicy;

    if (pPolCtx == NULL || pPolCtx->Type != LsaContextPolicy) {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    ntStatus = LsaSrvAllocateMemory((void**)&(SidArray.sids),
                                    sizeof(*SidArray.sids) * num_names);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory((void**)&pDomains,
                                    sizeof(*pDomains));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory((void**)&(pDomains->domains),
                                    sizeof(*pDomains->domains) * (num_names + 2));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvSelectAccountsByDomainName(pPolCtx,
                                                names,
                                                num_names,
                                                &DomainAccounts,
                                                &LocalAccounts,
                                                &BuiltinAccounts,
                                                &OtherAccounts);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    /*
     * Check remote (DOMAIN\name) names first.
     * This means asking the DC.
     */
    if (DomainAccounts.dwCount) {
        dwError = LWNetGetCurrentDomain(&pszDomainFqdn);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LWNetGetDomainController(pszDomainFqdn,
                                           &pszDcName);
        BAIL_ON_LSA_ERROR(dwError);

        ntStatus = LsaSrvGetSystemAccessToken(&pAccessToken);
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

        ntStatus = LsaLookupNames3(hLsaBinding,
                                   &hDcPolicy,
                                   DomainAccounts.dwCount,
                                   DomainAccounts.ppwszNames,
                                   &pRemoteDomains,
                                   &pRemoteSids,
                                   level,
                                   &dwRemoteSidsCount);

        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
            ntStatus != STATUS_NONE_MAPPED) {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        for (i = 0; i < pRemoteDomains->count; i++) {
            LsaDomainInfo *pDomInfo = NULL;
            LsaDomainInfo *pInfo = NULL;

            dwDomIndex = pDomains->count;
            pDomInfo   = &(pRemoteDomains->domains[dwDomIndex]);
            pInfo      = &(pDomains->domains[dwDomIndex]);

            ntStatus = LsaSrvDuplicateUnicodeStringEx(&pInfo->name,
                                                    &pDomInfo->name);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            ntStatus = LsaSrvDuplicateSid(&pInfo->sid, pDomInfo->sid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pDomains->count  = (++dwDomIndex);
        }

        for (i = 0; i < dwRemoteSidsCount ; i++) {
            TranslatedSid3 *pRemoteSid = &(pRemoteSids[i]);
            DWORD iTransSid = DomainAccounts.pdwIndices[i];
            TranslatedSid3 *pSid = &(SidArray.sids[iTransSid]);
            PSID pAcctSid = NULL;

            ntStatus = LsaSrvDuplicateSid(&pAcctSid, pRemoteSid->sid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pSid->type     = pRemoteSid->type;
            pSid->index    = pRemoteSid->index;
            pSid->unknown1 = pRemoteSid->unknown1;
            pSid->sid      = pAcctSid;
        }

        SidArray.count += dwRemoteSidsCount;

        if (pRemoteDomains) {
            LsaRpcFreeMemory(pRemoteDomains);
        }

        if (pRemoteSids) {
            LsaRpcFreeMemory(pRemoteSids);
        }

        ntStatus = LsaClose(hLsaBinding, &hDcPolicy);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        FreeLsaBinding(&hLsaBinding);
    }

    /*
     * Check local (MACHINE\name) names.
     * Call our local \samr server to lookup in MACHINE domain.
     */
    if (LocalAccounts.dwCount) {
        dwLocalDomIndex  = pDomains->count;
        pLocalDomainInfo = &(pDomains->domains[dwLocalDomIndex]);

        ntStatus = LsaSrvGetLocalSamDomain(pPolCtx,
                                         FALSE,    /* !BUILTIN */
                                         &pDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvInitUnicodeStringEx(&pLocalDomainInfo->name,
                                           pDomain->pwszName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvDuplicateSid(&pLocalDomainInfo->sid,
                                    pDomain->pSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwRids  = NULL;
        dwTypes = NULL;
        dwCount = 0;

        ntStatus = SamrLookupNames(pPolCtx->hSamrBinding,
                                   &pDomain->hDomain,
                                   LocalAccounts.dwCount,
                                   LocalAccounts.ppwszNames,
                                   &dwRids,
                                   &dwTypes,
                                   &dwCount);
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
            ntStatus != STATUS_NONE_MAPPED) {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        for (i = 0; i < dwCount; i++) {
            DWORD iTransSid = LocalAccounts.pdwIndices[i];
            TranslatedSid3 *pSid = &(SidArray.sids[iTransSid]);
            PSID pDomainSid = pDomain->pSid;
            PSID pAcctSid = NULL;

            ntStatus = LsaSrvSidAppendRid(&pAcctSid, pDomainSid, dwRids[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pSid->type     = dwTypes[i];
            pSid->sid      = pAcctSid;
            pSid->index    = dwLocalDomIndex;
            pSid->unknown1 = 0;
        }

        pDomains->count  = dwLocalDomIndex + 1;
        SidArray.count   += dwCount;
    }

    /*
     * Check builtin (BUILTIN\name) names.
     * Call our local \samr server to lookup in BUILTIN domain.
     */
    if (BuiltinAccounts.dwCount) {
        dwBuiltinDomIndex  = pDomains->count;
        pBuiltinDomainInfo = &(pDomains->domains[dwBuiltinDomIndex]);

        ntStatus = LsaSrvGetLocalSamDomain(pPolCtx,
                                           TRUE,      /* BUILTIN */
                                           &pDomain);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvInitUnicodeStringEx(&pBuiltinDomainInfo->name,
                                             pDomain->pwszName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvDuplicateSid(&pBuiltinDomainInfo->sid,
                                      pDomain->pSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        dwRids  = NULL;
        dwTypes = NULL;
        dwCount = 0;

        ntStatus = SamrLookupNames(pPolCtx->hSamrBinding,
                                   &pDomain->hDomain,
                                   BuiltinAccounts.dwCount,
                                   BuiltinAccounts.ppwszNames,
                                   &dwRids,
                                   &dwTypes,
                                   &dwCount);
        if (ntStatus != STATUS_SUCCESS &&
            ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
            ntStatus != STATUS_NONE_MAPPED) {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        for (i = 0; i < dwCount; i++) {
            DWORD iTransSid = BuiltinAccounts.pdwIndices[i];
            TranslatedSid3 *pSid = &(SidArray.sids[iTransSid]);
            PSID pDomainSid = pDomain->pSid;
            PSID pAcctSid = NULL;

            ntStatus = LsaSrvSidAppendRid(&pAcctSid, pDomainSid, dwRids[i]);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pSid->type     = dwTypes[i];
            pSid->sid      = pAcctSid;
            pSid->index    = dwBuiltinDomIndex;
            pSid->unknown1 = 0;
        }

        pDomains->count = dwBuiltinDomIndex + 1;
        SidArray.count  += dwCount;
    }

    /*
     * Check names we're not sure about.
     * Call our local \samr server to lookup in MACHINE domain first.
     * If a name can't be found there, try BUILTIN domain before
     * considering it unknown.
     */
    if (OtherAccounts.dwCount)
    {
        if (pLocalDomainInfo == NULL)
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

            pDomains->count = dwLocalDomIndex + 1;
        }

        if (pBuiltinDomainInfo == NULL)
        {
            dwBuiltinDomIndex  = pDomains->count;
            pBuiltinDomainInfo = &(pDomains->domains[dwBuiltinDomIndex]);

            ntStatus = LsaSrvGetLocalSamDomain(pPolCtx,
                                               TRUE,      /* BUILTIN */
                                               &pBuiltinDomain);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            ntStatus = LsaSrvInitUnicodeStringEx(&pBuiltinDomainInfo->name,
                                                 pBuiltinDomain->pwszName);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            ntStatus = LsaSrvDuplicateSid(&pBuiltinDomainInfo->sid,
                                          pBuiltinDomain->pSid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            pDomains->count = dwBuiltinDomIndex + 1;
        }

        ntStatus = SamrLookupNames(pPolCtx->hSamrBinding,
                                   &pLocalDomain->hDomain,
                                   OtherAccounts.dwCount,
                                   OtherAccounts.ppwszNames,
                                   &dwLocalRids,
                                   &dwLocalTypes,
                                   &dwCount);
        if (ntStatus == LW_STATUS_SOME_NOT_MAPPED ||
            ntStatus == STATUS_NONE_MAPPED)
        {
            ntStatus = SamrLookupNames(pPolCtx->hSamrBinding,
                                       &pBuiltinDomain->hDomain,
                                       OtherAccounts.dwCount,
                                       OtherAccounts.ppwszNames,
                                       &dwBuiltinRids,
                                       &dwBuiltinTypes,
                                       &dwCount);
            if (ntStatus != STATUS_SUCCESS &&
                ntStatus != LW_STATUS_SOME_NOT_MAPPED &&
                ntStatus != STATUS_NONE_MAPPED)
            {
                BAIL_ON_NTSTATUS_ERROR(ntStatus);
            }

        }
        else if (ntStatus != STATUS_SUCCESS)
        {
            BAIL_ON_NTSTATUS_ERROR(ntStatus);
        }

        for (i = 0; i < dwCount; i++)
        {
            DWORD iTransSid = OtherAccounts.pdwIndices[i];
            TranslatedSid3 *pSid   = &(SidArray.sids[iTransSid]);
            PSID pLocalDomainSid   = pLocalDomain->pSid;
            PSID pBuiltinDomainSid = pBuiltinDomain->pSid;
            PSID pAcctSid          = NULL;

            if (dwLocalTypes &&
                dwLocalTypes[i] != SID_TYPE_UNKNOWN)
            {
                ntStatus = LsaSrvSidAppendRid(&pAcctSid,
                                              pLocalDomainSid,
                                              dwLocalRids[i]);
                BAIL_ON_NTSTATUS_ERROR(ntStatus);

                pSid->type     = dwLocalTypes[i];
                pSid->sid      = pAcctSid;
                pSid->index    = dwLocalDomIndex;
                pSid->unknown1 = 0;

            }
            else
            {
                ntStatus = LsaSrvSidAppendRid(&pAcctSid,
                                              pBuiltinDomainSid,
                                              dwBuiltinRids[i]);
                BAIL_ON_NTSTATUS_ERROR(ntStatus);

                pSid->type     = dwBuiltinTypes[i];
                pSid->sid      = pAcctSid;
                pSid->index    = dwBuiltinDomIndex;
                pSid->unknown1 = 0;
            }
        }

        SidArray.count += dwCount;
    }

    /* Check if all names have been mapped to decide about
       returned status */
    for (i = 0; i < SidArray.count; i++)
    {
        if (SidArray.sids[i].type == SID_TYPE_UNKNOWN)
        {
             dwUnknownNamesNum++;
        }
    }

    if (dwUnknownNamesNum > 0)
    {
        if (dwUnknownNamesNum < SidArray.count)
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

    *domains    = pDomains;
    sids->count = SidArray.count;
    sids->sids  = SidArray.sids;
    *count      = SidArray.count;

cleanup:
    if (pAccessToken)
    {
        LwIoDeleteAccessToken(pAccessToken);
    }

    if (pwszSystemName)
    {
        LW_SAFE_FREE_MEMORY(pwszSystemName);
    }

    if (pszDomainFqdn)
    {
        LWNetFreeString(pszDomainFqdn);
    }

    if (pszDcName)
    {
        LWNetFreeString(pszDcName);
    }

    LsaSrvFreeAccountNames(&DomainAccounts);
    LsaSrvFreeAccountNames(&LocalAccounts);
    LsaSrvFreeAccountNames(&BuiltinAccounts);
    LsaSrvFreeAccountNames(&OtherAccounts);

    if (dwRids)
    {
        SamrFreeMemory(dwRids);
    }

    if (dwTypes)
    {
        SamrFreeMemory(dwTypes);
    }

    if (dwLocalRids)
    {
        SamrFreeMemory(dwLocalRids);
    }

    if (dwLocalTypes)
    {
        SamrFreeMemory(dwLocalTypes);
    }

    if (dwBuiltinRids)
    {
        SamrFreeMemory(dwBuiltinRids);
    }

    if (dwBuiltinTypes)
    {
        SamrFreeMemory(dwBuiltinTypes);
    }

    LsaSrvSamDomainEntryFree(&pDomain);
    LsaSrvSamDomainEntryFree(&pLocalDomain);
    LsaSrvSamDomainEntryFree(&pBuiltinDomain);

    return ntStatus;

error:
    if (pDomains)
    {
        LsaSrvFreeMemory(pDomains);
    }

    if (SidArray.sids)
    {
        LsaSrvFreeMemory(SidArray.sids);
    }

    *domains    = NULL;
    sids->count = 0;
    sids->sids  = NULL;
    *count      = 0;
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
