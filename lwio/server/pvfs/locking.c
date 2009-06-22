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
 *        locking.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Internal Lock Manager
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
CanLock(
    PPVFS_LOCK_TABLE pLockTable,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive,
    BOOLEAN bSelf
    );

static NTSTATUS
AddLock(
    PPVFS_CCB pCcb,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    );

static NTSTATUS
StoreLock(
    PPVFS_LOCK_TABLE pLockTable,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    );

static VOID
InitLockEntry(
    PPVFS_LOCK_ENTRY pEntry,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    PVFS_LOCK_FLAGS Flags
    );

static BOOLEAN
LockEntryEqual(
    PPVFS_LOCK_ENTRY pEntry1,
    PPVFS_LOCK_ENTRY pEntry2
    );

static NTSTATUS
PvfsAddPendingLock(
    PPVFS_FCB pFcb,
    PPVFS_IRP_CONTEXT pIrpCtx,
    PPVFS_CCB pCcb,
    PPVFS_LOCK_ENTRY pLock
    );

static VOID
PvfsProcessPendingLocks(
    PPVFS_FCB pFcb
    );

/* Code */

/**************************************************************
 *************************************************************/

NTSTATUS
PvfsLockFile(
    PPVFS_IRP_CONTEXT pIrpCtx,
    PPVFS_CCB pCcb,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    PVFS_LOCK_FLAGS Flags
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB_LIST_NODE pCursor = NULL;
    PPVFS_FCB pFcb = pCcb->pFcb;
    BOOLEAN bExclusive = FALSE;
    BOOLEAN bFcbReadLocked = FALSE;
    BOOLEAN bBrlWriteLocked = FALSE;
    BOOLEAN bLastFailedLock = FALSE;
    PVFS_LOCK_ENTRY RangeLock = {0};

    /* Sanity check -- WIll probably have to go back and determine
       the semantics of 0 byte locks */

    if (Length == 0) {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (Flags & PVFS_LOCK_EXCLUSIVE) {
        bExclusive = TRUE;
    }

    /* Read lock so no one can add a CCB to the list */

    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pFcb->rwLock);
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLocked, &pFcb->rwBrlLock);

    for (pCursor = PvfsNextCCBFromList(pFcb, pCursor);
         pCursor;
         pCursor = PvfsNextCCBFromList(pFcb, pCursor))
    {
        /* We'll deal with ourselves in AddLock() */

        if (pCcb == pCursor->pCcb) {
            continue;
        }

        ntError = CanLock(&pCursor->pCcb->LockTable,
                          Key, Offset, Length,
                          bExclusive, FALSE);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = AddLock(pCcb, Key, Offset, Length, bExclusive);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBrlWriteLocked, &pFcb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bFcbReadLocked, &pFcb->rwLock);

    return ntError;

error:
    LWIO_LOCK_MUTEX(bLastFailedLock, &pFcb->ControlBlock);

    InitLockEntry(&RangeLock, Key, Offset, Length, Flags);

    /* Only try to pend a blocking lock that failed due
       to a conflict.  Not for general failures */

    if ((Flags & PVFS_LOCK_BLOCK) && (ntError == STATUS_LOCK_NOT_GRANTED))
    {
        NTSTATUS ntErrorPending = STATUS_UNSUCCESSFUL;

        ntErrorPending = PvfsAddPendingLock(pFcb, pIrpCtx, pCcb, &RangeLock);
        if (ntErrorPending == STATUS_SUCCESS) {
            ntError = STATUS_PENDING;
        }
    }

    /* Windows 2003 & XP return FILE_LOCK_CONFLICT for all
       lock failures following the first.  The state machine
       resets every time a new lock range (on a new handle) is
       requested.  Pass all errors other than LOCK_NOT_GRANTED
       on through. */

    if (ntError == STATUS_LOCK_NOT_GRANTED)
    {
        if (LockEntryEqual(&pFcb->LastFailedLock, &RangeLock) &&
           (pFcb->pLastFailedLockOwner == pCcb))
        {
            ntError = STATUS_FILE_LOCK_CONFLICT;
        }
        InitLockEntry(&pFcb->LastFailedLock, Key, Offset, Length, Flags);
        pFcb->pLastFailedLockOwner = pCcb;
    }

    LWIO_UNLOCK_MUTEX(bLastFailedLock, &pFcb->ControlBlock);

    goto cleanup;
}

/* Caller must hold the mutex on the FCB */

