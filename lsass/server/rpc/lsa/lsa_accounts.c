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
 *        lsa_accounts.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Accounts selection functions (by name or SID) for use before
 *        doing lookups in remote lsa or local samr rpc servers
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


NTSTATUS
LsaSrvParseAccountName(
    PWSTR pwszName,
    PWSTR *ppwszDomainName,
    PWSTR *ppwszAcctName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PWSTR pwszCursor = NULL;
    PWSTR pwszDomainName = NULL;
    DWORD dwDomainNameLen = 0;
    PWSTR pwszAcctName = NULL;
    DWORD dwAcctNameLen = 0;

    pwszCursor = pwszName;

    while ((*pwszCursor) &&
           (*pwszCursor) != (WCHAR)'\\') pwszCursor++;

    if ((*pwszCursor) == (WCHAR)'\\') {
        dwDomainNameLen = (DWORD)(pwszCursor - pwszName);
        ntStatus = LsaSrvAllocateMemory((PVOID*)&pwszDomainName,
					(dwDomainNameLen + 1) * sizeof(WCHAR));
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        wc16sncpy(pwszDomainName, pwszName, dwDomainNameLen);
        pwszCursor++;

    } else {
        pwszCursor = pwszName;
    }

    dwAcctNameLen = wc16slen(pwszCursor);
    ntStatus = LsaSrvAllocateMemory((void**)&pwszAcctName,
                                   (dwAcctNameLen + 1) * sizeof(WCHAR));
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    wc16sncpy(pwszAcctName, pwszCursor, dwAcctNameLen);

    *ppwszDomainName = pwszDomainName;
    *ppwszAcctName   = pwszAcctName;

cleanup:
    return ntStatus;

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
LsaSrvSelectAccountsByDomainName(
    PPOLICY_CONTEXT pPolCtx,
    UnicodeStringEx *pNames,
    DWORD dwNumNames,
    PACCOUNT_NAMES pDomainAccounts,
    PACCOUNT_NAMES pLocalAccounts,
    PACCOUNT_NAMES pBuiltinAccounts,
    PACCOUNT_NAMES pOtherAccounts
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    ACCOUNT_NAMES DomainAccounts = {0};
    ACCOUNT_NAMES LocalAccounts = {0};
    ACCOUNT_NAMES BuiltinAccounts = {0};
    ACCOUNT_NAMES OtherAccounts = {0};
    DWORD dwDomainNamesNum = 0;
    DWORD dwLocalNamesNum = 0;
    DWORD dwBuiltinNamesNum = 0;
    DWORD dwOtherNamesNum = 0;
    DWORD i = 0;
    PWSTR pwszName = NULL;
    PWSTR pwszDomainName = NULL;
    PWSTR pwszAcctName = NULL;
    WCHAR wszBuiltin[] = LSA_BUILTIN_DOMAIN_NAME;
    PSAM_DOMAIN_ENTRY pDomain = NULL;

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&DomainAccounts.ppwszNames,
                            sizeof(*DomainAccounts.ppwszNames) * dwNumNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&DomainAccounts.pdwIndices,
                            sizeof(*DomainAccounts.pdwIndices) * dwNumNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memset(DomainAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumNames);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&LocalAccounts.ppwszNames,
                            sizeof(*LocalAccounts.ppwszNames) * dwNumNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&LocalAccounts.pdwIndices,
                            sizeof(*LocalAccounts.pdwIndices) * dwNumNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memset(LocalAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumNames);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&BuiltinAccounts.ppwszNames,
                            sizeof(*BuiltinAccounts.ppwszNames) * dwNumNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&BuiltinAccounts.pdwIndices,
                            sizeof(*BuiltinAccounts.pdwIndices) * dwNumNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memset(BuiltinAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumNames);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&OtherAccounts.ppwszNames,
                            sizeof(*OtherAccounts.ppwszNames) * dwNumNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&OtherAccounts.pdwIndices,
                            sizeof(*OtherAccounts.pdwIndices) * dwNumNames);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    memset(OtherAccounts.pdwIndices, -1, sizeof(DWORD) * dwNumNames);


    for (i = 0; i < dwNumNames; i++) {
        UnicodeStringEx *name = &(pNames[i]);

        ntStatus = LsaSrvGetFromUnicodeStringEx(&pwszName, name);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        ntStatus = LsaSrvParseAccountName(pwszName,
                                        &pwszDomainName,
                                        &pwszAcctName);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        if (pwszDomainName)
        {
            pDomain = NULL;

            ntStatus = LsaSrvGetSamDomainByName(pPolCtx,
                                               pwszDomainName,
                                               &pDomain);
            if (ntStatus == STATUS_NO_SUCH_DOMAIN)
            {
                dwError = LwAllocateMemory(sizeof(*pDomain),
                                           OUT_PPVOID(&pDomain));
                BAIL_ON_LSA_ERROR(dwError);

                pDomain->pwszName = pwszDomainName;
                pDomain->pSid     = NULL;
                pDomain->bLocal   = FALSE;
            }
            else if (ntStatus != STATUS_SUCCESS)
            {
                BAIL_ON_NTSTATUS_ERROR(ntStatus);
            }

            if (pDomain->bLocal)
            {
                if (!wc16scasecmp(pDomain->pwszName, wszBuiltin))
                {
                    ntStatus = LsaSrvDuplicateWC16String(
                             &(BuiltinAccounts.ppwszNames[dwBuiltinNamesNum]),
                            pwszAcctName);
                    BAIL_ON_NTSTATUS_ERROR(ntStatus);

                    BuiltinAccounts.pdwIndices[dwBuiltinNamesNum++] = i;
                }
                else
                {
                    ntStatus = LsaSrvDuplicateWC16String(
                                 &(LocalAccounts.ppwszNames[dwLocalNamesNum]),
                                 pwszAcctName);
                    BAIL_ON_NTSTATUS_ERROR(ntStatus);

                    LocalAccounts.pdwIndices[dwLocalNamesNum++] = i;
                }
            }
            else
            {
                ntStatus = LsaSrvDuplicateWC16String(
                               &(DomainAccounts.ppwszNames[dwDomainNamesNum]),
                               pwszName);
                BAIL_ON_NTSTATUS_ERROR(ntStatus);

                DomainAccounts.pdwIndices[dwDomainNamesNum++] = i;
            }
        }
        else
        {
            /* If the account name isn't prepended with domain name
               we're going to give local and buitin domain a try and
               then decide */
            ntStatus = LsaSrvDuplicateWC16String(
                         &(OtherAccounts.ppwszNames[dwOtherNamesNum]),
                         pwszName);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            OtherAccounts.pdwIndices[dwOtherNamesNum++] = i;
        }

        if (pwszName)
        {
            LsaSrvFreeMemory(pwszName);
            pwszName = NULL;
        }

        if (pwszDomainName)
        {
            LsaSrvFreeMemory(pwszDomainName);
            pwszDomainName = NULL;
        }

        if (pwszAcctName)
        {
            LsaSrvFreeMemory(pwszAcctName);
            pwszAcctName = NULL;
        }
    }

    pDomainAccounts->dwCount     = dwDomainNamesNum;
    pDomainAccounts->ppwszNames  = DomainAccounts.ppwszNames;
    pDomainAccounts->pdwIndices  = DomainAccounts.pdwIndices;
    pLocalAccounts->dwCount      = dwLocalNamesNum;
    pLocalAccounts->ppwszNames   = LocalAccounts.ppwszNames;
    pLocalAccounts->pdwIndices   = LocalAccounts.pdwIndices;
    pBuiltinAccounts->dwCount    = dwBuiltinNamesNum;
    pBuiltinAccounts->ppwszNames = BuiltinAccounts.ppwszNames;
    pBuiltinAccounts->pdwIndices = BuiltinAccounts.pdwIndices;
    pOtherAccounts->dwCount      = dwOtherNamesNum;
    pOtherAccounts->ppwszNames   = OtherAccounts.ppwszNames;
    pOtherAccounts->pdwIndices   = OtherAccounts.pdwIndices;

cleanup:
    return ntStatus;

error:
    pDomainAccounts->dwCount     = 0;

    if (pDomainAccounts->ppwszNames) {
        LsaSrvFreeMemory(pDomainAccounts->ppwszNames);
        pDomainAccounts->ppwszNames  = NULL;
    }

    if (pDomainAccounts->pdwIndices) {
        LsaSrvFreeMemory(pDomainAccounts->pdwIndices);
        pDomainAccounts->pdwIndices  = NULL;
    }

    pLocalAccounts->dwCount      = 0;

    if (pLocalAccounts->ppwszNames) {
        LsaSrvFreeMemory(pLocalAccounts->ppwszNames);
        pLocalAccounts->ppwszNames  = NULL;
    }

    if (pLocalAccounts->pdwIndices) {
        LsaSrvFreeMemory(pLocalAccounts->pdwIndices);
        pLocalAccounts->pdwIndices  = NULL;
    }

    pBuiltinAccounts->dwCount    = 0;

    if (pBuiltinAccounts->ppwszNames) {
        LsaSrvFreeMemory(pBuiltinAccounts->ppwszNames);
        pBuiltinAccounts->ppwszNames  = NULL;
    }

    if (pBuiltinAccounts->pdwIndices) {
        LsaSrvFreeMemory(pBuiltinAccounts->pdwIndices);
        pBuiltinAccounts->pdwIndices  = NULL;
    }

    pOtherAccounts->dwCount      = 0;

    if (pOtherAccounts->ppwszNames) {
        LsaSrvFreeMemory(pOtherAccounts->ppwszNames);
        pOtherAccounts->ppwszNames  = NULL;
    }

    if (pOtherAccounts->pdwIndices) {
        LsaSrvFreeMemory(pOtherAccounts->pdwIndices);
        pOtherAccounts->pdwIndices  = NULL;
    }

    goto cleanup;
}


