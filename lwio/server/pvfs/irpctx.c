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

        if (pIrpCtx->pFcb)
        {
            PvfsReleaseFCB(&pIrpCtx->pFcb);
        }

        pthread_mutex_destroy(&pIrpCtx->Mutex);

        PVFS_FREE(ppIrpContext);

#ifdef _PVFS_DEVELOPER_DEBUG
        InterlockedDecrement(&gPvfsIrpContextCount);
#endif
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

    ntError = PvfsAllocateMemory((PVOID*)&pIrpContext,
                                 sizeof(PVFS_IRP_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);

    pthread_mutex_init(&pIrpContext->Mutex, NULL);
    pIrpContext->RefCount = 1;

    pIrpContext->Flags = PVFS_IRP_CTX_FLAG_NONE;
    pIrpContext->QueueType = PVFS_QUEUE_TYPE_NONE;
    pIrpContext->pFcb = NULL;

    pIrpContext->pIrp = pIrp;

    *ppIrpContext = pIrpContext;

#ifdef _PVFS_DEVELOPER_DEBUG
    InterlockedIncrement(&gPvfsIrpContextCount);
#endif

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

    LWIO_LOCK_MUTEX(bLocked, &pIrpContext->Mutex);
    bIsSet = IsSetFlag(pIrpContext->Flags, BitToCheck);
    LWIO_UNLOCK_MUTEX(bLocked, &pIrpContext->Mutex);

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

    LWIO_LOCK_MUTEX(bLocked, &pIrpContext->Mutex);
    SetFlag(pIrpContext->Flags, BitToSet);
    LWIO_UNLOCK_MUTEX(bLocked, &pIrpContext->Mutex);

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

    LWIO_LOCK_MUTEX(bLocked, &pIrpContext->Mutex);
    ClearFlag(pIrpContext->Flags, BitToClear);
    LWIO_UNLOCK_MUTEX(bLocked, &pIrpContext->Mutex);

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

    LWIO_LOCK_MUTEX(bLocked, &pIrpContext->Mutex);
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
    LWIO_UNLOCK_MUTEX(bLocked, &pIrpContext->Mutex);

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



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
