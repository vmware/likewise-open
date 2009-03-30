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
 *        lsa_lookupnames2.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupNames2 function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvLookupNames2(
    handle_t b,
    POLICY_HANDLE hPolicy,
    uint32 num_names,
    UnicodeStringEx *names,
    RefDomainList **domains,
    TranslatedSidArray2 *sids,
    uint16 level,
    uint32 *count,
    uint32 unknown1,
    uint32 unknown2
    )
{
    const DWORD dwSamrConnAccess = SAMR_ACCESS_CONNECT_TO_SERVER;
    const DWORD dwSamrDomainAccess = DOMAIN_ACCESS_LOOKUP_INFO_1 |
                                     DOMAIN_ACCESS_LOOKUP_ALIAS;
    const DWORD dwPolicyAccessMask = LSA_ACCESS_LOOKUP_NAMES_SIDS;

    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    RPCSTATUS rpcstatus = 0;
    PPOLICY_CONTEXT pPolCtx = NULL;
    HANDLE hDirectory = NULL;
    PSTR pszSamrLpcSocketPath = NULL;
    handle_t hSamrBinding = NULL;
    handle_t hLsaBinding = NULL;
    PIO_ACCESS_TOKEN pAccessToken = NULL;
    PolicyHandle hConn;
    PolicyHandle hDomain;
    PolicyHandle hDcPolicy;
    PSID pDomainSid = NULL;
    PSAMR_DOMAIN pSamrDomains = NULL;
    PSAMR_DOMAIN pSamrDomain = NULL;
    SAMR_DOMAIN SamrDomain;
    CHAR szHostname[64];
    PSTR pszDomainName = NULL;
    PWSTR pwszSystemName = NULL;
    PWSTR pwszName = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszAcctName = NULL;
    PWSTR *ppwszLocalNames = NULL;
    DWORD dwLocalNamesNum = 0;
    PWSTR *ppwszBuiltinNames = NULL;
    DWORD dwBuiltinNamesNum = 0;
    PWSTR *ppwszDomainNames = NULL;
    DWORD dwDomainNamesNum = 0;
    PSTR pszDcName = NULL;
    PWSTR pwszDcName = NULL;
    RefDomainList *pRemoteDomains = NULL;
    TranslatedSid2 *pRemoteSids = NULL;
    DWORD dwRemoteSidsCount = 0;
    DWORD *dwRids = NULL;
    DWORD *dwTypes = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;
    DWORD dwDomIndex = 0;
    UnicodeString *pNonLocalNames = NULL;
    TranslatedSidArray2 *pSidArray = NULL;
    RefDomainList *pDomains = NULL;

    memset(&SamrDomain, 0, sizeof(SamrDomain));
    memset(szHostname, 0, sizeof(szHostname));

    pPolCtx = (PPOLICY_CONTEXT)hPolicy;

    if (pPolCtx == NULL || pPolCtx->Type != LsaContextPolicy) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    hDirectory = pPolCtx->hDirectory;

    status = LsaSrvAllocateMemory((void**)&pSidArray,
                                  sizeof(*pSidArray),
                                  pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LsaSrvAllocateMemory((void**)&(pSidArray->sids),
                                  sizeof(*pSidArray->sids) * num_names,
                                  pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LsaSrvAllocateMemory((void**)&pSamrDomain,
                                  sizeof(*pSamrDomain) * num_names,
                                  pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    pPolCtx->dwSamrDomainsNum = num_names;
    pPolCtx->pSamrDomain      = pSamrDomain;

    status = LsaSrvAllocateMemory((void**)&ppwszLocalNames,
                                  sizeof(*ppwszLocalNames) * num_names,
                                  pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    status = LsaSrvAllocateMemory((void**)&ppwszBuiltinNames,
                                  sizeof(*ppwszBuiltinNames) * num_names,
                                  pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    dwError = gethostname(szHostname, sizeof(szHostname));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SamrSrvConfigGetLpcSocketPath(&pszSamrLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    rpcstatus = InitSamrBindingFull(&hSamrBinding,
                                    "ncalrpc",
                                    szHostname,
                                    pszSamrLpcSocketPath,
                                    NULL,
                                    NULL,
                                    NULL);
    if (rpcstatus) {
        dwError = LSA_ERROR_RPC_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaMbsToWc16s((PSTR)szHostname, &pwszSystemName);
    BAIL_ON_LSA_ERROR(dwError);

    status = SamrConnect2(hSamrBinding,
                          pwszSystemName,
                          dwSamrConnAccess,
                          &hConn);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < num_names; i++) {
        UnicodeStringEx *name = &(names[0]);

        pwszName = GetFromUnicodeStringEx(name);
        if (!pwszName) {
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        status = LsaSrvParseAccountName(pwszName,
                                        &pwszDomainName,
                                        &pwszAcctName);
        BAIL_ON_NTSTATUS_ERROR(status);


        memset(&SamrDomain, 0, sizeof(SamrDomain));

        status = LsaSrvGetSamrDomain(pPolCtx,
                                     pwszDomainName,
                                     &SamrDomain);
        if (status == STATUS_NO_SUCH_DOMAIN) {

            status = SamrLookupDomain(hSamrBinding,
                                      &hConn,
                                      pwszDomainName,
                                      &pDomainSid);
            if (status == STATUS_NO_SUCH_DOMAIN) {

                SamrDomain.pwszName = pwszDomainName;
                SamrDomain.pSid     = NULL;
                SamrDomain.bLocal   = FALSE;

                status = LsaSrvSetSamrDomain(pPolCtx,
                                             &SamrDomain);
                BAIL_ON_NTSTATUS_ERROR(status);

            } else if (status != STATUS_SUCCESS) {
                BAIL_ON_NTSTATUS_ERROR(status);
            }

            status = SamrOpenDomain(hSamrBinding,
                                    &hConn,
                                    dwSamrDomainAccess,
                                    pDomainSid,
                                    &hDomain);
            BAIL_ON_NTSTATUS_ERROR(status);

            SamrDomain.pwszName = pwszDomainName;
            SamrDomain.pSid     = pDomainSid;
            SamrDomain.bLocal   = TRUE;
            SamrDomain.hDomain  = hDomain;

            status = LsaSrvSetSamrDomain(pPolCtx,
                                         &SamrDomain);
            BAIL_ON_NTSTATUS_ERROR(status);


        } else if (status != STATUS_SUCCESS) {
            BAIL_ON_NTSTATUS_ERROR(status);
        }


        if (SamrDomain.bLocal) {
            dwError = LsaWc16sToMbs(SamrDomain.pwszName, &pszDomainName);
            BAIL_ON_LSA_ERROR(dwError);

            if (!strcasecmp(pszDomainName, "BUILTIN")) {
                ppwszBuiltinNames[dwBuiltinNamesNum] = wc16sdup(pwszAcctName);
                BAIL_ON_NO_MEMORY(ppwszBuiltinNames[dwBuiltinNamesNum]);

                status = LsaSrvAddDepMemory(ppwszBuiltinNames[dwBuiltinNamesNum],
                                            ppwszBuiltinNames);
                BAIL_ON_NTSTATUS_ERROR(status);

                dwBuiltinNamesNum++;

            } else {
                ppwszLocalNames[dwLocalNamesNum] = wc16sdup(pwszAcctName);
                BAIL_ON_NO_MEMORY(ppwszLocalNames[dwLocalNamesNum]);

                status = LsaSrvAddDepMemory(ppwszLocalNames[dwLocalNamesNum],
                                            ppwszLocalNames);
                BAIL_ON_NTSTATUS_ERROR(status);

                dwLocalNamesNum++;
            }

        } else {
            ppwszDomainNames[dwDomainNamesNum] = wc16sdup(pwszName);
            BAIL_ON_NO_MEMORY(ppwszDomainNames[dwDomainNamesNum]);

            status = LsaSrvAddDepMemory(ppwszDomainNames[dwDomainNamesNum],
                                        ppwszDomainNames);
            BAIL_ON_NTSTATUS_ERROR(status);

            dwDomainNamesNum++;
        }


        if (pwszName) {
            LSA_SAFE_FREE_MEMORY(pwszName);
            pwszName = NULL;
        }

        if (pwszDomainName) {
            SamrSrvFreeMemory(pwszDomainName);
            pwszDomainName = NULL;
        }

        if (pwszAcctName) {
            SamrSrvFreeMemory(pwszAcctName);
            pwszAcctName = NULL;
        }

        if (dwRids) {
            SamrFreeMemory(dwRids);
            dwRids = NULL;
        }

        if (dwTypes) {
            SamrFreeMemory(dwTypes);
            dwRids = NULL;
        }

        if (pszDomainName) {
            LSA_SAFE_FREE_STRING(pszDomainName);
            pszDomainName = NULL;
        }
    }

    if (dwDomainNamesNum) {
        status = LwIoGetThreadAccessToken(&pAccessToken);
        BAIL_ON_NTSTATUS_ERROR(status);

        rpcstatus = InitLsaBindingDefault(&hLsaBinding,
                                          pszDcName,
                                          pAccessToken);
        if (rpcstatus) {
            dwError = LSA_ERROR_RPC_ERROR;
            BAIL_ON_LSA_ERROR(dwError)
        }

        dwError = LsaMbsToWc16s(pszDcName, &pwszDcName);
        BAIL_ON_LSA_ERROR(dwError);

        status = LsaOpenPolicy2(hLsaBinding,
                                pwszDcName,
                                NULL,
                                dwPolicyAccessMask,
                                &hDcPolicy);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaLookupNames2(hLsaBinding,
                                 &hDcPolicy,
                                 dwDomainNamesNum,
                                 ppwszDomainNames,
                                 &pRemoteDomains,
                                 &pRemoteSids,
                                 level,
                                 &dwRemoteSidsCount);

        if (status != STATUS_SUCCESS ||
            status != STATUS_SOME_UNMAPPED ||
            status != STATUS_NONE_MAPPED) {
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        for (i = 0; i < pRemoteDomains->count; i++) {
            LsaDomainInfo *pDomInfo = &(pRemoteDomains->domains[i]);
            LsaDomainInfo *pInfo = &(pDomains->domains[i]);

            status = CopyUnicodeStringEx(&pInfo->name, &pDomInfo->name);
            BAIL_ON_NTSTATUS_ERROR(status);

            status = LsaSrvAddDepMemory(pInfo->name.string,
                                        pDomains->domains);
            BAIL_ON_NTSTATUS_ERROR(status);

            status = RtlDuplicateSid(&pInfo->sid, pDomInfo->sid);
            BAIL_ON_NTSTATUS_ERROR(status);

            status = LsaSrvAddDepMemory(pInfo->sid,
                                        pDomains->domains);
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        for (i = 0; i < dwRemoteSidsCount ; i++) {
            TranslatedSid2 *pRemoteSid = &(pRemoteSids[i]);
            TranslatedSid2 *pSid = &(pSidArray->sids[i]);

            pSid->type     = pRemoteSid->type;
            pSid->rid      = pRemoteSid->rid;
            pSid->index    = pRemoteSid->index;
            pSid->unknown1 = pRemoteSid->unknown1;
        }

        pSidArray->count += dwRemoteSidsCount;

        if (pRemoteDomains) {
            LsaFreeMemory(pRemoteDomains);
        }

        if (pRemoteSids) {
            LsaFreeMemory(pRemoteSids);
        }

        status = LsaClose(&hDcPolicy);
        BAIL_ON_NTSTATUS_ERROR(status);

        FreeLsaBinding(&hLsaBinding);
    }

    if (dwLocalNamesNum) {
        dwDomIndex = pDomains->count;
        LsaDomainInfo *pLocalDomainInfo = &(pDomains->domains[dwDomIndex]);

        status = LsaSrvGetLocalSamrDomain(pPolCtx,
                                          FALSE,    /* !BUILTIN */
                                          &SamrDomain);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = InitUnicodeStringEx(&pLocalDomainInfo->name,
                                     SamrDomain.pwszName);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaSrvAddDepMemory(pLocalDomainInfo->name.string,
                                    pDomains->domains);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = RtlDuplicateSid(&pLocalDomainInfo->sid, SamrDomain.pSid);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaSrvAddDepMemory(pLocalDomainInfo->sid,
                                    pDomains->domains);
        BAIL_ON_NTSTATUS_ERROR(status);


        status = SamrLookupNames(hSamrBinding,
                                 &SamrDomain.hDomain,
                                 dwLocalNamesNum,
                                 ppwszLocalNames,
                                 &dwRids,
                                 &dwTypes,
                                 &dwCount);
        if (status != STATUS_SUCCESS ||
            status != STATUS_SOME_UNMAPPED ||
            status != STATUS_NONE_MAPPED) {
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        for (i = 0; i < dwCount; i++) {
            TranslatedSid2 *pSid = &(pSidArray->sids[i + pSidArray->count]);

            pSid->type     = dwTypes[i];
            pSid->rid      = dwRids[i];
            pSid->index    = dwDomIndex;
            pSid->unknown1 = 0;
        }

        pDomains->count  = (++dwDomIndex);
        pSidArray->count += dwCount;
    }

    if (dwBuiltinNamesNum) {
        dwDomIndex = pDomains->count;
        LsaDomainInfo *pBuiltinDomainInfo = &(pDomains->domains[dwDomIndex]);

        status = LsaSrvGetLocalSamrDomain(pPolCtx,
                                          TRUE,      /* BUILTIN */
                                          &SamrDomain);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = InitUnicodeStringEx(&pBuiltinDomainInfo->name,
                                     SamrDomain.pwszName);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaSrvAddDepMemory(pBuiltinDomainInfo->name.string,
                                    pDomains->domains);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = RtlDuplicateSid(&pBuiltinDomainInfo->sid, SamrDomain.pSid);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaSrvAddDepMemory(pBuiltinDomainInfo->sid,
                                    pDomains->domains);
        BAIL_ON_NTSTATUS_ERROR(status);

        dwRids  = NULL;
        dwTypes = NULL;
        dwCount = 0;

        status = SamrLookupNames(hSamrBinding,
                                 &SamrDomain.hDomain,
                                 dwBuiltinNamesNum,
                                 ppwszBuiltinNames,
                                 &dwRids,
                                 &dwTypes,
                                 &dwCount);
        if (status != STATUS_SUCCESS ||
            status != STATUS_SOME_UNMAPPED ||
            status != STATUS_NONE_MAPPED) {
            BAIL_ON_NTSTATUS_ERROR(status);
        }

        for (i = 0; i < dwCount; i++) {
            TranslatedSid2 *pSid = &(pSidArray->sids[i + pSidArray->count]);

            pSid->type     = dwTypes[i];
            pSid->rid      = dwRids[i];
            pSid->index    = dwDomIndex;
            pSid->unknown1 = 0;
        }

        pDomains->count = (++dwDomIndex);
        pSidArray->count += dwCount;
    }

    status = SamrClose(hSamrBinding, &hConn);
    BAIL_ON_NTSTATUS_ERROR(status);

    *domains    = pDomains;
    sids->count = pSidArray->count;
    sids->sids  = pSidArray->sids;
    *count      = pSidArray->count;

cleanup:
    if (hSamrBinding) {
        FreeSamrBinding(&hSamrBinding);
    }

    if (pwszSystemName) {
        LSA_SAFE_FREE_MEMORY(pwszSystemName);
    }

    if (pwszName) {
        LSA_SAFE_FREE_MEMORY(pwszName);
    }

    if (pwszDomainName) {
        LsaSrvFreeMemory(pwszDomainName);
    }

    if (pwszAcctName) {
        LsaSrvFreeMemory(pwszAcctName);
    }

    if (dwRids) {
        SamrFreeMemory(dwRids);
    }

    if (dwTypes) {
        SamrFreeMemory(dwTypes);
    }

    if (pSidArray) {
        LsaSrvFreeMemory(pSidArray);
    }

    if (pszSamrLpcSocketPath) {
        LSA_SAFE_FREE_STRING(pszSamrLpcSocketPath);
    }

    return status;

error:
    if (pSidArray->sids) {
        LsaSrvFreeMemory(pSidArray->sids);
    }

    sids->count = 0;
    sids->sids  = NULL;
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
