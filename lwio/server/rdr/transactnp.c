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
#include "rdr.h"

/* @todo: return real NT system error codes? */
NTSTATUS
ClientTransactNamedPipe(
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    DWORD  dwOutBufferSize,
    PVOID* ppOutBuffer,
    PDWORD pdwOutBufferSize
    )
{
    NTSTATUS ntStatus = 0;
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
        ntStatus = SMBAllocateMemory(
                        dwOutBufferSize * sizeof(BYTE),
                        (PVOID*)&pOutBuffer);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = NPTransact(
                    pFile->pTree,
                    pFile->fid,
                    pInBuffer,
                    dwInBufferSize,
                    pOutBuffer,
                    dwOutBufferSize,
                    &dwBytesRead);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppOutBuffer = pOutBuffer;
    *pdwOutBufferSize = dwBytesRead;

cleanup:

    return ntStatus;

error:

    *ppOutBuffer = NULL;
    *pdwOutBufferSize = 0;

    SMB_SAFE_FREE_MEMORY(pOutBuffer);

    goto cleanup;
*/

    return ntStatus;
}
