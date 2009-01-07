/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        main.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Listener Main
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
SrvCreateFileEx(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszFileName,
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
SrvReadFileEx(
    HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
SrvWriteFileEx(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
SrvGetSessionKey(
    HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
SrvCloseFileEx(
    HANDLE hFile
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvTreeConnect(
    PCWSTR  pszShareName,
    TID * pTid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

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
    LPWSTR pszFileName,
    FID * pFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

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
    LPWSTR pszFileName,
    FID * pFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvCreateTemporary(
    HANDLE hTreeObject,
    LPWSTR pszDirectoryName,
    LPWSTR * ppszFileName,
    USHORT * usFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvWriteFile(
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
SrvSeekFile(
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
SrvFlushFile(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvCloseFile(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvCloseFileAndDisconnect(
    HANDLE hTreeObject,
    USHORT usFid
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvDeleteFile(
    HANDLE hTreeObject,
    USHORT usSearchAttributes,
    LPWSTR pszFileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvCopyFile(
    HANDLE hTreeObject,
    HANDLE hTreeObject2,
    USHORT OpenFunction,
    USHORT Flags,
    UCHAR SourceFileFormat,
    LPWSTR SourceFileName,
    UCHAR TargetFileFormat,
    LPWSTR TargetFileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvTrans2QueryPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    LPWSTR FileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvTrans2QueryFileInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    LPWSTR FileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvTrans2SetPathInformation(
    HANDLE hTreeObject,
    USHORT InformationLevel,
    ULONG Reserved,
    LPWSTR FileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvTrans2CreateDirectory(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvTrans2DeleteDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvTrans2CheckDirectory(
    HANDLE hTreeObject,
    LPWSTR DirectoryName[]
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvTrans2FindNext2(
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
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvNTTransactNotifyChange(
    HANDLE hTreeObject,
    USHORT Fid,
    BOOLEAN WatchTree,
    UCHAR Reserved
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvTrans2GetDFSReferral(
    HANDLE hTreeObject
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

