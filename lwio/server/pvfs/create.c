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
 *        Likewise Posix File System Driver (PVFS)
 *
 *       Create Dispatch Routine
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
    PPVFS_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = IO_ALLOCATE(&pIrpContext, PVFS_IRP_CONTEXT, sizeof(*pIrpContext));
    BAIL_ON_NT_STATUS(ntStatus);

    pIrpContext->pIrp = pIrp;

    *ppIrpContext = pIrpContext;

    return(ntStatus);

error:

    *ppIrpContext = NULL;
    return(ntStatus);
}


NTSTATUS
PvfsAllocateCCB(
    PPVFS_CCB *ppCCB
    )
{
    NTSTATUS ntStatus = 0;
    PPVFS_CCB pCCB = NULL;

   /* ntStatus = IoMemoryAllocate(
                    sizeof(PVFS_CCB),
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

    FileHandle = pIrp->FileHandle;
    DesiredAccess = pIrp->Args.Create.DesiredAccess;
    AllocationSize = pIrp->Args.Create.AllocationSize;
    ShareAccess = pIrp->Args.Create.ShareAccess;
    CreateDisposition = pIrp->Args.Create.CreateDisposition;
    CreateOptions = pIrp->Args.Create.CreateOptions;

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
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;
    //IO_FILE_HANDLE hFileHandle = NULL;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;

    ntStatus = PvfsBuildAbsolutePathName(
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
                ntStatus = PvfsCommonCreateFileSupersede(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_CREATE:
                ntStatus = PvfsCommonCreateFileCreate(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN:
                ntStatus = PvfsCommonCreateFileOpen(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN_IF:
                ntStatus = PvfsCommonCreateFileOpenIf(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE:
                ntStatus = PvfsCommonCreateFileOverwrite(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE_IF:
                ntStatus = PvfsCommonCreateFileOverwriteIf(
                                    pIrpContext,
                                    pIrp
                                    );
                break;
    }

error:

    return(ntStatus);
}

NTSTATUS
PvfsBuildAbsolutePathName(
    UNICODE_STRING RootPathName,
    UNICODE_STRING RelativePathName,
    UNICODE_STRING AbsolutePathName
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
PvfsGetFilePathName(
    IO_FILE_HANDLE hFileHandle,
    UNICODE_STRING PathName
    )
{
    NTSTATUS ntStatus = 0;

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);

error:

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateFileOpen(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateFileOpenIf(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateFileOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);

}

NTSTATUS
PvfsCommonCreateFileOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
	IO_FILE_HANDLE hFileHandle = NULL;
	PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}


NTSTATUS
PvfsCommonCreateDirectory(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;
    IO_FILE_HANDLE hFileHandle = NULL;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;

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

    switch (CreateDisposition){

        case FILE_SUPERSEDE:
                ntStatus = PvfsCommonCreateDirectoryFileSupersede(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_CREATE:
                ntStatus = PvfsCommonCreateDirectoryFileCreate(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN:
                ntStatus = PvfsCommonCreateDirectoryFileOpen(
                                    pIrpContext,
                                    pIrp
                                    );
                break;

        case FILE_OPEN_IF:
                ntStatus = PvfsCommonCreateDirectoryFileOpenIf(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE:
                ntStatus = PvfsCommonCreateDirectoryFileOverwrite(
                                pIrpContext,
                                pIrp
                                );
                break;

        case FILE_OVERWRITE_IF:
                ntStatus = PvfsCommonCreateDirectoryFileOverwriteIf(
                                    pIrpContext,
                                    pIrp
                                    );
                break;
    }

error:

    return(ntStatus);
}


NTSTATUS
PvfsCommonCreateDirectoryFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;

    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateDirectoryFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateDirectoryFileOpen(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateDirectoryFileOpenIf(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}

NTSTATUS
PvfsCommonCreateDirectoryFileOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
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
PvfsCommonCreateDirectoryFileOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    IO_FILE_HANDLE hFileHandle = NULL;
    PPVFS_CCB pCCB = NULL;

    hFileHandle = pIrpContext->pIrp->FileHandle;
    ntStatus = PvfsAllocateCCB(&pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoFileSetContext(hFileHandle, pCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    return(ntStatus);
error:

    return(ntStatus);
}


