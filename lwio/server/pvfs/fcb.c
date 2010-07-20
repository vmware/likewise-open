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

static VOID
PvfsFreeFCB(
    PPVFS_FCB pFcb
    )
{
    if (pFcb)
    {
        if (pFcb->pParentFcb)
        {
            PvfsReleaseFCB(&pFcb->pParentFcb);
        }

        RtlCStringFree(&pFcb->pszFilename);

        pthread_mutex_destroy(&pFcb->ControlBlock);
        pthread_rwlock_destroy(&pFcb->rwLock);
        pthread_rwlock_destroy(&pFcb->rwCcbLock);
        pthread_rwlock_destroy(&pFcb->rwBrlLock);

        PvfsListDestroy(&pFcb->pPendingLockQueue);
        PvfsListDestroy(&pFcb->pOplockPendingOpsQueue);
        PvfsListDestroy(&pFcb->pOplockReadyOpsQueue);
        PvfsListDestroy(&pFcb->pOplockList);
        PvfsListDestroy(&pFcb->pCcbList);
        PvfsListDestroy(&pFcb->pNotifyListIrp);
        PvfsListDestroy(&pFcb->pNotifyListBuffer);


        PVFS_FREE(&pFcb);

        InterlockedDecrement(&gPvfsFcbCount);
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

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pFcb,
                  sizeof(PVFS_FCB),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    /* Initialize mutexes and refcounts */

    pthread_mutex_init(&pFcb->ControlBlock, NULL);
    pthread_rwlock_init(&pFcb->rwLock, NULL);
    pthread_rwlock_init(&pFcb->rwCcbLock, NULL);
    pthread_rwlock_init(&pFcb->rwBrlLock, NULL);

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

    PVFS_CLEAR_FILEID(pFcb->FileId);

    pFcb->LastWriteTime = 0;
    pFcb->bDeleteOnClose = FALSE;
    pFcb->bRemoved = FALSE;
    pFcb->bOplockBreakInProgress = FALSE;
    pFcb->pParentFcb = NULL;
    pFcb->pBucket = NULL;
    pFcb->pszFilename = NULL;

    *ppFcb = pFcb;

    InterlockedIncrement(&gPvfsFcbCount);

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    if (pFcb)
    {
        PvfsFreeFCB(pFcb);
    }

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


static
NTSTATUS
PvfsFlushLastWriteTime(
    PPVFS_FCB pFcb,
    LONG64 LastWriteTime
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
                           LastWriteTime,
                           LastAccessTime);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;

}

/***********************************************************************
 Make sure to alkways enter this unfunction with thee pFcb->ControlMutex
 locked
 **********************************************************************/

static
NTSTATUS
PvfsExecuteDeleteOnClose(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    /* Always reset the delete-on-close state to be safe */

    pFcb->bDeleteOnClose = FALSE;

    /* Verify we are deleting the file we think we are */

    ntError = PvfsValidatePath(pFcb, &pFcb->FileId);
    if (ntError == STATUS_SUCCESS)
    {
        ntError = PvfsSysRemove(pFcb->pszFilename);

        /* Reset dev/inode state */

        pFcb->FileId.Device = 0;
        pFcb->FileId.Inode  = 0;

    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    switch (ntError)
    {
    case STATUS_OBJECT_NAME_NOT_FOUND:
        break;

    default:
        LWIO_LOG_ERROR(
            "%s: Failed to execute delete-on-close on %s (%d,%d) (%s)\n",
            PVFS_LOG_HEADER,
            pFcb->pszFilename, pFcb->FileId.Device, pFcb->FileId.Inode,
            LwNtStatusToName(ntError));
        break;
    }

    goto cleanup;
}


VOID
PvfsReleaseFCB(
    PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bBucketLocked = FALSE;
    BOOLEAN bFcbLocked = FALSE;
    BOOLEAN bFcbControlLocked = FALSE;
    PVFS_STAT Stat = {0};
    PPVFS_FCB pFcb = NULL;
    PPVFS_FCB_TABLE_ENTRY pBucket = NULL;

    /* Do housekeeping such as setting the last write time or honoring
       DeletOnClose when the CCB handle count reaches 0.  Not necessarily
       when the RefCount reaches 0.  We may have a non-handle open in the
       FCB table for a path component (see PvfsFindParentFCB()). */

    if (ppFcb == NULL || *ppFcb == NULL)
    {
        goto cleanup;
    }

    pFcb = *ppFcb;

    LWIO_LOCK_RWMUTEX_SHARED(bFcbLocked, &pFcb->rwCcbLock);

    if (!PVFS_IS_DEVICE_HANDLE(pFcb) && (PvfsListLength(pFcb->pCcbList) == 0))
    {
        ntError = PvfsSysStat(pFcb->pszFilename, &Stat);
        if (ntError == STATUS_SUCCESS)
        {
            LONG64 LastWriteTime = PvfsClearLastWriteTimeFCB(pFcb);

            if (LastWriteTime != 0)
            {

                ntError = PvfsFlushLastWriteTime(pFcb, LastWriteTime);

                if (ntError == STATUS_SUCCESS)
                {
                    PvfsNotifyScheduleFullReport(
                        pFcb,
                        FILE_NOTIFY_CHANGE_LAST_WRITE,
                        FILE_ACTION_MODIFIED,
                        pFcb->pszFilename);
                }
            }

            LWIO_LOCK_MUTEX(bFcbControlLocked, &pFcb->ControlBlock);

            if (pFcb->bDeleteOnClose)
            {
                /* Clear the cache entry and remove the file but ignore any errors */

                ntError = PvfsExecuteDeleteOnClose(pFcb);

                /* The locking heirarchy requires that we drop the FCP control
                   block mutex before trying to pick up the FcbTable exclusive
                   lock */

                if (!pFcb->bRemoved)
                {
                    PPVFS_FCB_TABLE_ENTRY pBucket = pFcb->pBucket;

                    pFcb->bRemoved = TRUE;
                    pFcb->pBucket = NULL;

                    LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->ControlBlock);

                    PvfsFcbTableRemove(pBucket, pFcb);
                }
                LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->ControlBlock);

                PvfsPathCacheRemove(pFcb->pszFilename);

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

            LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->ControlBlock);
        }
    }

    LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->rwCcbLock);


    if (InterlockedDecrement(&pFcb->RefCount) == 0)
    {
        /* It is important to lock the FcbTable here so that there is no window
           between the refcount check and the remove. Otherwise another open request
           could search and locate the FCB in the tree and return free()'d memory.
           However, if the FCB has no bucket pointer, it has already been removed
           from the FcbTable so locking is unnecessary.

           We lock inside the InterlockedDecrement() block to avoid grabing an
           exclusive lock on every FCB release. */

        pBucket = pFcb->pBucket;

        if (pBucket)
        {
            LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBucketLocked, &pBucket->rwLock);
        }

        /* Check the refcount again in case someone grab a reference in
           between the InterlockedDecrement() and when we obtained the
           exclusive lock on the bucket */

        if (pFcb->RefCount == 0)
        {
            LWIO_LOCK_MUTEX(bFcbControlLocked, &pFcb->ControlBlock);
            if (!pFcb->bRemoved)
            {
                PPVFS_FCB_TABLE_ENTRY pBucket = pFcb->pBucket;

                pFcb->bRemoved = TRUE;
                pFcb->pBucket = NULL;

                LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->ControlBlock);

                PvfsFcbTableRemove_inlock(pBucket, pFcb);
            }
            LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->ControlBlock);

            if (pBucket)
            {
                LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);
            }

            /* Clear the path cache */

            PvfsPathCacheRemove(pFcb->pszFilename);

            PvfsFreeFCB(pFcb);
        }
        if (pBucket)
        {
            LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);
        }
    }

    *ppFcb = NULL;

