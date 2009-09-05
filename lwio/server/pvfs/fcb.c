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
 *        fcb.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        File Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

/* File Globals */

typedef struct _PVFS_FCB_TABLE
{
    pthread_rwlock_t rwLock;

    PLWRTL_RB_TREE pFcbTree;

} PVFS_FCB_TABLE;

static PVFS_FCB_TABLE gFcbTable;


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsFreeFCB(
    PPVFS_FCB pFcb
    )
{
    PLW_LIST_LINKS pLinks = NULL;
    PPVFS_OPLOCK_RECORD pOplockRec = NULL;
    PIRP pIrp = NULL;

    if (!pFcb) {
        return;
    }

    RtlCStringFree(&pFcb->pszFilename);

    pthread_mutex_destroy(&pFcb->ControlBlock);
    pthread_mutex_destroy(&pFcb->mutexOplock);
    pthread_rwlock_destroy(&pFcb->rwCcbLock);
    pthread_rwlock_destroy(&pFcb->rwBrlLock);

    LwRtlQueueDestroy(&pFcb->pPendingLockQueue);
    LwRtlQueueDestroy(&pFcb->pOplockPendingOpsQueue);
    LwRtlQueueDestroy(&pFcb->pOplockReadyOpsQueue);

    while (!LwListIsEmpty(&pFcb->OplockList))
    {
        pLinks = LwListRemoveTail(&pFcb->OplockList);
        pOplockRec = LW_STRUCT_FROM_FIELD(pLinks, PVFS_OPLOCK_RECORD, Oplocks);

        pIrp = pOplockRec->pIrpContext->pIrp;

        pIrp->IoStatusBlock.Status = STATUS_FILE_CLOSED;
        IoIrpComplete(pIrp);

        PvfsFreeIrpContext(&pOplockRec->pIrpContext);
        PvfsFreeOplockRecord(&pOplockRec);
    }

    PVFS_FREE(&pFcb);

    return;
}

/***********************************************************
 **********************************************************/

VOID
PvfsFreePendingLock(
    PVOID *ppData
    )
{
    if (!ppData || !*ppData) {
        return;
    }

    PVFS_FREE(ppData);

    return;
}

NTSTATUS
PvfsAllocateFCB(
    PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;

    *ppFcb = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pFcb, sizeof(PVFS_FCB));
    BAIL_ON_NT_STATUS(ntError);

    pFcb->bDeleteOnClose = FALSE;

    /* Setup pendlock byte-range lock queue */

    ntError = LwRtlQueueInit(&pFcb->pPendingLockQueue,
                             PVFS_FCB_MAX_PENDING_LOCKS,
                             PvfsFreePendingLock);
    BAIL_ON_NT_STATUS(ntError);

    /* Oplock pending ops queue */

    ntError = LwRtlQueueInit(&pFcb->pOplockPendingOpsQueue,
                             PVFS_FCB_MAX_PENDING_OPERATIONS,
                             NULL /* TODO - generic free function */);
    BAIL_ON_NT_STATUS(ntError);

    ntError = LwRtlQueueInit(&pFcb->pOplockReadyOpsQueue,
                             PVFS_FCB_MAX_PENDING_OPERATIONS,
                             NULL /* TODO - generic free function */);
    BAIL_ON_NT_STATUS(ntError);

    /* Oplock list and state */

    LwListInit(&pFcb->OplockList);
    pFcb->bOplockBreakInProgress = FALSE;

    /* Initialize mutexes and refcounts */

    pthread_mutex_init(&pFcb->ControlBlock, NULL);
    pthread_mutex_init(&pFcb->mutexOplock, NULL);
    pthread_rwlock_init(&pFcb->rwCcbLock, NULL);
    pthread_rwlock_init(&pFcb->rwBrlLock, NULL);

    /* Add initial ref count */

    pFcb->RefCount = 1;

    *ppFcb = pFcb;
    pFcb = NULL;

    ntError = STATUS_SUCCESS;

cleanup:
    PvfsFreeFCB(pFcb);

    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

