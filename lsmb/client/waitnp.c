/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        waitnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSASS)
 *
 *        WaitNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

SMB_API
DWORD
SMBWaitNamedPipeA(
    HANDLE hConnection,
    HANDLE hAccessToken,
    LPCSTR pszNamedPipeName,
    DWORD  dwTimeout
    )
{
    DWORD    dwError = 0;
    LPWSTR pwszNamedPipeName = NULL;

    dwError = SMBMbsToWc16s(
                    pszNamedPipeName,
                    &pwszNamedPipeName);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBWaitNamedPipeW(
        hConnection,
        hAccessToken,
        pwszNamedPipeName,
        dwTimeout);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_SAFE_FREE_MEMORY(pwszNamedPipeName);

    return dwError;

error:

    goto cleanup;
}

SMB_API
DWORD
SMBWaitNamedPipeW(
    HANDLE    hConnection,
    HANDLE    hAccessToken,
    LPCWSTR   pwszNamedPipeName,
    DWORD     dwTimeout
    )
{
    DWORD  dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = NULL;
    SMB_WAIT_NP_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;

    BAIL_ON_INVALID_WSTRING(pwszNamedPipeName);

    pConnection = (PSMB_SERVER_CONNECTION)hConnection;

    dwError = SMBAPIHandleGetSecurityToken(hAccessToken, &request.pSecurityToken);
    BAIL_ON_SMB_ERROR(dwError);

    request.pwszName = (LPWSTR)pwszNamedPipeName;
    request.dwTimeout = dwTimeout;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                                   pConnection->pAssoc,
                                   SMB_WAIT_NAMED_PIPE,
                                   &request,
                                   &replyType,
                                   &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_WAIT_NAMED_PIPE_SUCCESS:

            break;

        case SMB_WAIT_NAMED_PIPE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    goto cleanup;
}
