/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
    LPCSTR pszName,
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
    LPWSTR pwszName = NULL;

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
    LPCWSTR   pwszName,
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
    PSMB_SERVER_CONNECTION pConnection = NULL;

    BAIL_ON_INVALID_WSTRING(pwszName);
    BAIL_ON_INVALID_POINTER(phNamedPipe);

    pConnection = (PSMB_SERVER_CONNECTION) hConnection;

    dwError = SMBAPIHandleGetSecurityToken(hAccessToken, &request.pSecurityToken);
    BAIL_ON_SMB_ERROR(dwError);

    request.pwszName = (LPWSTR)pwszName;
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
