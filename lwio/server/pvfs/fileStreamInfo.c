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
 *        fileStreamInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileStreamInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
PvfsQueryFileStreamInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* File Globals */



/* Code */


NTSTATUS
PvfsFileStreamInfo(
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
        ntError = PvfsQueryFileStreamInfo(pIrpContext);
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
PvfsQueryFileStreamInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = (PPVFS_CCB)IoFileGetContext(pIrp->FileHandle);;
    PFILE_STREAM_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetInformation;
    PVFS_STAT Stat = {0};
    PWSTR pwszStreamName = NULL;
    size_t StreamNameLen = RtlCStringNumChars("::$DATA") * sizeof(WCHAR);

    /* Sanity checks */

    PVFS_BAIL_ON_INVALID_CCB(pCcb, ntError);
    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_READ_ATTRIBUTES);
    BAIL_ON_NT_STATUS(ntError);

    /* Make sure buffer is large enough for the static structure
       fields and the magic WCHAR("::$DATA") string */

    if (Args.Length < (sizeof(*pFileInfo) + StreamNameLen))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo = (PFILE_STREAM_INFORMATION)Args.FileInformation;

    /* Real work starts here */

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlWC16StringAllocateFromCString(&pwszStreamName, "::$DATA");
    BAIL_ON_NT_STATUS(ntError);

    pFileInfo->NextEntryOffset = 0;
    pFileInfo->StreamSize = Stat.s_size;
    pFileInfo->StreamAllocationSize = Stat.s_blksize;
    pFileInfo->StreamNameLength = RtlWC16StringNumChars(pwszStreamName);
    memcpy(pFileInfo->StreamName, pwszStreamName, pFileInfo->StreamNameLength);

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    PVFS_SAFE_FREE_MEMORY(pwszStreamName);

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