NTSTATUS
LsaSrvSelectAccountsByDomainSid(
    PPOLICY_CONTEXT pPolCtx,
    SidArray *pSids,
    DWORD dwNumSids,
    PACCOUNT_SIDS pDomainAccounts,
    PACCOUNT_SIDS pLocalAccounts,
    PACCOUNT_SIDS pBuiltinAccounts
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD i = 0;
    PSID pSid = NULL;
    PSAM_DOMAIN_ENTRY pLocalDomain = NULL;
    PSAM_DOMAIN_ENTRY pBuiltinDomain = NULL;
    ACCOUNT_SIDS DomainAccounts = {0};
    ACCOUNT_SIDS LocalAccounts = {0};
    ACCOUNT_SIDS BuiltinAccounts = {0};
    DWORD dwDomainSidsNum = 0;
    DWORD dwLocalSidsNum = 0;
    DWORD dwBuiltinSidsNum = 0;

    ntStatus = LsaSrvAllocateMemory(
                            OUT_PPVOID(&DomainAccounts.ppSids),
                            sizeof(DomainAccounts.ppSids[0]) * dwNumSids);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&DomainAccounts.pdwIndices,
                            sizeof(*DomainAccounts.pdwIndices) * dwNumSids);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(
                            OUT_PPVOID(&LocalAccounts.ppSids),
                            sizeof(LocalAccounts.ppSids[0]) * dwNumSids);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&LocalAccounts.pdwIndices,
                            sizeof(*LocalAccounts.pdwIndices) * dwNumSids);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvAllocateMemory(
                            OUT_PPVOID(&BuiltinAccounts.ppSids),
                            sizeof(BuiltinAccounts.ppSids[0]) * dwNumSids);

    ntStatus = LsaSrvAllocateMemory(
                            (void**)&BuiltinAccounts.pdwIndices,
                            sizeof(*BuiltinAccounts.pdwIndices) * dwNumSids);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);


    ntStatus = LsaSrvGetLocalSamDomain(pPolCtx,
                                       FALSE,   /* !BUILTIN */
                                       &pLocalDomain);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvGetLocalSamDomain(pPolCtx,
                                       TRUE,   /* BUILTIN */
                                       &pBuiltinDomain);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    for (i = 0; i < dwNumSids; i++)
    {
        pSid = pSids->sids[i].sid;

        if (RtlIsPrefixSid(pLocalDomain->pSid,
                           pSid))
        {
            ntStatus = RtlDuplicateSid(
                                &LocalAccounts.ppSids[dwLocalSidsNum],
                                pSid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            LocalAccounts.pdwIndices[dwLocalSidsNum++] = i;
        }
        else if (RtlIsPrefixSid(pBuiltinDomain->pSid,
                                pSid))
        {
            ntStatus = RtlDuplicateSid(
                                &BuiltinAccounts.ppSids[dwBuiltinSidsNum],
                                pSid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            BuiltinAccounts.pdwIndices[dwBuiltinSidsNum++] = i;
        }
        else
        {
            ntStatus = RtlDuplicateSid(
                                &DomainAccounts.ppSids[dwDomainSidsNum],
                                pSid);
            BAIL_ON_NTSTATUS_ERROR(ntStatus);

            DomainAccounts.pdwIndices[dwDomainSidsNum++] = i;
        }
    }

    pDomainAccounts->dwCount     = dwDomainSidsNum;
    pDomainAccounts->ppSids      = DomainAccounts.ppSids;
    pDomainAccounts->pdwIndices  = DomainAccounts.pdwIndices;
    pLocalAccounts->dwCount      = dwLocalSidsNum;
    pLocalAccounts->ppSids       = LocalAccounts.ppSids;
    pLocalAccounts->pdwIndices   = LocalAccounts.pdwIndices;
    pBuiltinAccounts->dwCount    = dwBuiltinSidsNum;
    pBuiltinAccounts->ppSids     = BuiltinAccounts.ppSids;
    pBuiltinAccounts->pdwIndices = BuiltinAccounts.pdwIndices;


cleanup:
    LsaSrvSamDomainEntryFree(&pLocalDomain);
    LsaSrvSamDomainEntryFree(&pBuiltinDomain);

    return ntStatus;

error:
    LsaSrvFreeAccountSids(&DomainAccounts);
    LsaSrvFreeAccountSids(&LocalAccounts);
    LsaSrvFreeAccountSids(&BuiltinAccounts);

    goto cleanup;
}


VOID
LsaSrvFreeAccountNames(
    PACCOUNT_NAMES pAccountNames
    )
{
    DWORD iName = 0;

    for (iName = 0; iName < pAccountNames->dwCount; iName++)
    {
        LsaSrvFreeMemory(pAccountNames->ppwszNames[iName]);
    }

    LsaSrvFreeMemory(pAccountNames->ppwszNames);
}


VOID
LsaSrvFreeAccountSids(
    PACCOUNT_SIDS pAccountSids
    )
{
    DWORD iSid = 0;

    for (iSid = 0; iSid < pAccountSids->dwCount; iSid++)
    {
        RTL_FREE(&pAccountSids->ppSids[iSid]);
    }

    LsaSrvFreeMemory(&pAccountSids->ppSids);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
