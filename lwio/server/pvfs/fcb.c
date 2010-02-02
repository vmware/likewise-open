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

/*****************************************************************************
 ****************************************************************************/

static
int
FcbTableFilenameCompare(
    PVOID a,
    PVOID b
    );

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

/*****************************************************************************
 ****************************************************************************/

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


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsFreeFCB(
    PPVFS_FCB pFcb
    )
{
    if (pFcb)
    {
        if (pFcb->pParentFcb)
        {
            PvfsReleaseFCB(pFcb->pParentFcb);
        }

        RtlCStringFree(&pFcb->pszFilename);

        pthread_mutex_destroy(&pFcb->ControlBlock);
        pthread_mutex_destroy(&pFcb->mutexOplock);
        pthread_mutex_destroy(&pFcb->mutexNotify);
        pthread_rwlock_destroy(&pFcb->rwCcbLock);
        pthread_rwlock_destroy(&pFcb->rwBrlLock);
        pthread_rwlock_destroy(&pFcb->rwFileName);

        PvfsListDestroy(&pFcb->pPendingLockQueue);
        PvfsListDestroy(&pFcb->pOplockPendingOpsQueue);
        PvfsListDestroy(&pFcb->pOplockReadyOpsQueue);
        PvfsListDestroy(&pFcb->pOplockList);
        PvfsListDestroy(&pFcb->pCcbList);
        PvfsListDestroy(&pFcb->pNotifyListIrp);
        PvfsListDestroy(&pFcb->pNotifyListBuffer);


        PVFS_FREE(&pFcb);
    }

    return;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsFCBFreeCCB(
PVOID *ppData
    )
{
    /* This should never be called.  The CCB count has to be 0 when
       we call PvfsFreeFCB() and hence destroy the FCB->pCcbList */

    return;
}


/*****************************************************************************
 ****************************************************************************/

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

    /* Initialize mutexes and refcounts */

    pthread_mutex_init(&pFcb->ControlBlock, NULL);
    pthread_mutex_init(&pFcb->mutexOplock, NULL);
    pthread_mutex_init(&pFcb->mutexNotify, NULL);
    pthread_rwlock_init(&pFcb->rwCcbLock, NULL);
    pthread_rwlock_init(&pFcb->rwBrlLock, NULL);
    pthread_rwlock_init(&pFcb->rwFileName, NULL);

    pFcb->RefCount = 1;

    /* Setup pendlock byte-range lock queue */

    ntError = PvfsListInit(
                  &pFcb->pPendingLockQueue,
                  PVFS_FCB_MAX_PENDING_LOCKS,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreePendingLock);
    BAIL_ON_NT_STATUS(ntError);

    /* Oplock pending ops queue */

    ntError = PvfsListInit(
                  &pFcb->pOplockPendingOpsQueue,
                  PVFS_FCB_MAX_PENDING_OPERATIONS,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreePendingOp);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsListInit(
                  &pFcb->pOplockReadyOpsQueue,
                  PVFS_FCB_MAX_PENDING_OPERATIONS,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreePendingOp);
    BAIL_ON_NT_STATUS(ntError);

    /* Oplock list and state */

    pFcb->bOplockBreakInProgress = FALSE;
    ntError = PvfsListInit(
                  &pFcb->pOplockList,
                  0,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreeOplockRecord);
    BAIL_ON_NT_STATUS(ntError);

    /* List of CCBs */

    ntError = PvfsListInit(
                  &pFcb->pCcbList,
                  0,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFCBFreeCCB);
    BAIL_ON_NT_STATUS(ntError);

    /* List of Notify requests */

    ntError = PvfsListInit(
                  &pFcb->pNotifyListIrp,
                  PVFS_FCB_MAX_PENDING_NOTIFY,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreeNotifyRecord);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsListInit(
                  &pFcb->pNotifyListBuffer,
                  PVFS_FCB_MAX_PENDING_NOTIFY,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreeNotifyRecord);
    BAIL_ON_NT_STATUS(ntError);

    /* Miscellaneous */

    pFcb->bDeleteOnClose = FALSE;
    pFcb->pParentFcb = NULL;

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

PPVFS_FCB
PvfsReferenceFCB(
    IN PPVFS_FCB pFcb
    )
{
    InterlockedIncrement(&pFcb->RefCount);

    return pFcb;
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

    ntError = PvfsValidatePath(pFcb, &pFcb->FileId);
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
    BOOLEAN bFcbLocked = FALSE;
    PVFS_STAT Stat = {0};

    /* Do housekeeping such as setting the last write time or
       honoring DeletOnClose when the CCB handle count reaches
       0.  Not necessaril.y when the RefCount reaches 0.  We
       may have a non-handle open in the FCB table for a
       path component (see PvfsFindParentFCB()). */

    LWIO_LOCK_RWMUTEX_SHARED(bFcbLocked, &pFcb->rwCcbLock);

    if (PvfsListLength(pFcb->pCcbList) == 0)
    {
        ntError = PvfsSysStat(pFcb->pszFilename, &Stat);
        if (ntError == STATUS_SUCCESS)
        {
            if (pFcb->LastWriteTime != 0)
            {

                ntError = PvfsSetLastWriteTime(pFcb);
                pFcb->LastWriteTime = 0;

                if (ntError == STATUS_SUCCESS)
                {
                    PvfsNotifyScheduleFullReport(
                        pFcb,
                        FILE_NOTIFY_CHANGE_LAST_WRITE,
                        FILE_ACTION_MODIFIED,
                        pFcb->pszFilename);
                }
            }

            if (pFcb->bDeleteOnClose)
            {
                ntError = PvfsExecuteDeleteOnClose(pFcb);
                pFcb->bDeleteOnClose = FALSE;

                if (ntError == STATUS_SUCCESS)
                {
                    PvfsNotifyScheduleFullReport(
                        pFcb,
                        S_ISDIR(Stat.s_mode) ?
                            FILE_NOTIFY_CHANGE_DIR_NAME :
                            FILE_NOTIFY_CHANGE_FILE_NAME,
                        FILE_ACTION_REMOVED,
                        pFcb->pszFilename);
                }
            }
        }
    }

    LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->rwCcbLock);


    /* It is important to lock the FcbTable here so that
       there is no window between the decrement and the remove.
       Otherwise another open request could search and locate the
       FCB in the tree and return free()'d memory. */

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &gFcbTable.rwLock);

    if (InterlockedDecrement(&pFcb->RefCount) == 0)
    {
        PvfsRemoveFCB(pFcb);

        LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

        PvfsFreeFCB(pFcb);
    }

    LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

    return;
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

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsFindParentFCB(
    PPVFS_FCB *ppParentFcb,
    PCSTR pszFilename
    );

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

    /* Lookup the parent FCB */

    ntError = PvfsFindParentFCB(&pFcb->pParentFcb, pszFilename);
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


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsFindParentFCB(
    PPVFS_FCB *ppParentFcb,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    PSTR pszDirname = NULL;

    if (LwRtlCStringIsEqual(pszFilename, "/", TRUE))
    {
        ntError = STATUS_SUCCESS;
        *ppParentFcb = NULL;

        goto cleanup;
    }

    ntError = PvfsFileDirname(&pszDirname, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = _PvfsFindFCB(&pFcb, pszDirname);
    if (ntError != STATUS_SUCCESS)
    {
        ntError = PvfsAllocateFCB(&pFcb);
        BAIL_ON_NT_STATUS(ntError);

        pFcb->pszFilename = pszDirname;
        pszDirname = NULL;

        /* Lookup the parent FCB */

        ntError = PvfsFindParentFCB(&pFcb->pParentFcb, pFcb->pszFilename);
        BAIL_ON_NT_STATUS(ntError);

        /* Add to the file handle table */

        ntError = PvfsAddFCB(pFcb);
        BAIL_ON_NT_STATUS(ntError);
    }

    *ppParentFcb = pFcb;
    pFcb = NULL;

    ntError = STATUS_SUCCESS;

cleanup:
    if (pFcb)
    {
        PvfsReleaseFCB(pFcb);
    }

    LwRtlCStringFree(&pszDirname);

    return ntError;

error:
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
    BOOLEAN bFcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwCcbLock);

    ntError = PvfsListAddTail(pFcb->pCcbList, &pCcb->FcbList);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->pFcb = pFcb;

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwCcbLock);

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
    BOOLEAN bFcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwCcbLock);

    ntError = PvfsListRemoveItem(pFcb->pCcbList, &pCcb->FcbList);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwCcbLock);

    return ntError;

