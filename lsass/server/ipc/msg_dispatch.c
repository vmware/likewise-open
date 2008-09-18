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
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Inter-process communication (Server) message dispatch API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "ipc.h"

VOID
LsaSrvHandleConnection(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PLSASERVERCONNECTIONCONTEXT pContext =
           (PLSASERVERCONNECTIONCONTEXT)hConnection;
    PLSAMESSAGE pMessage = NULL;

    dwError = LsaVerifyConnection(pContext);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaPrepareHandle(pContext->fd);
    BAIL_ON_LSA_ERROR(dwError);

    do
    {
        LSA_SAFE_FREE_MESSAGE(pMessage);

        dwError = LsaSecureReadNextMessage(
                            pContext->fd,
        	                pContext->peerUID,
        	                &LsaCheckIncomingMessageAndSender,
                            &pMessage);
        if (dwError != ENOENT) {
           BAIL_ON_LSA_ERROR(dwError);
        } else {
           goto cleanup;
        }

        if (pMessage) {
            PFNLSA_MESSAGE_HANDLER pFn = NULL;
            
            LsaMessageType msgType = pMessage->header.messageType;
            
            if (msgType >= LSA_MESSAGE_SENTINEL) {
               dwError = LSA_ERROR_INVALID_MESSAGE;
               BAIL_ON_LSA_ERROR(dwError);
            }

            pFn = gMessageHandlers[msgType].pfnHandler;
            if (!pFn) {
               dwError = LSA_ERROR_NO_HANDLER;
               BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = pFn(hConnection, pMessage);
            BAIL_ON_LSA_ERROR(dwError);
        }

    } while (pMessage != NULL);

cleanup:

    LSA_SAFE_FREE_MESSAGE(pMessage);
    
    if (hConnection != (HANDLE)NULL) {
        LsaSrvCloseConnection(hConnection);
    }

    return;

error:

    LSA_LOG_DEBUG("Closing connection [fd:%d] upon error [code: %d]", (int)pContext->fd, dwError);

    goto cleanup;
}

DWORD
LsaVerifyConnection(
    PLSASERVERCONNECTIONCONTEXT pContext
    )
{
    DWORD dwError = 0;
    uid_t uid;
    gid_t gid;

    dwError = LsaRecvCreds(pContext->fd, &uid, &gid);
    BAIL_ON_LSA_ERROR(dwError);

    pContext->peerUID = uid;
    pContext->peerGID = gid;

cleanup:
    return dwError;

error:

    pContext->peerUID = 0;
    pContext->peerGID = 0;

    LSA_LOG_ERROR("Local socket client authentication failed");

    goto cleanup;
}

DWORD
LsaPrepareHandle(
    int fd
    )
{
    DWORD dwError = 0;

    DWORD dwFlags = fcntl(fd, F_GETFL, 0);
    if (dwFlags < 0) {
       dwError = errno;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwFlags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, dwFlags) < 0) {
       dwError = errno;
       BAIL_ON_LSA_ERROR(dwError);
    }

error:

    return dwError;
}

DWORD
LsaCheckIncomingMessageAndSender(
    PLSAMESSAGE pMessage,
    uid_t peerUID
    )
{
    DWORD dwError = 0;
    LsaMessageType msgType = pMessage->header.messageType;
    
    if (msgType >= LSA_MESSAGE_SENTINEL) {
        dwError = LSA_ERROR_INVALID_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if (gMessageHandlers[msgType].messageSender == LsaServer) {
        dwError = LSA_ERROR_UNEXPECTED_MESSAGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    return dwError;
}
