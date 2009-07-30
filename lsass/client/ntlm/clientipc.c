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
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CLIENT_CONNECTION_CONTEXT pContext = NULL;
    static LWMsgTime connectTimeout = {2, 0};

    BAIL_ON_NTLM_INVALID_POINTER(phConnection);

    dwError = LsaAllocateMemory(
        sizeof(NTLM_CLIENT_CONNECTION_CONTEXT),
        (PVOID*)(PVOID)&pContext
        );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_protocol_new(NULL, &pContext->pProtocol)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_protocol_add_protocol_spec(
            pContext->pProtocol,
            NtlmIpcGetProtocolSpec()
            )
        );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_connection_new(NULL, pContext->pProtocol, &pContext->pAssoc)
        );

    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_connection_set_endpoint(
                                  pContext->pAssoc,
                                  LWMSG_CONNECTION_MODE_LOCAL,
                                  CACHEDIR "/" NTLM_SERVER_FILENAME));
    BAIL_ON_NTLM_ERROR(dwError);

    if(getenv("LW_DISABLE_CONNECT_TIMEOUT") == NULL)
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
    if(pContext)
    {
        if(pContext->pAssoc)
        {
            lwmsg_assoc_delete(pContext->pAssoc);
        }
        if(pContext->pProtocol)
        {
            lwmsg_protocol_delete(pContext->pProtocol);
        }
        LsaFreeMemory(pContext);
    }
    if(phConnection)
    {
        *phConnection = (HANDLE)INVALID_HANDLE;
    }
    goto cleanup;
}

DWORD
NtlmCloseServer(
    HANDLE hConnection
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hConnection;

    if(pContext->pAssoc)
    {
        lwmsg_assoc_close(pContext->pAssoc);
        lwmsg_assoc_delete(pContext->pAssoc);
    }

    if(pContext->pProtocol)
    {
        lwmsg_protocol_delete(pContext->pProtocol);
    }

    LsaFreeMemory(pContext);

    return dwError;
}

