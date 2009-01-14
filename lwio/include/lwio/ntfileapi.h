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
 *        ntfileapi.h
 *
 * Abstract:
 *
 *        NT File API
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#ifndef __NT_FILE_API_H__
#define __NT_FILE_API_H__

#include <lwio/io-types.h>

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

LW_NTSTATUS
LwNtCreateFile(
    LW_OUT PIO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FileName,
    LW_IN ACCESS_MASK DesiredAccess,
    LW_IN LW_OPTIONAL LONG64 AllocationSize,
    LW_IN FILE_ATTRIBUTES FileAttributes,
    LW_IN FILE_SHARE_FLAGS ShareAccess,
    LW_IN FILE_CREATE_DISPOSITION CreateDisposition,
    LW_IN FILE_CREATE_OPTIONS CreateOptions,
    LW_IN LW_OPTIONAL PIO_EA_BUFFER pEaBuffer,
    LW_IN LW_OPTIONAL LW_PVOID SecurityDescriptor, // TBD
    LW_IN LW_PVOID SecurityQualityOfService // TBD
    );

LW_NTSTATUS
LwNtCloseFile(
    LW_IN IO_FILE_HANDLE FileHandle
    );

LW_NTSTATUS
LwNtReadFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID Buffer,
    LW_IN LW_ULONG Length,
    LW_IN LW_OPTIONAL LW_PLONG64 ByteOffset,
    LW_IN LW_OPTIONAL LW_PULONG Key
    );

LW_NTSTATUS
LwNtWriteFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_PVOID Buffer,
    LW_IN LW_ULONG Length,
    LW_IN LW_OPTIONAL LW_PLONG64 ByteOffset,
    LW_IN LW_OPTIONAL LW_PULONG Key
    );

LW_NTSTATUS
LwNtDeviceIoControlFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_ULONG IoControlCode,
    LW_IN LW_PVOID InputBuffer,
    LW_IN LW_ULONG InputBufferLength,
    LW_OUT LW_PVOID OutputBuffer,
    LW_IN LW_ULONG OutputBufferLength
    );

LW_NTSTATUS
LwNtFsControlFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_ULONG FsControlCode,
    LW_IN LW_PVOID InputBuffer,
    LW_IN LW_ULONG InputBufferLength,
    LW_OUT LW_PVOID OutputBuffer,
    LW_IN LW_ULONG OutputBufferLength
    );

LW_NTSTATUS
LwNtFlushBuffersFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock
    );

LW_NTSTATUS
LwNtQueryInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID FileInformation,
    LW_IN LW_ULONG Length,
    LW_IN FILE_INFORMATION_CLASS FileInformationClass
    );

LW_NTSTATUS
LwNtSetInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_PVOID FileInformation,
    LW_IN LW_ULONG Length,
    LW_IN FILE_INFORMATION_CLASS FileInformationClass
    );

//
// Additional Operations
//

LW_NTSTATUS
LwNtQueryFullAttributesFile(
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FileName,
    LW_OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

LW_NTSTATUS
LwNtQueryDirectoryFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID FileInformation,
    LW_IN LW_ULONG Length,
    LW_IN FILE_INFORMATION_CLASS FileInformationClass,
    LW_IN LW_BOOLEAN ReturnSingleEntry,
    LW_IN LW_OPTIONAL PIO_FILE_SPEC FileSpec,
    LW_IN IO_NAME_OPTIONS IoNameOptions,
    LW_IN LW_BOOLEAN RestartScan
    );

LW_NTSTATUS
LwNtQueryVolumeInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID FsInformation,
    LW_IN LW_ULONG Length,
    LW_IN FS_INFORMATION_CLASS FsInformationClass
    );

LW_NTSTATUS
LwNtSetVolumeInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_PVOID FsInformation,
    LW_IN LW_ULONG Length,
    LW_IN FS_INFORMATION_CLASS FsInformationClass
    );

