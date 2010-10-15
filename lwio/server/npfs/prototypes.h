/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

// close.c

NTSTATUS
NpfsClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonClose(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCloseHandle(
    PNPFS_CCB pCCB
    );

NTSTATUS
NpfsServerCloseHandle(
    PNPFS_CCB pSCB
    );

NTSTATUS
NpfsClientCloseHandle(
    PNPFS_CCB pCCB
    );

// create.c

NTSTATUS
NpfsCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsValidateCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PUNICODE_STRING pPipeName
    );

NTSTATUS
NpfsCommonCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsAllocateIrpContext(
    PIRP pIrp,
    PNPFS_IRP_CONTEXT * ppIrpContext
    );


NTSTATUS
NpfsFreeIrpContext(
    PNPFS_IRP_CONTEXT pIrpContext
    );

// deviceio.c

NTSTATUS
NpfsDeviceIo(
    IO_DEVICE_HANDLE DriverHandle,
    PIRP pIrp
    );


/* memory.c */
NTSTATUS
NpfsAllocateMemory(
    IN ULONG ulSize,
    OUT PVOID* ppMemory
    );

VOID
NpfsFreeMemory(
    IN OUT PVOID pMemory
    );

/* pipe.c */

NTSTATUS
NpfsCreatePipe(
    PNPFS_FCB pFCB,
    PNPFS_PIPE * ppPipe
    );

NTSTATUS
NpfsFindAvailablePipe(
    PNPFS_FCB pFCB,
    PNPFS_PIPE * ppPipe
    );

NTSTATUS
NpfsFreePipeContext(
    PNPFS_PIPE pPipe
    );


VOID
NpfsReleasePipe(
    PNPFS_PIPE pPipe
    );

VOID
NpfsAddRefPipe(
    PNPFS_PIPE pPipe
    );


NTSTATUS
NpfsFreePipe(
    PNPFS_PIPE pPipe
    );

VOID
NpfsRemovePipeFromFCB(
    PNPFS_FCB pFCB,
    PNPFS_PIPE pPipe
    );

/* createnp.c */

