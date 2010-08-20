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

/***********************************************************************
 **********************************************************************/

VOID
PvfsQueueCancelIrp(
    PIRP pIrp,
    PVOID pCancelContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpContext = (PPVFS_IRP_CONTEXT)pCancelContext;
    USHORT SetFlag = 0;

    PvfsReferenceIrpContext(pIrpContext);

    SetFlag = PvfsIrpContextConditionalSetFlag(
                  pIrpContext,
                  PVFS_IRP_CTX_FLAG_ACTIVE,
                  PVFS_IRP_CTX_FLAG_REQUEST_CANCEL,
                  PVFS_IRP_CTX_FLAG_CANCELLED);

    if (IsSetFlag(SetFlag, PVFS_IRP_CTX_FLAG_CANCELLED))
    {
        switch(pIrpContext->QueueType)
        {
        case PVFS_QUEUE_TYPE_IO:
            /* Nothing to do.  Let the general Io Work queue processing
               handle it */
            ntError = STATUS_SUCCESS;
            break;

        case PVFS_QUEUE_TYPE_OPLOCK:
            ntError = PvfsScheduleCancelOplock(pIrpContext);
            break;

        case PVFS_QUEUE_TYPE_PENDING_OPLOCK_BREAK:
            ntError = PvfsScheduleCancelPendingOp(pIrpContext);
            break;

        case PVFS_QUEUE_TYPE_PENDING_LOCK:
            ntError = PvfsScheduleCancelLock(pIrpContext);
            break;

        case PVFS_QUEUE_TYPE_NOTIFY:
            ntError = PvfsScheduleCancelNotify(pIrpContext);
            break;

        default:
            /* Should never be reachable */
            ntError = STATUS_INTERNAL_ERROR;
            break;
        }
    }

    PvfsReleaseIrpContext(&pIrpContext);

    return;
}



/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsQueueCancelIrpIfRequested(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    USHORT SetFlag = 0;

    /* First check to see if we've been requested to cancel the IRP */

    SetFlag = PvfsIrpContextConditionalSetFlag(
                  pIrpContext,
                  PVFS_IRP_CTX_FLAG_REQUEST_CANCEL,
                  PVFS_IRP_CTX_FLAG_CANCELLED,
                  0);

    if (IsSetFlag(SetFlag, PVFS_IRP_CTX_FLAG_CANCELLED))
    {
        PvfsIrpContextClearFlag(pIrpContext, PVFS_IRP_CTX_FLAG_ACTIVE);

        PvfsQueueCancelIrp(pIrpContext->pIrp, pIrpContext);
        ntError = STATUS_CANCELLED;
    }

    return ntError;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsScheduleIoWorkItem(
    IN PPVFS_WORK_CONTEXT pWorkContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    if (IsSetFlag(pWorkContext->Flags, PVFS_WORK_CTX_FLAG_IRP_CONTEXT))
    {
        pIrpCtx = (PPVFS_IRP_CONTEXT)pWorkContext->pContext;
        pIrpCtx->QueueType = PVFS_QUEUE_TYPE_IO;

        PvfsIrpMarkPending(pIrpCtx, PvfsQueueCancelIrp, pIrpCtx);
    }

    ntError = PvfsAddWorkItem(gpPvfsIoWorkQueue, (PVOID)pWorkContext);

    if (ntError != STATUS_SUCCESS &&
        IsSetFlag(pWorkContext->Flags, PVFS_WORK_CTX_FLAG_IRP_CONTEXT))
    {
        pIrpCtx->pIrp->IoStatusBlock.Status = ntError;

        PvfsAsyncIrpComplete(pIrpCtx);
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_PENDING;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCreateWorkContext(
    OUT PPVFS_WORK_CONTEXT *ppWorkContext,
    IN  LONG lFlags,
    IN  PVOID pContext,
    IN  PPVFS_WORK_CONTEXT_CALLBACK pfnCompletion,
    IN  PPVFS_WORK_CONTEXT_FREE_CTX pfnFreeContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pWorkCtx,
                  sizeof(PVFS_WORK_CONTEXT),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    pWorkCtx->Flags = lFlags;
    pWorkCtx->pContext = pContext;

    if (IsSetFlag(lFlags, PVFS_WORK_CTX_FLAG_IRP_CONTEXT))
    {
        PvfsReferenceIrpContext((PPVFS_IRP_CONTEXT)pContext);
    }

    pWorkCtx->pfnCompletion = pfnCompletion;
    pWorkCtx->pfnFreeContext = pfnFreeContext;

    *ppWorkContext = pWorkCtx;

    InterlockedIncrement(&gPvfsWorkContextCount);

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
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    if (ppWorkContext && *ppWorkContext)
    {
        pWorkCtx = *ppWorkContext;

        if (pWorkCtx->pContext)
        {
            if (IsSetFlag(pWorkCtx->Flags, PVFS_WORK_CTX_FLAG_IRP_CONTEXT))
            {
                PvfsReleaseIrpContext((PPVFS_IRP_CONTEXT*)&pWorkCtx->pContext);
            }
            else
            {
                if (pWorkCtx->pfnFreeContext)
                {
                    pWorkCtx->pfnFreeContext(&pWorkCtx->pContext);
                }
                else
                {
                    PVFS_FREE(&pWorkCtx->pContext);
                }
            }
        }

        PVFS_FREE(ppWorkContext);

        InterlockedDecrement(&gPvfsWorkContextCount);
    }

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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsCreate,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
PvfsAsyncClose(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsClose,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
PvfsAsyncLockControl(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsLockControl,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsRead,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsWrite,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsFlushBuffers,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsQueryInformationFile,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsSetInformationFile,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsQueryDirInformation,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsQueryVolumeInformation,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsQuerySecurityFile,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
                  TRUE,
                  (PVOID)pIrpContext,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsSetSecurityFile,
                  NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsScheduleIoWorkItem(pWorkCtx);
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
