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
 * Abstract: Lsa memory (de)allocation routines (rpc client library)
 *
 * Authors: Rafal Szczesniak (rafal@likewisesoftware.com)
 */

#include "includes.h"

/* File globals */

static PVOID pLsaMemoryList = NULL;
static pthread_mutex_t gLsaDataMutex = PTHREAD_MUTEX_INITIALIZER;
static BOOLEAN bLsaInitialised = 0;


/* Code */

NTSTATUS
LsaRpcInitMemory(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    LIBRPC_LOCK_MUTEX(bLocked, &gLsaDataMutex);

    if (!bLsaInitialised)
    {
        ntStatus = MemPtrListInit((PtrList**)&pLsaMemoryList);
        BAIL_ON_NT_STATUS(ntStatus);

        bLsaInitialised = 1;
    }

cleanup:
    LIBRPC_UNLOCK_MUTEX(bLocked, &gLsaDataMutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
LsaRpcDestroyMemory(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    LIBRPC_LOCK_MUTEX(bLocked, &gLsaDataMutex);

    if (bLsaInitialised && pLsaMemoryList)
    {
        ntStatus = MemPtrListDestroy((PtrList**)&pLsaMemoryList);
        BAIL_ON_NT_STATUS(ntStatus);

        bLsaInitialised = 0;
    }

cleanup:
    LIBRPC_UNLOCK_MUTEX(bLocked, &gLsaDataMutex);

    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
LsaRpcAllocateMemory(
    OUT PVOID *ppOut,
    IN  size_t Size,
    IN  PVOID  pDependent
    )
{
    return MemPtrAllocate(
               (PtrList*)pLsaMemoryList,
               ppOut,
               Size,
               pDependent);
}


NTSTATUS
LsaRpcFreeMemory(
    IN PVOID pBuffer
    )
{
    if (pBuffer == NULL)
    {
        return STATUS_SUCCESS;
    }

    return MemPtrFree((PtrList*)pLsaMemoryList, pBuffer);
}


NTSTATUS
LsaRpcAddDepMemory(
    IN PVOID pBuffer,
    IN PVOID pDependent
    )
{
    return MemPtrAddDependant(
               (PtrList*)pLsaMemoryList,
               pBuffer,
               pDependent);
}


NTSTATUS
LsaAllocateTranslatedSids(
    OUT TranslatedSid **ppOut,
    IN  TranslatedSidArray *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    TranslatedSid *pNewArray = NULL;
    int i = 0;
    int count = (pIn == NULL) ? 1 : pIn->count;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = LsaRpcAllocateMemory(
                   (PVOID*)&pNewArray,
                   sizeof(TranslatedSid) * count,
                   NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn != NULL)
    {
        for (i = 0; i < count; i++)
        {
            pNewArray[i].type  = pIn->sids[i].type;
            pNewArray[i].rid   = pIn->sids[i].rid;
            pNewArray[i].index = pIn->sids[i].index;
        }
    }

    *ppOut = pNewArray;
    pNewArray = NULL;

cleanup:
    return ntStatus;

error:
    LsaRpcFreeMemory((PVOID)pNewArray);

    *ppOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaAllocateTranslatedSids2(
    OUT TranslatedSid2 **ppOut,
    IN  TranslatedSidArray2 *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    TranslatedSid2 *pNewArray = NULL;
    int i = 0;
    int count = (pIn == NULL) ? 1 : pIn->count;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = LsaRpcAllocateMemory(
                   (PVOID*)&pNewArray,
                   sizeof(TranslatedSid2) * count,
                   NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn != NULL)
    {
        for (i = 0; i < count; i++)
        {
            pNewArray[i].type     = pIn->sids[i].type;
            pNewArray[i].rid      = pIn->sids[i].rid;
            pNewArray[i].index    = pIn->sids[i].index;
            pNewArray[i].unknown1 = pIn->sids[i].unknown1;
        }
    }

    *ppOut = pNewArray;
    pNewArray = NULL;

cleanup:
    return ntStatus;

error:
    LsaRpcFreeMemory((PVOID)pNewArray);

    *ppOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaAllocateTranslatedSids3(
    OUT TranslatedSid3 **ppOut,
    IN  TranslatedSidArray3 *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    TranslatedSid3 *pNewArray = NULL;
    int i = 0;
    int count = (pIn == NULL) ? 1 : pIn->count;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = LsaRpcAllocateMemory(
                   (PVOID*)&pNewArray,
                   sizeof(TranslatedSid2) * count,
                   NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn != NULL)
    {
        for (i = 0; i < count; i++)
        {
            pNewArray[i].type     = pIn->sids[i].type;
            pNewArray[i].index    = pIn->sids[i].index;
            pNewArray[i].unknown1 = pIn->sids[i].unknown1;

            if (pIn->sids[i].sid)
            {
                ntStatus = MsRpcDuplicateSid(
                               &(pNewArray[i].sid),
                               pIn->sids[i].sid);
                BAIL_ON_NT_STATUS(ntStatus);

            }
            else
            {
                pNewArray[i].sid = NULL;
            }

            ntStatus = LsaRpcAddDepMemory(pNewArray[i].sid, pNewArray);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *ppOut = pNewArray;
    pNewArray = NULL;

cleanup:
    return ntStatus;

error:
    LsaRpcFreeMemory((PVOID)pNewArray);

    *ppOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaAllocateRefDomainList(
    OUT RefDomainList **ppOut,
    IN  RefDomainList *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    RefDomainList *pDomList = NULL;
    int i = 0;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = LsaRpcAllocateMemory(
                   (PVOID*)&pDomList,
                   sizeof(RefDomainList),
                   NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn == NULL) goto cleanup;

    pDomList->count    = pIn->count;
    pDomList->max_size = pIn->max_size;

    ntStatus = LsaRpcAllocateMemory(
                   (PVOID*)&pDomList->domains,
                   sizeof(LsaDomainInfo) * pDomList->count,
                   (PVOID)pDomList);
    BAIL_ON_NT_STATUS(ntStatus);

    for (i = 0; i < pDomList->count; i++)
    {
        LsaDomainInfo *pDomInfo = &(pDomList->domains[i]);

        ntStatus = CopyUnicodeStringEx(&pDomInfo->name, &pIn->domains[i].name);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pDomInfo->name.string)
        {
            ntStatus = LsaRpcAddDepMemory(
                           (PVOID)pDomInfo->name.string,
                           (PVOID)pDomList);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        MsRpcDuplicateSid(&pDomInfo->sid, pIn->domains[i].sid);
        BAIL_ON_NULL_PTR(pDomInfo->sid, ntStatus);

        if (pDomInfo->sid)
        {
            ntStatus = LsaRpcAddDepMemory(
                           (PVOID)pDomInfo->sid,
                           (PVOID)pDomList);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *ppOut = pDomList;
    pDomList = NULL;

cleanup:
    return ntStatus;

error:
    LsaRpcFreeMemory(pDomList);

    *ppOut = NULL;
    goto cleanup;
}


NTSTATUS
LsaAllocateTranslatedNames(
    OUT TranslatedName **ppOut,
    IN  TranslatedNameArray *pIn
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    TranslatedName *pNameArray = NULL;
    int i = 0;
    int count = (pIn == NULL) ? 1 : pIn->count;;

    BAIL_ON_INVALID_PTR(ppOut, ntStatus);

    ntStatus = LsaRpcAllocateMemory(
                   (PVOID*)&pNameArray,
                   sizeof(TranslatedName) * count,
                   NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn != NULL)
    {
        for (i = 0; i < pIn->count; i++)
        {
            TranslatedName *pNameOut = &pNameArray[i];
            TranslatedName *pNameIn  = &pIn->names[i];

            pNameOut->type      = pNameIn->type;
            pNameOut->sid_index = pNameIn->sid_index;

            ntStatus = CopyUnicodeString(&pNameOut->name, &pNameIn->name);
            BAIL_ON_NT_STATUS(ntStatus);

            if (pNameOut->name.string)
            {
                ntStatus = LsaRpcAddDepMemory(
                               (PVOID)pNameOut->name.string,
                               (PVOID)pNameArray);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }
    }

    *ppOut = pNameArray;
    pNameArray = NULL;

cleanup:
    return ntStatus;

error:
    LsaRpcFreeMemory((PVOID)pNameArray);

    *ppOut = NULL;
    goto cleanup;
}


static
NTSTATUS
LsaCopyPolInfoField(
    OUT PVOID pOut,
    IN  PVOID pIn,
    IN  size_t Size)
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    memcpy(pOut, pIn, Size);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaCopyPolInfoAuditEvents(
    OUT AuditEventsInfo *pOut,
    IN  AuditEventsInfo *pIn,
    IN  PVOID pDependent
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    pOut->auditing_mode = pIn->auditing_mode;
    pOut->count         = pIn->count;

    ntStatus = LsaRpcAllocateMemory(
                   (PVOID*)&pOut->settings,
                   (size_t)pOut->count,
                   pDependent);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy((PVOID)&pOut->settings,
           (PVOID)&pIn->settings,
           pOut->count);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaCopyPolInfoLsaDomain(
    OUT LsaDomainInfo *pOut,
    IN  LsaDomainInfo *pIn,
    IN  PVOID pDependent
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = CopyUnicodeStringEx(&pOut->name, &pIn->name);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->name.string)
    {
        ntStatus = LsaRpcAddDepMemory(
                       (PVOID)pOut->name.string,
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    MsRpcDuplicateSid(&pOut->sid, pIn->sid);
    BAIL_ON_NULL_PTR(pOut->sid, ntStatus);

    if (pOut->sid)
    {
        ntStatus = LsaRpcAddDepMemory(
                       (PVOID)pOut->sid,
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaCopyPolInfoPDAccount(
    OUT PDAccountInfo *pOut,
    IN  PDAccountInfo *pIn,
    IN  PVOID pDependent
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = CopyUnicodeString(&pOut->name, &pIn->name);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->name.string)
    {
        ntStatus = LsaRpcAddDepMemory(
                       (PVOID)pOut->name.string,
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaCopyPolInfoReplicaSource(
    OUT ReplicaSourceInfo *pOut,
    IN  ReplicaSourceInfo *pIn,
    IN  PVOID pDependent
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = CopyUnicodeString(&pOut->source, &pIn->source);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->source.string)
    {
        ntStatus = LsaRpcAddDepMemory(
                       (PVOID)pOut->source.string,
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeString(&pOut->account, &pIn->account);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->account.string)
    {
        ntStatus = LsaRpcAddDepMemory(
                       (PVOID)pOut->account.string,
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


static
NTSTATUS
LsaCopyPolInfoDnsDomain(
    OUT DnsDomainInfo *pOut,
    IN  DnsDomainInfo *pIn,
    IN  PVOID pDependent
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);
    BAIL_ON_INVALID_PTR(pIn, ntStatus);

    ntStatus = CopyUnicodeStringEx(&pOut->name, &pIn->name);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->name.string)
    {
        ntStatus = LsaRpcAddDepMemory(
                       (PVOID)pOut->name.string,
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeStringEx(&pOut->dns_domain, &pIn->dns_domain);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->dns_domain.string)
    {
        ntStatus = LsaRpcAddDepMemory(
                       (PVOID)pOut->dns_domain.string,
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = CopyUnicodeStringEx(&pOut->dns_forest, &pIn->dns_forest);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOut->dns_forest.string)
    {
        ntStatus = LsaRpcAddDepMemory(
                       (PVOID)pOut->dns_forest.string,
                       pDependent);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy((PVOID)&pOut->domain_guid,
           (PVOID)&pIn->domain_guid,
           sizeof(Guid));

    MsRpcDuplicateSid(&pOut->sid, pIn->sid);
    BAIL_ON_NULL_PTR(pOut->sid, ntStatus);

    if (pOut->sid) {
        ntStatus = LsaRpcAddDepMemory((PVOID)pOut->sid, pDependent);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


NTSTATUS
LsaAllocatePolicyInformation(
    OUT LsaPolicyInformation **pOut,
    IN  LsaPolicyInformation *pIn,
    IN  UINT32 Level
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LsaPolicyInformation *pPolInfo = NULL;

    BAIL_ON_INVALID_PTR(pOut, ntStatus);

    ntStatus = LsaRpcAllocateMemory(
                   (PVOID*)&pPolInfo,
                   sizeof(LsaPolicyInformation),
                   NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIn == NULL)
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    switch (Level) {
    case LSA_POLICY_INFO_AUDIT_LOG:
        ntStatus = LsaCopyPolInfoField(
                       (PVOID)&pPolInfo->audit_log,
                       (PVOID)&pIn->audit_log,
                       sizeof(AuditLogInfo));
        break;

    case LSA_POLICY_INFO_AUDIT_EVENTS:
        ntStatus = LsaCopyPolInfoAuditEvents(
                       &pPolInfo->audit_events,
                       &pIn->audit_events,
                       (PVOID)pPolInfo);
        break;

    case LSA_POLICY_INFO_DOMAIN:
        ntStatus = LsaCopyPolInfoLsaDomain(
                       &pPolInfo->domain,
                       &pIn->domain,
                       (PVOID)pPolInfo);
        break;

    case LSA_POLICY_INFO_PD:
        ntStatus = LsaCopyPolInfoPDAccount(
                       &pPolInfo->pd,
                       &pIn->pd,
                       (PVOID)pPolInfo);
        break;

    case LSA_POLICY_INFO_ACCOUNT_DOMAIN:
        ntStatus = LsaCopyPolInfoLsaDomain(
                       &pPolInfo->account_domain,
                       &pIn->account_domain,
                       (PVOID)pPolInfo);
        break;

    case LSA_POLICY_INFO_ROLE:
        ntStatus = LsaCopyPolInfoField(
                       (PVOID)&pPolInfo->role,
                       (PVOID)&pIn->role,
                       sizeof(ServerRole));
        break;

    case LSA_POLICY_INFO_REPLICA:
        ntStatus = LsaCopyPolInfoReplicaSource(
                       &pPolInfo->replica,
                       &pIn->replica,
                       (PVOID)pPolInfo);
        break;

    case LSA_POLICY_INFO_QUOTA:
        ntStatus = LsaCopyPolInfoField(
                       (PVOID)&pPolInfo->quota,
                       (PVOID)&pIn->quota,
                       sizeof(DefaultQuotaInfo));
        break;

    case LSA_POLICY_INFO_DB:
        ntStatus = LsaCopyPolInfoField(
                       (PVOID)&pPolInfo->db,
                       (PVOID)&pIn->db,
                       sizeof(ModificationInfo));
        break;

    case LSA_POLICY_INFO_AUDIT_FULL_SET:
        ntStatus = LsaCopyPolInfoField(
                       (PVOID)&pPolInfo->audit_set,
                       (PVOID)&pIn->audit_set,
                       sizeof(AuditFullSetInfo));
        break;

    case LSA_POLICY_INFO_AUDIT_FULL_QUERY:
        ntStatus = LsaCopyPolInfoField(
                       (PVOID)&pPolInfo->audit_query,
                       (PVOID)&pIn->audit_query,
                       sizeof(AuditFullQueryInfo));
        break;

    case LSA_POLICY_INFO_DNS:
        ntStatus = LsaCopyPolInfoDnsDomain(
                       &pPolInfo->dns,
                       &pIn->dns,
                       (PVOID)pPolInfo);
        break;

    default:
        ntStatus = STATUS_INVALID_LEVEL;
    }

    BAIL_ON_NT_STATUS(ntStatus);

    *pOut = pPolInfo;
    pPolInfo = NULL;

cleanup:
    return ntStatus;

error:
    LsaRpcFreeMemory((PVOID)pPolInfo);

    *pOut = NULL;
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
