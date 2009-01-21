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
 *        Likewise Posix File System Driver (NPFS)
 *
 *       Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#include "npfs.h"

NTSTATUS
NpfsCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = NpfsAllocateIrpContext(
                        pIrp,
                        &pIrpContext
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsCommonCreate(pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}



NTSTATUS
NpfsAllocateIrpContext(
    PIRP pIrp,
    PNPFS_IRP_CONTEXT * ppIrpContext
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = IO_ALLOCATE(&pIrpContext, NPFS_IRP_CONTEXT, sizeof(*pIrpContext));
    BAIL_ON_NT_STATUS(ntStatus);

    pIrpContext->pIrp = pIrp;

    *ppIrpContext = pIrpContext;

    return(ntStatus);

error:

    *ppIrpContext = NULL;
    return(ntStatus);
}


NTSTATUS
NpfsAllocateCCB(
    PNPFS_CCB *ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PNPFS_CCB pCCB = NULL;

   /* ntStatus = IoMemoryAllocate(
                    sizeof(NPFS_CCB),
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
NpfsCommonCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
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

    FileHandle = pIrp->FileHandle;
    DesiredAccess = pIrp->Args.Create.DesiredAccess;
    AllocationSize = pIrp->Args.Create.AllocationSize;
    ShareAccess = pIrp->Args.Create.ShareAccess;
    CreateDisposition = pIrp->Args.Create.CreateDisposition;
    CreateOptions = pIrp->Args.Create.CreateOptions;

    if (CreateOptions & FILE_DIRECTORY_FILE) {

            ntStatus = NpfsCommonCreateDirectory(
                            pIrpContext,
                            pIrp
                            );
            BAIL_ON_NT_STATUS(ntStatus);
    }else {

        ntStatus = NpfsCommonCreateFile(
                            pIrpContext,
                            pIrp
                            );
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateFile(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;
    //IO_FILE_HANDLE hFileHandle = NULL;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;

    ntStatus = NpfsBuildAbsolutePathName(
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
                ntStatus = NpfsCommonCreateFileSupersede(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_CREATE:
                ntStatus = NpfsCommonCreateFileCreate(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN:
                ntStatus = NpfsCommonCreateFileOpen(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN_IF:
                ntStatus = NpfsCommonCreateFileOpenIf(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE:
                ntStatus = NpfsCommonCreateFileOverwrite(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE_IF:
                ntStatus = NpfsCommonCreateFileOverwriteIf(
                                    pIrpContext,
                                    pIrp
                                    );
                break;
    }

error:

    return(ntStatus);
}

NTSTATUS
NpfsBuildAbsolutePathName(
    UNICODE_STRING RootPathName,
    UNICODE_STRING RelativePathName,
    UNICODE_STRING AbsolutePathName
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
NpfsGetFilePathName(
    IO_FILE_HANDLE hFileHandle,
    UNICODE_STRING PathName
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateFileSupersede(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateFileCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);

error:

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateFileOpen(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateFileOpenIf(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateFileOverwrite(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);

}

NTSTATUS
NpfsCommonCreateFileOverwriteIf(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
	PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}


NTSTATUS
NpfsCommonCreateDirectory(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;
    IO_FILE_HANDLE hFileHandle = NULL;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;

    ntStatus = NpfsGetFilePathName(
                    hFileHandle,
                    RootPathName
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NpfsBuildAbsolutePathName(
                        RootPathName,
                        RelativePathName,
                        AbsolutePathName
                        );
    BAIL_ON_NT_STATUS(ntStatus);

    switch (CreateDisposition){

        case FILE_SUPERSEDE:
                ntStatus = NpfsCommonCreateDirectoryFileSupersede(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_CREATE:
                ntStatus = NpfsCommonCreateDirectoryFileCreate(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN:
                ntStatus = NpfsCommonCreateDirectoryFileOpen(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN_IF:
                ntStatus = NpfsCommonCreateDirectoryFileOpenIf(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE:
                ntStatus = NpfsCommonCreateDirectoryFileOverwrite(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE_IF:
                ntStatus = NpfsCommonCreateDirectoryFileOverwriteIf(
                                    pIrpContext,
                                    pIrp
                                    );
                break;
    }

error:

    return(ntStatus);
}


NTSTATUS
NpfsCommonCreateDirectoryFileSupersede(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;

    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateDirectoryFileCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateDirectoryFileOpen(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateDirectoryFileOpenIf(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
NpfsCommonCreateDirectoryFileOverwrite(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
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
NpfsCommonCreateDirectoryFileOverwriteIf(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PNPFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = NpfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}


