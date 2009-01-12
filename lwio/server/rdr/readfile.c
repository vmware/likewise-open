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
 *        readfile.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        ReadFile API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


DWORD
RdrReadFileEx(
    HANDLE hFile,
    DWORD  dwBytesToRead,
    PVOID* ppOutBuffer,
    PDWORD pdwBytesRead
    )
{
    DWORD dwError = 0;
    PBYTE pBuffer = NULL;
    DWORD dwBytesRead = 0;
    DWORD dwBufferLen = 0;
    WORD  wBytesAvailable = 0;
    WORD  wBytesRead = 0;
    uint16_t wBytesToRead = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = (PSMB_CLIENT_FILE_HANDLE)hFile;
    BOOLEAN bFileIsLocked = FALSE;

    SMB_LOCK_MUTEX(bFileIsLocked, &pFile->mutex);

    do
    {
        wBytesToRead = UINT16_MAX;

        if (dwBytesToRead < UINT16_MAX)
        {
            wBytesToRead = (uint16_t)dwBytesToRead;
        }

        if (wBytesAvailable < wBytesToRead)
        {
            WORD wAdditional = wBytesToRead - wBytesAvailable;

            SMB_LOG_DEBUG("ClientReadFile: Available [%d] Need [%d]", wBytesAvailable, wBytesToRead);

            dwError = SMBReallocMemory(
                            pBuffer,
                            (PVOID*)&pBuffer,
                            dwBufferLen + wAdditional);
            BAIL_ON_SMB_ERROR(dwError);

            dwBufferLen += wAdditional;
            wBytesAvailable += wAdditional;
        }

        dwError = WireReadFile(
                    pFile->pTree,
                    pFile->fid,
                    pFile->llOffset,
                    pBuffer + dwBytesRead,
                    wBytesToRead,
                    &wBytesRead,
                    NULL);
        BAIL_ON_SMB_ERROR(dwError);

        pFile->llOffset += wBytesRead;
        dwBytesRead += wBytesRead;
        dwBytesToRead -= wBytesRead;

        wBytesAvailable -= wBytesRead;

    } while (dwBytesToRead && (wBytesRead == wBytesToRead));

    *ppOutBuffer = pBuffer;
    *pdwBytesRead = dwBytesRead;

cleanup:

    SMB_UNLOCK_MUTEX(bFileIsLocked, &pFile->mutex);

    return dwError;

error:

    *ppOutBuffer = NULL;
    *pdwBytesRead = 0;

    SMB_SAFE_FREE_MEMORY(pBuffer);

    goto cleanup;
}
