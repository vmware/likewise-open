/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        peeknp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSASS)
 *
 *        PeekNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


SMB_API
DWORD
SMBPeekNamedPipe(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PVOID  pInBuffer,
    DWORD  dwInBufferSize,
    PDWORD pdwBytesRead,
    PDWORD pdwTotalBytesAvail,
    PDWORD pdwBytesLeftThisMessage
    )
{
    DWORD  dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION) hConnection;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE) hNamedPipe;
    SMB_PEEK_NP_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_PEEK_NP_RESPONSE pNPResponse = NULL;

    BAIL_ON_INVALID_POINTER(pInBuffer);
    BAIL_ON_INVALID_POINTER(pdwBytesRead);
    BAIL_ON_INVALID_POINTER(pdwTotalBytesAvail);
    BAIL_ON_INVALID_POINTER(pdwBytesLeftThisMessage);
    BAIL_IF_NOT_FILE_HANDLE(hNamedPipe);

    request.dwInBufferSize = dwInBufferSize;
    request.pInBuffer = (PBYTE)pInBuffer;
    request.hNamedPipe = pAPIHandle->variant.hIPCHandle;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_PEEK_NAMED_PIPE,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_PEEK_NAMED_PIPE_SUCCESS:

            break;

        case SMB_PEEK_NAMED_PIPE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pResponse);

    pNPResponse = (PSMB_PEEK_NP_RESPONSE)pResponse;

    *pdwBytesRead = pNPResponse->dwBytesRead;
    *pdwTotalBytesAvail = pNPResponse->dwTotalBytesAvail;
    *pdwBytesLeftThisMessage = pNPResponse->dwBytesLeftThisMessage;

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

    if (pdwTotalBytesAvail)
    {
        *pdwTotalBytesAvail = 0;
    }

    if (pdwBytesLeftThisMessage)
    {
        *pdwBytesLeftThisMessage = 0;
    }

    goto cleanup;
}

