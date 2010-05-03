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
 *        write.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *       Write Dispatch Routine
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsWriteInternal(
    PPVFS_IRP_CONTEXT  pIrpContext
    );

static NTSTATUS
PvfsPrepareZctWrite(
    IN PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsPreCheckWrite(
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb
    );

static NTSTATUS
PvfsWriteFileWithContext(
    PVOID pContext
    );

static NTSTATUS
PvfsCreateWriteContext(
    OUT PPVFS_PENDING_WRITE *ppWriteCtx,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb
    );

static VOID
PvfsFreeWriteContext(
    IN OUT PVOID *ppContext
    );


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsWrite(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch (pIrpContext->pIrp->Args.ReadWrite.ZctOperation)
    {
    case IRP_ZCT_OPERATION_NONE:
        LWIO_ASSERT(!pIrpContext->pIrp->Args.ReadWrite.Length || pIrpContext->pIrp->Args.ReadWrite.Buffer);
        LWIO_ASSERT(!pIrpContext->pIrp->Args.ReadWrite.ZctCompletionContext);
        ntError = PvfsWriteInternal(pIrpContext);
        break;
    case IRP_ZCT_OPERATION_PREPARE:
        LWIO_ASSERT(pIrpContext->pIrp->Args.ReadWrite.Zct);
        LWIO_ASSERT(!pIrpContext->pIrp->Args.ReadWrite.ZctCompletionContext);
        ntError = PvfsPrepareZctWrite(pIrpContext);
        break;
    case IRP_ZCT_OPERATION_COMPLETE:
        LWIO_ASSERT(pIrpContext->pIrp->Args.ReadWrite.ZctCompletionContext);
        ntError = PvfsWriteInternal(pIrpContext);
        break;
    default:
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsWriteInternal(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PPVFS_PENDING_WRITE pWriteCtx = NULL;
    PPVFS_ZCT_CONTEXT pZctContext = NULL;

    if (pIrp->Args.ReadWrite.ZctOperation == IRP_ZCT_OPERATION_COMPLETE)
    {
        pZctContext = (PPVFS_ZCT_CONTEXT) pIrp->Args.ReadWrite.ZctCompletionContext;
        LwListRemove(&pZctContext->CcbLinks);
    }

    ntError = PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (!pZctContext)
    {
        ntError = PvfsPreCheckWrite(pIrpContext, pCcb);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsCreateWriteContext(&pWriteCtx, pIrpContext, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    pWriteCtx->pZctContext = pZctContext;
    pZctContext = NULL;

    ntError = PvfsOplockBreakIfLocked(pIrpContext, pCcb, pCcb->pFcb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsWriteFileWithContext(pWriteCtx);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pWriteCtx->pCcb->pFcb,
                      pIrpContext,
                      pWriteCtx->pCcb,
                      PvfsWriteFileWithContext,
                      PvfsFreeWriteContext,
                      (PVOID)pWriteCtx);
        if (ntError == STATUS_SUCCESS) {
            pWriteCtx = NULL;
            ntError = STATUS_PENDING;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pWriteCtx->pCcb->pFcb,
                      pIrpContext,
                      PvfsWriteFileWithContext,
                      PvfsFreeWriteContext,
                      (PVOID)pWriteCtx);
        if (ntError == STATUS_SUCCESS) {
            pWriteCtx = NULL;
            ntError = STATUS_PENDING;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeWriteContext((PVOID*)&pWriteCtx);
    PvfsFreeZctContext(&pZctContext);

    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsPrepareZctWrite(
    IN PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PPVFS_ZCT_CONTEXT pZctContext = NULL;
    BOOLEAN bMutexLocked = FALSE;

    ntError = PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPreCheckWrite(pIrpContext, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCreateZctContext(
                    &pZctContext,
                    pIrpContext,
                    pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAddZctWriteEntries(pIrp->Args.ReadWrite.Zct,
                                     pZctContext,
                                     pIrp->Args.ReadWrite.Length);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);

    /* Save the ZCT context for complete */
    pIrp->Args.ReadWrite.ZctCompletionContext = pZctContext;

    /* Cannot fail */
    ntError = PvfsListAddTail(
                  pCcb->pZctContextList,
                  &pZctContext->CcbLinks);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_UNLOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);

    ntError = STATUS_SUCCESS;

cleanup:
    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    PvfsFreeZctContext(&pZctContext);

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsPreCheckWrite(
    IN PPVFS_IRP_CONTEXT pIrpContext,
    IN PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* Sanity checks */

    if (PVFS_IS_DIR(pCcb)) {
        ntError = STATUS_FILE_IS_A_DIRECTORY;
        BAIL_ON_NT_STATUS(ntError);
    }

#if 0xFFFFFFFF > SSIZE_MAX
    if ((size_t)pIrpContext->pIrp->Args.ReadWrite.Length > (size_t) SSIZE_MAX) {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }
#endif

    ntError = PvfsAccessCheckAnyFileHandle(pCcb, FILE_WRITE_DATA|FILE_APPEND_DATA);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsWriteFileWithContext(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_WRITE pWriteCtx = (PPVFS_PENDING_WRITE)pContext;
    PIRP pIrp = pWriteCtx->pIrpContext->pIrp;
    PPVFS_CCB pCcb = pWriteCtx->pCcb;
    ULONG bufLen = pWriteCtx->pZctContext ? pIrp->Args.ReadWrite.ZctWriteBytesTransferred : pIrp->Args.ReadWrite.Length;
    ULONG Key = pIrp->Args.ReadWrite.Key ? *pIrp->Args.ReadWrite.Key : 0;
    ULONG totalBytesWritten = 0;
    LONG64 Offset = 0;
    BOOLEAN bMutexLocked = FALSE;

    /* Enter critical region - WriteFile() needs to fill
       the buffer atomically while it may take several write()
       calls */

    LWIO_LOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);

    if (pIrp->Args.ReadWrite.ByteOffset) {
        Offset = *pIrp->Args.ReadWrite.ByteOffset;
    } else {
        off_t offset = 0;

        ntError = PvfsSysLseek(pCcb->fd, 0, SEEK_CUR, &offset);
        BAIL_ON_NT_STATUS(ntError);

        Offset = offset;
    }

    ntError = PvfsCheckLockedRegion(pCcb, PVFS_OPERATION_WRITE,
                                    Key, Offset, bufLen);
    BAIL_ON_NT_STATUS(ntError);

    if (pWriteCtx->pZctContext)
    {
        ntError = PvfsDoZctWriteIo(pWriteCtx->pZctContext,
                                   bufLen,
                                   Offset,
                                   &totalBytesWritten);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        ntError = PvfsDoWriteIo(pCcb,
                                pIrp->Args.ReadWrite.Buffer,
                                bufLen,
                                Offset,
                                &totalBytesWritten);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Can only get here is the I/O was completed successfully */

    pIrp->IoStatusBlock.BytesTransferred = totalBytesWritten;

    if (totalBytesWritten > 0)
    {
        pCcb->ChangeEvent |= FILE_NOTIFY_CHANGE_LAST_WRITE;

        if ((Offset + totalBytesWritten) > pCcb->FileSize)
        {
            pCcb->FileSize = Offset + totalBytesWritten;
            pCcb->ChangeEvent |= FILE_NOTIFY_CHANGE_SIZE;
        }
    }

cleanup:
    LWIO_UNLOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsDoWriteIo(
    IN PPVFS_CCB pCcb,
    OUT PVOID pBuffer,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesWritten
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    LONG64 currentOffset = Offset;
    size_t totalBytesWritten = 0;

    while (totalBytesWritten < BufferLength)
    {
        ULONG bytesWritten = 0;

        ntError = PvfsSysWrite(pCcb,
                               pBuffer + totalBytesWritten,
                               BufferLength - totalBytesWritten,
                               &currentOffset,
                               &bytesWritten);
        if (ntError == STATUS_MORE_PROCESSING_REQUIRED)
        {
            continue;
        }
        BAIL_ON_NT_STATUS(ntError);

        totalBytesWritten += bytesWritten;
        currentOffset += bytesWritten;
    }

    /* Can only get here is the loop was completed successfully */

    *pBytesWritten = totalBytesWritten;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsCreateWriteContext(
    OUT PPVFS_PENDING_WRITE *ppWriteContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_WRITE pWriteCtx = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pWriteCtx,
                  sizeof(PVFS_PENDING_WRITE));
    BAIL_ON_NT_STATUS(ntError);

    pWriteCtx->pIrpContext = PvfsReferenceIrpContext(pIrpContext);
    pWriteCtx->pCcb = PvfsReferenceCCB(pCcb);

    *ppWriteContext = pWriteCtx;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsFreeWriteContext(
    IN OUT PVOID *ppContext
    )
{
    PPVFS_PENDING_WRITE pWriteCtx = NULL;

    if (ppContext && *ppContext)
    {
        pWriteCtx = (PPVFS_PENDING_WRITE)(*ppContext);

        if (pWriteCtx->pZctContext)
        {
            PvfsFreeZctContext(&pWriteCtx->pZctContext);
        }

        if (pWriteCtx->pIrpContext)
        {
            PvfsReleaseIrpContext(&pWriteCtx->pIrpContext);
        }

        if (pWriteCtx->pCcb)
        {
            PvfsReleaseCCB(pWriteCtx->pCcb);
        }

        PVFS_FREE(ppContext);
    }

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

