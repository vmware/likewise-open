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

static NTSTATUS
PvfsPendIrp(
    IN PPVFS_WORK_CONTEXT pWorkContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = pWorkContext->pIrpContext;

    IoIrpMarkPending(pIrpCtx->pIrp, PvfsCancelIrp, pIrpCtx);

    ntError = PvfsAddGlobalWorkItem((PVOID)pWorkContext);
    if (ntError != STATUS_SUCCESS) {
        pIrpCtx->pIrp->IoStatusBlock.Status = ntError;
        IoIrpComplete(pIrpCtx->pIrp);

        PvfsFreeIrpContext(&pIrpCtx);
    }

    /* Always return STATUS_PENDING here */

    return STATUS_PENDING;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCreateWorkContext(
    OUT PPVFS_WORK_CONTEXT *ppWorkContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PVOID pContext,
    IN  PPVFS_WORK_CONTEXT_CALLBACK pfnCompletion,
    IN  PPVFS_WORK_CONTEXT_FREE_CTX pfnFreeContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pWorkCtx,
                  sizeof(PVFS_WORK_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);

    pWorkCtx->pIrpContext = pIrpContext;
    pWorkCtx->pContext = pContext;
    pWorkCtx->pfnCompletion = pfnCompletion;
    pWorkCtx->pfnFreeContext = pfnFreeContext;

    *ppWorkContext = pWorkCtx;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreeWorkContext(
    IN OUT PPVFS_WORK_CONTEXT *ppWorkContext
    )
{
    PVFS_FREE(ppWorkContext);

    return;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncCreate(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsCreate,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:

    goto cleanup;
}

/************************************************************
 ***********************************************************/

static VOID
PvfsCancelLockControlIrp(
    PIRP pIrp,
    PVOID pCancelContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = (PPVFS_IRP_CONTEXT)pCancelContext;
    BOOLEAN bIsLocked = FALSE;

    LWIO_LOCK_MUTEX(bIsLocked, &pIrpCtx->Mutex);

    pIrpCtx->bIsCancelled = TRUE;

    if (pIrpCtx->pPendingLock)
    {
        PPVFS_WORK_CONTEXT pWorkCtx = NULL;

        /* Cancel the pending lock */

        pIrpCtx->pPendingLock->bIsCancelled = TRUE;
        PvfsReleaseCCB(pIrpCtx->pPendingLock->pCcb);
        pIrpCtx->pPendingLock->pCcb = NULL;

        /* Add the canceled IRP_CONTEXT back to the work queue.  If
           this fails, we will fallback to calling IoIrpComplete()
           in PvfsProcessPendingLocks() */

        ntError = PvfsCreateWorkContext(
                      &pWorkCtx,
                      pIrpCtx,
                      NULL,
                      NULL,    /* Cancelled - no completion function */
                      NULL);
        if (ntError == STATUS_SUCCESS)
        {
            ntError = PvfsAddGlobalWorkItem((PVOID)pWorkCtx);
            if (ntError == STATUS_SUCCESS) {
                pIrpCtx->pIrp = NULL;
            } else {
                PvfsFreeWorkContext(&pWorkCtx);
            }
        }
    }

    LWIO_UNLOCK_MUTEX(bIsLocked, &pIrpCtx->Mutex);

    return;
}

static NTSTATUS
PvfsPendLockControlIrp(
    IN PPVFS_WORK_CONTEXT pWorkContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = pWorkContext->pIrpContext;

    IoIrpMarkPending(pIrpCtx->pIrp, PvfsCancelLockControlIrp, pIrpCtx);

    ntError = PvfsAddGlobalWorkItem((PVOID)pWorkContext);
    if (ntError != STATUS_SUCCESS) {
        pIrpCtx->pIrp->IoStatusBlock.Status = ntError;
        IoIrpComplete(pIrpCtx->pIrp);

        PvfsFreeIrpContext(&pIrpCtx);
    }

    /* Always return STATUS_PENDING here */

    return STATUS_PENDING;
}

NTSTATUS
PvfsAsyncLockControl(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsLockControl,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendLockControlIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncRead(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsRead,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncWrite(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsWrite,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncFlushBuffers(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsFlushBuffers,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncQueryInformationFile(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsQueryInformationFile,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncSetInformationFile(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsSetInformationFile,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncQueryDirInformation(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsQueryDirInformation,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncQueryVolumeInformation(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsQueryVolumeInformation,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncQuerySecurityFile(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsQuerySecurityFile,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

    return ntError;

error:
    goto cleanup;
}

/************************************************************
 ***********************************************************/

NTSTATUS
PvfsAsyncSetSecurityFile(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  pIrpContext,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsSetSecurityFile,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPendIrp(pWorkCtx);
    if (ntError == STATUS_PENDING)
    {
        pWorkCtx = NULL;
    }

cleanup:
    PvfsFreeWorkContext(&pWorkCtx);

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
