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
 *        fileEndOfFileInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileEndOfFileInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* File Globals */



/* Code */

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsSetFileEndOfFileInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileEndOfFileInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = PvfsSetFileEndOfFileInfo(pIrpContext);
        break;

    case PVFS_QUERY:
        ntError = STATUS_NOT_SUPPORTED;
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsSetEndOfFileWithContext(
    PVOID pContext
    );

static NTSTATUS
PvfsCreateSetEndOfFileContext(
    OUT PPVFS_PENDING_SET_END_OF_FILE *ppSetEndOfFileContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb
    );

static VOID
PvfsFreeSetEndOfFileContext(
    IN OUT PVOID *ppContext
    );

static NTSTATUS
PvfsSetFileEndOfFileInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = {0};
    PFILE_END_OF_FILE_INFORMATION pFileInfo = NULL;
    PPVFS_PENDING_SET_END_OF_FILE pSetEoFCtx = NULL;

    Args = pIrpContext->pIrp->Args.QuerySetInformation;
    pFileInfo = (PFILE_END_OF_FILE_INFORMATION)Args.FileInformation;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_WRITE_DATA);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCreateSetEndOfFileContext(
                  &pSetEoFCtx,
                  pIrpContext,
                  pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsOplockBreakIfLocked(pIrpContext, pCcb, pCcb->pFcb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsSetEndOfFileWithContext(pSetEoFCtx);
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pSetEoFCtx->pCcb->pFcb,
                      pIrpContext,
                      PvfsSetEndOfFileWithContext,
                      PvfsFreeSetEndOfFileContext,
                      (PVOID)pSetEoFCtx);
        if (ntError == STATUS_SUCCESS) {
            pSetEoFCtx = NULL;
            ntError = STATUS_PENDING;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeSetEndOfFileContext((PVOID*)&pSetEoFCtx);

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
PvfsSetEndOfFileWithContext(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_SET_END_OF_FILE pSetEoFCtx = (PPVFS_PENDING_SET_END_OF_FILE)pContext;
    PIRP pIrp = pSetEoFCtx->pIrpContext->pIrp;
    PPVFS_CCB pCcb = pSetEoFCtx->pCcb;
    IRP_ARGS_QUERY_SET_INFORMATION Args = {0};
    PFILE_END_OF_FILE_INFORMATION pFileInfo = NULL;

    Args = pSetEoFCtx->pIrpContext->pIrp->Args.QuerySetInformation;
    pFileInfo = (PFILE_END_OF_FILE_INFORMATION)Args.FileInformation;

    ntError = PvfsSysFtruncate(pCcb->fd, (off_t)pFileInfo->EndOfFile);
    BAIL_ON_NT_STATUS(ntError);

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsCreateSetEndOfFileContext(
    OUT PPVFS_PENDING_SET_END_OF_FILE *ppSetEndOfFileContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_SET_END_OF_FILE pSetEndOfFileCtx = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pSetEndOfFileCtx,
                  sizeof(PVFS_PENDING_READ));
    BAIL_ON_NT_STATUS(ntError);

    pSetEndOfFileCtx->pIrpContext = pIrpContext;
    pSetEndOfFileCtx->pCcb = pCcb;

    *ppSetEndOfFileContext = pSetEndOfFileCtx;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsFreeSetEndOfFileContext(
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

