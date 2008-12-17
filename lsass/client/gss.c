/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        gss.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        GSS client stub
 *
 * Authors: Todd Stecher (v-todds@likewise.com)
 *
 */
#include "client.h"

#define ZERO_STRUCT(_s_) memset((char*)&(_s_), 0, sizeof((_s_))

static
VOID
LsaNTMLCopyGssSecBuffer(
    IN PSEC_BUFFER pSecBuffer,
    OUT SEC_BUFFER SecBuffer
    )
{
    SecBuffer.length = pSecBuffer->length;
    SecBuffer.maxLength = pSecBuffer->maxLength;
    SecBuffer.buffer = pSecBuffer->buffer;
}

static
VOID
LsaNTMLCopyGssSecBufferS(
    IN PSEC_BUFFER_S pSecBufferS,
    OUT SEC_BUFFER_S SecBufferS
    )
{
    int i = 0;

    SecBufferS.length = pSecBufferS->length;
    SecBufferS.maxLength = pSecBufferS->maxLength;
    for (i = 0; i < S_BUFLEN; i++)
    {
        SecBufferS.buffer[i] = pSecBufferS->buffer[i];
    }
}
/*
 *  LsaGSSBuildAuthMessage
 *
 *  Builds NTLM authenticate message, which requires access to creds inside of
 *  lsassd.
 */
static
DWORD
LsaTransactGSSBuildAuthMessage(
    IN HANDLE          hServer,
    IN PSEC_BUFFER     credentials,
    IN PSEC_BUFFER_S   serverChallenge,
    IN PSEC_BUFFER     targetInfo,
    IN ULONG           negotiateFlags,
    OUT PSEC_BUFFER     authenticateMessage,
    OUT PSEC_BUFFER_S   baseSessionKey
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_MAKE_AUTH_MSG_REQ makeAuthMsgReq;
    // Do not free pResult and pError
    PLSA_GSS_R_MAKE_AUTH_MSG pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};


    makeAuthMsgReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    makeAuthMsgReq.negotiateFlags = negotiateFlags;

    LsaNTMLCopyGssSecBuffer(credentials, makeAuthMsgReq.credentials);
    LsaNTMLCopyGssSecBufferS(serverChallenge, makeAuthMsgReq.serverChallenge);
    LsaNTMLCopyGssSecBuffer(targetInfo, makeAuthMsgReq.targetInfo);

    request.tag = LSA_Q_GSS_MAKE_AUTH_MSG;
    request.object = &makeAuthMsgReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_GSS_MAKE_AUTH_MSG_SUCCESS:
            pResult = (PLSA_GSS_R_MAKE_AUTH_MSG)response.object;
            authenticateMessage = &(pResult->authenticateMessage);
            baseSessionKey = &(pResult->baseSessionKey);

            break;
        case LSA_R_GSS_MAKE_AUTH_MSG_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    // If dwError == 0, we have no LSA bail out in this function,
    // return the msgError value we get from NTML server call "LsaSrvIpcBuildAuthMessage"
    // Otherwise, we return dwError itself
    return !dwError ? pResult->msgError : dwError;
error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    authenticateMessage->buffer = NULL;
    authenticateMessage->length = authenticateMessage->maxLength = 0;

    memset(baseSessionKey->buffer, 0, S_BUFLEN);
    baseSessionKey->length = baseSessionKey->maxLength = 0;

    goto cleanup;
}

static
DWORD
LsaTransactGSSValidateAuthMessage(
    IN HANDLE          hServer,
    IN ULONG           negotiateFlags,
    IN PSEC_BUFFER_S   serverChallenge,
    IN PSEC_BUFFER     targetInfo,
    IN PSEC_BUFFER     authenticateMessage,
    OUT PSEC_BUFFER_S   baseSessionKey
    )
{
    DWORD dwError = 0;
    PLSA_CLIENT_CONNECTION_CONTEXT pContext =
                     (PLSA_CLIENT_CONNECTION_CONTEXT)hServer;
    LSA_IPC_CHECK_AUTH_MSG_REQ checkAuthMsgReq;
    // Do not free pResult and pError
    PLSA_GSS_R_CHECK_AUTH_MSG pResult = NULL;
    PLSA_IPC_ERROR pError = NULL;

    LWMsgMessage request = {-1, NULL};
    LWMsgMessage response = {-1, NULL};


    checkAuthMsgReq.Handle = (LsaIpcServerHandle*)pContext->hServer;
    checkAuthMsgReq.negotiateFlags = negotiateFlags;

    LsaNTMLCopyGssSecBufferS(serverChallenge, checkAuthMsgReq.serverChallenge);
    LsaNTMLCopyGssSecBuffer(targetInfo, checkAuthMsgReq.targetInfo);
    LsaNTMLCopyGssSecBuffer(authenticateMessage, checkAuthMsgReq.authenticateMessage);

    request.tag = LSA_Q_GSS_CHECK_AUTH_MSG;
    request.object = &checkAuthMsgReq;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_LSA_ERROR(dwError);

    switch (response.tag)
    {
        case LSA_R_GSS_CHECK_AUTH_MSG_SUCCESS:
            pResult = (PLSA_GSS_R_CHECK_AUTH_MSG)response.object;
            baseSessionKey = &(pResult->baseSessionKey);

            break;
        case LSA_R_GSS_CHECK_AUTH_MSG_FAILURE:
            pError = (PLSA_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    // If dwError == 0, we have no LSA bail out in this function,
    // return the msgError value we get from NTML server call "LsaSrvIpcBuildAuthMessage"
    // Otherwise, we return dwError itself
    return !dwError ? pResult->msgError : dwError;
error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    memset(baseSessionKey->buffer, 0, S_BUFLEN);
    baseSessionKey->length = baseSessionKey->maxLength = 0;

    goto cleanup;
}


LSASS_API
DWORD
LsaGSSBuildAuthMessage(
    IN HANDLE          hLsaConnection,
    IN PSEC_BUFFER     credentials,
    IN PSEC_BUFFER_S   serverChallenge,
    IN PSEC_BUFFER     targetInfo,
    IN ULONG           negotiateFlags,
    OUT PSEC_BUFFER     authenticateMessage,
    OUT PSEC_BUFFER_S   baseSessionKey
    )
{
    return LsaTransactGSSBuildAuthMessage(
                      hLsaConnection,
                      credentials,
                      serverChallenge,
                      targetInfo,
                      negotiateFlags,
                      authenticateMessage,
                      baseSessionKey);
}

/*
 *  LsaGSSValidateAuthMessage
 *
 *  Validate NTLM authenticate message, which requires access to creds inside of
 *  lsassd.
 */

LSASS_API
DWORD
LsaGSSValidateAuthMessage(
    IN HANDLE          hLsaConnection,
    IN ULONG           negotiateFlags,
    IN PSEC_BUFFER_S   serverChallenge,
    IN PSEC_BUFFER     targetInfo,
    IN PSEC_BUFFER     authenticateMessage,
    OUT PSEC_BUFFER_S   baseSessionKey
    )
{
    return LsaTransactGSSValidateAuthMessage(
                        hLsaConnection,
                        negotiateFlags,
                        serverChallenge,
                        targetInfo,
                        authenticateMessage,
                        baseSessionKey);
}

