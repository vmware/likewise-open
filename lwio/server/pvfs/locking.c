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

static
NTSTATUS
CanLock(
    PPVFS_LOCK_TABLE pLockTable,
    ULONG Key,
    ULONG64 Offset,
    ULONG64 Length,
    BOOLEAN bExclusive,
    BOOLEAN bSelf
    );

static
NTSTATUS
AddLock(
    PPVFS_CCB pCcb,
    ULONG Key,
    ULONG64 Offset,
    ULONG64 Length,
    BOOLEAN bExclusive
    );

static
NTSTATUS
StoreLock(
    PPVFS_LOCK_TABLE pLockTable,
    ULONG Key,
    ULONG64 Offset,
    ULONG64 Length,
    BOOLEAN bExclusive
    );

static
VOID
InitLockEntry(
    OUT PPVFS_LOCK_ENTRY pEntry,
    IN  ULONG Key,
    IN  ULONG64 Offset,
    IN  ULONG64 Length,
    IN  PVFS_LOCK_FLAGS Flags
    );

static
NTSTATUS
PvfsAddPendingLock(
    PPVFS_SCB pScb,
    PPVFS_IRP_CONTEXT pIrpCtx,
    PPVFS_CCB pCcb,
    PPVFS_LOCK_ENTRY pLock
    );

static
VOID
PvfsProcessPendingLocks(
    PPVFS_SCB pScb
    );

/* Code */

/**************************************************************
 *************************************************************/

NTSTATUS
PvfsLockFile(
    PPVFS_IRP_CONTEXT pIrpCtx,
    PPVFS_CCB pCcb,
    ULONG Key,
    ULONG64 Offset,
    ULONG64 Length,
    PVFS_LOCK_FLAGS Flags
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PLW_LIST_LINKS pCursor = NULL;
    PPVFS_SCB pScb = pCcb->pScb;
    BOOLEAN bExclusive = FALSE;
    BOOLEAN bScbReadLocked = FALSE;
    BOOLEAN bBrlWriteLocked = FALSE;
    PVFS_LOCK_ENTRY RangeLock = {0};
    PPVFS_CCB pCurrentCcb = NULL;

    // Test for wrap-around ranges

    if ((Length != 0) && ((Offset + Length - 1) < Offset))
    {
        ntError = STATUS_INVALID_LOCK_RANGE;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (Flags & PVFS_LOCK_EXCLUSIVE)
    {
        bExclusive = TRUE;
    }

    /* Read lock so no one can add a CCB to the list */

    LWIO_LOCK_RWMUTEX_SHARED(bScbReadLocked, &pScb->rwCcbLock);
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLocked, &pScb->rwBrlLock);

    while ((pCursor = PvfsListTraverse(pScb->pCcbList, pCursor)) != NULL)
    {
        pCurrentCcb = LW_STRUCT_FROM_FIELD(
                          pCursor,
                          PVFS_CCB,
                          ScbList);

        /* We'll deal with ourselves in AddLock() */

        if (pCcb == pCurrentCcb)
        {
            continue;
        }

        ntError = CanLock(
                      &pCurrentCcb->LockTable,
                      Key,
                      Offset,
                      Length,
                      bExclusive,
                      FALSE);
        BAIL_ON_NT_STATUS(ntError);

        pCurrentCcb = NULL;
    }

    ntError = AddLock(pCcb, Key, Offset, Length, bExclusive);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBrlWriteLocked, &pScb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bScbReadLocked, &pScb->rwCcbLock);

    return ntError;