LW_NTSTATUS
LwNtLockFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LONG64 ByteOffset,
    LW_IN LONG64 Length,
    LW_IN LW_ULONG Key,
    LW_IN LW_BOOLEAN FailImmediately,
    LW_IN LW_BOOLEAN ExclusiveLock
    );

LW_NTSTATUS
LwNtUnlockFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LONG64 ByteOffset,
    LW_IN LONG64 Length,
    LW_IN LW_ULONG Key
    );

//
// Namespace Operations
//
// These are in flux due NT vs POSIX issues.
//

LW_NTSTATUS
LwNtRemoveDirectoryFile(
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FileName
    );

LW_NTSTATUS
LwNtDeleteFile(
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FileName
    );

LW_NTSTATUS
LwNtLinkFile(
    LW_IN PIO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME LinkName
    );

LW_NTSTATUS
LwNtRenameFile(
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN PIO_FILE_NAME FromName,
    LW_IN PIO_FILE_NAME ToName
    );

LW_NTSTATUS
LwNtQueryQuotaInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_OUT LW_PVOID Buffer,
    LW_IN LW_ULONG Length,
    LW_IN LW_BOOLEAN ReturnSingleEntry,
    LW_IN LW_OPTIONAL LW_PVOID SidList,
    LW_IN LW_ULONG SidListLength,
    LW_IN LW_OPTIONAL PSID StartSid,
    LW_IN LW_BOOLEAN RestartScan
    );

//
// Advanced Operations
//

LW_NTSTATUS
LwNtSetQuotaInformationFile(
    LW_IN IO_FILE_HANDLE FileHandle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN LW_PVOID Buffer,
    LW_IN LW_ULONG Length
    );

LW_NTSTATUS
LwNtQuerySecurityFile(
    LW_IN IO_FILE_HANDLE  Handle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN SECURITY_INFORMATION SecurityInformation,
    LW_OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    LW_IN LW_ULONG Length,
    LW_OUT LW_PULONG LengthNeeded
    );

LW_NTSTATUS
LwNtSetSecurityFile(
    LW_IN IO_FILE_HANDLE Handle,
    LW_IN LW_OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    LW_OUT PIO_STATUS_BLOCK IoStatusBlock,
    LW_IN SECURITY_INFORMATION SecurityInformation,
    LW_IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

// TODO: QueryEaFile and SetEaFile.

#ifndef LW_STRICT_NAMESPACE

#define NtCreateFile                    LwNtCreateFile
#define NtCloseFile                     LwNtCloseFile
#define NtReadFile                      LwNtReadFile
#define NtWriteFile                     LwNtWriteFile
#define NtDeviceIoControlFile           LwNtDeviceIoControlFile
#define NtFsControlFile                 LwNtFsControlFile
#define NtFlushBuffersFile              LwNtFlushBuffersFile
#define NtQueryInformationFile          LwNtQueryInformationFile
#define NtSetInformationFile            LwNtSetInformationFile
#define NtQueryFullAttributesFile       LwNtQueryFullAttributesFile
#define NtQueryDirectoryFile            LwNtQueryDirectoryFile
#define NtQueryVolumeInformationFile    LwNtQueryVolumeInformationFile
#define NtSetVolumeInformationFile      LwNtSetVolumeInformationFile
#define NtLockFile                      LwNtLockFile
#define NtUnlockFile                    LwNtUnlockFile
#define NtRemoveDirectoryFile           LwNtRemoveDirectoryFile
#define NtDeleteFile                    LwNtDeleteFile
#define NtLinkFile                      LwNtLinkFile
#define NtRenameFile                    LwNtRenameFile
#define NtQueryQuotaInformationFile     LwNtQueryQuotaInformationFile
#define NtSetQuotaInformationFile       LwNtSetQuotaInformationFile
#define NtQuerySecurityFile             LwNtQuerySecurityFile
#define NtSetSecurityFile               LwNtSetSecurityFile

#endif /* LW_STRICT_NAMESPACE */

#endif /* __NT_FILE_API__ */