static NTSTATUS
PvfsAddPendingLock(
    PPVFS_FCB pFcb,
    PPVFS_IRP_CONTEXT pIrpCtx,
    PPVFS_CCB pCcb,
    PPVFS_LOCK_ENTRY pLock
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_LOCK pPendingLock = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pPendingLock,
                                 sizeof(PVFS_PENDING_LOCK));
    BAIL_ON_NT_STATUS(ntError);

    pPendingLock->pIrpContext = pIrpCtx;
    pIrpCtx->pPendingLock = pPendingLock;  /* back link */

    pPendingLock->pCcb = pCcb;
    pPendingLock->PendingLock = *pLock;   /* structure assignment */

    pPendingLock->bIsCancelled = FALSE;

    ntError = LwRtlQueueAddItem(pFcb->pPendingLockQueue,
                                (PVOID)pPendingLock);
    BAIL_ON_NT_STATUS(ntError);

    /* Memory has been given to the Queue */

    pPendingLock = NULL;

cleanup:
    PVFS_FREE(&pPendingLock);

    return ntError;

error:
    pIrpCtx->pPendingLock = NULL;

    goto cleanup;
}

/**************************************************************
 *************************************************************/

NTSTATUS
PvfsUnlockFile(
    PPVFS_CCB pCcb,
    BOOLEAN bUnlockAll,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length
    )
{
    NTSTATUS ntError = STATUS_RANGE_NOT_LOCKED;
    PPVFS_FCB pFcb = pCcb->pFcb;
    PPVFS_LOCK_LIST pExclLocks = &pCcb->LockTable.ExclusiveLocks;
    PPVFS_LOCK_LIST pSharedLocks = &pCcb->LockTable.SharedLocks;
    PPVFS_LOCK_ENTRY pEntry = NULL;
    ULONG i = 0;
    BOOLEAN bBrlWriteLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLock, &pFcb->rwBrlLock);

    /* If the caller wants to release all locks, just set the
       NumberOfLocks to 0 */

    if (bUnlockAll && (Key == 0)) {
        pExclLocks->NumberOfLocks = 0;
        pSharedLocks->NumberOfLocks = 0;

        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    /* Exclusive Locks */

    for (i=0; i<pExclLocks->NumberOfLocks; i++)
    {
        pEntry = &pExclLocks->pLocks[i];

        /* Two success cases:
           1. bUnlockAll is set and the Key ID matches
           2. Match a single lock
        */

        if (bUnlockAll)
        {
            if (Key == pEntry->Key) {
                if (((pExclLocks->NumberOfLocks-i) - 1) > 0) {
                    RtlMoveMemory(pEntry, pEntry+1,
                                  sizeof(PVFS_LOCK_ENTRY)* ((pExclLocks->NumberOfLocks-i)-1));
                }
                pExclLocks->NumberOfLocks--;

                ntError = STATUS_SUCCESS;
            }

            continue;
        }
        else if ((Offset == pEntry->Offset) &&
                 (Length == pEntry->Length) &&
                 (Key == pEntry->Key))
        {
            if (((pExclLocks->NumberOfLocks-i) - 1) > 0) {
                RtlMoveMemory(pEntry, pEntry+1,
                              sizeof(PVFS_LOCK_ENTRY)* ((pExclLocks->NumberOfLocks-i)-1));
            }
            pExclLocks->NumberOfLocks--;

            ntError = STATUS_SUCCESS;
            goto cleanup;
        }
    }

    /* Shared Locks */

    for (i=0; i<pSharedLocks->NumberOfLocks; i++)
    {
        pEntry = &pSharedLocks->pLocks[i];

        /* Two success cases:
           1. bUnlockAll is set and the Key ID matches
           2. Match a single lock
        */

        if (bUnlockAll)
        {
            if (Key == pEntry->Key) {
                if (((pSharedLocks->NumberOfLocks-i) - 1) > 0) {
                    RtlMoveMemory(pEntry, pEntry+1,
                                  sizeof(PVFS_LOCK_ENTRY)* ((pSharedLocks->NumberOfLocks-i)-1));
                }
                pSharedLocks->NumberOfLocks--;

                ntError = STATUS_SUCCESS;
            }
            continue;
        }
        else if ((Offset == pEntry->Offset) &&
                 (Length == pEntry->Length) &&
                 (Key == pEntry->Key))
        {
            if (((pSharedLocks->NumberOfLocks-i) - 1) > 0) {
                RtlMoveMemory(pEntry, pEntry+1,
                              sizeof(PVFS_LOCK_ENTRY)* ((pSharedLocks->NumberOfLocks-i)-1));
            }
            pSharedLocks->NumberOfLocks--;

            ntError = STATUS_SUCCESS;
            goto cleanup;
        }
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBrlWriteLock, &pFcb->rwBrlLock);

    /* See if any pending locks can be granted now */

    if (ntError == STATUS_SUCCESS) {
        PvfsProcessPendingLocks(pFcb);
    }

    return ntError;
}

