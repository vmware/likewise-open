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

static VOID
PvfsCancelOplockRequestIrp(
    IN PIRP pIrp,
    IN PVOID pCancelContext
    );

static NTSTATUS
PvfsOplockGrantBatchOrLevel1(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    BOOLEAN bIsBatchOplock
    );

static NTSTATUS
PvfsOplockGrantLevel2(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    );

NTSTATUS
PvfsOplockRequest(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PVOID InputBuffer,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN  ULONG OutputBufferLength
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PIO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER pOplockRequest = NULL;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(InputBuffer, ntError);
    BAIL_ON_INVALID_PTR(OutputBuffer, ntError);

    if ((InputBufferLength < sizeof(IO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER)) ||
        (OutputBufferLength < sizeof(IO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pOplockRequest = (PIO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER)InputBuffer;

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (PVFS_IS_DIR(pCcb)) {
        ntError = STATUS_INVALID_PARAMETER;
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

    IoIrpMarkPending(
        pIrpContext->pIrp,
        PvfsCancelOplockRequestIrp,
        pIrpContext);
    pIrpContext->bIsPended = TRUE;

    ntError = STATUS_PENDING;

cleanup:
    if ((ntError != STATUS_PENDING) && pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockProcessReadyItems(
    PPVFS_FCB pFcb
    );

static VOID
PvfsFreeOplockReadyItemsContext(
    IN OUT PPVFS_FCB *ppFcb
    );

NTSTATUS
PvfsOplockBreakAck(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PVOID InputBuffer,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN  ULONG OutputBufferLength
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB pCcb = NULL;
    PPVFS_FCB pFcb = NULL;
    PVOID pData = NULL;
    BOOLEAN bOplockGranted = FALSE;
    PIO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER pOplockBreakResp = NULL;
    BOOLEAN bCcbLocked = FALSE;
    BOOLEAN bFcbLocked = FALSE;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(InputBuffer, ntError);

    if ((InputBufferLength < sizeof(IO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER)) ||
        (OutputBufferLength < sizeof(IO_FSCTL_OPLOCK_BREAK_ACK_OUTPUT_BUFFER)))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pOplockBreakResp = (PIO_FSCTL_OPLOCK_BREAK_ACK_INPUT_BUFFER)InputBuffer;

    ntError =  PvfsAcquireCCB(pIrpContext->pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (PVFS_IS_DIR(pCcb)) {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (!pCcb->bOplockBreakInProgress) {
        ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
        PVFS_ASSERT(pCcb->bOplockBreakInProgress);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Check to see if we need to re-register a level 2 oplock */

    switch (pOplockBreakResp->Response)
    {
    case IO_OPLOCK_BREAK_ACKNOWLEDGE:
        /* Only have work if we brok to a level2 oplock */
        if (pCcb->OplockBreakResult == IO_OPLOCK_BROKEN_TO_LEVEL_2)
        {
            ntError = PvfsOplockGrantLevel2(pIrpContext, pCcb);

            switch (ntError)
            {
            case STATUS_SUCCESS:
                /* If we were successfully granted the oplock, then
                   save that information for the return NTSTATUS value. */
                bOplockGranted = TRUE;
                break;

            case STATUS_OPLOCK_NOT_GRANTED:
                ntError = STATUS_SUCCESS;
                break;

            default:
                /* We may not actually want to bail here.  Needs more
                   testing */
                BAIL_ON_NT_STATUS(ntError);
            }

        }
        break;

    case IO_OPLOCK_BREAK_ACK_NO_LEVEL_2:
        if (pCcb->OplockBreakResult != IO_OPLOCK_BROKEN_TO_LEVEL_2) {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            PVFS_ASSERT(pCcb->OplockBreakResult == IO_OPLOCK_BROKEN_TO_LEVEL_2);
            BAIL_ON_NT_STATUS(ntError);
        }
        break;

    case IO_OPLOCK_BREAK_CLOSE_PENDING:
        ntError = STATUS_SUCCESS;
        break;

    default:
        break;

    }

    /* Reset oplock break state variables */

    LWIO_LOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);
    pCcb->bOplockBreakInProgress = FALSE;
    pCcb->OplockBreakResult = 0;
    LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

    /*****
     * Here we will mark all the deferred operations as ready by
     * removing them from the "pending" queue and adding them to the
     * "ready" queue.  Then we will add a global work item to process
     * all of them at once.
     *****/

    pFcb = pCcb->pFcb;

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

    pFcb->bOplockBreakInProgress = FALSE;

    PvfsReferenceFCB(pFcb);
    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  NULL, /* no IrpContext */
                  (PVOID)pFcb,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsOplockProcessReadyItems,
                  (PPVFS_WORK_CONTEXT_FREE_CTX)PvfsFreeOplockReadyItemsContext);
    BAIL_ON_NT_STATUS(ntError);

    /* We remove/add like this rather than changing pointers
       to deal with a non-empty ready queue */

    while (!LwRtlQueueIsEmpty(pFcb->pOplockPendingOpsQueue))
    {
        ntError = LwRtlQueueRemoveItem(
                      pFcb->pOplockPendingOpsQueue,
                      &pData);
        BAIL_ON_NT_STATUS(ntError);

        ntError = LwRtlQueueAddItem(
                      pFcb->pOplockReadyOpsQueue,
                      pData);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAddGlobalWorkItem((PVOID)pWorkCtx);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

    if (bOplockGranted)
    {
        /* Successful grant so pend the result now */

        IoIrpMarkPending(
            pIrpContext->pIrp,
            PvfsCancelOplockRequestIrp,
            pIrpContext);
        pIrpContext->bIsPended = TRUE;

        ntError = STATUS_PENDING;
    }

    if ((ntError != STATUS_PENDING) && pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnCreate(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static NTSTATUS
PvfsOplockBreakOnRead(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static NTSTATUS
PvfsOplockBreakOnWrite(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static NTSTATUS
PvfsOplockBreakOnLockControl(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static NTSTATUS
PvfsOplockBreakOnSetFileInformation(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_OPLOCK_RECORD pOplock,
    OUT PULONG pBreakResult
    );

static BOOLEAN
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
    IN PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    NTSTATUS ntBreakStatus = STATUS_SUCCESS;
    BOOLEAN bFcbLocked = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;
    ULONG BreakResult = 0;

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

    /*
       An OplockBreakInProgress means we are waiting for an acknowledgement.
       The caller must treat an OPLOCK_BREAK_IN_PROGRESS just as if it were
       a new break and pend the IRP's remaining work (including this test
       again).

       However, the handle on which the break is currently in progress is ok
       and should be give full access without blocking or retriggering another
       round of break prcessing.  Since We only set the OplockBreakInProgress
       flag when breaking a batch/level1 oplock, if the CCB's own
       bOplockBreakInProgress flag is set, then we assume that it is in fact
       this handle that matches.  The alternative solution is to store a
       pointer to the associated "breaking" CCBs in the FCB and compare handles.

       PS: Lock requests were seen during the oplock break processing using
       MS PowerPoint 2007.
    */

    if (pFcb->bOplockBreakInProgress)
    {
        if (pCcb->bOplockBreakInProgress == TRUE) {
            ntError = STATUS_SUCCESS;
        } else {
            ntError = STATUS_OPLOCK_BREAK_IN_PROGRESS;
        }
        goto cleanup;
    }

    pOplockLink = LwListTraverse(&pFcb->OplockList, NULL);

    while (pOplockLink)
    {
        /* Setup */

        pOplock = LW_STRUCT_FROM_FIELD(
                      pOplockLink,
                      PVFS_OPLOCK_RECORD,
                      Oplocks);

        /* Just removed the oplock record if the IRP was cancelled
           such as when a file is closed */

        if (pOplock->pIrpContext->bIsCancelled)
        {
            pNextLink = LwListTraverse(&pFcb->OplockList, pOplockLink);
            LwListRemove(pOplockLink);
            pOplockLink = pNextLink;

            pOplock->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;
            PVFS_ASSERT(pOplock->pIrpContext->bIsPended);
            IoIrpComplete(pOplock->pIrpContext->pIrp);
            PvfsFreeIrpContext(&pOplock->pIrpContext);

            PvfsFreeOplockRecord(&pOplock);

            continue;
        }

        /* Do we need to break? */

        switch (pIrpContext->pIrp->Type)
        {
        case IRP_TYPE_CREATE:
            ntBreakStatus = PvfsOplockBreakOnCreate(
                                pIrpContext,
                                pFcb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_READ:
            ntBreakStatus = PvfsOplockBreakOnRead(
                                pIrpContext,
                                pFcb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_WRITE:
            ntBreakStatus = PvfsOplockBreakOnWrite(
                                pIrpContext,
                                pFcb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_LOCK_CONTROL:
            ntBreakStatus = PvfsOplockBreakOnLockControl(
                                pIrpContext,
                                pFcb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_SET_INFORMATION:
            ntBreakStatus = PvfsOplockBreakOnSetFileInformation(
                                pIrpContext,
                                pFcb,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        default:
            pOplockLink = LwListTraverse(&pFcb->OplockList, pOplockLink);
            continue;
        }

        /* No break -- just continue processing */

        if (BreakResult == IO_OPLOCK_NOT_BROKEN) {
            pOplockLink = LwListTraverse(&pFcb->OplockList, pOplockLink);
            continue;
        }

        pNextLink = LwListTraverse(&pFcb->OplockList, pOplockLink);
        LwListRemove(pOplockLink);
        pOplockLink = pNextLink;

        PvfsFreeOplockRecord(&pOplock);

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
    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

    if (ntBreakStatus != STATUS_SUCCESS) {
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
                  pTestContext->pFcb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = pTestContext->pfnCompletion(
                      pTestContext->pCompletionContext);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pTestContext->pFcb,
                      pTestContext->pIrpContext,
                      pTestContext->pCcb,
                      pTestContext->pfnCompletion,
                      pTestContext->pfnFreeContext,
                      pTestContext->pCompletionContext);
        if (ntError == STATUS_SUCCESS)
        {
            pTestContext->pCompletionContext = NULL;
            ntError = STATUS_PENDING;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pTestContext->pFcb,
                      pTestContext->pIrpContext,
                      pTestContext->pfnCompletion,
                      pTestContext->pfnFreeContext,
                      pTestContext->pCompletionContext);
        if (ntError == STATUS_SUCCESS)
        {
            pTestContext->pCompletionContext = NULL;
            ntError = STATUS_PENDING;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    if (pTestContext->pCompletionContext) {
        pTestContext->pfnFreeContext(&pTestContext->pCompletionContext);
    }

    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakAllLevel2Oplocks(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;
    BOOLEAN bFcbLocked = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;
    PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER pOutputBuffer = NULL;

    if (!PvfsFileIsOplocked(pFcb)) {
        goto cleanup;
    }

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

    pOplockLink = LwListTraverse(&pFcb->OplockList, NULL);

    while (!LwListIsEmpty(&pFcb->OplockList))
    {
        /* Setup */

        pOplockLink = LwListRemoveTail(&pFcb->OplockList);
        pOplock = LW_STRUCT_FROM_FIELD(
                      pOplockLink,
                      PVFS_OPLOCK_RECORD,
                      Oplocks);
        pIrpCtx = pOplock->pIrpContext;
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pIrpCtx->pIrp->Args.IoFsControl.OutputBuffer;

        /* This should never fire */

        if (pOplock->OplockType != IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2) {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            BAIL_ON_NT_STATUS(ntError);
        }

        /* Break */

        pOutputBuffer->OplockBreakResult = IO_OPLOCK_BROKEN_TO_NONE;

        pIrpCtx->pIrp->IoStatusBlock.Status = STATUS_SUCCESS;
        PVFS_ASSERT(pIrpCtx->bIsPended);
        IoIrpComplete(pIrpCtx->pIrp);

        PvfsFreeIrpContext(&pIrpCtx);
        PvfsFreeOplockRecord(&pOplock);
    }

    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnCreate(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
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

    if (PvfsOplockIsMine(pCcb, pOplock)) {
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
        pOplock->pCcb->bOplockBreakInProgress = TRUE;
        pOplock->pCcb->OplockBreakResult = BreakResult;
        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        pFcb->bOplockBreakInProgress = TRUE;

        ntError = STATUS_PENDING;
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        switch(pIrpContext->pIrp->Args.Create.CreateDisposition)
        {
        case FILE_SUPERSEDE:
        case FILE_OVERWRITE:
        case FILE_OVERWRITE_IF:
            BreakResult = IO_OPLOCK_BROKEN_TO_NONE;
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
        PVFS_ASSERT(pOplock->pIrpContext->bIsPended);
        IoIrpComplete(pOplock->pIrpContext->pIrp);

        PvfsFreeIrpContext(&pOplock->pIrpContext);
    }

cleanup:
    *pBreakResult = BreakResult;

    return ntError;
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnRead(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
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

    if (PvfsOplockIsMine(pCcb, pOplock)) {
        goto cleanup;
    }

    switch (pOplock->OplockType)
    {
    case IO_OPLOCK_REQUEST_OPLOCK_BATCH:
    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

        LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
        pOplock->pCcb->bOplockBreakInProgress = TRUE;
        pOplock->pCcb->OplockBreakResult = BreakResult;
        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        pFcb->bOplockBreakInProgress = TRUE;

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
        PVFS_ASSERT(pOplock->pIrpContext->bIsPended);
        IoIrpComplete(pOplock->pIrpContext->pIrp);

        PvfsFreeIrpContext(&pOplock->pIrpContext);
    }

cleanup:
    *pBreakResult = BreakResult;

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnWrite(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
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
        if (!PvfsOplockIsMine(pCcb, pOplock)) {
            BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
            pOplock->pCcb->bOplockBreakInProgress = TRUE;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pFcb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
        }
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;
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
        PVFS_ASSERT(pOplock->pIrpContext->bIsPended);
        IoIrpComplete(pOplock->pIrpContext->pIrp);

        PvfsFreeIrpContext(&pOplock->pIrpContext);
    }

    *pBreakResult = BreakResult;

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnLockControl(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
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
        if (!PvfsOplockIsMine(pCcb, pOplock)) {
            BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
            pOplock->pCcb->bOplockBreakInProgress = TRUE;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pFcb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
        }
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;
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
        PVFS_ASSERT(pOplock->pIrpContext->bIsPended);
        IoIrpComplete(pOplock->pIrpContext->pIrp);

        PvfsFreeIrpContext(&pOplock->pIrpContext);
    }

    *pBreakResult = BreakResult;

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnSetFileInformation(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_FCB pFcb,
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
        if (!PvfsOplockIsMine(pCcb, pOplock)) {
            BreakResult = IO_OPLOCK_BROKEN_TO_NONE;

            LWIO_LOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);
            pOplock->pCcb->bOplockBreakInProgress = TRUE;
            pOplock->pCcb->OplockBreakResult = BreakResult;
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            pFcb->bOplockBreakInProgress = TRUE;

            ntError = STATUS_PENDING;
        }
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;
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
        PVFS_ASSERT(pOplock->pIrpContext->bIsPended);
        IoIrpComplete(pOplock->pIrpContext->pIrp);

        PvfsFreeIrpContext(&pOplock->pIrpContext);
    }

    *pBreakResult = BreakResult;

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsCancelOplockRequestIrp(
    IN PIRP pIrp,
    IN PVOID pCancelContext
    )
{
    PPVFS_IRP_CONTEXT pIrpCtx = (PPVFS_IRP_CONTEXT)pCancelContext;
    BOOLEAN bIsLocked = FALSE;

    LWIO_LOCK_MUTEX(bIsLocked, &pIrpCtx->Mutex);

    pIrpCtx->bIsCancelled = TRUE;

    LWIO_UNLOCK_MUTEX(bIsLocked, &pIrpCtx->Mutex);

    return;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockGrantBatchOrLevel1(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    BOOLEAN bIsBatchOplock
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    BOOLEAN bFcbControlLocked = FALSE;
    ULONG OplockType = bIsBatchOplock ?
                           IO_OPLOCK_REQUEST_OPLOCK_BATCH :
                           IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1;

    BAIL_ON_INVALID_PTR(pCcb->pFcb, ntError);

    pFcb = pCcb->pFcb;

    LWIO_LOCK_MUTEX(bFcbControlLocked, &pFcb->mutexOplock);

    /* Any other opens - FAIL */
    /* Cannot grant a second exclusive oplock - FAIL*/

    if (PvfsFileHasOtherOpens(pFcb, pCcb) ||
        PvfsFileIsOplockedExclusive(pFcb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Break any Level 2 oplocks and proceed - GRANT */

    if (!PvfsFileIsOplocked(pFcb) ||
        PvfsFileIsOplockedShared(pFcb))
    {
        ntError = PvfsOplockBreakAllLevel2Oplocks(pFcb);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsAddOplockRecord(
                      pFcb,
                      pIrpContext,
                      pCcb,
                      OplockType);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->mutexOplock);

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockGrantLevel2(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    BOOLEAN bFcbLocked = FALSE;

    BAIL_ON_INVALID_PTR(pCcb->pFcb, ntError);

    pFcb = pCcb->pFcb;

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

    /* Cannot grant a level2 on an existing exclusive oplock - FAIL*/

    if (PvfsFileIsOplockedExclusive(pFcb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Cannot grant a level2 if there are any open byte range
       locks on the file */

    if (PvfsFileHasOpenByteRangeLocks(pFcb))
    {
        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Can have multiple level2 oplocks - GRANT */

    if (!PvfsFileIsOplocked(pFcb) ||
        PvfsFileIsOplockedShared(pFcb))
    {
        ntError = PvfsAddOplockRecord(
                      pFcb,
                      pIrpContext,
                      pCcb,
                      IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

    return ntError;

error:
    goto cleanup;
}



/*****************************************************************************
 ****************************************************************************/

static BOOLEAN
PvfsOplockIsMine(
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock
    )
{
    return (pCcb == pOplock->pCcb);
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockProcessReadyItems(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PIRP pIrp = NULL;
    BOOLEAN bFcbLocked = FALSE;
    PPVFS_OPLOCK_PENDING_OPERATION pPendingOp = NULL;
    PVOID pData = NULL;
    BOOLEAN bFinished = FALSE;

    while (!bFinished)
    {
        /* Only keep the FCB locked long enough to get an item from
           the ready queue.  The completeion fn may need to relock the
           FCB and we don't want to deadlock */

        LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

        if (LwRtlQueueIsEmpty(pFcb->pOplockReadyOpsQueue))
        {
            bFinished = TRUE;
            continue;
        }

        ntError = LwRtlQueueRemoveItem(
                      pFcb->pOplockReadyOpsQueue,
                      &pData);
        BAIL_ON_NT_STATUS(ntError);

        LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

        pPendingOp = (PPVFS_OPLOCK_PENDING_OPERATION)pData;
        pIrp = pPendingOp->pIrpContext->pIrp;

        if (pPendingOp->pIrpContext->bIsCancelled) {
            ntError = STATUS_CANCELLED;
        } else {
            ntError = pPendingOp->pfnCompletion(pPendingOp->pCompletionContext);
        }

        if (ntError != STATUS_PENDING)
        {
            pIrp->IoStatusBlock.Status = ntError;
            PVFS_ASSERT(pPendingOp->pIrpContext->bIsPended);
            IoIrpComplete(pIrp);

            PvfsFreeIrpContext(&pPendingOp->pIrpContext);
        }

        if (pPendingOp->pCompletionContext) {
            pPendingOp->pfnFreeContext(&pPendingOp->pCompletionContext);
        }

        PVFS_FREE(&pPendingOp);
    }

cleanup:
    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->mutexOplock);

    return ntError;
error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsFreeOplockReadyItemsContext(
    IN OUT PPVFS_FCB *ppFcb
    )
{
    PPVFS_FCB pFcb = *ppFcb;

    if (pFcb) {
        PvfsReleaseFCB(pFcb);
    }

    return;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCreateOplockBreakTestContext(
    OUT PPVFS_PENDING_OPLOCK_BREAK_TEST *ppTestContext,
    IN  PPVFS_FCB pFcb,
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
                  sizeof(PVFS_PENDING_OPLOCK_BREAK_TEST));
    BAIL_ON_NT_STATUS(ntError);

    pTestCtx->pFcb = pFcb;
    pTestCtx->pIrpContext = pIrpContext;
    pTestCtx->pCcb = pCcb;
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
    PVFS_FREE(ppContext);

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
