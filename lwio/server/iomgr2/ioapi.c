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
 *        ioapi.c
 *
 * Abstract:
 *
 *        IO Manager File API Implementation
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "iop.h"

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
IoCreateFile(
    OUT PIO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_CREATE_SECURITY_CONTEXT SecurityContext,
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
    IO_FILE_NAME fileName = *FileName;
    PIO_DEVICE_OBJECT pDevice = NULL;
    PWSTR pszFileName = NULL;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    PIO_FILE_OBJECT pFileObject = NULL;

    if (AsyncControlBlock ||
        pEaBuffer ||
        SecurityDescriptor ||
        SecurityQualityOfService)
    {
        // Not yet implemented.
        assert(FALSE);
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    // TODO -- Add basic param validation...

    status = IopParse(&fileName, &pDevice);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (fileName.FileName)
    {
        status = RtlWC16StringDuplicate(&pszFileName, fileName.FileName);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = IopFileObjectAllocate(&pFileObject, pDevice);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopIrpCreate(&pIrp, IRP_TYPE_CREATE, pFileObject);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.Create.SecurityContext = SecurityContext;

    pIrp->Args.Create.FileName = fileName;
    pIrp->Args.Create.FileName.FileName = pszFileName;
    pszFileName = NULL;

    pIrp->Args.Create.DesiredAccess = DesiredAccess;
    pIrp->Args.Create.AllocationSize = AllocationSize;
    pIrp->Args.Create.FileAttributes = FileAttributes;
    pIrp->Args.Create.ShareAccess = ShareAccess;
    pIrp->Args.Create.CreateDisposition = CreateDisposition;
    pIrp->Args.Create.CreateOptions = CreateOptions;
    pIrp->Args.Create.pEaBuffer = pEaBuffer;
    pIrp->Args.Create.SecurityDescriptor = SecurityDescriptor;
    pIrp->Args.Create.SecurityQualityOfService = SecurityQualityOfService;

    status = IopDeviceCallDriver(pDevice, pIrp);
    ioStatusBlock = pIrp->IoStatusBlock;
    // TODO -- handle asyc behavior.
    assert(ioStatusBlock.Status == status);

cleanup:
    RTL_FREE(&pszFileName);
    if (pIrp)
    {
        pszFileName = (PWSTR) pIrp->Args.Create.FileName.FileName;
        RTL_FREE(&pszFileName);
    }
    IopIrpFree(&pIrp);

    // TODO -- handle asych behavior.
    if (status)
    {
        // Note that the IRP must be freed before the File Object.
        ioStatusBlock.Status = status;
        IopFileObjectFree(&pFileObject);
    }

    *FileHandle = pFileObject;
    *IoStatusBlock = ioStatusBlock;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

NTSTATUS
IoCloseFile(
    IN OUT IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    // In case we need it in the future...
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    PIO_FILE_OBJECT pFileObject = FileHandle;

    status = IopIrpCreate(&pIrp, IRP_TYPE_CLOSE, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopDeviceCallDriver(FileHandle->pDevice, pIrp);
    ioStatusBlock = pIrp->IoStatusBlock;
    // TODO -- handle asyc behavior.
    assert(ioStatusBlock.Status == status);

cleanup:
    IopIrpFree(&pIrp);
    if (!status)
    {
        IopFileObjectFree(&pFileObject);
    }

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
IopReadWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN BOOLEAN bIsWrite,
    IN OUT PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsWrite ? IRP_TYPE_WRITE : IRP_TYPE_READ;

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.ReadWrite.Buffer = Buffer;
    pIrp->Args.ReadWrite.Length = Length;
    pIrp->Args.ReadWrite.ByteOffset = ByteOffset;
    pIrp->Args.ReadWrite.Key = Key;

    status = IopDeviceCallDriver(FileHandle->pDevice, pIrp);
    ioStatusBlock = pIrp->IoStatusBlock;
    // TODO -- handle asyc behavior.
    assert(ioStatusBlock.Status == status);

cleanup:
    IopIrpFree(&pIrp);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsWrite ? "Write" : "Read");
    return status;
}

NTSTATUS
IoReadFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    return IopReadWriteFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                FALSE,
                Buffer,
                Length,
                ByteOffset,
                Key);
}

NTSTATUS
IoWriteFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length,
    IN OPTIONAL PLONG64 ByteOffset,
    IN OPTIONAL PULONG Key
    )
{
    return IopReadWriteFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                TRUE,
                Buffer,
                Length,
                ByteOffset,
                Key);
}