error:
    goto cleanup;
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
                  (PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK)
                      PvfsOplockPendingBreakIfLocked,
                  (PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX)
                      PvfsFreeOplockBreakTestContext,
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

    ntError = PvfsListAddTail(
                  pFcb->pOplockPendingOpsQueue,
                  (PVOID)pPendingOp);
    BAIL_ON_NT_STATUS(ntError);
    pIrpContext->QueueType = PVFS_QUEUE_TYPE_PENDING_OPLOCK_BREAK;

    pIrpContext->pFcb = PvfsReferenceFCB(pFcb);

    /* An item could be requeued here */

    if (!pIrpContext->bIsPended)
    {
        PvfsIrpMarkPending(
            pIrpContext,
            PvfsQueueCancelIrp,
            pIrpContext);
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
    PLW_LIST_LINKS pCursor = NULL;
    PPVFS_CCB pCurrentCcb = NULL;
    BOOLEAN bNonSelfOpen = FALSE;
    BOOLEAN bFcbReadLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pFcb->rwCcbLock);

    while((pCursor = PvfsListTraverse(pFcb->pCcbList, pCursor)) != NULL)
    {
        pCurrentCcb = LW_STRUCT_FROM_FIELD(
                          pCursor,
                          PVFS_CCB,
                          FcbList);

        if (pCcb != pCurrentCcb)
        {
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
    return !PvfsListIsEmpty(pFcb->pOplockList);
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFileIsOplockedExclusive(
    IN PPVFS_FCB pFcb
    )
{
    BOOLEAN bExclusiveOplock = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;

    if (PvfsListIsEmpty(pFcb->pOplockList))
    {
        return FALSE;
    }

    /* We only need to check for the first non-cancelled oplock record
       in the list since the list itself must be consistent and
       non-conflicting */

    while ((pOplockLink = PvfsListTraverse(pFcb->pOplockList, pOplockLink)) != NULL)
    {
        pOplock = LW_STRUCT_FROM_FIELD(
                      pOplockLink,
                      PVFS_OPLOCK_RECORD,
                      OplockList);

        if (pOplock->pIrpContext->bIsCancelled)
        {
            pOplock = NULL;
            continue;
        }

        /* Found first non-cancelled oplock */
        break;
    }

    if (pOplock &&
        ((pOplock->OplockType == IO_OPLOCK_REQUEST_OPLOCK_BATCH) ||
         (pOplock->OplockType == IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1)))
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
    return !PvfsFileIsOplockedExclusive(pFcb);
}

/*****************************************************************************
 ****************************************************************************/

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
    pOplock->pCcb = PvfsReferenceCCB(pCcb);
    pOplock->pIrpContext = pIrpContext;

    ntError = PvfsListAddTail(pFcb->pOplockList, &pOplock->OplockList);
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
    PPVFS_CCB pCcb = NULL;
    PPVFS_IRP_CONTEXT pIrpContext = NULL;

    if (ppOplockRec && *ppOplockRec)
    {
        pCcb = (*ppOplockRec)->pCcb;
        pIrpContext = (*ppOplockRec)->pIrpContext;

        if (pIrpContext)
        {
            pIrpContext->pIrp->IoStatusBlock.Status = STATUS_FILE_CLOSED;

            PvfsAsyncIrpComplete(pIrpContext);
            PvfsFreeIrpContext(&pIrpContext);
        }


        if (pCcb)
        {
            PvfsReleaseCCB(pCcb);
        }

        PVFS_FREE(ppOplockRec);
    }

    return;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockCleanPendingOpQueue(
    PVOID pContext
    );

static
VOID
PvfsOplockCleanPendingOpFree(
    PVOID *ppContext
    );

NTSTATUS
PvfsScheduleCancelPendingOp(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    BAIL_ON_INVALID_PTR(pIrpContext->pFcb, ntError);

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  FALSE,
                  pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsOplockCleanPendingOpQueue,
                  (PPVFS_WORK_CONTEXT_FREE_CTX)PvfsOplockCleanPendingOpFree);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAddWorkItem(gpPvfsInternalWorkQueue, (PVOID)pWorkCtx);
    BAIL_ON_NT_STATUS(ntError);

    pWorkCtx = NULL;

cleanup:
    PVFS_FREE(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockCleanPendingOpInternal(
    PPVFS_LIST pQueue,
    PPVFS_IRP_CONTEXT pIrpContext
    );

static
NTSTATUS
PvfsOplockCleanPendingOpQueue(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_IRP_CONTEXT pIrpCtx = (PPVFS_IRP_CONTEXT)pContext;
    PPVFS_FCB pFcb = PvfsReferenceFCB(pIrpCtx->pFcb);
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_MUTEX(bLocked, &pFcb->mutexOplock);

    /* We have to check both the "pending" and the "ready" queues.
       Although, it is possible that the "ready" queue processed a
       cancelled IRP before we get to it here. */

    ntError = PvfsOplockCleanPendingOpInternal(
                  pFcb->pOplockPendingOpsQueue,
                  pIrpCtx);
    if (ntError != STATUS_SUCCESS)
    {
        ntError = PvfsOplockCleanPendingOpInternal(
                      pFcb->pOplockReadyOpsQueue,
                      pIrpCtx);
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->mutexOplock);

    if (pFcb)
    {
        PvfsReleaseFCB(pFcb);
    }

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockCleanPendingOpInternal(
    PPVFS_LIST pQueue,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_NOT_FOUND;
    PPVFS_OPLOCK_PENDING_OPERATION pOperation = NULL;
    PLW_LIST_LINKS pOpLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;

    pOpLink = PvfsListTraverse(pQueue, NULL);

    while (pOpLink)
    {
        pOperation = LW_STRUCT_FROM_FIELD(
                         pOpLink,
                         PVFS_OPLOCK_PENDING_OPERATION,
                         PendingOpList);

        pNextLink = PvfsListTraverse(pQueue, pOpLink);

        if (pOperation->pIrpContext != pIrpContext)
        {
            pOpLink = pNextLink;
            continue;
        }

        PvfsListRemoveItem(pQueue, pOpLink);
        pOpLink = NULL;

        pOperation->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsAsyncIrpComplete(pOperation->pIrpContext);
        PvfsFreeIrpContext(&pOperation->pIrpContext);

        PvfsFreePendingOp(&pOperation);

        /* Can only be one IrpContext match so we are done */
        ntError = STATUS_SUCCESS;
    }

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static
 VOID
PvfsOplockCleanPendingOpFree(
    PVOID *ppContext
    )
{
    /* No op -- context released in PvfsOplockCleanPendingOpQueue */
    return;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsRenameFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb,
    PCSTR pszNewFilename
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bTableLocked = FALSE;
    BOOLEAN bFcbLocked = FALSE;
    BOOLEAN bCcbLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bTableLocked, &gFcbTable.rwLock);

        ntError = PvfsValidatePath(pCcb->pFcb, &pCcb->FileId);
        BAIL_ON_NT_STATUS(ntError);

        LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbLocked, &pFcb->rwFileName);

            ntError = PvfsSysRename(pCcb->pszFilename, pszNewFilename);
            BAIL_ON_NT_STATUS(ntError);

            /* Remove the FCB from the table, update the lookup key,
               and then re-add.  Otherwise you will get memory corruption
               as a freed pointer gets left in the Table because if
               cannot be located using the current (updated) filename.
               Another reason to use the dev/inode pair instead if
               we could solve the "Create New File" issue.  */

            ntError = PvfsRemoveFCB(pFcb);
            BAIL_ON_NT_STATUS(ntError);

            PVFS_FREE(&pFcb->pszFilename);
            ntError = LwRtlCStringDuplicate(&pFcb->pszFilename, pszNewFilename);
            BAIL_ON_NT_STATUS(ntError);

            ntError = PvfsAddFCB(pFcb);
            BAIL_ON_NT_STATUS(ntError);

        LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->rwFileName);

    LWIO_UNLOCK_RWMUTEX(bTableLocked, &gFcbTable.rwLock);

    LWIO_LOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

        PVFS_FREE(&pCcb->pszFilename);
        ntError = LwRtlCStringDuplicate(&pCcb->pszFilename, pszNewFilename);
        BAIL_ON_NT_STATUS(ntError);

    LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

cleanup:

    return ntError;

error:
    LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);
    LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->rwFileName);
    LWIO_UNLOCK_RWMUTEX(bTableLocked, &gFcbTable.rwLock);

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