NTSTATUS
NpfsCreateNamedPipe(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsAllocateCCB(
    PNPFS_CCB *ppCCB
    );

NTSTATUS
NpfsCommonCreateNamedPipe(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsValidateCreateNamedPipe(
    PNPFS_IRP_CONTEXT pIrpContext,
    PUNICODE_STRING  pPath
    );

/* ccb.c */

NTSTATUS
NpfsCreateSCB(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_PIPE pPipe,
    PNPFS_CCB * ppSCB
    );

NTSTATUS
NpfsCreateCCB(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_PIPE pPipe,
    PNPFS_CCB * ppCCB
    );


VOID
NpfsReleaseCCB(
    PNPFS_CCB pCCB
    );

VOID
NpfsAddRefCCB(
    PNPFS_CCB pCCB
    );

VOID
NpfsFreeCCB(
    PNPFS_CCB pCCB
    );


NTSTATUS
NpfsGetCCB(
    IO_FILE_HANDLE FileHandle,
    PNPFS_CCB * ppCCB
    );

NTSTATUS
NpfsSetCCB(
    IO_FILE_HANDLE FileHandle,
    PNPFS_CCB pCCB
    );

/* connectnp.c */

NTSTATUS
NpfsAsyncConnectNamedPipe(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

/* file_basic_info.c */

NTSTATUS
NpfsFileBasicInfo(
    NPFS_INFO_TYPE Type,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsQueryFileBasicInfo(
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsQueryCcbFileBasicInfo(
    PNPFS_CCB               pCcb,
    PFILE_BASIC_INFORMATION pFileInfo
    );


/* file_std_info.c */

NTSTATUS
NpfsFileStandardInfo(
    NPFS_INFO_TYPE Type,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsQueryFileStandardInfo(
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsQueryCcbFileStandardInfo(
    PNPFS_CCB                  pCcb,
    PFILE_STANDARD_INFORMATION pFileInfo
    );

/* file_access_info.c */

NTSTATUS
NpfsQueryFileAccessInfo(
    PNPFS_IRP_CONTEXT pIrpContext
    );


NTSTATUS
NpfsFileAccessInfo(
    NPFS_INFO_TYPE Type,
    PNPFS_IRP_CONTEXT pIrpContext
    );

/* file_pipe_info.c */

NTSTATUS
NpfsFilePipeInfo(
    NPFS_INFO_TYPE    Type,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsQueryCcbFilePipeInfo(
    PNPFS_CCB              pCcb,
    PFILE_PIPE_INFORMATION pPipeInfo
    );

/* file_pipe_local_info.c */

NTSTATUS
NpfsFilePipeLocalInfo(
    NPFS_INFO_TYPE    Type,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsQueryCcbFilePipeLocalInfo(
    PNPFS_CCB                    pCcb,
    PFILE_PIPE_LOCAL_INFORMATION pPipeInfo
    );

/* fcb.c */


NTSTATUS
NpfsCreateFCB(
    PUNICODE_STRING pUnicodeString,
    PNPFS_FCB * ppFcb
    );

NTSTATUS
NpfsFindFCB(
    PUNICODE_STRING pUnicodeString,
    PNPFS_FCB * ppFcb
    );


VOID
NpfsReleaseFCB(
    PNPFS_FCB pFCB
    );

VOID
NpfsAddRefFCB(
    PNPFS_FCB pFCB
    );

NTSTATUS
NpfsFreeFCB(
    PNPFS_FCB pFCB
    );


/* fsctl.c */

NTSTATUS
NpfsFsCtl(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonFsCtl(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

/* mdl.c */
NTSTATUS
NpfsEnqueueBuffer(
    PLW_LIST_LINKS pMdlList,
    PVOID pBuffer,
    ULONG Length,
    PULONG pulBytesTransferred
    );

NTSTATUS
NpfsDequeueBuffer(
    PLW_LIST_LINKS pMdlList,
    PVOID pBuffer,
    ULONG Length,
    PULONG pulBytesTransferred
    );

NTSTATUS
NpfsCreateMdl(
    ULONG Length,
    PVOID pBuffer,
    PNPFS_MDL * ppMdl
    );

VOID
NpfsEnqueueMdl(
    PLW_LIST_LINKS pMdlList,
    PNPFS_MDL pMdl
    );

VOID
NpfsDequeueMdl(
    PLW_LIST_LINKS pMdlList,
    PNPFS_MDL *ppMdl
    );

NTSTATUS
NpfsCopyMdl(
    PNPFS_MDL pMdl,
    PVOID pBuffer,
    ULONG Length,
    ULONG *ppLengthCopied
    );

NTSTATUS
NpfsDeleteMdl(
    PNPFS_MDL pMdl
    );

NTSTATUS
NpfsAddMdltoInboundQueue(
    PNPFS_CCB pCCB,
    PNPFS_MDL pMdl
    );

VOID
NpfsFreeMdlList(
    PLW_LIST_LINKS pMdlList
    );

VOID
NpfsFreeMdl(
    PNPFS_MDL pMdl
    );

BOOLEAN
NpfsMdlListIsEmpty(
    PLW_LIST_LINKS pMdlList
    );

/* queryinfo.c */

NTSTATUS
NpfsQueryInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonQueryInformation(
    PNPFS_IRP_CONTEXT  pIrpContext,
    PIRP pIrp
    );

/* read.c */

NTSTATUS
NpfsRead(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );


NTSTATUS
NpfsCommonRead(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS
NpfsReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );


NTSTATUS
NpfsServerReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

VOID
NpfsServerCompleteReadFile(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsClientReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

VOID
NpfsClientCompleteReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsServerReadFile_Connected(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsClientReadFile_Connected(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

/* setinfo.c */

NTSTATUS
NpfsSetInformation(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );

/* write.c */

NTSTATUS
NpfsWrite(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );


NTSTATUS
NpfsCommonWrite(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsWriteFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsServerWriteFile(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsClientWriteFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsServerWriteFile_Connected(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsClientWriteFile_Connected(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );
