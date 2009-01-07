/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        connectnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        ConnectNamedPipe API
 *
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"


SMB_API
DWORD
SMBConnectNamedPipe(
    HANDLE      hConnection,
    HANDLE      hNamedPipe,
    POVERLAPPED pOverlapped
    )
{
    DWORD  dwError = 0;
    LWMsgMessageTag replyType;
    PVOID pResponse = NULL;
    PSMB_SERVER_CONNECTION pConnection = (PSMB_SERVER_CONNECTION) hConnection;
    PSMB_API_HANDLE pAPIHandle = (PSMB_API_HANDLE) hNamedPipe;

    BAIL_IF_NOT_FILE_HANDLE(hNamedPipe);

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_send_transact(
                                   pConnection->pAssoc,
                                   SMB_CONNECT_NAMED_PIPE,
                                   pAPIHandle->variant.hIPCHandle,
                                   &replyType,
                                   &pResponse));
    BAIL_ON_SMB_ERROR(dwError);

    switch (replyType)
    {
        case SMB_CONNECT_NAMED_PIPE_SUCCESS:

            break;

        case SMB_CONNECT_NAMED_PIPE_FAILED:

            BAIL_ON_INVALID_POINTER(pResponse);

            dwError = ((PSMB_STATUS_REPLY)pResponse)->dwError;

            break;

        default:

            dwError = EINVAL;

            break;
    }
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}