static NTSTATUS
PvfsRemoveFCB(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* We must have the mutex locked exclusively coming
       into this */

    ntError = LwRtlRBTreeRemove(gFcbTable.pFcbTree,
                               (PVOID)pFcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

ULONG
PvfsReferenceFCB(
    IN PPVFS_FCB pFcb
    )
{
    return InterlockedIncrement(&pFcb->RefCount);
}

/*******************************************************
 ******************************************************/


static NTSTATUS
PvfsSetLastWriteTime(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVFS_STAT Stat = {0};
    LONG64 LastAccessTime = 0;

    /* Need the original access time */

    ntError = PvfsSysStat(pFcb->pszFilename, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&LastAccessTime, Stat.s_atime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysUtime(pFcb->pszFilename,
                           pFcb->LastWriteTime,
                           LastAccessTime);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;

}

static NTSTATUS
PvfsExecuteDeleteOnClose(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    /* Check for renames */

    ntError = PvfsValidatePath(pFcb->pszFilename, &pFcb->FileId);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysRemove(pFcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


VOID
PvfsReleaseFCB(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;

    /* It is important to lock the FcbTable here so that
       there is no window between the decrement and the remove.
       Otherwise another open request could search and locate the
       FCB in the tree and return free()'d memory

       Keep the FcbTable locked until any cleanup operations
       (DeleteOnClose are SetLastWriteTime) are done.
    */

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &gFcbTable.rwLock);

    if (InterlockedDecrement(&pFcb->RefCount) == 0)
    {
        PvfsRemoveFCB(pFcb);

        if (pFcb->LastWriteTime != 0) {
            ntError = PvfsSetLastWriteTime(pFcb);
            /* Don't fail */
        }

        if (pFcb->bDeleteOnClose)
        {
            ntError = PvfsExecuteDeleteOnClose(pFcb);
            /* Don't fail */
        }

        LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

        PvfsFreeFCB(pFcb);
    }

    LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

    return;
}

/*******************************************************
 ******************************************************/

static int
FcbTableFilenameCompare(
    PVOID a,
    PVOID b
    )
{
    int iReturn = 0;

    PSTR pszFilename1 = (PSTR)a;
    PSTR pszFilename2 = (PSTR)b;

    iReturn = RtlCStringCompare(pszFilename1, pszFilename2, TRUE);

    return iReturn;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsInitializeFCBTable(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    pthread_rwlock_init(&gFcbTable.rwLock, NULL);

    ntError = LwRtlRBTreeCreate(&FcbTableFilenameCompare,
                                NULL,
                                NULL,
                                &gFcbTable.pFcbTree);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsDestroyFCBTable(
    VOID
    )
{
    /* Need to add tree traversal here and remove
       data */

    LwRtlRBTreeFree(gFcbTable.pFcbTree);
    pthread_rwlock_destroy(&gFcbTable.rwLock);

    PVFS_ZERO_MEMORY(&gFcbTable);

    return STATUS_SUCCESS;
}

/*******************************************************
 ******************************************************/

static NTSTATUS
_PvfsFindFCB(
    PPVFS_FCB *ppFcb,
    PSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;

    ntError = LwRtlRBTreeFind(gFcbTable.pFcbTree,
                              (PVOID)pszFilename,
                              (PVOID*)&pFcb);
    if (ntError == STATUS_NOT_FOUND) {
        ntError = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntError);

    PvfsReferenceFCB(pFcb);

    *ppFcb = pFcb;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsFindFCB(
    PPVFS_FCB *ppFcb,
    PSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bLocked, &gFcbTable.rwLock);

    ntError = _PvfsFindFCB(ppFcb, pszFilename);

    LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

    return ntError;
}


/*******************************************************
 ******************************************************/

static NTSTATUS
PvfsAddFCB(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    ntError = LwRtlRBTreeAdd(gFcbTable.pFcbTree,
                             (PVOID)pFcb->pszFilename,
                             (PVOID)pFcb);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsCreateFCB(
    OUT PPVFS_FCB *ppFcb,
    IN  PSTR pszFilename,
    IN  FILE_SHARE_FLAGS SharedAccess,
    IN  ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    BOOLEAN bFcbTableLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbTableLocked, &gFcbTable.rwLock);

    /* Protect against adding a duplicate */

    ntError = _PvfsFindFCB(&pFcb, pszFilename);
    if (ntError == STATUS_SUCCESS) {

        ntError = PvfsEnforceShareMode(pFcb,
                                       SharedAccess,
                                       DesiredAccess);
        BAIL_ON_NT_STATUS(ntError);

        *ppFcb = pFcb;

        goto cleanup;
    }

    ntError = PvfsAllocateFCB(&pFcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCStringDuplicate(&pFcb->pszFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Add to the file handle table */

    ntError = PvfsAddFCB(pFcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Return a reference to the FCB */

    *ppFcb = pFcb;
    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbTableLocked, &gFcbTable.rwLock);

    return ntError;

error:
    if (pFcb) {
        PvfsReleaseFCB(pFcb);
    }
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsAddCCBToFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB_LIST_NODE pCcbNode = NULL;
    BOOLEAN bFcbWriteLocked = FALSE;

    ntError = PvfsAllocateMemory((PVOID*)&pCcbNode,
                                 sizeof(PVFS_CCB_LIST_NODE));
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwCcbLock);

    /* Add to the front of the list */

    pCcbNode->pCcb  = pCcb;
    pCcbNode->pNext = pFcb->pCcbList;
    if (pFcb->pCcbList) {
        pFcb->pCcbList->pPrevious = pCcbNode;
    }

    pFcb->pCcbList  = pCcbNode;
    pFcb->CcbCount++;

    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwCcbLock);

    pCcb->pFcb = pFcb;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsRemoveCCBFromFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB_LIST_NODE pCursor = NULL;
    PPVFS_CCB_LIST_NODE pTmp = NULL;
    BOOLEAN bFcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwCcbLock);

    for (pCursor=pFcb->pCcbList; pCursor; pCursor = pCursor->pNext)
    {
        if (pCursor->pCcb == pCcb) {
            break;
        }
        pTmp = pCursor;
    }

    if (!pCursor) {
        ntError = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (pCursor == pFcb->pCcbList) {
        pFcb->pCcbList = pCursor->pNext;
        if (pFcb->pCcbList) {
            pFcb->pCcbList->pPrevious = NULL;
        }
    } else {
        pTmp->pNext = pCursor->pNext;
        if (pCursor->pNext) {
            pCursor->pNext->pPrevious = pTmp;
        }

    }

    pFcb->CcbCount--;
    PVFS_FREE(&pCursor);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwCcbLock);

    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

PPVFS_CCB_LIST_NODE
PvfsNextCCBFromList(
    PPVFS_FCB pFcb,
    PPVFS_CCB_LIST_NODE pCurrent
    )
{
    if (pCurrent == NULL) {
        return pFcb->pCcbList;
    }

    return pCurrent->pNext;
}

/*******************************************************
 ******************************************************/

PPVFS_CCB_LIST_NODE
PvfsPreviousCCBFromList(
    PPVFS_FCB pFcb,
    PPVFS_CCB_LIST_NODE pCurrent
    )
{
    if (pCurrent == NULL) {
        return NULL;
    }

    return pCurrent->pPrevious;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsPendOplockBreakTest(
    IN PPVFS_FCB pFcb,
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_OPLOCK_BREAK_TEST pTestCtx = NULL;

    ntError = PvfsCreateOplockBreakTestContext(
                  &pTestCtx,
                  pFcb,
                  pIrpContext,
                  pCcb,
                  pfnCompletion,
                  pfnFreeContext,
                  pCompletionContext);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAddItemPendingOplockBreakAck(
                  pFcb,
                  pIrpContext,
                  (PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK)PvfsOplockPendingBreakIfLocked,
                  (PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX)PvfsFreeOplockBreakTestContext,
                  (PVOID)pTestCtx);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    PvfsFreeOplockBreakTestContext(&pTestCtx);

    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsAddItemPendingOplockBreakAck(
    IN PPVFS_FCB pFcb,
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;
    PPVFS_OPLOCK_PENDING_OPERATION pPendingOp = NULL;

    BAIL_ON_INVALID_PTR(pFcb, ntError);
    BAIL_ON_INVALID_PTR(pfnCompletion, ntError);

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pPendingOp,
                  sizeof(PVFS_OPLOCK_PENDING_OPERATION));
    BAIL_ON_NT_STATUS(ntError);

    pPendingOp->pIrpContext = pIrpContext;
    pPendingOp->pfnCompletion = pfnCompletion;
    pPendingOp->pfnFreeContext = pfnFreeContext;
    pPendingOp->pCompletionContext = pCompletionContext;

    LWIO_LOCK_MUTEX(bLocked, &pFcb->mutexOplock);

    ntError = LwRtlQueueAddItem(pFcb->pOplockPendingOpsQueue,
                                (PVOID)pPendingOp);
    BAIL_ON_NT_STATUS(ntError);

    /* Some calls such as LockControl may have already been
       pended before this block */

    if (!pIrpContext->bIsPended)
    {
        IoIrpMarkPending(pIrpContext->pIrp, PvfsCancelIrp, pIrpContext);
        pIrpContext->bIsPended = TRUE;
    }

cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->mutexOplock);

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFileHasOtherOpens(
    IN PPVFS_FCB pFcb,
    IN PPVFS_CCB pCcb
    )
{
    PPVFS_CCB_LIST_NODE pCursor = NULL;
    BOOLEAN bNonSelfOpen = FALSE;
    BOOLEAN bFcbReadLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pFcb->rwCcbLock);

    for (pCursor = PvfsNextCCBFromList(pFcb, pCursor);
         pCursor;
         pCursor = PvfsNextCCBFromList(pFcb, pCursor))
    {
        /* We'll deal with ourselves in AddLock() */

        if (pCcb != pCursor->pCcb) {
            bNonSelfOpen = TRUE;
            break;
        }
    }

    LWIO_UNLOCK_RWMUTEX(bFcbReadLocked, &pFcb->rwCcbLock);

    return bNonSelfOpen;
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFileIsOplocked(
    IN PPVFS_FCB pFcb
    )
{
    return !LwListIsEmpty(&pFcb->OplockList);
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFileIsOplockedExclusive(
    IN PPVFS_FCB pFcb
    )
{
    BOOLEAN bExclusiveOplock = FALSE;
    PPVFS_OPLOCK_RECORD pOplockRec = NULL;

    if (LwListIsEmpty(&pFcb->OplockList)) {
        return FALSE;
    }

    /* We only need to check the list head since the list itself must
       be consistent and non-conflicting */

    pOplockRec = LW_STRUCT_FROM_FIELD(
                     &pFcb->OplockList,
                     PVFS_OPLOCK_RECORD,
                     Oplocks);

    if ((pOplockRec->OplockType == IO_OPLOCK_REQUEST_OPLOCK_BATCH) ||
        (pOplockRec->OplockType == IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1))
    {
        bExclusiveOplock = TRUE;
    }

    return bExclusiveOplock;
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFileIsOplockedShared(
    IN PPVFS_FCB pFcb
    )
{
    if (LwListIsEmpty(&pFcb->OplockList)) {
        return FALSE;
    }

    return !PvfsFileIsOplockedExclusive(pFcb);
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsStoreOplockRecord(
    IN OUT PPVFS_FCB pFcb,
    IN     PPVFS_OPLOCK_RECORD pOplock
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    LwListInsertTail(&pFcb->OplockList, &pOplock->Oplocks);

    return ntError;
}

NTSTATUS
PvfsAddOplockRecord(
    IN OUT PPVFS_FCB pFcb,
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PPVFS_CCB pCcb,
    IN     ULONG OplockType
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_OPLOCK_RECORD pOplock = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pOplock,
                  sizeof(PVFS_OPLOCK_RECORD));
    BAIL_ON_NT_STATUS(ntError);

    pOplock->OplockType = OplockType;
    pOplock->pCcb = pCcb;
    pOplock->pIrpContext = pIrpContext;

    ntError = PvfsStoreOplockRecord(pFcb, pOplock);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}



/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreeOplockRecord(
    PPVFS_OPLOCK_RECORD *ppOplockRec
    )
{
    if (!ppOplockRec || !*ppOplockRec) {
        return;
    }

    PvfsReleaseCCB((*ppOplockRec)->pCcb);

    PVFS_FREE(ppOplockRec);

    return;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
