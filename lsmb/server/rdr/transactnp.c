/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        transactnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSASS)
 *
 *        TransactNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

/* @todo: return real NT system error codes? */
DWORD
ClientTransactNamedPipe(
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    DWORD  dwOutBufferSize,
    PVOID* ppOutBuffer,
    PDWORD pdwOutBufferSize
    )
{
    DWORD dwError = 0;
/*
    PBYTE pBuffer = NULL;
    DWORD dwBytesRead = 0;
    DWORD dwBufferLen = 0;
    WORD  wBytesAvailable = 0;
    WORD  wBytesRead = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = (PSMB_CLIENT_FILE_HANDLE)hNamedPipe;
    PBYTE pOutBuffer = NULL;
    DWORD dwBytesRead = 0;

    if (dwOutBufferSize)
    {
        dwError = SMBAllocateMemory(
                        dwOutBufferSize * sizeof(BYTE),
                        (PVOID*)&pOutBuffer);
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = NPTransact(
                    pFile->pTree,
                    pFile->fid,
                    pInBuffer,
                    dwInBufferSize,
                    pOutBuffer,
                    dwOutBufferSize,
                    &dwBytesRead);
    BAIL_ON_SMB_ERROR(dwError);

    *ppOutBuffer = pOutBuffer;
    *pdwOutBufferSize = dwBytesRead;

cleanup:

    return dwError;

error:

    *ppOutBuffer = NULL;
    *pdwOutBufferSize = 0;

    SMB_SAFE_FREE_MEMORY(pOutBuffer);

    goto cleanup;
*/

    return dwError;
}
