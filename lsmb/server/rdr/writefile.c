/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        writefile.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        WriteFile API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


DWORD
RdrWriteFileEx(
    HANDLE hFile,
    DWORD  dwNumBytesToWrite,
    PVOID  pBuffer,
    PDWORD pdwNumBytesWritten
    )
{
    DWORD dwError = 0;
    DWORD dwNumBytesWritten = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = (PSMB_CLIENT_FILE_HANDLE)hFile;
    BOOLEAN bFileIsLocked = FALSE;

    SMB_LOCK_MUTEX(bFileIsLocked, &pFile->mutex);

    do
    {
        uint16_t wBytesToWrite = UINT16_MAX;
        uint16_t wBytesWritten = 0;

        if (dwNumBytesToWrite < UINT16_MAX)
        {
            wBytesToWrite = (uint16_t)dwNumBytesToWrite;
        }

        dwError = WireWriteFile(
                    pFile->pTree,
                    pFile->fid,
                    pFile->llOffset,
                    pBuffer + dwNumBytesWritten,
                    wBytesToWrite,
                    &wBytesWritten,
                    NULL);
        BAIL_ON_SMB_ERROR(dwError);

        pFile->llOffset += wBytesWritten;
        dwNumBytesWritten += wBytesWritten;
        dwNumBytesToWrite -= wBytesWritten;

    } while (dwNumBytesToWrite);

error:

    SMB_UNLOCK_MUTEX(bFileIsLocked, &pFile->mutex);

    *pdwNumBytesWritten = dwNumBytesWritten;

    return dwError;
}
