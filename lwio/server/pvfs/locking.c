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
    OUT PPVFS_LOCK_ENTRY pEntry,
    IN  ULONG Key,
    IN  LONG64 Offset,
    IN  LONG64 Length,
    IN  PVFS_LOCK_FLAGS Flags
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
    PLW_LIST_LINKS pCursor = NULL;
    PPVFS_FCB pFcb = pCcb->pFcb;
    BOOLEAN bExclusive = FALSE;
    BOOLEAN bFcbReadLocked = FALSE;
    BOOLEAN bBrlWriteLocked = FALSE;
    PVFS_LOCK_ENTRY RangeLock = {0};
    PPVFS_CCB pCurrentCcb = NULL;

    /* Negative locks cannot cross the 0 offset boundary */

    if ((Offset < 0) && (Length != 0) && ((Offset + Length - 1) >= 0))
    {
        ntError = STATUS_INVALID_LOCK_RANGE;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (Flags & PVFS_LOCK_EXCLUSIVE)
    {
        bExclusive = TRUE;
    }

    /* Read lock so no one can add a CCB to the list */

    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pFcb->rwCcbLock);
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLocked, &pFcb->rwBrlLock);

    while ((pCursor = PvfsListTraverse(pFcb->pCcbList, pCursor)) != NULL)
    {
        pCurrentCcb = LW_STRUCT_FROM_FIELD(
                          pCursor,
                          PVFS_CCB,
                          FcbList);

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
    LWIO_UNLOCK_RWMUTEX(bBrlWriteLocked, &pFcb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bFcbReadLocked, &pFcb->rwCcbLock);

    return ntError;

error:
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLocked, &pFcb->rwBrlLock);

    InitLockEntry(&RangeLock, Key, Offset, Length, Flags);

    /* Only try to pend a blocking lock that failed due
       to a conflict.  Not for general failures */

    if ((Flags & PVFS_LOCK_BLOCK) && (ntError == STATUS_LOCK_NOT_GRANTED))
    {
        NTSTATUS ntErrorPending = STATUS_UNSUCCESSFUL;

        ntErrorPending = PvfsAddPendingLock(pFcb, pIrpCtx, pCcb, &RangeLock);
        if (ntErrorPending == STATUS_PENDING) {
            ntError = STATUS_PENDING;
        }
    }

    LWIO_UNLOCK_RWMUTEX(bBrlWriteLocked, &pFcb->rwBrlLock);

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

    pPendingLock->pCcb = PvfsReferenceCCB(pCcb);
    pPendingLock->PendingLock = *pLock;   /* structure assignment */

    ntError = PvfsListAddTail(
                  pFcb->pPendingLockQueue,
                  &pPendingLock->LockList);
    BAIL_ON_NT_STATUS(ntError);

    if (!pIrpCtx->pFcb)
    {
        pIrpCtx->pFcb = PvfsReferenceFCB(pCcb->pFcb);
    }
    pIrpCtx->QueueType = PVFS_QUEUE_TYPE_PENDING_LOCK;

    if (!pIrpCtx->bIsPended)
    {
        PvfsIrpMarkPending(pIrpCtx, PvfsQueueCancelIrp, pIrpCtx);
        ntError = STATUS_PENDING;
    }


    /* Memory has been given to the Queue */

    pPendingLock = NULL;

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

    /* Take the pending lock queue for processing */

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLocked, &pFcb->rwBrlLock);

    if (!PvfsListIsEmpty(pFcb->pPendingLockQueue))
    {
        pProcessingQueue = pFcb->pPendingLockQueue;
        pFcb->pPendingLockQueue = NULL;

        ntError = PvfsListInit(
                      &pFcb->pPendingLockQueue,
                      PVFS_FCB_MAX_PENDING_LOCKS,
                      (PPVFS_LIST_FREE_DATA_FN)PvfsFreePendingLock);
    }

    LWIO_UNLOCK_RWMUTEX(bBrlWriteLocked, &pFcb->rwBrlLock);
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

        if (pPendingLock->pIrpContext->bIsCancelled)
        {
            pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

            PvfsAsyncIrpComplete(pPendingLock->pIrpContext);
            PvfsFreeIrpContext(&pPendingLock->pIrpContext);

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
           add it back to the FCB's pending lock queue.
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

            PvfsAsyncIrpComplete(pPendingLock->pIrpContext);
            PvfsFreeIrpContext(&pPendingLock->pIrpContext);
        }

        /* If the lock was pended again, it ahs a new record, so
           free this one.  But avoid Freeing the IrpContext here.
           This should probably be using a ref count.  But because of
           the relationship between completing an Irp and an IrpContext,
           we do not.  */

        pPendingLock->pIrpContext = NULL;
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

