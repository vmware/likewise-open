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

#ifndef __IOMGR_H__
#define __IOMGR_H__

DWORD
IOMgrInitialize(
    PCSTR pszConfigFilePath
    );

DWORD
IOMgrCallNamedPipe(
    PIO_ACCESS_TOKEN pSecurityToken,
    PCWSTR pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    DWORD     dwOutBufferSize,
    DWORD     dwTimeout,
    PVOID*    ppOutBuffer,
    PDWORD    pdwOutBufferSize
    );

DWORD
IOMgrCreateNamedPipe(
    PIO_ACCESS_TOKEN pSecurityToken,
    PCWSTR pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PSMB_FILE_HANDLE* phNamedPipe
    );

DWORD
IOMgrGetNamedPipeInfo(
    PSMB_FILE_HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwInBufferSize,
    PDWORD pdwOutBufferSize,
    PDWORD pdwMaxInstances
    );

DWORD
IOMgrConnectNamedPipe(
    PSMB_FILE_HANDLE hNamedPipe
    );

DWORD
IOMgrTransactNamedPipe(
    PSMB_FILE_HANDLE hNamedPipe,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    DWORD     dwOutBufferSize,
    PVOID*    ppOutBuffer,
    PDWORD    pdwOutBufferSize
    );

DWORD
IOMgrWaitNamedPipe(
    PIO_ACCESS_TOKEN pSecurityToken,
    PCWSTR pwszName,
    DWORD dwTimeout
    );

DWORD
IOMgrGetClientComputerName(
    PSMB_FILE_HANDLE hNamedPipe,
    DWORD     dwComputerNameMaxSize,
    PWSTR* ppwszName,
    PDWORD    pdwLength
    );

DWORD
IOMgrGetClientProcessId(
    PSMB_FILE_HANDLE hNamedPipe,
    PDWORD    pdwId
    );

DWORD
IOMgrGetServerProcessId(
    PSMB_FILE_HANDLE hNamedPipe,
    PDWORD    pdwId
    );

DWORD
IOMgrGetClientSessionId(
    PSMB_FILE_HANDLE hNamedPipe,
    PDWORD    pdwId
    );

DWORD
IOMgrPeekNamedPipe(
    PSMB_FILE_HANDLE hNamedPipe,
    PVOID pInBuffer,
    DWORD dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    );

DWORD
IOMgrDisconnectNamedPipe(
    PSMB_FILE_HANDLE hNamedPipe
    );

DWORD
IOMgrCreateFile(
    PIO_ACCESS_TOKEN pSecurityToken,
    PCWSTR pwszFileName,
    DWORD dwDesiredAccess,
    DWORD dwSharedMode,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PSMB_FILE_HANDLE* phFile
    );

DWORD
IOMgrSetNamedPipeHandleState(
    PSMB_FILE_HANDLE hFile,
    PDWORD pdwMode,
    PDWORD pdwCollectionCount,
    PDWORD pdwTimeout
    );

DWORD
IOMgrReadFile(
    PSMB_FILE_HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    );

DWORD
IOMgrWriteFile(
    PSMB_FILE_HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    );

DWORD
IOMgrCloseFile(
    PSMB_FILE_HANDLE hFile
    );

DWORD
IOMgrGetSessionKey(
    PSMB_FILE_HANDLE hFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    );

DWORD
IOMgrShutdown(
    VOID
    );

#endif /* __IOMGR_H__ */

