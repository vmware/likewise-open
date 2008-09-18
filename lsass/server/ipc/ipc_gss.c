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
 *        ipc_gss.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Inter-process communication (Server) API for GSS NTLM
 *
 * Author: Todd Stecher (v-todds@likewise.com) 2008
 *
 */
#include "ipc.h"

#define ZERO_STRUCT(_s_) memset((char*)&(_s_),0,sizeof(_s_))


DWORD
LsaSrvIpcBuildAuthMessage(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError;
    DWORD msgStatus;
    DWORD dwMsgLen;
    ULONG negotiateFlags;
    SEC_BUFFER marshaledCredential;
    SEC_BUFFER authenticateMessage;
    SEC_BUFFER targetInfo;
    SEC_BUFFER_S serverChallenge;
    SEC_BUFFER_S baseSessionKey;

    PLSAMESSAGE pResponse = NULL;
    PLSASERVERCONNECTIONCONTEXT pContext =
        (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;

    ZERO_STRUCT(marshaledCredential);
    ZERO_STRUCT(authenticateMessage);
    ZERO_STRUCT(targetInfo);
    ZERO_STRUCT(baseSessionKey);

    dwError = LsaUnMarshalGSSMakeAuthMsgQ(
    		pMessage->pData,
                pMessage->header.messageLength,
                &marshaledCredential,
                &serverChallenge,
                &targetInfo,
                &negotiateFlags
                );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    /* call NTLMSRV stub */
    msgStatus = NTLMGssBuildAuthenticateMessage(
                        negotiateFlags,
                        pContext->peerUID,
                        &marshaledCredential,
                        &serverChallenge,
                        &targetInfo,
                        &authenticateMessage,
                        &baseSessionKey
                        );

    dwError = LsaMarshalGSSMakeAuthMsgR(
                        msgStatus,
                        &authenticateMessage,
                        &baseSessionKey,
                        NULL,
                        &dwMsgLen
                        );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_R_GSS_MAKE_AUTH_MSG,
                dwMsgLen,
                1,
                1,
                &pResponse
                );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalGSSMakeAuthMsgR(
                        msgStatus,
                        &authenticateMessage,
                        &baseSessionKey,
                        pResponse->pData,
                        &dwMsgLen
                        );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
error:

    NTLMGssFreeSecBuffer(&authenticateMessage);
    
    return dwError;
}

DWORD
LsaSrvIpcCheckAuthMessage(
    HANDLE hConnection,
    PLSAMESSAGE pMessage
    )
{
    DWORD dwError;
    DWORD msgStatus;
    DWORD dwMsgLen;
    ULONG negotiateFlags;
    SEC_BUFFER authenticateMessage;
    SEC_BUFFER targetInfo;
    SEC_BUFFER_S serverChallenge;
    SEC_BUFFER_S baseSessionKey;

    PLSAMESSAGE pResponse = NULL;
    PLSASERVERCONNECTIONCONTEXT pContext =
        (PLSASERVERCONNECTIONCONTEXT)hConnection;
    HANDLE hServer = (HANDLE)NULL;

    ZERO_STRUCT(authenticateMessage);
    ZERO_STRUCT(targetInfo);
    ZERO_STRUCT(baseSessionKey);

    dwError = LsaUnMarshalGSSCheckAuthMsgQ(
    		pMessage->pData,
                pMessage->header.messageLength,
                &negotiateFlags,
                &serverChallenge,
                &targetInfo,
                &authenticateMessage
                );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvIpcOpenServer(hConnection, &hServer);
    BAIL_ON_LSA_ERROR(dwError);

    /* call NTLMSRV stub */
    msgStatus = NTLMGssCheckAuthenticateMessage(
                        negotiateFlags,
                        &serverChallenge,
                        &targetInfo,
                        &authenticateMessage,
                        &baseSessionKey
                        );

    dwError = LsaMarshalGSSCheckAuthMsgR(
                        msgStatus,
                        &baseSessionKey,
                        NULL,
                        &dwMsgLen
                        );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaBuildMessage(
                LSA_R_GSS_CHECK_AUTH_MSG,
                dwMsgLen,
                1,
                1,
                &pResponse
                );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMarshalGSSCheckAuthMsgR(
                        msgStatus,
                        &baseSessionKey,
                        pResponse->pData,
                        &dwMsgLen
                        );

    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaWriteMessage(pContext->fd, pResponse);
    BAIL_ON_LSA_ERROR(dwError);
    
error:

    return dwError;
}

