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
 *        lsa_lookupnames.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        LsaLookupNames function
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvParseAccountName(
    PWSTR pwszName,
    PWSTR *ppwszDomainName,
    PWSTR *ppwszAcctName
    );


NTSTATUS
LsaSrvGetSamrDomain(
    PPOLICY_CONTEXT pPolCtx,
    PWSTR pwszDomainName,
    PSAMR_DOMAIN pSamrDomain
    );


NTSTATUS
LsaSrvSetSamrDomain(
    PPOLICY_CONTEXT pPolCtx,
    PSAMR_DOMAIN pSamrDomain
    );


NTSTATUS
LsaSrvLookupNames(
    /* [in] */ handle_t IDL_handle,
    /* [in] */ POLICY_HANDLE hPolicy,
    /* [in] */ uint32 num_names,
    /* [in] */ UnicodeString *names,
    /* [out] */ RefDomainList **domains,
    /* [in, out] */ TranslatedSidArray *sids,
    /* [in] */ uint16 level,
    /* [in, out] */ uint32 *count
    )
{
    const DWORD dwSamrConnAccess = SAMR_ACCESS_CONNECT_TO_SERVER;
    const DWORD dwSamrDomainAccess = DOMAIN_ACCESS_LOOKUP_INFO_1 |
                                     DOMAIN_ACCESS_LOOKUP_ALIAS;

    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    RPCSTATUS rpcstatus = 0;
    PPOLICY_CONTEXT pPolCtx = NULL;
    HANDLE hDirectory = NULL;
    PSTR pszSamrLpcSocketPath = NULL;
    handle_t hSamrBinding = NULL;
    PolicyHandle hConn;
    PolicyHandle hDomain;
    PSID pDomainSid = NULL;
    PSAMR_DOMAIN pSamrDomains = NULL;
    PSAMR_DOMAIN pSamrDomain = NULL;
    SAMR_DOMAIN SamrDomain;
    CHAR szHostname[64];
    PWSTR pwszSystemName = NULL;
    PWSTR pwszName = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszAcctName = NULL;
    UnicodeString *name = NULL;
    PWSTR pwszNames[1];
    DWORD *dwRids = NULL;
    DWORD *dwTypes = NULL;
    DWORD dwCount = 0;
    DWORD i = 0;
    UnicodeString *pNonLocalNames = NULL;
    TranslatedSidArray *pSidArray = NULL;
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

    status = LsaSrvAllocateMemory((void**)&(pSamrDomain),
                                  sizeof(*pSamrDomain) * num_names,
                                  pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    pPolCtx->dwSamrDomainsNum = num_names;
    pPolCtx->pSamrDomain      = pSamrDomain;

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
        name = &(names[0]);

        pwszName = GetFromUnicodeString(name);
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
    }

    status = SamrClose(hSamrBinding, &hConn);
    BAIL_ON_NTSTATUS_ERROR(status);

    sids->count = pSidArray->count;
    sids->sids  = pSidArray->sids;

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


NTSTATUS
LsaSrvParseAccountName(
    PWSTR pwszName,
    PWSTR *ppwszDomainName,
    PWSTR *ppwszAcctName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PWSTR pwszCursor = NULL;
    PWSTR pwszDomainName = NULL;
    DWORD dwDomainNameLen = 0;
    PWSTR pwszAcctName = NULL;
    DWORD dwAcctNameLen = 0;

    pwszCursor = pwszName;

    while ((*pwszCursor) &&
           (*pwszCursor) != (WCHAR)'\\') pwszCursor++;

    if ((*pwszCursor) == (WCHAR)'\\') {
        dwDomainNameLen = (pwszCursor - pwszName);
        status = LsaSrvAllocateMemory((void**)&pwszDomainName,
                                      (dwDomainNameLen + 1) + sizeof(WCHAR),
                                      NULL);
        BAIL_ON_NTSTATUS_ERROR(status);

        wc16sncpy(pwszDomainName, pwszName, dwDomainNameLen);
        pwszCursor += 2;

    } else {
        pwszCursor = pwszName;
    }

    dwAcctNameLen = wc16slen(pwszCursor);
    status = LsaSrvAllocateMemory((void**)&pwszAcctName,
                                  (dwAcctNameLen + 1) * sizeof(WCHAR),
                                  NULL);
    BAIL_ON_NTSTATUS_ERROR(status);

    wc16sncpy(pwszAcctName, pwszCursor, dwAcctNameLen);

    *ppwszDomainName = pwszDomainName;
    *ppwszAcctName   = pwszAcctName;

cleanup:
    return status;

error:
    if (pwszDomainName) {
        LsaSrvFreeMemory(pwszDomainName);
    }

    if (pwszAcctName) {
        LsaSrvFreeMemory(pwszAcctName);
    }

    *ppwszDomainName = NULL;
    *ppwszAcctName = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvGetSamrDomain(
    PPOLICY_CONTEXT pPolCtx,
    PWSTR pwszDomainName,
    PSAMR_DOMAIN pSamrDomain
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD i = 0;

    for (i = 0; i < pPolCtx->dwSamrDomainsNum; i++) {
        PSAMR_DOMAIN pDomain = &(pPolCtx->pSamrDomain[i]);

        if (wc16scmp(pDomain->pwszName, pwszDomainName) == 0) {
            pSamrDomain->pwszName = pDomain->pwszName;
            pSamrDomain->pSid     = pDomain->pSid;
            pSamrDomain->bLocal   = pDomain->bLocal;
            pSamrDomain->hDomain  = pDomain->hDomain;

            goto cleanup;
        }
    }

    memset(pSamrDomain, 0, sizeof(*pSamrDomain));
    status = STATUS_NO_SUCH_DOMAIN;

cleanup:
    return status;
}


NTSTATUS
LsaSrvSetSamrDomain(
    PPOLICY_CONTEXT pPolCtx,
    PSAMR_DOMAIN pSamrDomain
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    DWORD i = 0;
    PSAMR_DOMAIN pDomain = NULL;

    for (i = 0;
         i < pPolCtx->dwSamrDomainsNum && pPolCtx->pSamrDomain[i].pwszName;
         i++)
    {
        pDomain = &(pPolCtx->pSamrDomain[i]);

        if (wc16scmp(pDomain->pwszName, pSamrDomain->pwszName) == 0) {
            goto cleanup;
        }
    }

    if ((++i) < pPolCtx->dwSamrDomainsNum) {
        pDomain = &(pPolCtx->pSamrDomain[i]);

        status = LsaSrvAllocateMemory((void**)&(pDomain->pwszName),
                                      (wc16slen(pSamrDomain->pwszName) + 1) *
                                      sizeof(WCHAR),
                                      pPolCtx->pSamrDomain);
        BAIL_ON_NTSTATUS_ERROR(status);

        wc16scpy(pSamrDomain->pwszName, pDomain->pwszName);

        status = RtlDuplicateSid(&pDomain->pSid,
                                 pSamrDomain->pSid);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaSrvAddDepMemory(pDomain->pSid,
                                    pPolCtx->pSamrDomain);
        BAIL_ON_NTSTATUS_ERROR(status);

        pDomain->bLocal  = pSamrDomain->bLocal;
        pDomain->hDomain = pSamrDomain->hDomain;
    }

cleanup:
    return status;

error:
    if (i < pPolCtx->dwSamrDomainsNum) {
        pDomain = &(pPolCtx->pSamrDomain[i]);

        if (pDomain->pwszName) {
            LsaSrvFreeMemory(pDomain->pwszName);
            pDomain->pwszName = NULL;
        }

        if (pDomain->pSid) {
            LsaSrvFreeMemory(pDomain->pSid);
            pDomain->pSid = NULL;
        }
    }

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
