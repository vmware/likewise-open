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
 *        callnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CallNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
SMBCallNamedPipeA(
    HANDLE  hConnection,
    HANDLE  hAccessToken,
    PCSTR  pszNamedPipeName,
    PVOID   pInBuffer,
    DWORD   dwInBufferSize,
    PVOID   pOutBuffer,
    DWORD   dwOutBufferSize,
    PDWORD  pdwBytesRead,
    DWORD   dwTimeout
    )
{
    DWORD    dwError = 0;
    PWSTR pwszNamedPipeName = NULL;

    dwError = SMBMbsToWc16s(
                    pszNamedPipeName,
                    &pwszNamedPipeName);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCallNamedPipeW(
        hConnection,
        hAccessToken,
        pwszNamedPipeName,
        pInBuffer,
        dwInBufferSize,
        pOutBuffer,
        dwOutBufferSize,
        pdwBytesRead,
        dwTimeout);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_SAFE_FREE_MEMORY(pwszNamedPipeName);

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBCallNamedPipeW(
    HANDLE    hConnection,
    HANDLE    hAccessToken,
    PCWSTR   pwszNamedPipeName,
    PVOID     pInBuffer,
    DWORD     dwInBufferSize,
    PVOID     pOutBuffer,
    DWORD     dwOutBufferSize,
    PDWORD    pdwBytesRead,
    DWORD     dwTimeout
    )
{
    DWORD  dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = NULL;
    SMB_CALL_NP_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_CALL_NP_RESPONSE pNPResponse = NULL;

    BAIL_ON_INVALID_WSTRING(pwszNamedPipeName);
    BAIL_ON_INVALID_POINTER(pInBuffer);
    BAIL_ON_INVALID_POINTER(pOutBuffer);
    BAIL_ON_INVALID_POINTER(pdwBytesRead);

    pConnection = (PSMB_SERVER_CONNECTION)hConnection;

    dwError = SMBAPIHandleGetSecurityToken(hAccessToken, &request.pSecurityToken);
    BAIL_ON_SMB_ERROR(dwError);

    request.dwInBufferSize = dwInBufferSize;
    request.dwOutBufferSize = dwOutBufferSize;
    request.pInBuffer = (PBYTE)pInBuffer;
    request.dwTimeout = dwTimeout;
    request.pwszNamedPipeName = (PWSTR)pwszNamedPipeName;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_CALL_NAMED_PIPE,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_CALL_NAMED_PIPE_SUCCESS:

            break;

        case SMB_CALL_NAMED_PIPE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pResponse);

    pNPResponse = (PSMB_CALL_NP_RESPONSE)pResponse;

    memcpy(pOutBuffer, pNPResponse->pOutBuffer, pNPResponse->dwOutBufferSize);
    *pdwBytesRead = pNPResponse->dwOutBufferSize;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    goto cleanup;
}
