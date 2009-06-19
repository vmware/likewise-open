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

static VOID
PvfsCancelOplockRequestIrp(
    PIRP pIrp,
    PVOID pCancelContext
    );

static NTSTATUS
PvfsOplockGrant(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    ULONG OplockType
    );


/* File Globals */


/* Code */

/********************************************************
 *******************************************************/

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
    if (InputBufferLength < sizeof(IO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pOplockRequest = (PIO_FSCTL_OPLOCK_REQUEST_INPUT_BUFFER)InputBuffer;

    /* Verify the oplock request type */

    switch(pOplockRequest->OplockRequestType) {
    case IO_OPLOCK_REQUEST_BATCH_OPLOCK:
    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1:
    case IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2:
        break;
    default:
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
        break;
    }

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (PVFS_IS_DIR(pCcb)) {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsOplockGrant(pIrpContext, pCcb, pOplockRequest->OplockRequestType);
    BAIL_ON_NT_STATUS(ntError);

    /* Successful grant so pend the resulit now */

    IoIrpMarkPending(pIrpContext->pIrp,
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

/********************************************************
 *******************************************************/

NTSTATUS
PvfsOplockAckBreak(
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PVOID InputBuffer,
    IN  ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN  ULONG OutputBufferLength
    )
{
    return STATUS_SUCCESS;
}


/********************************************************
 *******************************************************/

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

/********************************************************
 *******************************************************/

static NTSTATUS
PvfsOplockGrant(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    ULONG OplockType
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_OPLOCK_RECORD pNewOplock = NULL;
    PPVFS_CCB_LIST_NODE pCursor = NULL;
    PPVFS_FCB pFcb = NULL;
    BOOLEAN bFcbReadLocked = FALSE;

    BAIL_ON_INVALID_PTR(pCcb->pFcb, ntError);

    /* Case #1 - We have a registered oplock on the file */

    if (pCcb->pFcb->pOplockList) {
        /* We only support Batch/Exclusive so just fail the
           request for now.

           Technically, a batch/level1 oplock request will
           break a level2 oplock. */

        ntError = STATUS_OPLOCK_NOT_GRANTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Case #2 - We have more than one open file handle on
       this object so we have to check the perms and possible
       sharing flags */

    /* Read lock so no one can add a CCB to the list */

    ntError = STATUS_SUCCESS;
    pFcb = pCcb->pFcb;

    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pFcb->rwLock);

    for (pCursor = PvfsNextCCBFromList(pFcb, pCursor);
         pCursor;
         pCursor = PvfsNextCCBFromList(pFcb, pCursor))
    {
        /* Be stupid for now and fail if there are any
           other open handles on this object */

        if (pCursor->pCcb != pCcb) {
            ntError = STATUS_OPLOCK_NOT_GRANTED;
            break;
        }
    }

    LWIO_UNLOCK_RWMUTEX(bFcbReadLocked, &pFcb->rwLock);

    BAIL_ON_NT_STATUS(ntError);


    /* Case #3 - No current oplock register on the file and no
       conflicting opens.  Grant it */

    ntError = PvfsAllocateMemory((PVOID*)&pNewOplock,
                                 sizeof(PVFS_OPLOCK_RECORD));
    BAIL_ON_NT_STATUS(ntError);

    pNewOplock->OplockType = OplockType;
    pNewOplock->pCcb = pCcb;
    pNewOplock->pIrpContext = pIrpContext;

    pCcb->pFcb->pOplockList = pNewOplock;
    pNewOplock = NULL;

    ntError = STATUS_SUCCESS;

cleanup:
    PVFS_FREE(&pNewOplock);

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
