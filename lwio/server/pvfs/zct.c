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
 *        zct.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Zero Copy Transfer Support Routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

#ifdef HAVE_SPLICE
// On Linux (the only system that implements splice), the pipe buffer
// size is hardcoded to 16 "pipe buffers", each of which is a single
// page.  So, on most systems, this is 64 KB.  Back in 2006, around
// the time when splice was introduced, Linus mentioned the possibility
// of allowing dynamically changing the size of a pipe with ioctl/fcntl.
// However, this has not happened as of 2010.
#define SPLICE_PIPE_BUFFER_SIZE (16 * (4 * 1024))
#endif

/* Forward declarations */

static NTSTATUS
PvfsDoZctMemoryReadIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    OUT PLW_ZCT_VECTOR pZct,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesRead
    );

#ifdef HAVE_SPLICE
static NTSTATUS
PvfsDoZctSpliceReadIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    OUT PLW_ZCT_VECTOR pZct,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesRead
    );
#endif

static NTSTATUS
PvfsDoZctMemoryWriteIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesWritten
    );

#ifdef HAVE_SPLICE
static NTSTATUS
PvfsDoZctSpliceWriteIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesWritten
    );
#endif

/* Code */

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsInitializeZctSupport(
    IN PPVFS_CCB pCcb,
    IN IO_FILE_HANDLE FileHandle
    )
{
    PVFS_ZCT_MODE ZctMode = gPvfsDriverConfig.ZctMode;
    LW_ZCT_ENTRY_MASK zctReadMask = 0;
    LW_ZCT_ENTRY_MASK zctWriteMask = 0;

    if (!PVFS_IS_DIR(pCcb))
    {
        switch (ZctMode)
        {
        case PVFS_ZCT_MODE_MEMORY:
            zctReadMask = zctWriteMask = LW_ZCT_ENTRY_MASK_MEMORY;
            break;
#ifdef HAVE_SPLICE
        case PVFS_ZCT_MODE_SPLICE:
            zctReadMask = zctWriteMask = LW_ZCT_ENTRY_MASK_FD_PIPE;
            break;
#endif
        default:
            break;
        }

        if (zctReadMask || zctWriteMask)
        {
            IoFileSetZctSupportMask(FileHandle, zctReadMask, zctWriteMask);
        }
    }
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCreateZctContext(
    OUT PPVFS_ZCT_CONTEXT *ppZctContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_ZCT_CONTEXT pZctContext = NULL;
    PVFS_ZCT_MODE ZctMode = gPvfsDriverConfig.ZctMode;

    if (IRP_ZCT_OPERATION_PREPARE != pIrpContext->pIrp->Args.ReadWrite.ZctOperation)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    switch (ZctMode)
    {
    case PVFS_ZCT_MODE_MEMORY:
#ifdef HAVE_SPLICE
    case PVFS_ZCT_MODE_SPLICE:
#endif
        break;
    default:
        /* So the I/O does a backoff to non-ZCT */
        ntError = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pZctContext,
                  sizeof(*pZctContext),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    pZctContext->Mode = ZctMode;
    switch (pZctContext->Mode)
    {
    case PVFS_ZCT_MODE_MEMORY:
        ntError = PvfsAllocateMemory(
                      OUT_PPVOID(&pZctContext->pBuffer),
                      pIrpContext->pIrp->Args.ReadWrite.Length,
                      TRUE);
        BAIL_ON_NT_STATUS(ntError);
        break;
#ifdef HAVE_SPLICE
    case PVFS_ZCT_MODE_SPLICE:
        if (pIrpContext->pIrp->Args.ReadWrite.Length > SPLICE_PIPE_BUFFER_SIZE)
        {
            ntError = STATUS_NOT_SUPPORTED;
            BAIL_ON_NT_STATUS(ntError);
        }

        // TODO: Convert to using pipe2 (Linux).
        ntError = PvfsSysPipe(pZctContext->PipeFds);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysSetNonBlocking(pZctContext->PipeFds[0]);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysSetNonBlocking(pZctContext->PipeFds[1]);
        BAIL_ON_NT_STATUS(ntError);
        break;
#endif
    default:
        // can never happen
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    pZctContext->pCcb = PvfsReferenceCCB(pCcb);

    ntError = STATUS_SUCCESS;

cleanup:
    *ppZctContext = pZctContext;

    return ntError;

error:
    /* Backoff to non-ZCT because could not allocate resources */
    ntError = STATUS_NOT_SUPPORTED;

    PvfsFreeZctContext(&pZctContext);
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreeZctContext(
    IN OUT PPVFS_ZCT_CONTEXT *ppZctContext
    )
{
    PPVFS_ZCT_CONTEXT pZctContext = *ppZctContext;

    // pZctContext->pCcb must be locked if the context
    // is in the CCB list of ZCT contexts.

    if (pZctContext)
    {
        switch (pZctContext->Mode)
        {
        case PVFS_ZCT_MODE_MEMORY:
            PVFS_FREE(&pZctContext->pBuffer);
            break;
#ifdef HAVE_SPLICE
        case PVFS_ZCT_MODE_SPLICE:
            if (pZctContext->PipeFds[1] >= 0)
            {
                PvfsSysClose(pZctContext->PipeFds[1]);
            }

            if (pZctContext->PipeFds[0] >= 0)
            {
                PvfsSysClose(pZctContext->PipeFds[0]);
            }
            break;
#endif
        default:
            // can never happen
            break;
        }
        if (pZctContext->pCcb)
        {
            PvfsReleaseCCB(pZctContext->pCcb);
            if (pZctContext->CcbLinks.Next)
            {
                PvfsListRemoveItem(
                    pZctContext->pCcb->pZctContextList,
                    &pZctContext->CcbLinks);
            }
        }
        PVFS_FREE(ppZctContext);
    }
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsDoZctReadIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    IN OUT PLW_ZCT_VECTOR pZct,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesRead
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch (pZctContext->Mode)
    {
    case PVFS_ZCT_MODE_MEMORY:
        ntError = PvfsDoZctMemoryReadIo(pZctContext,
                                        pZct,
                                        BufferLength,
                                        Offset,
                                        pBytesRead);
        break;
#ifdef HAVE_SPLICE
    case PVFS_ZCT_MODE_SPLICE:
        ntError = PvfsDoZctSpliceReadIo(pZctContext,
                                        pZct,
                                        BufferLength,
                                        Offset,
                                        pBytesRead);
        break;
#endif
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
PvfsDoZctMemoryReadIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    OUT PLW_ZCT_VECTOR pZct,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesRead
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    ULONG totalBytesRead = 0;
    LW_ZCT_ENTRY entry = { 0 };

    ntError = PvfsDoReadIo(pZctContext->pCcb,
                           pZctContext->pBuffer,
                           BufferLength,
                           Offset,
                           &totalBytesRead);
    BAIL_ON_NT_STATUS(ntError);

    if (totalBytesRead > 0)
    {
        entry.Type = LW_ZCT_ENTRY_TYPE_MEMORY;
        entry.Length = totalBytesRead;
        entry.Data.Memory.Buffer = pZctContext->pBuffer;

        ntError = LwZctAppend(pZct, &entry, 1);
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    *pBytesRead = totalBytesRead;

    return ntError;

error:
    totalBytesRead = 0;

    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

#ifdef HAVE_SPLICE
static NTSTATUS
PvfsDoZctSpliceReadIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    OUT PLW_ZCT_VECTOR pZct,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesRead
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    LONG64 currentOffset = Offset;
    ULONG totalBytesRead = 0;
    LW_ZCT_ENTRY entry = { 0 };

    while (totalBytesRead < BufferLength)
    {
        ULONG bytesRead = 0;

        // Using SPLICE_F_NONBLOCK because would block
        // on non-blocking pipe otherwise.
        ntError = PvfsSysSplice(pZctContext->pCcb->fd,
                                &currentOffset,
                                pZctContext->PipeFds[1],
                                NULL,
                                BufferLength - totalBytesRead,
                                SPLICE_F_MOVE | SPLICE_F_NONBLOCK,
                                &bytesRead);
        if (ntError == STATUS_MORE_PROCESSING_REQUIRED)
        {
            continue;
        }
        if (ntError == STATUS_RETRY)
        {
            // splice returned EAGAIN.  This normally
            // happens if the pipe buffer is not bit enough.
            // This should never happen due to the splice
            // pipe buffer check in PvfsCreateZctContext.
            ntError = STATUS_NOT_SUPPORTED;
        }
        BAIL_ON_NT_STATUS(ntError);

        /* Check for EOF */
        if (bytesRead == 0) {
            break;
        }

        totalBytesRead += bytesRead;
    }

    /* Can only get here is the loop was completed successfully */

    if (totalBytesRead > 0)
    {
        entry.Type = LW_ZCT_ENTRY_TYPE_FD_PIPE;
        entry.Length = totalBytesRead;
        entry.Data.FdPipe.Fd = pZctContext->PipeFds[0];

        ntError = LwZctAppend(pZct, &entry, 1);
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    *pBytesRead = totalBytesRead;

    return ntError;

error:
    totalBytesRead = 0;

    goto cleanup;
}
#endif

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsZctCompleteRead(
    IN PPVFS_IRP_CONTEXT pIrpContext
    )
{
    PPVFS_ZCT_CONTEXT pZctContext = (PPVFS_ZCT_CONTEXT) pIrpContext->pIrp->Args.ReadWrite.ZctCompletionContext;
    PPVFS_CCB pCcb = pZctContext->pCcb;
    BOOLEAN bMutexLocked = FALSE;

    LWIO_LOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);
    PvfsFreeZctContext(&pZctContext);
    LWIO_UNLOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);

    return STATUS_SUCCESS;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsAddZctWriteEntries(
    IN OUT PLW_ZCT_VECTOR pZct,
    IN PPVFS_ZCT_CONTEXT pZctContext,
    IN ULONG Length
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    LW_ZCT_ENTRY entry = { 0 };

    switch (pZctContext->Mode)
    {
    case PVFS_ZCT_MODE_MEMORY:
        entry.Type = LW_ZCT_ENTRY_TYPE_MEMORY;
        entry.Length = Length;
        entry.Data.Memory.Buffer = pZctContext->pBuffer;
        break;
#ifdef HAVE_SPLICE
    case PVFS_ZCT_MODE_SPLICE:
        entry.Type = LW_ZCT_ENTRY_TYPE_FD_PIPE;
        entry.Length = Length;
        entry.Data.FdPipe.Fd = pZctContext->PipeFds[1];
        break;
#endif
    default:
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = LwZctAppend(pZct, &entry, 1);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsDoZctWriteIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesWritten
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch (pZctContext->Mode)
    {
    case PVFS_ZCT_MODE_MEMORY:
        ntError = PvfsDoZctMemoryWriteIo(pZctContext,
                                         BufferLength,
                                         Offset,
                                         pBytesWritten);
        break;
#ifdef HAVE_SPLICE
    case PVFS_ZCT_MODE_SPLICE:
        ntError = PvfsDoZctSpliceWriteIo(pZctContext,
                                         BufferLength,
                                         Offset,
                                         pBytesWritten);
        break;
#endif
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
PvfsDoZctMemoryWriteIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesWritten
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    ULONG totalBytesWritten = 0;

    ntError = PvfsDoWriteIo(pZctContext->pCcb,
                            pZctContext->pBuffer,
                            BufferLength,
                            Offset,
                            &totalBytesWritten);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    *pBytesWritten = totalBytesWritten;

    return ntError;

error:
    totalBytesWritten = 0;

    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

#ifdef HAVE_SPLICE
static NTSTATUS
PvfsDoZctSpliceWriteIo(
    IN PPVFS_ZCT_CONTEXT pZctContext,
    IN ULONG BufferLength,
    IN LONG64 Offset,
    OUT PULONG pBytesWritten
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    LONG64 currentOffset = Offset;
    ULONG totalBytesWritten = 0;

    while (totalBytesWritten < BufferLength)
    {
        ULONG bytesWritten = 0;

        // Using SPLICE_F_NONBLOCK because would block
        // on non-blocking pipe otherwise.
        ntError = PvfsSysSplice(pZctContext->PipeFds[0],
                                NULL,
                                pZctContext->pCcb->fd,
                                &currentOffset,
                                BufferLength - totalBytesWritten,
                                SPLICE_F_MOVE | SPLICE_F_NONBLOCK,
                                &bytesWritten);
        if (ntError == STATUS_MORE_PROCESSING_REQUIRED)
        {
            continue;
        }
        if (ntError == STATUS_RETRY)
        {
            // splice returned EAGAIN.  This normally
            // happens if the pipe buffer is not bit enough.
            // This should never happen due to the splice
            // pipe buffer check in PvfsCreateZctContext.
            // In write completion, there is nothing we can
            // do about it except to log.
            LWIO_LOG_ERROR("Unexpected splice failure with EAGAIN/STATUS_RETRY");
        }
        BAIL_ON_NT_STATUS(ntError);

        /* Check for EOF */
        if (bytesWritten == 0) {
            break;
        }

        totalBytesWritten += bytesWritten;
    }

    /* Can only get here is the loop was completed successfully */

cleanup:
    *pBytesWritten = totalBytesWritten;

    return ntError;

error:
    totalBytesWritten = 0;

    goto cleanup;
}
#endif

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsZctCloseCcb(
    IN PPVFS_CCB pCcb
    )
{
    BOOLEAN bMutexLocked = FALSE;
    PLW_LIST_LINKS pZctCtxLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;
    PPVFS_ZCT_CONTEXT pZctContext = NULL;

    LWIO_LOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);

    pZctCtxLink = PvfsListTraverse(pCcb->pZctContextList, NULL);

    while (pZctCtxLink)
    {
        pZctContext = LW_STRUCT_FROM_FIELD(
                          pZctCtxLink,
                          PVFS_ZCT_CONTEXT,
                          CcbLinks);

        pNextLink = PvfsListTraverse(pCcb->pZctContextList, pZctCtxLink);
        PvfsListRemoveItem(pCcb->pZctContextList, pZctCtxLink);
        pZctCtxLink = pNextLink;

        PvfsFreeZctContext(&pZctContext);
    }

    LWIO_UNLOCK_MUTEX(bMutexLocked, &pCcb->FileMutex);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
