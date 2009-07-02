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
 *        ntlmsrvipc.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Inter-process Communication API (NTLM Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#include "ntlmsrvapi.h"

DWORD
NtlmSrvApiInit(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;

    dwError = pthread_rwlock_init(&gpNtlmContextList_rwlock, NULL);
    if(LSA_ERROR_SUCCESS != dwError)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_NTLM_ERROR(dwError);
    }

error:
    return dwError;
}

DWORD
NtlmSrvApiShutdown(
    VOID
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    ENTER_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);

    dwError = NtlmRemoveAllContext();

    LEAVE_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);

    return dwError;
}

LWMsgDispatchSpec*
NtlmSrvGetDispatchSpec(
    void
    )
{
    return gNtlmMessageHandlers;
}

DWORD
NtlmSrvIpcCreateError(
    DWORD dwErrorCode,
    PNTLM_IPC_ERROR* ppError
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LsaAllocateMemory(sizeof(*pError), (void**) (void*) &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = dwErrorCode;

    *ppError = pError;

error:
    return dwError;
}

LWMsgStatus
NtlmSrvIpcAcceptSecurityContext(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ACCEPT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    DWORD fContextAttr = 0;
    TimeStamp tsTimeStamp;

    memset(&tsTimeStamp, 0, sizeof(TimeStamp));

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerAcceptSecurityContext(
        //(HANDLE)Handle,
        pReq->phCredential,
        pReq->phContext,
        pReq->pInput,
        pReq->fContextReq,
        pReq->TargetDataRep,
        pReq->phNewContext,
        pReq->pOutput,
        &fContextAttr,
        &tsTimeStamp
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACCEPT_SEC_CTXT_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_ACCEPT_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcAcquireCredentialsHandle(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ACQUIRE_CREDS_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    CredHandle hCredential;
    TimeStamp tsExpiry;

    memset(&hCredential, 0, sizeof(CredHandle));
    memset(&tsExpiry, 0, sizeof(TimeStamp));

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerAcquireCredentialsHandle(
        //(HANDLE)Handle,
        pReq->pszPrincipal,
        pReq->pszPackage,
        pReq->fCredentialUse,
        pReq->pvLogonID,
        pReq->pAuthData,
        // NOT NEEDED FOR NTLM - pReq->pGetKeyFn,
        // NOT NEEDED FOR NTLM - pReq->pvGetKeyArgument,
        &hCredential,
        &tsExpiry
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDS_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcDecryptMessage(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_DECRYPT_MSG_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    DWORD nQoP = 0;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerDecryptMessage(
        //(HANDLE)Handle,
        pReq->phContext,
        pReq->pMessage,
        pReq->MessageSeqNo,
        &nQoP
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_DECRYPT_MSG_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_DECRYPT_MSG_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcDeleteSecurityContext(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_DELETE_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerDeleteSecurityContext(
        //(HANDLE)Handle,
        pReq->phContext
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_DELETE_SEC_CTXT_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_DELETE_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcEncryptMessage(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ENCRYPT_MSG_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerEncryptMessage(
        pReq->phContext,
        pReq->bEncrypt,
        pReq->pMessage,
        pReq->MessageSeqNo
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_ENCRYPT_MSG_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_ENCRYPT_MSG_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}


LWMsgStatus
NtlmSrvIpcExportSecurityContext(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_EXPORT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerExportSecurityContext(
        //(HANDLE)Handle,
        pReq->phContext,
        pReq->fFlags,
        pReq->pPackedContext,
        pReq->pToken
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_EXPORT_SEC_CTXT_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_EXPORT_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}
LWMsgStatus
NtlmSrvIpcFreeCredentialsHandle(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_FREE_CREDS_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerFreeCredentialsHandle(
        //(HANDLE)Handle,
        pReq->phCredential
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_FREE_CREDS_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_FREE_CREDS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}


LWMsgStatus
NtlmSrvIpcImportSecurityContext(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_IMPORT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerImportSecurityContext(
        //(HANDLE)Handle,
        pReq->pszPackage,
        pReq->pPackedContext,
        pReq->pToken,
        pReq->phContext
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_IMPORT_SEC_CTXT_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_IMPORT_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}
LWMsgStatus
NtlmSrvIpcInitializeSecurityContext(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_INIT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    DWORD nContextAttr = 0;
    TimeStamp tsExpiry;

    memset(&tsExpiry, 0, sizeof(TimeStamp));

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerInitializeSecurityContext(
        //(HANDLE)Handle,
        pReq->phCredential,
        pReq->phContext,
        pReq->pszTargetName,
        pReq->fContextReq,
        pReq->Reserved1,
        pReq->TargetDataRep,
        pReq->pInput,
        pReq->Reserved2,
        pReq->phNewContext,
        pReq->pOutput,
        &nContextAttr,
        &tsExpiry
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_INIT_SEC_CTXT_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_INIT_SEC_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}



LWMsgStatus
NtlmSrvIpcMakeSignature(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_MAKE_SIGN_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerMakeSignature(
        //(HANDLE)Handle,
        pReq->phContext,
        pReq->bEncrypt,
        pReq->pMessage,
        pReq->MessageSeqNo
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_MAKE_SIGN_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_MAKE_SIGN_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}
LWMsgStatus
NtlmSrvIpcQueryCredentialsAttributes(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_QUERY_CREDS_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    PVOID pBuffer = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerQueryCredentialsAttributes(
        //(HANDLE)Handle,
        pReq->phCredential,
        pReq->ulAttribute,
        pBuffer
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_QUERY_CREDS_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_QUERY_CREDS_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}


LWMsgStatus
NtlmSrvIpcQueryContextAttributes(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_QUERY_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    PVOID pBuffer = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerQueryContextAttributes(
        //(HANDLE)Handle,
        pReq->phContext,
        pReq->ulAttribute,
        pBuffer
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_QUERY_CTXT_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_QUERY_CTXT_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}
LWMsgStatus
NtlmSrvIpcVerifySignature(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_VERIFY_SIGN_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    BOOL bEncrypted = 0;

    dwError = NTLM_MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerVerifySignature(
        //(HANDLE)Handle,
        pReq->phContext,
        pReq->pMessage,
        pReq->MessageSeqNo,
        &bEncrypted
        );

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    if (!dwError)
    {
        pResponse->tag = NTLM_R_VERIFY_SIGN_SUCCESS;
        pResponse->object = pError;
    }
    else
    {
        pResponse->tag = NTLM_R_VERIFY_SIGN_FAILURE;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}
