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
 *        irpctx.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Pvfs IRP Context routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/***********************************************************************
 **********************************************************************/

static
VOID
PvfsFreeIrpContext(
	PPVFS_IRP_CONTEXT *ppIrpContext
    )
{
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    if (ppIrpContext && *ppIrpContext)
    {
        pIrpCtx = *ppIrpContext;

        if (pIrpCtx->pIrp &&
            PvfsIrpContextCheckFlag(pIrpCtx, PVFS_IRP_CTX_FLAG_PENDED))
        {
            pIrpCtx->pIrp->IoStatusBlock.Status = STATUS_FILE_CLOSED;
            PvfsAsyncIrpComplete(pIrpCtx);
        }

        if (pIrpCtx->pScb)
        {
            PvfsReleaseSCB(&pIrpCtx->pScb);
        }

        PVFS_FREE(ppIrpContext);

        InterlockedDecrement(&gPvfsIrpContextCount);
    }
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsAllocateIrpContext(
	PPVFS_IRP_CONTEXT *ppIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpContext = NULL;

    *ppIrpContext = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pIrpContext,
                  sizeof(PVFS_IRP_CONTEXT),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    pIrpContext->RefCount = 1;

    pIrpContext->Flags = PVFS_IRP_CTX_FLAG_NONE;
    pIrpContext->QueueType = PVFS_QUEUE_TYPE_NONE;
    pIrpContext->pScb = NULL;
    pIrpContext->Callback = NULL;

    pIrpContext->pIrp = pIrp;

    *ppIrpContext = pIrpContext;

    InterlockedIncrement(&gPvfsIrpContextCount);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/***********************************************************************
 ***********************************************************************/

BOOLEAN
PvfsIrpContextCheckFlag(
    PPVFS_IRP_CONTEXT pIrpContext,
    USHORT BitToCheck
    )
{
    BOOLEAN bLocked = FALSE;
    BOOLEAN bIsSet = FALSE;

    LWIO_LOCK_MUTEX(bLocked, &gPvfsIrpContextMutex);
    bIsSet = IsSetFlag(pIrpContext->Flags, BitToCheck);
    LWIO_UNLOCK_MUTEX(bLocked, &gPvfsIrpContextMutex);

    return bIsSet;
}

/***********************************************************************
 ***********************************************************************/

VOID
PvfsIrpContextSetFlag(
    PPVFS_IRP_CONTEXT pIrpContext,
    USHORT BitToSet
    )
{
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_MUTEX(bLocked, &gPvfsIrpContextMutex);
    SetFlag(pIrpContext->Flags, BitToSet);
    LWIO_UNLOCK_MUTEX(bLocked, &gPvfsIrpContextMutex);

    return;
}


/***********************************************************************
 ***********************************************************************/

VOID
PvfsIrpContextClearFlag(
    PPVFS_IRP_CONTEXT pIrpContext,
    USHORT BitToClear
    )
{
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_MUTEX(bLocked, &gPvfsIrpContextMutex);
    ClearFlag(pIrpContext->Flags, BitToClear);
    LWIO_UNLOCK_MUTEX(bLocked, &gPvfsIrpContextMutex);

    return;
}

/***********************************************************************
 ***********************************************************************/

BOOLEAN
PvfsIrpContextMarkIfNotSetFlag(
    PPVFS_IRP_CONTEXT pIrpContext,
    USHORT BitToCheck,
    USHORT BitToSet
    )
{
    USHORT SetFlag = 0;

    SetFlag = PvfsIrpContextConditionalSetFlag(
                  pIrpContext,
                  BitToCheck,
                  0,
                  BitToSet);

    return (SetFlag == BitToSet) ? TRUE : FALSE;
}


/***********************************************************************
 ***********************************************************************/

BOOLEAN
PvfsIrpContextMarkIfSetFlag(
    PPVFS_IRP_CONTEXT pIrpContext,
    USHORT BitToCheck,
    USHORT BitToSet
    )
{
    USHORT SetFlag = 0;

    SetFlag = PvfsIrpContextConditionalSetFlag(
                  pIrpContext,
                  BitToCheck,
                  BitToSet,
                  0);

    return (SetFlag == BitToSet) ? TRUE : FALSE;
}


/***********************************************************************
 ***********************************************************************/

USHORT
PvfsIrpContextConditionalSetFlag(
    PPVFS_IRP_CONTEXT pIrpContext,
    USHORT BitToCheck,
    USHORT BitToSetOnTrue,
    USHORT BitToSetOnFalse
    )
{
    BOOLEAN bLocked = FALSE;
    USHORT FlagWasSet = 0;

    LWIO_LOCK_MUTEX(bLocked, &gPvfsIrpContextMutex);
    if (IsSetFlag(pIrpContext->Flags, BitToCheck))
    {
        SetFlag(pIrpContext->Flags, BitToSetOnTrue);
        FlagWasSet = BitToSetOnTrue;
    }
    else
    {
        SetFlag(pIrpContext->Flags, BitToSetOnFalse);
        FlagWasSet = BitToSetOnFalse;
    }
    LWIO_UNLOCK_MUTEX(bLocked, &gPvfsIrpContextMutex);

    return FlagWasSet;
}

/***********************************************************************
 ***********************************************************************/

VOID
PvfsIrpMarkPending(
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PIO_IRP_CALLBACK CancelCallback,
    IN OPTIONAL PVOID CancelCallbackContext
    )
{
    BOOLEAN bPendIrp = FALSE;

    bPendIrp = PvfsIrpContextMarkIfNotSetFlag(
                   pIrpContext,
                   PVFS_IRP_CTX_FLAG_PENDED,
                   PVFS_IRP_CTX_FLAG_PENDED);

    if (bPendIrp)
    {
        IoIrpMarkPending(
            pIrpContext->pIrp,
            CancelCallback,
            CancelCallbackContext);
    }
}

/***********************************************************************
 ***********************************************************************/

VOID
PvfsAsyncIrpComplete(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    BOOLEAN bComplete = FALSE;

    bComplete = PvfsIrpContextMarkIfSetFlag(
                    pIrpContext,
                    PVFS_IRP_CTX_FLAG_PENDED,
                    PVFS_IRP_CTX_FLAG_COMPLETE);
    if (bComplete)
    {
        IoIrpComplete(pIrpContext->pIrp);
        pIrpContext->pIrp = NULL;
    }
}


/***********************************************************************
 ***********************************************************************/

PPVFS_IRP_CONTEXT
PvfsReferenceIrpContext(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    InterlockedIncrement(&pIrpContext->RefCount);

    return pIrpContext;
}

/***********************************************************************
 ***********************************************************************/

VOID
PvfsReleaseIrpContext(
    PPVFS_IRP_CONTEXT *ppIrpContext
    )
{
    PPVFS_IRP_CONTEXT pIrpContext = *ppIrpContext;

    if (InterlockedDecrement(&pIrpContext->RefCount) == 0)
    {
        PvfsFreeIrpContext(&pIrpContext);
    }

    *ppIrpContext = NULL;
}

////////////////////////////////////////////////////////////////////////

static
VOID
PvfsProcessIrpContext(
    PVOID
    );

NTSTATUS
PvfsScheduleIrpContext(
    IN PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    // Take a reference for the work queue
    pIrpCtx = PvfsReferenceIrpContext(pIrpContext);

    pIrpCtx->QueueType = PVFS_QUEUE_TYPE_IO;
    PvfsIrpMarkPending(pIrpCtx, PvfsQueueCancelIrp, pIrpCtx);

    ntError = LwRtlQueueWorkItem(
                  gPvfsDriverState.ThreadPool,
                  PvfsProcessIrpContext,
                  pIrpCtx,
                  0);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_PENDING;

error:
    if (ntError != STATUS_PENDING)
    {
        pIrpCtx->pIrp->IoStatusBlock.Status = ntError;
        PvfsAsyncIrpComplete(pIrpCtx);

        if (pIrpCtx)
        {
            PvfsReleaseIrpContext(&pIrpCtx);
        }
    }

    return ntError;
}

////////////////////////////////////////////////////////////////////////

static
VOID
PvfsProcessIrpContext(
    IN PVOID Context
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;
    BOOLEAN bActive = FALSE;

    // We were given a reference to the IrpContext when it was queued

    pIrpCtx = (PPVFS_IRP_CONTEXT)Context;

    PvfsQueueCancelIrpIfRequested(pIrpCtx);

    bActive = PvfsIrpContextMarkIfNotSetFlag(
                  pIrpCtx,
                  PVFS_IRP_CTX_FLAG_CANCELLED,
                  PVFS_IRP_CTX_FLAG_ACTIVE);

    if (bActive)
    {
        ntError = pIrpCtx->Callback(pIrpCtx);
    }
    else
    {
        ntError = STATUS_CANCELLED;
    }

    if (ntError != STATUS_PENDING)
    {
        pIrpCtx->pIrp->IoStatusBlock.Status = ntError;

        PvfsAsyncIrpComplete(pIrpCtx);
    }

    PvfsReleaseIrpContext(&pIrpCtx);

    return;
}
