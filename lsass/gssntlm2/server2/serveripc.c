

LWMsgStatus
NtlmSrvIpcAcceptSecurityContext(
    LWMsgAssoc* assoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* data
    )
{
    DWORD dwError = 0;
    PNTLM_IPC_ACCEPT_SECURITY_CONTEXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

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
                        pReq->pfContextAttr,
                        pReq->ptsTimeStamp
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACCEPT_SECURITY_CONTEXT_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACCEPT_SECURITY_CONTEXT_FAILURE;
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
    PNTLM_IPC_ACQUIRE_CREDENTIALS_HANDLE_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerAcquireCredentialsHandle(
                        (HANDLE)Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);
                        pReq->pszPrincipal,
                        pReq->pszPackage,
                        pReq->fCredentialUse,
                        pReq->pvLogonId,
                        pReq->pGetKeyFn,
                        pReq->pvGetKeyArgument,
                        pReq->phCredential,
                        pReq->ptsExpiry
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_DECRYPT_MESSAGE_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerDecryptMessage(
                        (HANDLE)Handle,
                        pReq->phContext,
                        pReq->pMessage,
                        pReq->MessageSeqNo,
                        pReq->pfQoP
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_ENCRYPT_MESSAGE_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerEncryptMessage(
                        (HANDLE)Handle,
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_EXPORT_SECURITY_CONTEXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerExportSecurityContext(
                        (HANDLE)Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);
                        pReq->pszPrincipal,
                        pReq->pszPackage,
                        pReq->fCredentialUse,
                        pReq->pvLogonId,
                        pReq->pGetKeyFn,
                        pReq->pvGetKeyArgument,
                        pReq->phCredential,
                        pReq->ptsExpiry
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_FREE_CREDENTIALS_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerFreeCredentials(
                        (HANDLE)Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);
                        pReq->pszPrincipal,
                        pReq->pszPackage,
                        pReq->fCredentialUse,
                        pReq->pvLogonId,
                        pReq->pGetKeyFn,
                        pReq->pvGetKeyArgument,
                        pReq->phCredential,
                        pReq->ptsExpiry
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_IMPORT_SECURITY_CONTEXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerImportSecurityContext(
                        (HANDLE)Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);
                        pReq->pszPrincipal,
                        pReq->pszPackage,
                        pReq->fCredentialUse,
                        pReq->pvLogonId,
                        pReq->pGetKeyFn,
                        pReq->pvGetKeyArgument,
                        pReq->phCredential,
                        pReq->ptsExpiry
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_INITIALIZE_SECURITY_CONTEXT_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerInitializeSecurityContext(
                        (HANDLE)Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);
                        pReq->pszPrincipal,
                        pReq->pszPackage,
                        pReq->fCredentialUse,
                        pReq->pvLogonId,
                        pReq->pGetKeyFn,
                        pReq->pvGetKeyArgument,
                        pReq->phCredential,
                        pReq->ptsExpiry
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_MAKE_SIGNATURE_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerMakeSignature(
                        (HANDLE)Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);
                        pReq->pszPrincipal,
                        pReq->pszPackage,
                        pReq->fCredentialUse,
                        pReq->pvLogonId,
                        pReq->pGetKeyFn,
                        pReq->pvGetKeyArgument,
                        pReq->phCredential,
                        pReq->ptsExpiry
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_QUERY_CREDENTIALS_ATTRIBUTES_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerQueryCredentialsAttributes(
                        (HANDLE)Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);
                        pReq->pszPrincipal,
                        pReq->pszPackage,
                        pReq->fCredentialUse,
                        pReq->pvLogonId,
                        pReq->pGetKeyFn,
                        pReq->pvGetKeyArgument,
                        pReq->phCredential,
                        pReq->ptsExpiry
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_QUERY_CONTEXT_ATTRIBUTES_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerQueryContextAttributes(
                        (HANDLE)Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);
                        pReq->pszPrincipal,
                        pReq->pszPackage,
                        pReq->fCredentialUse,
                        pReq->pvLogonId,
                        pReq->pGetKeyFn,
                        pReq->pvGetKeyArgument,
                        pReq->phCredential,
                        pReq->ptsExpiry
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
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
    PNTLM_IPC_VERIFY_SIGNATURE_REQ pReq = pRequest->object;
    PNTLM_IPC_ERROR pError = NULL;
    PVOID Handle = NULL;

    dwError = MAP_LWMSG_ERROR(lwmsg_assoc_get_session_data(assoc, (PVOID*) (PVOID) &Handle));
    BAIL_ON_NTLM_ERROR(dwError);

    dwError = NtlmServerVerifySignature(
                        (HANDLE)Handle,
                        pReq->pszLoginName,
                        pReq->pszPassword);
                        pReq->pszPrincipal,
                        pReq->pszPackage,
                        pReq->fCredentialUse,
                        pReq->pvLogonId,
                        pReq->pGetKeyFn,
                        pReq->pvGetKeyArgument,
                        pReq->phCredential,
                        pReq->ptsExpiry
                        );
    if (!dwError)
    {
        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = NULL;
    }
    else
    {
        dwError = NtlmSrvIpcCreateError(dwError, NULL, &pError);
        BAIL_ON_NTLM_ERROR(dwError);

        pResponse->tag = NTLM_R_ACQUIRE_CREDENTIALS_HANDLE_SUCCESS;
        pResponse->object = pError;
    }

cleanup:
    return MAP_NTLM_ERROR_IPC(dwError);

error:
    goto cleanup;
}
