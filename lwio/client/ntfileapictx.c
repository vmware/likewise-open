/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ntfileapiipc.c
 *
 * Abstract:
 *
 *        NT File API IPC Implementation
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

static
VOID
NtpCtxFreeResponse(
    IN PIO_CONTEXT pConnection,
    IN LWMsgMessageTag ReponseType,
    IN PVOID pResponse
    )
{
    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, ReponseType, pResponse);
    }
}

static
NTSTATUS
NtpCtxCall(
    IN PIO_CONTEXT pConnection,
    IN LWMsgMessageTag RequestType,
    IN PVOID pRequest,
    IN LWMsgMessageTag ResponseType,
    OUT PVOID* ppResponse
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    LWMsgMessageTag actualResponseType = (LWMsgMessageTag) -1;
    PVOID pResponse = NULL;

    status = NtIpcLWMsgStatusToNtStatus(lwmsg_assoc_send_transact(
                pConnection->pAssoc,
                RequestType,
                pRequest,
                &actualResponseType,
                &pResponse));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (actualResponseType != ResponseType)
    {
        assert(FALSE);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (status)
    {
        NtpCtxFreeResponse(pConnection, actualResponseType, pResponse);
        pResponse = NULL;
    }

    *ppResponse = pResponse;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
NtpCtxGetIoResult(
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    IN PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse
    )
{
    pIoStatusBlock->Status = pResponse->Status;
    pIoStatusBlock->BytesTransferred = pResponse->BytesTransferred;
    return pIoStatusBlock->Status;
}

static
NTSTATUS
NtpCtxGetBufferResult(
    OUT PIO_STATUS_BLOCK pIoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse
    )
{
    pIoStatusBlock->Status = pResponse->Status;
    pIoStatusBlock->BytesTransferred = pResponse->BytesTransferred;
    assert(pResponse->BytesTransferred <= Length);
    memcpy(Buffer, pResponse->Buffer, SMB_MIN(pResponse->BytesTransferred, Length));
    return pIoStatusBlock->Status;
}

// Need to add a way to cancel operation from outside IRP layer.
// Probably requires something in IO_ASYNC_CONTROL_BLOCK.

//
// The operations below are in categories:
//
// - Core I/O
// - Additional
// - Namespace
// - Advanced
//

//
// Core I/O Operations
//

NTSTATUS
LwNtCtxCreateNamedPipeFile(
    IN PIO_CONTEXT pConnection,
    IN LW_PIO_ACCESS_TOKEN pSecurityToken,
    OUT PIO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    IN OPTIONAL PVOID SecurityDescriptor, // TBD
    IN OPTIONAL PVOID SecurityQualityOfService, // TBD
    IN ACCESS_MASK DesiredAccess,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN FILE_PIPE_TYPE_MASK NamedPipeType,
    IN FILE_PIPE_READ_MODE_MASK ReadMode,
    IN FILE_PIPE_COMPLETION_MODE_MASK CompletionMode,
    IN ULONG MaximumInstances,
    IN ULONG InboundQuota,
    IN ULONG OutboundQuota,
    IN OPTIONAL PLONG64 DefaultTimeout
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_FILE_HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    PIO_ECP_LIST ecpList = NULL;
    IO_ECP_NAMED_PIPE pipeParams = { 0 };

    status = IoRtlEcpListAllocate(&ecpList);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pipeParams.NamedPipeType = NamedPipeType;
    pipeParams.ReadMode = ReadMode;
    pipeParams.CompletionMode = CompletionMode;
    pipeParams.MaximumInstances = MaximumInstances;
    pipeParams.InboundQuota = InboundQuota;
    pipeParams.OutboundQuota = OutboundQuota;
    if (DefaultTimeout)
    {
        pipeParams.DefaultTimeout = *DefaultTimeout;
        pipeParams.HaveDefaultTimeout = TRUE;
    }

    status = IoRtlEcpListInsert(ecpList,
                                IO_ECP_TYPE_NAMED_PIPE,
                                &pipeParams,
                                sizeof(pipeParams),
                                NULL);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtCtxCreateFile(
                    pConnection,
                    pSecurityToken,
                    &fileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileName,
                    SecurityDescriptor,
                    SecurityQualityOfService,
                    DesiredAccess,
                    0,
                    0,
                    ShareAccess,
                    CreateDisposition,
                    CreateOptions,
                    NULL,
                    0,
                    ecpList);
cleanup:
    IoRtlEcpListFree(&ecpList);

    *FileHandle = fileHandle;
    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxCreateFile(
    IN PIO_CONTEXT pConnection,
    IN LW_PIO_ACCESS_TOKEN pSecurityToken,
    OUT PIO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    IN OPTIONAL PVOID SecurityDescriptor, // TBD
    IN OPTIONAL PVOID SecurityQualityOfService, // TBD
    IN ACCESS_MASK DesiredAccess,
    IN OPTIONAL LONG64 AllocationSize,
    IN FILE_ATTRIBUTES FileAttributes,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN OPTIONAL PVOID EaBuffer, // PFILE_FULL_EA_INFORMATION
    IN ULONG EaLength,
    IN OPTIONAL PIO_ECP_LIST EcpList
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_CREATE_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_CREATE_FILE_RESULT;
    NT_IPC_MESSAGE_CREATE_FILE request = { 0 };
    PNT_IPC_MESSAGE_CREATE_FILE_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_FILE_HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.pSecurityToken = (PIO_ACCESS_TOKEN) pSecurityToken;
    request.FileName = *FileName;
    request.DesiredAccess = DesiredAccess;
    request.AllocationSize = AllocationSize;
    request.FileAttributes = FileAttributes;
    request.ShareAccess = ShareAccess;
    request.CreateDisposition = CreateDisposition;
    request.CreateOptions = CreateOptions;
    request.EaBuffer = EaBuffer;
    request.EaLength = EaLength;
    // TODO -- SDs, etc.
    request.EcpCount = IoRtlEcpListGetCount(EcpList);
    if (request.EcpCount)
    {
        PCSTR pszType = NULL;
        ULONG ecpIndex = 0;

        status = RTL_ALLOCATE(&request.EcpList, NT_IPC_HELPER_ECP, sizeof(*request.EcpList) * request.EcpCount);
        ioStatusBlock.Status = status;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);

        while (ecpIndex < request.EcpCount)
        {
            status = IoRtlEcpListGetNext(
                            EcpList,
                            pszType,
                            &request.EcpList[ecpIndex].pszType,
                            &request.EcpList[ecpIndex].pData,
                            &request.EcpList[ecpIndex].Size);
            ioStatusBlock.Status = status;
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);

            pszType = request.EcpList[ecpIndex].pszType;
            ecpIndex++;
        }

        assert(ecpIndex == request.EcpCount);
        status = STATUS_SUCCESS;
    }

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_CREATE_FILE_RESULT) pReply;

    fileHandle = pResponse->FileHandle;
    ioStatusBlock.Status = pResponse->Status;
    ioStatusBlock.CreateResult = pResponse->CreateResult;

    status = ioStatusBlock.Status;

    /* Unlink file handle from response message so it does not get freed */
    pResponse->FileHandle = NULL;

cleanup:
    if (status)
    {
        // TODO !!!! -- ASK BRIAN ABOUT FileHandle and failures...
        assert(!fileHandle);
        assert(!pResponse || !pResponse->FileHandle);
    }

    RTL_FREE(&request.EcpList);
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    *FileHandle = fileHandle;
    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxCloseFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_CLOSE_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_CLOSE_FILE_RESULT;
    NT_IPC_MESSAGE_GENERIC_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    // In case we add IO_STATUS_BLOCK to close later, which we may want/need.
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.FileHandle = FileHandle;

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    assert(0 == ioStatusBlock.BytesTransferred);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcUnregisterFileHandle(pConnection->pAssoc, FileHandle);

cleanup:
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxReadFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_READ_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_READ_FILE_RESULT;
    NT_IPC_MESSAGE_READ_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.FileHandle = FileHandle;
    request.Length = Length;
    request.ByteOffset = ByteOffset;
    request.Key = Key;

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, Buffer, Length, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxWriteFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_WRITE_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_WRITE_FILE_RESULT;
    NT_IPC_MESSAGE_WRITE_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.FileHandle = FileHandle;
    request.Buffer = Buffer;
    request.Length = Length;
    request.ByteOffset = ByteOffset;
    request.Key = Key;

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    assert(ioStatusBlock.BytesTransferred <= Length);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS 
LwNtCtxDeviceIoControlFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_DEVICE_IO_CONTROL_FILE_RESULT;
    NT_IPC_MESSAGE_GENERIC_CONTROL_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.FileHandle = FileHandle;
    request.ControlCode = IoControlCode;
    request.InputBuffer = InputBuffer;
    request.InputBufferLength = InputBufferLength;
    request.OutputBufferLength = OutputBufferLength;

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, OutputBuffer, OutputBufferLength, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxFsControlFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_FS_CONTROL_FILE_RESULT;
    NT_IPC_MESSAGE_GENERIC_CONTROL_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.FileHandle = FileHandle;
    request.ControlCode = FsControlCode;
    request.InputBuffer = InputBuffer;
    request.InputBufferLength = InputBufferLength;
    request.OutputBufferLength = OutputBufferLength;

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, OutputBuffer, OutputBufferLength, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxFlushBuffersFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_FLUSH_BUFFERS_FILE_RESULT;
    NT_IPC_MESSAGE_GENERIC_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.FileHandle = FileHandle;

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
LwNtCtxQueryInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_QUERY_INFORMATION_FILE_RESULT;
    NT_IPC_MESSAGE_QUERY_INFORMATION_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.FileHandle = FileHandle;
    request.Length = Length;
    request.FileInformationClass = FileInformationClass;

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, FileInformation, Length, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS 
LwNtCtxSetInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_SET_INFORMATION_FILE_RESULT;
    NT_IPC_MESSAGE_SET_INFORMATION_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.FileHandle = FileHandle;
    request.FileInformation = FileInformation;
    request.Length = Length;
    request.FileInformationClass = FileInformationClass;

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    status = NtpCtxGetIoResult(&ioStatusBlock, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

//
// Additional Operations
//

NTSTATUS
LwNtCtxQueryFullAttributesFile(
    IN PIO_CONTEXT pConnection,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

NTSTATUS 
LwNtCtxQueryDirectoryFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN OPTIONAL PIO_FILE_SPEC FileSpec,
    IN BOOLEAN RestartScan
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    const LWMsgMessageTag requestType = NT_IPC_MESSAGE_TYPE_QUERY_DIRECTORY_FILE;
    const LWMsgMessageTag responseType = NT_IPC_MESSAGE_TYPE_QUERY_DIRECTORY_FILE_RESULT;
    NT_IPC_MESSAGE_QUERY_DIRECTORY_FILE request = { 0 };
    PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT pResponse = NULL;
    PVOID pReply = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    request.FileHandle = FileHandle;
    request.Length = Length;
    request.FileInformationClass = FileInformationClass;
    request.ReturnSingleEntry = ReturnSingleEntry;
    request.FileSpec = FileSpec;
    request.RestartScan = RestartScan;

    status = NtpCtxCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_BUFFER_RESULT) pReply;

    status = NtpCtxGetBufferResult(&ioStatusBlock, FileInformation, Length, pResponse);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    NtpCtxFreeResponse(pConnection, responseType, pResponse);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}


NTSTATUS
LwNtCtxQueryVolumeInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS
LwNtCtxSetVolumeInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS 
LwNtCtxLockFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock
    );

