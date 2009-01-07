/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
#include "includes.h"


SMB_API
DWORD
SMBTransactNamedPipe(
    HANDLE      hConnection,
    HANDLE      hNamedPipe,
    PVOID       pInBuffer,
    DWORD       dwInBufferSize,
    PVOID       pOutBuffer,
    DWORD       dwOutBufferSize,
    PDWORD      pdwBytesRead,
    POVERLAPPED pOverlapped
    )
{
    DWORD  dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION) hConnection;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE)hNamedPipe;
    SMB_TRANSACT_NP_REQUEST request = {0};
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_CALL_NP_RESPONSE pNPResponse = NULL;

    BAIL_ON_INVALID_POINTER(pAPIHandle);
    BAIL_ON_INVALID_POINTER(pInBuffer);
    BAIL_ON_INVALID_POINTER(pOutBuffer);
    BAIL_ON_INVALID_POINTER(pdwBytesRead);
    BAIL_IF_NOT_FILE_HANDLE(hNamedPipe);

    request.dwInBufferSize = dwInBufferSize;
    request.dwOutBufferSize = dwOutBufferSize;
    request.pInBuffer = (PBYTE)pInBuffer;
    request.hNamedPipe = pAPIHandle->variant.hIPCHandle;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_TRANSACT_NAMED_PIPE,
                    &request,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_TRANSACT_NAMED_PIPE_SUCCESS:

            break;

        case SMB_TRANSACT_NAMED_PIPE_FAILED:

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

    if (pdwBytesRead)
    {
        *pdwBytesRead = 0;
    }

    goto cleanup;
}

