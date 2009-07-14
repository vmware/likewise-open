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
#include "api.h"

#define ZERO_STRUCT(_s_) memset((char*)&(_s_),0,sizeof(_s_))

static
VOID
LsaNTLMGssFreeAuthMsg(
    PLSA_GSS_R_MAKE_AUTH_MSG pAuthMsgReply
    )
{
    if (pAuthMsgReply)
    {
        NTLMGssFreeSecBuffer(&pAuthMsgReply->authenticateMessage);
        LSA_SAFE_FREE_MEMORY(pAuthMsgReply);
    }
}

LWMsgStatus
LsaSrvIpcBuildAuthMessage(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_MAKE_AUTH_MSG_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    PLSA_GSS_R_MAKE_AUTH_MSG pAuthMsgReply = NULL;
    uid_t peerUID = 0;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(sizeof(*pAuthMsgReply),
                                (PVOID*)&pAuthMsgReply);
    BAIL_ON_LSA_ERROR(dwError);

    ZERO_STRUCT(pAuthMsgReply->authenticateMessage);
    ZERO_STRUCT(pAuthMsgReply->baseSessionKey);

    LsaSrvGetUid((HANDLE)Handle, &peerUID);

    pAuthMsgReply->msgError = NTLMGssBuildAuthenticateMessage(
                        pReq->negotiateFlags,
                        peerUID,
                        &pReq->credentials,
                        &pReq->serverChallenge,
                        &pReq->targetInfo,
                        &pAuthMsgReply->authenticateMessage,
                        &pAuthMsgReply->baseSessionKey);

    pResponse->tag = LSA_R_GSS_MAKE_AUTH_MSG_SUCCESS;
    pResponse->object = pAuthMsgReply;

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);

    pResponse->tag = LSA_R_GSS_MAKE_AUTH_MSG_FAILURE;
    pResponse->object = pError;

    if(pAuthMsgReply)
    {
        LsaNTLMGssFreeAuthMsg(pAuthMsgReply);
    }

    goto cleanup;
}

LWMsgStatus
LsaSrvIpcCheckAuthMessage(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PLSA_IPC_CHECK_AUTH_MSG_REQ pReq = pRequest->object;
    PLSA_IPC_ERROR pError = NULL;
    PLSA_GSS_R_CHECK_AUTH_MSG pCheckAuthMsgReply = NULL;

    dwError = LsaAllocateMemory(sizeof(*pCheckAuthMsgReply),
                                (PVOID*)&pCheckAuthMsgReply);
    BAIL_ON_LSA_ERROR(dwError);

    ZERO_STRUCT(pCheckAuthMsgReply->baseSessionKey);

    pCheckAuthMsgReply->msgError = NTLMGssCheckAuthenticateMessage(
                                        pReq->negotiateFlags,
                                        &pReq->serverChallenge,
                                        &pReq->targetInfo,
                                        &pReq->authenticateMessage,
                                        &pCheckAuthMsgReply->baseSessionKey);

    pResponse->tag = LSA_R_GSS_CHECK_AUTH_MSG_SUCCESS;
    pResponse->object = pCheckAuthMsgReply;

cleanup:
    return MAP_LW_ERROR_IPC(dwError);

error:
    dwError = LsaSrvIpcCreateError(dwError, NULL, &pError);

    pResponse->tag = LSA_R_GSS_CHECK_AUTH_MSG_FAILURE;
    pResponse->object = pError;

    if(pCheckAuthMsgReply)
    {
        LSA_SAFE_FREE_MEMORY(pCheckAuthMsgReply);
    }

    goto cleanup;
}