DWORD
NtlmTransactAcceptSecurityContext(
    IN HANDLE hServer,
    IN PLSA_CRED_HANDLE phCredential,
    IN OUT PLSA_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pInput,
    IN DWORD fContextReq,
    IN DWORD TargetDataRep,
    IN OUT PLSA_CONTEXT_HANDLE phNewContext,
    IN OUT PSecBufferDesc pOutput,
    OUT PDWORD  pfContextAttr,
    OUT PTimeStamp ptsTimeStamp
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_ACCEPT_SEC_CTXT_REQ AcceptSecCtxtReq;

    memset(&AcceptSecCtxtReq, 0, sizeof(NTLM_IPC_ACCEPT_SEC_CTXT_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    AcceptSecCtxtReq.hCredential = *phCredential;
    AcceptSecCtxtReq.hContext = *phContext;
    AcceptSecCtxtReq.pInput = pInput;
    AcceptSecCtxtReq.fContextReq = fContextReq;
    AcceptSecCtxtReq.TargetDataRep = TargetDataRep;
    AcceptSecCtxtReq.hNewContext = *phNewContext;
    AcceptSecCtxtReq.pOutput = pOutput;

    request.tag = NTLM_Q_ACCEPT_SEC_CTXT;
    request.object = &AcceptSecCtxtReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_ACCEPT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE)response.object;

            dwError = NtlmTransferSecBufferDesc(pOutput, &pResultList->Output);
            BAIL_ON_NTLM_ERROR(dwError);

            *phContext = pResultList->hContext;
            *phNewContext = pResultList->hNewContext;
            *pfContextAttr = pResultList->fContextAttr;
            *ptsTimeStamp = pResultList->tsTimeStamp;

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
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
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
    OUT PLSA_CRED_HANDLE phCredential,
    OUT PTimeStamp ptsExpiry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_ACQUIRE_CREDS_REQ AcquireCredsReq;

    memset(&AcquireCredsReq, 0, sizeof(AcquireCredsReq));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_ACQUIRE_CREDS_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    AcquireCredsReq.pszPrincipal = pszPrincipal;
    AcquireCredsReq.pszPackage = pszPackage;
    AcquireCredsReq.fCredentialUse = fCredentialUse;
    AcquireCredsReq.pvLogonID = pvLogonID;
    AcquireCredsReq.pAuthData = pAuthData;

    request.tag = NTLM_Q_ACQUIRE_CREDS;
    request.object = &AcquireCredsReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_ACQUIRE_CREDS_SUCCESS:
            pResultList = (PNTLM_IPC_ACQUIRE_CREDS_RESPONSE)response.object;

            *phCredential = pResultList->hCredential;
            *ptsExpiry = pResultList->tsExpiry;

            break;
        case NTLM_R_ACQUIRE_CREDS_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if (response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    goto cleanup;
}

DWORD
NtlmTransactDecryptMessage(
    IN HANDLE hServer,
    IN PLSA_CONTEXT_HANDLE phContext,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbEncrypted
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_DECRYPT_MSG_REQ DecryptMsgReq;

    memset(&DecryptMsgReq, 0, sizeof(NTLM_IPC_DECRYPT_MSG_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_DECRYPT_MSG_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    DecryptMsgReq.hContext = *phContext;
    DecryptMsgReq.pMessage = pMessage;
    DecryptMsgReq.MessageSeqNo = MessageSeqNo;

    request.tag = NTLM_Q_DECRYPT_MSG;
    request.object = &DecryptMsgReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_DECRYPT_MSG_SUCCESS:
            pResultList = (PNTLM_IPC_DECRYPT_MSG_RESPONSE)response.object;

            dwError = NtlmTransferSecBufferDesc(
                pMessage,
                &pResultList->Message
                );
            BAIL_ON_NTLM_ERROR(dwError);

            memcpy(pbEncrypted, &pResultList->bEncrypted, sizeof(BOOL));

            break;
        case NTLM_R_DECRYPT_MSG_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransactDeleteSecurityContext(
    IN HANDLE hServer,
    IN PLSA_CONTEXT_HANDLE phContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_DELETE_SEC_CTXT_REQ DeleteSecCtxtReq;

    memset(&DeleteSecCtxtReq, 0, sizeof(NTLM_IPC_DELETE_SEC_CTXT_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    DeleteSecCtxtReq.hContext = *phContext;

    request.tag = NTLM_Q_DELETE_SEC_CTXT;
    request.object = &DeleteSecCtxtReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_DELETE_SEC_CTXT_SUCCESS:
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
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransactEncryptMessage(
    IN HANDLE hServer,
    IN PLSA_CONTEXT_HANDLE phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_ENCRYPT_MSG_REQ EncryptMsgReq;

    memset(&EncryptMsgReq, 0, sizeof(NTLM_IPC_ENCRYPT_MSG_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_ENCRYPT_MSG_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    EncryptMsgReq.hContext = *phContext;
    EncryptMsgReq.bEncrypt = bEncrypt;
    EncryptMsgReq.pMessage = pMessage;
    EncryptMsgReq.MessageSeqNo = MessageSeqNo;

    request.tag = NTLM_Q_ENCRYPT_MSG;
    request.object = &EncryptMsgReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_ENCRYPT_MSG_SUCCESS:
            pResultList = (PNTLM_IPC_ENCRYPT_MSG_RESPONSE)response.object;

            dwError = NtlmTransferSecBufferDesc(
                pMessage,
                &pResultList->Message
                );
            BAIL_ON_NTLM_ERROR(dwError);

            break;
        case NTLM_R_ENCRYPT_MSG_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransactExportSecurityContext(
    IN HANDLE hServer,
    IN PLSA_CONTEXT_HANDLE phContext,
    IN DWORD fFlags,
    OUT PSecBuffer pPackedContext,
    OUT OPTIONAL HANDLE *pToken
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_EXPORT_SEC_CTXT_REQ ExportSecCtxtReq;

    memset(&ExportSecCtxtReq, 0, sizeof(NTLM_IPC_EXPORT_SEC_CTXT_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_EXPORT_SEC_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    ExportSecCtxtReq.hContext = *phContext;
    ExportSecCtxtReq.fFlags = fFlags;

    request.tag = NTLM_Q_EXPORT_SEC_CTXT;
    request.object = &ExportSecCtxtReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_EXPORT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_EXPORT_SEC_CTXT_RESPONSE)response.object;

            pPackedContext->cbBuffer = pResultList->PackedContext.cbBuffer;
            pPackedContext->BufferType = pResultList->PackedContext.BufferType;
            pPackedContext->pvBuffer = pResultList->PackedContext.pvBuffer;

            if(pToken)
            {
                memcpy(pToken, &pResultList->hToken, sizeof(HANDLE));
            }

            break;
        case NTLM_R_EXPORT_SEC_CTXT_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransactFreeCredentialsHandle(
    IN HANDLE hServer,
    IN PLSA_CRED_HANDLE phCredential
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_FREE_CREDS_REQ FreeCredsReq;

    memset(&FreeCredsReq, 0, sizeof(NTLM_IPC_FREE_CREDS_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    FreeCredsReq.hCredential = *phCredential;

    request.tag = NTLM_Q_FREE_CREDS;
    request.object = &FreeCredsReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_FREE_CREDS_SUCCESS:
            break;
        case NTLM_R_FREE_CREDS_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransactImportSecurityContext(
    IN HANDLE hServer,
    IN PSECURITY_STRING *pszPackage,
    IN PSecBuffer pPackedContext,
    IN OPTIONAL HANDLE pToken,
    OUT PLSA_CONTEXT_HANDLE phContext
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_IMPORT_SEC_CTXT_REQ ImportSecCtxtReq;

    memset(&ImportSecCtxtReq, 0, sizeof(NTLM_IPC_IMPORT_SEC_CTXT_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_IMPORT_SEC_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    ImportSecCtxtReq.pszPackage = pszPackage;
    ImportSecCtxtReq.pPackedContext = pPackedContext;
    ImportSecCtxtReq.pToken = pToken;

    request.tag = NTLM_Q_IMPORT_SEC_CTXT;
    request.object = &ImportSecCtxtReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_IMPORT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_IMPORT_SEC_CTXT_RESPONSE)response.object;

            *phContext = pResultList->hContext;

            break;
        case NTLM_R_IMPORT_SEC_CTXT_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransactInitializeSecurityContext(
    IN HANDLE hServer,
    IN OPTIONAL PLSA_CRED_HANDLE phCredential,
    IN OPTIONAL PLSA_CONTEXT_HANDLE phContext,
    IN OPTIONAL SEC_CHAR * pszTargetName,
    IN DWORD fContextReq,
    IN DWORD Reserved1,
    IN DWORD TargetDataRep,
    IN OPTIONAL PSecBufferDesc pInput,
    IN DWORD Reserved2,
    IN OUT OPTIONAL PLSA_CONTEXT_HANDLE phNewContext,
    IN OUT OPTIONAL PSecBufferDesc pOutput,
    OUT PDWORD pfContextAttr,
    OUT OPTIONAL PTimeStamp ptsExpiry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
                     (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_INIT_SEC_CTXT_REQ InitSecCtxtReq;

    memset(&InitSecCtxtReq, 0, sizeof(NTLM_IPC_INIT_SEC_CTXT_REQ));

    // Do not free pResult and pError
    // change this one to the corret results list when ready
    PNTLM_IPC_INIT_SEC_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    if(phCredential)
    {
        InitSecCtxtReq.hCredential = *phCredential;
    }
    if(phContext)
    {
        InitSecCtxtReq.hContext = *phContext;
    }
    InitSecCtxtReq.pszTargetName = pszTargetName;
    InitSecCtxtReq.fContextReq = fContextReq;
    InitSecCtxtReq.Reserved1 = Reserved1;
    InitSecCtxtReq.TargetDataRep = TargetDataRep;
    InitSecCtxtReq.pInput = pInput;
    InitSecCtxtReq.Reserved2 = Reserved2;
    if(phNewContext)
    {
        InitSecCtxtReq.hNewContext = *phNewContext;
    }
    InitSecCtxtReq.pOutput = pOutput;

    request.tag = NTLM_Q_INIT_SEC_CTXT;
    request.object = &InitSecCtxtReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_INIT_SEC_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_INIT_SEC_CTXT_RESPONSE)response.object;

            if(pOutput)
            {

                dwError = NtlmTransferSecBufferDesc(
                    pOutput,
                    &pResultList->Output
                    );
                BAIL_ON_NTLM_ERROR(dwError);

            }

            if(phNewContext)
            {
                *phNewContext = pResultList->hNewContext;
            }

            *pfContextAttr = pResultList->fContextAttr;

            if(ptsExpiry)
            {
               *ptsExpiry = pResultList->tsExpiry;
            }

            dwError = pResultList->dwStatus;

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
    return dwError;

error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }

    *phNewContext = NULL;
    *pfContextAttr = 0;
    *ptsExpiry = 0;
    memset(pOutput, 0, sizeof(SecBufferDesc));
    goto cleanup;
}

DWORD
NtlmTransactMakeSignature(
    IN HANDLE hServer,
    IN PLSA_CONTEXT_HANDLE phContext,
    IN BOOL bEncrypt,
    IN OUT PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_MAKE_SIGN_REQ MakeSignReq;

    memset(&MakeSignReq, 0, sizeof(NTLM_IPC_MAKE_SIGN_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_MAKE_SIGN_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    MakeSignReq.hContext = *phContext;
    MakeSignReq.bEncrypt = bEncrypt;
    MakeSignReq.pMessage = pMessage;
    MakeSignReq.MessageSeqNo = MessageSeqNo;

    request.tag = NTLM_Q_MAKE_SIGN;
    request.object = &MakeSignReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_MAKE_SIGN_SUCCESS:
            pResultList = (PNTLM_IPC_MAKE_SIGN_RESPONSE)response.object;

            dwError = NtlmTransferSecBufferDesc(
                pMessage,
                &pResultList->Message
                );
            BAIL_ON_NTLM_ERROR(dwError);

            break;
        case NTLM_R_MAKE_SIGN_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransactQueryContextAttributes(
    IN HANDLE hServer,
    IN PLSA_CONTEXT_HANDLE phContext,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_QUERY_CTXT_REQ QueryCtxtReq;

    memset(&QueryCtxtReq, 0, sizeof(NTLM_IPC_QUERY_CTXT_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_QUERY_CTXT_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    QueryCtxtReq.hContext = *phContext;
    QueryCtxtReq.ulAttribute = ulAttribute;

    request.tag = NTLM_Q_QUERY_CTXT;
    request.object = &QueryCtxtReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_QUERY_CTXT_SUCCESS:
            pResultList = (PNTLM_IPC_QUERY_CTXT_RESPONSE)response.object;

            // for now, the only result we query for is a size structure
            // which doesn't need a deep copy
            memcpy(pBuffer, pResultList->pBuffer, pResultList->dwBufferSize);

            break;
        case NTLM_R_QUERY_CTXT_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransactQueryCredentialsAttributes(
    IN HANDLE hServer,
    IN PLSA_CRED_HANDLE phCredential,
    IN DWORD ulAttribute,
    OUT PVOID pBuffer
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_QUERY_CREDS_REQ QueryCredsReq;

    memset(&QueryCredsReq, 0, sizeof(NTLM_IPC_QUERY_CREDS_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_QUERY_CREDS_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    QueryCredsReq.hCredential = *phCredential;
    QueryCredsReq.ulAttribute = ulAttribute;

    request.tag = NTLM_Q_QUERY_CREDS;
    request.object = &QueryCredsReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_QUERY_CREDS_SUCCESS:
            pResultList = (PNTLM_IPC_QUERY_CREDS_RESPONSE)response.object;

            // This should be a name of a user we're getting back, so this copy
            // should be sufficient
            memcpy(pBuffer, pResultList->pBuffer, pResultList->dwBufferSize);

            break;
        case NTLM_R_QUERY_CREDS_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransactVerifySignature(
    IN HANDLE hServer,
    IN PLSA_CONTEXT_HANDLE phContext,
    IN PSecBufferDesc pMessage,
    IN DWORD MessageSeqNo,
    OUT PBOOL pbVerified,
    OUT PBOOL pbEncrypted
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    PNTLM_CLIENT_CONNECTION_CONTEXT pContext =
        (PNTLM_CLIENT_CONNECTION_CONTEXT)hServer;

    NTLM_IPC_VERIFY_SIGN_REQ VerifySignReq;

    memset(&VerifySignReq, 0, sizeof(NTLM_IPC_VERIFY_SIGN_REQ));

    // Do not free pResult and pError
    // Change this to the correct result list when ready.
    PNTLM_IPC_VERIFY_SIGN_RESPONSE pResultList = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    LWMsgMessage request = LWMSG_MESSAGE_INITIALIZER;
    LWMsgMessage response = LWMSG_MESSAGE_INITIALIZER;

    VerifySignReq.hContext = *phContext;
    VerifySignReq.pMessage = pMessage;
    VerifySignReq.MessageSeqNo = MessageSeqNo;

    request.tag = NTLM_Q_VERIFY_SIGN;
    request.object = &VerifySignReq;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_send_message_transact(
                              pContext->pAssoc,
                              &request,
                              &response));
    BAIL_ON_NTLM_ERROR(dwError);

    switch(response.tag)
    {
        case NTLM_R_VERIFY_SIGN_SUCCESS:
            pResultList = (PNTLM_IPC_VERIFY_SIGN_RESPONSE)response.object;

            memcpy(pbVerified, &pResultList->bVerified, sizeof(BOOL));
            memcpy(pbEncrypted, &pResultList->bEncrypted, sizeof(BOOL));

            break;
        case NTLM_R_VERIFY_SIGN_FAILURE:
            pError = (PNTLM_IPC_ERROR) response.object;
            dwError = pError->dwError;
            BAIL_ON_NTLM_ERROR(dwError);
            break;
        default:
            dwError = EINVAL;
            BAIL_ON_NTLM_ERROR(dwError);
    }

cleanup:
    return dwError;
error:
    if(response.object)
    {
        lwmsg_assoc_free_message(pContext->pAssoc, &response);
    }
    goto cleanup;
}

DWORD
NtlmTransferSecBufferDesc(
    OUT PSecBufferDesc pOut,
    IN PSecBufferDesc pIn
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD nIndex = 0;

    if(pOut->cBuffers != pIn->cBuffers)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LW_ERROR(dwError);
    }

    for(nIndex= 0; nIndex < pIn->cBuffers; nIndex++)
    {
        pOut->pBuffers[nIndex].pvBuffer = pIn->pBuffers[nIndex].pvBuffer;
        pOut->pBuffers[nIndex].BufferType = pIn->pBuffers[nIndex].BufferType;
        pOut->pBuffers[nIndex].cbBuffer = pIn->pBuffers[nIndex].cbBuffer;
    }

cleanup:
    return dwError;
error:
    goto cleanup;

}