NTSTATUS 
LwNtCtxUnlockFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key
    );

//
// Namespace Operations
//
// These are in flux due NT vs POSIX issues.
//

NTSTATUS
LwNtCtxRemoveDirectoryFile(
    IN PIO_CONTEXT pConnection,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
LwNtCtxDeleteFile(
    IN PIO_CONTEXT pConnection,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
LwNtCtxLinkFile(
    IN PIO_CONTEXT pConnection,
    IN PIO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME LinkName
    );

NTSTATUS
LwNtCtxRenameFile(
    IN PIO_CONTEXT pConnection,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FromName,
    IN PIO_FILE_NAME ToName
    );

//
// Advanced Operations
//

NTSTATUS
LwNtCtxQueryQuotaInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN OPTIONAL PVOID SidList,
    IN ULONG SidListLength,
    IN OPTIONAL PSID StartSid,
    IN BOOLEAN RestartScan
    );

NTSTATUS
LwNtCtxSetQuotaInformationFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
LwNtCtxQuerySecurityFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE  Handle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG LengthNeeded
    ); 

NTSTATUS
LwNtCtxSetSecurityFile(
    IN PIO_CONTEXT pConnection,
    IN IO_FILE_HANDLE Handle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    ); 

// TODO: QueryEaFile and SetEaFile.

