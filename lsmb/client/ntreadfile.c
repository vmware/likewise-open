/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        ntreadfile.c
 *
 * Abstract:
 *
 *        Likewise NT Subsystem (NT)
 *
 *        NTReadFile API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


NT_API
DWORD
NTReadFile(
    HANDLE      hConnection,
    HANDLE      hFile,
    PVOID       pBuffer,
    DWORD       dwNumberOfBytesToRead,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    )
{
    DWORD  ntStatus = 0;
    PNT_SERVER_CONNECTION pConnection = (PNT_SERVER_CONNECTION)hConnection;
    PNT_API_HANDLE pAPIHandle = (PNT_API_HANDLE)hFile;
    NT_READ_FILE_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PNT_READ_FILE_RESPONSE pNPResponse = NULL;

    BAIL_ON_INVALID_POINTER(pBuffer);
    BAIL_ON_INVALID_POINTER(pdwBytesRead);
    BAIL_IF_NOT_FILE_HANDLE(hFile);

    request.dwBytesToRead = dwNumberOfBytesToRead;
    request.hFile = pAPIHandle->variant.hIPCHandle;

    ntStatus = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    NT_READ_FILE,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_NT_STATUS(ntStatus);

    switch (replyType)
    {
        case NT_READ_FILE_SUCCESS:

            break;

        case NT_READ_FILE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            ntStatus = ((PNT_STATUS_REPLY)pResponse)->ntStatus;

            break;

        default:

            ntStatus = EINVAL;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    BAIL_ON_INVALID_POINTER(pResponse);

    pNPResponse = (PNT_READ_FILE_RESPONSE)pResponse;

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

    return ntStatus;

error:

    if (pdwBytesRead)
    {
        *pdwBytesRead = 0;
    }

    goto cleanup;
}

