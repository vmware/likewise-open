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
    LPCSTR  pszNamedPipeName,
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
    LPCWSTR   pwszNamedPipeName,
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
    LPCSTR    pszName,
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
    LPCWSTR   pwszName,
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
    LPCSTR pszNamedPipeName,
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
    LPCWSTR   pwszNamedPipeName,
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
    LPWSTR   pwszClientComputerName,
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
    LPCSTR               pszFileName,
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
    LPCWSTR              pwszFileName,
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

SMB_CLIENT_API
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
