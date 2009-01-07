/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        auth.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSASS)
 *
 *        GetNamedPipeClientProcessId API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


SMB_API
DWORD
SMBGetNamedPipeClientProcessId(
    HANDLE hConnection,
    HANDLE hNamedPipe,
    PDWORD pdwClientProcessId
    )
{
    DWORD  dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION)hConnection;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE)hNamedPipe;
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_ID_REPLY pNPResponse = NULL;

    BAIL_ON_INVALID_POINTER(pdwClientProcessId);
    BAIL_IF_NOT_FILE_HANDLE(hNamedPipe);

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_GET_CLIENT_PROCESS_ID,
                    pAPIHandle->variant.hIPCHandle,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_GET_CLIENT_PROCESS_ID_SUCCESS:

            break;

        case SMB_GET_CLIENT_PROCESS_ID_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);

    pNPResponse = (PSMB_ID_REPLY)pResponse;

    BAIL_ON_INVALID_POINTER(pNPResponse);

    *pdwClientProcessId = pNPResponse->dwId;

cleanup:

    if (pResponse)
    {
        lwmsg_assoc_free_graph(pConnection->pAssoc, replyType, pResponse);
    }

    return dwError;

error:

    goto cleanup;
}

