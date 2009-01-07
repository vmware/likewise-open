/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        createfile.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateFile API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

SMB_API
DWORD
SMBCreateFileA(
    HANDLE               hConnection,
    HANDLE               hAccessToken,
    LPCSTR               pszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile,
    PHANDLE              phFile
    )
{
    DWORD dwError = 0;
    LPWSTR pwszFileName = NULL;

    BAIL_ON_INVALID_POINTER(phFile);

    dwError = SMBMbsToWc16s(
                    pszFileName,
                    &pwszFileName);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBCreateFileW(
        hConnection,
        hAccessToken,
        pwszFileName,
        dwDesiredAccess,
        dwSharedMode,
        pSecurityAttributes,
        dwCreationDisposition,
        dwFlagsAndAttributes,
        hTemplateFile,
        phFile);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_SAFE_FREE_MEMORY(pwszFileName);

    return dwError;

error:

    if (phFile)
    {
        *phFile = (HANDLE)NULL;
    }

    goto cleanup;
}

SMB_API
DWORD
SMBCreateFileW(
    HANDLE               hConnection,
    HANDLE               hAccessToken,
    LPCWSTR              pwszFileName,
    DWORD                dwDesiredAccess,
    DWORD                dwSharedMode,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    DWORD                dwCreationDisposition,
    DWORD                dwFlagsAndAttributes,
    HANDLE               hTemplateFile,
    PHANDLE              phFile
    )
{
    DWORD  dwError = 0;
    SMB_CREATE_FILE_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_API_HANDLE pAPIHandle = NULL;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION) hConnection;

    BAIL_ON_INVALID_WSTRING(pwszFileName);
    BAIL_ON_INVALID_POINTER(phFile);

    dwError = SMBAPIHandleGetSecurityToken(hAccessToken, &request.pSecurityToken);
    BAIL_ON_SMB_ERROR(dwError);

    request.pwszFileName = (LPWSTR)pwszFileName;
    request.dwDesiredAccess = dwDesiredAccess;
    request.dwSharedMode = dwSharedMode;
    request.dwCreationDisposition = dwCreationDisposition;
    request.dwFlagsAndAttributes = dwFlagsAndAttributes;
    request.pSecurityAttributes = pSecurityAttributes;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_CREATE_FILE,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_CREATE_FILE_SUCCESS:
            break;

        case SMB_CREATE_FILE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);
    
    dwError = SMBAllocateMemory(sizeof(*pAPIHandle), (void**) (void*) &pAPIHandle);
    BAIL_ON_SMB_ERROR(dwError);

    
    pConnection = pConnection;
    pAPIHandle->variant.hIPCHandle = (HANDLE) pResponse;
    
    *phFile = (HANDLE) pAPIHandle;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    if (phFile)
    {
        *phFile = (HANDLE)NULL;
    }

    goto cleanup;
}
