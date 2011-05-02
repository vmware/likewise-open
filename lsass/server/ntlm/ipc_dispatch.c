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
 *        ipc_dispatch.c
 *
 * Abstract:
 *
 *        NTLM Authentication
 *
 *        Server IPC dispatch table
 *
 * Authors: Marc Guy (mguy@likewise.com)
 *
 */

#include "ntlmsrvapi.h"

static
DWORD
NtlmSrvIpcRegisterHandle(
    LWMsgCall* pCall,
    PCSTR pszHandleType,
    PVOID pHandle,
    LWMsgHandleCleanupFunction pfnCleanup
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_register_handle(pSession, pszHandleType, pHandle, pfnCleanup));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
DWORD
NtlmSrvIpcRetainHandle(
    LWMsgCall* pCall,
    PVOID pHandle
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_retain_handle(pSession, pHandle));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

#if 0
static
DWORD
NtlmSrvIpcReleaseHandle(
    LWMsgCall* pCall,
    PVOID pHandle
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_release_handle(pSession, pHandle));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}
#endif

static
DWORD
NtlmSrvIpcUnregisterHandle(
    LWMsgCall* pCall,
    PVOID pHandle
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    dwError = MAP_LWMSG_ERROR(lwmsg_session_unregister_handle(pSession, pHandle));
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}

static
HANDLE
NtlmSrvIpcGetSessionData(
    LWMsgCall* pCall
    )
{
    LWMsgSession* pSession = lwmsg_call_get_session(pCall);

    return lwmsg_session_get_data(pSession);
}

static
VOID
NtlmSrvCleanupCredHandle(
    PVOID pData
    )
{
    NtlmReleaseCredential((PNTLM_CRED_HANDLE)&pData);
}

static
VOID
NtlmSrvCleanupContextHandle(
    PVOID pData
    )
{
    NtlmReleaseContext((PNTLM_CONTEXT_HANDLE)&pData);
}

static
DWORD
NtlmSrvIpcCreateError(
    DWORD dwErrorCode,
    PNTLM_IPC_ERROR* ppError
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(sizeof(*pError), OUT_PPVOID(&pError));
    BAIL_ON_LSA_ERROR(dwError);

    pError->dwError = dwErrorCode;

    *ppError = pError;

error:
    return dwError;
}

