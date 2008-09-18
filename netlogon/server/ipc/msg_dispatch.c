/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see 
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        msg_dispatch.c
 *
 * Abstract:
 *
 *        Likewise Site Manager
 * 
 *        Inter-process communication (Server)
 *
 *        Message dispatch API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

VOID
LWNetSrvHandleConnection(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PLWNETSERVERCONNECTIONCONTEXT pContext =
           (PLWNETSERVERCONNECTIONCONTEXT)hConnection;
    PLWNETMESSAGE pMessage = NULL;

    dwError = LWNetVerifyConnection(pContext);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetPrepareHandle(pContext->fd);
    BAIL_ON_LWNET_ERROR(dwError);

    do
    {
        LWNET_SAFE_FREE_MESSAGE(pMessage);

        dwError = LWNetSecureReadNextMessage(
                            pContext->fd,
                            pContext->peerUID,
                            &LWNetCheckIncomingMessageAndSender,
                            &pMessage);
        BAIL_ON_LWNET_ERROR(dwError);

        if (pMessage) {
            PFNLWNET_MESSAGE_HANDLER pFn = NULL;
            
            LWNetMessageType msgType = pMessage->header.messageType;
            
            if (msgType >= LWNET_MESSAGE_SENTINEL) {
               dwError = LWNET_ERROR_INVALID_MESSAGE;
               BAIL_ON_LWNET_ERROR(dwError);
            }

            pFn = gMessageHandlers[msgType].pfnHandler;
            if (!pFn) {
               dwError = LWNET_ERROR_NO_HANDLER;
               BAIL_ON_LWNET_ERROR(dwError);
            }

            dwError = pFn(hConnection, pMessage);
            BAIL_ON_LWNET_ERROR(dwError);
        }

    } while (pMessage != NULL);

cleanup:

    LWNET_SAFE_FREE_MESSAGE(pMessage);
    
    if (hConnection != (HANDLE)NULL) {
        LWNetSrvCloseConnection(hConnection);
    }

    return;

error:

    LWNET_LOG_DEBUG("Closing connection [fd:%d] upon error [code: %d]", (int)pContext->fd, dwError);

    goto cleanup;
}

DWORD
LWNetVerifyConnection(
    PLWNETSERVERCONNECTIONCONTEXT pContext
    )
{
    DWORD dwError = 0;
    uid_t uid;
    gid_t gid;

    dwError = LWNetRecvCreds(pContext->fd, &uid, &gid);
    BAIL_ON_LWNET_ERROR(dwError);

    pContext->peerUID = uid;
    pContext->peerGID = gid;

cleanup:
    return dwError;

error:

    pContext->peerUID = 0;
    pContext->peerGID = 0;

    LWNET_LOG_ERROR("Local socket client authentication failed");

    goto cleanup;
}

DWORD
LWNetPrepareHandle(
    int fd
    )
{
    DWORD dwError = 0;

    DWORD dwFlags = fcntl(fd, F_GETFL, 0);
    if (dwFlags < 0) {
       dwError = errno;
       BAIL_ON_LWNET_ERROR(dwError);
    }

    dwFlags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, dwFlags) < 0) {
       dwError = errno;
       BAIL_ON_LWNET_ERROR(dwError);
    }

error:

    return dwError;
}

DWORD
LWNetCheckIncomingMessageAndSender(
    PLWNETMESSAGE pMessage,
    uid_t peerUID
    )
{
    DWORD dwError = 0;
    LWNetMessageType msgType = pMessage->header.messageType;
    
    if (msgType >= LWNET_MESSAGE_SENTINEL) {
        dwError = LWNET_ERROR_INVALID_MESSAGE;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    
    if (gMessageHandlers[msgType].messageSender == LWNetServer) {
        dwError = LWNET_ERROR_UNEXPECTED_MESSAGE;
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:

    return dwError;
}