/**************************************************************
 *************************************************************/

static VOID
PvfsProcessPendingLocks(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PLWRTL_QUEUE pProcessingQueue = NULL;
    PPVFS_PENDING_LOCK pPendingLock = NULL;
    PVOID pData = NULL;
    LONG64 ByteOffset = 0;
    LONG64 Length = 0;
    PVFS_LOCK_FLAGS Flags = 0;
    ULONG Key = 0;
    PIRP pIrp = NULL;
    PPVFS_IRP_CONTEXT pIrpContext = NULL;
    PPVFS_CCB pCcb = NULL;
    BOOLEAN bFcbLocked = FALSE;

    /* Take the pending lock queue for processing */

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);

    if (!LwRtlQueueIsEmpty(pFcb->pPendingLockQueue))
    {
        pProcessingQueue = pFcb->pPendingLockQueue;
        pFcb->pPendingLockQueue = NULL;

        ntError = LwRtlQueueInit(&pFcb->pPendingLockQueue,
                                 PVFS_FCB_MAX_PENDING_LOCKS,
                                 PvfsFreePendingLock);
    }

    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);
    BAIL_ON_NT_STATUS(ntError);

    if (pProcessingQueue == NULL) {
        goto cleanup;
    }

    /* Process the pending lock queue */

    while (!LwRtlQueueIsEmpty(pProcessingQueue))
    {
        pData = NULL;
        pPendingLock = NULL;

        ntError = LwRtlQueueRemoveItem(pProcessingQueue,
                                       &pData);
        BAIL_ON_NT_STATUS(ntError);

        pPendingLock = (PPVFS_PENDING_LOCK)pData;

        pIrpContext = pPendingLock->pIrpContext;
        pCcb        = pPendingLock->pCcb;
        pIrp        = pIrpContext->pIrp;

        if (pPendingLock->bIsCancelled) {
            /* Safety net in case the canceled IrpContext
               failed to be added to the general Irp work queue.
               Otherwise pIrp will have been completed by the
               worker thread queue and pIrpContext would have
               been freed then as well.   See PvfsWorkerDoWork()
               for details. */

            if (pIrp) {
                pIrp->IoStatusBlock.Status = STATUS_CANCELLED;
                IoIrpComplete(pIrp);

                PvfsFreeIrpContext(&pIrpContext);
            }

            PvfsFreePendingLock((PVOID*)&pPendingLock);
            continue;
        }

        ByteOffset = pPendingLock->PendingLock.Offset;
        Length     = pPendingLock->PendingLock.Length;
        Key        = pPendingLock->PendingLock.Key;
        Flags      = PVFS_LOCK_BLOCK;
        if (pPendingLock->PendingLock.bExclusive) {
            Flags |= PVFS_LOCK_EXCLUSIVE;
        }

        /* If the lock still cannot be granted, this will
           add it back to the FCB's pending lock queue.
           We don't need to lock the IrpCtx here since it
           is too late to cancel and we are not in the worker
           thread queue. */

        ntError = PvfsLockFile(pIrpContext,
                               pCcb,
                               Key,
                               ByteOffset,
                               Length,
                               Flags);
        PvfsFreePendingLock((PVOID*)&pPendingLock);
        if (ntError == STATUS_PENDING) {
            continue;
        }

        /* We've processed the lock (to success or failure) */

        PvfsReleaseCCB(pCcb);
        PvfsFreeIrpContext(&pIrpContext);

        pIrp->IoStatusBlock.Status = ntError;
        IoIrpComplete(pIrp);
    }

cleanup:
    return;

error:
    goto cleanup;
}

/**************************************************************
 *************************************************************/

/* Does Lock #1 overlap with Lock #2? */

static BOOLEAN
DoRangesOverlap(
    LONG64 Offset1,
    LONG64 Length1,
    LONG64 Offset2,
    LONG64 Length2
    )
{
    LONG64 Start1, Start2, End1, End2;

    Start1 = Offset1;
    End1   = Offset1 + Length1 - 1;

    Start2 = Offset2;
    End2   = Offset2 + Length2 - 1;

    if (((Start1 >= Start2) && (Start1 <= End2)) ||
        ((End1 >= Start2) && (End1 <= End2)))
    {
        return TRUE;
    }

    return FALSE;
}

