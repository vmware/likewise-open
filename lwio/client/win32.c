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
    PSMB_CLIENT_CONTEXT pContext = NULL;

    if (SMBGetClientContext(&pContext))
    {
        abort();
    }

    return pContext->dwLastError;
}

void
SetLastError(
    DWORD dwError
    )
{
    PSMB_CLIENT_CONTEXT pContext = NULL;

    if (SMBGetClientContext(&pContext))
    {
        abort();
    }

    pContext->dwLastError = dwError;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCallNamedPipeA(
        &connection,
        pContext->hAccessToken,
        pszNamedPipeName,
        pInBuffer,
        dwInBufferSize,
        pOutBuffer,
        dwOutBufferSize,
        pdwBytesRead,
        dwTimeout);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCallNamedPipeW(
        &connection,
        pContext->hAccessToken,
        pwszNamedPipeName,
        pInBuffer,
        dwInBufferSize,
        pOutBuffer,
        dwOutBufferSize,
        pdwBytesRead,
        dwTimeout);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;
    HANDLE hHandle = INVALID_HANDLE_VALUE;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreateNamedPipeA(
        &connection,
        pContext->hAccessToken,
        pszName,
        dwOpenMode,
        dwPipeMode,
        dwMaxInstances,
        dwOutBufferSize,
        dwInBufferSize,
        dwDefaultTimeOut,
        pSecurityAttributes,
        &hHandle);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return hHandle;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;
    HANDLE hHandle = INVALID_HANDLE_VALUE;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreateNamedPipeW(
        &connection,
        pContext->hAccessToken,
        pwszName,
        dwOpenMode,
        dwPipeMode,
        dwMaxInstances,
        dwOutBufferSize,
        dwInBufferSize,
        dwDefaultTimeOut,
        pSecurityAttributes,
        &hHandle);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return hHandle;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBGetNamedPipeInfo(
        &connection,
        hNamedPipe,
        pdwFlags,
        pdwOutBufferSize,
        pdwInBufferSize,
        pdwMaxInstances);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}

BOOL
ConnectNamedPipe(
    HANDLE      hNamedPipe,
    POVERLAPPED pOverlapped
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBConnectNamedPipe(
        &connection,
        hNamedPipe,
        pOverlapped);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBTransactNamedPipe(
        &connection,
        hNamedPipe,
        pInBuffer,
        dwInBufferSize,
        pOutBuffer,
        dwOutBufferSize,
        pdwBytesRead,
        pOverlapped);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}

BOOL
WaitNamedPipeA(
    PCSTR pszNamedPipeName,
    DWORD  dwTimeOut
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWaitNamedPipeA(
        &connection,
        pContext->hAccessToken,
        pszNamedPipeName,
        dwTimeOut);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}


BOOL
WaitNamedPipeW(
    PCWSTR   pwszNamedPipeName,
    DWORD     dwTimeOut
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWaitNamedPipeW(
        &connection,
        pContext->hAccessToken,
        pwszNamedPipeName,
        dwTimeOut);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}

BOOL
GetNamedPipeClientComputerNameA(
    HANDLE hNamedPipe,
    PSTR   pszClientComputerName,
    DWORD  dwClientComputerNameLength
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBGetNamedPipeClientComputerNameA(
        &connection,
        hNamedPipe,
        pszClientComputerName,
        dwClientComputerNameLength);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}

BOOL
GetNamedPipeClientComputerNameW(
    HANDLE   hNamedPipe,
    PWSTR   pwszClientComputerName,
    DWORD    dwClientComputerNameLength
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBGetNamedPipeClientComputerNameW(
        &connection,
        hNamedPipe,
        pwszClientComputerName,
        dwClientComputerNameLength);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}

BOOL
GetNamedPipeClientProcessId(
    HANDLE hNamedPipe,
    PDWORD pdwClientProcessId
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBGetNamedPipeClientProcessId(
        &connection,
        hNamedPipe,
        pdwClientProcessId);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}

BOOL
GetNamedPipeServerProcessId(
    HANDLE hNamedPipe,
    PDWORD pdwServerProcessId
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBGetNamedPipeServerProcessId(
        &connection,
        hNamedPipe,
        pdwServerProcessId);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}

BOOL
GetNamedPipeClientSessionId(
    HANDLE hNamedPipe,
    PDWORD pdwClientSessionId
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBGetNamedPipeClientSessionId(
        &connection,
        hNamedPipe,
        pdwClientSessionId);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBPeekNamedPipe(
        &connection,
        hNamedPipe,
        pInBuffer,
        dwInBufferSize,
        pdwBytesRead,
        pdwTotalBytesAvail,
        pdwBytesLeftThisMessage);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}

BOOL
DisconnectNamedPipe(
    HANDLE hNamedPipe
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBDisconnectNamedPipe(
        &connection,
        hNamedPipe);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;
    HANDLE hHandle = INVALID_HANDLE_VALUE;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreateFileA(
        &connection,
        pContext->hAccessToken,
        pszFileName,
        dwDesiredAccess,
        dwSharedMode,
        pSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile,
        &hHandle);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return hHandle;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;
    HANDLE hHandle = INVALID_HANDLE_VALUE;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreateFileW(
        &connection,
        pContext->hAccessToken,
        pwszFileName,
        dwDesiredAccess,
        dwSharedMode,
        pSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile,
        &hHandle);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return hHandle;
}

BOOL
SetNamedPipeHandleState(
    HANDLE      hPipe,
    PDWORD      pdwMode,
    PDWORD      pdwMaxCollectionCount,
    PDWORD      pdwMaxTimeout
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSetNamedPipeHandleState(
                    &connection,
                    hPipe,
                    pdwMode,
                    pdwMaxCollectionCount,
                    pdwMaxTimeout);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBReadFile(
        &connection,
        hFile,
        pBuffer,
        dwNumberOfBytesToRead,
        pdwBytesRead,
        pOverlapped);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
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
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWriteFile(
        &connection,
        hFile,
        pBuffer,
        dwNumBytesToWrite,
        pdwNumBytesWritten,
        pOverlapped);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}

BOOL
CloseHandle(
    HANDLE hFile
    )
{
    DWORD dwError = 0;
    SMB_SERVER_CONNECTION connection;
    PSMB_CLIENT_CONTEXT pContext = NULL;

    dwError = SMBAcquireState(&connection, &pContext);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCloseHandle(
        &connection,
        hFile);
    BAIL_ON_SMB_ERROR(dwError);

error:

    pContext->dwLastError = dwError;

    SMBReleaseState(&connection, pContext);

    return dwError ? FALSE : TRUE;
}


BOOL
SetThreadToken(
    PHANDLE phThread,
    HANDLE hAccessToken
    )
{
    DWORD dwError = 0;

    if (phThread)
    {
        dwError = SMB_ERROR_NOT_IMPLEMENTED;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBSetThreadToken(hAccessToken);
    BAIL_ON_SMB_ERROR(dwError);

error:

    SetLastError(dwError);

    return dwError ? FALSE : TRUE;
}

BOOL
OpenThreadToken(
    HANDLE hThread,
    DWORD dwDesiredAccess,
    BOOL bOpenAsSelf,
    PHANDLE phAccessToken
    )
{
    DWORD dwError = 0;

    if (hThread)
    {
        dwError = SMB_ERROR_NOT_IMPLEMENTED;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBGetThreadToken(phAccessToken);
    BAIL_ON_SMB_ERROR(dwError);

error:
    
    SetLastError(dwError);

    return dwError ? FALSE : TRUE;
}