LWMsgStatus
NtlmSrvIpcAcceptSecurityContext(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_ACCEPT_SEC_CTXT_REQ pReq = pIn->data;
    PNTLM_IPC_ACCEPT_SEC_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    NTLM_CONTEXT_HANDLE hNewContext = NULL;

    dwError = LwAllocateMemory(sizeof(*pNtlmResp), OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerAcceptSecurityContext(
        NtlmSrvIpcGetSessionData(pCall),
        pReq->hCredential,
        &pReq->hContext,
        pReq->pInput,
        pReq->fContextReq,
        pReq->TargetDataRep,
        &hNewContext,
        &pNtlmResp->Output,
        &pNtlmResp->fContextAttr,
        &pNtlmResp->tsTimeStamp);

    if (dwError == LW_ERROR_SUCCESS || dwError == LW_WARNING_CONTINUE_NEEDED)
    {
        if (LW_ERROR_SUCCESS == dwError)
        {
            // We're going to return an updated context to the user and since
            // they only clean up the most recent one received, we need to clean
            // up the old one they were nice enough to pass in.
            dwError = NtlmSrvIpcUnregisterHandle(pCall, pReq->hContext);
            BAIL_ON_LSA_ERROR(dwError);
        }

        pNtlmResp->dwStatus = dwError;
        dwError = LW_ERROR_SUCCESS;

        pNtlmResp->hNewContext = hNewContext;
        hNewContext = NULL;

        dwError = NtlmSrvIpcRegisterHandle(
            pCall,
            "NTLM_CONTEXT_HANDLE",
            pNtlmResp->hNewContext,
            NtlmSrvCleanupContextHandle);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_ACCEPT_SEC_CTXT_SUCCESS;
        pOut->data = pNtlmResp;

        dwError = NtlmSrvIpcRetainHandle(pCall, pNtlmResp->hNewContext);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pNtlmResp->Output.pvBuffer);
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcAcquireCredentialsHandle(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_ACQUIRE_CREDS_REQ pReq = pIn->data;
    PNTLM_IPC_ACQUIRE_CREDS_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_IPC_ACQUIRE_CREDS_RESPONSE),
        OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerAcquireCredentialsHandle(
        pCall,
        pReq->pszPrincipal,
        pReq->pszPackage,
        pReq->fCredentialUse,
        pReq->pvLogonID,
        pReq->pAuthData,
        &pNtlmResp->hCredential,
        &pNtlmResp->tsExpiry);

    if (!dwError)
    {
        dwError = NtlmSrvIpcRegisterHandle(
            pCall,
            "NTLM_CRED_HANDLE",
            pNtlmResp->hCredential,
            NtlmSrvCleanupCredHandle);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_ACQUIRE_CREDS_SUCCESS;
        pOut->data = pNtlmResp;

        dwError = NtlmSrvIpcRetainHandle(pCall, pNtlmResp->hCredential);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

DWORD
NtlmServerDuplicateBuffers(
    IN const SecBufferDesc* pIn,
    OUT PSecBufferDesc pOut
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    DWORD nIndex = 0;

    pOut->cBuffers = pIn->cBuffers;
    dwError = LwAllocateMemory(
        sizeof(pIn->pBuffers[0]) * pIn->cBuffers,
        OUT_PPVOID(&pOut->pBuffers));
    BAIL_ON_LSA_ERROR(dwError);

    for (nIndex= 0; nIndex < pIn->cBuffers; nIndex++)
    {
        pOut->pBuffers[nIndex].cbBuffer = pIn->pBuffers[nIndex].cbBuffer;
        dwError = LwAllocateMemory(
            pOut->pBuffers[nIndex].cbBuffer,
            OUT_PPVOID(&pOut->pBuffers[nIndex].pvBuffer));
        BAIL_ON_LSA_ERROR(dwError);

        memcpy(pOut->pBuffers[nIndex].pvBuffer,
                pIn->pBuffers[nIndex].pvBuffer,
                pIn->pBuffers[nIndex].cbBuffer);
        pOut->pBuffers[nIndex].BufferType = pIn->pBuffers[nIndex].BufferType;
    }

cleanup:
    return dwError;
error:
    NtlmServerFreeBuffers(pOut);
    goto cleanup;
}

void
NtlmServerFreeBuffers(
    IN PSecBufferDesc pBuffer
    )
{
    DWORD nIndex = 0;

    if (pBuffer != NULL && pBuffer->pBuffers != NULL)
    {
        for (nIndex = 0; nIndex < pBuffer->cBuffers; nIndex++)
        {
            LW_SAFE_FREE_MEMORY(pBuffer->pBuffers[nIndex].pvBuffer);
        }
        LW_SAFE_FREE_MEMORY(pBuffer->pBuffers);
    }
}

LWMsgStatus
NtlmSrvIpcDecryptMessage(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    const PNTLM_IPC_DECRYPT_MSG_REQ pReq = pIn->data;
    PNTLM_IPC_DECRYPT_MSG_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_IPC_DECRYPT_MSG_RESPONSE),
        OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerDuplicateBuffers(
                pReq->pMessage,
                &pNtlmResp->Message);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerDecryptMessage(
        pReq->hContext,
        &pNtlmResp->Message,
        pReq->MessageSeqNo,
        &pNtlmResp->bEncrypted);

    if (!dwError)
    {
        pOut->tag = NTLM_R_DECRYPT_MSG_SUCCESS;
        pOut->data = pNtlmResp;
    }
    else
    {
        NtlmServerFreeBuffers(&pNtlmResp->Message);
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcDeleteSecurityContext(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_ERROR pError = NULL;
    PNTLM_IPC_DELETE_SEC_CTXT_REQ pReq = pIn->data;

    dwError = NtlmSrvIpcUnregisterHandle(pCall, pReq->hContext);
    if (!dwError)
    {
        pOut->tag = NTLM_R_DELETE_SEC_CTXT_SUCCESS;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcEncryptMessage(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_ENCRYPT_MSG_REQ pReq = pIn->data;
    PNTLM_IPC_ENCRYPT_MSG_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_IPC_ENCRYPT_MSG_RESPONSE),
        OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerDuplicateBuffers(
                pReq->pMessage,
                &pNtlmResp->Message);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerEncryptMessage(
        &pReq->hContext,
        pReq->bEncrypt,
        &pNtlmResp->Message,
        pReq->MessageSeqNo);

    if (!dwError)
    {
        pOut->tag = NTLM_R_ENCRYPT_MSG_SUCCESS;
        pOut->data = pNtlmResp;
    }
    else
    {
        NtlmServerFreeBuffers(&pNtlmResp->Message);
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcExportSecurityContext(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_EXPORT_SEC_CTXT_REQ pReq = pIn->data;
    PNTLM_IPC_EXPORT_SEC_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_IPC_EXPORT_SEC_CTXT_RESPONSE),
        OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerExportSecurityContext(
        &pReq->hContext,
        pReq->fFlags,
        &(pNtlmResp->PackedContext),
        &(pNtlmResp->hToken));

    if (!dwError)
    {
        pOut->tag = NTLM_R_EXPORT_SEC_CTXT_SUCCESS;
        pOut->data = pNtlmResp;
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcFreeCredentialsHandle(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_ERROR pError = NULL;
    PNTLM_IPC_FREE_CREDS_REQ pReq = pIn->data;

    dwError = NtlmSrvIpcUnregisterHandle(pCall, pReq->hCredential);
    if (!dwError)
    {
        pOut->tag = NTLM_R_FREE_CREDS_SUCCESS;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcImportSecurityContext(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_IMPORT_SEC_CTXT_REQ pReq = pIn->data;
    PNTLM_IPC_IMPORT_SEC_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_IPC_IMPORT_SEC_CTXT_RESPONSE),
        OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerImportSecurityContext(
        pReq->pszPackage,
        pReq->pPackedContext,
        pReq->pToken,
        &(pNtlmResp->hContext));

    if (!dwError)
    {
        pOut->tag = NTLM_R_IMPORT_SEC_CTXT_SUCCESS;
        pOut->data = pNtlmResp;
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcInitializeSecurityContext(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    const NTLM_IPC_INIT_SEC_CTXT_REQ* pReq = pIn->data;
    PNTLM_IPC_INIT_SEC_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;
    NTLM_CONTEXT_HANDLE hNewContext = NULL;

    dwError = LwAllocateMemory(sizeof(*pNtlmResp), OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerInitializeSecurityContext(
        pReq->hCredential,
        pReq->hContext,
        pReq->pszTargetName,
        pReq->fContextReq,
        pReq->Reserved1,
        pReq->TargetDataRep,
        pReq->pInput,
        pReq->Reserved2,
        &hNewContext,
        &pNtlmResp->Output,
        &pNtlmResp->fContextAttr,
        &pNtlmResp->tsExpiry
        );

    if (dwError == LW_ERROR_SUCCESS || dwError == LW_WARNING_CONTINUE_NEEDED)
    {
        if (LW_ERROR_SUCCESS == dwError)
        {
            // We're going to return an updated context to the user and since
            // they only clean up the most recent one received, we need to clean
            // up the old one they were nice enough to pass in.
            dwError = NtlmSrvIpcUnregisterHandle(pCall, pReq->hContext);
            BAIL_ON_LSA_ERROR(dwError);
        }

        pNtlmResp->dwStatus = dwError;
        dwError = LW_ERROR_SUCCESS;

        pNtlmResp->hNewContext = hNewContext;
        hNewContext = NULL;

        dwError = NtlmSrvIpcRegisterHandle(
            pCall,
            "NTLM_CONTEXT_HANDLE",
            pNtlmResp->hNewContext,
            NtlmSrvCleanupContextHandle);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_INIT_SEC_CTXT_SUCCESS;
        pOut->data = pNtlmResp;

        dwError = NtlmSrvIpcRetainHandle(pCall, pNtlmResp->hNewContext);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pNtlmResp->Output.pvBuffer);
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcMakeSignature(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_MAKE_SIGN_REQ pReq = pIn->data;
    PNTLM_IPC_MAKE_SIGN_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_IPC_MAKE_SIGN_RESPONSE),
        OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerDuplicateBuffers(
                pReq->pMessage,
                &pNtlmResp->Message);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerMakeSignature(
        &pReq->hContext,
        pReq->dwQop,
        &pNtlmResp->Message,
        pReq->MessageSeqNo);

    if (!dwError)
    {
        pOut->tag = NTLM_R_MAKE_SIGN_SUCCESS;
        pOut->data = pNtlmResp;
    }
    else
    {
        NtlmServerFreeBuffers(&pNtlmResp->Message);
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcQueryCredentialsAttributes(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_QUERY_CREDS_REQ pReq = pIn->data;
    PNTLM_IPC_QUERY_CREDS_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_IPC_QUERY_CREDS_RESPONSE),
        OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerQueryCredentialsAttributes(
        &pReq->hCredential,
        pReq->ulAttribute,
        (PVOID)&pNtlmResp->Buffer);

    if (!dwError)
    {
        pNtlmResp->ulAttribute = pReq->ulAttribute;

        pOut->tag = NTLM_R_QUERY_CREDS_SUCCESS;
        pOut->data = pNtlmResp;
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcQueryContextAttributes(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_QUERY_CTXT_REQ pReq = pIn->data;
    PNTLM_IPC_QUERY_CTXT_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_IPC_QUERY_CTXT_RESPONSE),
        OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerQueryContextAttributes(
        &pReq->hContext,
        pReq->ulAttribute,
        (PVOID)&pNtlmResp->Buffer);

    if (!dwError)
    {
        pNtlmResp->ulAttribute = pReq->ulAttribute;

        pOut->tag = NTLM_R_QUERY_CTXT_SUCCESS;
        pOut->data = pNtlmResp;
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

LWMsgStatus
NtlmSrvIpcVerifySignature(
    LWMsgCall* pCall,
    const LWMsgParams* pIn,
    LWMsgParams* pOut,
    PVOID pData
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PNTLM_IPC_VERIFY_SIGN_REQ pReq = pIn->data;
    PNTLM_IPC_VERIFY_SIGN_RESPONSE pNtlmResp = NULL;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = LwAllocateMemory(
        sizeof(NTLM_IPC_VERIFY_SIGN_RESPONSE),
        OUT_PPVOID(&pNtlmResp));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = NtlmServerVerifySignature(
        &pReq->hContext,
        pReq->pMessage,
        pReq->MessageSeqNo,
        &(pNtlmResp->dwQop));

    if (!dwError)
    {
        pOut->tag = NTLM_R_VERIFY_SIGN_SUCCESS;
        pOut->data = pNtlmResp;
    }
    else
    {
        LW_SAFE_FREE_MEMORY(pNtlmResp);

        dwError = NtlmSrvIpcCreateError(dwError, &pError);
        BAIL_ON_LSA_ERROR(dwError);

        pOut->tag = NTLM_R_GENERIC_FAILURE;
        pOut->data = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);
error:
    goto cleanup;
}

static LWMsgDispatchSpec gMessageHandlers[] =
{
    LWMSG_DISPATCH_BLOCK(NTLM_Q_ACCEPT_SEC_CTXT, NtlmSrvIpcAcceptSecurityContext),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_ACQUIRE_CREDS, NtlmSrvIpcAcquireCredentialsHandle),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_DECRYPT_MSG, NtlmSrvIpcDecryptMessage),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_DELETE_SEC_CTXT, NtlmSrvIpcDeleteSecurityContext),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_ENCRYPT_MSG, NtlmSrvIpcEncryptMessage),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_EXPORT_SEC_CTXT, NtlmSrvIpcExportSecurityContext),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_FREE_CREDS, NtlmSrvIpcFreeCredentialsHandle),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_IMPORT_SEC_CTXT, NtlmSrvIpcImportSecurityContext),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_INIT_SEC_CTXT, NtlmSrvIpcInitializeSecurityContext),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_MAKE_SIGN, NtlmSrvIpcMakeSignature),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_QUERY_CREDS, NtlmSrvIpcQueryCredentialsAttributes),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_QUERY_CTXT, NtlmSrvIpcQueryContextAttributes),
    LWMSG_DISPATCH_BLOCK(NTLM_Q_VERIFY_SIGN, NtlmSrvIpcVerifySignature),
    LWMSG_DISPATCH_END
};

LWMsgDispatchSpec*
NtlmSrvGetDispatchSpec(
    VOID
    )
{
    return gMessageHandlers;
}
