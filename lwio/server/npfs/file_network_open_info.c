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
 *        file_network_open_info.c
 *
 * Abstract:
 *
 *        Likewise Named Pipe File System Driver (NPFS)
 *
 *        FileNetworkOpenInformation Handler
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
NpfsQueryFileNetworkOpenInfo(
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsFileNetworkOpenInfo(
    NPFS_INFO_TYPE Type,
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
        case NPFS_SET:
            ntStatus = STATUS_NOT_SUPPORTED;
            break;

        case NPFS_QUERY:
            ntStatus = NpfsQueryFileNetworkOpenInfo(pIrpContext);
            break;

        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
NpfsQueryFileNetworkOpenInfo(
    PNPFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PNPFS_CCB pCcb = NULL;
    PFILE_NETWORK_OPEN_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args =
                                    pIrpContext->pIrp->Args.QuerySetInformation;

    /* Sanity checks */

    ntStatus = NpfsGetCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntStatus);

    /* No access checked needed for this call */

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntStatus);

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileInfo = (PFILE_NETWORK_OPEN_INFORMATION)Args.FileInformation;

    ntStatus = NpfsQueryCcbFileNetworkOpenInfo(pCcb, pFileInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);

    ntStatus = STATUS_SUCCESS;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
NpfsQueryCcbFileNetworkOpenInfo(
    PNPFS_CCB                      pCcb,
    PFILE_NETWORK_OPEN_INFORMATION pFileInfo
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pFileInfo->LastAccessTime = 0;
    pFileInfo->LastWriteTime = 0;
    pFileInfo->ChangeTime = 0;
    pFileInfo->CreationTime = 0;
    pFileInfo->FileAttributes = FILE_ATTRIBUTE_NORMAL;
    pFileInfo->AllocationSize = 8192;
    pFileInfo->EndOfFile      = 0;

    return ntStatus;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/