cleanup:
    return;
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
    IN PSTR pszFilename,
    IN BOOLEAN bCheckShareAccess,
    IN FILE_SHARE_FLAGS SharedAccess,
    IN ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    BOOLEAN bBucketLocked = FALSE;
    PPVFS_FCB_TABLE_ENTRY pBucket = NULL;
    PPVFS_FCB pParentFcb = NULL;
    BOOLEAN bFcbLocked = FALSE;

    ntError = PvfsFindParentFCB(&pParentFcb, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFcbTableGetBucket(&pBucket, &gFcbTable, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Protect against adding a duplicate */

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBucketLocked, &pBucket->rwLock);

    ntError = PvfsFcbTableLookup_inlock(&pFcb, pBucket, pszFilename);
    if (ntError == STATUS_SUCCESS)
    {
        LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);

        if (bCheckShareAccess)
        {
            ntError = PvfsEnforceShareMode(
                          pFcb,
                          SharedAccess,
                          DesiredAccess);
        }

        /* If we have success, then we are good.  If we have a sharing
           violation, give the caller a chance to break the oplock and
           we'll try again when the create is resumed. */

        if (ntError == STATUS_SUCCESS ||
            ntError == STATUS_SHARING_VIOLATION)
        {
            *ppFcb = PvfsReferenceFCB(pFcb);
        }

        goto cleanup;
    }

    ntError = PvfsAllocateFCB(&pFcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCStringDuplicate(&pFcb->pszFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    pFcb->pParentFcb = pParentFcb ? PvfsReferenceFCB(pParentFcb) : NULL;

    /* Add to the file handle table */

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);
    ntError = PvfsFcbTableAdd_inlock(pBucket, pFcb);
    if (ntError == STATUS_SUCCESS)
    {
        pFcb->pBucket = pBucket;
    }
    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);
    BAIL_ON_NT_STATUS(ntError);

    /* Return a reference to the FCB */

    *ppFcb = PvfsReferenceFCB(pFcb);
    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);

    if (pParentFcb)
    {
        PvfsReleaseFCB(&pParentFcb);
    }

    if (pFcb)
    {
        PvfsReleaseFCB(&pFcb);
    }

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
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    PSTR pszDirname = NULL;
    PPVFS_FCB_TABLE_ENTRY pBucket = NULL;

    if (LwRtlCStringIsEqual(pszFilename, "/", TRUE))
    {
        ntError = STATUS_SUCCESS;
        *ppParentFcb = NULL;

        goto cleanup;
    }

    ntError = PvfsFileDirname(&pszDirname, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFcbTableGetBucket(&pBucket, &gFcbTable, pszDirname);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFcbTableLookup(&pFcb, pBucket, pszDirname);
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ntError = PvfsCreateFCB(
                      &pFcb,
                      pszDirname,
                      FALSE,
                      0,
                      0);
    }
    BAIL_ON_NT_STATUS(ntError);

    *ppParentFcb = PvfsReferenceFCB(pFcb);

