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
SrvSessionIsInParent_inlock(
    PLWIO_SRV_SESSION pSession
    );

static
NTSTATUS
SrvSessionAddTree_inlock(
    PLWIO_SRV_SESSION pSession,
    PLWIO_SRV_TREE pTree
    );

static
NTSTATUS
SrvSessionCountTotalFiles(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvSessionUpdateLastActivityTime(
   PLWIO_SRV_SESSION pSession
   );

static
NTSTATUS
SrvSessionUpdateLastActivityTime_inlock(
   PLWIO_SRV_SESSION pSession
   );

static
NTSTATUS
SrvSessionAcquireTreeId_inlock(
   PLWIO_SRV_SESSION pSession,
   PUSHORT          pTid
   );

static
int
SrvSessionTreeCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvSessionTreeRelease(
    PVOID pTree
    );

static
VOID
SrvSessionFree(
    PLWIO_SRV_SESSION pSession
    );

static
BOOLEAN
SrvSessionIsRundown_inlock(
    PLWIO_SRV_SESSION pSession
    );

static
VOID
SrvSessionSetRundown_inlock(
    PLWIO_SRV_SESSION pSession
    );

static
BOOLEAN
SrvSessionGatherRundownTreeListCallback(
    PLWIO_SRV_TREE pTree,
    PVOID pContext
    );

static
VOID
SrvSessionRundownTreeList(
    PLWIO_SRV_TREE pRundownList
    );

NTSTATUS
SrvSessionCreate(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT            uid,
    PLWIO_SRV_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;

    LWIO_LOG_DEBUG("Creating session [uid:%u]", uid);

    ntStatus = SrvAllocateMemory(
                    sizeof(LWIO_SRV_SESSION),
                    (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->refcount = 1;

    pthread_rwlock_init(&pSession->mutex, NULL);
    pSession->pMutex = &pSession->mutex;

    pSession->pConnection = pConnection;
    SrvConnectionAcquire(pConnection);

    pSession->uid = uid;
    pSession->ulConnectionResourceId = pConnection->resource.ulResourceId;

    ntStatus = WireGetCurrentNTTime(&pSession->llBirthTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->llLastActivityTime = pSession->llBirthTime;

    LWIO_LOG_DEBUG("Associating session [object:0x%x][uid:%u]", pSession, uid);

    ntStatus = LwRtlRBTreeCreate(
                    &SrvSessionTreeCompare,
                    NULL,
                    &SrvSessionTreeRelease,
                    &pSession->pTreeCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFinderCreateRepository(
                    &pSession->hFinderRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    SRV_ELEMENTS_INCREMENT_SESSIONS;

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:

    *ppSession = NULL;

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    goto cleanup;
}

BOOLEAN
SrvSessionIsInParent(
    PLWIO_SRV_SESSION pSession
    )
{
    BOOLEAN bIsInParent = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);
    bIsInParent = SrvSessionIsInParent_inlock(pSession);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return bIsInParent;
}

static
BOOLEAN
SrvSessionIsInParent_inlock(
    PLWIO_SRV_SESSION pSession
    )
{
    return IsSetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
}

VOID
SrvSessionSetInParent(
    PLWIO_SRV_SESSION pSession
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);
    LWIO_ASSERT(!IsSetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    SetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);
}

VOID
SrvSessionClearInParent(
    PLWIO_SRV_SESSION pSession
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);
    LWIO_ASSERT(IsSetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    ClearFlag(pSession->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);
}

NTSTATUS
SrvSessionFindTree(
    PLWIO_SRV_SESSION pSession,
    USHORT            tid,
    PLWIO_SRV_TREE*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_TREE pTree = NULL;

    ntStatus = SrvSessionUpdateLastActivityTime(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    pTree = pSession->lruTree[tid % SRV_LRU_CAPACITY];

    if (!pTree || (pTree->tid != tid))
    {
        ntStatus = LwRtlRBTreeFind(
                        pSession->pTreeCollection,
                        &tid,
                        (PVOID*)&pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        pSession->lruTree[tid % SRV_LRU_CAPACITY] = pTree;
    }

    InterlockedIncrement(&pTree->refcount);

    *ppTree = pTree;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_INVALID_HANDLE;
    }

    *ppTree = NULL;

    goto cleanup;
}

NTSTATUS
SrvSessionRemoveTree(
    PLWIO_SRV_SESSION pSession,
    PLWIO_SRV_TREE pTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_TREE pCachedTree = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = SrvSessionUpdateLastActivityTime_inlock(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    if (SrvTreeIsInParent(pTree))
    {
        pCachedTree = pSession->lruTree[ pTree->tid % SRV_LRU_CAPACITY ];
        if (pCachedTree && (pCachedTree->tid == pTree->tid))
        {
            pSession->lruTree[ pTree->tid % SRV_LRU_CAPACITY ] = NULL;
        }

        // removal automatically releases reference
        ntStatus = LwRtlRBTreeRemove(
                        pSession->pTreeCollection,
                        &pTree->tid);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvTreeClearInParent(pTree);
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvSessionCreateTree(
    PLWIO_SRV_SESSION pSession,
    PSRV_SHARE_INFO   pShareInfo,
    PLWIO_SRV_TREE*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_TREE pTree = NULL;
    BOOLEAN bInLock = FALSE;
    USHORT  tid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    if (SrvSessionIsRundown_inlock(pSession))
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvSessionUpdateLastActivityTime_inlock(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionAcquireTreeId_inlock(
                    pSession,
                    &tid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeCreate(
                    pSession,
                    tid,
                    pShareInfo,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionAddTree_inlock(pSession, pTree);
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
        SrvTreeRundown(pTree);
        SrvTreeRelease(pTree);
    }

    goto cleanup;
}

static
NTSTATUS
SrvSessionAddTree_inlock(
    PLWIO_SRV_SESSION pSession,
    PLWIO_SRV_TREE pTree
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = LwRtlRBTreeAdd(
                    pSession->pTreeCollection,
                    &pTree->tid,
                    pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    // Reference from parent
    SrvTreeAcquire(pTree);
    SrvTreeSetInParent(pTree);

    pSession->lruTree[pTree->tid % SRV_LRU_CAPACITY] = pTree;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvSessionSetPrincipalName(
    PLWIO_SRV_SESSION pSession,
    PCSTR             pszClientPrincipal
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

    ntStatus = SMBMbsToWc16s(pszClientPrincipal, &pwszClientPrincipalName);
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
SrvSessionIsMatchPrincipal(
    PLWIO_SRV_SESSION pSession,
    PCWSTR            pwszClientPrincipal
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
SrvSessionGetPrincipalName(
    PLWIO_SRV_SESSION pSession,
    PWSTR*            ppwszClientPrincipal
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
SrvSessionSetUserFlags(
    PLWIO_SRV_SESSION pSession,
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
SrvSessionGetFileCount(
    PLWIO_SRV_SESSION pSession,
    PULONG64          pullFileCount
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock = FALSE;
    ULONG64  ullTotalFileCount = 0;

    ntStatus = SrvSessionUpdateLastActivityTime(pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    ntStatus = LwRtlRBTreeTraverse(
                    pSession->pTreeCollection,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvSessionCountTotalFiles,
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
SrvSessionCountTotalFiles(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_SRV_TREE pTree = (PLWIO_SRV_TREE)pData;
    PULONG64 pullTotalFileCount = (PULONG64)pUserData;
    BOOLEAN bTreeInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pTree->mutex);

    *pullTotalFileCount += pTree->ulNumOpenFiles;

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTree->mutex);

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

PLWIO_SRV_SESSION
SrvSessionAcquire(
    PLWIO_SRV_SESSION pSession
    )
{
    LWIO_LOG_DEBUG("Acquiring session [uid:%u]", pSession->uid);

    InterlockedIncrement(&pSession->refcount);

    return pSession;
}

VOID
SrvSessionRelease(
    PLWIO_SRV_SESSION pSession
    )
{
    LWIO_LOG_DEBUG("Releasing session [uid:%u]", pSession->uid);

    if (InterlockedDecrement(&pSession->refcount) == 0)
    {
        SRV_ELEMENTS_DECREMENT_SESSIONS;

        SrvSessionFree(pSession);
    }
}

VOID
SrvSessionRundown(
    PLWIO_SRV_SESSION pSession
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bDoRundown = FALSE;
    BOOLEAN bIsInParent = FALSE;
    PLWIO_SRV_TREE pRundownTreeList = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    SrvSessionUpdateLastActivityTime_inlock(pSession);

    if (!SrvSessionIsRundown_inlock(pSession))
    {
        SrvSessionSetRundown_inlock(pSession);

        bDoRundown = TRUE;
        bIsInParent = SrvSessionIsInParent_inlock(pSession);

        SrvEnumTreeCollection(
                pSession->pTreeCollection,
                SrvSessionGatherRundownTreeListCallback,
                &pRundownTreeList);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    if (bIsInParent)
    {
        SrvConnectionRemoveSession(pSession->pConnection, pSession);
    }

    if (bDoRundown)
    {
        // Cannot rundown with lock held as they self-remove
        SrvSessionRundownTreeList(pRundownTreeList);
    }
}

static
NTSTATUS
SrvSessionUpdateLastActivityTime(
   PLWIO_SRV_SESSION pSession
   )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = SrvSessionUpdateLastActivityTime_inlock(pSession);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;
}

static
NTSTATUS
SrvSessionUpdateLastActivityTime_inlock(
   PLWIO_SRV_SESSION pSession
   )
{
    return WireGetCurrentNTTime(&pSession->llLastActivityTime);
}

static
NTSTATUS
SrvSessionAcquireTreeId_inlock(
   PLWIO_SRV_SESSION pSession,
   PUSHORT          pTid
   )
{
    NTSTATUS ntStatus = 0;
    USHORT   candidateTid = pSession->nextAvailableTid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_SRV_TREE pTree = NULL;

        /* 0 is never a valid tid */

        if ((candidateTid == 0) || (candidateTid == UINT16_MAX))
        {
            candidateTid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pSession->pTreeCollection,
                        &candidateTid,
                        (PVOID*)&pTree);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            candidateTid++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateTid != pSession->nextAvailableTid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_LINKS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pTid = candidateTid;

    /* Increment by 1 by make sure tyo deal with wraparound */

    candidateTid++;
    pSession->nextAvailableTid = candidateTid ? candidateTid : 1;

cleanup:

    return ntStatus;

error:

    *pTid = 0;

    goto cleanup;
}

static
int
SrvSessionTreeCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PUSHORT pTid1 = (PUSHORT)pKey1;
    PUSHORT pTid2 = (PUSHORT)pKey2;

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
SrvSessionTreeRelease(
    PVOID pTree
    )
{
    SrvTreeRelease((PLWIO_SRV_TREE)pTree);
}

static
VOID
SrvSessionFree(
    PLWIO_SRV_SESSION pSession
    )
{
    LWIO_LOG_DEBUG("Freeing session [object:0x%x][uid:%u]",
                    pSession,
                    pSession->uid);

    // Cannot be in the parent since parent would have a reference.
    LWIO_ASSERT(!SrvSessionIsInParent_inlock(pSession));

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

    if (pSession->pwszClientPrincipalName)
    {
        SrvFreeMemory(pSession->pwszClientPrincipalName);
    }

    if (pSession->pIoSecurityContext)
    {
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
SrvSessionIsRundown_inlock(
    PLWIO_SRV_SESSION pSession
    )
{
    return IsSetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}

static
VOID
SrvSessionSetRundown_inlock(
    PLWIO_SRV_SESSION pSession
    )
{
    LWIO_ASSERT(!SrvSessionIsRundown_inlock(pSession));
    SetFlag(pSession->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}

static
BOOLEAN
SrvSessionGatherRundownTreeListCallback(
    PLWIO_SRV_TREE pTree,
    PVOID pContext
    )
{
    PLWIO_SRV_TREE* ppRundownList = (PLWIO_SRV_TREE*) pContext;

    LWIO_ASSERT(!pTree->pRundownNext);
    pTree->pRundownNext = *ppRundownList;
    *ppRundownList = SrvTreeAcquire(pTree);

    return TRUE;
}

static
VOID
SrvSessionRundownTreeList(
    PLWIO_SRV_TREE pRundownList
    )
{
    while (pRundownList)
    {
        PLWIO_SRV_TREE pTree = pRundownList;

        pRundownList = pTree->pRundownNext;
        SrvTreeRundown(pTree);
        SrvTreeRelease(pTree);
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