error:
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLocked, &pScb->rwBrlLock);

    InitLockEntry(&RangeLock, Key, Offset, Length, Flags);

    /* Only try to pend a blocking lock that failed due
       to a conflict.  Not for general failures */

    if ((Flags & PVFS_LOCK_BLOCK) && (ntError == STATUS_LOCK_NOT_GRANTED))
    {
        NTSTATUS ntErrorPending = STATUS_UNSUCCESSFUL;

        ntErrorPending = PvfsAddPendingLock(pScb, pIrpCtx, pCcb, &RangeLock);
        if (ntErrorPending == STATUS_PENDING) {
            ntError = STATUS_PENDING;
        }
    }

    LWIO_UNLOCK_RWMUTEX(bBrlWriteLocked, &pScb->rwBrlLock);

    goto cleanup;
}

/* Caller must hold the mutex on the SCB */

static
NTSTATUS
PvfsAddPendingLock(
    PPVFS_SCB pScb,
    PPVFS_IRP_CONTEXT pIrpCtx,
    PPVFS_CCB pCcb,
    PPVFS_LOCK_ENTRY pLock
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_LOCK pPendingLock = NULL;

    /* Look for a cancellation request before re-queuing the request */

    ntError = PvfsQueueCancelIrpIfRequested(pIrpCtx);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pPendingLock,
                  sizeof(PVFS_PENDING_LOCK),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    pPendingLock->pIrpContext = PvfsReferenceIrpContext(pIrpCtx);

    pPendingLock->pCcb = PvfsReferenceCCB(pCcb);
    pPendingLock->PendingLock = *pLock;   /* structure assignment */

    ntError = PvfsListAddTail(
                  pScb->pPendingLockQueue,
                  &pPendingLock->LockList);
    BAIL_ON_NT_STATUS(ntError);

    if (!pIrpCtx->pScb)
    {
        pIrpCtx->pScb = PvfsReferenceSCB(pCcb->pScb);
    }
    pIrpCtx->QueueType = PVFS_QUEUE_TYPE_PENDING_LOCK;

    PvfsIrpMarkPending(pIrpCtx, PvfsQueueCancelIrp, pIrpCtx);

    /* Allow the call to be cancelled while in the queue */

    PvfsIrpContextClearFlag(
        pPendingLock->pIrpContext,
        PVFS_IRP_CTX_FLAG_ACTIVE);

    /* Memory has been given to the Queue */

    pPendingLock = NULL;

    /* Whether we pending the IRP this time or it was previously
       marked as pending, we are back on the queue */

    ntError = STATUS_PENDING;

cleanup:
    PvfsFreePendingLock(&pPendingLock);

    return ntError;

error:
    goto cleanup;
}

/**************************************************************
 *************************************************************/

NTSTATUS
PvfsUnlockFile(
    PPVFS_CCB pCcb,
    BOOLEAN bUnlockAll,
    ULONG Key,
    ULONG64 Offset,
    ULONG64 Length
    )
{
    NTSTATUS ntError = STATUS_RANGE_NOT_LOCKED;
    PPVFS_SCB pScb = pCcb->pScb;
    PPVFS_LOCK_LIST pExclLocks = &pCcb->LockTable.ExclusiveLocks;
    PPVFS_LOCK_LIST pSharedLocks = &pCcb->LockTable.SharedLocks;
    PPVFS_LOCK_ENTRY pEntry = NULL;
    ULONG i = 0;
    BOOLEAN bBrlWriteLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLock, &pScb->rwBrlLock);

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
                    RtlMoveMemory(
                        pEntry, pEntry+1,
                        sizeof(PVFS_LOCK_ENTRY)*
                            ((pExclLocks->NumberOfLocks-i)-1));
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
                RtlMoveMemory(
                    pEntry, pEntry+1,
                    sizeof(PVFS_LOCK_ENTRY)*
                        ((pExclLocks->NumberOfLocks-i)-1));
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
    LWIO_UNLOCK_RWMUTEX(bBrlWriteLock, &pScb->rwBrlLock);

    /* See if any pending locks can be granted now */

    if (ntError == STATUS_SUCCESS) {
        PvfsProcessPendingLocks(pScb);
    }

    return ntError;
}

/**************************************************************
 *************************************************************/

