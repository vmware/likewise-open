/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        clientipc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication (NTLM Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "client.h"

DWORD
NtlmOpenServer(
    PHANDLE phConnection
    )
{
    DWORD dwError = 0;
    PNTLM_CLIENT_CONNECTION_CONTEXT pContext = NULL;
    static LWMsgTime connectTimeout = {2, 0};

    BAIL_ON_NTLM_INVALID_POINTER(phConnection);

    dwError = LsaAllocateMemory(sizeof(NTLM_CLIENT_CONNECTION_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_protocol_new(NULL, &pContext->pProtocol));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(pContext->pProtocol, NtlmIpcGetProtocolSpec()));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_connection_new(NULL, pContext->pProtocol, &pContext->pAssoc));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_connection_set_endpoint(
                                  pContext->pAssoc,
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  CACHEDIR "/" NTLM_SERVER_FILENAME));
    BAIL_ON_NTLM_ERROR(dwError);

    /* Attempt to automatically restore the connection if the server closes it.
       This allows us to transparently recover from lsassd being restarted
       as long as:

       1. No operation is attempted during the window that lsassd is down
       2. The shutdown did not wipe out any state such as enumeration handles

       lwmsg will handle reconnecting for us if these conditions are met.
    */

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_set_action(
                                  pContext->pAssoc,
                                  LWMSG_STATUS_PEER_RESET,
                                  LWMSG_ASSOC_ACTION_RESET_AND_RETRY));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_set_action(
                                  pContext->pAssoc,
                                  LWMSG_STATUS_PEER_CLOSE,
                                  LWMSG_ASSOC_ACTION_RESET_AND_RETRY));
    BAIL_ON_NTLM_ERROR(dwError);

    if (getenv("LW_DISABLE_CONNECT_TIMEOUT") == NULL)
    {
        /* Give up connecting within 2 seconds in case lsassd
           is unresponsive (e.g. it's being traced in a debugger) */
        dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_set_timeout(
                                      pContext->pAssoc,
                                      LWMSG_TIMEOUT_ESTABLISH,
                                      &connectTimeout));
        BAIL_ON_NTLM_ERROR(dwError);
    }

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_establish(pContext->pAssoc));
    BAIL_ON_NTLM_ERROR(dwError);

    *phConnection = (HANDLE)pContext;

cleanup:
    return dwError;

error:
    if (pContext)
    {
        if (pContext->pAssoc)
        {
            lwmsg_assoc_delete(pContext->pAssoc);
        }

        if (pContext->pProtocol)
        {
            lwmsg_protocol_delete(pContext->pProtocol);
        }

        LsaFreeMemory(pContext);
    }

    if (phConnection)
    {
        *phConnection = (HANDLE)NULL;
    }

    goto cleanup;
}

DWORD
NtlmCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = 0;
    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
                     (PNTLM_CLIENT_CONNECTION_CONTEXT)hConnection;

    if (pContext->pAssoc)
    {
        lwmsg_assoc_close(pContext->pAssoc);
        lwmsg_assoc_delete(pContext->pAssoc);
    }

    if (pContext->pProtocol)
    {
        lwmsg_protocol_delete(pContext->pProtocol);
    }

    LsaFreeMemory(pContext);

    return dwError;
}

