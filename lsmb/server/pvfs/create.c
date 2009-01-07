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
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Driver Entry Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "pvfs.h"

NTSTATUS
PvfsCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PPVFS_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = PvfsAllocateIrpContext(
                        pIrp,
                        &pIrpContext
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = PvfsCommonCreate(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}



NTSTATUS
PvfsAllocateIrpContext(
    PIRP pIrp,
    PPVFS_IRP_CONTEXT * ppIrpContext
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}


NTSTATUS
PvfsAllocateCCB(
    PPVFS_CCB *ppCCB
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}



NTSTATUS
PvfsCommonCreate(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE FileHandle;
    //PIO_FILE_NAME FileName;
    ACCESS_MASK DesiredAccess;
    LONG64 AllocationSize;
    //FILE_ATTRIBUTES FileAttributes;
    FILE_SHARE_FLAGS ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions;
    PIO_EA_BUFFER pEaBuffer;
    PVOID SecurityDescriptor;
    //PVOID SecurityQualityOfService;

    FileHandle = pIrp->Args.Create.FileHandle;
    DesiredAccess = pIrp->Args.Create.DesiredAccess;
    AllocationSize = pIrp->Args.Create.AllocationSize;
    ShareAccess = pIrp->Args.Create.ShareAccess;
    CreateDisposition = pIrp->Args.Create.CreateDisposition;
    CreateOptions = pIrp->Args.Create.CreateOptions;
    pEaBuffer = pIrp->Args.Create.pEaBuffer;
    SecurityDescriptor = pIrp->Args.Create.SecurityDescriptor;

    if (CreateOptions & FILE_DIRECTORY_FILE) {

            ntStatus = PvfsCommonCreateDirectory(
                            pIrpContext,
                            pIrp
                            );
            BAIL_ON_NT_STATUS(ntStatus);
    }else {

        ntStatus = PvfsCommonCreateFile(
                            pIrpContext,
                            pIrp
                            );
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateFile(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_UNICODE_STRING RootPathName;
    IO_UNICODE_STRING RelativePathName;
    IO_UNICODE_STRING AbsolutePathName;
    IO_FILE_HANDLE hFileHandle = NULL;


    ntStatus = PvfsGetFilePathName(
                    hFileHandle,
                    RootPathName
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = PvfsBuildAbsolutePathName(
                        RootPathName,
                        RelativePathName,
                        AbsolutePathName
                        );
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return(ntStatus);
}

NTSTATUS
PvfsBuildAbsolutePathName(
    IO_UNICODE_STRING RootPathName,
    IO_UNICODE_STRING RelativePathName,
    IO_UNICODE_STRING AbsolutePathName
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
PvfsGetFilePathName(
    IO_FILE_HANDLE hFileHandle,
    IO_UNICODE_STRING PathName
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateDirectory(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}
