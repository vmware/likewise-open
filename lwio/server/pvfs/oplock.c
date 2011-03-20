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
 *       oplock.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Oplock Package
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/* Forward declarations */


/* File Globals */


/* Code */

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockGrantBatchOrLevel1(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    BOOLEAN bIsBatchOplock
    );

static
NTSTATUS
PvfsOplockGrantLevel2(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    );

NTSTATUS
PvfsOplockRequest(
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PVOID  InputBuffer,
    IN     ULONG  InputBufferLength,
    OUT    PVOID  OutputBuffer,
    IN OUT PULONG pOutputBufferLength
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PIO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER pOplockRequest = NULL;
    ULONG OutputBufLen = *pOutputBufferLength;
    BOOLEAN bCcbLocked = FALSE;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(InputBuffer, ntError);
    BAIL_ON_INVALID_PTR(OutputBuffer, ntError);

    if ((InputBufferLength < sizeof(IO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER)) ||
        (OutputBufLen < sizeof(IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pOplockRequest = (PIO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER)InputBuffer;

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (!IsSetFlag(pCcb->Flags, PVFS_CCB_FLAG_CREATE_COMPLETE))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (PVFS_IS_DIR(pCcb))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (!gPvfsDriverConfig.EnableOplocks)
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Verify the oplock request type */

    switch(pOplockRequest->OplockRequestType)
    {
    case IO_OPLOCK_REQUEST_OPLOCK_BATCH:
        ntError = PvfsOplockGrantBatchOrLevel1(pIrpContext, pCcb, TRUE);
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1:
        ntError = PvfsOplockGrantBatchOrLevel1(pIrpContext, pCcb, FALSE);
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        ntError = PvfsOplockGrantLevel2(pIrpContext, pCcb);
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

    /* Successful grant so pend the resulit now */

    LWIO_LOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);
    pCcb->OplockState = PVFS_OPLOCK_STATE_GRANTED;
    LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

    pIrpContext->pScb = PvfsReferenceSCB(pCcb->pScb);
    pIrpContext->QueueType = PVFS_QUEUE_TYPE_OPLOCK;

    /* Allow the oplock request to be cancelled now */

    PvfsIrpContextClearFlag(pIrpContext, PVFS_IRP_CTX_FLAG_ACTIVE);

    *pOutputBufferLength = sizeof(IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER);

    ntError = STATUS_PENDING;

cleanup:
    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    if (PvfsIrpContextCheckFlag(pIrpContext, PVFS_IRP_CTX_FLAG_PENDED))
    {
        pIrpContext->pIrp->IoStatusBlock.Status = ntError;
        PvfsAsyncIrpComplete(pIrpContext);
    }

    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsOplockBreakAck(
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PVOID  InputBuffer,
    IN     ULONG  InputBufferLength,
    OUT    PVOID  OutputBuffer,
    IN OUT PULONG pOutputBufferLength
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB pCcb = NULL;
    PPVFS_SCB pScb = NULL;
    PIO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER pOplockBreakResp = NULL;
    ULONG OutputBufLen = *pOutputBufferLength;
    BOOLEAN bCcbLocked = FALSE;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(InputBuffer, ntError);

    if ((InputBufferLength < sizeof(IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER)) ||
        (OutputBufLen < sizeof(IO_FSCTL_OPLOCK_BREAK_ACK_OUTPUT_BUFFER)))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pOplockBreakResp = (PIO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER)InputBuffer;

    ntError =  PvfsAcquireCCB(pIrpContext->pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (!IsSetFlag(pCcb->Flags, PVFS_CCB_FLAG_CREATE_COMPLETE))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pScb = pCcb->pScb;

    if (PVFS_IS_DIR(pCcb))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    LWIO_LOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);
    if (pCcb->OplockState != PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS)
    {
        LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

        ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Reset oplock state */

    pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
    LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

    /* Check to see if we need to re-register a level 2 oplock */

    switch (pOplockBreakResp->Response)
    {
    case IO_OPLOCK_BREAK_ACKNOWLEDGE:
        /* Only have work if we broke to a level2 oplock */
        if (pCcb->OplockBreakResult == IO_OPLOCK_BROKEN_TO_LEVEL_2)
        {
            ntError = PvfsOplockGrantLevel2(pIrpContext, pCcb);

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
                /* We may not actually want to bail here.  Needs more
                   testing */
                BAIL_ON_NT_STATUS(ntError);
                break;
            }

        }
        break;

    case IO_OPLOCK_BREAK_ACK_NO_LEVEL_2:
        if (pCcb->OplockBreakResult != IO_OPLOCK_BROKEN_TO_LEVEL_2)
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            BAIL_ON_NT_STATUS(ntError);
        }
        ntError = STATUS_SUCCESS;
        break;

    case IO_OPLOCK_BREAK_CLOSE_PENDING:
        ntError = STATUS_SUCCESS;
        break;

    default:
        ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
        BAIL_ON_NT_STATUS(ntError);
        break;
    }

    ntError = PvfsOplockMarkPendedOpsReady(pScb);
    BAIL_ON_NT_STATUS(ntError);

    *pOutputBufferLength = sizeof(IO_FSCTL_OPLOCK_BREAK_ACK_OUTPUT_BUFFER);

cleanup:
    if (PvfsIrpContextCheckFlag(pIrpContext, PVFS_IRP_CTX_FLAG_PENDED))
    {
        ntError = STATUS_PENDING;
    }

    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}


////////////////////////////////////////////////////////////////////////
/// Input and Output buffers are both NULL

static
NTSTATUS
PvfsProcessPendingOplockBreakNotify(
    IN PVOID pContext
    );

static
VOID
PvfsFreePendingOplockBreakNotify(
    IN OUT PVOID *ppContext
    );

NTSTATUS
PvfsOplockBreakNotify(
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PVOID  InputBuffer,
    IN     ULONG  InputBufferLength,
    OUT    PVOID  OutputBuffer,
    IN OUT PULONG pOutputBufferLength
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PPVFS_PENDING_OPLOCK_BREAK_NOTIFY pOplockBreakNotifyCtx = NULL;

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (PVFS_IS_DIR(pCcb))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pOplockBreakNotifyCtx,
                  sizeof(*pOplockBreakNotifyCtx),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pOplockBreakNotifyCtx->pCcb = PvfsReferenceCCB(pCcb);

    ntError = PvfsAddItemPendingOplockBreakAck(
                  pCcb->pScb,
                  pIrpContext,
                  PvfsProcessPendingOplockBreakNotify,
                  PvfsFreePendingOplockBreakNotify,
                  (PVOID)pOplockBreakNotifyCtx);
    BAIL_ON_NT_STATUS(ntError);

error:

    if (ntError != STATUS_PENDING)
    {
        if (pOplockBreakNotifyCtx)
        {
            PvfsFreePendingOplockBreakNotify((PVOID*)&pOplockBreakNotifyCtx);
        }
    }

    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsProcessPendingOplockBreakNotify(
    IN PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_PENDING_OPLOCK_BREAK_NOTIFY pOplockBreakNotify = NULL;

    pOplockBreakNotify = (PPVFS_PENDING_OPLOCK_BREAK_NOTIFY)pContext;

    // Recheck the share mode before returning

    ntError = PvfsEnforceShareMode(
                  pOplockBreakNotify->pCcb->pScb,
                  pOplockBreakNotify->pCcb->ShareFlags,
                  pOplockBreakNotify->pCcb->AccessGranted);
    BAIL_ON_NT_STATUS(ntError);

    SetFlag(pOplockBreakNotify->pCcb->Flags, PVFS_CCB_FLAG_CREATE_COMPLETE);

error:

    return ntError;
}


////////////////////////////////////////////////////////////////////////

static
VOID
PvfsFreePendingOplockBreakNotify(
    IN OUT PVOID *ppContext
    )
{
    PPVFS_PENDING_OPLOCK_BREAK_NOTIFY pOplockBreakNotify = NULL;

    if (ppContext)
    {
        pOplockBreakNotify = (PPVFS_PENDING_OPLOCK_BREAK_NOTIFY)*ppContext;

        if (pOplockBreakNotify->pCcb)
        {
            PvfsReleaseCCB(pOplockBreakNotify->pCcb);
        }

        PVFS_FREE(ppContext);
    }

    return;
}

////////////////////////////////////////////////////////////////////////

static
VOID
PvfsProcessOplockDeferredOperation(
    PVOID Context
    );

NTSTATUS
PvfsOplockMarkPendedOpsReady(
    PPVFS_SCB pScb
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PLW_LIST_LINKS pData = NULL;
    BOOLEAN bScbLocked = FALSE;
    PPVFS_OPLOCK_PENDING_OPERATION pPendingOp = NULL;
    PPVFS_LIST pProcessingQueue = NULL;
    PPVFS_LIST pNewPendingQueue = NULL;

    LWIO_LOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    pScb->bOplockBreakInProgress = FALSE;

    if (PvfsListIsEmpty(pScb->pOplockPendingOpsQueue))
    {
        // Nothing to process
        status = STATUS_SUCCESS;
        goto error;
    }

    // Take ownership of the pending op list so we can drop the
    // SCB Mutex as soon as possible

    status = PvfsListInit(
                 &pNewPendingQueue,
                 PVFS_SCB_MAX_PENDING_OPERATIONS,
                 (PPVFS_LIST_FREE_DATA_FN)PvfsFreePendingOp);
    BAIL_ON_NT_STATUS(status);

    pProcessingQueue = pScb->pOplockPendingOpsQueue;
    pScb->pOplockPendingOpsQueue = pNewPendingQueue;
    pNewPendingQueue = NULL;

    LWIO_UNLOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    while (!PvfsListIsEmpty(pProcessingQueue))
    {
        status = PvfsListRemoveHead(
                     pProcessingQueue,
                     &pData);
        BAIL_ON_NT_STATUS(status);

        pPendingOp = LW_STRUCT_FROM_FIELD(
                         pData,
                         PVFS_OPLOCK_PENDING_OPERATION,
                         PendingOpList);

        status = LwRtlQueueWorkItem(
                      gPvfsDriverState.ThreadPool,
                      PvfsProcessOplockDeferredOperation,
                      pPendingOp,
                      LW_SCHEDULE_HIGH_PRIORITY);
        if (!NT_SUCCESS(status))
        {
            pPendingOp->pIrpContext->pIrp->IoStatusBlock.Status = status;
            PvfsAsyncIrpComplete(pPendingOp->pIrpContext);
            PvfsFreePendingOp(&pPendingOp);
        }

        pData = NULL;
    }

error:

    LWIO_UNLOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    if (pNewPendingQueue)
    {
        PvfsListDestroy(&pNewPendingQueue);
    }

    return status;
}

////////////////////////////////////////////////////////////////////////

static
VOID
PvfsProcessOplockDeferredOperation(
    PVOID Context
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_OPLOCK_PENDING_OPERATION pPendingOp = NULL;
    BOOLEAN irpIsActive = FALSE;

    pPendingOp = (PPVFS_OPLOCK_PENDING_OPERATION)Context;

    PvfsQueueCancelIrpIfRequested(pPendingOp->pIrpContext);

    irpIsActive = PvfsIrpContextMarkIfNotSetFlag(
                  pPendingOp->pIrpContext,
                  PVFS_IRP_CTX_FLAG_CANCELLED,
                  PVFS_IRP_CTX_FLAG_ACTIVE);

    if (irpIsActive)
    {
        ntError = pPendingOp->pfnCompletion(pPendingOp->pCompletionContext);
    }
    else
    {
        ntError = STATUS_CANCELLED;
    }

    /* Cancelled IRPs are handled here since they have already
       been removed from the list */

    if (ntError != STATUS_PENDING)
    {
        pPendingOp->pIrpContext->pIrp->IoStatusBlock.Status = ntError;
        PvfsAsyncIrpComplete(pPendingOp->pIrpContext);
    }

    PvfsFreePendingOp(&pPendingOp);

    return;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockBreakOnCreate(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static
NTSTATUS
PvfsOplockBreakOnRead(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static
NTSTATUS
PvfsOplockBreakOnWrite(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static
NTSTATUS
PvfsOplockBreakOnLockControl(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static
NTSTATUS
PvfsOplockBreakOnSetFileInformation(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static
BOOLEAN
PvfsOplockIsMine(
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock
    );

/**
 * Return values
 *   STATUS_SUCCESS - No break or no deferred operation necessary
 *   STATUS_PENDING - break is in progresss that requires caller
 *                    to defer remaining operation
 *   STATUS_OPLOCK_BREAK_IN_PROGRESS - Oplock break is underway.
 *                    Caller must re-queue oplock break test for a
 *                    later time.
 **/

NTSTATUS
PvfsOplockBreakIfLocked(
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb,
    IN PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    NTSTATUS ntBreakStatus = STATUS_SUCCESS;
    BOOLEAN bScbLocked = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;
    ULONG BreakResult = 0;
    BOOLEAN bActive = FALSE;
    BOOLEAN bCcbLocked = FALSE;
    PPVFS_LIST pBrokenOplocks = NULL;

    LWIO_LOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    /*
       An OplockBreakInProgress means we are waiting for an acknowledgement.
       The caller must treat an OPLOCK_BREAK_IN_PROGRESS just as if it were
       a new break and pend the IRP's remaining work (including this test
       again).

       However, the handle on which the break is currently in progress is ok
       and should be give full access without blocking or retriggering another
       round of break processing.  Since We only set the OplockBreakInProgress
       flag when breaking a batch/level1 oplock, if the CCB's own
       PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS flag is set, then we assume that
       it is in fact this handle that matches.  The alternative solution
       is to store a pointer to the associated "breaking" CCBs in the SCB
       and compare handles.

       PS: Lock requests were seen during the oplock break processing using
       MS PowerPoint 2007.
    */

    if (pScb->bOplockBreakInProgress)
    {
        LWIO_LOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);
        if (pCcb->OplockState == PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS)
        {
            ntError = STATUS_SUCCESS;
        }
        else
        {
            // TODO: This state may be unnecessary since we are adding the
            // CCB to teh SCB list before testing for an oplock break.
            // Any new oplock requests will take the incomplete CCB into
            // account so we should not get into an inconsistent state.
            // However, leacing this in place for now until the new Lease
            // code is in place.  --gcarter@likewise.com

            ntError = STATUS_OPLOCK_BREAK_IN_PROGRESS;
        }
        LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);
        goto cleanup;
    }

    pOplockLink = PvfsListTraverse(pScb->pOplockList, NULL);

    while (pOplockLink)
    {
        /* Setup */

        pOplock = LW_STRUCT_FROM_FIELD(
                      pOplockLink,
                      PVFS_OPLOCK_RECORD,
                      OplockList);

        PvfsQueueCancelIrpIfRequested(pOplock->pIrpContext);

        bActive = PvfsIrpContextMarkIfNotSetFlag(
                      pOplock->pIrpContext,
                      PVFS_IRP_CTX_FLAG_CANCELLED,
                      PVFS_IRP_CTX_FLAG_ACTIVE);

        if (!bActive)
        {
            pNextLink = PvfsListTraverse(pScb->pOplockList, pOplockLink);
            PvfsListRemoveItem(pScb->pOplockList, pOplockLink);
            pOplockLink = pNextLink;

            PvfsFreeOplockRecord(&pOplock);

            continue;
        }

        /* Do we need to break? */

        switch (pIrpContext->pIrp->Type)
        {
        case IRP_TYPE_CREATE:
            ntBreakStatus = PvfsOplockBreakOnCreate(
                                pIrpContext,
                                pScb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_READ:
            ntBreakStatus = PvfsOplockBreakOnRead(
                                pIrpContext,
                                pScb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_WRITE:
            ntBreakStatus = PvfsOplockBreakOnWrite(
                                pIrpContext,
                                pScb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_LOCK_CONTROL:
            ntBreakStatus = PvfsOplockBreakOnLockControl(
                                pIrpContext,
                                pScb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_SET_INFORMATION:
            ntBreakStatus = PvfsOplockBreakOnSetFileInformation(
                                pIrpContext,
                                pScb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        default:
            /* This IRP will not cause a break at all */
            ntBreakStatus = STATUS_SUCCESS;
            ntError = STATUS_SUCCESS;
            goto cleanup;
        }

        /* No break -- just continue processing */

        if (BreakResult == IO_OPLOCK_NOT_BROKEN)
        {
            PvfsIrpContextClearFlag(pOplock->pIrpContext, PVFS_IRP_CTX_FLAG_ACTIVE);
            pOplockLink = PvfsListTraverse(pScb->pOplockList, pOplockLink);
            continue;
        }

        pNextLink = PvfsListTraverse(pScb->pOplockList, pOplockLink);
        PvfsListRemoveItem(pScb->pOplockList, pOplockLink);
        pOplockLink = pNextLink;

        /* Add the broken oplock to a list that we can free outside of the current
           SCB ControlBlock lock */

        if (pBrokenOplocks == NULL)
        {
            ntError = PvfsListInit(
                          &pBrokenOplocks,
                          0,
                          (PPVFS_LIST_FREE_DATA_FN)PvfsFreeOplockRecord);
            BAIL_ON_NT_STATUS(ntError);
        }

        ntError = PvfsListAddTail(pBrokenOplocks, &pOplock->OplockList);
        BAIL_ON_NT_STATUS(ntError);

        /* Broken -- See if we need to defer the calling operation
           and remove the oplock record from the list */

        if (ntBreakStatus == STATUS_PENDING)
        {
            ntError = ntBreakStatus;
            ntBreakStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntBreakStatus);
    }


cleanup:
    LWIO_UNLOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    PvfsListDestroy(&pBrokenOplocks);

    if (ntBreakStatus != STATUS_SUCCESS)
    {
        ntError = ntBreakStatus;
    }

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsOplockPendingBreakIfLocked(
    IN PPVFS_PENDING_OPLOCK_BREAK_TEST pTestContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    ntError = PvfsOplockBreakIfLocked(
                  pTestContext->pIrpContext,
                  pTestContext->pCcb,
                  pTestContext->pScb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = pTestContext->pfnCompletion(
                      pTestContext->pCompletionContext);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pTestContext->pScb,
                      pTestContext->pIrpContext,
                      pTestContext->pCcb,
                      pTestContext->pfnCompletion,
                      pTestContext->pfnFreeContext,
                      pTestContext->pCompletionContext);
        if (ntError == STATUS_PENDING)
        {
            pTestContext->pCompletionContext = NULL;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pTestContext->pScb,
                      pTestContext->pIrpContext,
                      pTestContext->pfnCompletion,
                      pTestContext->pfnFreeContext,
                      pTestContext->pCompletionContext);
        if (ntError == STATUS_PENDING)
        {
            pTestContext->pCompletionContext = NULL;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    if (pTestContext->pCompletionContext)
    {
        pTestContext->pfnFreeContext(&pTestContext->pCompletionContext);
    }

    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockBreakAllLevel2Oplocks(
    PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;
    PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER pOutputBuffer = NULL;
    BOOLEAN bCcbLocked = FALSE;

    if (!PvfsStreamIsOplocked(pScb))
    {
        goto cleanup;
    }

    while (!PvfsListIsEmpty(pScb->pOplockList))
    {
        bCcbLocked = FALSE;

        /* Setup */

        ntError = PvfsListRemoveHead(pScb->pOplockList, &pOplockLink);
        BAIL_ON_NT_STATUS(ntError);

        pOplock = LW_STRUCT_FROM_FIELD(
                      pOplockLink,
                      PVFS_OPLOCK_RECORD,
                      OplockList);
        pIrpCtx = pOplock->pIrpContext;
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pIrpCtx->pIrp->Args.IoFsControl.OutputBuffer;

        /* This should never fire */

        if (pOplock->OplockType != IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2)
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            BAIL_ON_NT_STATUS(ntError);
        }

        /* Break */

        pOutputBuffer->OplockBreakResult = IO_OPLOCK_BROKEN_TO_NONE;

        pIrpCtx->pIrp->IoStatusBlock.Status = STATUS_SUCCESS;

        PvfsAsyncIrpComplete(pIrpCtx);

        LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
        pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        PvfsFreeOplockRecord(&pOplock);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockBreakOnCreate(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER pOutputBuffer = NULL;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;

    /* Don't break our own oplock */

    if (PvfsOplockIsMine(pCcb, pOplock))
    {
        goto cleanup;
    }

    switch (pOplock->OplockType)
    {
    case IO_OPLOCK_REQUEST_OPLOCK_BATCH:
    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1:
        switch(pIrpContext->pIrp->Args.Create.CreateDisposition)
        {
        case FILE_SUPERSEDE:
        case FILE_OVERWRITE:
        case FILE_OVERWRITE_IF:
            BreakResult = IO_OPLOCK_BROKEN_TO_NONE;
            break;
        default:
            BreakResult = IO_OPLOCK_BROKEN_TO_LEVEL_2;
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

        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        pScb->bOplockBreakInProgress = TRUE;

        ntError = STATUS_PENDING;
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        switch(pIrpContext->pIrp->Args.Create.CreateDisposition)
        {
        case FILE_SUPERSEDE:
        case FILE_OVERWRITE:
        case FILE_OVERWRITE_IF:
            BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
            pOplock->pCcb->OplockBreakResult = BreakResult;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            break;

        default:
            BreakResult = IO_OPLOCK_NOT_BROKEN;
            break;
        }

        ntError = STATUS_SUCCESS;
        break;

    default:
        break;
    }

    if (BreakResult != IO_OPLOCK_NOT_BROKEN)
    {
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pOplock->pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_SUCCESS;

        PvfsAsyncIrpComplete(pOplock->pIrpContext);
    }

cleanup:
    *pBreakResult = BreakResult;

    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockBreakOnRead(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER pOutputBuffer = NULL;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;

    /* Don't break our own lock */

    if (PvfsOplockIsMine(pCcb, pOplock))
    {
        goto cleanup;
    }

    switch (pOplock->OplockType)
    {
    case IO_OPLOCK_REQUEST_OPLOCK_BATCH:
    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1:
        BreakResult = IO_OPLOCK_BROKEN_TO_LEVEL_2;

        LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
            BAIL_ON_NT_STATUS(ntError);
        }

        pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
        pOplock->pCcb->OplockBreakResult = BreakResult;

        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        pScb->bOplockBreakInProgress = TRUE;

        ntError = STATUS_PENDING;

        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_NOT_BROKEN;
        ntError = STATUS_SUCCESS;
        break;

    default:
        break;
    }

    if (BreakResult != IO_OPLOCK_NOT_BROKEN)
    {
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pOplock->pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_SUCCESS;

        PvfsAsyncIrpComplete(pOplock->pIrpContext);
    }

cleanup:
    *pBreakResult = BreakResult;

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockBreakOnWrite(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER pOutputBuffer = NULL;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;

    switch (pOplock->OplockType)
    {
    case IO_OPLOCK_REQUEST_OPLOCK_BATCH:
    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1:
        /* Don't break our own lock */
        if (!PvfsOplockIsMine(pCcb, pOplock))
        {
            BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
        }
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

        LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
            BAIL_ON_NT_STATUS(ntError);
        }

        pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
        pOplock->pCcb->OplockBreakResult = BreakResult;

        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        ntError = STATUS_SUCCESS;
        break;

    default:
        break;
    }

    if (BreakResult != IO_OPLOCK_NOT_BROKEN)
    {
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pOplock->pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_SUCCESS;

        PvfsAsyncIrpComplete(pOplock->pIrpContext);
    }

    *pBreakResult = BreakResult;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockBreakOnLockControl(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER pOutputBuffer = NULL;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;

    switch (pOplock->OplockType)
    {
    case IO_OPLOCK_REQUEST_OPLOCK_BATCH:
    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1:
        /* Don't break our own lock */
        if (!PvfsOplockIsMine(pCcb, pOplock))
        {
            BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
        }
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

        LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
            BAIL_ON_NT_STATUS(ntError);
        }

        pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
        pOplock->pCcb->OplockBreakResult = BreakResult;

        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        ntError = STATUS_SUCCESS;
        break;

    default:
        break;
    }

    if (BreakResult != IO_OPLOCK_NOT_BROKEN)
    {
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pOplock->pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_SUCCESS;

        PvfsAsyncIrpComplete(pOplock->pIrpContext);
    }

    *pBreakResult = BreakResult;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockBreakOnSetFileInformation(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bCcbLocked = FALSE;
    PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER pOutputBuffer = NULL;
    ULONG BreakResult = IO_OPLOCK_NOT_BROKEN;

    switch (pOplock->OplockType)
    {
    case IO_OPLOCK_REQUEST_OPLOCK_BATCH:
    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1:
        /* Don't break our own lock */
        if (!PvfsOplockIsMine(pCcb, pOplock))
        {
            BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
            {
                ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
                LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
                BAIL_ON_NT_STATUS(ntError);
            }

            pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS;
            pOplock->pCcb->OplockBreakResult = BreakResult;

            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pScb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
        }
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

        LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        if (pOplock->pCcb->OplockState != PVFS_OPLOCK_STATE_GRANTED)
        {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
            BAIL_ON_NT_STATUS(ntError);
        }

        pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
        pOplock->pCcb->OplockBreakResult = BreakResult;

        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        ntError = STATUS_SUCCESS;
        break;

    default:
        break;
    }

    if (BreakResult != IO_OPLOCK_NOT_BROKEN)
    {
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pOplock->pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_SUCCESS;

        PvfsAsyncIrpComplete(pOplock->pIrpContext);
    }

    *pBreakResult = BreakResult;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsOplockCleanOplockQueue(
    PVOID pContext
    );

NTSTATUS
PvfsScheduleCancelOplock(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    BAIL_ON_INVALID_PTR(pIrpContext->pScb, ntError);

    pIrpCtx = PvfsReferenceIrpContext(pIrpContext);

    ntError = LwRtlQueueWorkItem(
                  gPvfsDriverState.ThreadPool,
                  PvfsOplockCleanOplockQueue,
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

/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsOplockCleanOplockQueue(
    PVOID pContext
    )
{
    PPVFS_IRP_CONTEXT pIrpCtx = (PPVFS_IRP_CONTEXT)pContext;
    PPVFS_SCB pScb = PvfsReferenceSCB(pIrpCtx->pScb);
    BOOLEAN bScbLocked = FALSE;
    BOOLEAN bCcbLocked= FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;
    BOOLEAN bFound = FALSE;

    LWIO_LOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    pOplockLink = PvfsListTraverse(pScb->pOplockList, NULL);

    while (pOplockLink)
    {
        pOplock = LW_STRUCT_FROM_FIELD(
                      pOplockLink,
                      PVFS_OPLOCK_RECORD,
                      OplockList);

        pNextLink = PvfsListTraverse(pScb->pOplockList, pOplockLink);

        if (pOplock->pIrpContext != pIrpCtx)
        {
            pOplockLink = pNextLink;
            continue;
        }

        bFound = TRUE;

        PvfsListRemoveItem(pScb->pOplockList, pOplockLink);
        pOplockLink = NULL;

        LWIO_UNLOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

        pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsAsyncIrpComplete(pOplock->pIrpContext);

        LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
        pOplock->pCcb->OplockState = PVFS_OPLOCK_STATE_NONE;
        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        PvfsFreeOplockRecord(&pOplock);

        /* Can only be one IrpContext match so we are done */
    }

    LWIO_UNLOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);


    if (!bFound)
    {
        pIrpCtx->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsAsyncIrpComplete(pIrpCtx);
    }


    if (pScb)
    {
        PvfsReleaseSCB(&pScb);
    }

    if (pIrpCtx)
    {
        PvfsReleaseIrpContext(&pIrpCtx);
    }

    return;
}

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockGrantBatchOrLevel1(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    BOOLEAN bIsBatchOplock
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_SCB pScb = NULL;
    BOOLEAN bScbControlLocked = FALSE;
    ULONG OplockType = bIsBatchOplock ?
                           IO_OPLOCK_REQUEST_OPLOCK_BATCH :
                           IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1;

    BAIL_ON_INVALID_PTR(pCcb->pScb, ntError);

    pScb = pCcb->pScb;

    LWIO_LOCK_MUTEX(bScbControlLocked, &pScb->BaseControlBlock.Mutex);

    /* Any other opens - FAIL */
    /* Cannot grant a second exclusive oplock - FAIL*/

    if (PvfsStreamHasOtherOpens(pScb, pCcb) ||
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


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsOplockGrantLevel2(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_OPLOCK_NOT_GRANTED;
    PPVFS_SCB pScb = NULL;
    BOOLEAN bScbLocked = FALSE;

    BAIL_ON_INVALID_PTR(pCcb->pScb, ntError);

    pScb = pCcb->pScb;

    LWIO_LOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    /* Cannot grant a level2 on an existing exclusive oplock - FAIL*/

    if (PvfsStreamIsOplockedExclusive(pScb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Cannot grant a level2 if there are any open byte range
       locks on the file */

    if (PvfsStreamHasOpenByteRangeLocks(pScb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Can have multiple level2 oplocks - GRANT */

    if (!PvfsStreamIsOplocked(pScb) ||
        PvfsStreamIsOplockedShared(pScb))
    {
        PvfsIrpMarkPending(pIrpContext, PvfsQueueCancelIrp, pIrpContext);

        ntError = PvfsAddOplockRecord(
                      pScb,
                      pIrpContext,
                      pCcb,
                      IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2);
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    LWIO_UNLOCK_MUTEX(bScbLocked, &pScb->BaseControlBlock.Mutex);

    return ntError;

error:
    goto cleanup;
}



/*****************************************************************************
 ****************************************************************************/

static
BOOLEAN
PvfsOplockIsMine(
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock
    )
{
    return (pCcb == pOplock->pCcb);
}


/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreePendingOp(
    PPVFS_OPLOCK_PENDING_OPERATION *ppPendingOp
    )
{
    PPVFS_OPLOCK_PENDING_OPERATION pPendingOp = NULL;

    if (ppPendingOp && *ppPendingOp)
    {
        pPendingOp = *ppPendingOp;

        /* We may need to complete an IRP */

        if (pPendingOp->pIrpContext)
        {
            PvfsReleaseIrpContext(&pPendingOp->pIrpContext);
        }

        /* Release the context state */

        if (pPendingOp->pfnFreeContext)
        {
            pPendingOp->pfnFreeContext(&pPendingOp->pCompletionContext);
        }
        else
        {
            PVFS_FREE(&pPendingOp->pCompletionContext);
        }

        PVFS_FREE(ppPendingOp);
    }

    return;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCreateOplockBreakTestContext(
    OUT PPVFS_PENDING_OPLOCK_BREAK_TEST *ppTestContext,
    IN  PPVFS_SCB pScb,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN  PPVFS_OPLOCK_PENDING_COMPLETION_FREE_CTX pfnFreeContext,
    IN  PVOID pCompletionContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_OPLOCK_BREAK_TEST pTestCtx = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pTestCtx,
                  sizeof(PVFS_PENDING_OPLOCK_BREAK_TEST),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pTestCtx->pScb = PvfsReferenceSCB(pScb);
    pTestCtx->pIrpContext = PvfsReferenceIrpContext(pIrpContext);
    pTestCtx->pCcb = PvfsReferenceCCB(pCcb);
    pTestCtx->pfnCompletion = pfnCompletion;
    pTestCtx->pfnFreeContext = pfnFreeContext;
    pTestCtx->pCompletionContext = pCompletionContext;

    *ppTestContext = pTestCtx;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreeOplockBreakTestContext(
    IN OUT PPVFS_PENDING_OPLOCK_BREAK_TEST *ppContext
    )
{
    PPVFS_PENDING_OPLOCK_BREAK_TEST pCtx = NULL;

    if (ppContext && *ppContext)
    {
        pCtx = *ppContext;

        if (pCtx->pIrpContext)
        {
            PvfsReleaseIrpContext(&pCtx->pIrpContext);
        }

        if (pCtx->pCcb)
        {
            PvfsReleaseCCB(pCtx->pCcb);
        }
        if (pCtx->pScb)
        {
            PvfsReleaseSCB(&pCtx->pScb);
        }

        PVFS_FREE(ppContext);
    }

    return;
}

