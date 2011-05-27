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
 *       lease.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        lease Package
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

static
BOOLEAN
PvfsIsSameOplockKey(
    PPVFS_OPLOCK_KEY pOplockKey1,
    PPVFS_OPLOCK_KEY pOplockKey2
    );

static
BOOLEAN
PvfsLeaseIsMine(
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock
    );

static
BOOLEAN
PvfsStreamAllOtherOpensHasSameOplockKey(
    IN PPVFS_SCB pScb
    );

static
NTSTATUS
PvfsLeaseGrantRead(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    );

static
NTSTATUS
PvfsLeaseGrantReadHandle(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    );

static
NTSTATUS
PvfsLeaseGrantReadWrite(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    );

static
NTSTATUS
PvfsLeaseGrantReadWriteHandle(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    );

BOOLEAN
PvfsIsReadLease(
    IN IO_LEASE_STATE LeaseState
    )
{
    return (IsSetFlag(LeaseState, IO_LEASE_STATE_READ) &&
           !IsSetFlag(LeaseState, IO_LEASE_STATE_WRITE) &&
           !IsSetFlag(LeaseState, IO_LEASE_STATE_HANDLE));
}

BOOLEAN
PvfsIsReadHandleLease(
    IN IO_LEASE_STATE LeaseState
    )
{
    return (IsSetFlag(LeaseState, IO_LEASE_STATE_READ) &&
           !IsSetFlag(LeaseState, IO_LEASE_STATE_WRITE) &&
           IsSetFlag(LeaseState, IO_LEASE_STATE_HANDLE));
}

BOOLEAN
PvfsIsReadWriteLease(
    IN IO_LEASE_STATE LeaseState
    )
{
    return (IsSetFlag(LeaseState, IO_LEASE_STATE_READ) &&
            IsSetFlag(LeaseState, IO_LEASE_STATE_WRITE) &&
           !IsSetFlag(LeaseState, IO_LEASE_STATE_HANDLE));
}

BOOLEAN
PvfsIsReadWriteHandleLease(
    IN IO_LEASE_STATE LeaseState
    )
{
    return (IsSetFlag(LeaseState, IO_LEASE_STATE_READ) &&
            IsSetFlag(LeaseState, IO_LEASE_STATE_WRITE) &&
            IsSetFlag(LeaseState, IO_LEASE_STATE_HANDLE));
}

BOOLEAN
PvfsIsValidLeaseState(
    IN IO_LEASE_STATE LeaseState
    )
{
    return PvfsIsReadLease(LeaseState) ||
           PvfsIsReadHandleLease(LeaseState) ||
           PvfsIsReadWriteLease(LeaseState) ||
           PvfsIsReadWriteHandleLease(LeaseState);
}

NTSTATUS
PvfsOplockGrantLease(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    IO_LEASE_STATE LeaseState
    )
{
    NTSTATUS ntError = STATUS_OPLOCK_NOT_GRANTED;

    switch (LeaseState)
    {
        case IO_LEASE_STATE_READ:
            ntError = PvfsLeaseGrantRead(pIrpContext, pCcb);
            break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE:
            ntError = PvfsLeaseGrantReadHandle(pIrpContext, pCcb);
            break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE:
            ntError = PvfsLeaseGrantReadWrite(pIrpContext, pCcb);
            break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE | IO_LEASE_STATE_HANDLE:
            ntError = PvfsLeaseGrantReadWriteHandle(pIrpContext, pCcb);
            break;

        default:
            ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);

error:
    return ntError;
}

