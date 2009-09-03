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
 *        read.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Read Dispatch Routine
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */



/* Code */

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsReadFileWithContext(
    PVOID pContext
    );

static NTSTATUS
PvfsCreateReadContext(
    OUT PPVFS_PENDING_READ *ppReadContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb
    );

static VOID
PvfsFreeReadContext(
    IN OUT PVOID *ppContext
    );


NTSTATUS
PvfsRead(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PVOID pBuffer = pIrp->Args.ReadWrite.Buffer;
    PPVFS_CCB pCcb = NULL;
    PPVFS_PENDING_READ pReadCtx = NULL;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(pBuffer, ntError);

    if (PVFS_IS_DIR(pCcb)) {
        ntError = STATUS_FILE_IS_A_DIRECTORY;
        BAIL_ON_NT_STATUS(ntError);
    }

#if 0xFFFFFFFF > SSIZE_MAX
    if ((size_t)pIrp->Args.ReadWrite.Length > (size_t) SSIZE_MAX) {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }
#endif

    /* Check the right permissions based on the PagingIo flag */

    if (pIrp->Args.ReadWrite.IsPagingIo)
    {
        ntError = PvfsAccessCheckAnyFileHandle(
                      pCcb,
                      FILE_READ_DATA|FILE_EXECUTE);
    }
    else
    {
        ntError = PvfsAccessCheckFileHandle(
                      pCcb,
                      FILE_READ_DATA);
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCreateReadContext(&pReadCtx, pIrpContext, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsOplockBreakIfLocked(pIrpContext, pCcb, pCcb->pFcb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsReadFileWithContext(pReadCtx);
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pReadCtx->pCcb->pFcb,
                      pIrpContext,
                      PvfsReadFileWithContext,
                      PvfsFreeReadContext,
                      (PVOID)pReadCtx);
        if (ntError == STATUS_SUCCESS) {
            pReadCtx = NULL;
            ntError = STATUS_PENDING;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeReadContext((PVOID*)&pReadCtx);

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
PvfsReadFileWithContext(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_READ pReadCtx = (PPVFS_PENDING_READ)pContext;
    PIRP pIrp = pReadCtx->pIrpContext->pIrp;
    PPVFS_CCB pCcb = pReadCtx->pCcb;
    PVOID pBuffer = pIrp->Args.ReadWrite.Buffer;
    ULONG bufLen = pIrp->Args.ReadWrite.Length;
    ULONG Key = pIrp->Args.ReadWrite.Key ? *pIrp->Args.ReadWrite.Key : 0;
    size_t totalBytesRead = 0;
    LONG64 Offset = 0;
    BOOLEAN bMutexLocked = FALSE;

    /* Enter critical region - ReadFile() needs to fill
       the buffer atomically while it may take several read()
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

    ntError = PvfsCheckLockedRegion(
                  pCcb,
                  PVFS_OPERATION_READ,
                  Key,
                  Offset,
                  bufLen);
    BAIL_ON_NT_STATUS(ntError);

    while (totalBytesRead < bufLen)
    {
        ULONG bytesRead = 0;

        ntError = PvfsSysRead(pCcb,
                              pBuffer + totalBytesRead,
                              bufLen - totalBytesRead,
                              &Offset,
                              &bytesRead);
        if (ntError == STATUS_PENDING) {
            continue;
        }
        BAIL_ON_NT_STATUS(ntError);

        /* Check for EOF */
        if (bytesRead == 0) {
            break;
        }

        totalBytesRead += bytesRead;
        Offset += bytesRead;
    }

    /* Can only get here is the loop was completed successfully */

    pIrp->IoStatusBlock.BytesTransferred = totalBytesRead;

    ntError = (totalBytesRead > 0) ?
        STATUS_SUCCESS :
        STATUS_END_OF_FILE;

cleanup:
    LWIO_UNLOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsCreateReadContext(
    OUT PPVFS_PENDING_READ *ppReadContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_READ pReadCtx = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pReadCtx,
                  sizeof(PVFS_PENDING_READ));
    BAIL_ON_NT_STATUS(ntError);

    pReadCtx->pIrpContext = pIrpContext;
    pReadCtx->pCcb = pCcb;

    *ppReadContext = pReadCtx;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsFreeReadContext(
    IN OUT PVOID *ppContext
    )
{
    if (!ppContext || !*ppContext) {
        return;
    }

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
