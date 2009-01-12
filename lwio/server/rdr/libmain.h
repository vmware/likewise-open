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


NTSTATUS
RdrTreeConnect(
    PCWSTR  pszShareName,
    TID * pTid
    );

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
    );

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
    );

NTSTATUS
RdrCreateTemporary(
    HANDLE hTreeObject,
    PWSTR pszDirectoryName,
    PWSTR * ppszFileName,
    USHORT * usFid
    );

NTSTATUS
RdrReadFile(
    HANDLE hTreeObject,
    USHORT usFid,
    ULONG ulOffset,
    UCHAR  *pBuffer,
    USHORT MaxCount
    );

NTSTATUS
RdrWriteFile(
    HANDLE hTreeObject,
    USHORT WriteMode,
    UCHAR *pBuffer,
    USHORT usByteCount,
    USHORT *pusBytesWritten
    );

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
    );

NTSTATUS
RdrSeekFile(
    HANDLE hTreeObject,
    USHORT usFid,
    USHORT Mode,
    ULONG Offset,
    ULONG * plReturnedOffset
    );

NTSTATUS
RdrFlushFile(
    HANDLE hTreeObject
    );

NTSTATUS
RdrCloseFile(
    HANDLE hTreeObject
    );

NTSTATUS
RdrCloseFileAndDisconnect(
    HANDLE hTreeObject,
    USHORT usFid
    );

NTSTATUS
RdrDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    PWSTR pszFileName
    );

NTSTATUS
RdrRenameFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    PWSTR pszOldFileName,
    PWSTR pszNewFileName
    );

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
    );

NTSTATUS
RdrTrans2QueryPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    );

NTSTATUS
RdrTrans2QueryFileInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    );

NTSTATUS
RdrTrans2SetPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    PWSTR FileName
    );

NTSTATUS
RdrTrans2CreateDirectory(
    HANDLE hTreeObject
    );

NTSTATUS
RdrTrans2DeleteDirectory(
    HANDLE hTreeObject,
    PWSTR DirectoryName[]
    );

NTSTATUS
RdrTrans2CheckDirectory(
    HANDLE hTreeObject,
    PWSTR DirectoryName[]
    );

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
    );

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
    );

NTSTATUS
RdrNTTransactNotifyChange(
    HANDLE hTreeObject,
    USHORT Fid,
    BOOLEAN WatchTree,
    UCHAR Reserved
    );

NTSTATUS
RdrTrans2GetDFSReferral(
    HANDLE hTreeObject
    );

int
RefOrCreateSocket(
    uchar8_t    *pszHostname,
    SMB_SOCKET **ppSocket
    );

uint32_t
RefOrCreateAuthSession(
    SMB_SOCKET   *pSocket,
    uchar8_t     *pszPrincipal,
    SMB_SESSION **ppSession
    );

uint32_t
ReleaseAuthSession(
    SMB_SESSION *pSession
    );

uint32_t
RefOrCreateTree(
    SMB_SESSION *pSession,
    uchar8_t    *pwszPath,
    SMB_TREE   **pTree
    );

