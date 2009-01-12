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
 *        dependencies.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        SMB Dependency Management
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

DWORD
VFSInitializeProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszProviderName,
    PNTVFS_DRIVER* ppFnTable
    )
{
    DWORD dwError = 0;

    pthread_rwlock_init(&gSocketHashLock, NULL);

    dwError = RdrSocketInit();
    BAIL_ON_SMB_ERROR(dwError);

    dwError = RdrReaperStart();
    BAIL_ON_SMB_ERROR(dwError);

    *ppszProviderName = gpszRdrProviderName;
    *ppFnTable = &gRdrProviderTable;

cleanup:

    return dwError;

error:

    *ppszProviderName = NULL;
    *ppFnTable = NULL;

    goto cleanup;
}

NTSTATUS
RdrTreeConnect(
    PCWSTR  pszShareName,
    TID * pTid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrNTCreate(
    HANDLE hTreeObject,
    ULONG RootDirectoryFid,
    ACCESS_MASK DesiredAccess,
    ULONG ExtFileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    ULONG ImpersonationLevel,
    UCHAR SecurityFlags,
    PWSTR pszFileName,
    FID * pFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrNTTransactCreate(
    HANDLE hTreeObject,
    ULONG RootDirectoryFid,
    ULONG ulFlags,
    ACCESS_MASK DesiredAccess,
    ULONG ExtFileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    ULONG SecurityDescriptorLength,
    ULONG EaLengyh,
    ULONG ImpersonationLevel,
    PWSTR pszFileName,
    FID * pFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrCreateTemporary(
    HANDLE hTreeObject,
    PWSTR pszDirectoryName,
    PWSTR * ppszFileName,
    USHORT * usFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrReadFile(
    HANDLE hTreeObject,
    USHORT usFid,
    ULONG ulOffset,
    UCHAR  *pBuffer,
    USHORT MaxCount
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrWriteFile(
    HANDLE hTreeObject,
    USHORT WriteMode,
    UCHAR *pBuffer,
    USHORT usByteCount,
    USHORT *pusBytesWritten
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrLockFile(
    HANDLE hTreeObject,
    USHORT usFid,
    UCHAR LockType,
    ULONG TimeOut,
    USHORT NumberofUnlocks,
    USHORT NumberOfLocks,
    LOCKING_ANDX_RANGE Unlocks[],
    LOCKING_ANDX_RANGE Locks[]
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrSeekFile(
    HANDLE hTreeObject,
    USHORT usFid,
    USHORT Mode,
    ULONG Offset,
    ULONG * plReturnedOffset
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrFlushFile(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrCloseFile(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrCloseFileAndDisconnect(
    HANDLE hTreeObject,
    USHORT usFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    PWSTR pszFileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    PWSTR pszOldFileName,
    PWSTR pszNewFileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrCopyFile(
    HANDLE hTreeObject,
    HANDLE hTreeObject2,
    USHORT OpenFunction,
    USHORT Flags,
    UCHAR SourceFileFormat,
    PWSTR SourceFileName,
    UCHAR TargetFileFormat,
    PWSTR TargetFileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrTrans2QueryPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrTrans2QueryFileInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrTrans2SetPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrTrans2CreateDirectory(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrTrans2DeleteDirectory(
    HANDLE hTreeObject,
    PWSTR DirectoryName[]
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrTrans2CheckDirectory(
    HANDLE hTreeObject,
    PWSTR DirectoryName[]
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrTrans2FindFirst2(
    HANDLE hTreeObject,
    USHORT SearchAttributes,
    USHORT Flags,
    USHORT InformationLevel,
    ULONG SearchStorageType,
    PWSTR FileName,
    USHORT * pusSid,
    USHORT * puSearchCount,
    USHORT * pusEndofSearch,
    USHORT * pusLastNameOffset,
    PVOID * lppBuffer
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrTrans2FindNext2(
    HANDLE hTreeObject,
    USHORT usSid,
    USHORT SearchCount,
    USHORT InformationLevel,
    ULONG ResumeKey,
    USHORT Flags,
    PWSTR FileName,
    USHORT * pusSearchCount,
    USHORT * pusEndOfSearch,
    USHORT *pusEaErrorOffset,
    USHORT * pusLastNameOffset
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrNTTransactNotifyChange(
    HANDLE hTreeObject,
    USHORT Fid,
    BOOLEAN WatchTree,
    UCHAR Reserved
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RdrTrans2GetDFSReferral(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
VFSShutdownProvider(
    PSTR pszProviderName,
    PNTVFS_DRIVER pFnTable
    )
{
    DWORD dwError = 0;

    dwError = RdrSocketShutdown();
    BAIL_ON_SMB_ERROR(dwError);

    dwError = RdrReaperStop();
    BAIL_ON_SMB_ERROR(dwError);

    pthread_rwlock_destroy(&gSocketHashLock);

error:

    return dwError;
}

