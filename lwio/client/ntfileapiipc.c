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
#include "ntfileapiipc.h"
#include "ntipcmsg.h"
#include "goto.h"
#include "ntlogmacros.h"

static
VOID
NtpIpcFreeResponse(
    IN PSMB_SERVER_CONNECTION pConnection,
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
NtpIpcCall(
    IN PSMB_SERVER_CONNECTION pConnection,
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
        assert(false);
        status = STATUS_INTERNAL_ERROR;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (status)
    {
        NtpIpcFreeResponse(pConnection, actualResponseType, pResponse);
        pResponse = NULL;
    }

    *ppResponse = pResponse;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
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
NtIpcCreateFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN PSMB_SECURITY_TOKEN_REP pSecurityToken,
    OUT PIO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    IN ACCESS_MASK DesiredAccess,
    IN OPTIONAL LONG64 AllocationSize,
    IN FILE_ATTRIBUTES FileAttributes,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN FILE_CREATE_DISPOSITION CreateDisposition,
    IN FILE_CREATE_OPTIONS CreateOptions,
    IN OPTIONAL PIO_EA_BUFFER pEaBuffer,
    IN OPTIONAL PVOID SecurityDescriptor, // TBD
    IN PVOID SecurityQualityOfService // TBD
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

    request.pSecurityToken = pSecurityToken;
#ifdef _NT_IPC_USE_PSEUDO_TYPES
    NtIpcRealToPseudoIoFileName(FileName, &request.FileName);
#else
    request.FileName = *FileName;
#endif
    request.DesiredAccess = DesiredAccess;
    request.AllocationSize = AllocationSize;
    request.FileAttributes = FileAttributes;
    request.ShareAccess = ShareAccess;
    request.CreateDisposition = CreateDisposition;
    request.CreateOptions = CreateOptions;

    // TODO -- EAs, SDs, etc.

    status = NtpIpcCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_CREATE_FILE_RESULT) pReply;

#ifdef _NT_IPC_USE_PSEUDO_TYPES
    NtIpcRealFromPseudoIoFileHandle(&fileHandle, &pResponse->FileHandle);
#else
    fileHandle = pResponse->FileHandle;
#endif
    ioStatusBlock.Status = pResponse->Status;
    ioStatusBlock.CreateResult = pResponse->CreateResult;

    status = ioStatusBlock.Status;

cleanup:
    if (status)
    {
        // TODO !!!! -- ASK BRIAN ABOUT FileHandle and failures...
        assert(!fileHandle);
#ifdef _NT_IPC_USE_PSEUDO_TYPES
        assert(!pResponse || !pResponse->FileHandle.IsValid);
#else
        assert(!pResponse || !pResponse->FileHandle);
#endif
    }

    NtpIpcFreeResponse(pConnection, responseType, pResponse);

    *FileHandle = fileHandle;
    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
NtIpcCloseFile(
    IN PSMB_SERVER_CONNECTION pConnection,
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

    status = NtpIpcCall(pConnection,
                        requestType,
                        &request,
                        responseType,
                        &pReply);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pResponse = (PNT_IPC_MESSAGE_GENERIC_FILE_IO_RESULT) pReply;

    ioStatusBlock.Status = pResponse->Status;
    assert(0 == pResponse->BytesTransferred);

    status = ioStatusBlock.Status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcUnregisterFileHandle(pConnection->pAssoc, FileHandle);

cleanup:
    NtpIpcFreeResponse(pConnection, responseType, pResponse);

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
NtIpcReadFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtIpcWriteFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
NtIpcDeviceIoControlFile(
    IN PSMB_SERVER_CONNECTION pConnection,
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
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtIpcFsControlFile(
    IN PSMB_SERVER_CONNECTION pConnection,
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
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NtIpcFlushBuffersFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
NtIpcQueryInformationFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS 
NtIpcSetInformationFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

//
// Additional Operations
//

NTSTATUS
NtIpcQueryFullAttributesFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

NTSTATUS 
NtIpcQueryDirectoryFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass,
    IN BOOLEAN ReturnSingleEntry,
    IN OPTIONAL PIO_FILE_SPEC FileSpec,
    IN IO_NAME_OPTIONS IoNameOptions,
    IN BOOLEAN RestartScan
    );

NTSTATUS
NtIpcQueryVolumeInformationFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS
NtIpcSetVolumeInformationFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS 
NtIpcLockFile(
    IN PSMB_SERVER_CONNECTION pConnection,
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
NtIpcUnlockFile(
    IN PSMB_SERVER_CONNECTION pConnection,
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
NtIpcRemoveDirectoryFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
NtIpcDeleteFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
NtIpcLinkFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN PIO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME LinkName
    );

NTSTATUS
NtIpcRenameFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FromName,
    IN PIO_FILE_NAME ToName
    );

NTSTATUS
NtIpcQueryQuotaInformationFile(
    IN PSMB_SERVER_CONNECTION pConnection,
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

//
// Advanced Operations
//

NTSTATUS
NtIpcSetQuotaInformationFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
NtIpcQuerySecurityFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE  Handle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG LengthNeeded
    ); 

NTSTATUS
NtIpcSetSecurityFile(
    IN PSMB_SERVER_CONNECTION pConnection,
    IN IO_FILE_HANDLE Handle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    ); 

// TODO: QueryEaFile and SetEaFile.

