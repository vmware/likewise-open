#include <ntlmipc.h>

DWORD
NtlmAllocateMemory(
    DWORD dwSize,
    PVOID *ppMemory
    )
{
    DWORD dwError = 0;
    PVOID pMemory = NULL;

    pMemory = malloc(dwSize);
    if (!pMemory)
    {
        dwError = ENOMEM;
        *ppMemory = NULL;
    }
    else
    {
        memset(pMemory,0, dwSize);
        *ppMemory = pMemory;
    }

    return dwError;
}

DWORD
NtlmSrvIpcCreateError(
    DWORD dwErrorCode,
    PNTLM_IPC_ERROR* ppError
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ERROR pError = NULL;

    dwError = NtlmAllocateMemory(sizeof(*pError), (void**) (void*) &pError);
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

    ULONG fContextAttr = 0;
    TimeStamp tsTimeStamp;

    memset(&tsTimeStamp, 0, sizeof(TimeStamp));

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerAcceptSecurityContext(
        (HANDLE)Handle,
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

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerAcquireCredentialsHandle(
        (HANDLE)Handle,
        pReq->pszPrincipal,
        pReq->pszPackage,
        pReq->fCredentialUse,
        pReq->pvLogonID,
        pReq->pAuthData,
        pReq->pGetKeyFn,
        pReq->pvGetKeyArgument,
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

    ULONG nQoP = 0;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerDecryptMessage(
        (HANDLE)Handle,
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

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerEncryptMessage(
        pReq->phContext,
        pReq->fQoP,
        pReq->pMessage,
        pReq->MessageSeqNo
        );

    dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
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

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerExportSecurityContext(
        (HANDLE)Handle,
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
NtlmSrvIpcFreeCredentials(
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

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerFreeCredentials(
        (HANDLE)Handle,
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

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerImportSecurityContext(
        (HANDLE)Handle,
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

    ULONG nContextAttr = 0;
    TimeStamp tsExpiry;

    memset(&tsExpiry, 0, sizeof(TimeStamp));

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerInitializeSecurityContext(
        (HANDLE)Handle,
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

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerMakeSignature(
        (HANDLE)Handle,
        pReq->phContext,
        pReq->fQoP,
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

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerQueryCredentialsAttributes(
        (HANDLE)Handle,
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

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerQueryContextAttributes(
        (HANDLE)Handle,
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

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerVerifySignature(
        (HANDLE)Handle,
        pReq->phContext,
        pReq->pMessage,
        pReq->MessageSeqNo
        );

    dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
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
