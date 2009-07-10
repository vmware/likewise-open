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
    if(LW_ERROR_SUCCESS != dwError)
    {
        dwError = LW_ERROR_INTERNAL;
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
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    ENTER_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);

    dwError = NtlmRemoveAllContext();

    LEAVE_NTLM_CONTEXT_LIST_WRITER_LOCK(bInLock);

    return dwError;
}

LWMsgDispatchSpec*
NtlmSrvGetDispatchSpec(
    VOID
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

    dwError = LwAllocateMemory(sizeof(*pError), (PVOID*)(PVOID)&pError);
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ACCEPT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    DWORD fContextAttr = 0;
    TimeStamp tsTimeStamp;

    memset(&tsTimeStamp, 0, sizeof(TimeStamp));

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerAcceptSecurityContext(
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

    if(!pError->dwError)
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
    PVOID data
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

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerAcquireCredentialsHandle(
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

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_DECRYPT_MSG_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    DWORD nQoP = 0;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerDecryptMessage(
        pReq->phContext,
        pReq->pMessage,
        pReq->MessageSeqNo,
        &nQoP
        );

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_DELETE_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerDeleteSecurityContext(
        pReq->phContext
        );

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ENCRYPT_MSG_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerEncryptMessage(
        pReq->phContext,
        pReq->bEncrypt,
        pReq->pMessage,
        pReq->MessageSeqNo
        );

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_EXPORT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerExportSecurityContext(
        pReq->phContext,
        pReq->fFlags,
        pReq->pPackedContext,
        pReq->pToken
        );

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_FREE_CREDS_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerFreeCredentialsHandle(
        pReq->phCredential
        );

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_IMPORT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerImportSecurityContext(
        pReq->pszPackage,
        pReq->pPackedContext,
        pReq->pToken,
        pReq->phContext
        );

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_INIT_SEC_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    DWORD nContextAttr = 0;
    TimeStamp tsExpiry;

    memset(&tsExpiry, 0, sizeof(TimeStamp));

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerInitializeSecurityContext(
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

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_MAKE_SIGN_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerMakeSignature(
        pReq->phContext,
        pReq->bEncrypt,
        pReq->pMessage,
        pReq->MessageSeqNo
        );

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_QUERY_CREDS_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    PVOID pBuffer = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerQueryCredentialsAttributes(
        pReq->phCredential,
        pReq->ulAttribute,
        pBuffer
        );

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_QUERY_CTXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    PVOID pBuffer = NULL;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerQueryContextAttributes(
        pReq->phContext,
        pReq->ulAttribute,
        pBuffer
        );

    if(!pError->dwError)
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
    PVOID data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_VERIFY_SIGN_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    BOOL bEncrypted = 0;

    dwError = NTLM_MAP_LWMSG_ERROR(
        lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle)
        );
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmSrvIpcCreateError(dwError, &pError);
    BAIL_ON_NTLM_ERROR(dwError);

    pError->dwError = NtlmServerVerifySignature(
        pReq->phContext,
        pReq->pMessage,
        pReq->MessageSeqNo,
        &bEncrypted
        );

    if(!pError->dwError)
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
