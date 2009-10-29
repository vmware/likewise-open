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
 *        fileNetworkOpenInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileNetworkOpenInformation Handler
 *
 * Authors: Sriram Nambakam <snambakam@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
PvfsQueryFileNetworkOpenInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* File Globals */



/* Code */


NTSTATUS
PvfsFileNetworkOpenInfo(
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
        ntError = PvfsQueryFileNetworkOpenInfo(pIrpContext);
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

static NTSTATUS
PvfsQueryFileNetworkOpenInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PFILE_NETWORK_OPEN_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetInformation;
    PVFS_STAT Stat = {0};

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    /* No access checked needed for this call */

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo = (PFILE_NETWORK_OPEN_INFORMATION)Args.FileInformation;

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

    pFileInfo->AllocationSize = Stat.s_alloc;
    pFileInfo->EndOfFile      = Stat.s_size;

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
