/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Named Pipes File System (NPVFS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
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

    BAIL_ON_INVALID_POINTER(ppszProviderName);

    *ppszProviderName = gpszNPVFSProviderName;
    *ppFnTable = &gNPVFSProviderTable;

cleanup:

    return dwError;

error:

    *ppszProviderName = NULL;
    *ppFnTable = NULL;

    goto cleanup;
}

DWORD
NPVfsCreateFileEx(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    PCWSTR pwszFileName,
    DWORD   dwDesiredAccess,
    DWORD   dwSharedMode,
    DWORD   dwCreationDisposition,
    DWORD   dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE phFile
    )
{
    return STATUS_NOT_SUPPORTED;
}

DWORD
NPVfsReadFileEx(
    HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
NPVfsWriteFileEx(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
NPVfsGetSessionKey(
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
NPVfsCloseFileEx(
    HANDLE hFile
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
NTSTATUS
NPVfsTreeConnect(
    PCWSTR  pszShareName,
    HANDLE * phTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsNTCreate(
    HANDLE hTreeObject,
    ULONG hRootDirectoryObject,
    ACCESS_MASK DesiredAccess,
    ULONG ExtFileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    ULONG ImpersonationLevel,
    UCHAR SecurityFlags,
    PWSTR pszFileName,
    HANDLE * phFileObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsNTTransactCreate(
    HANDLE hTreeObject,
    ULONG hRootDirectoryObject,
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
    HANDLE * phFileObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsCreateTemporary(
    HANDLE hTreeObject,
    PWSTR pszDirectoryName,
    PWSTR * ppszFileName,
    USHORT * usFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsReadFile(
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
NPVfsWriteFile(
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
NPVfsLockFile(
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
NPVfsSeekFile(
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
NPVfsFlushFile(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsCloseFile(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsCloseFileAndDisconnect(
    HANDLE hTreeObject,
    USHORT usFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    PWSTR pszFileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    PWSTR pszOldFileName,
    PWSTR pszNewFileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsCopyFile(
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
NPVfsTrans2QueryPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsTrans2QueryFileInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsTrans2SetPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsTrans2CreateDirectory(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsTrans2DeleteDirectory(
    HANDLE hTreeObject,
    PWSTR DirectoryName[]
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsTrans2CheckDirectory(
    HANDLE hTreeObject,
    PWSTR DirectoryName[]
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsTrans2FindFirst2(
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
NPVfsTrans2FindNext2(
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
NPVfsNTTransactNotifyChange(
    HANDLE hTreeObject,
    USHORT Fid,
    BOOLEAN WatchTree,
    UCHAR Reserved
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NPVfsTrans2GetDFSReferral(
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
    return 0;
}