static
NTSTATUS
IopControlFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN BOOLEAN bIsFsControl,
    IN ULONG ControlCode,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsFsControl? IRP_TYPE_FS_CONTROL : IRP_TYPE_DEVICE_IO_CONTROL;

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.IoFsControl.ControlCode = ControlCode;
    pIrp->Args.IoFsControl.InputBuffer = InputBuffer;
    pIrp->Args.IoFsControl.InputBufferLength = InputBufferLength;
    pIrp->Args.IoFsControl.OutputBuffer = OutputBuffer;
    pIrp->Args.IoFsControl.OutputBufferLength = OutputBufferLength;

    status = IopDeviceCallDriver(FileHandle->pDevice, pIrp);
    ioStatusBlock = pIrp->IoStatusBlock;
    // TODO -- handle asyc behavior.
    assert(ioStatusBlock.Status == status);

cleanup:
    IopIrpFree(&pIrp);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsFsControl ? "FsControl" : "DeviceIoControl" );
    return status;
}

NTSTATUS 
IoDeviceIoControlFile(
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
    return IopControlFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                FALSE,
                IoControlCode,
                InputBuffer,
                InputBufferLength,
                OutputBuffer,
                OutputBufferLength);
}

NTSTATUS
IoFsControlFile(
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
    return IopControlFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                TRUE,
                FsControlCode,
                InputBuffer,
                InputBufferLength,
                OutputBuffer,
                OutputBufferLength);
}

NTSTATUS
IoFlushBuffersFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };

    status = IopIrpCreate(&pIrp, IRP_TYPE_FLUSH_BUFFERS, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopDeviceCallDriver(FileHandle->pDevice, pIrp);
    ioStatusBlock = pIrp->IoStatusBlock;
    // TODO -- handle asyc behavior.
    assert(ioStatusBlock.Status == status);

cleanup:
    IopIrpFree(&pIrp);

    *IoStatusBlock = ioStatusBlock;

    IO_LOG_LEAVE_ON_STATUS_EE(status, EE);
    return status;
}

static
NTSTATUS
IopQuerySetInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN BOOLEAN bIsSet,
    IN OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIRP pIrp = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IRP_TYPE irpType = bIsSet ? IRP_TYPE_SET_INFORMATION : IRP_TYPE_QUERY_INFORMATION;

    status = IopIrpCreate(&pIrp, irpType, FileHandle);
    ioStatusBlock.Status = status;
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    pIrp->Args.QuerySetInformation.FileInformation = FileInformation;
    pIrp->Args.QuerySetInformation.Length = Length;
    pIrp->Args.QuerySetInformation.FileInformationClass = FileInformationClass;

    status = IopDeviceCallDriver(FileHandle->pDevice, pIrp);
    ioStatusBlock = pIrp->IoStatusBlock;
    // TODO -- handle asyc behavior.
    assert(ioStatusBlock.Status == status);

cleanup:
    IopIrpFree(&pIrp);

    *IoStatusBlock = ioStatusBlock;

    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "op = %s", bIsSet ? "Set" : "Query");
    return status;
}

NTSTATUS 
IoQueryInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    return IopQuerySetInformationFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                FALSE,
                FileInformation,
                Length,
                FileInformationClass);
}

NTSTATUS 
IoSetInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
    )
{
    return IopQuerySetInformationFile(
                FileHandle,
                AsyncControlBlock,
                IoStatusBlock,
                TRUE,
                FileInformation,
                Length,
                FileInformationClass);
}


//
// Additional Operations
//

NTSTATUS
IoQueryFullAttributesFile(
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName,
    OUT PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS 
IoQueryDirectoryFile(
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
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoQueryVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoSetVolumeInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS 
IoLockFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key,
    IN BOOLEAN FailImmediately,
    IN BOOLEAN ExclusiveLock
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS 
IoUnlockFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN LONG64 ByteOffset,
    IN LONG64 Length,
    IN ULONG Key
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

//
// Namespace Operations
//
// These are in flux due NT vs POSIX issues.
//

NTSTATUS
IoRemoveDirectoryFile(
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoDeleteFile(
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FileName
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoLinkFile(
    IN PIO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME LinkName
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoRenameFile(
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_FILE_NAME FromName,
    IN PIO_FILE_NAME ToName
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoQueryQuotaInformationFile(
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
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

//
// Advanced Operations
//

NTSTATUS
IoSetQuotaInformationFile(
    IN IO_FILE_HANDLE FileHandle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN PVOID Buffer,
    IN ULONG Length
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoQuerySecurityFile(
    IN IO_FILE_HANDLE  Handle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN ULONG Length,
    OUT PULONG LengthNeeded
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

NTSTATUS
IoSetSecurityFile(
    IN IO_FILE_HANDLE Handle,
    IN OPTIONAL PIO_ASYNC_CONTROL_BLOCK AsyncControlBlock,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    return status;
}

// TODO: QueryEaFile and SetEaFile.

