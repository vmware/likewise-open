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
 *        createnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
SMBCreateNamedPipeA(
    HANDLE hConnection,
    HANDLE hAccessToken,
    PCSTR pszName,
    DWORD  dwOpenMode,
    DWORD  dwPipeMode,
    DWORD  dwMaxInstances,
    DWORD  dwOutBufferSize,
    DWORD  dwInBufferSize,
    DWORD  dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE phNamedPipe
    )
{
    DWORD dwError = 0;
    PWSTR pwszName = NULL;

    BAIL_ON_INVALID_POINTER(phNamedPipe);

    dwError = SMBMbsToWc16s(
                    pszName,
                    &pwszName);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreateNamedPipeW(
        hConnection,
        hAccessToken,
        pwszName,
        dwOpenMode,
        dwPipeMode,
        dwMaxInstances,
        dwOutBufferSize,
        dwInBufferSize,
        dwDefaultTimeOut,
        pSecurityAttributes,
        phNamedPipe);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_SAFE_FREE_MEMORY(pwszName);

    return dwError;

error:

    if (phNamedPipe)
    {
        *phNamedPipe = (HANDLE)NULL;
    }

    goto cleanup;
}

DWORD
SMBCreateNamedPipeW(
    HANDLE    hConnection,
    HANDLE    hAccessToken,
    PCWSTR   pwszName,
    DWORD     dwOpenMode,
    DWORD     dwPipeMode,
    DWORD     dwMaxInstances,
    DWORD     dwOutBufferSize,
    DWORD     dwInBufferSize,
    DWORD     dwDefaultTimeOut,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHANDLE   phNamedPipe
    )
{
    DWORD  dwError = 0;
    SMB_CREATE_NP_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_API_HANDLE pAPIHandle = NULL;
    PIO_CONTEXT pConnection = NULL;

    BAIL_ON_INVALID_WSTRING(pwszName);
    BAIL_ON_INVALID_POINTER(phNamedPipe);

    pConnection = (PIO_CONTEXT) hConnection;

    dwError = SMBAPIHandleGetSecurityToken(hAccessToken, &request.pSecurityToken);
    BAIL_ON_SMB_ERROR(dwError);

    request.pwszName = (PWSTR)pwszName;
    request.dwOpenMode = dwOpenMode;
    request.dwPipeMode = dwPipeMode;
    request.dwMaxInstances = dwMaxInstances;
    request.dwOutBufferSize = dwOutBufferSize;
    request.dwInBufferSize = dwInBufferSize;
    request.dwDefaultTimeOut = dwDefaultTimeOut;
    request.pSecurityAttributes = pSecurityAttributes;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_CREATE_NAMED_PIPE,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_CREATE_NAMED_PIPE_SUCCESS:

            break;

        case SMB_CREATE_NAMED_PIPE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pResponse);

    dwError = SMBAllocateMemory(sizeof(*pAPIHandle), (void**) (void*) &pAPIHandle);
    BAIL_ON_SMB_ERROR(dwError);

    pAPIHandle->variant.hIPCHandle = pResponse;

    *phNamedPipe = (HANDLE) pAPIHandle;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    if (phNamedPipe)
    {
        *phNamedPipe = (HANDLE)NULL;
    }

    goto cleanup;
}
