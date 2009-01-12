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
 *        libmain.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Listener Main
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LIBMAIN_H__
#define __LIBMAIN_H__

DWORD
SrvCreateFileEx(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    PCWSTR pwszFileName,
    DWORD   dwDesiredAccess,
    DWORD   dwSharedMode,
    DWORD   dwCreationDisposition,
    DWORD   dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE phFile
    );

DWORD
SrvReadFileEx(
    HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    );

DWORD
SrvWriteFileEx(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    );

DWORD
SrvGetSessionKey(
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    );

DWORD
SrvCloseFileEx(
    HANDLE hFile
    );

NTSTATUS
SrvTreeConnect(
    PCWSTR  pszShareName,
    TID * pTid
    );

NTSTATUS
SrvNTCreate(
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
    );

NTSTATUS
SrvNTTransactCreate(
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
    );

NTSTATUS
SrvCreateTemporary(
    HANDLE hTreeObject,
    PWSTR pszDirectoryName,
    PWSTR * ppszFileName,
    USHORT * usFid
    );

NTSTATUS
SrvReadFile(
    HANDLE hTreeObject,
    USHORT usFid,
    ULONG ulOffset,
    UCHAR  *pBuffer,
    USHORT MaxCount
    );

NTSTATUS
SrvWriteFile(
    HANDLE hTreeObject,
    USHORT WriteMode,
    UCHAR *pBuffer,
    USHORT usByteCount,
    USHORT *pusBytesWritten
    );

NTSTATUS
SrvLockFile(
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
SrvSeekFile(
    HANDLE hTreeObject,
    USHORT usFid,
    USHORT Mode,
    ULONG Offset,
    ULONG * plReturnedOffset
    );

NTSTATUS
SrvFlushFile(
    HANDLE hTreeObject
    );

NTSTATUS
SrvCloseFile(
    HANDLE hTreeObject
    );

NTSTATUS
SrvCloseFileAndDisconnect(
    HANDLE hTreeObject,
    USHORT usFid
    );

NTSTATUS
SrvRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    PWSTR pszOldFileName,
    PWSTR pszNewFileName
    );

NTSTATUS
SrvCopyFile(
    HANDLE hTreeObject,
    HANDLE hTreeObject2,
    USHORT OpenFunction,
    USHORT Flags,
    UCHAR SourceFileFormat,
    PWSTR SourceFileName,
    UCHAR TargetFileFormat,
    PWSTR TargetFileName
    );

NTSTATUS
SrvTrans2QueryPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    );

NTSTATUS
SrvTrans2QueryFileInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    );

NTSTATUS
SrvTrans2SetPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    );

NTSTATUS
SrvTrans2CreateDirectory(
    HANDLE hTreeObject
    );

NTSTATUS
SrvTrans2DeleteDirectory(
    HANDLE hTreeObject,
    PWSTR DirectoryName[]
    );

NTSTATUS
SrvTrans2CheckDirectory(
    HANDLE hTreeObject,
    PWSTR DirectoryName[]
    );

NTSTATUS
SrvTrans2FindNext2(
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
    );

NTSTATUS
SrvNTTransactNotifyChange(
    HANDLE hTreeObject,
    USHORT Fid,
    BOOLEAN WatchTree,
    UCHAR Reserved
    );

NTSTATUS
SrvTrans2GetDFSReferral(
    HANDLE hTreeObject
    );

#endif /* __LIBMAIN_H__ */