static BOOLEAN
DoRangesOverlap(
    LONG64 Offset1,
    LONG64 Length1,
    LONG64 Offset2,
    LONG64 Length2
    )
{
    LONG64 Start1, Start2, End1, End2;

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
    if ((ntError == STATUS_LOCK_NOT_GRANTED) && (Offset >= 0xEF000000))
    {
        ntError = STATUS_FILE_LOCK_CONFLICT;
    }

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
    OUT PPVFS_LOCK_ENTRY pEntry,
    IN  ULONG Key,
    IN  LONG64 Offset,
    IN  LONG64 Length,
    IN  PVFS_LOCK_FLAGS Flags
    )
{
    /* Should never happen, but don't crash if it does */

    if (pEntry == NULL) {
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
    PLW_LIST_LINKS pCursor = NULL;
    PPVFS_FCB pFcb = pCcb->pFcb;
    BOOLEAN bFcbLocked = FALSE;
    BOOLEAN bBrlLocked = FALSE;
    PPVFS_CCB pCurrentCcb = NULL;

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

    /* Zero byte reads and writes don't conflict */

    if (Length == 0)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }


    /* Read locks so no one can add a CCB to the list,
       or add a new BRL. */

    LWIO_LOCK_RWMUTEX_SHARED(bFcbLocked, &pFcb->rwCcbLock);
    LWIO_LOCK_RWMUTEX_SHARED(bBrlLocked, &pFcb->rwBrlLock);

    while ((pCursor = PvfsListTraverse(pFcb->pCcbList, pCursor))!= NULL)
    {
        pCurrentCcb = LW_STRUCT_FROM_FIELD(
                          pCursor,
                          PVFS_CCB,
                          FcbList);

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
    LWIO_UNLOCK_RWMUTEX(bBrlLocked, &pFcb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->rwCcbLock);

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
    IN  LONG64 Offset,
    IN  LONG64 Length,
    IN  PVFS_LOCK_FLAGS Flags
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_LOCK pLockCtx;

    ntError = PvfsAllocateMemory((PVOID*)&pLockCtx, sizeof(PVFS_PENDING_LOCK));
    BAIL_ON_NT_STATUS(ntError);

    pLockCtx->pIrpContext = pIrpContext;
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

    if (!ppContext || !(*ppContext))
    {
        return;
    }

    pLockCtx = (PPVFS_PENDING_LOCK)(*ppContext);

    if (pLockCtx->pCcb)
    {
        PvfsReleaseCCB(pLockCtx->pCcb);
    }

    PVFS_FREE(&pLockCtx);
    *ppContext = NULL;

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

static BOOLEAN
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
PvfsFileHasOpenByteRangeLocks(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PLW_LIST_LINKS pCursor = NULL;
    BOOLEAN bFcbLocked = FALSE;
    BOOLEAN bBrlLocked = FALSE;
    BOOLEAN bFileIsLocked = FALSE;
    PPVFS_CCB pCurrentCcb = NULL;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(pFcb, ntError);

    /* Read locks so no one can add a CCB to the list,
       or add a new BRL. */

    LWIO_LOCK_RWMUTEX_SHARED(bFcbLocked, &pFcb->rwCcbLock);
    LWIO_LOCK_RWMUTEX_SHARED(bBrlLocked, &pFcb->rwBrlLock);

    while ((pCursor = PvfsListTraverse(pFcb->pCcbList, pCursor)) != NULL)
    {
        pCurrentCcb = LW_STRUCT_FROM_FIELD(
                          pCursor,
                          PVFS_CCB,
                          FcbList);

        if (PvfsHandleHasOpenByteRangeLocks(pCurrentCcb))
        {
            bFileIsLocked = TRUE;
            break;
        }

        pCurrentCcb = NULL;
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBrlLocked, &pFcb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->rwCcbLock);

    return bFileIsLocked;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsCleanPendingLockQueue(
    PVOID pContext
    );

static
VOID
PvfsCleanPendingLockFree(
    PVOID *ppContext
    );

NTSTATUS
PvfsScheduleCancelLock(
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
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsCleanPendingLockQueue,
                  (PPVFS_WORK_CONTEXT_FREE_CTX)PvfsCleanPendingLockFree);
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
PvfsCleanPendingLockQueue(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_IRP_CONTEXT pIrpContext = (PPVFS_IRP_CONTEXT)pContext;
    PPVFS_FCB pFcb = PvfsReferenceFCB(pIrpContext->pFcb);
    BOOLEAN bLocked = FALSE;
    PPVFS_PENDING_LOCK pLockRecord = NULL;
    PLW_LIST_LINKS pLockRecordLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pFcb->rwBrlLock);

    pLockRecordLink = PvfsListTraverse(pFcb->pPendingLockQueue, NULL);

    while (pLockRecordLink)
    {
        pLockRecord = LW_STRUCT_FROM_FIELD(
                      pLockRecordLink,
                      PVFS_PENDING_LOCK,
                      LockList);

        pNextLink = PvfsListTraverse(pFcb->pPendingLockQueue, pLockRecordLink);

        if (pLockRecord->pIrpContext != pIrpContext)
        {
            pLockRecordLink = pNextLink;
            continue;
        }

        PvfsListRemoveItem(pFcb->pPendingLockQueue, pLockRecordLink);
        pLockRecordLink = NULL;

        pLockRecord->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsAsyncIrpComplete(pLockRecord->pIrpContext);
        PvfsFreeIrpContext(&pLockRecord->pIrpContext);

        PvfsFreePendingLock(&pLockRecord);

        /* Can only be one IrpContext match so we are done */
    }

    LWIO_UNLOCK_RWMUTEX(bLocked, &pFcb->rwBrlLock);

    if (pFcb)
    {
        PvfsReleaseFCB(pFcb);
    }

    return ntError;
}

/*****************************************************************************
 ****************************************************************************/

static
 VOID
PvfsCleanPendingLockFree(
    PVOID *ppContext
    )
{
    /* No op -- context released in PvfsOplockCleanPendingLockQueue */
    return;
}


/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreePendingLock(
    PPVFS_PENDING_LOCK *ppPendingLock
    )
{
    if (ppPendingLock && *ppPendingLock)
    {
        if ((*ppPendingLock)->pIrpContext)
        {
            PPVFS_IRP_CONTEXT pIrpContext = (*ppPendingLock)->pIrpContext;

            pIrpContext->pIrp->IoStatusBlock.Status = STATUS_FILE_CLOSED;

            PvfsAsyncIrpComplete(pIrpContext);
            PvfsFreeIrpContext(&pIrpContext);

            (*ppPendingLock)->pIrpContext = NULL;
        }

        if ((*ppPendingLock)->pCcb)
        {
            PvfsReleaseCCB((*ppPendingLock)->pCcb);
        }

        PVFS_FREE(ppPendingLock);
    }

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
