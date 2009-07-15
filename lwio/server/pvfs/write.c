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


NTSTATUS
PvfsWrite(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PVOID pBuffer = pIrp->Args.ReadWrite.Buffer;
    PPVFS_CCB pCcb = NULL;
    PPVFS_PENDING_WRITE pWriteCtx = NULL;

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

    ntError = PvfsAccessCheckAnyFileHandle(pCcb, FILE_WRITE_DATA|FILE_APPEND_DATA);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCreateWriteContext(&pWriteCtx, pIrpContext, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsOplockBreakIfLocked(pIrpContext, pCcb, pCcb->pFcb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsWriteFileWithContext(pWriteCtx);
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
PvfsWriteFileWithContext(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_WRITE pReadCtx = (PPVFS_PENDING_WRITE)pContext;
    PIRP pIrp = pReadCtx->pIrpContext->pIrp;
    PPVFS_CCB pCcb = pReadCtx->pCcb;
    PVOID pBuffer = pIrp->Args.ReadWrite.Buffer;
    ULONG bufLen = pIrp->Args.ReadWrite.Length;
    ULONG Key = pIrp->Args.ReadWrite.Key ? *pIrp->Args.ReadWrite.Key : 0;
    size_t totalBytesWritten = 0;
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

    while (totalBytesWritten < bufLen)
    {
        ULONG bytesWritten = 0;

        ntError = PvfsSysWrite(pCcb,
                               pBuffer + totalBytesWritten,
                               bufLen - totalBytesWritten,
                               &Offset,
                               &bytesWritten);
        if (ntError == STATUS_PENDING) {
            continue;
        }
        BAIL_ON_NT_STATUS(ntError);

        totalBytesWritten += bytesWritten;
        Offset += bytesWritten;
    }

    /* Can only get here is the loop was completed successfully */

    pIrp->IoStatusBlock.BytesTransferred = totalBytesWritten;
    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);

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

    pWriteCtx->pIrpContext = pIrpContext;
    pWriteCtx->pCcb = pCcb;

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

