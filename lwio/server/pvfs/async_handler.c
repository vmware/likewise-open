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
 *        async_handler.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Async IRP dispatch routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* Code */

/************************************************************
 ***********************************************************/

static VOID
PvfsCancelIrp(
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

/************************************************************
 ***********************************************************/

static NTSTATUS
PvfsPendIrp(
    PPVFS_IRP_CONTEXT pIrpCtx
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pIrpCtx, ntError);

    ntError = PvfsAddWorkItem(gpPvfsIoWorkQueue, (PVOID)pIrpCtx);
    BAIL_ON_NT_STATUS(ntError);

    IoIrpMarkPending(pIrpCtx->pIrp, PvfsCancelIrp, pIrpCtx);

    ntError = STATUS_PENDING;

cleanup:

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncCreate(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* Save the pvfs callback and pend the Irp */

    pIrpContext->pfnWorkCallback = PvfsCreate;

    ntError = PvfsPendIrp(pIrpContext);

    return ntError;
}

/************************************************************
 ***********************************************************/

/************************************************************
 ***********************************************************/

static VOID
PvfsCancelLockControlIrp(
    PIRP pIrp,
    PVOID pCancelContext
    )
{
    PPVFS_IRP_CONTEXT pIrpCtx = (PPVFS_IRP_CONTEXT)pCancelContext;
    BOOLEAN bIsLocked = FALSE;

    LWIO_LOCK_MUTEX(bIsLocked, &pIrpCtx->Mutex);

    pIrpCtx->bIsCancelled = TRUE;

    /* Complete a LOCK request (since it is not on the general
       work queue),but allow the PENDING_LOCK to stay
       in the FCB's queue.  It will be released the next time we
       do pending lock processing */

    if (pIrpCtx->pIrp->Args.LockControl.LockControl == IO_LOCK_CONTROL_LOCK)
    {
        pIrp->IoStatusBlock.Status = STATUS_CANCELLED;
        IoIrpComplete(pIrp);

        pIrpCtx->pIrp = NULL;
    }

    LWIO_UNLOCK_MUTEX(bIsLocked, &pIrpCtx->Mutex);

    return;
}

static NTSTATUS
PvfsPendLockControlIrp(
    PPVFS_IRP_CONTEXT pIrpCtx
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    BAIL_ON_INVALID_PTR(pIrpCtx, ntError);

    ntError = PvfsAddWorkItem(gpPvfsIoWorkQueue, (PVOID)pIrpCtx);
    BAIL_ON_NT_STATUS(ntError);

    IoIrpMarkPending(pIrpCtx->pIrp, PvfsCancelLockControlIrp, pIrpCtx);

    ntError = STATUS_PENDING;

cleanup:

    return ntError;

error:
    goto cleanup;
}

NTSTATUS
PvfsAsyncLockControl(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* Save the pvfs callback and pend the Irp */

    pIrpContext->pfnWorkCallback = PvfsLockControl;

    ntError = PvfsPendLockControlIrp(pIrpContext);

    return ntError;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncRead(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* Save the pvfs callback and pend the Irp */

    pIrpContext->pfnWorkCallback = PvfsRead;

    ntError = PvfsPendIrp(pIrpContext);

    return ntError;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncWrite(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* Save the pvfs callback and pend the Irp */

    pIrpContext->pfnWorkCallback = PvfsWrite;

    ntError = PvfsPendIrp(pIrpContext);

    return ntError;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncFlushBuffers(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* Save the pvfs callback and pend the Irp */

    pIrpContext->pfnWorkCallback = PvfsFlushBuffers;

    ntError = PvfsPendIrp(pIrpContext);

    return ntError;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
