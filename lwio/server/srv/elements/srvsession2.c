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
NTSTATUS
SrvSession2RundownTreeRbTreeVisit(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

NTSTATUS
SrvSession2Create(
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
    ULONG               ulTid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_TREE_2 pTree = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = SrvSession2UpdateLastActivityTime_inlock(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree = pSession->lruTree[ ulTid % SRV_LRU_CAPACITY ];
    if (pTree && (pTree->ulTid == ulTid))
    {
        pSession->lruTree[ ulTid % SRV_LRU_CAPACITY ] = NULL;
    }

    ntStatus = LwRtlRBTreeRemove(
                    pSession->pTreeCollection,
                    &ulTid);
    BAIL_ON_NT_STATUS(ntStatus);

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

    ntStatus = SrvSession2UpdateLastActivityTime_inlock(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2AcquireTreeId_inlock(
                    pSession,
                    &ulTid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2Create(
                    ulTid,
                    pShareInfo,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pSession->pTreeCollection,
                    &pTree->ulTid,
                    pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pTree->refcount);

    *ppTree = pTree;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *ppTree = NULL;

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    goto cleanup;
}

NTSTATUS
SrvSession2IncrementFileCount(
    PLWIO_SRV_SESSION_2 pSession
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = SrvSession2UpdateLastActivityTime_inlock(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->ullTotalFileCount++;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvSession2GetFileCount(
    PLWIO_SRV_SESSION_2 pSession,
    PULONG64            pullFileCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    ntStatus = SrvSession2UpdateLastActivityTime(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    *pullFileCount = pSession->ullTotalFileCount;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *pullFileCount = 0;

    goto cleanup;
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

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    SrvSession2UpdateLastActivityTime_inlock(pSession);

    LwRtlRBTreeTraverse(
            pSession->pTreeCollection,
            LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
            SrvSession2RundownTreeRbTreeVisit,
            NULL);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);
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

    IO_SAFE_FREE_MEMORY(pSession->pszClientPrincipalName);

    if (pSession->pIoSecurityContext) {
        IoSecurityDereferenceSecurityContext(&pSession->pIoSecurityContext);
    }

    SrvFreeMemory(pSession);
}

static
NTSTATUS
SrvSession2RundownTreeRbTreeVisit(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_SRV_TREE_2 pTree = (PLWIO_SRV_TREE_2)pData;

    if (pTree)
    {
        SrvTree2Rundown(pTree);
    }

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
