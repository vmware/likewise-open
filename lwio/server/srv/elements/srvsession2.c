/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */
/*
 * Copyright Likewise Software
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
 *        srvsession.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Session Object
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
BOOLEAN
SrvSession2IsInParent_inlock(
    PLWIO_SRV_SESSION_2 pSession
    );

static
NTSTATUS
SrvSession2AddTree_inlock(
    PLWIO_SRV_SESSION_2 pSession,
    PLWIO_SRV_TREE_2 pTree
    );

static
NTSTATUS
SrvSession2CountTotalFiles(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvSession2UpdateLastActivityTime(
   PLWIO_SRV_SESSION_2 pSession
   );

static
NTSTATUS
SrvSession2UpdateLastActivityTime_inlock(
   PLWIO_SRV_SESSION_2 pSession
   );

static
NTSTATUS
SrvSession2AcquireTreeId_inlock(
   PLWIO_SRV_SESSION_2 pSession,
   PULONG              pulTid
   );

static
int
SrvSession2TreeCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvSession2TreeRelease(
    PVOID pTree
    );

static
VOID
SrvSession2Free(
    PLWIO_SRV_SESSION_2 pSession
    );

static
BOOLEAN
SrvSession2IsRundown_inlock(
    PLWIO_SRV_SESSION_2 pSession
    );

static
VOID
SrvSession2SetRundown_inlock(
    PLWIO_SRV_SESSION_2 pSession
    );

static
BOOLEAN
SrvSession2GatherRundownTreeListCallback(
    PLWIO_SRV_TREE_2 pTree,
    PVOID pContext
    );

static
VOID
SrvSession2RundownTreeList(
    PLWIO_SRV_TREE_2 pRundownList
    );

NTSTATUS
SrvSession2Create(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullUid,
    PLWIO_SRV_SESSION_2* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION_2 pSession = NULL;

    LWIO_LOG_DEBUG("Creating session [uid:%lu]", ullUid);

    ntStatus = SrvAllocateMemory(
                    sizeof(LWIO_SRV_SESSION_2),
                    (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->refcount = 1;

    pthread_rwlock_init(&pSession->mutex, NULL);
    pSession->pMutex = &pSession->mutex;

    pSession->pConnection = pConnection;
    SrvConnectionAcquire(pConnection);

    pSession->ulConnectionResourceId = pConnection->resource.ulResourceId;

    pSession->ullUid = ullUid;

    ntStatus = WireGetCurrentNTTime(&pSession->llBirthTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->llLastActivityTime = pSession->llBirthTime;

    LWIO_LOG_DEBUG("Associating session [object:0x%x][uid:%lu]",
                    pSession,
                    ullUid);

    ntStatus = LwRtlRBTreeCreate(
                    &SrvSession2TreeCompare,
                    NULL,
                    &SrvSession2TreeRelease,
                    &pSession->pTreeCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFinderCreateRepository(&pSession->hFinderRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    SRV_ELEMENTS_INCREMENT_SESSIONS;

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:

    *ppSession = NULL;

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    goto cleanup;
}

BOOLEAN
SrvSession2IsInParent(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    BOOLEAN bIsInParent = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);
    bIsInParent = SrvSession2IsInParent_inlock(pSession);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return bIsInParent;
}

static
BOOLEAN
SrvSession2IsInParent_inlock(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    return IsSetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
}

VOID
SrvSession2SetInParent(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);
    LWIO_ASSERT(!IsSetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    SetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);
}

VOID
SrvSession2ClearInParent(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);
    LWIO_ASSERT(IsSetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    ClearFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);
}

NTSTATUS
SrvSession2FindTree(
    PLWIO_SRV_SESSION_2 pSession,
    ULONG               ulTid,
    PLWIO_SRV_TREE_2*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_TREE_2 pTree = NULL;

    ntStatus = SrvSession2UpdateLastActivityTime(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    pTree = pSession->lruTree[ulTid % SRV_LRU_CAPACITY];

    if (!pTree || (pTree->ulTid != ulTid))
    {
        ntStatus = LwRtlRBTreeFind(
                        pSession->pTreeCollection,
                        &ulTid,
                        (PVOID*)&pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        pSession->lruTree[ ulTid % SRV_LRU_CAPACITY ] = pTree;
    }

    InterlockedIncrement(&pTree->refcount);

    *ppTree = pTree;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_NETWORK_NAME_DELETED;
    }

    *ppTree = NULL;

    goto cleanup;
}

NTSTATUS
SrvSession2RemoveTree(
    PLWIO_SRV_SESSION_2 pSession,
    PLWIO_SRV_TREE_2 pTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_TREE_2 pCachedTree = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = SrvSession2UpdateLastActivityTime_inlock(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    if (SrvTree2IsInParent(pTree))
    {
        pCachedTree = pSession->lruTree[ pTree->ulTid % SRV_LRU_CAPACITY ];
        if (pCachedTree && (pCachedTree->ulTid == pTree->ulTid))
        {
            pSession->lruTree[ pTree->ulTid % SRV_LRU_CAPACITY ] = NULL;
        }

        // removal automatically releases reference
        ntStatus = LwRtlRBTreeRemove(
                        pSession->pTreeCollection,
                        &pTree->ulTid);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvTree2ClearInParent(pTree);
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvSession2CreateTree(
    PLWIO_SRV_SESSION_2 pSession,
    PSRV_SHARE_INFO     pShareInfo,
    PLWIO_SRV_TREE_2*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_TREE_2 pTree = NULL;
    BOOLEAN bInLock = FALSE;
    ULONG   ulTid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    if (SrvSession2IsRundown_inlock(pSession))
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvSession2UpdateLastActivityTime_inlock(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2AcquireTreeId_inlock(
                    pSession,
                    &ulTid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2Create(
                    pSession,
                    ulTid,
                    pShareInfo,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2AddTree_inlock(pSession, pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppTree = pTree;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    *ppTree = NULL;

    if (pTree)
    {
        SrvTree2Rundown(pTree);
        SrvTree2Release(pTree);
    }

    goto cleanup;
}

static
NTSTATUS
SrvSession2AddTree_inlock(
    PLWIO_SRV_SESSION_2 pSession,
    PLWIO_SRV_TREE_2 pTree
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = LwRtlRBTreeAdd(
                    pSession->pTreeCollection,
                    &pTree->ulTid,
                    pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    // Reference from parent
    SrvTree2Acquire(pTree);
    SrvTree2SetInParent(pTree);

    pSession->lruTree[pTree->ulTid % SRV_LRU_CAPACITY] = pTree;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvSession2SetPrincipalName(
    PLWIO_SRV_SESSION_2 pSession,
    PCSTR               pszClientPrincipal
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PWSTR    pwszClientPrincipalName = NULL;

    if (!pszClientPrincipal)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBMbsToWc16s(
                    pszClientPrincipal,
                    &pwszClientPrincipalName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    SRV_SAFE_FREE_MEMORY(pSession->pwszClientPrincipalName);

    pSession->pwszClientPrincipalName = pwszClientPrincipalName;
    // pwszClientPrincipalName = NULL;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    SRV_SAFE_FREE_MEMORY(pwszClientPrincipalName);

    goto cleanup;
}

BOOLEAN
SrvSession2IsMatchPrincipal(
    PLWIO_SRV_SESSION_2 pSession,
    PCWSTR              pwszClientPrincipal
    )
{
    BOOLEAN bIsMatch = FALSE;
    BOOLEAN bInLock  = FALSE;

    if (IsNullOrEmptyString(pwszClientPrincipal))
    {
        bIsMatch = TRUE;
    }
    else
    {
        LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

        if (pSession->pwszClientPrincipalName &&
            (0 == SMBWc16sCaseCmp(pSession->pwszClientPrincipalName,
                                  pwszClientPrincipal)))
        {
            bIsMatch = TRUE;
        }

        LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);
    }

    return bIsMatch;
}

NTSTATUS
SrvSession2GetPrincipalName(
    PLWIO_SRV_SESSION_2 pSession,
    PWSTR*              ppwszClientPrincipal
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PWSTR    pwszClientPrincipal = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    if (pSession->pwszClientPrincipalName)
    {
        ntStatus = SMBAllocateStringW(
                        pSession->pwszClientPrincipalName,
                        &pwszClientPrincipal);
    }
    else
    {
        ntStatus = STATUS_NO_USER_SESSION_KEY;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszClientPrincipal = pwszClientPrincipal;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *ppwszClientPrincipal = NULL;

    goto cleanup;
}

VOID
SrvSession2SetUserFlags(
    PLWIO_SRV_SESSION_2 pSession,
    ULONG Flags
    )
{
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    SetFlag(pSession->ulUserFlags, Flags);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return;
}

NTSTATUS
SrvSession2GetFileCount(
    PLWIO_SRV_SESSION_2 pSession,
    PULONG64            pullFileCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock = FALSE;
    ULONG64  ullTotalFileCount = 0;

    ntStatus = SrvSession2UpdateLastActivityTime(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    ntStatus = LwRtlRBTreeTraverse(
                        pSession->pTreeCollection,
                        LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                        &SrvSession2CountTotalFiles,
                        &ullTotalFileCount);
    BAIL_ON_NT_STATUS(ntStatus);

    *pullFileCount = ullTotalFileCount;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *pullFileCount = 0;

    goto cleanup;
}

static
NTSTATUS
SrvSession2CountTotalFiles(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_SRV_TREE_2 pTree = (PLWIO_SRV_TREE_2)pData;
    PULONG64 pullTotalFileCount = (PULONG64)pUserData;
    BOOLEAN bTreeInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pTree->mutex);

    *pullTotalFileCount += pTree->ulNumOpenFiles;

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTree->mutex);

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

PLWIO_SRV_SESSION_2
SrvSession2Acquire(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    LWIO_LOG_DEBUG("Acquiring session [uid:%u]", pSession->ullUid);

    InterlockedIncrement(&pSession->refcount);

    return pSession;
}

VOID
SrvSession2Release(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    LWIO_LOG_DEBUG("Releasing session [uid:%u]", pSession->ullUid);

    if (InterlockedDecrement(&pSession->refcount) == 0)
    {
        SRV_ELEMENTS_DECREMENT_SESSIONS;

        SrvSession2Free(pSession);
    }
}

VOID
SrvSession2Rundown(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bDoRundown = FALSE;
    BOOLEAN bIsInParent = FALSE;
    PLWIO_SRV_TREE_2 pRundownTreeList = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    SrvSession2UpdateLastActivityTime_inlock(pSession);

    if (!SrvSession2IsRundown_inlock(pSession))
    {
        SrvSession2SetRundown_inlock(pSession);

        bDoRundown = TRUE;
        bIsInParent = SrvSession2IsInParent_inlock(pSession);

        SrvEnumTree2Collection(
                pSession->pTreeCollection,
                SrvSession2GatherRundownTreeListCallback,
                &pRundownTreeList);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    if (bIsInParent)
    {
        SrvConnection2RemoveSession(pSession->pConnection, pSession);
    }

    if (bDoRundown)
    {
        // Cannot rundown with lock held as they self-remove
        SrvSession2RundownTreeList(pRundownTreeList);
    }
}

static
NTSTATUS
SrvSession2UpdateLastActivityTime(
   PLWIO_SRV_SESSION_2 pSession
   )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = SrvSession2UpdateLastActivityTime_inlock(pSession);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;
}

static
NTSTATUS
SrvSession2UpdateLastActivityTime_inlock(
   PLWIO_SRV_SESSION_2 pSession
   )
{
    return WireGetCurrentNTTime(&pSession->llLastActivityTime);
}

static
NTSTATUS
SrvSession2AcquireTreeId_inlock(
   PLWIO_SRV_SESSION_2 pSession,
   PULONG              pulTid
   )
{
    NTSTATUS ntStatus = 0;
    ULONG    ulCandidateTid = pSession->ulNextAvailableTid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_SRV_TREE_2 pTree = NULL;

        /* 0 is never a valid tid */

        if ((ulCandidateTid == 0) || (ulCandidateTid == UINT32_MAX))
        {
            ulCandidateTid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pSession->pTreeCollection,
                        &ulCandidateTid,
                        (PVOID*)&pTree);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            ulCandidateTid++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((ulCandidateTid != pSession->ulNextAvailableTid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_LINKS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pulTid = ulCandidateTid;

    /* Increment by 1 by make sure tyo deal with wraparound */

    ulCandidateTid++;
    pSession->ulNextAvailableTid = ulCandidateTid ? ulCandidateTid : 1;

cleanup:

    return ntStatus;

error:

    *pulTid = 0;

    goto cleanup;
}

static
int
SrvSession2TreeCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG pTid1 = (PULONG)pKey1;
    PULONG pTid2 = (PULONG)pKey2;

    assert (pTid1 != NULL);
    assert (pTid2 != NULL);

    if (*pTid1 > *pTid2)
    {
        return 1;
    }
    else if (*pTid1 < *pTid2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
SrvSession2TreeRelease(
    PVOID pTree
    )
{
    SrvTree2Release((PLWIO_SRV_TREE_2)pTree);
}

static
VOID
SrvSession2Free(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    LWIO_LOG_DEBUG("Freeing session [object:0x%x][uid:%u]",
                    pSession,
                    pSession->ullUid);

    // Cannot be in the parent since parent would have a reference.
    LWIO_ASSERT(!SrvSession2IsInParent_inlock(pSession));

    if (pSession->pMutex)
    {
        pthread_rwlock_destroy(&pSession->mutex);
        pSession->pMutex = NULL;
    }

    if (pSession->pTreeCollection)
    {
        LwRtlRBTreeFree(pSession->pTreeCollection);
    }

    if (pSession->hFinderRepository)
    {
        SrvFinderCloseRepository(pSession->hFinderRepository);
    }

    SRV_SAFE_FREE_MEMORY(pSession->pwszClientPrincipalName);

    if (pSession->pIoSecurityContext) {
        IoSecurityDereferenceSecurityContext(&pSession->pIoSecurityContext);
    }

    if (pSession->pOEMSession)
    {
        SrvOEMCloseSession(
                pSession->pOEMSession,
                pSession->ulOEMSessionLength);
    }

    // Release parent at the end
    if (pSession->pConnection)
    {
        SrvConnectionRelease(pSession->pConnection);
    }

    SrvFreeMemory(pSession);
}

static
BOOLEAN
SrvSession2IsRundown_inlock(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    return IsSetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}

static
VOID
SrvSession2SetRundown_inlock(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    LWIO_ASSERT(!SrvSession2IsRundown_inlock(pSession));
    SetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}

static
BOOLEAN
SrvSession2GatherRundownTreeListCallback(
    PLWIO_SRV_TREE_2 pTree,
    PVOID pContext
    )
{
    PLWIO_SRV_TREE_2* ppRundownList = (PLWIO_SRV_TREE_2*) pContext;

    LWIO_ASSERT(!pTree->pRundownNext);
    pTree->pRundownNext = *ppRundownList;
    *ppRundownList = SrvTree2Acquire(pTree);

    return TRUE;
}

static
VOID
SrvSession2RundownTreeList(
    PLWIO_SRV_TREE_2 pRundownList
    )
{
    while (pRundownList)
    {
        PLWIO_SRV_TREE_2 pTree = pRundownList;

        pRundownList = pTree->pRundownNext;
        SrvTree2Rundown(pTree);
        SrvTree2Release(pTree);
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
