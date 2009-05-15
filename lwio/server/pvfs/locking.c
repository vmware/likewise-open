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

/* Code */

/**************************************************************
 TODO: Implement blocking locks
 *************************************************************/

NTSTATUS
PvfsLockFile(
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
    InitLockEntry(&RangeLock, pKey, Offset, Length, Flags);

    /* Windows 2003 & XP return LOCK_NOT_GRANTED on the first
       failure, and FILE_LOCK_CONFLICT on subsequent errors */

    LWIO_LOCK_MUTEX(bLastFailedLock, &pFcb->ControlBlock);
    if (!LockEntryEqual(&pFcb->LastFailedLock, &RangeLock)) {
        ntError = STATUS_LOCK_NOT_GRANTED;
        InitLockEntry(&pFcb->LastFailedLock, pKey, Offset, Length, Flags);
    }
    LWIO_UNLOCK_MUTEX(bLastFailedLock, &pFcb->ControlBlock);

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

    ENTER_WRITER_RW_LOCK(&pFcb->rwBrlLock);

    /* Exclusive Locks */

    for (i=0; i<pExclLocks->NumberOfLocks; i++)
    {
        pEntry = &pExclLocks->pLocks[i];

        if ((Offset == pEntry->Offset) && (Length == pEntry->Length))
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

        if ((Offset == pEntry->Offset) && (Length == pEntry->Length))
        {
            if (((pExclLocks->NumberOfLocks-i) - 1) > 0) {
                RtlMoveMemory(pEntry, pEntry+1,
                              sizeof(PVFS_LOCK_ENTRY)* ((pSharedLocks->NumberOfLocks-i)-1));
            }
            pSharedLocks->NumberOfLocks--;

            ntError = STATUS_SUCCESS;
            goto cleanup;
        }
    }

cleanup:
    LEAVE_WRITER_RW_LOCK(&pFcb->rwBrlLock);

    return ntError;
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

    ENTER_READER_RW_LOCK(&pFcb->rwLock);
    bFcbReadLocked = TRUE;

    /* Store the pending lock */

    ENTER_READER_RW_LOCK(&pFcb->rwBrlLock);
    bBrlReadLocked = TRUE;

    for (pCursor = PvfsNextCCBFromList(pFcb, pCursor);
         pCursor;
         pCursor = PvfsNextCCBFromList(pFcb, pCursor))
    {
        /* We'll deal with ourselves in AddLock() */

        if (pCcb == pCursor->pCcb) {
            continue;
        }

        ntError = CanLock(&pCursor->pCcb->LockTable,
                          pKey, Offset, Length, bExclusive, TRUE);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    if (bBrlReadLocked) {
        LEAVE_READER_RW_LOCK(&pFcb->rwBrlLock);
    }

    if (bFcbReadLocked) {
        LEAVE_READER_RW_LOCK(&pFcb->rwLock);
    }

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
            (Offset <= (pEntry->Offset + pEntry->Length)))
        {
            ntError = STATUS_FILE_LOCK_CONFLICT;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    /* Shared Locks */

    for (i=0; i<pSharedLocks->NumberOfLocks; i++)
    {
        pEntry = &pSharedLocks->pLocks[i];

        if ((Offset >= pEntry->Offset) &&
            (Offset <= (pEntry->Offset + pEntry->Length)))
        {
            /* Owning CCB can overlap shared locks only */
            if (!bExclusive || !bAllowOverlaps) {
                ntError = STATUS_FILE_LOCK_CONFLICT;
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