DWORD
NtlmTransactAcceptSecurityContext(
    IN HANDLE hServer,
    IN PCredHandle phCredential,
    IN OUT PCtxtHandle phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PCtxtHandle phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    )
{
    DWORD dwError = 0;
    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
                     (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;
    NTLM_IPC_ACCEPT_SEC_CTXT_REQ AcceptSecCtxtReq;

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_ERROR pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    AcceptSecCtxtReq.phCredential = phCredential;
    AcceptSecCtxtReq.phContext = phContext;
    AcceptSecCtxtReq.pInput = pInput;
    AcceptSecCtxtReq.fContextReq = fContextReq;
    AcceptSecCtxtReq.TargetDataRep = TargetDataRep;
    AcceptSecCtxtReq.phNewContext = phNewContext;
    AcceptSecCtxtReq.pOutput = pOutput;

    request.tag = NTLM_Q_ACCEPT_SEC_CTXT;
    request.object = &AcceptSecCtxtReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch (response.tag)
    {
        case NTLM_R_ACCEPT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_ERROR)response.object;
            break;
        case NTLM_R_ACCEPT_SEC_CTXT_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
NtlmTransactAcquireCredentialsHandle(
    IN HANDLE hServer,
    IN SEC_CHAR *pszPrincipal,
    IN SEC_CHAR *pszPackage,
    IN DWORD fCredentialUse,
    IN PLUID pvLogonID,
    IN PVOID pAuthData,
    // NOT USED BY NTLM - IN SEC_GET_KEY_FN pGetKeyFn,
    // NOT USED BY NTLM - IN PVOID pvGetKeyArgument,
    OUT PCredHandle phCredential,
    OUT PTimeStamp ptsExpiry
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactDecryptMessage(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbEncrypted
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactDeleteSecurityContext(
    IN HANDLE hServer,
    IN PCtxtHandle phContext
    )
{
    DWORD dwError = 0;
    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
                     (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;
    NTLM_IPC_DELETE_SEC_CTXT_REQ AcceptSecCtxtReq;

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_ERROR pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    AcceptSecCtxtReq.phContext = phContext;

    request.tag = NTLM_Q_DELETE_SEC_CTXT;
    request.object = &AcceptSecCtxtReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch (response.tag)
    {
        case NTLM_R_DELETE_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_ERROR)response.object;
            break;
        case NTLM_R_DELETE_SEC_CTXT_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
NtlmTransactEncryptMessage(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactExportSecurityContext(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN DWORD fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactFreeCredentialsHandle(
    IN HANDLE hServer,
    IN PCredHandle phCredential
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactImportSecurityContext(
    IN HANDLE hServer,
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PCtxtHandle phContext
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactInitializeSecurityContext(
    IN HANDLE hServer,
    IN OPTIONAL PCredHandle phCredential,
    IN OPTIONAL PCtxtHandle phContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PCtxtHandle phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    )
{
    DWORD dwError = 0;
    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
                     (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;
    NTLM_IPC_INIT_SEC_CTXT_REQ InitSecCtxtReq;

    // Do not free pResult and pError
    // change this one to the corret results list when ready
    PNTLM_IPC_ERROR pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    InitSecCtxtReq.phCredential = phCredential;
    InitSecCtxtReq.phContext = phContext;
    InitSecCtxtReq.pszTargetName = pszTargetName;
    InitSecCtxtReq.fContextReq = fContextReq;
    InitSecCtxtReq.Reserved1 = Reserved1;
    InitSecCtxtReq.TargetDataRep = TargetDataRep;
    InitSecCtxtReq.pInput = pInput;
    InitSecCtxtReq.Reserved2 = Reserved2;
    InitSecCtxtReq.phNewContext = phNewContext;
    InitSecCtxtReq.pOutput = pOutput;


    request.tag = NTLM_Q_INIT_SEC_CTXT;
    request.object = &InitSecCtxtReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch (response.tag)
    {
        case NTLM_R_INIT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_ERROR)response.object;
            break;
        case NTLM_R_INIT_SEC_CTXT_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    return dwError;

error:
    memset(phNewContext, 0, sizeof(CtxtHandle));
    pfContextAttr = 0;
    memset(ptsExpiry, 0, sizeof(TimeStamp));
    memset(pOutput, 0, sizeof(SecBufferDesc));
    goto cleanup;
}

DWORD
NtlmTransactMakeSignature(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactQueryContextAttributes(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactQueryCredentialsAttributes(
    IN HANDLE hServer,
    IN PCredHandle phCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactQuerySecurityContextAttributes(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
NtlmTransactVerifySignature(
    IN HANDLE hServer,
    IN PCtxtHandle phContext,
    IN PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbEncrypted
    )
{
    DWORD dwError = 0;

    BAIL_ON_NTLM_ERROR(dwError);

cleanup:
    return dwError;
error:
    goto cleanup;
}