static NTSTATUS
CanLock(
    PPVFS_LOCK_TABLE pLockTable,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive,
    BOOLEAN bSelf
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    ULONG i = 0;
    PPVFS_LOCK_LIST pExclLocks = &pLockTable->ExclusiveLocks;
    PPVFS_LOCK_LIST pSharedLocks = &pLockTable->SharedLocks;
    PPVFS_LOCK_ENTRY pEntry = NULL;

    /* Trivial case */

    if ((pExclLocks->NumberOfLocks == 0) && (pSharedLocks->NumberOfLocks == 0))
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    /* Exclusive Locks */

    for (i=0; i<pExclLocks->NumberOfLocks; i++)
    {
        pEntry = &pExclLocks->pLocks[i];

        if (DoRangesOverlap(Offset, Length, pEntry->Offset, pEntry->Length))
        {
            if (bExclusive || !(bSelf && (Key == pEntry->Key)))
            {
                ntError = STATUS_LOCK_NOT_GRANTED;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
    }

    /* Shared Locks */

    /* Fast path.  No Exclusive locks */

    if (pExclLocks->NumberOfLocks == 0) {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    for (i=0; i<pSharedLocks->NumberOfLocks; i++)
    {
        pEntry = &pSharedLocks->pLocks[i];

        if (DoRangesOverlap(Offset, Length, pEntry->Offset, pEntry->Length))
        {
            if (bExclusive)
            {
                ntError = STATUS_LOCK_NOT_GRANTED;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;
error:
    goto cleanup;
}


/**************************************************************
 *************************************************************/

static NTSTATUS
AddLock(
    PPVFS_CCB pCcb,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_LOCK_TABLE pLockTable = &pCcb->LockTable;

    ntError = CanLock(pLockTable, Key, Offset, Length, bExclusive, TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = StoreLock(pLockTable, Key, Offset, Length, bExclusive);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/**************************************************************
 *************************************************************/

static NTSTATUS
StoreLock(
    PPVFS_LOCK_TABLE pLockTable,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    )
{
    NTSTATUS ntError= STATUS_SUCCESS;
    ULONG NewListSize = 0;
    PPVFS_LOCK_ENTRY pLocks = NULL;
    ULONG NumLocks = 0;
    PPVFS_LOCK_LIST pList = NULL;

    if (bExclusive) {
        pList = &pLockTable->ExclusiveLocks;
    } else {
        pList = &pLockTable->SharedLocks;
    }

    if (pList->NumberOfLocks == pList->ListSize)
    {
        NewListSize = pList->ListSize + 64;
        ntError = PvfsReallocateMemory((PVOID*)&pList->pLocks,
                                       sizeof(PVFS_LOCK_ENTRY)*NewListSize);
        BAIL_ON_NT_STATUS(ntError);

        pList->ListSize = NewListSize;
    }

    pLocks   = pList->pLocks;
    NumLocks = pList->NumberOfLocks;

    pLocks[NumLocks].bExclusive = bExclusive;
    pLocks[NumLocks].Offset     = Offset;
    pLocks[NumLocks].Length     = Length;
    pLocks[NumLocks].Key        = Key;

    NumLocks++;

    pList->NumberOfLocks = NumLocks;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/**************************************************************
 *************************************************************/

static VOID
InitLockEntry(
    PPVFS_LOCK_ENTRY pEntry,
    ULONG Key,
    LONG64 Offset,
    LONG64 Length,
    PVFS_LOCK_FLAGS Flags
    )
{
    /* Should never happen, but don't crash if it does */

    if (pEntry == NULL) {
        return;
    }

    pEntry->bExclusive = (Flags & PVFS_LOCK_EXCLUSIVE) ? TRUE : FALSE;
    pEntry->Key = Key;
    pEntry->Offset = Offset;
    pEntry->Length = Length;

    return;
}


/**************************************************************
 *************************************************************/

static BOOLEAN
LockEntryEqual(
    PPVFS_LOCK_ENTRY pEntry1,
    PPVFS_LOCK_ENTRY pEntry2
    )
{
    if (pEntry1 == pEntry2) {
        return TRUE;
    }

    if ((pEntry1 == NULL) || (pEntry2 == NULL)) {
        return FALSE;
    }

    /* According to tests, the lock type is ignored */

    if ((pEntry1->Key == pEntry2->Key) &&
        (pEntry1->Offset == pEntry2->Offset) &&
        (pEntry1->Length == pEntry2->Length))
    {
        return TRUE;
    }

    return FALSE;
}


/**************************************************************
 *************************************************************/

static NTSTATUS
PvfsCheckLockedRegionCanRead(
    IN PPVFS_CCB pCcb,
    IN BOOLEAN bSelf,
    IN ULONG Key,
    IN LONG64 Offset,
    IN ULONG Length
    );

static NTSTATUS
PvfsCheckLockedRegionCanWrite(
    IN PPVFS_CCB pCcb,
    IN BOOLEAN bSelf,
    IN ULONG Key,
    IN LONG64 Offset,
    IN ULONG Length
    );

NTSTATUS
PvfsCheckLockedRegion(
    IN PPVFS_CCB pCcb,
    IN PVFS_OPERATION_TYPE Operation,
    IN ULONG Key,
    IN LONG64 Offset,
    IN ULONG Length
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB_LIST_NODE pCursor = NULL;
    PPVFS_FCB pFcb = pCcb->pFcb;
    BOOLEAN bFcbLocked = FALSE;
    BOOLEAN bBrlLocked = FALSE;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(pFcb, ntError);

    switch(Operation) {
    case PVFS_OPERATION_READ:
    case PVFS_OPERATION_WRITE:
        ntError = STATUS_SUCCESS;
        break;
    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

    /* Read locks so no one can add a CCB to the list,
       or add a new BRL. */

    LWIO_LOCK_RWMUTEX_SHARED(bFcbLocked, &pFcb->rwLock);
    LWIO_LOCK_RWMUTEX_SHARED(bBrlLocked, &pFcb->rwBrlLock);

    for (pCursor = PvfsNextCCBFromList(pFcb, pCursor);
         pCursor;
         pCursor = PvfsNextCCBFromList(pFcb, pCursor))
    {
        switch(Operation)
        {
        case PVFS_OPERATION_READ:
            ntError = PvfsCheckLockedRegionCanRead(pCursor->pCcb,
                                                   (pCcb == pCursor->pCcb) ? TRUE : FALSE,
                                                   Key, Offset, Length);
            break;

         case PVFS_OPERATION_WRITE:
            ntError = PvfsCheckLockedRegionCanWrite(pCursor->pCcb,
                                                    (pCcb == pCursor->pCcb) ? TRUE : FALSE,
                                                    Key, Offset, Length);
            break;
        }
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBrlLocked, &pFcb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->rwLock);

    return ntError;

error:
    goto cleanup;

}

/**************************************************************
 *************************************************************/

static NTSTATUS
PvfsCheckLockedRegionCanRead(
    IN PPVFS_CCB pCcb,
    IN BOOLEAN bSelf,
    IN ULONG Key,
    IN LONG64 Offset,
    IN ULONG Length
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    ULONG i = 0;
    PPVFS_LOCK_LIST pExclLocks = &pCcb->LockTable.ExclusiveLocks;
    PPVFS_LOCK_ENTRY pEntry = NULL;

    /* Trivial case */

    if (pExclLocks->NumberOfLocks == 0)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    /* Check Lock table */

    for (i=0; i<pExclLocks->NumberOfLocks; i++)
    {
        pEntry = &pExclLocks->pLocks[i];

        if (DoRangesOverlap(Offset, Length, pEntry->Offset, pEntry->Length))
        {
            if (!(bSelf && (Key == pEntry->Key)))
            {
                ntError = STATUS_FILE_LOCK_CONFLICT;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**************************************************************
 *************************************************************/

static NTSTATUS
PvfsCheckLockedRegionCanWrite(
    IN PPVFS_CCB pCcb,
    IN BOOLEAN bSelf,
    IN ULONG Key,
    IN LONG64 Offset,
    IN ULONG Length
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    ULONG i = 0;
    PPVFS_LOCK_LIST pExclLocks = &pCcb->LockTable.ExclusiveLocks;
    PPVFS_LOCK_LIST pSharedLocks = &pCcb->LockTable.SharedLocks;
    PPVFS_LOCK_ENTRY pEntry = NULL;

    /* A Shared lock prevents all write access to a region...even
       ourself */

    for (i=0; i<pSharedLocks->NumberOfLocks; i++)
    {
        pEntry = &pSharedLocks->pLocks[i];

        if (DoRangesOverlap(Offset, Length, pEntry->Offset, pEntry->Length))
        {
            ntError = STATUS_FILE_LOCK_CONFLICT;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    /* Fast path...if this is ourself, there can be no conflict
       with a write lock */

    if (bSelf) {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    /* Region must be unlocked  */

    for (i=0; i<pExclLocks->NumberOfLocks; i++)
    {
        pEntry = &pExclLocks->pLocks[i];

        if (DoRangesOverlap(Offset, Length, pEntry->Offset, pEntry->Length))
        {
            ntError = STATUS_FILE_LOCK_CONFLICT;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

cleanup:
    return ntError;

error:
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