cleanup:
    if (pFcb)
    {
        PvfsReleaseFCB(&pFcb);
    }

    if (pszDirname)
    {
        LwRtlCStringFree(&pszDirname);
    }

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

    pCcb->pFcb = PvfsReferenceFCB(pFcb);

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

    BAIL_ON_INVALID_PTR(pFcb, ntError);
    BAIL_ON_INVALID_PTR(pfnCompletion, ntError);

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

    LWIO_LOCK_MUTEX(bLocked, &pFcb->ControlBlock);

    if (!pFcb->bOplockBreakInProgress)
    {
        LWIO_UNLOCK_MUTEX(bLocked, &pFcb->ControlBlock);

        ntError = pfnCompletion(pCompletionContext);
        goto cleanup;
    }

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pPendingOp,
                  sizeof(PVFS_OPLOCK_PENDING_OPERATION),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    PVFS_INIT_LINKS(&pPendingOp->PendingOpList);

    pPendingOp->pIrpContext = PvfsReferenceIrpContext(pIrpContext);
    pPendingOp->pfnCompletion = pfnCompletion;
    pPendingOp->pfnFreeContext = pfnFreeContext;
    pPendingOp->pCompletionContext = pCompletionContext;

    ntError = PvfsListAddTail(
                  pFcb->pOplockPendingOpsQueue,
                  (PVOID)pPendingOp);
    BAIL_ON_NT_STATUS(ntError);

    pIrpContext->QueueType = PVFS_QUEUE_TYPE_PENDING_OPLOCK_BREAK;

    pIrpContext->pFcb = PvfsReferenceFCB(pFcb);

    PvfsIrpMarkPending(
        pIrpContext,
        PvfsQueueCancelIrp,
        pIrpContext);

    /* Set the request in a cancellable state */

    PvfsIrpContextClearFlag(pIrpContext, PVFS_IRP_CTX_FLAG_ACTIVE);

    ntError = STATUS_PENDING;

cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->ControlBlock);

    return ntError;

error:
    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->ControlBlock);

    if (pPendingOp)
    {
        PvfsFreePendingOp(&pPendingOp);
    }

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

        if (PvfsIrpContextCheckFlag(
                pOplock->pIrpContext,
                PVFS_IRP_CTX_FLAG_CANCELLED))
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
                  sizeof(PVFS_OPLOCK_RECORD),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    PVFS_INIT_LINKS(&pOplock->OplockList);

    pOplock->OplockType = OplockType;
    pOplock->pCcb = PvfsReferenceCCB(pCcb);
    pOplock->pIrpContext = PvfsReferenceIrpContext(pIrpContext);

    ntError = PvfsListAddTail(pFcb->pOplockList, &pOplock->OplockList);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    if (pOplock)
    {
        PvfsFreeOplockRecord(&pOplock);
    }

    goto cleanup;
}



/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreeOplockRecord(
    PPVFS_OPLOCK_RECORD *ppOplockRec
    )
{
    PPVFS_OPLOCK_RECORD pOplock = NULL;

    if (ppOplockRec && *ppOplockRec)
    {
        pOplock = *ppOplockRec;

        if (pOplock->pIrpContext)
        {
            PvfsReleaseIrpContext(&pOplock->pIrpContext);
        }

        if (pOplock->pCcb)
        {
            PvfsReleaseCCB(pOplock->pCcb);
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
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    BAIL_ON_INVALID_PTR(pIrpContext->pFcb, ntError);

    pIrpCtx = PvfsReferenceIrpContext(pIrpContext);

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  FALSE,
                  pIrpCtx,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsOplockCleanPendingOpQueue,
                  (PPVFS_WORK_CONTEXT_FREE_CTX)PvfsOplockCleanPendingOpFree);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAddWorkItem(gpPvfsInternalWorkQueue, (PVOID)pWorkCtx);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    if (pIrpCtx)
    {
        PvfsReleaseIrpContext(&pIrpCtx);
    }

    PvfsFreeWorkContext(&pWorkCtx);

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockCleanPendingOpInternal(
    PPVFS_FCB pFcb,
    PPVFS_LIST pQueue,
    PPVFS_IRP_CONTEXT pIrpContext,
    BOOLEAN bCancelIfNotFound
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

    /* We have to check both the "pending" and the "ready" queues.
       Although, it is possible that the "ready" queue processed a
       cancelled IRP before we get to it here. */

    ntError = PvfsOplockCleanPendingOpInternal(
                  pFcb,
                  pFcb->pOplockPendingOpsQueue,
                  pIrpCtx,
                  FALSE);
    if (ntError != STATUS_SUCCESS)
    {
        ntError = PvfsOplockCleanPendingOpInternal(
                      pFcb,
                      pFcb->pOplockReadyOpsQueue,
                      pIrpCtx,
                      TRUE);
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    if (pFcb)
    {
        PvfsReleaseFCB(&pFcb);
    }

    if (pIrpCtx)
    {
        PvfsReleaseIrpContext(&pIrpCtx);
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
    PPVFS_FCB pFcb,
    PPVFS_LIST pQueue,
    PPVFS_IRP_CONTEXT pIrpContext,
    BOOLEAN bCancelIfNotFound
    )
{
    NTSTATUS ntError = STATUS_NOT_FOUND;
    PPVFS_OPLOCK_PENDING_OPERATION pOperation = NULL;
    PLW_LIST_LINKS pOpLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;
    BOOLEAN bFound = FALSE;
    BOOLEAN bFcbLocked = FALSE;

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);

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

        bFound = TRUE;

        PvfsListRemoveItem(pQueue, pOpLink);
        pOpLink = NULL;

        LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);

        pOperation->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsAsyncIrpComplete(pOperation->pIrpContext);

        PvfsFreePendingOp(&pOperation);

        /* Can only be one IrpContext match so we are done */
        ntError = STATUS_SUCCESS;
    }
    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);

    if (!bFound && bCancelIfNotFound)
    {
        pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsAsyncIrpComplete(pIrpContext);
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
    PSTR pszNewFilename
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FCB pNewParentFcb = NULL;
    PPVFS_FCB pOldParentFcb = NULL;
    PPVFS_FCB pTargetFcb = NULL;
    PPVFS_FCB_TABLE_ENTRY pTargetBucket = NULL;
    PPVFS_FCB_TABLE_ENTRY pCurrentBucket = NULL;
    BOOLEAN bCurrentFcbControl = FALSE;
    BOOLEAN bTargetFcbControl = FALSE;
    BOOLEAN bTargetBucketLocked = FALSE;
    BOOLEAN bCurrentBucketLocked = FALSE;
    BOOLEAN bFcbRwLocked = FALSE;
    BOOLEAN bCcbLocked = FALSE;

    ntError = PvfsValidatePath(pCcb->pFcb, &pCcb->FileId);
    BAIL_ON_NT_STATUS(ntError);

    /* If the target has an existing FCB, remove it from the Table and let
       the existing ref counters play out (e.g. pending change notifies. */

    ntError = PvfsFindParentFCB(&pNewParentFcb, pszNewFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFcbTableGetBucket(&pTargetBucket, &gFcbTable, pszNewFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Locks - pTargetBucket(Excl) */
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bTargetBucketLocked, &pTargetBucket->rwLock);

    ntError = PvfsFcbTableLookup_inlock(
                  &pTargetFcb,
                  pTargetBucket,
                  pszNewFilename);
    if (ntError == STATUS_SUCCESS)
    {
        /* Make sure we have a different FCB */

        if (pTargetFcb != pFcb)
        {
            LWIO_LOCK_MUTEX(bTargetFcbControl, &pTargetFcb->ControlBlock);
            if (!pTargetFcb->bRemoved)
            {
                pTargetFcb->bRemoved = TRUE;
                pTargetFcb->pBucket = NULL;

                LWIO_UNLOCK_MUTEX(bTargetFcbControl, &pTargetFcb->ControlBlock);

                PvfsFcbTableRemove_inlock(pTargetBucket, pTargetFcb);
            }
            LWIO_UNLOCK_MUTEX(bTargetFcbControl, &pTargetFcb->ControlBlock);
        }

        PvfsReleaseFCB(&pTargetFcb);
    }

    /* Locks - pTargetBucket(Excl), pFcb->rwLock(Excl) */
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbRwLocked, &pFcb->rwLock);

    ntError = PvfsSysRename(pCcb->pFcb->pszFilename, pszNewFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Clear the cache entry but ignore any errors */

    ntError = PvfsPathCacheRemove(pFcb->pszFilename);

    /* Remove the FCB from the table, update the lookup key, and then re-add.
       Otherwise you will get memory corruption as a freed pointer gets left
       in the Table because if cannot be located using the current (updated)
       filename. Another reason to use the dev/inode pair instead if we could
       solve the "Create New File" issue. */

    pCurrentBucket = pFcb->pBucket;

    LWIO_LOCK_MUTEX(bCurrentFcbControl, &pFcb->ControlBlock);
    pFcb->bRemoved = TRUE;
    pFcb->pBucket = NULL;
    LWIO_UNLOCK_MUTEX(bCurrentFcbControl, &pFcb->ControlBlock);

    /* Locks - pTargetBucket(Excl), pFcb->rwLock(Excl), pCurrentBucket(Excl) */
    /* WARNING - Potential Deadly Embrace!!! */

    if (pCurrentBucket != pTargetBucket)
    {
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(bCurrentBucketLocked, &pCurrentBucket->rwLock);
    }

    ntError = PvfsFcbTableRemove_inlock(pCurrentBucket, pFcb);
    LWIO_UNLOCK_RWMUTEX(bCurrentBucketLocked, &pCurrentBucket->rwLock);
    BAIL_ON_NT_STATUS(ntError);

    PVFS_FREE(&pFcb->pszFilename);

    ntError = LwRtlCStringDuplicate(&pFcb->pszFilename, pszNewFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Have to update the parent links as well */

    if (pNewParentFcb != pFcb->pParentFcb)
    {
        pOldParentFcb = pFcb->pParentFcb;
        pFcb->pParentFcb = pNewParentFcb;
        pNewParentFcb = NULL;
    }

    /* Locks - pTargetBucket(Excl), pFcb->rwLock(Excl), pFcb->ControlBlock */
    LWIO_LOCK_MUTEX(bCurrentFcbControl, &pFcb->ControlBlock);
    ntError = PvfsFcbTableAdd_inlock(pTargetBucket, pFcb);
    if (ntError == STATUS_SUCCESS)
    {
        pFcb->bRemoved = FALSE;
        pFcb->pBucket = pTargetBucket;
    }
    LWIO_UNLOCK_MUTEX(bCurrentFcbControl, &pFcb->ControlBlock);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_UNLOCK_RWMUTEX(bFcbRwLocked, &pFcb->rwLock);
    LWIO_UNLOCK_RWMUTEX(bTargetBucketLocked, &pTargetBucket->rwLock);


    LWIO_LOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

        PVFS_FREE(&pCcb->pszFilename);
        ntError = LwRtlCStringDuplicate(&pCcb->pszFilename, pszNewFilename);
        BAIL_ON_NT_STATUS(ntError);

    LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

cleanup:
    LWIO_UNLOCK_RWMUTEX(bTargetBucketLocked, &pTargetBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(bCurrentBucketLocked, &pCurrentBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(bFcbRwLocked, &pFcb->rwLock);
    LWIO_UNLOCK_MUTEX(bCurrentFcbControl, &pFcb->ControlBlock);
    LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

    if (pNewParentFcb)
    {
        PvfsReleaseFCB(&pNewParentFcb);
    }

    if (pOldParentFcb)
    {
        PvfsReleaseFCB(&pOldParentFcb);
    }

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFcbIsPendingDelete(
    PPVFS_FCB pFcb
    )
{
    BOOLEAN bPendingDelete = FALSE;
    BOOLEAN bIsLocked = FALSE;

    LWIO_LOCK_MUTEX(bIsLocked, &pFcb->ControlBlock);
    bPendingDelete = pFcb->bDeleteOnClose;
    LWIO_UNLOCK_MUTEX(bIsLocked, &pFcb->ControlBlock);

    return bPendingDelete;
}

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFcbSetPendingDelete(
    PPVFS_FCB pFcb,
    BOOLEAN bPendingDelete
    )
{
    BOOLEAN bIsLocked = FALSE;

    LWIO_LOCK_MUTEX(bIsLocked, &pFcb->ControlBlock);
    pFcb->bDeleteOnClose = bPendingDelete;
    LWIO_UNLOCK_MUTEX(bIsLocked, &pFcb->ControlBlock);
}

/*****************************************************************************
 ****************************************************************************/

PPVFS_FCB
PvfsGetParentFCB(
    PPVFS_FCB pFcb
    )
{
    PPVFS_FCB pParent = NULL;
    BOOLEAN bLocked = FALSE;

    if (pFcb)
    {
        LWIO_LOCK_RWMUTEX_SHARED(bLocked, &pFcb->rwLock);
        if (pFcb->pParentFcb)
        {
            pParent = PvfsReferenceFCB(pFcb->pParentFcb);
        }
        LWIO_UNLOCK_RWMUTEX(bLocked, &pFcb->rwLock);
    }

    return pParent;
}

/*****************************************************************************
 ****************************************************************************/

LONG64
PvfsClearLastWriteTimeFCB(
    PPVFS_FCB pFcb
    )
{
    BOOLEAN bLocked = FALSE;
    LONG64 LastWriteTime = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pFcb->rwLock);
    LastWriteTime = pFcb->LastWriteTime;
    pFcb->LastWriteTime = 0;
    LWIO_UNLOCK_RWMUTEX(bLocked, &pFcb->rwLock);

    return LastWriteTime;
}

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsSetLastWriteTimeFCB(
    PPVFS_FCB pFcb,
    LONG64 LastWriteTime
    )
{
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pFcb->rwLock);
    pFcb->LastWriteTime = LastWriteTime;
    LWIO_UNLOCK_RWMUTEX(bLocked, &pFcb->rwLock);
}




/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
