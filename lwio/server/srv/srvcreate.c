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
 *        create.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (SRV)
 *
 *       Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "pvfs.h"

NTSTATUS
SrvCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = SrvAllocateIrpContext(
                        pIrp,
                        &pIrpContext
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvCommonCreate(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}



NTSTATUS
SrvAllocateIrpContext(
    PIRP pIrp,
    PSRV_IRP_CONTEXT * ppIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_IRP_CONTEXT pIrpContext = NULL;

    /*ntStatus = IoMemoryAllocate(
                    sizeof(SRV_IRP_CONTEXT),
                    &pIrpContext
                    );*/
    BAIL_ON_NT_STATUS(ntStatus);

    *ppIrpContext = pIrpContext;

    return(ntStatus);

error:

    *ppIrpContext = NULL;
    return(ntStatus);
}


NTSTATUS
SrvAllocateCCB(
    PSRV_CCB *ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_CCB pCCB = NULL;

   /* ntStatus = IoMemoryAllocate(
                    sizeof(SRV_CCB),
                    &pCCB
                    );*/
    BAIL_ON_NT_STATUS(ntStatus);

    *ppCCB = pCCB;

    return(ntStatus);

error:

    *ppCCB = NULL;

    return(ntStatus);
}



NTSTATUS
SrvCommonCreate(
    PSRV_IRP_CONTEXT pIrpContext,
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

    FileHandle = pIrp->FileHandle;
    DesiredAccess = pIrp->Args.Create.DesiredAccess;
    AllocationSize = pIrp->Args.Create.AllocationSize;
    ShareAccess = pIrp->Args.Create.ShareAccess;
    CreateDisposition = pIrp->Args.Create.CreateDisposition;
    CreateOptions = pIrp->Args.Create.CreateOptions;
    pEaBuffer = pIrp->Args.Create.pEaBuffer;
    SecurityDescriptor = pIrp->Args.Create.SecurityDescriptor;

    if (CreateOptions & FILE_DIRECTORY_FILE) {

            ntStatus = SrvCommonCreateDirectory(
                            pIrpContext,
                            pIrp
                            );
            BAIL_ON_NT_STATUS(ntStatus);
    }else {

        ntStatus = SrvCommonCreateFile(
                            pIrpContext,
                            pIrp
                            );
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateFile(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;
    //IO_FILE_HANDLE hFileHandle = NULL;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;

    ntStatus = SrvBuildAbsolutePathName(
                        RootPathName,
                        RelativePathName,
                        AbsolutePathName
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    pIrpContext->RootPathName = RootPathName;
    pIrpContext->RelativePathName = RelativePathName;
    pIrpContext->AbsolutePathName = AbsolutePathName;

    switch (CreateDisposition){

        case FILE_SUPERSEDE:
                ntStatus = SrvCommonCreateFileSupersede(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_CREATE:
                ntStatus = SrvCommonCreateFileCreate(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN:
                ntStatus = SrvCommonCreateFileOpen(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN_IF:
                ntStatus = SrvCommonCreateFileOpenIf(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE:
                ntStatus = SrvCommonCreateFileOverwrite(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE_IF:
                ntStatus = SrvCommonCreateFileOverwriteIf(
                                    pIrpContext,
                                    pIrp
                                    );
                break;
    }

error:

    return(ntStatus);
}

NTSTATUS
SrvBuildAbsolutePathName(
    UNICODE_STRING RootPathName,
    UNICODE_STRING RelativePathName,
    UNICODE_STRING AbsolutePathName
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
SrvGetFilePathName(
    IO_FILE_HANDLE hFileHandle,
    UNICODE_STRING PathName
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateFileSupersede(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateFileCreate(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);

error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateFileOpen(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateFileOpenIf(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateFileOverwrite(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);

}

NTSTATUS
SrvCommonCreateFileOverwriteIf(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
	PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}


NTSTATUS
SrvCommonCreateDirectory(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;
    IO_FILE_HANDLE hFileHandle = NULL;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;

    ntStatus = SrvGetFilePathName(
                    hFileHandle,
                    RootPathName
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildAbsolutePathName(
                        RootPathName,
                        RelativePathName,
                        AbsolutePathName
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    switch (CreateDisposition){

        case FILE_SUPERSEDE:
                ntStatus = SrvCommonCreateDirectoryFileSupersede(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_CREATE:
                ntStatus = SrvCommonCreateDirectoryFileCreate(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN:
                ntStatus = SrvCommonCreateDirectoryFileOpen(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN_IF:
                ntStatus = SrvCommonCreateDirectoryFileOpenIf(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE:
                ntStatus = SrvCommonCreateDirectoryFileOverwrite(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE_IF:
                ntStatus = SrvCommonCreateDirectoryFileOverwriteIf(
                                    pIrpContext,
                                    pIrp
                                    );
                break;
    }

error:

    return(ntStatus);
}


NTSTATUS
SrvCommonCreateDirectoryFileSupersede(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;

    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateDirectoryFileCreate(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateDirectoryFileOpen(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateDirectoryFileOpenIf(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateDirectoryFileOverwrite(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	//
	//
	//

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);


    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
SrvCommonCreateDirectoryFileOverwriteIf(
    PSRV_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PSRV_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = SrvAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}