static
NTSTATUS
PvfsLeaseGrantRead(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_OPLOCK_NOT_GRANTED;
    PPVFS_SCB pScb = NULL;
    BOOLEAN bScbLocked = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;

    BAIL_ON_INVALID_PTR(pCcb->pScb, ntError);

    pScb = pCcb->pScb;

    LWIO_LOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    /* Cannot grant a read lease on an existing exclusive oplock or RW or RWH - FAIL*/

    if (PvfsStreamIsOplockedExclusive(pScb) ||
        PvfsStreamIsLeaseReadWrite(pScb) ||
        PvfsStreamIsLeaseReadWriteHandle(pScb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Cannot grant a read lease if there are any open byte range
       locks on the file */

    if (PvfsStreamHasOpenByteRangeLocks(pScb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Go through existing OplockList to see whether we generate 'STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE' (for level 2 and read)
     * Or STATUS_OPLOCK_NOT_GRANTED */
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

        // Current lease is level 2 / R && Requesting R
        if (pOplock->OplockType == IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2 ||
            (pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE && PvfsIsReadLease(pOplock->LeaseState)))
        {
		if (PvfsIsSameOplockKey(pOplock->pCcb->pOplockKey, pCcb->pOplockKey))
		{
                // remove pOplock, and set status to STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE
                PvfsListRemoveItem(pScb->pOplockList, pOplockLink);
                pOplockLink = NULL;

                // should be at most one present
                pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE;
                PvfsAsyncCompleteIrpContext(pOplock->pIrpContext);

                PvfsFreeOplockRecord(&pOplock);

                break;
		}
        }
        // Current lease is RH && Requesting R
        else if (pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE &&
                     PvfsIsReadHandleLease(pOplock->LeaseState))
             {
		     if (PvfsIsSameOplockKey(pOplock->pCcb->pOplockKey, pCcb->pOplockKey))
		     {
			 ntError = STATUS_OPLOCK_NOT_GRANTED;
			 BAIL_ON_NT_STATUS(ntError);
		     }
             }
        // this should not happen as it has already been checked up front
        // and read lease won't be granted
        else
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            BAIL_ON_NT_STATUS(ntError);
        }
    }// end-of-while

    /* Can have multiple level2 oplocks and/or read leases - GRANT */
    PvfsIrpMarkPending(pIrpContext, PvfsQueueCancelIrp, pIrpContext);

    ntError = PvfsAddLeaseRecord(
                  pScb,
                  pIrpContext,
                  pCcb,
                  IO_LEASE_STATE_READ);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    return ntError;

error:
    goto cleanup;
}

static
NTSTATUS
PvfsLeaseGrantReadHandle(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_OPLOCK_NOT_GRANTED;
    PPVFS_SCB pScb = NULL;
    BOOLEAN bScbLocked = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;
    int iTotalRorRHLease = 0;

    BAIL_ON_INVALID_PTR(pCcb->pScb, ntError);

    pScb = pCcb->pScb;

    LWIO_LOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    /* Cannot grant a read lease on an existing exclusive oplock or RW or RWH - FAIL*/

    if (PvfsStreamIsOplockedExclusive(pScb) ||
        PvfsStreamIsOplockedShared(pScb) ||
        PvfsStreamIsLeaseReadWrite(pScb) ||
        PvfsStreamIsLeaseReadWriteHandle(pScb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Cannot grant a read lease if there are any open byte range
       locks on the file */

    if (PvfsStreamHasOpenByteRangeLocks(pScb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Go through existing OplockList to see whether we generate 'STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE'
     * for read or read handle */
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

        if (((pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE && PvfsIsReadLease(pOplock->LeaseState)) ||
            (pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE && PvfsIsReadHandleLease(pOplock->LeaseState))) &&
            PvfsIsSameOplockKey(pOplock->pCcb->pOplockKey, pCcb->pOplockKey))
        {
            // remove pOplock
            PvfsListRemoveItem(pScb->pOplockList, pOplockLink);

            // should be at most one present
            pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE;
            PvfsAsyncCompleteIrpContext(pOplock->pIrpContext);

            PvfsFreeOplockRecord(&pOplock);

            iTotalRorRHLease++;

            // at most one Read and one Read handle lease (total 2) with the same oplockkey should be in oplocklist
            if (iTotalRorRHLease == 2)
            {
                break;
            }
        }
        // this should not happen as it has already been checked upfront
        // and read lease won't be granted
        else
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            BAIL_ON_NT_STATUS(ntError);
        }
    }// end-of-while

    /* Can have multiple read and read handle leases - GRANT */
    PvfsIrpMarkPending(pIrpContext, PvfsQueueCancelIrp, pIrpContext);

    ntError = PvfsAddLeaseRecord(
                  pScb,
                  pIrpContext,
                  pCcb,
                  IO_LEASE_STATE_READ|IO_LEASE_STATE_HANDLE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    return ntError;

error:
    goto cleanup;
}

static
NTSTATUS
PvfsLeaseGrantReadWrite(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_SCB pScb = NULL;
    BOOLEAN bScbControlLocked = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;
    int iTotalRorRWLease = 0;

    BAIL_ON_INVALID_PTR(pCcb->pScb, ntError);

    pScb = pCcb->pScb;

    LWIO_LOCK_MUTEX(bScbControlLocked, &pScb->BaseControlBlock.Mutex);

    /* Any other opens - FAIL */
    /* Cannot grant a second exclusive oplock - FAIL*/

    if ((pScb->OpenHandleCount > 1 &&
         !PvfsStreamAllOtherOpensHasSameOplockKey(pScb)) ||
        PvfsStreamIsOplockedExclusive(pScb) ||
        PvfsStreamIsOplockedShared(pScb) ||
        PvfsStreamIsLeaseReadHandle(pScb) ||
        PvfsStreamIsLeaseReadWriteHandle(pScb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Go through existing OplockList to see whether we generate 'STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE'
     * (for read or read-write) */
    while ((pOplockLink = PvfsListTraverse(pScb->pOplockList, pOplockLink)) != NULL)
    {
        pOplock = LW_STRUCT_FROM_FIELD(
                          pOplockLink,
                          PVFS_OPLOCK_RECORD,
                          OplockList);

        if (PvfsIrpContextCheckFlag(pOplock->pIrpContext, PVFS_IRP_CTX_FLAG_CANCELLED))
        {
            pOplock = NULL;
            continue;
        }

        if (((pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE &&
              PvfsIsReadLease(pOplock->LeaseState)) ||
            (pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE &&
             PvfsIsReadWriteLease(pOplock->LeaseState))))
        {
            if (PvfsIsSameOplockKey(pOplock->pCcb->pOplockKey, pCcb->pOplockKey))
            {
                // remove pOplock
                PvfsListRemoveItem(pScb->pOplockList, pOplockLink);

                // should be at most one present
                pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE;
                PvfsAsyncCompleteIrpContext(pOplock->pIrpContext);

                PvfsFreeOplockRecord(&pOplock);

                iTotalRorRWLease++;

                // at most one Read and one Read Write lease (total 2) with the same oplockkey in oplocklist
                if (iTotalRorRWLease == 2)
                {
                    break;
                }
            }
            else
            {
                ntError = STATUS_OPLOCK_NOT_GRANTED;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        // this should not happen as it has already been checked upfront
        // and read lease won't be granted
        else
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            BAIL_ON_NT_STATUS(ntError);
        }
    }// end-of-while

    /* Grant RW lease */
    PvfsIrpMarkPending(pIrpContext, PvfsQueueCancelIrp, pIrpContext);

    ntError = PvfsAddLeaseRecord(
                  pScb,
                  pIrpContext,
                  pCcb,
                  IO_LEASE_STATE_READ|IO_LEASE_STATE_WRITE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_MUTEX(bScbControlLocked, &pScb->BaseControlBlock.Mutex);

    return ntError;

error:
    goto cleanup;
}

static
NTSTATUS
PvfsLeaseGrantReadWriteHandle(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_SCB pScb = NULL;
    BOOLEAN bScbControlLocked = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;
    int iTotalRorRHOrRWOrRWHLease = 0;

    BAIL_ON_INVALID_PTR(pCcb->pScb, ntError);

    pScb = pCcb->pScb;

    LWIO_LOCK_MUTEX(bScbControlLocked, &pScb->BaseControlBlock.Mutex);

    /* Any other opens - FAIL */
    /* Cannot grant a second exclusive oplock - FAIL*/

    if ((pScb->OpenHandleCount > 1 &&
         !PvfsStreamAllOtherOpensHasSameOplockKey(pScb)) ||
        PvfsStreamIsOplockedExclusive(pScb) ||
        PvfsStreamIsOplockedShared(pScb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Go through existing OplockList to see whether we generate 'STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE'
     * (for read or read-write) */
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

        if (((pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE && PvfsIsReadLease(pOplock->LeaseState)) ||
            (pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE && PvfsIsReadWriteLease(pOplock->LeaseState))) ||
            (pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE && PvfsIsReadHandleLease(pOplock->LeaseState)) ||
            (pOplock->OplockType == IO_OPLOCK_REQUEST_LEASE && PvfsIsReadWriteHandleLease(pOplock->LeaseState)))
        {
            if (PvfsIsSameOplockKey(pOplock->pCcb->pOplockKey, pCcb->pOplockKey))
            {
                // remove pOplock
                PvfsListRemoveItem(pScb->pOplockList, pOplockLink);

                // should be at most one present
                pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_OPLOCK_SWITCHED_TO_NEW_HANDLE;
                PvfsAsyncCompleteIrpContext(pOplock->pIrpContext);

                PvfsFreeOplockRecord(&pOplock);

                iTotalRorRHOrRWOrRWHLease++;

                // at most one R, one RH, one RW, one RWH (total 4) with the same oplockkey
                // in oplocklist with a SCB
                if (iTotalRorRHOrRWOrRWHLease == 4)
                {
                    break;
                }
            }
            else
            {
                ntError = STATUS_OPLOCK_NOT_GRANTED;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        // this should not happen as it has already been checked upfront
        // and read lease won't be granted
        else
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            BAIL_ON_NT_STATUS(ntError);
        }
    }// end-of-while

    /* Grant RWH lease */
    PvfsIrpMarkPending(pIrpContext, PvfsQueueCancelIrp, pIrpContext);

    ntError = PvfsAddLeaseRecord(
                  pScb,
                  pIrpContext,
                  pCcb,
                  IO_LEASE_STATE_READ|IO_LEASE_STATE_WRITE|IO_LEASE_STATE_HANDLE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_MUTEX(bScbControlLocked, &pScb->BaseControlBlock.Mutex);

    return ntError;

error:
    goto cleanup;
}

static
BOOLEAN
PvfsIsSameOplockKey(
    PPVFS_OPLOCK_KEY pOplockKey1,
    PPVFS_OPLOCK_KEY pOplockKey2
    )
{
    // Todo: implement when oplockey is specified
    return FALSE;
}

static
BOOLEAN
PvfsStreamAllOtherOpensHasSameOplockKey(
    IN PPVFS_SCB pScb
    )
{
    PLW_LIST_LINKS pCursor = NULL;
    PPVFS_CCB pCurrCcb = NULL;
    PPVFS_CCB pPrevCcb = NULL;
    BOOLEAN bLocked = FALSE;
    BOOLEAN bHasSameOplockKey = TRUE;

    LWIO_LOCK_RWMUTEX_SHARED(bLocked, &pScb->rwCcbLock);

    while ((pCursor = PvfsListTraverse(pScb->pCcbList, pCursor)) != NULL)
    {
        pCurrCcb = LW_STRUCT_FROM_FIELD(
                   pCursor,
                   PVFS_CCB,
                   ScbList);

        if (pPrevCcb && !PvfsIsSameOplockKey(pCurrCcb->pOplockKey, pPrevCcb->pOplockKey))
        {
            bHasSameOplockKey = FALSE;
            break;
        }

        pPrevCcb = pCurrCcb;
        pCurrCcb = NULL;
    }

    LWIO_UNLOCK_RWMUTEX(bLocked, &pScb->rwCcbLock);

    return bHasSameOplockKey;
}

/*****************************************************************************
 ****************************************************************************/

static
BOOLEAN
PvfsLeaseIsMine(
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock
    )
{
    return PvfsIsSameOplockKey(pCcb->pOplockKey,
                               pOplock->pCcb->pOplockKey);
}



NTSTATUS
PvfsLeaseBreakOnCreate(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
#if 0
    IN  BOOLEAN bShareViolation,
#endif
    OUT PULONG pBreakResult,
    OUT PIO_LEASE_STATE pNewLeaseState
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;
    IO_LEASE_STATE NewLeaseState = IO_LEASE_STATE_NONE;

    /* break only if the oplock key is different */

    if (PvfsLeaseIsMine(pCcb, pOplock))
    {
        goto cleanup;
    }

    switch (pOplock->LeaseState)
    {
        case IO_LEASE_STATE_READ:

            switch(pIrpContext->pIrp->Args.Create.CreateDisposition)
            {
                case FILE_SUPERSEDE:
                case FILE_OVERWRITE:
                case FILE_OVERWRITE_IF:
                    BreakResult = IO_OPLOCK_BROKEN_LEASE;
                    NewLeaseState = IO_LEASE_STATE_NONE;

                    LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

                    if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
                    {
                        ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                        BAIL_ON_NT_STATUS(ntError);
                    }

                    pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
                    pOplock->pCcb->OplockBreakResult = BreakResult;
                    pOplock->pCcb->NewLeaseState = IO_LEASE_STATE_NONE;

                    LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

                    ntError = STATUS_SUCCESS;

                    break;
                default:
                    BreakResult = IO_OPLOCK_NOT_BROKEN;
                    break;
            }

            ntError = STATUS_SUCCESS;
            break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE:

            switch(pIrpContext->pIrp->Args.Create.CreateDisposition)
            {
                case FILE_SUPERSEDE:
                case FILE_OVERWRITE:
                case FILE_OVERWRITE_IF:
                    BreakResult = IO_OPLOCK_BROKEN_LEASE;
                    NewLeaseState = IO_LEASE_STATE_NONE;
                    break;
                default:
                    BreakResult = IO_OPLOCK_BROKEN_LEASE;
                    NewLeaseState = IO_LEASE_STATE_READ;
                    break;
            }

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
            break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE | IO_LEASE_STATE_HANDLE:

            switch(pIrpContext->pIrp->Args.Create.CreateDisposition)
            {
                case FILE_SUPERSEDE:
                case FILE_OVERWRITE:
                case FILE_OVERWRITE_IF:
                    BreakResult = IO_OPLOCK_BROKEN_LEASE;
                    NewLeaseState = IO_LEASE_STATE_NONE;
                    break;

                default:
#if 0
                    if (bShareViolation)
                    {
                        BreakResult = IO_OPLOCK_BROKEN_LEASE;
                        NewLeaseState = IO_LEASE_STATE_READ|IO_LEASE_STATE_WRITE;
                    }
                    else
                    {
                        BreakResult = IO_OPLOCK_BROKEN_LEASE;
                        NewLeaseState = IO_LEASE_STATE_READ|IO_LEASE_STATE_HANDLE;
                    }
#endif

                    break;
            }

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;

            break;

        default:
            ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    *pBreakResult = BreakResult;
    *pNewLeaseState = NewLeaseState;

    return ntError;

error:
    goto cleanup;
}

NTSTATUS
PvfsLeaseBreakOnRead(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult,
    OUT PIO_LEASE_STATE pNewLeaseState
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;
    IO_LEASE_STATE NewLeaseState = IO_LEASE_STATE_NONE;


    /* break only if the oplock key is different */

    if (PvfsLeaseIsMine(pCcb, pOplock))
    {
        goto cleanup;
    }

    switch (pOplock->LeaseState)
    {
        case IO_LEASE_STATE_READ:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE:

            BreakResult = IO_OPLOCK_NOT_BROKEN;
            ntError = STATUS_SUCCESS;

            break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE | IO_LEASE_STATE_HANDLE:

            BreakResult = IO_OPLOCK_BROKEN_LEASE;
            if ((IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE) == pOplock->LeaseState)
            {
                NewLeaseState = IO_LEASE_STATE_READ;
            }
            else
            {
                NewLeaseState = IO_LEASE_STATE_READ|IO_LEASE_STATE_HANDLE;
            }

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;

            break;

        default:
            ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    *pBreakResult = BreakResult;
    *pNewLeaseState = NewLeaseState;

    return ntError;

error:
    goto cleanup;
}

NTSTATUS
PvfsLeaseBreakOnWrite(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult,
    OUT PIO_LEASE_STATE pNewLeaseState
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;
    IO_LEASE_STATE NewLeaseState = IO_LEASE_STATE_NONE;


    /* break only if the oplock key is different */

    if (PvfsLeaseIsMine(pCcb, pOplock))
    {
        goto cleanup;
    }

    switch (pOplock->LeaseState)
    {
        case IO_LEASE_STATE_READ:

            BreakResult = IO_OPLOCK_BROKEN_LEASE;
            NewLeaseState = IO_LEASE_STATE_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            ntError = STATUS_SUCCESS;

            break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE | IO_LEASE_STATE_HANDLE:

            BreakResult = IO_OPLOCK_BROKEN_LEASE;
            NewLeaseState = IO_LEASE_STATE_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
            break;

        default:
            ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    *pBreakResult = BreakResult;
    *pNewLeaseState = NewLeaseState;

    return ntError;

error:
    goto cleanup;
}

NTSTATUS
PvfsLeaseBreakOnLockControl(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult,
    OUT PIO_LEASE_STATE pNewLeaseState
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;
    IO_LEASE_STATE NewLeaseState = IO_LEASE_STATE_NONE;


    /* break only if the oplock key is different */

    if (PvfsLeaseIsMine(pCcb, pOplock))
    {
        goto cleanup;
    }

    switch (pOplock->LeaseState)
    {
        case IO_LEASE_STATE_READ:

            BreakResult = IO_OPLOCK_BROKEN_LEASE;
            NewLeaseState = IO_LEASE_STATE_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            ntError = STATUS_SUCCESS;

            break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE | IO_LEASE_STATE_HANDLE:

            BreakResult = IO_OPLOCK_BROKEN_LEASE;
            NewLeaseState = IO_LEASE_STATE_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
            break;

        default:
            ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    *pBreakResult = BreakResult;
    *pNewLeaseState = NewLeaseState;

    return ntError;

error:
    goto cleanup;
}

NTSTATUS
PvfsLeaseBreakOnSetFileInformationEoFOrAllocation(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult,
    OUT PIO_LEASE_STATE pNewLeaseState
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;
    IO_LEASE_STATE NewLeaseState = IO_LEASE_STATE_NONE;


    /* break only if the oplock key is different */

    if (PvfsLeaseIsMine(pCcb, pOplock))
    {
        goto cleanup;
    }

    switch (pOplock->LeaseState)
    {
        case IO_LEASE_STATE_READ:

            BreakResult = IO_OPLOCK_BROKEN_LEASE;
            NewLeaseState = IO_LEASE_STATE_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            ntError = STATUS_SUCCESS;

            break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE | IO_LEASE_STATE_HANDLE:

            BreakResult = IO_OPLOCK_BROKEN_LEASE;
            NewLeaseState = IO_LEASE_STATE_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
            break;

        default:
            ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    *pBreakResult = BreakResult;
    *pNewLeaseState = NewLeaseState;

    return ntError;

error:
    goto cleanup;
}

NTSTATUS
PvfsLeaseBreakOnSetFileInformationRenameOrLink(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult,
    OUT PIO_LEASE_STATE pNewLeaseState
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;
    IO_LEASE_STATE NewLeaseState = IO_LEASE_STATE_NONE;


    /* break only if the oplock key is different */

    if (PvfsLeaseIsMine(pCcb, pOplock))
    {
        goto cleanup;
    }

    switch (pOplock->LeaseState)
    {
        case IO_LEASE_STATE_READ:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE:

            BreakResult = IO_OPLOCK_NOT_BROKEN;
            ntError = STATUS_SUCCESS;

        break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE | IO_LEASE_STATE_HANDLE:

            BreakResult = IO_OPLOCK_BROKEN_LEASE;

            if ((IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE) == pOplock->LeaseState)
            {
                NewLeaseState = IO_LEASE_STATE_READ;
            }
            else
            {
                NewLeaseState = IO_LEASE_STATE_READ|IO_LEASE_STATE_WRITE;
            }

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
            break;

        default:
            ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    *pBreakResult = BreakResult;
    *pNewLeaseState = NewLeaseState;

    return ntError;

error:
    goto cleanup;
}

NTSTATUS
PvfsLeaseBreakOnSetFileInformationDisposition(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult,
    OUT PIO_LEASE_STATE pNewLeaseState
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;
    IO_LEASE_STATE NewLeaseState = IO_LEASE_STATE_NONE;


    /* break only if the oplock key is different */

    if (PvfsLeaseIsMine(pCcb, pOplock))
    {
        goto cleanup;
    }

    switch (pOplock->LeaseState)
    {
        case IO_LEASE_STATE_READ:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE:

            BreakResult = IO_OPLOCK_NOT_BROKEN;
            ntError = STATUS_SUCCESS;

        break;

        case IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE:
        case IO_LEASE_STATE_READ | IO_LEASE_STATE_WRITE | IO_LEASE_STATE_HANDLE:

            BreakResult = IO_OPLOCK_BROKEN_LEASE;

            if ((IO_LEASE_STATE_READ | IO_LEASE_STATE_HANDLE) == pOplock->LeaseState)
            {
                NewLeaseState = IO_LEASE_STATE_READ;
            }
            else
            {
                NewLeaseState = IO_LEASE_STATE_READ|IO_LEASE_STATE_WRITE;
            }

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            pOplock->pCcb->NewLeaseState = NewLeaseState;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
            break;

        default:
            ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    *pBreakResult = BreakResult;
    *pNewLeaseState = NewLeaseState;

    return ntError;

error:
    goto cleanup;
}

NTSTATUS
PvfsLeaseBreakAck(
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb,
    IN PPVFS_SCB pScb,
    IN IO_LEASE_STATE AckLeaseState
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;

    if (pCcb->OplockBreakResult != IO_OPLOCK_BROKEN_LEASE)
    {
        ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
        BAIL_ON_NT_STATUS(ntError);
    }

    // Acknowledged lease state should be no higher than the intended new lease state
    // that server has broken to

    switch (pCcb->NewLeaseState)
    {
        case IO_LEASE_STATE_NONE:
            if (IsSetFlag(AckLeaseState, IO_LEASE_STATE_READ) ||
                IsSetFlag(AckLeaseState, IO_LEASE_STATE_WRITE) ||
                IsSetFlag(AckLeaseState, IO_LEASE_STATE_HANDLE))
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            }
            break;


        case IO_LEASE_STATE_READ:
            if (IsSetFlag(AckLeaseState, IO_LEASE_STATE_WRITE) ||
                IsSetFlag(AckLeaseState, IO_LEASE_STATE_HANDLE))
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            }
            break;

        case IO_LEASE_STATE_READ|IO_LEASE_STATE_WRITE:
            if (IsSetFlag(AckLeaseState, IO_LEASE_STATE_HANDLE))
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            }
            break;

        case IO_LEASE_STATE_READ|IO_LEASE_STATE_HANDLE:
            if (IsSetFlag(AckLeaseState, IO_LEASE_STATE_WRITE))
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            }
            break;

        default:
            ntError = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsOplockGrantLease(pIrpContext,
                                       pCcb,
                                       AckLeaseState);
    switch (ntError)
    {
        case STATUS_SUCCESS:
            pIrpContext->pScb = PvfsReferenceSCB(pScb);
            pIrpContext->QueueType = PVFS_QUEUE_TYPE_OPLOCK;

            LWIO_LOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);
            pCcb->OplockState = PVFS_OPLOCK_STATE_GRANTED;
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

            PvfsIrpMarkPending(
                pIrpContext,
                PvfsQueueCancelIrp,
                pIrpContext);
            break;

        case STATUS_OPLOCK_NOT_GRANTED:
            ntError = STATUS_SUCCESS;
            break;

        default:
            BAIL_ON_NT_STATUS(ntError);
            break;
    }

error:
    return ntError;
}

#if 0
static
BOOLEAN
PvfsEqualOplockKey(
    PVFS_OPLOCK_KEY OplockKey1,
    PVFS_OPLOCK_KEY OplockKey2
    );

static
NTSTATUS
PvfsLeaseGrantRW(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_SCB pScb = NULL;
    BOOLEAN bScbControlLocked = FALSE;

    BAIL_ON_INVALID_PTR(pCcb->pScb, ntError);

    pScb = pCcb->pScb;

    LWIO_LOCK_MUTEX(bScbControlLocked, &pScb->BaseControlBlock.Mutex);

    /* Any other opens - FAIL */
    /* Cannot grant a second exclusive oplock - FAIL*/

    if (pScb->OpenHandleCount > 1 ||
        PvfsStreamIsOplockedExclusive(pScb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Break any Level 2 oplocks and proceed - GRANT */

    if (!PvfsStreamIsOplocked(pScb) ||
        PvfsStreamIsOplockedShared(pScb))
    {
        ntError = PvfsOplockBreakAllLevel2Oplocks(pScb);
        BAIL_ON_NT_STATUS(ntError);

        PvfsIrpMarkPending(pIrpContext, PvfsQueueCancelIrp, pIrpContext);

        ntError = PvfsAddOplockRecord(
                      pScb,
                      pIrpContext,
                      pCcb,
                      OplockType);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_MUTEX(bScbControlLocked, &pScb->BaseControlBlock.Mutex);

    return ntError;

error:
    goto cleanup;
}

static
BOOLEAN
PvfsEqualOplockKey(
    PVFS_OPLOCK_KEY OplockKey1,
    PVFS_OPLOCK_KEY OplockKey2
    )
{
    return (!uuid_compare(OplockKey1.ClientGuid, OplockKey2.ClientGuid) &&
            !uuid_compare(OplockKey1.LeaseKey, OplockKey2.LeaseKey));
}
#endif


