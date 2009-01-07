/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        disconnectnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSASS)
 *
 *        DisconnectNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


SMB_API
DWORD
SMBDisconnectNamedPipe(
    HANDLE hConnection,
    HANDLE hNamedPipe
    )
{
    DWORD  dwError = 0;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION)hConnection;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE)hNamedPipe;
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;

    if (pAPIHandle->type != SMB_API_HANDLE_FILE)
    {
        dwError = SMB_ERROR_INVALID_HANDLE;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                    pConnection->pAssoc,
                    SMB_DISCONNECT_NAMED_PIPE,
                    pAPIHandle->variant.hIPCHandle,
                    &replyType,
                    &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_DISCONNECT_NAMED_PIPE_SUCCESS:

            break;

        case SMB_DISCONNECT_NAMED_PIPE_FAILED:

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
