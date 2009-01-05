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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        getnpclientcompname.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSASS)
 *
 *        GetNamedPipeClientComputerName API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

SMB_API
DWORD
SMBGetNamedPipeClientComputerNameA(
    HANDLE   hConnection,
    HANDLE   hNamedPipe,
    PSTR     pszClientComputerName,
    DWORD    dwClientComputerNameLength
    )
{
    DWORD dwError = 0;
    LPWSTR pwszClientComputerName = NULL;
    PSTR     pszName = NULL;

    if (dwClientComputerNameLength == 0)
    {
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBAllocateMemory(
                sizeof(wchar_t) * (dwClientComputerNameLength + 1),
                (PVOID*)&pwszClientComputerName);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBGetNamedPipeClientComputerNameW(
        hConnection,
        hNamedPipe,
        pwszClientComputerName,
        dwClientComputerNameLength);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWc16sToMbs(
        pwszClientComputerName,
        &pszName);
    BAIL_ON_SMB_ERROR(dwError);

    memcpy(pszClientComputerName, pszName, strlen(pszName));

cleanup:

    SMB_SAFE_FREE_MEMORY(pwszClientComputerName);
    SMB_SAFE_FREE_STRING(pszName);

    return dwError;

error:

    goto cleanup;
}

SMB_API
DWORD
SMBGetNamedPipeClientComputerNameW(
    HANDLE   hConnection,
    HANDLE   hNamedPipe,
    LPWSTR pwszClientComputerName,
    DWORD    dwClientComputerNameLength
    )
{
    DWORD  dwError = 0;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE)hNamedPipe;
    SMB_GET_CLIENT_COMPUTER_NAME_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_GET_CLIENT_COMPUTER_NAME_RESPONSE pNPResponse = NULL;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION) hConnection;

    BAIL_ON_INVALID_POINTER(pwszClientComputerName);
    BAIL_IF_NOT_FILE_HANDLE(hNamedPipe);

    request.hNamedPipe = pAPIHandle->variant.hIPCHandle;
    request.dwComputerNameMaxSize = dwClientComputerNameLength;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_GET_CLIENT_COMPUTER_NAME,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_GET_CLIENT_COMPUTER_NAME_SUCCESS:

            break;

        case SMB_GET_CLIENT_COMPUTER_NAME_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);

    pNPResponse = (PSMB_GET_CLIENT_COMPUTER_NAME_RESPONSE)pResponse;

    BAIL_ON_INVALID_POINTER(pNPResponse);

    memset(pwszClientComputerName, 0, dwClientComputerNameLength);
    memcpy(pwszClientComputerName, pNPResponse->pwszName, pNPResponse->dwLength);

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    goto cleanup;
}

