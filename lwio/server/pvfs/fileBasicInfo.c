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
 *        fileBasicInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileBasicInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
PvfsQueryFileBasicInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsSetFileBasicInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* File Globals */



/* Code */


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsFileBasicInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = PvfsSetFileBasicInfo(pIrpContext);
        break;

    case PVFS_QUERY:
        ntError = PvfsQueryFileBasicInfo(pIrpContext);
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

static NTSTATUS
PvfsQueryFileBasicInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PFILE_BASIC_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetInformation;
    PVFS_STAT Stat = {0};

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

#if 0
    /* Disabled for now until I can properly deal with
       NTcreate&X with only WRITE_DAC access.  For now, treat
       this like FILE_STANDARD_INFORMATION. */

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_READ_ATTRIBUTES);
    BAIL_ON_NT_STATUS(ntError);
#endif


    if (Args.Length < sizeof(*pFileInfo))
    {        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo = (PFILE_BASIC_INFORMATION)Args.FileInformation;

    /* Real work starts here */

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    /* Timestamps */

    ntError = PvfsUnixToWinTime(&pFileInfo->LastAccessTime, Stat.s_atime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->LastWriteTime, Stat.s_mtime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->ChangeTime, Stat.s_ctime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->CreationTime, Stat.s_crtime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsGetFileAttributes(pCcb, &pFileInfo->FileAttributes);
    BAIL_ON_NT_STATUS(ntError);

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

static NTSTATUS
PvfsSetFileBasicInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PFILE_BASIC_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetInformation;
    LONG64 WriteTime = 0;
    LONG64 AccessTime = 0;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_WRITE_ATTRIBUTES);
    BAIL_ON_NT_STATUS(ntError);

    if (Args.Length < sizeof(*pFileInfo))
    {        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo = (PFILE_BASIC_INFORMATION)Args.FileInformation;

    /* Real work starts here */

    ntError = PvfsValidatePath(pCcb->pFcb->pszFilename, &pCcb->pFcb->FileId);
    BAIL_ON_NT_STATUS(ntError);

    /* We cant's set the Change Time so ignore it */

    WriteTime  = pFileInfo->LastWriteTime;
    AccessTime = pFileInfo->LastAccessTime;

    /* Ignore 0xffffffff */

    if (WriteTime == (LONG64)-1) {
        WriteTime = 0;
    }

    if (AccessTime == (LONG64)-1) {
        AccessTime = 0;
    }

    /* Save for "sticky" WriteTime sematics */

    if (WriteTime != 0 && pCcb->pFcb) {
        pCcb->pFcb->LastWriteTime = WriteTime;
    }

    /* Check if we need to preserve any original timestamps */

    if (WriteTime == 0 || AccessTime == 0) {
        PVFS_STAT Stat = {0};

        ntError = PvfsSysFstat(pCcb->fd, &Stat);
        BAIL_ON_NT_STATUS(ntError);

        if (WriteTime == 0) {
            ntError = PvfsUnixToWinTime(&WriteTime, Stat.s_mtime);
            BAIL_ON_NT_STATUS(ntError);
        }

        if (AccessTime == 0) {
            ntError = PvfsUnixToWinTime(&AccessTime, Stat.s_atime);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    ntError = PvfsSysUtime(pCcb->pszFilename, WriteTime, AccessTime);
    BAIL_ON_NT_STATUS(ntError);

    if (pFileInfo->FileAttributes != 0) {
        ntError = PvfsSetFileAttributes(pCcb, pFileInfo->FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

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

