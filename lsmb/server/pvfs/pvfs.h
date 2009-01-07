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
 *        includes
 *
 * Abstract:
 *
 *        Likewise Posix File System (SMBSS)
 *
 *        Service Entry API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __PVFS_H__
#define __PVFS_H__

DWORD
PVfsCreateFileEx(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszFileName,
    DWORD   dwDesiredAccess,
    DWORD   dwSharedMode,
    DWORD   dwCreationDisposition,
    DWORD   dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE phFile
    );

DWORD
PVfsReadFileEx(
    HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    );

DWORD
PVfsWriteFileEx(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    );

DWORD
PVfsGetSessionKey(
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    );

DWORD
PVfsCloseFileEx(
    HANDLE hFile
    );

NTSTATUS
PVfsTreeConnect(
    PCWSTR  pszShareName,
    TID * pTid
    );

NTSTATUS
PVfsNTCreate(
    HANDLE hTreeObject,
    ULONG RootDirectoryFid,
    ACCESS_MASK DesiredAccess,
    ULONG ExtFileAttributes,
    ULONG ShareAccess,
    ULONG CreateDisposition,
    ULONG CreateOptions,
    ULONG ImpersonationLevel,
    UCHAR SecurityFlags,
    LPWSTR pszFileName,
    FID * pFid
    );

NTSTATUS
PVfsNTTransactCreate(
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
    LPWSTR pszFileName,
    FID * pFid
    );

NTSTATUS
PVfsCreateTemporary(
    HANDLE hTreeObject,
    LPWSTR pszDirectoryName,
    LPWSTR * ppszFileName,
    USHORT * usFid
    );

NTSTATUS
PVfsReadFile(
    HANDLE hTreeObject,
    USHORT usFid,
    ULONG ulOffset,
    UCHAR  *pBuffer,
    USHORT MaxCount
    );

NTSTATUS
PVfsWriteFile(
    HANDLE hTreeObject,
    USHORT WriteMode,
    UCHAR *pBuffer,
    USHORT usByteCount,
    USHORT *pusBytesWritten
    );

NTSTATUS
PVfsLockFile(
    HANDLE hTreeObject,
    USHORT usFid,
    UCHAR LockType,
    ULONG TimeOut,
    USHORT NumberofUnlocks,
    USHORT NumberOfLocks,
    LOCKING_ANDX_RANGE Unlocks[],
    LOCKING_ANDX_RANGE Locks[]
    );

NTSTATUS
PVfsSeekFile(
    HANDLE hTreeObject,
    USHORT usFid,
    USHORT Mode,
    ULONG Offset,
    ULONG * plReturnedOffset
    );

NTSTATUS
PVfsFlushFile(
    HANDLE hTreeObject
    );

NTSTATUS
PVfsCloseFile(
    HANDLE hTreeObject
    );

NTSTATUS
PVfsCloseFileAndDisconnect(
    HANDLE hTreeObject,
    USHORT usFid
    );

NTSTATUS
PVfsDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszFileName
    );

NTSTATUS
PVfsRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszOldFileName,
    LPWSTR pszNewFileName
    );

NTSTATUS
PVfsCopyFile(
    HANDLE hTreeObject,
    HANDLE hTreeObject2,
    USHORT OpenFunction,
    USHORT Flags,
    UCHAR SourceFileFormat,
    LPWSTR SourceFileName,
    UCHAR TargetFileFormat,
    LPWSTR TargetFileName
    );

NTSTATUS
PVfsTrans2QueryPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    LPWSTR FileName
    );

NTSTATUS
PVfsTrans2QueryFileInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    LPWSTR FileName
    );

NTSTATUS
PVfsTrans2SetPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    LPWSTR FileName
    );

NTSTATUS
PVfsTrans2CreateDirectory(
    HANDLE hTreeObject
    );

NTSTATUS
PVfsTrans2DeleteDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    );

NTSTATUS
PVfsTrans2CheckDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    );

NTSTATUS
PVfsTrans2FindFirst2(
    HANDLE hTreeObject,
    USHORT SearchAttributes,
    USHORT Flags,
    USHORT InformationLevel,
    ULONG SearchStorageType,
    LPWSTR FileName,
    USHORT * pusSid,
    USHORT * puSearchCount,
    USHORT * pusEndofSearch,
    USHORT * pusLastNameOffset,
    PVOID * lppBuffer
    );

NTSTATUS
PVfsTrans2FindNext2(
    HANDLE hTreeObject,
    USHORT usSid,
    USHORT SearchCount,
    USHORT InformationLevel,
    ULONG ResumeKey,
    USHORT Flags,
    LPWSTR FileName,
    USHORT * pusSearchCount,
    USHORT * pusEndOfSearch,
    USHORT *pusEaErrorOffset,
    USHORT * pusLastNameOffset
    );

NTSTATUS
PVfsNTTransactNotifyChange(
    HANDLE hTreeObject,
    USHORT Fid,
    BOOLEAN WatchTree,
    UCHAR Reserved
    );

NTSTATUS
PVfsTrans2GetDFSReferral(
    HANDLE hTreeObject
    );

#endif /* __PVFS_H__ */



