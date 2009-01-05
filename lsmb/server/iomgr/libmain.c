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

#include "includes.h"

DWORD
IOMgrInitialize(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;

    dwError = IOMgrInitProviders(pszConfigFilePath);
    BAIL_ON_SMB_ERROR(dwError);

error:

    return dwError;
}

DWORD
IOMgrCallNamedPipe(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    DWORD     dwOutBufferSize,
    DWORD     dwTimeout,
    PVOID*    ppOutBuffer,
    PDWORD    pdwOutBufferSize
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrCreateNamedPipe(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PSMB_FILE_HANDLE* phNamedPipe
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrGetNamedPipeInfo(
    PSMB_FILE_HANDLE pNamedPipe,
    PDWORD pdwFlags,
    PDWORD pdwInBufferSize,
    PDWORD pdwOutBufferSize,
    PDWORD pdwMaxInstances
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrConnectNamedPipe(
    PSMB_FILE_HANDLE pNamedPipe
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrTransactNamedPipe(
    PSMB_FILE_HANDLE pNamedPipe,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    DWORD     dwOutBufferSize,
    PVOID*    ppOutBuffer,
    PDWORD    pdwOutBufferSize
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrWaitNamedPipe(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszName,
    DWORD dwTimeout
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrGetClientComputerName(
    PSMB_FILE_HANDLE pNamedPipe,
    DWORD     dwComputerNameMaxSize,
    LPWSTR* ppwszName,
    PDWORD    pdwLength
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrGetClientProcessId(
    PSMB_FILE_HANDLE pNamedPipe,
    PDWORD    pdwId
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrGetServerProcessId(
    PSMB_FILE_HANDLE pNamedPipe,
    PDWORD    pdwId
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrGetClientSessionId(
    PSMB_FILE_HANDLE pNamedPipe,
    PDWORD    pdwId
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrPeekNamedPipe(
    PSMB_FILE_HANDLE pNamedPipe,
    PVOID pInBuffer,
    DWORD dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrDisconnectNamedPipe(
    PSMB_FILE_HANDLE pNamedPipe
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrCreateFile(
    PSMB_SECURITY_TOKEN_REP pSecurityToken,
    LPCWSTR pwszFileName,
    DWORD dwDesiredAccess,
    DWORD dwSharedMode,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PSMB_FILE_HANDLE* phFile
    )
{
    DWORD dwError = 0;
    PSMB_FILE_HANDLE pHandle = NULL;
    DWORD iProvider = 0;

    dwError = SMBAllocateMemory(
                sizeof(SMB_FILE_HANDLE),
                (PVOID*)&pHandle);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = STATUS_NOT_IMPLEMENTED;

    for (; iProvider < gdwNumVFSProviders; iProvider++)
    {
        PNTVFS_PROVIDER pProvider = &gpVFSProviderArray[iProvider];

        dwError = pProvider->pFnTable->pfnVFSLWCreateFile(
                        pSecurityToken,
                        pwszFileName,
                        dwDesiredAccess,
                        dwSharedMode,
                        dwCreationDisposition,
                        dwFlagsAndAttributes,
                        pSecurityAttributes,
                        &pHandle->hFile);
        if (dwError == STATUS_SUCCESS)
        {
            pHandle->pProvider = pProvider;
            break;
        }

        if ((dwError != STATUS_NOT_SUPPORTED) &&
            (dwError != STATUS_NOT_IMPLEMENTED))
        {
            break;
        }
    }
    BAIL_ON_SMB_ERROR(dwError);

    *phFile = (HANDLE)pHandle;

cleanup:

    return dwError;

error:

    *phFile = NULL;

    SMB_SAFE_FREE_MEMORY(pHandle);

    goto cleanup;
}

DWORD
IOMgrSetNamedPipeHandleState(
    PSMB_FILE_HANDLE pFileHandle,
    PDWORD pdwMode,
    PDWORD pdwCollectionCount,
    PDWORD pdwTimeout
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

DWORD
IOMgrReadFile(
    PSMB_FILE_HANDLE pSMBFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    )
{
    DWORD dwError = 0;
    PNTVFS_PROVIDER pProvider = NULL;
    PVOID pBuffer = NULL;
    DWORD dwNumBytesRead = 0;

    BAIL_ON_INVALID_POINTER(pSMBFile);
    BAIL_ON_INVALID_POINTER(pSMBFile->pProvider);

    pProvider = pSMBFile->pProvider;

    dwError = pProvider->pFnTable->pfnVFSLWReadFile(
                    pSMBFile->hFile,
                    dwBytesToRead,
                    &pBuffer,
                    &dwNumBytesRead);
    BAIL_ON_SMB_ERROR(dwError);

    *ppOutBuffer = pBuffer;
    *pdwBytesRead = dwNumBytesRead;

cleanup:

    return dwError;

error:

    *ppOutBuffer = NULL;
    *pdwBytesRead = 0;

    SMB_SAFE_FREE_MEMORY(pBuffer);

    goto cleanup;
}

DWORD
IOMgrWriteFile(
    PSMB_FILE_HANDLE pSMBFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    )
{
    DWORD dwError = 0;
    PNTVFS_PROVIDER pProvider = NULL;
    DWORD dwNumBytesWritten = 0;

    BAIL_ON_INVALID_POINTER(pSMBFile);
    BAIL_ON_INVALID_POINTER(pSMBFile->pProvider);

    pProvider = pSMBFile->pProvider;

    dwError = pProvider->pFnTable->pfnVFSLWWriteFile(
                    pSMBFile->hFile,
                    dwNumBytesToWrite,
                    pBuffer,
                    &dwNumBytesWritten);
    BAIL_ON_SMB_ERROR(dwError);

    *pdwNumBytesWritten = dwNumBytesWritten;

cleanup:

    return dwError;

error:

    *pdwNumBytesWritten = 0;

    goto cleanup;
}

DWORD
IOMgrCloseFile(
    PSMB_FILE_HANDLE pSMBFile
    )
{
    DWORD dwError = 0;
    PNTVFS_PROVIDER pProvider = NULL;

    BAIL_ON_INVALID_POINTER(pSMBFile);
    BAIL_ON_INVALID_POINTER(pSMBFile->pProvider);

    pProvider = pSMBFile->pProvider;

    dwError = pProvider->pFnTable->pfnVFSLWCloseFile(pSMBFile->hFile);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_SAFE_FREE_MEMORY(pSMBFile);

    return dwError;

error:

    goto cleanup;
}

DWORD
IOMgrGetSessionKey(
    PSMB_FILE_HANDLE pSMBFile,
    PDWORD pdwSessionKeyLength,
    PBYTE* ppSessionKey
    )
{
    DWORD dwError = 0;
    PNTVFS_PROVIDER pProvider = NULL;
    PBYTE pSessionKey = NULL;
    DWORD dwSessionKeyLength = 0;

    BAIL_ON_INVALID_POINTER(pSMBFile);
    BAIL_ON_INVALID_POINTER(pSMBFile->pProvider);

    pProvider = pSMBFile->pProvider;

    dwError = pProvider->pFnTable->pfnVFSLWGetSessionKey(
                    pSMBFile->hFile,
                    &dwSessionKeyLength,
                    &pSessionKey);
    BAIL_ON_SMB_ERROR(dwError);

    *ppSessionKey = pSessionKey;
    *pdwSessionKeyLength = dwSessionKeyLength;

cleanup:

    return dwError;

error:

    *ppSessionKey = NULL;
    *pdwSessionKeyLength = 0;

    goto cleanup;
}

DWORD
IOMgrShutdown(
    VOID
    )
{
    IOMgrFreeProviders();

    return 0;
}

