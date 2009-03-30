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
LsaSrvGetLocalSamrDomain(
    PPOLICY_CONTEXT pPolCtx,
    PSAMR_DOMAIN *pSamrDomain
    );


NTSTATUS
LsaSrvGetBuiltinSamrDomain(
    PPOLICY_CONTEXT pPolCtx,
    PSAMR_DOMAIN *pSamDomain
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
    NTSTATUS status = STATUS_SUCCESS;
    DWORD dwError = 0;
    PPOLICY_CONTEXT pPolCtx = NULL;
    UnicodeStringEx *pNames = NULL;
    PWSTR pwszName = NULL;
    DWORD i = 0;
    RefDomainList *pDomains = NULL;
    TranslatedSidArray2 *pSidArray = NULL;
    DWORD dwCount = 0;

    pPolCtx = (PPOLICY_CONTEXT)hPolicy;

    if (pPolCtx == NULL || pPolCtx->Type != LsaContextPolicy) {
        status = STATUS_INVALID_HANDLE;
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    status = LsaSrvAllocateMemory((void**)pNames,
                                  sizeof(*pNames) * num_names,
                                  pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < num_names; i++) {
        pwszName = GetFromUnicodeString(&(names[i]));
        BAIL_ON_NO_MEMORY(pwszName);

        status = InitUnicodeStringEx(&(pNames[i]), pwszName);
        BAIL_ON_NTSTATUS_ERROR(status);

        status = LsaSrvAddDepMemory(pNames[i].string,
                                    pNames);
        BAIL_ON_NTSTATUS_ERROR(status);

        if (pwszName) {
            LSA_SAFE_FREE_MEMORY(pwszName);
        }
    }

    status = LsaSrvLookupNames2(IDL_handle,
                                hPolicy,
                                num_names,
                                pNames,
                                pDomains,
                                pSidArray,
                                level,
                                &dwCount,
                                0, 0);
    if (status != STATUS_SUCCESS ||
        status != STATUS_SOME_UNMAPPED ||
        status != STATUS_NONE_MAPPED) {
        BAIL_ON_NTSTATUS_ERROR(status);
    }

    sids->count = pSidArray->count;

    status = LsaSrvAllocateMemory((void**)&(sids->sids),
                                  sizeof(sids->sids[0]) * sids->count,
                                  pPolCtx);
    BAIL_ON_NTSTATUS_ERROR(status);

    for (i = 0; i < sids->count; i++) {
        TranslatedSid2 *pTransSid2 = &(pSidArray->sids[i]);
        TranslatedSid *pTransSid = &(sids->sids[i]);

        pTransSid->type  = pTransSid2->type;
        pTransSid->rid   = pTransSid2->rid;
        pTransSid->index = pTransSid2->index;
    }

    *domains = pDomains;
    *count   = dwCount;

cleanup:
    if (pwszName) {
        LSA_SAFE_FREE_MEMORY(pwszName);
    }

    if (pNames) {
        LsaSrvFreeMemory(pNames);
    }

    return status;

error:
    if (pDomains) {
        LsaSrvFreeMemory(pDomains);
    }

    if (sids->sids) {
        LsaSrvFreeMemory(sids->sids);
    }

    *domains    = NULL;
    sids->sids  = NULL;
    sids->count = 0;
    *count      = 0;

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


NTSTATUS
LsaSrvGetLocalSamrDomain(
    PPOLICY_CONTEXT pPolCtx,
    PSAMR_DOMAIN *pSamrDomain
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    return status;
}


NTSTATUS
LsaSrvGetBuiltinSamrDomain(
    PPOLICY_CONTEXT pPolCtx,
    PSAMR_DOMAIN *pSamDomain
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    return status;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
