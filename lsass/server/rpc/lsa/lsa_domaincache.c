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
 *        lsa_domaincache.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        SAM domains cache for use with samr rpc client calls
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
int
LsaSrvSamDomainKeyCompare(
    PCVOID pEntry1,
    PCVOID pEntry2
    );


static
size_t
LsaSrvSamDomainKeyHash(
    PCVOID pKey
    );


static
void
LsaSrvSamDomainHashEntryFree(
    const LSA_HASH_ENTRY *pEntry
    );


static
NTSTATUS
LsaSrvSamDomainEntryCopy(
    PSAM_DOMAIN_ENTRY *ppOut,
    const PSAM_DOMAIN_ENTRY pIn
    );


static
NTSTATUS
LsaSrvCreateSamDomainKey(
    PSAM_DOMAIN_KEY *ppKey,
    PCWSTR pwszName,
    const PSID pSid
    );


static
void
LsaSrvSamDomainKeyFree(
    PSAM_DOMAIN_KEY *ppKey
    );


NTSTATUS
LsaSrvCreateSamDomainsTable(
    PLSA_HASH_TABLE *ppDomains
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PLSA_HASH_TABLE pDomains = NULL;

    dwError = LsaHashCreate(20,
                            LsaSrvSamDomainKeyCompare,
                            LsaSrvSamDomainKeyHash,
                            LsaSrvSamDomainHashEntryFree,
                            NULL,
                            &pDomains);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDomains = pDomains;

cleanup:
    return ntStatus;

error:
    *ppDomains = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvGetSamDomainByName(
    PPOLICY_CONTEXT pPolCtx,
    PCWSTR pwszDomainName,
    PSAM_DOMAIN_ENTRY *ppDomain
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSAM_DOMAIN_KEY pKey = NULL;
    PVOID pEntry = NULL;
    PSAM_DOMAIN_ENTRY pDomain = NULL;

    BAIL_ON_INVALID_PTR(pPolCtx);
    BAIL_ON_INVALID_PTR(pwszDomainName);
    BAIL_ON_INVALID_PTR(ppDomain);

    ntStatus = LsaSrvCreateSamDomainKey(&pKey,
                                        pwszDomainName,
                                        NULL);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LsaHashGetValue(pPolCtx->pDomains,
                              (PVOID)&pKey,
                              OUT_PPVOID(&pEntry));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaSrvSamDomainEntryCopy(&pDomain,
                                        (PSAM_DOMAIN_ENTRY)pEntry);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppDomain = pDomain;

cleanup:
    LsaSrvSamDomainKeyFree(&pKey);

    return ntStatus;

error:
    LsaSrvSamDomainEntryFree(&pDomain);

    *ppDomain = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvGetSamDomainBySid(
    PPOLICY_CONTEXT pPolCtx,
    const PSID pSid,
    PSAM_DOMAIN_ENTRY *ppDomain
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSAM_DOMAIN_KEY pKey = NULL;
    PVOID pEntry = NULL;
    PSAM_DOMAIN_ENTRY pDomain = NULL;

    BAIL_ON_INVALID_PTR(pPolCtx);
    BAIL_ON_INVALID_PTR(pSid);
    BAIL_ON_INVALID_PTR(ppDomain);

    ntStatus = LsaSrvCreateSamDomainKey(&pKey,
                                        NULL,
                                        pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LsaHashGetValue(pPolCtx->pDomains,
                              (PVOID)pKey,
                              OUT_PPVOID(&pEntry));
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaSrvSamDomainEntryCopy(&pDomain,
                                        (PSAM_DOMAIN_ENTRY)pEntry);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    *ppDomain = pDomain;

cleanup:
    LsaSrvSamDomainKeyFree(&pKey);

    return ntStatus;

error:
    LsaSrvSamDomainEntryFree(&pDomain);

    *ppDomain = NULL;
    goto cleanup;
}


NTSTATUS
LsaSrvSetSamDomain(
    PPOLICY_CONTEXT pPolCtx,
    const PSAM_DOMAIN_ENTRY pDomain
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    PSAM_DOMAIN_KEY pKeyName = NULL;
    PSAM_DOMAIN_ENTRY pEntryByName = NULL;
    PSAM_DOMAIN_KEY pKeySid = NULL;
    PSAM_DOMAIN_ENTRY pEntryBySid = NULL;

    BAIL_ON_INVALID_PTR(pPolCtx);
    BAIL_ON_INVALID_PTR(pDomain);

    ntStatus = LsaSrvSamDomainEntryCopy(&pEntryByName,
                                        pDomain);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvCreateSamDomainKey(&pKeyName,
                                        pDomain->pwszName,
                                        NULL);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LsaHashSetValue(pPolCtx->pDomains,
                              pKeyName,
                              pEntryByName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = LsaSrvSamDomainEntryCopy(&pEntryBySid,
                                        pDomain);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    ntStatus = LsaSrvCreateSamDomainKey(&pKeySid,
                                        NULL,
                                        pDomain->pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    dwError = LsaHashSetValue(pPolCtx->pDomains,
                              pKeySid,
                              pEntryBySid);

cleanup:
    return ntStatus;

error:
    if (pKeyName)
    {
        LsaSrvSamDomainKeyFree(&pKeyName);
    }

    if (pKeySid)
    {
        LsaSrvSamDomainKeyFree(&pKeySid);
    }

    if (pEntryByName)
    {
        LsaSrvSamDomainEntryFree(&pEntryByName);
    }

    if (pEntryBySid)
    {
        LsaSrvSamDomainEntryFree(&pEntryBySid);
    }

    goto cleanup;
}


NTSTATUS
LsaSrvGetLocalSamDomain(
    PPOLICY_CONTEXT pPolCtx,
    BOOLEAN bBuiltin,
    PSAM_DOMAIN_ENTRY *ppDomain
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = 0;
    LSA_HASH_ITERATOR IterDomains = {0};
    LSA_HASH_ENTRY *pHashEntry = NULL;
    PSAM_DOMAIN_ENTRY pEntry = NULL;
    PSAM_DOMAIN_ENTRY pDomain = NULL;
    WCHAR wszBuiltinName[] = LSA_BUILTIN_DOMAIN_NAME;
    BOOLEAN bIsBuiltin = FALSE;

    dwError = LsaHashGetIterator(pPolCtx->pDomains,
                                 &IterDomains);
    BAIL_ON_LSA_ERROR(dwError);

    for (pHashEntry = LsaHashNext(&IterDomains);
         pHashEntry != NULL;
         pHashEntry = LsaHashNext(&IterDomains))
    {
        pEntry = (PSAM_DOMAIN_ENTRY)pHashEntry->pValue;
        if (!pEntry->bLocal) continue;

        bIsBuiltin = (!wc16scasecmp(pEntry->pwszName, wszBuiltinName));
        if (bIsBuiltin != bBuiltin) continue;

        ntStatus = LsaSrvSamDomainEntryCopy(&pDomain,
                                            pEntry);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

        *ppDomain = pDomain;
        goto cleanup;
    }

    ntStatus = STATUS_NO_SUCH_DOMAIN;
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

cleanup:
    return ntStatus;

error:
    LsaSrvSamDomainEntryFree(&pDomain);

    *ppDomain = NULL;
    goto cleanup;
}


VOID
LsaSrvSamDomainEntryFree(
    PSAM_DOMAIN_ENTRY *ppEntry
    )
{
    PSAM_DOMAIN_ENTRY pEntry = *ppEntry;

    if (!pEntry) return;

    RTL_FREE(&pEntry->pSid);
    RTL_FREE(&pEntry->pwszName);
    RTL_FREE(&pEntry);

    *ppEntry = pEntry;
}


static
int
LsaSrvSamDomainKeyCompare(
    PCVOID pK1,
    PCVOID pK2
    )
{
    int ret = 0;
    PSAM_DOMAIN_KEY pKey1 = (PSAM_DOMAIN_KEY)pK1;
    PSAM_DOMAIN_KEY pKey2 = (PSAM_DOMAIN_KEY)pK2;

    if (pKey1->eType != pKey2->eType)
    {
        ret = 1;
        goto cleanup;
    }

    switch (pKey1->eType)
    {
    case eSamDomainSid:
        if (!RtlEqualSid(pKey1->pSid, pKey2->pSid))
        {
            ret = 1;
        }
        break;

    case eSamDomainName:
        ret = wc16scmp(pKey1->pwszName, pKey2->pwszName);
        break;
    }

cleanup:
    return ret;
}


static
size_t
LsaSrvSamDomainKeyHash(
    PCVOID pK
    )
{
    PSAM_DOMAIN_KEY pKey = (PSAM_DOMAIN_KEY)pK;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszKeyStr = NULL;
    size_t hash = 0;

    switch (pKey->eType)
    {
    case eSamDomainSid:
        ntStatus = RtlAllocateCStringFromSid(
                                  &pszKeyStr,
                                  pKey->pSid);
        break;

    case eSamDomainName:
        ntStatus = LwRtlCStringAllocateFromWC16String(
                                  &pszKeyStr,
                                  pKey->pwszName);
        break;
    }
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    hash = LsaHashCaselessStringHash(pszKeyStr);

cleanup:
    RTL_FREE(&pszKeyStr);
    return hash;

error:
    hash = 0;
    goto cleanup;
}


static
void
LsaSrvSamDomainHashEntryFree(
    const LSA_HASH_ENTRY *pEntry
    )
{
    LsaSrvSamDomainKeyFree((PSAM_DOMAIN_KEY*)&pEntry->pKey);
    LsaSrvSamDomainEntryFree((PSAM_DOMAIN_ENTRY*)&pEntry->pValue);
}


static
NTSTATUS
LsaSrvSamDomainEntryCopy(
    PSAM_DOMAIN_ENTRY *ppOut,
    const PSAM_DOMAIN_ENTRY pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSAM_DOMAIN_ENTRY pOut = NULL;

    BAIL_ON_INVALID_PTR(ppOut);
    BAIL_ON_INVALID_PTR(pIn);

    dwError = LwAllocateMemory(sizeof(*pOut),
                               OUT_PPVOID(&pOut));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateWc16String(&pOut->pwszName,
                                   pIn->pwszName);
    BAIL_ON_LSA_ERROR(dwError);

    ntStatus = RtlDuplicateSid(&pOut->pSid,
                               pIn->pSid);
    BAIL_ON_NTSTATUS_ERROR(ntStatus);

    pOut->bLocal  = pIn->bLocal;
    pOut->hDomain = pIn->hDomain;

    *ppOut = pOut;

cleanup:
    return ntStatus;

error:
    *ppOut = NULL;
    goto cleanup;
}


static
NTSTATUS
LsaSrvCreateSamDomainKey(
    PSAM_DOMAIN_KEY *ppKey,
    PCWSTR pwszName,
    const PSID pSid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    DWORD dwError = ERROR_SUCCESS;
    PSAM_DOMAIN_KEY pKey = NULL;

    BAIL_ON_INVALID_PTR(ppKey);

    dwError = LwAllocateMemory(sizeof(*pKey),
                               OUT_PPVOID(&pKey));
    BAIL_ON_LSA_ERROR(dwError);

    if (pwszName)
    {
        pKey->eType = eSamDomainName;

        dwError = LwAllocateWc16String(&pKey->pwszName,
                                       pwszName);
        BAIL_ON_LSA_ERROR(dwError);

    }
    else if (pSid)
    {
        pKey->eType = eSamDomainSid;

        ntStatus = RtlDuplicateSid(&pKey->pSid,
                                   pSid);
        BAIL_ON_NTSTATUS_ERROR(ntStatus);

    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NTSTATUS_ERROR(ntStatus);
    }

    *ppKey = pKey;

cleanup:
    return ntStatus;

error:
    LsaSrvSamDomainKeyFree(&pKey);
    goto cleanup;
}


static
void
LsaSrvSamDomainKeyFree(
    PSAM_DOMAIN_KEY *ppKey
    )
{
    PSAM_DOMAIN_KEY pKey = *ppKey;

    if (!pKey) return;

    switch (pKey->eType)
    {
    case eSamDomainName:
        LW_SAFE_FREE_MEMORY(pKey->pwszName);
        break;

    case eSamDomainSid:
        RTL_FREE(&pKey->pSid);
        break;
    }

    LW_SAFE_FREE_MEMORY(pKey);
    *ppKey = NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