static
VOID
PvfsProcessPendingLocks(
    PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_LIST pProcessingQueue = NULL;
    PPVFS_PENDING_LOCK pPendingLock = NULL;
    PLW_LIST_LINKS pData = NULL;
    LONG64 ByteOffset = 0;
    LONG64 Length = 0;
    PVFS_LOCK_FLAGS Flags = 0;
    ULONG Key = 0;
    PIRP pIrp = NULL;
    PPVFS_CCB pCcb = NULL;
    BOOLEAN bBrlWriteLocked = FALSE;
    BOOLEAN bActive = FALSE;

    /* Take the pending lock queue for processing */

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLocked, &pScb->rwBrlLock);

    if (!PvfsListIsEmpty(pScb->pPendingLockQueue))
    {
        pProcessingQueue = pScb->pPendingLockQueue;
        pScb->pPendingLockQueue = NULL;

        ntError = PvfsListInit(
                      &pScb->pPendingLockQueue,
                      PVFS_SCB_MAX_PENDING_LOCKS,
                      (PPVFS_LIST_FREE_DATA_FN)PvfsFreePendingLock);
    }

    LWIO_UNLOCK_RWMUTEX(bBrlWriteLocked, &pScb->rwBrlLock);
    BAIL_ON_NT_STATUS(ntError);

    if (pProcessingQueue == NULL)
    {
        goto cleanup;
    }

    /* Process the pending lock queue */

    while (!PvfsListIsEmpty(pProcessingQueue))
    {
        pData = NULL;
        pPendingLock = NULL;

        ntError = PvfsListRemoveHead(pProcessingQueue, &pData);
        BAIL_ON_NT_STATUS(ntError);

        pPendingLock = LW_STRUCT_FROM_FIELD(
                           pData,
                           PVFS_PENDING_LOCK,
                           LockList);

        pCcb        = pPendingLock->pCcb;
        pIrp        = pPendingLock->pIrpContext->pIrp;

        PvfsQueueCancelIrpIfRequested(pPendingLock->pIrpContext);

        bActive = PvfsIrpContextMarkIfNotSetFlag(
                      pPendingLock->pIrpContext,
                      PVFS_IRP_CTX_FLAG_CANCELLED,
                      PVFS_IRP_CTX_FLAG_ACTIVE);

        if (!bActive)
        {
            PvfsFreePendingLock(&pPendingLock);
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
           add it back to the SCB's pending lock queue.
           We don't need to lock the IrpCtx here since it
           is too late to cancel and we are not in the worker
           thread queue. */

        ntError = PvfsLockFile(pPendingLock->pIrpContext,
                               pCcb,
                               Key,
                               ByteOffset,
                               Length,
                               Flags);

        if (ntError != STATUS_PENDING)
        {
            /* We've processed the lock (to success or failure) */

            pIrp->IoStatusBlock.Status = ntError;

            PvfsCompleteIrpContext(pPendingLock->pIrpContext);
        }

        /* If the lock was pended again, it ahs a new record, so
           free this one.  But avoid Freeing the IrpContext here.
           This should probably be using a ref count.  But because of
           the relationship between completing an Irp and an IrpContext,
           we do not.  */

        PvfsFreePendingLock(&pPendingLock);
    }

cleanup:
    if (pProcessingQueue)
    {
        PvfsListDestroy(&pProcessingQueue);
    }

    return;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

static
BOOLEAN
DoRangesOverlap(
    ULONG64 Offset1,
    ULONG64 Length1,
    ULONG64 Offset2,
    ULONG64 Length2
    )
{
    ULONG64 Start1, Start2, End1, End2;

    /* Zero byte locks form a boundary that overlaps when crossed */

    if ((Length1 == 0) && (Length2 == 0))
    {
        /* Two Zero byte locks never conflict with each other */

        return FALSE;
    }
    else if (Length1 == 0)
    {
        Start2 = Offset2;
        End2   = Offset2 + Length2 - 1;

        return ((Offset1 > Start2) && (Offset1 <= End2)) ?
               TRUE : FALSE;
    }
    else if (Length2 == 0)
    {
        Start1 = Offset1;
        End1   = Offset1 + Length1 - 1;

        return ((Offset2 > Start1) && (Offset2 <= End1)) ?
               TRUE : FALSE;
    }
    else
    {
        Start1 = Offset1;
        End1   = Offset1 + Length1 - 1;

        Start2 = Offset2;
        End2   = Offset2 + Length2 - 1;

        if (((Start1 >= Start2) && (Start1 <= End2)) ||
            ((End1 >= Start2) && (End1 <= End2)))
        {
            return TRUE;
        }
    }

    return FALSE;
}

static
NTSTATUS
CanLock(
    PPVFS_LOCK_TABLE pLockTable,
    ULONG Key,
    ULONG64 Offset,
    ULONG64 Length,
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

    if ((pExclLocks->NumberOfLocks == 0) && (!bExclusive)) {
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

static
NTSTATUS
AddLock(
    PPVFS_CCB pCcb,
    ULONG Key,
    ULONG64 Offset,
    ULONG64 Length,
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

static
NTSTATUS
StoreLock(
    PPVFS_LOCK_TABLE pLockTable,
    ULONG Key,
    ULONG64 Offset,
    ULONG64 Length,
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

static
VOID
InitLockEntry(
    OUT PPVFS_LOCK_ENTRY pEntry,
    IN  ULONG Key,
    IN  ULONG64 Offset,
    IN  ULONG64 Length,
    IN  PVFS_LOCK_FLAGS Flags
    )
{
    /* Should never happen, but don't crash if it does */

    if (pEntry == NULL)
    {
        return;
    }

    pEntry->bFailImmediately = (Flags & PVFS_LOCK_BLOCK) ? FALSE : TRUE;
    pEntry->bExclusive = (Flags & PVFS_LOCK_EXCLUSIVE) ? TRUE : FALSE;
    pEntry->Key = Key;
    pEntry->Offset = Offset;
    pEntry->Length = Length;

    return;
}


/**************************************************************
 *************************************************************/

static
NTSTATUS
PvfsCheckLockedRegionCanRead(
    IN PPVFS_CCB pCcb,
    IN BOOLEAN bSelf,
    IN ULONG Key,
    IN LONG64 Offset,
    IN ULONG Length
    );

static
NTSTATUS
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
    IN ULONG64 Offset,
    IN ULONG Length
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PLW_LIST_LINKS pCursor = NULL;
    PPVFS_SCB pScb = pCcb->pScb;
    BOOLEAN bScbLocked = FALSE;
    BOOLEAN bBrlLocked = FALSE;
    PPVFS_CCB pCurrentCcb = NULL;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(pScb, ntError);

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

    /* Zero byte reads and writes don't conflict */

    if (Length == 0)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }


    /* Read locks so no one can add a CCB to the list,
       or add a new BRL. */

    LWIO_LOCK_RWMUTEX_SHARED(bScbLocked, &pScb->rwCcbLock);
    LWIO_LOCK_RWMUTEX_SHARED(bBrlLocked, &pScb->rwBrlLock);

    while ((pCursor = PvfsListTraverse(pScb->pCcbList, pCursor))!= NULL)
    {
        pCurrentCcb = LW_STRUCT_FROM_FIELD(
                          pCursor,
                          PVFS_CCB,
                          ScbList);

        switch(Operation)
        {
        case PVFS_OPERATION_READ:
            ntError = PvfsCheckLockedRegionCanRead(
                          pCurrentCcb,
                          (pCcb == pCurrentCcb) ? TRUE : FALSE,
                          Key, Offset, Length);
            break;

        case PVFS_OPERATION_WRITE:
            ntError = PvfsCheckLockedRegionCanWrite(
                          pCurrentCcb,
                          (pCcb == pCurrentCcb) ? TRUE : FALSE,
                          Key, Offset, Length);
            break;
        }
        BAIL_ON_NT_STATUS(ntError);

        pCurrentCcb = NULL;
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBrlLocked, &pScb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bScbLocked, &pScb->rwCcbLock);

    return ntError;

error:
    goto cleanup;
}

/**************************************************************
 *************************************************************/

static
NTSTATUS
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

static
NTSTATUS
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

    /* Region must be unlocked  */

    for (i=0; i<pExclLocks->NumberOfLocks; i++)
    {
        pEntry = &pExclLocks->pLocks[i];

        if (DoRangesOverlap(Offset, Length, pEntry->Offset, pEntry->Length))
        {
            if (!(bSelf && (Key == pEntry->Key))) {
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


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCreateLockContext(
    OUT PPVFS_PENDING_LOCK *ppLockContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb,
    IN  ULONG Key,
    IN  ULONG64 Offset,
    IN  ULONG64 Length,
    IN  PVFS_LOCK_FLAGS Flags
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_LOCK pLockCtx;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pLockCtx,
                  sizeof(PVFS_PENDING_LOCK),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    PVFS_INIT_LINKS(&pLockCtx->LockList);

    pLockCtx->pIrpContext = PvfsReferenceIrpContext(pIrpContext);
    pLockCtx->pCcb = PvfsReferenceCCB(pCcb);
    InitLockEntry(&pLockCtx->PendingLock, Key, Offset, Length, Flags);

    *ppLockContext = pLockCtx;
    pLockCtx = NULL;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreeLockContext(
    IN OUT PVOID *ppContext
    )
{
    PPVFS_PENDING_LOCK pLockCtx = NULL;

    if (ppContext && *ppContext)
    {
        pLockCtx = (PPVFS_PENDING_LOCK)(*ppContext);

        if (pLockCtx->pIrpContext)
        {
            PvfsReleaseIrpContext(&pLockCtx->pIrpContext);
        }

        if (pLockCtx->pCcb)
        {
            PvfsReleaseCCB(pLockCtx->pCcb);
        }

        PVFS_FREE(ppContext);
    }

    return;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsLockFileWithContext(
    PVOID pContext
    )
{
    PPVFS_PENDING_LOCK pLockCtx = (PPVFS_PENDING_LOCK)pContext;
    PVFS_LOCK_FLAGS Flags = 0;

    if (pLockCtx->PendingLock.bExclusive) {
        Flags |= PVFS_LOCK_EXCLUSIVE;
    }

    if (!pLockCtx->PendingLock.bFailImmediately) {
        Flags |= PVFS_LOCK_BLOCK;
    }

    return PvfsLockFile(
               pLockCtx->pIrpContext,
               pLockCtx->pCcb,
               pLockCtx->PendingLock.Key,
               pLockCtx->PendingLock.Offset,
               pLockCtx->PendingLock.Length,
               Flags);
}


/*****************************************************************************
 ****************************************************************************/

static
BOOLEAN
PvfsHandleHasOpenByteRangeLocks(
    PPVFS_CCB pCcb
    )
{
    BOOLEAN bHasLocks = FALSE;

    if ((pCcb->LockTable.ExclusiveLocks.NumberOfLocks != 0) ||
        (pCcb->LockTable.SharedLocks.NumberOfLocks != 0))
    {
        bHasLocks = TRUE;
    }

    return bHasLocks;
}

BOOLEAN
PvfsStreamHasOpenByteRangeLocks(
    PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PLW_LIST_LINKS pCursor = NULL;
    BOOLEAN bScbLocked = FALSE;
    BOOLEAN bBrlLocked = FALSE;
    BOOLEAN bFileIsLocked = FALSE;
    PPVFS_CCB pCurrentCcb = NULL;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(pScb, ntError);

    /* Read locks so no one can add a CCB to the list,
       or add a new BRL. */

    LWIO_LOCK_RWMUTEX_SHARED(bScbLocked, &pScb->rwCcbLock);
    LWIO_LOCK_RWMUTEX_SHARED(bBrlLocked, &pScb->rwBrlLock);

    while ((pCursor = PvfsListTraverse(pScb->pCcbList, pCursor)) != NULL)
    {
        pCurrentCcb = LW_STRUCT_FROM_FIELD(
                          pCursor,
                          PVFS_CCB,
                          ScbList);

        if (PvfsHandleHasOpenByteRangeLocks(pCurrentCcb))
        {
            bFileIsLocked = TRUE;
            break;
        }

        pCurrentCcb = NULL;
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBrlLocked, &pScb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bScbLocked, &pScb->rwCcbLock);

    return bFileIsLocked;

error:
    goto cleanup;
}


////////////////////////////////////////////////////////////////////////

static
VOID
PvfsCleanPendingLockQueue(
    PVOID pContext
    );

NTSTATUS
PvfsScheduleCancelLock(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    BAIL_ON_INVALID_PTR(pIrpContext->pScb, ntError);

    pIrpCtx = PvfsReferenceIrpContext(pIrpContext);

    ntError = LwRtlQueueWorkItem(
                  gPvfsDriverState.ThreadPool,
                  PvfsCleanPendingLockQueue,
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
PvfsCleanPendingLockQueue(
    PVOID pContext
    )
{
    PPVFS_IRP_CONTEXT pIrpCtx = (PPVFS_IRP_CONTEXT)pContext;
    BOOLEAN bLocked = FALSE;
    PPVFS_PENDING_LOCK pLockRecord = NULL;
    PLW_LIST_LINKS pLockRecordLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;
    BOOLEAN bFound = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pIrpCtx->pScb->rwBrlLock);

    pLockRecordLink = PvfsListTraverse(pIrpCtx->pScb->pPendingLockQueue, NULL);

    while (pLockRecordLink)
    {
        pLockRecord = LW_STRUCT_FROM_FIELD(
                      pLockRecordLink,
                      PVFS_PENDING_LOCK,
                      LockList);

        pNextLink = PvfsListTraverse(pIrpCtx->pScb->pPendingLockQueue, pLockRecordLink);

        if (pLockRecord->pIrpContext != pIrpCtx)
        {
            pLockRecordLink = pNextLink;
            continue;
        }

        bFound = TRUE;

        PvfsListRemoveItem(pIrpCtx->pScb->pPendingLockQueue, pLockRecordLink);
        pLockRecordLink = NULL;

        LWIO_UNLOCK_RWMUTEX(bLocked, &pIrpCtx->pScb->rwBrlLock);

        pLockRecord->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsCompleteIrpContext(pLockRecord->pIrpContext);

        PvfsFreePendingLock(&pLockRecord);

        /* Can only be one IrpContext match so we are done */
    }

    LWIO_UNLOCK_RWMUTEX(bLocked, &pIrpCtx->pScb->rwBrlLock);

    if (!bFound)
    {
        pIrpCtx->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsCompleteIrpContext(pIrpCtx);
    }

    if (pIrpCtx)
    {
        PvfsReleaseIrpContext(&pIrpCtx);
    }

    return;
}

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreePendingLock(
    PPVFS_PENDING_LOCK *ppPendingLock
    )
{
    PPVFS_PENDING_LOCK pLock = NULL;

    if (ppPendingLock && *ppPendingLock)
    {
        pLock = *ppPendingLock;

        if (pLock->pIrpContext)
        {
            PvfsReleaseIrpContext(&pLock->pIrpContext);
        }

        if (pLock->pCcb)
        {
            PvfsReleaseCCB(pLock->pCcb);
        }

        PVFS_FREE(ppPendingLock);
    }

    return;
}


