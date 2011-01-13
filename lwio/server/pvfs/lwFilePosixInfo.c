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
 *        lwFilePosixInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        LwFilePosixInformation Handler
 *
 * Authors: Evgeny Popovich <epopovich@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static
NTSTATUS
PvfsQueryLwFilePosixInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* File Globals */



/* Code */


NTSTATUS
PvfsLwFilePosixInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = STATUS_NOT_SUPPORTED;
        break;

    case PVFS_QUERY:
        ntError = PvfsQueryLwFilePosixInfo(pIrpContext);
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

static
NTSTATUS
PvfsQueryLwFilePosixInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PLW_FILE_POSIX_INFORMATION pFilePosixInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION args = pIrpContext->pIrp->Args.QuerySetInformation;
    PVFS_STAT stat = { 0 };

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(args.FileInformation, ntError);

    /* No access checked needed for this call */

    if (args.Length < sizeof(*pFilePosixInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFilePosixInfo = (PLW_FILE_POSIX_INFORMATION)args.FileInformation;

    /* Real work starts here */

    ntError = PvfsSysFstat(pCcb->fd, &stat);
    BAIL_ON_NT_STATUS(ntError);

    pFilePosixInfo->UnixAccessTime = stat.s_atime;
    pFilePosixInfo->UnixModificationTime = stat.s_mtime;
    pFilePosixInfo->UnixChangeTime = stat.s_ctime;

    ntError = PvfsGetFileAttributes(pCcb, &pFilePosixInfo->FileAttributes);
    BAIL_ON_NT_STATUS(ntError);

    pFilePosixInfo->EndOfFile = stat.s_size;
    pFilePosixInfo->AllocationSize = stat.s_alloc;
    pFilePosixInfo->UnixNumberOfLinks = stat.s_nlink;
    pFilePosixInfo->UnixMode = stat.s_mode;
    pFilePosixInfo->Uid = stat.s_uid;
    pFilePosixInfo->Gid = stat.s_gid;

    // TODO - fill in the correct volume id
    // pFilePosixInfo->VolumeId = stat.s_dev;
    pFilePosixInfo->VolumeId = 1;

    // TODO - is this the file id we want?  Also get generation number.
    // pFilePosixInfo->InodeNumber = stat.s_ino;
    // pFilePosixInfo->GenerationNumber = 0;

    // TODO - Remove it later.  Inode and gen encode full path now
    {
        pFilePosixInfo->GenerationNumber = 0;
        strcpy((char*)&pFilePosixInfo->InodeNumber, "/pvfs");
        strncpy(((char*)&pFilePosixInfo->InodeNumber) + 5, pCcb->pszFilename, 10);
    }

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFilePosixInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}
