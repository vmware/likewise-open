/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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


SMB_API
DWORD
SMBReadFile(
    HANDLE      hConnection,
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumberOfBytesToRead,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    )
{
    DWORD  dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION)hConnection;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE)hFile;
    SMB_READ_FILE_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_READ_FILE_RESPONSE pNPResponse = NULL;

    BAIL_ON_INVALID_POINTER(pBuffer);
    BAIL_ON_INVALID_POINTER(pdwBytesRead);
    BAIL_IF_NOT_FILE_HANDLE(hFile);

    request.dwBytesToRead = dwNumberOfBytesToRead;
    request.hFile = pAPIHandle->variant.hIPCHandle;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_READ_FILE,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_READ_FILE_SUCCESS:

            break;

        case SMB_READ_FILE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pResponse);

    pNPResponse = (PSMB_READ_FILE_RESPONSE)pResponse;

    if (pNPResponse->dwBytesRead)
    {
        memcpy(pBuffer, pNPResponse->pBuffer, pNPResponse->dwBytesRead);
    }

    *pdwBytesRead = pNPResponse->dwBytesRead;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    if (pdwBytesRead)
    {
        *pdwBytesRead = 0;
    }

    goto cleanup;
}

