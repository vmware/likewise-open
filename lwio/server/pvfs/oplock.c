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
    PIRP pIrp,
    PVOID pCancelContext
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

    ntError = STATUS_PENDING;

cleanup:
    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

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
    PIRP pIrp = NULL;
    PPVFS_CCB pCcb = NULL;
    PPVFS_FCB pFcb = NULL;
    PPVFS_OPLOCK_PENDING_OPERATION pPendingOp = NULL;
    PVOID pData = NULL;
    BOOLEAN bLocked = FALSE;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrpContext->pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (PVFS_IS_DIR(pCcb)) {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (!pCcb->bOplockBreakInProgress) {
        ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Process pending operations */

    pFcb = pCcb->pFcb;

    LWIO_LOCK_MUTEX(bLocked, &pFcb->ControlBlock);

    while (!LwRtlQueueIsEmpty(pFcb->pOplockPendingOpsQueue))
    {
        ntError = LwRtlQueueRemoveItem(
                      pFcb->pOplockPendingOpsQueue,
                      &pData);
        BAIL_ON_NT_STATUS(ntError);

        pPendingOp = (PPVFS_OPLOCK_PENDING_OPERATION)pData;
        pIrp = pPendingOp->pIrpContext->pIrp;

        if (!pPendingOp->pIrpContext->bIsCancelled) {
            ntError = pPendingOp->pfnCompletion(pPendingOp->pCompletionContext);
        } else {
            ntError = STATUS_CANCELLED;
        }

        pIrp->IoStatusBlock.Status = ntError;

        IoIrpComplete(pIrp);
        PvfsFreeIrpContext(&pPendingOp->pIrpContext);

        pPendingOp->pfnFree(&pPendingOp->pCompletionContext);
        PVFS_FREE(&pPendingOp);
    }


cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->ControlBlock);

    if (pCcb) {
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
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
    );

static NTSTATUS
PvfsOplockBreakOnRead(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
    );

static NTSTATUS
PvfsOplockBreakOnWrite(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
    );

static NTSTATUS
PvfsOplockBreakOnLockControl(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
    );

static NTSTATUS
PvfsOplockBreakOnSetFileInformation(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
    );

static BOOLEAN
PvfsOplockIsMine(
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock
    );


NTSTATUS
PvfsOplockBreakIfLocked(
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb,
    IN PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    NTSTATUS ntBreakStatus = STATUS_SUCCESS;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;
    BOOLEAN bFcbLocked = FALSE;
    PPVFS_OPLOCK_RECORD pOplock = NULL;
    PLW_LIST_LINKS pOplockLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;
    ULONG BreakResult = 0;

    if (!PvfsFileIsOplocked(pFcb)) {
        goto cleanup;
    }

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);

    pOplockLink = LwListTraverse(&pFcb->OplockList, NULL);

    while (pOplockLink)
    {
        /* Setup */

        pOplock = LW_STRUCT_FROM_FIELD(
                      pOplockLink,
                      PVFS_OPLOCK_RECORD,
                      Oplocks);
        pIrpCtx = pOplock->pIrpContext;

        /* Do we need to break? */

        switch (pIrpCtx->pIrp->Type)
        {
        case IRP_TYPE_CREATE:
            ntBreakStatus = PvfsOplockBreakOnCreate(
                                pIrpContext,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_READ:
            ntBreakStatus = PvfsOplockBreakOnRead(
                                pIrpContext,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_WRITE:
            ntBreakStatus = PvfsOplockBreakOnWrite(
                                pIrpContext,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_LOCK_CONTROL:
            ntBreakStatus = PvfsOplockBreakOnLockControl(
                                pIrpContext,
                                pCcb,
                                pOplock,
                                &BreakResult);
            break;

        case IRP_TYPE_SET_INFORMATION:
            ntBreakStatus = PvfsOplockBreakOnSetFileInformation(
                                pIrpContext,
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

        /* Broken -- See if we need to defer the calling operation
           and remove the oplock record from the list */

        if (ntBreakStatus != STATUS_SUCCESS) {
            ntError = ntBreakStatus;
        }

        pNextLink = LwListTraverse(&pFcb->OplockList, pOplockLink);
        LwListRemove(pOplockLink);
        pOplockLink = pNextLink;

        PvfsFreeOplockRecord(&pOplock);
    }

    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);

cleanup:
    return ntError;
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

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);

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

        if (pOplock->OplockType == IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2) {
            ntError = STATUS_INVALID_OPLOCK_PROTOCOL;
            BAIL_ON_NT_STATUS(ntError);
        }

        /* Break */

        pOutputBuffer->OplockBreakResult = IO_OPLOCK_BROKEN_TO_NONE;

        IoIrpComplete(pIrpCtx->pIrp);

        PvfsFreeIrpContext(&pIrpCtx);
        PvfsFreeOplockRecord(&pOplock);
    }

    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->ControlBlock);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnCreate(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
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
        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

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
                        pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        IoIrpComplete(pIrpContext->pIrp);
        PvfsFreeIrpContext(&pIrpContext);
    }

cleanup:
    *pBreakResult = BreakResult;

    return ntError;
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnRead(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
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
        LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

        ntError = STATUS_PENDING;
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_NOT_BROKEN;
        break;

    default:
        break;
    }

    if (BreakResult != IO_OPLOCK_NOT_BROKEN)
    {
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        IoIrpComplete(pIrpContext->pIrp);
        PvfsFreeIrpContext(&pIrpContext);
    }

cleanup:
    *pBreakResult = BreakResult;

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnWrite(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
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
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            ntError = STATUS_PENDING;
        }
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;
        break;

    default:
        break;
    }

    if (BreakResult != IO_OPLOCK_NOT_BROKEN)
    {
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        IoIrpComplete(pIrpContext->pIrp);
        PvfsFreeIrpContext(&pIrpContext);
    }

    *pBreakResult = BreakResult;

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnLockControl(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
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
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            ntError = STATUS_PENDING;
        }
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;
        break;

    default:
        break;
    }

    if (BreakResult != IO_OPLOCK_NOT_BROKEN)
    {
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        IoIrpComplete(pIrpContext->pIrp);
        PvfsFreeIrpContext(&pIrpContext);
    }

    *pBreakResult = BreakResult;

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsOplockBreakOnSetFileInformation(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    PPVFS_OPLOCK_RECORD pOplock,
    PULONG pBreakResult
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
            LWIO_UNLOCK_MUTEX(bCcbLocked, &pOplock->pCcb->ControlBlock);

            ntError = STATUS_PENDING;
        }
        break;

    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        BreakResult = IO_OPLOCK_BROKEN_TO_NONE;
        break;

    default:
        break;
    }

    if (BreakResult != IO_OPLOCK_NOT_BROKEN)
    {
        pOutputBuffer = (PIO_FSCTL_OPLOCK_REQUEST_OUTPUT_BUFFER)
                        pIrpContext->pIrp->Args.IoFsControl.OutputBuffer;
        pOutputBuffer->OplockBreakResult = BreakResult;

        IoIrpComplete(pIrpContext->pIrp);
        PvfsFreeIrpContext(&pIrpContext);
    }

    *pBreakResult = BreakResult;

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsCancelOplockRequestIrp(
    PIRP pIrp,
    PVOID pCancelContext
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

    LWIO_LOCK_MUTEX(bFcbControlLocked, &pFcb->ControlBlock);

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
    LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->ControlBlock);

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
    return STATUS_OPLOCK_NOT_GRANTED;
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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
