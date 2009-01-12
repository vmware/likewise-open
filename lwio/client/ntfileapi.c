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
 *        ntfileapi.c
 *
 * Abstract:
 *
 *        NT File API Implementation
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"
#include "ntfileapi.h"
#include "ntfileapiipc.h"
#include "goto.h"
#include "ntlogmacros.h"

// TODO-Clean up security token API a little.
static
NTSTATUS
NtpGetSecurityToken(
    OUT PSMB_SECURITY_TOKEN_REP* ppSecurityToken
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    DWORD dwError = 0;
    PSMB_CLIENT_CONTEXT pClientContext = NULL;
    PSMB_SECURITY_TOKEN_REP pSecurityToken = NULL;
    
    dwError = SMBGetClientContext(&pClientContext);
    if (dwError)
    {
        SMB_LOG_ERROR("Failed to get client context (dwError = %u)", dwError);
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    dwError = SMBAPIHandleGetSecurityToken(pClientContext->hAccessToken, &pSecurityToken);
    if (dwError)
    {
        SMB_LOG_ERROR("Failed to get security token (dwError = %u)", dwError);
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    *ppSecurityToken = pSecurityToken;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
NtpAcquireConnection(
    OUT PSMB_SERVER_CONNECTION pConnection
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection = { 0 };

    dwError = SMBAcquireConnection(&connection);
    if (dwError)
    {
        SMB_LOG_ERROR("Failed to acquire connection (dwError = %u)", dwError);
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

cleanup:
    if (status)
    {
        SMBReleaseConnection(&connection);
    }

    *pConnection = connection;

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

static
VOID
NtpReleaseConnection(
    IN OUT PSMB_SERVER_CONNECTION pConnection
    )
{
    SMBReleaseConnection(pConnection);
}

static
VOID
NtpInitializeIoStatusBlock(
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    memset(IoStatusBlock, 0, sizeof(*IoStatusBlock));
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
NtCreateFile(
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
    IN OPTIONAL PVOID SecurityQualityOfService // TBD
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    SMB_SERVER_CONNECTION connection = { 0 };
    PSMB_SECURITY_TOKEN_REP pSecurityToken = NULL;

    *FileHandle = NULL;
    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = NtpAcquireConnection(&connection);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtpGetSecurityToken(&pSecurityToken);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcCreateFile(
                    &connection,
                    pSecurityToken,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileName,
                    DesiredAccess,
                    AllocationSize,
                    FileAttributes,
                    ShareAccess,
                    CreateDisposition,
                    CreateOptions,
                    pEaBuffer,
                    SecurityDescriptor,
                    SecurityQualityOfService);

cleanup:
    NtpReleaseConnection(&connection);
    return status;
}

NTSTATUS
NtCloseFile(
    IN IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    SMB_SERVER_CONNECTION connection = { 0 };

    status = NtpAcquireConnection(&connection);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcCloseFile(
                    &connection,
                    FileHandle);

cleanup:
    NtpReleaseConnection(&connection);
    return status;
}

NTSTATUS
NtReadFile(
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
    SMB_SERVER_CONNECTION connection = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = NtpAcquireConnection(&connection);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcReadFile(
                    &connection,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    Buffer,
                    Length,
                    ByteOffset,
                    Key);

cleanup:
    NtpReleaseConnection(&connection);
    return status;
}

NTSTATUS
NtWriteFile(
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
    SMB_SERVER_CONNECTION connection = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = NtpAcquireConnection(&connection);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcWriteFile(
                    &connection,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    Buffer,
                    Length,
                    ByteOffset,
                    Key);

cleanup:
    NtpReleaseConnection(&connection);
    return status;
}

NTSTATUS 
NtDeviceIoControlFile(
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
    SMB_SERVER_CONNECTION connection = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = NtpAcquireConnection(&connection);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcDeviceIoControlFile(
                    &connection,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    IoControlCode,
                    InputBuffer,
                    InputBufferLength,
                    OutputBuffer,
                    OutputBufferLength);

cleanup:
    NtpReleaseConnection(&connection);
    return status;
}

NTSTATUS
NtFsControlFile(
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
    SMB_SERVER_CONNECTION connection = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = NtpAcquireConnection(&connection);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcFsControlFile(
                    &connection,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FsControlCode,
                    InputBuffer,
                    InputBufferLength,
                    OutputBuffer,
                    OutputBufferLength);

cleanup:
    NtpReleaseConnection(&connection);
    return status;
}

NTSTATUS
NtFlushBuffersFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    SMB_SERVER_CONNECTION connection = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = NtpAcquireConnection(&connection);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcFlushBuffersFile(
                    &connection,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock);

cleanup:
    NtpReleaseConnection(&connection);
    return status;
}

NTSTATUS 
NtQueryInformationFile(
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
    SMB_SERVER_CONNECTION connection = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = NtpAcquireConnection(&connection);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcQueryInformationFile(
                    &connection,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileInformation,
                    Length,
                    FileInformationClass);

cleanup:
    NtpReleaseConnection(&connection);
    return status;
}

NTSTATUS 
NtSetInformationFile(
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
    SMB_SERVER_CONNECTION connection = { 0 };

    NtpInitializeIoStatusBlock(IoStatusBlock);

    status = NtpAcquireConnection(&connection);
    IoStatusBlock->Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = NtIpcSetInformationFile(
                    &connection,
                    FileHandle,
                    AsyncControlBlock,
                    IoStatusBlock,
                    FileInformation,
                    Length,
                    FileInformationClass);

cleanup:
    NtpReleaseConnection(&connection);
    return status;
}

//
// Additional Operations
//

NTSTATUS
NtQueryFullAttributesFile(
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

NTSTATUS 
NtQueryDirectoryFile(
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
NtQueryVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS
NtSetVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    );

NTSTATUS 
NtLockFile(
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
NtUnlockFile(
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
NtRemoveDirectoryFile(
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
NtDeleteFile(
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    );

NTSTATUS
NtLinkFile(
    IN PIO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME LinkName
    );

NTSTATUS
NtRenameFile(
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FromName,
    IN PIO_FILE_NAME ToName
    );

NTSTATUS
NtQueryQuotaInformationFile(
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
NtSetQuotaInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    );

NTSTATUS
NtQuerySecurityFile(
    IN IO_FILE_HANDLE  Handle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG LengthNeeded
    ); 

NTSTATUS
NtSetSecurityFile(
    IN IO_FILE_HANDLE Handle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    ); 

// TODO: QueryEaFile and SetEaFile.

