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
 *        fileFsDeviceInfo
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileFsDeviceInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsQueryFileFsDeviceInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileFsDeviceInfo(
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
        ntError = PvfsQueryFileFsDeviceInfo(pIrpContext);
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


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsQueryFileFsDeviceInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PFILE_FS_DEVICE_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_VOLUME_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetVolumeInformation;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* not sure exact access rights to check for here */

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_READ_ATTRIBUTES);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FsInformation, ntError);

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo = (PFILE_FS_DEVICE_INFORMATION)Args.FsInformation;

    /* Real work starts here */

    pFileInfo->DeviceType = FILE_DEVICE_DISK;
    pFileInfo->DeviceCharacteristics = FILE_DEVICE_IS_MOUNTED;

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    if (pCcb)
    {
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

