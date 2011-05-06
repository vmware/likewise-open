/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        scb.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        File Stream Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsFreeSCB(
    PPVFS_SCB pScb
    )
{
    if (pScb)
    {
        LWIO_ASSERT(
            (pScb->BaseControlBlock.pBucket == NULL) &&
            (pScb->BaseControlBlock.RefCount == 0));

        if (pScb->pOwnerFcb)
        {
            PvfsReleaseFCB(&pScb->pOwnerFcb);
        }

        RtlCStringFree(&pScb->pszStreamname);

        pthread_rwlock_destroy(&pScb->rwCcbLock);
        pthread_rwlock_destroy(&pScb->rwBrlLock);

        PvfsListDestroy(&pScb->pPendingLockQueue);
        PvfsListDestroy(&pScb->pOplockPendingOpsQueue);
        PvfsListDestroy(&pScb->pOplockList);
        PvfsListDestroy(&pScb->pCcbList);

        PvfsDestroyCB(&pScb->BaseControlBlock);

        PVFS_FREE(&pScb);

        InterlockedDecrement(&gPvfsDriverState.Counters.Scb);
    }

    return;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsSCBFreeCCB(
    PVOID *ppData
    )
{
    /* This should never be called.  The CCB count has to be 0 when
       we call PvfsFreeSCB() and hence destroy the SCB->pCcbList */

    return;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsAllocateSCB(
    PPVFS_SCB *ppScb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_SCB pScb = NULL;

    *ppScb = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pScb,
                  sizeof(PVFS_SCB),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    PvfsInitializeCB(&pScb->BaseControlBlock);

    /* Initialize mutexes and refcounts */

    pthread_rwlock_init(&pScb->rwCcbLock, NULL);
    pthread_rwlock_init(&pScb->rwBrlLock, NULL);

    /* Setup FcbList */

    PVFS_INIT_LINKS(&pScb->FcbList);

    /* Setup pendlock byte-range lock queue */

    ntError = PvfsListInit(
                  &pScb->pPendingLockQueue,
                  PVFS_SCB_MAX_PENDING_LOCKS,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreePendingLock);
    BAIL_ON_NT_STATUS(ntError);

    /* Oplock pending ops queue */

    ntError = PvfsListInit(
                  &pScb->pOplockPendingOpsQueue,
                  PVFS_SCB_MAX_PENDING_OPERATIONS,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreePendingOp);
    BAIL_ON_NT_STATUS(ntError);

    /* Oplock list and state */

    pScb->bOplockBreakInProgress = FALSE;
    ntError = PvfsListInit(
                  &pScb->pOplockList,
                  0,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreeOplockRecord);
    BAIL_ON_NT_STATUS(ntError);

    /* List of CCBs */

    ntError = PvfsListInit(
                  &pScb->pCcbList,
                  0,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsSCBFreeCCB);
    BAIL_ON_NT_STATUS(ntError);

    /* Miscellaneous */

    PVFS_CLEAR_FILEID(pScb->FileId);

    pScb->AllocationSize = 0;
    pScb->OpenHandleCount = 0;
    pScb->bDeleteOnClose = FALSE;
    pScb->bOplockBreakInProgress = FALSE;
    pScb->pszStreamname = NULL;
    pScb->pOwnerFcb = NULL;

    *ppScb = pScb;

    InterlockedIncrement(&gPvfsDriverState.Counters.Scb);

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    if (pScb)
    {
        PvfsFreeSCB(pScb);
    }

    goto cleanup;
}

/*******************************************************
 ******************************************************/

PPVFS_SCB
PvfsReferenceSCB(
    IN PPVFS_SCB pScb
    )
{
    InterlockedIncrement(&pScb->BaseControlBlock.RefCount);

    return pScb;
}

////////////////////////////////////////////////////////////////////////

VOID
PvfsReleaseSCB(
    PPVFS_SCB *ppScb
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bucketLocked = FALSE;
    BOOLEAN scbLocked = FALSE;
    BOOLEAN scbListLocked = FALSE;
    PPVFS_SCB pScb = NULL;
    PPVFS_CB_TABLE_ENTRY pBucket = NULL;
    LONG refCount = 0;

    LWIO_ASSERT((ppScb != NULL) &&  (*ppScb != NULL));

    pScb = *ppScb;

    // It is important to lock the ScbTable here so that there is no window
    // between the refcount check and the remove. Otherwise another open request
    // could search and locate the SCB in the tree and return free()'d memory.
    // However, if the SCB has no bucket pointer, it has already been removed
    // from the ScbTable so locking is unnecessary.

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(scbListLocked, &pScb->pOwnerFcb->rwScbLock);
    LWIO_LOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

    pBucket = pScb->BaseControlBlock.pBucket;

    if (pBucket)
    {
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(bucketLocked, &pBucket->rwLock);
    }

    refCount = InterlockedDecrement(&pScb->BaseControlBlock.RefCount);
    LWIO_ASSERT(refCount >= 0);

    if (refCount == 0)
    {
        LWIO_ASSERT(PvfsListIsEmpty(pScb->pCcbList));

        PvfsRemoveSCBFromFCB_inlock(pScb->pOwnerFcb, pScb);
        LWIO_UNLOCK_RWMUTEX(scbListLocked, &pScb->pOwnerFcb->rwScbLock);

        if (pBucket)
        {
            PPVFS_FILE_NAME streamName = NULL;
            PSTR fullStreamName = NULL;

            status = PvfsAllocateFileNameFromScb(&streamName, pScb);
            if (STATUS_SUCCESS == status)
            {
                status = PvfsAllocateCStringFromFileName(&fullStreamName, streamName);
                if (STATUS_SUCCESS == status)
                {
                    status = PvfsCbTableRemove_inlock(
                                 (PPVFS_CONTROL_BLOCK)pScb,
                                 fullStreamName);
                    LWIO_ASSERT(status == STATUS_SUCCESS);
                    LWIO_UNLOCK_RWMUTEX(bucketLocked, &pBucket->rwLock);
                }
            }

            if (fullStreamName)
            {
                LwRtlCStringFree(&fullStreamName);
            }
            if (streamName)
            {
                PvfsFreeFileName(streamName);
            }
        }

        LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

        if (STATUS_SUCCESS == status)
        {
            PvfsFreeSCB(pScb);
        }
    }

    LWIO_UNLOCK_RWMUTEX(bucketLocked, &pBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(scbListLocked, &pScb->pOwnerFcb->rwScbLock);
    LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

    *ppScb = NULL;

    return;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsFindOwnerFCB(
    PPVFS_FCB *ppOwnerFcb,
    PCSTR pszFilename
    );

NTSTATUS
PvfsCreateSCB(
    OUT PPVFS_SCB *ppScb,
    IN PPVFS_FILE_NAME FileName,
    IN BOOLEAN bCheckShareAccess,
    IN FILE_SHARE_FLAGS SharedAccess,
    IN ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CB_TABLE scbTable = &gPvfsDriverState.ScbTable;
    PPVFS_SCB pScb = NULL;
    PPVFS_FCB pOwnerFcb = NULL;
    BOOLEAN bBucketLocked = FALSE;
    BOOLEAN fcbListLocked = FALSE;
    PPVFS_CB_TABLE_ENTRY pBucket = NULL;
    PSTR fullFileName = NULL;

    *ppScb = NULL;

    ntError = PvfsAllocateCStringFromFileName(&fullFileName, FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCbTableGetBucket(&pBucket, scbTable, fullFileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFindOwnerFCB(&pOwnerFcb, PvfsGetCStringBaseFileName(FileName));
    BAIL_ON_NT_STATUS(ntError);

    /* Protect against adding a duplicate */

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(fcbListLocked, &pOwnerFcb->rwScbLock);
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBucketLocked, &pBucket->rwLock);

    ntError = PvfsCbTableLookup_inlock(
                  (PPVFS_CONTROL_BLOCK*)&pScb,
                  pBucket,
                  fullFileName);
    if (ntError == STATUS_SUCCESS)
    {
        LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);
        LWIO_UNLOCK_RWMUTEX(fcbListLocked, &pOwnerFcb->rwScbLock);

        LWIO_ASSERT(pScb->pOwnerFcb == pOwnerFcb);

        if (bCheckShareAccess)
        {
            ntError = PvfsEnforceShareMode(
                          pScb,
                          SharedAccess,
                          DesiredAccess);
        }

        /* If we have success, then we are good.  If we have a sharing
           violation, give the caller a chance to break the oplock and
           we'll try again when the create is resumed. */

        if (ntError == STATUS_SUCCESS ||
            ntError == STATUS_SHARING_VIOLATION)
        {
            *ppScb = PvfsReferenceSCB(pScb);
        }

        goto error;
    }

    ntError = PvfsAllocateSCB(&pScb);
    BAIL_ON_NT_STATUS(ntError);

    if (!PvfsIsDefaultStreamName(FileName))
    {
        ntError = RtlCStringDuplicate(&pScb->pszStreamname,
                                      PvfsGetCStringBaseStreamName(FileName));
        BAIL_ON_NT_STATUS(ntError);
    }

    pScb->StreamType = FileName->Type;

    ntError = PvfsAddSCBToFCB_inlock(pOwnerFcb, pScb);
    BAIL_ON_NT_STATUS(ntError);

    pScb->pOwnerFcb = PvfsReferenceFCB(pOwnerFcb);

    /* Add to the file handle table */

    ntError = PvfsCbTableAdd_inlock(
                  pBucket,
                  fullFileName,
                  (PPVFS_CONTROL_BLOCK)pScb);
    BAIL_ON_NT_STATUS(ntError);

    /* Return a reference to the SCB */

    *ppScb = PvfsReferenceSCB(pScb);

error:
    LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(fcbListLocked, &pOwnerFcb->rwScbLock);

    if (pScb)
    {
        PvfsReleaseSCB(&pScb);
    }

    if (pOwnerFcb)
    {
        PvfsReleaseFCB(&pOwnerFcb);
    }

    if (fullFileName)
    {
        LwRtlCStringFree(&fullFileName);

    }

    return ntError;
}

/*******************************************************
 ******************************************************/


static
NTSTATUS
PvfsFindOwnerFCB(
    PPVFS_FCB *ppOwnerFcb,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    PPVFS_FCB pFcb = NULL;
    PPVFS_CB_TABLE_ENTRY pBucket = NULL;

    ntError = PvfsCbTableGetBucket(&pBucket, &gPvfsDriverState.FcbTable, (PVOID)pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCbTableLookup(
                  (PPVFS_CONTROL_BLOCK*)&pFcb,
                  pBucket,
                  pszFilename);
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ntError = PvfsCreateFCB(
                      &pFcb,
                      pszFilename,
                      FALSE,
                      0,
                      0);
    }
    BAIL_ON_NT_STATUS(ntError);

    *ppOwnerFcb = PvfsReferenceFCB(pFcb);

cleanup:
    if (pFcb)
    {
        PvfsReleaseFCB(&pFcb);
    }

    return ntError;

error:

    goto cleanup;
}



/*******************************************************
 ******************************************************/

NTSTATUS
PvfsAddCCBToSCB(
    PPVFS_SCB pScb,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN scbWriteLocked = FALSE;
    BOOLEAN fcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(scbWriteLocked, &pScb->rwCcbLock);
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(fcbWriteLocked, &pScb->pOwnerFcb->rwScbLock);

    ntError = PvfsListAddTail(pScb->pCcbList, &pCcb->ScbList);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->pScb = PvfsReferenceSCB(pScb);

    pScb->OpenHandleCount++;
    pScb->pOwnerFcb->OpenHandleCount++;

    LWIO_ASSERT((pScb->OpenHandleCount > 0) &&
                (pScb->pOwnerFcb->OpenHandleCount > 0));

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(fcbWriteLocked, &pScb->pOwnerFcb->rwScbLock);
    LWIO_UNLOCK_RWMUTEX(scbWriteLocked, &pScb->rwCcbLock);

    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsRemoveCCBFromSCB(
    PPVFS_SCB pScb,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN scbWriteLocked = FALSE;
    BOOLEAN fcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(scbWriteLocked, &pScb->rwCcbLock);
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(fcbWriteLocked, &pScb->pOwnerFcb->rwScbLock);

    ntError = PvfsListRemoveItem(pScb->pCcbList, &pCcb->ScbList);
    BAIL_ON_NT_STATUS(ntError);

    pScb->OpenHandleCount--;
    pScb->pOwnerFcb->OpenHandleCount--;

    LWIO_ASSERT((pScb->OpenHandleCount >= 0) &&
                (pScb->pOwnerFcb->OpenHandleCount >= 0));

    ntError = PvfsCloseHandleCleanup(pScb);
    // Ignore errors

    if (pScb->OpenHandleCount == 0)
    {
        // Clear and reset this from disk
        pScb->AllocationSize = 0;
    }


error:

    LWIO_UNLOCK_RWMUTEX(fcbWriteLocked, &pScb->pOwnerFcb->rwScbLock);
    LWIO_UNLOCK_RWMUTEX(scbWriteLocked, &pScb->rwCcbLock);


    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsPendOplockBreakTest(
    IN PPVFS_SCB pScb,
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_OPLOCK_BREAK_TEST pTestCtx = NULL;

    BAIL_ON_INVALID_PTR(pScb, ntError);
    BAIL_ON_INVALID_PTR(pfnCompletion, ntError);

    ntError = PvfsCreateOplockBreakTestContext(
                  &pTestCtx,
                  pScb,
                  pIrpContext,
                  pCcb,
                  pfnCompletion,
                  pfnFreeContext,
                  pCompletionContext);
    BAIL_ON_NT_STATUS(ntError);

    // Returns STATUS_PENDING on success

    ntError = PvfsAddItemPendingOplockBreakAck(
                  pScb,
                  pIrpContext,
                  (PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK)
                      PvfsOplockPendingBreakIfLocked,
                  (PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX)
                      PvfsFreeOplockBreakTestContext,
                  (PVOID)pTestCtx);
    if ((ntError != STATUS_SUCCESS) &&
        (ntError != STATUS_PENDING))
    {
        BAIL_ON_NT_STATUS(ntError);
    }

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
    IN PPVFS_SCB pScb,
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;
    PPVFS_OPLOCK_PENDING_OPERATION pPendingOp = NULL;

    BAIL_ON_INVALID_PTR(pScb, ntError);
    BAIL_ON_INVALID_PTR(pfnCompletion, ntError);

    LWIO_LOCK_MUTEX(bLocked, &pScb->BaseControlBlock.Mutex);

    if (!pScb->bOplockBreakInProgress)
    {
        LWIO_UNLOCK_MUTEX(bLocked, &pScb->BaseControlBlock.Mutex);

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
                  pScb->pOplockPendingOpsQueue,
                  (PVOID)pPendingOp);
    BAIL_ON_NT_STATUS(ntError);

    pIrpContext->QueueType = PVFS_QUEUE_TYPE_PENDING_OPLOCK_BREAK;

    pIrpContext->pScb = PvfsReferenceSCB(pScb);

    PvfsIrpMarkPending(
        pIrpContext,
        PvfsQueueCancelIrp,
        pIrpContext);

    /* Set the request in a cancellable state */

    PvfsIrpContextClearFlag(pIrpContext, PVFS_IRP_CTX_FLAG_ACTIVE);

    ntError = STATUS_PENDING;

cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &pScb->BaseControlBlock.Mutex);

    return ntError;

error:
    LWIO_UNLOCK_MUTEX(bLocked, &pScb->BaseControlBlock.Mutex);

    if (pPendingOp)
    {
        PvfsFreePendingOp(&pPendingOp);
    }

    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsStreamIsOplocked(
    IN PPVFS_SCB pScb
    )
{
    return !PvfsListIsEmpty(pScb->pOplockList);
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsStreamIsOplockedExclusive(
    IN PPVFS_SCB pScb
    )
{
    BOOLEAN bExclusiveOplock = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;

    if (PvfsListIsEmpty(pScb->pOplockList))
    {
        return FALSE;
    }

    /* We only need to check for the first non-cancelled oplock record
       in the list since the list itself must be consistent and
       non-conflicting */

    while ((pOplockLink = PvfsListTraverse(pScb->pOplockList, pOplockLink)) != NULL)
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
PvfsStreamIsOplockedShared(
    IN PPVFS_SCB pScb
    )
{
    return !PvfsStreamIsOplockedExclusive(pScb);
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsAddOplockRecord(
    IN OUT PPVFS_SCB pScb,
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

    ntError = PvfsListAddTail(pScb->pOplockList, &pOplock->OplockList);
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

////////////////////////////////////////////////////////////////////////

static
VOID
PvfsOplockCleanPendingOpQueue(
    PVOID pContext
    );

NTSTATUS
PvfsScheduleCancelPendingOp(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    BAIL_ON_INVALID_PTR(pIrpContext->pScb, ntError);

    pIrpCtx = PvfsReferenceIrpContext(pIrpContext);

    ntError = LwRtlQueueWorkItem(
                  gPvfsDriverState.ThreadPool,
                  PvfsOplockCleanPendingOpQueue,
                  pIrpCtx,
                  LW_SCHEDULE_HIGH_PRIORITY);
    BAIL_ON_NT_STATUS(ntError);

error:
    if (!NT_SUCCESS(ntError))
    {
        if (pIrpCtx)
        {
            PvfsReleaseIrpContext(&pIrpCtx);
        }
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

static
VOID
PvfsOplockCleanPendingOpQueue(
    PVOID pContext
    )
{
    PPVFS_IRP_CONTEXT pIrpCtx = (PPVFS_IRP_CONTEXT)pContext;
    PPVFS_OPLOCK_PENDING_OPERATION pOperation = NULL;
    PLW_LIST_LINKS pOpLink = NULL;
    BOOLEAN bScbLocked = FALSE;

    LWIO_LOCK_MUTEX(bScbLocked, &pIrpCtx->pScb->BaseControlBlock.Mutex);

    for (pOpLink = PvfsListTraverse(pIrpCtx->pScb->pOplockPendingOpsQueue, NULL);
         pOpLink;
         pOpLink = PvfsListTraverse(pIrpCtx->pScb->pOplockPendingOpsQueue, pOpLink))
    {
        pOperation = LW_STRUCT_FROM_FIELD(
                         pOpLink,
                         PVFS_OPLOCK_PENDING_OPERATION,
                         PendingOpList);

        if (pOperation->pIrpContext != pIrpCtx)
        {
            continue;
        }

        PvfsListRemoveItem(pIrpCtx->pScb->pOplockPendingOpsQueue, pOpLink);
        pOpLink = NULL;

        LWIO_UNLOCK_MUTEX(bScbLocked, &pIrpCtx->pScb->BaseControlBlock.Mutex);

        pOperation->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsCompleteIrpContext(pOperation->pIrpContext);

        PvfsFreePendingOp(&pOperation);

        /* Can only be one IrpContext match so we are done */
    }

    LWIO_UNLOCK_MUTEX(bScbLocked, &pIrpCtx->pScb->BaseControlBlock.Mutex);

    // If we didn't find the IrpContext, it must be in the WorkQueue
    // It will be cleaned up during normal IrpProcessing

    if (pIrpCtx)
    {
        PvfsReleaseIrpContext(&pIrpCtx);
    }

    return;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsRenameSCB(
    IN PPVFS_SCB pScb,
    IN PPVFS_CCB pCcb,
    IN PPVFS_FILE_NAME pNewStreamName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_CB_TABLE scbTable = &gPvfsDriverState.ScbTable;
    PPVFS_SCB pTargetScb = NULL;
    PPVFS_CB_TABLE_ENTRY pTargetBucket = NULL;
    PPVFS_CB_TABLE_ENTRY pCurrentBucket = NULL;
    BOOLEAN currentScbControl = FALSE;
    BOOLEAN targetScbListLocked = FALSE;
    BOOLEAN targetBucketLocked = FALSE;
    BOOLEAN currentBucketLocked = FALSE;
    BOOLEAN fcbListLocked = FALSE;
    BOOLEAN scbRwLocked = FALSE;
    BOOLEAN renameLock = FALSE;
    PVFS_FILE_NAME currentFileName = { 0 };
    PSTR currentStreamName = NULL;
    PSTR newStreamName = NULL;
    PVFS_STAT Stat = {0};
    PPVFS_FCB pOwnerFcb = NULL;

    ntError = PvfsValidatePathSCB(pCcb->pScb, &pCcb->FileId);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(pScb, ntError);

    ntError = PvfsAllocateCStringFromFileName(&newStreamName, pNewStreamName);
    BAIL_ON_NT_STATUS(ntError);

    // if target unnamed stream is non-zero length fail rename
    if (PvfsIsDefaultStreamName(pNewStreamName))
    {
        ntError = PvfsSysStatByFileName(pNewStreamName, &Stat);
        if (STATUS_SUCCESS == ntError && Stat.s_size != 0)
        {
            ntError = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntError);
        }
        ntError = STATUS_SUCCESS;
    }

    ntError = PvfsBuildFileNameFromScb(&currentFileName, pScb);
    BAIL_ON_NT_STATUS(ntError);

    pOwnerFcb = PvfsReferenceFCB(pScb->pOwnerFcb);

    // Obtain all locks for the rename
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(renameLock, &scbTable->rwLock);
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(fcbListLocked, &pOwnerFcb->rwScbLock);
    LWIO_LOCK_MUTEX(currentScbControl, &pScb->BaseControlBlock.Mutex);

    pCurrentBucket = pScb->BaseControlBlock.pBucket;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(currentBucketLocked, &pCurrentBucket->rwLock);

    ntError = PvfsCbTableGetBucket(&pTargetBucket, scbTable, newStreamName);
    BAIL_ON_NT_STATUS(ntError);

    if (pCurrentBucket != pTargetBucket)
    {
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(targetBucketLocked, &pTargetBucket->rwLock);
    }

    // Do the rename work now
    ntError = PvfsCbTableLookup_inlock(
                  (PPVFS_CONTROL_BLOCK*)&pTargetScb,
                  pTargetBucket,
                  newStreamName);
    if (ntError == STATUS_SUCCESS)
    {
        LWIO_ASSERT(pTargetScb->pOwnerFcb == pOwnerFcb);

        if (pTargetScb != pScb)
        {
            // Remove an existing SCB for the target file/stream (if one exists)
            // But make sure it is a different SCB

            LWIO_LOCK_RWMUTEX_SHARED(targetScbListLocked, &pTargetScb->rwCcbLock);

            if (pTargetScb->OpenHandleCount > 0 )
            {
                // if TargetScb has open handles cannot rename
                // This except in the batch-oplock case
                ntError = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(ntError);
            }

            LWIO_UNLOCK_RWMUTEX(targetScbListLocked, &pTargetScb->rwCcbLock);

            PvfsRemoveSCBFromFCB_inlock(pOwnerFcb, pTargetScb);
            // TODO - How to get the Control Mutex without violating the
            // lock heirarchy?  Does it matter at all ?
            ntError = PvfsCbTableRemove_inlock(
                              (PPVFS_CONTROL_BLOCK)pTargetScb,
                              newStreamName);
            LWIO_ASSERT(ntError == STATUS_SUCCESS);
        }
    }

    ntError = PvfsSysRenameByFileName(&currentFileName, pNewStreamName);
    // Ignore the error here and continue

    ntError = PvfsPathCacheRemove(&currentFileName);
    // Ignore the error here and continue

    /* Remove the SCB from the table, update the lookup key, and then re-add.
       Otherwise you will get memory corruption as a freed pointer gets left
       in the Table because if cannot be located using the current (updated)
       filename. Another reason to use the dev/inode pair instead if we could
       solve the "Create New File" issue.  */

    // Remove SCB
    ntError = PvfsAllocateCStringFromFileName(&currentStreamName, &currentFileName);
    if (ntError == STATUS_SUCCESS)
    {
        ntError = PvfsCbTableRemove_inlock((PPVFS_CONTROL_BLOCK)pScb, currentStreamName);
        LWIO_ASSERT(ntError == STATUS_SUCCESS);

        LwRtlCStringFree(&currentStreamName);
    }
    BAIL_ON_NT_STATUS(ntError);

    // Rename SCB
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(scbRwLocked, &pScb->BaseControlBlock.RwLock);
    if (pScb->pszStreamname)
    {
        LwRtlCStringFree(&pScb->pszStreamname);
    }

    if (pNewStreamName->StreamName)
    {
        ntError = LwRtlCStringDuplicate(
                      &pScb->pszStreamname,
                      pNewStreamName->StreamName);
        BAIL_ON_NT_STATUS(ntError);
    }
    LWIO_UNLOCK_RWMUTEX(scbRwLocked, &pScb->BaseControlBlock.RwLock);

    // Re-Add SCB
    ntError = PvfsCbTableAdd_inlock(
                  pTargetBucket,
                  newStreamName,
                  (PPVFS_CONTROL_BLOCK)pScb);
    BAIL_ON_NT_STATUS(ntError);

error:

    // Release all locks ... Whew!
    LWIO_UNLOCK_RWMUTEX(targetScbListLocked, &pTargetScb->rwCcbLock);
    LWIO_UNLOCK_RWMUTEX(targetBucketLocked, &pTargetBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(currentBucketLocked, &pCurrentBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(scbRwLocked, &pScb->BaseControlBlock.RwLock);
    LWIO_UNLOCK_RWMUTEX(fcbListLocked, &pOwnerFcb->rwScbLock);
    LWIO_UNLOCK_MUTEX(currentScbControl, &pScb->BaseControlBlock.Mutex);
    LWIO_UNLOCK_RWMUTEX(renameLock, &scbTable->rwLock);

    if (pOwnerFcb)
    {
        PvfsReleaseFCB(&pOwnerFcb);
    }

    if (pTargetScb)
    {
        PvfsReleaseSCB(&pTargetScb);
    }

    PvfsDestroyFileName(&currentFileName);

    if (currentStreamName)
    {
        LwRtlCStringFree(&currentStreamName);
    }
    if (newStreamName)
    {
        LwRtlCStringFree(&newStreamName);
    }

    return ntError;
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsScbIsPendingDelete(
    PPVFS_SCB pScb
    )
{
    BOOLEAN bPendingDelete = FALSE;
    BOOLEAN bIsLocked = FALSE;

    LWIO_LOCK_MUTEX(bIsLocked, &pScb->BaseControlBlock.Mutex);
    bPendingDelete = pScb->bDeleteOnClose || PvfsFcbIsPendingDelete(pScb->pOwnerFcb);
    LWIO_UNLOCK_MUTEX(bIsLocked, &pScb->BaseControlBlock.Mutex);

    return bPendingDelete;
}

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsScbSetPendingDelete(
    PPVFS_SCB pScb,
    BOOLEAN bPendingDelete
    )
{
    BOOLEAN bIsLocked = FALSE;

    LWIO_LOCK_MUTEX(bIsLocked, &pScb->BaseControlBlock.Mutex);
    pScb->bDeleteOnClose = bPendingDelete;
    if (PvfsIsDefaultStream(pScb))
    {
        PvfsFcbSetPendingDelete(pScb->pOwnerFcb, bPendingDelete);
    }
    LWIO_UNLOCK_MUTEX(bIsLocked, &pScb->BaseControlBlock.Mutex);
}


////////////////////////////////////////////////////////////////////////

BOOLEAN
PvfsIsDefaultStream(
    PPVFS_SCB pScb
    )
{
    BOOLEAN isDefaultStream = FALSE;
    BOOLEAN scbRwLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(scbRwLocked, &pScb->BaseControlBlock.RwLock);
    if ((pScb->StreamType == PVFS_STREAM_TYPE_DATA) &&
        ((pScb->pszStreamname == NULL) || (*pScb->pszStreamname == '\0')))
    {
        isDefaultStream = TRUE;
    }
    LWIO_UNLOCK_RWMUTEX(scbRwLocked, &pScb->BaseControlBlock.RwLock);

    return isDefaultStream;
}

////////////////////////////////////////////////////////////////////////

BOOLEAN
PvfsIsDefaultStreamName(
    PPVFS_FILE_NAME pFileName
    )
{
    BOOLEAN isDefaultStream = FALSE;

    if (pFileName->FileName &&
        (pFileName->StreamName == NULL) &&
        (pFileName->Type == PVFS_STREAM_TYPE_DATA))
    {
        isDefaultStream = TRUE;
    }

    return isDefaultStream;
}

////////////////////////////////////////////////////////////////////////

VOID
PvfsSetScbAllocationSize(
    IN PPVFS_SCB pScb,
    IN LONG64 AllocationSize
    )
{
    BOOLEAN scbLocked = FALSE;

    LWIO_LOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);
    pScb->AllocationSize = AllocationSize;
    LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

    return;
}

////////////////////////////////////////////////////////////////////////

LONG64
PvfsGetScbAllocationSize(
    IN PPVFS_SCB pScb
    )
{
    LONG64 allocationSize = 0;
    BOOLEAN scbLocked = FALSE;

    LWIO_LOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);
    allocationSize = pScb->AllocationSize;
    LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

    return allocationSize;
}
