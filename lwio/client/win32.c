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

#include "includes.h"

/* These functions are thin wrappers around the reentrant versions which
   acquire implicit state from thread-local storage */

DWORD
GetLastError(
    void
    )
{
    abort();
}

void
SetLastError(
    DWORD dwError
    )
{
    abort();
}

BOOL
CallNamedPipeA(
    PCSTR  pszNamedPipeName,
    PVOID   pInBuffer,
    DWORD   dwInBufferSize,
    PVOID   pOutBuffer,
    DWORD   dwOutBufferSize,
    PDWORD  pdwBytesRead,
    DWORD   dwTimeout
    )
{
    abort();
}


BOOL
CallNamedPipeW(
    PCWSTR   pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    PVOID     pOutBuffer,
    DWORD     dwOutBufferSize,
    PDWORD    pdwBytesRead,
    DWORD     dwTimeout
    )
{
    abort();
}

HANDLE
CreateNamedPipeA(
    PCSTR    pszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes
    )
{
    abort();
}

HANDLE
CreateNamedPipeW(
    PCWSTR   pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes
    )
{
    abort();
}

BOOL
GetNamedPipeInfo(
    HANDLE hNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwOutBufferSize,
    PDWORD pdwInBufferSize,
    PDWORD pdwMaxInstances
    )
{
    abort();
}

BOOL
ConnectNamedPipe(
    HANDLE      hNamedPipe,
    POVERLAPPED pOverlapped
    )
{
    abort();
}

BOOL
TransactNamedPipe(
    HANDLE      hNamedPipe,
    PVOID       pInBuffer,
    DWORD       dwInBufferSize,
    PVOID       pOutBuffer,
    DWORD       dwOutBufferSize,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    )
{
    abort();
}

BOOL
WaitNamedPipeA(
    PCSTR pszNamedPipeName,
    DWORD  dwTimeOut
    )
{
    abort();
}


BOOL
WaitNamedPipeW(
    PCWSTR   pwszNamedPipeName,
    DWORD     dwTimeOut
    )
{
    abort();
}

BOOL
GetNamedPipeClientComputerNameA(
    HANDLE hNamedPipe,
    PSTR   pszClientComputerName,
    DWORD  dwClientComputerNameLength
    )
{
    abort();
}

BOOL
GetNamedPipeClientComputerNameW(
    HANDLE   hNamedPipe,
    PWSTR   pwszClientComputerName,
    DWORD    dwClientComputerNameLength
    )
{
    abort();
}

BOOL
GetNamedPipeClientProcessId(
    HANDLE hNamedPipe,
    PDWORD pdwClientProcessId
    )
{
    abort();
}

BOOL
GetNamedPipeServerProcessId(
    HANDLE hNamedPipe,
    PDWORD pdwServerProcessId
    )
{
    abort();
}

BOOL
GetNamedPipeClientSessionId(
    HANDLE hNamedPipe,
    PDWORD pdwClientSessionId
    )
{
    abort();
}


BOOL
PeekNamedPipe(
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    )
{
    abort();
}

BOOL
DisconnectNamedPipe(
    HANDLE hNamedPipe
    )
{
    abort();
}

HANDLE
CreateFileA(
    PCSTR               pszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile
    )
{
    abort();
}

HANDLE
CreateFileW(
    PCWSTR              pwszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile
    )
{
    abort();
}

BOOL
SetNamedPipeHandleState(
    HANDLE      hPipe,
    PDWORD      pdwMode,
    PDWORD      pdwMaxCollectionCount,
    PDWORD      pdwMaxTimeout
    )
{
    abort();
}

BOOL
ReadFile(
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumberOfBytesToRead,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    )
{
    abort();
}

BOOL
WriteFile(
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumBytesToWrite,
    PDWORD      pdwNumBytesWritten,
    POVERLAPPED pOverlapped
    )
{
    abort();
}

BOOL
CloseHandle(
    HANDLE hFile
    )
{
    abort();
}


BOOL
SetThreadToken(
    PHANDLE phThread,
    HANDLE hAccessToken
    )
{
    abort();
}

BOOL
OpenThreadToken(
    HANDLE hThread,
    DWORD dwDesiredAccess,
    BOOL bOpenAsSelf,
    PHANDLE phAccessToken
    )
{
    abort();
}
