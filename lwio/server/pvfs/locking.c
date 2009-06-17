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
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive,
    BOOLEAN bAllowOverlaps
    );

static NTSTATUS
AddLock(
    PPVFS_CCB pCcb,
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    );

static NTSTATUS
StoreLock(
    PPVFS_LOCK_TABLE pLockTable,
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    );

static VOID
InitLockEntry(
    PPVFS_LOCK_ENTRY pEntry,
    PULONG pKey,
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
    PULONG pKey,
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

    if (Flags & PVFS_LOCK_EXCLUSIVE) {
        bExclusive = TRUE;
    }

    /* Read lock so no one can add a CCB to the list */

    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pFcb->rwLock);

    /* Store the pending lock */

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
                          pKey, Offset, Length, bExclusive, FALSE);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = AddLock(pCcb, pKey, Offset, Length, bExclusive);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBrlWriteLocked, &pFcb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bFcbReadLocked, &pFcb->rwLock);

    return ntError;

error:
    LWIO_LOCK_MUTEX(bLastFailedLock, &pFcb->ControlBlock);

    InitLockEntry(&RangeLock, pKey, Offset, Length, Flags);

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
       resets every time a new lock range is requested.
       Pass all errors other than LOCK_NOT_GRANTED on through. */

    if (ntError == STATUS_LOCK_NOT_GRANTED)
    {
        if (LockEntryEqual(&pFcb->LastFailedLock, &RangeLock)) {
            ntError = STATUS_FILE_LOCK_CONFLICT;
        }
        InitLockEntry(&pFcb->LastFailedLock, pKey, Offset, Length, Flags);
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
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length
    )
{
    NTSTATUS ntError = STATUS_INVALID_LOCK_SEQUENCE;
    PPVFS_FCB pFcb = pCcb->pFcb;
    PPVFS_LOCK_LIST pExclLocks = &pCcb->LockTable.ExclusiveLocks;
    PPVFS_LOCK_LIST pSharedLocks = &pCcb->LockTable.SharedLocks;
    PPVFS_LOCK_ENTRY pEntry = NULL;
    ULONG i = 0;
    BOOLEAN bBrlWriteLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBrlWriteLock, &pFcb->rwBrlLock);

    /* If the caller wants to release all locks, just set the
       NumberOfLocks to 0 */

    if (bUnlockAll && (pKey == NULL)) {
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
            if (*pKey == pEntry->Key) {
                if (((pExclLocks->NumberOfLocks-i) - 1) > 0) {
                    RtlMoveMemory(pEntry, pEntry+1,
                                  sizeof(PVFS_LOCK_ENTRY)* ((pExclLocks->NumberOfLocks-i)-1));
                }
                pExclLocks->NumberOfLocks--;

                ntError = STATUS_SUCCESS;
            }

            continue;
        }
        else if ((Offset == pEntry->Offset) && (Length == pEntry->Length))
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
            if (*pKey == pEntry->Key) {
                if (((pSharedLocks->NumberOfLocks-i) - 1) > 0) {
                    RtlMoveMemory(pEntry, pEntry+1,
                                  sizeof(PVFS_LOCK_ENTRY)* ((pSharedLocks->NumberOfLocks-i)-1));
                }
                pSharedLocks->NumberOfLocks--;

                ntError = STATUS_SUCCESS;
            }
            continue;
        }
        else if ((Offset == pEntry->Offset) && (Length == pEntry->Length))
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

    PvfsProcessPendingLocks(pFcb);

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
                               Key != 0 ? &Key : NULL,
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
 FIXME!!!!!
 *************************************************************/

NTSTATUS
PvfsCanReadWriteFile(
    PPVFS_CCB pCcb,
    PULONG pKey,
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
    BOOLEAN bBrlReadLocked = FALSE;

    if (Flags & PVFS_LOCK_EXCLUSIVE) {
        bExclusive = TRUE;
    }

    /* Read lock so no one can add a CCB to the list */

    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pFcb->rwLock);

    /* Store the pending lock */

    LWIO_LOCK_RWMUTEX_SHARED(bBrlReadLocked, &pFcb->rwBrlLock);

    for (pCursor = PvfsNextCCBFromList(pFcb, pCursor);
         pCursor;
         pCursor = PvfsNextCCBFromList(pFcb, pCursor))
    {
        if (pCcb == pCursor->pCcb) {
            continue;
        }

        ntError = CanLock(&pCursor->pCcb->LockTable,
                          pKey, Offset, Length, bExclusive, TRUE);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBrlReadLocked, &pFcb->rwBrlLock);
    LWIO_UNLOCK_RWMUTEX(bFcbReadLocked, &pFcb->rwLock);

    return ntError;

error:
    goto cleanup;
}



/**************************************************************
 *************************************************************/

static NTSTATUS
CanLock(
    PPVFS_LOCK_TABLE pLockTable,
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive,
    BOOLEAN bAllowOverlaps
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

        /* No overlaps ever allowed for exclusive locks */

        if ((Offset >= pEntry->Offset) &&
            (Offset < (pEntry->Offset + pEntry->Length)))
        {
            ntError = STATUS_LOCK_NOT_GRANTED;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    /* Shared Locks */

    for (i=0; i<pSharedLocks->NumberOfLocks; i++)
    {
        pEntry = &pSharedLocks->pLocks[i];

        if ((Offset >= pEntry->Offset) &&
            (Offset < (pEntry->Offset + pEntry->Length)))
        {
            /* Owning CCB can overlap shared locks only */
            if (!bExclusive || !bAllowOverlaps) {
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
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_LOCK_TABLE pLockTable = &pCcb->LockTable;

    /* See if we can lock the region.  Allow overlaps since we
       own the lock table */

    ntError = CanLock(pLockTable, pKey, Offset, Length, bExclusive, TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = StoreLock(pLockTable, pKey, Offset, Length, bExclusive);
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
    PULONG pKey,
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
    pLocks[NumLocks].Key        = pKey ? *pKey : 0;

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
    PULONG pKey,
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
    pEntry->Key = pKey != NULL ? *pKey : 0;
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

    if ((pEntry1->bExclusive == pEntry2->bExclusive) &&
        (pEntry1->Key == pEntry2->Key) &&
        (pEntry1->Offset == pEntry2->Offset) &&
        (pEntry1->Length == pEntry2->Length))
    {
        return TRUE;
    }

    return FALSE;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
