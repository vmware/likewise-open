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
 *        fileDispositionInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileDispositionInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static
NTSTATUS
PvfsSetFileDispositionInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );

/* File Globals */



/* Code */

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsSetFileDispositionInfoWithContext(
    PVOID pContext
    );

static
NTSTATUS
PvfsCreateSetFileDispositionInfoContext(
    OUT PPVFS_PENDING_SET_FILE_DISPOSITION *ppSetDispositionInfoContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb
    );

static
VOID
PvfsFreeSetFileDispositionInfoContext(
    IN OUT PVOID *ppContext
    );



NTSTATUS
PvfsFileDispositionInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = PvfsSetFileDispositionInfo(pIrpContext);
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

/****************************************************************
 ***************************************************************/

static
NTSTATUS
PvfsSetFileDispositionInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = {0};
    PFILE_DISPOSITION_INFORMATION pFileInfo = NULL;
    PPVFS_PENDING_SET_FILE_DISPOSITION pSetDispositionInfoCtx = NULL;

    Args = pIrpContext->pIrp->Args.QuerySetInformation;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);
    pFileInfo = (PFILE_DISPOSITION_INFORMATION)Args.FileInformation;

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckFileHandle(pCcb, DELETE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCreateSetFileDispositionInfoContext(
                  &pSetDispositionInfoCtx,
                  pIrpContext,
                  pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (pFileInfo->DeleteFile)
    {
        ntError = PvfsOplockBreakIfLocked(pIrpContext, pCcb, pCcb->pScb);
    }
    else
    {
        ntError = STATUS_SUCCESS;
    }

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsSetFileDispositionInfoWithContext(pSetDispositionInfoCtx);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pSetDispositionInfoCtx->pCcb->pScb,
                      pIrpContext,
                      pSetDispositionInfoCtx->pCcb,
                      PvfsSetFileDispositionInfoWithContext,
                      PvfsFreeSetFileDispositionInfoContext,
                      (PVOID)pSetDispositionInfoCtx);
        if (ntError == STATUS_PENDING)
        {
            pSetDispositionInfoCtx = NULL;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pSetDispositionInfoCtx->pCcb->pScb,
                      pIrpContext,
                      PvfsSetFileDispositionInfoWithContext,
                      PvfsFreeSetFileDispositionInfoContext,
                      (PVOID)pSetDispositionInfoCtx);
        if (ntError == STATUS_PENDING)
        {
            pSetDispositionInfoCtx = NULL;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeSetFileDispositionInfoContext(OUT_PPVOID(&pSetDispositionInfoCtx));

    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;

}


static
NTSTATUS
PvfsSetFileDispositionInfoWithContext(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_SET_FILE_DISPOSITION pSetFileDispositionCtx = (PPVFS_PENDING_SET_FILE_DISPOSITION)pContext;
    PIRP pIrp = pSetFileDispositionCtx->pIrpContext->pIrp;
    PPVFS_CCB pCcb = pSetFileDispositionCtx->pCcb;
    IRP_ARGS_QUERY_SET_INFORMATION Args = {0};
    PFILE_DISPOSITION_INFORMATION pFileInfo = NULL;
    IO_MATCH_FILE_SPEC FileSpec = {0};
    WCHAR wszPattern[2] = {L'*', 0x0 };
    FILE_ATTRIBUTES Attributes = 0;

    /* Sanity checks */

    Args = pSetFileDispositionCtx->pIrpContext->pIrp->Args.QuerySetInformation;
    pFileInfo = (PFILE_DISPOSITION_INFORMATION)Args.FileInformation;

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Real work starts here */

    if (pFileInfo->DeleteFile == TRUE)
    {
        /* Checks as to whether we can set the delete-on-close bit
           differs for files and directortories */

        if (PVFS_IS_DIR(pCcb))
        {
            BOOLEAN bDirLocked = FALSE;

            LwRtlUnicodeStringInit(&FileSpec.Pattern, wszPattern);

            LWIO_LOCK_MUTEX(bDirLocked, &pCcb->ControlBlock);
            ntError = PvfsEnumerateDirectory(pCcb, &FileSpec, 1, TRUE);
            LWIO_UNLOCK_MUTEX(bDirLocked,  &pCcb->ControlBlock);

            if (ntError == STATUS_SUCCESS)
            {
                ntError = STATUS_DIRECTORY_NOT_EMPTY;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        else
        {
            ntError = PvfsGetFileAttributes(pCcb, &Attributes);
            BAIL_ON_NT_STATUS(ntError);

            if (Attributes & FILE_ATTRIBUTE_READONLY)
            {
                ntError = STATUS_CANNOT_DELETE;
                BAIL_ON_NT_STATUS(ntError);
            }
        }

        PvfsScbSetPendingDelete(pCcb->pScb, TRUE);
    }
    else
    {
        PvfsScbSetPendingDelete(pCcb->pScb, FALSE);
    }

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsCreateSetFileDispositionInfoContext(
    OUT PPVFS_PENDING_SET_FILE_DISPOSITION *ppSetDispositionInfoContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_SET_FILE_DISPOSITION pSetDispositionInfoCtx = NULL;

    ntError = PvfsAllocateMemory(
		      OUT_PPVOID(&pSetDispositionInfoCtx),
                  sizeof(PVFS_PENDING_SET_FILE_DISPOSITION),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->SetFileType = PVFS_SET_FILE_DISPOSITION;
    pSetDispositionInfoCtx->pIrpContext = PvfsReferenceIrpContext(pIrpContext);
    pSetDispositionInfoCtx->pCcb = PvfsReferenceCCB(pCcb);

    *ppSetDispositionInfoContext = pSetDispositionInfoCtx;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsFreeSetFileDispositionInfoContext(
    IN OUT PVOID *ppContext
    )
{
    PPVFS_PENDING_SET_FILE_DISPOSITION pDispositionInfoCtx = NULL;

    if (ppContext && *ppContext)
    {
        pDispositionInfoCtx = (PPVFS_PENDING_SET_FILE_DISPOSITION)(*ppContext);

        if (pDispositionInfoCtx->pIrpContext)
        {
            PvfsReleaseIrpContext(&pDispositionInfoCtx->pIrpContext);
        }

        if (pDispositionInfoCtx->pCcb)
        {
            PvfsReleaseCCB(pDispositionInfoCtx->pCcb);
        }

        PVFS_FREE(ppContext);
    }

    return;
}

