#include "includes.h"

static
VOID
IOMgrFreeSMBHandleObject(
    PVOID pHandle
    );

LWMsgStatus
SMBSrvIpcRefreshConfiguration(
    LWMsgAssoc*         pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PCSTR pszConfigPath = SMB_CONFIG_FILE_PATH;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvRefreshConfig(pszConfigPath);

    /* Transmit refresh error to client but do not fail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_REFRESH_CONFIG_FAILED;
        pResponse->object = (PVOID) pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_REFRESH_CONFIG_SUCCESS;
    pResponse->object = (PVOID) pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcSetLogInfo(
    LWMsgAssoc*         pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage*       pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(pRequest->object);

    dwError = SMBLogSetInfo_r((PSMB_LOG_INFO)pRequest->object);

    /* Transmit failure to client but do not bail out of dispatch loop */
    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_SET_LOG_LEVEL_FAILED;
        pResponse->object = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_SET_LOG_LEVEL_SUCCESS;
    pResponse->object = pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcGetLogInfo(
    LWMsgAssoc*       pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage* pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_LOG_INFO pLogInfo = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBLogGetInfo_r(&pLogInfo);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_GET_LOG_INFO_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_GET_LOG_INFO_SUCCESS;
    pResponse->object = pLogInfo;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcCallNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_CALL_NP_REQUEST pNPRequest = NULL;
    PSMB_CALL_NP_RESPONSE pNPResponse = NULL;
    PBYTE pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_CALL_NP_REQUEST)pRequest->object;

    BAIL_ON_INVALID_POINTER(pNPRequest);

    dwError = IOMgrCallNamedPipe(
                    pNPRequest->pSecurityToken,
                    pNPRequest->pwszNamedPipeName,
                    pNPRequest->pInBuffer,
                    pNPRequest->dwInBufferSize,
                    pNPRequest->dwOutBufferSize,
                    pNPRequest->dwTimeout,
                    (PVOID*)&pOutBuffer,
                    &dwOutBufferSize);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_CALL_NAMED_PIPE_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    dwError = SMBAllocateMemory(
                    sizeof(SMB_CALL_NP_RESPONSE),
                    (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    if (dwOutBufferSize)
    {
        pNPResponse->pOutBuffer = pOutBuffer;
        pOutBuffer = NULL;

        pNPResponse->dwOutBufferSize = dwOutBufferSize;
    }

    pResponse->tag = SMB_CALL_NAMED_PIPE_SUCCESS;
    pResponse->object = pNPResponse;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);
    SMB_SAFE_FREE_MEMORY(pOutBuffer);

    return status;

error:

    SMB_SAFE_FREE_MEMORY(pNPResponse);

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcCreateNamedPipe(
    LWMsgAssoc*         pAssoc,
    const LWMsgMessage* pRequest,
    LWMsgMessage*       pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_CREATE_NP_REQUEST pNPRequest = NULL;
    PSMB_FILE_HANDLE pFileHandle = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_CREATE_NP_REQUEST)pRequest->object;

    BAIL_ON_INVALID_POINTER(pNPRequest);

    dwError = IOMgrCreateNamedPipe(
                    pNPRequest->pSecurityToken,
                    pNPRequest->pwszName,
                    pNPRequest->dwOpenMode,
                    pNPRequest->dwPipeMode,
                    pNPRequest->dwMaxInstances,
                    pNPRequest->dwOutBufferSize,
                    pNPRequest->dwInBufferSize,
                    pNPRequest->dwDefaultTimeOut,
                    pNPRequest->pSecurityAttributes,
                    &pFileHandle);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_CALL_NAMED_PIPE_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_register_handle(
                                    pAssoc,
                                    "SMB_FILE_HANDLE",
                                    pFileHandle,
                                    &IOMgrFreeSMBHandleObject));
    BAIL_ON_SMB_ERROR(dwError);

    pResponse->tag = SMB_CALL_NAMED_PIPE_SUCCESS;
    pResponse->object = pFileHandle;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    if (pFileHandle)
    {
        IOMgrCloseFile(pFileHandle);
    }

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcGetNamedPipeInfo(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_FILE_HANDLE hNamedPipe = NULL;
    PSMB_NP_INFO pNPResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    hNamedPipe = (PSMB_FILE_HANDLE)pRequest->object;

    BAIL_ON_INVALID_POINTER(hNamedPipe);

    dwError = SMBAllocateMemory(
                    sizeof(SMB_NP_INFO),
                    (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = IOMgrGetNamedPipeInfo(
                    hNamedPipe,
                    &pNPResponse->dwFlags,
                    &pNPResponse->dwInBufferSize,
                    &pNPResponse->dwOutBufferSize,
                    &pNPResponse->dwMaxInstances);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_GET_NAMED_PIPE_INFO_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_GET_NAMED_PIPE_INFO_SUCCESS;
    pResponse->object = pNPResponse;
    pNPResponse = NULL;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);

    if (pNPResponse)
    {
        SMBFreeMemory(pNPResponse);
    }

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcConnectNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_FILE_HANDLE hNamedPipe = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    hNamedPipe = (PSMB_FILE_HANDLE)pRequest->object;

    dwError = IOMgrConnectNamedPipe(hNamedPipe);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_CONNECT_NAMED_PIPE_FAILED;
        pResponse->object = (PVOID) pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_CONNECT_NAMED_PIPE_SUCCESS;
    pResponse->object = (PVOID) pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcTransactNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_TRANSACT_NP_REQUEST pNPRequest = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_CALL_NP_RESPONSE pNPResponse = NULL;
    PVOID pOutBuffer = NULL;
    DWORD dwOutBufferSize = 0;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_TRANSACT_NP_REQUEST)pRequest->object;

    dwError = IOMgrTransactNamedPipe(
                    pNPRequest->hNamedPipe,
                    pNPRequest->pInBuffer,
                    pNPRequest->dwInBufferSize,
                    pNPRequest->dwOutBufferSize,
                    &pOutBuffer,
                    &dwOutBufferSize);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_TRANSACT_NAMED_PIPE_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    dwError = SMBAllocateMemory(
                        sizeof(SMB_CALL_NP_RESPONSE),
                        (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    if (dwOutBufferSize)
    {
        pNPResponse->pOutBuffer = pOutBuffer;
        pOutBuffer = NULL;

        pNPResponse->dwOutBufferSize = dwOutBufferSize;
    }

    pResponse->tag = SMB_TRANSACT_NAMED_PIPE_SUCCESS;
    pResponse->object = pNPResponse;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);
    SMB_SAFE_FREE_MEMORY(pOutBuffer);

    return status;

error:

    SMB_SAFE_FREE_MEMORY(pNPResponse);

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcWaitNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_WAIT_NP_REQUEST pNPRequest = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_WAIT_NP_REQUEST)pRequest->object;

    dwError = IOMgrWaitNamedPipe(
        pNPRequest->pSecurityToken,
        pNPRequest->pwszName,
        pNPRequest->dwTimeout);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_WAIT_NAMED_PIPE_FAILED;
        pResponse->object = (PVOID) pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_WAIT_NAMED_PIPE_SUCCESS;
    pResponse->object = (PVOID) pStatusResponse;

cleanup:

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcGetClientComputerName(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_GET_CLIENT_COMPUTER_NAME_REQUEST pNPRequest = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_GET_CLIENT_COMPUTER_NAME_RESPONSE pNPResponse = NULL;
    LPWSTR pwszName = NULL;
    DWORD    dwLength = 0;
    DWORD    dwAlignBytes = 0;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_GET_CLIENT_COMPUTER_NAME_REQUEST)pRequest->object;

    dwError = IOMgrGetClientComputerName(
                    pNPRequest->hNamedPipe,
                    pNPRequest->dwComputerNameMaxSize,
                    &pwszName,
                    &dwLength);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_GET_CLIENT_COMPUTER_NAME_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    if (dwLength)
    {
        dwAlignBytes = (((unsigned long)((PBYTE)pNPResponse + sizeof(SMB_GET_CLIENT_COMPUTER_NAME_RESPONSE))) % sizeof(unsigned long)) * sizeof(unsigned long);
    }

    dwError = SMBAllocateMemory(
                        sizeof(SMB_GET_CLIENT_COMPUTER_NAME_RESPONSE) + ((dwLength+1) * sizeof(wchar16_t)) + dwAlignBytes,
                        (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    if (dwLength)
    {
        pNPResponse->pwszName = (LPWSTR)((PBYTE)pNPResponse + sizeof(SMB_GET_CLIENT_COMPUTER_NAME_RESPONSE) + dwAlignBytes);
        memcpy(pNPResponse->pwszName, pwszName, dwLength * sizeof(wchar16_t));
        pNPResponse->dwLength = dwLength;
    }

    pResponse->tag = SMB_GET_CLIENT_COMPUTER_NAME_SUCCESS;
    pResponse->object = pNPResponse;
    pNPResponse = NULL;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);
    SMB_SAFE_FREE_MEMORY(pwszName);

    return status;

error:

    SMB_SAFE_FREE_MEMORY(pNPResponse);

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcGetClientProcessId(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_FILE_HANDLE hNamedPipe = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_ID_REPLY pNPResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    hNamedPipe = (PSMB_FILE_HANDLE)pRequest->object;

    dwError = SMBAllocateMemory(
                        sizeof(SMB_ID_REPLY),
                        (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = IOMgrGetClientProcessId(
                    hNamedPipe,
                    &pNPResponse->dwId);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_GET_CLIENT_PROCESS_ID_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_GET_CLIENT_PROCESS_ID_SUCCESS;
    pResponse->object = pNPResponse;
    pNPResponse = NULL;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);
    SMB_SAFE_FREE_MEMORY(pNPResponse);

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcGetServerProcessId(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_FILE_HANDLE hNamedPipe = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_ID_REPLY pNPResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    hNamedPipe = (PSMB_FILE_HANDLE)pRequest->object;

    dwError = SMBAllocateMemory(
                        sizeof(SMB_ID_REPLY),
                        (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = IOMgrGetServerProcessId(
                    hNamedPipe,
                    &pNPResponse->dwId);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_GET_SERVER_PROCESS_ID_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_GET_SERVER_PROCESS_ID_SUCCESS;
    pResponse->object = pNPResponse;
    pNPResponse = NULL;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);
    SMB_SAFE_FREE_MEMORY(pNPResponse);

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcGetClientSessionId(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_FILE_HANDLE hNamedPipe = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_ID_REPLY pNPResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    hNamedPipe = (PSMB_FILE_HANDLE)pRequest->object;

    dwError = SMBAllocateMemory(
                        sizeof(SMB_ID_REPLY),
                        (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = IOMgrGetClientSessionId(
                    hNamedPipe,
                    &pNPResponse->dwId);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_GET_CLIENT_SESSION_ID_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_GET_CLIENT_SESSION_ID_SUCCESS;
    pResponse->object = pNPResponse;
    pNPResponse = NULL;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);
    SMB_SAFE_FREE_MEMORY(pNPResponse);

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcPeekNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_PEEK_NP_REQUEST pNPRequest = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_PEEK_NP_RESPONSE pNPResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_PEEK_NP_REQUEST)pRequest->object;

    dwError = SMBAllocateMemory(
                        sizeof(SMB_PEEK_NP_RESPONSE),
                        (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = IOMgrPeekNamedPipe(
                    pNPRequest->hNamedPipe,
                    pNPRequest->pInBuffer,
                    pNPRequest->dwInBufferSize,
                    &pNPResponse->dwBytesRead,
                    &pNPResponse->dwTotalBytesAvail,
                    &pNPResponse->dwBytesLeftThisMessage);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_PEEK_NAMED_PIPE_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_PEEK_NAMED_PIPE_SUCCESS;
    pResponse->object = pNPResponse;
    pNPResponse = NULL;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);
    SMB_SAFE_FREE_MEMORY(pNPResponse);

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcDisconnectNamedPipe(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_FILE_HANDLE hNamedPipe = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    hNamedPipe = (PSMB_FILE_HANDLE)pRequest->object;

    dwError = IOMgrDisconnectNamedPipe(hNamedPipe);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_DISCONNECT_NAMED_PIPE_FAILED;
        pResponse->object = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_DISCONNECT_NAMED_PIPE_SUCCESS;
    pResponse->object = pStatusResponse;

cleanup:

    return status;

error:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcCreateFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_CREATE_FILE_REQUEST pNPRequest = NULL;
    PSMB_FILE_HANDLE pFileHandle = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_CREATE_FILE_REQUEST)pRequest->object;

    BAIL_ON_INVALID_POINTER(pNPRequest);

    dwError = IOMgrCreateFile(
        pNPRequest->pSecurityToken,
        pNPRequest->pwszFileName,
        pNPRequest->dwDesiredAccess,
        pNPRequest->dwSharedMode,
        pNPRequest->dwCreationDisposition,
        pNPRequest->dwFlagsAndAttributes,
        pNPRequest->pSecurityAttributes,
        &pFileHandle);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_CREATE_FILE_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_CREATE_FILE_SUCCESS;
    pResponse->object = pFileHandle;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    if (pFileHandle)
    {
        IOMgrCloseFile(pFileHandle);
    }

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcSetNamedPipeHandleState(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_SETNAMEDPIPEHANDLESTATE_REQUEST pNPRequest = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_SETNAMEDPIPEHANDLESTATE_REQUEST)pRequest->object;

    BAIL_ON_INVALID_POINTER(pNPRequest);

    dwError = IOMgrSetNamedPipeHandleState(
                    pNPRequest->hPipe,
                    pNPRequest->pdwMode,
                    pNPRequest->pdwCollectionCount,
                    pNPRequest->pdwTimeout);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_SET_NAMED_PIPE_HANDLE_STATE_FAILED;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_SET_NAMED_PIPE_HANDLE_STATE_SUCCESS;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcReadFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_READ_FILE_REQUEST pNPRequest = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_READ_FILE_RESPONSE pNPResponse = NULL;
    PVOID pOutBuffer = NULL;
    DWORD dwBytesRead = 0;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_READ_FILE_REQUEST)pRequest->object;

    dwError = IOMgrReadFile(
                    pNPRequest->hFile,
                    pNPRequest->dwBytesToRead,
                    &pOutBuffer,
                    &dwBytesRead);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_READ_FILE_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    dwError = SMBAllocateMemory(
                        sizeof(SMB_READ_FILE_RESPONSE),
                        (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    if (dwBytesRead)
    {
        pNPResponse->pBuffer = pOutBuffer;
        pOutBuffer = NULL;

        pNPResponse->dwBytesRead = dwBytesRead;
    }

    pResponse->tag = SMB_READ_FILE_SUCCESS;
    pResponse->object = pNPResponse;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);
    SMB_SAFE_FREE_MEMORY(pOutBuffer);

    return status;

error:

    SMB_SAFE_FREE_MEMORY(pNPResponse);

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcWriteFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_WRITE_FILE_REQUEST pNPRequest = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_WRITE_FILE_RESPONSE pNPResponse = NULL;
    DWORD dwBytesWritten = 0;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPRequest = (PSMB_WRITE_FILE_REQUEST)pRequest->object;

    dwError = IOMgrWriteFile(
                    pNPRequest->hFile,
                    pNPRequest->dwBytesToWrite,
                    pNPRequest->pBuffer,
                    &dwBytesWritten);

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_WRITE_FILE_FAILED;
        pResponse->object = pStatusResponse;
        pStatusResponse = NULL;

        dwError = 0;
        goto cleanup;
    }

    dwError = SMBAllocateMemory(
                        sizeof(SMB_WRITE_FILE_RESPONSE),
                        (PVOID*)&pNPResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pNPResponse->dwBytesWritten = dwBytesWritten;

    pResponse->tag = SMB_WRITE_FILE_SUCCESS;
    pResponse->object = pNPResponse;

cleanup:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);

    return status;

error:

    SMB_SAFE_FREE_MEMORY(pNPResponse);

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcCloseFile(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_FILE_HANDLE pFileHandle = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;

    dwError = SMBAllocateMemory(
                    sizeof(SMB_STATUS_REPLY),
                    (PVOID*)&pStatusResponse);
    BAIL_ON_SMB_ERROR(dwError);

    pFileHandle = (PSMB_FILE_HANDLE)pRequest->object;

    dwError = MAP_LWMSG_STATUS(lwmsg_assoc_unregister_handle(
                                    pAssoc,
                                    pFileHandle,
                                    LWMSG_FALSE));
    BAIL_ON_SMB_ERROR(dwError);

    dwError = IOMgrCloseFile(pFileHandle);
    // IOMgrCloseFile frees pFileHandle, even on error, so the handle needs
    // to be set to null so that it will not get double freed in cleanup.
    pFileHandle = NULL;

    if (dwError)
    {
        pStatusResponse->dwError = dwError;
        pResponse->tag = SMB_CLOSE_FILE_FAILED;
        pResponse->object = pStatusResponse;

        dwError = 0;
        goto cleanup;
    }

    pResponse->tag = SMB_CLOSE_FILE_SUCCESS;
    pResponse->object = pStatusResponse;

cleanup:

    SMB_SAFE_FREE_MEMORY(pFileHandle);

    return status;

error:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);

    goto cleanup;
}

LWMsgStatus
SMBSrvIpcGetSessionKey(
    LWMsgAssoc*             pAssoc,
    const LWMsgMessage*     pRequest,
    LWMsgMessage*           pResponse,
    void* pData
    )
{
    DWORD dwError = 0;
    DWORD dwCallError = 0;
    LWMsgStatus status = LWMSG_STATUS_SUCCESS;
    PSMB_FILE_HANDLE hFile = NULL;
    PSMB_STATUS_REPLY pStatusResponse = NULL;
    PSMB_GET_SESSION_KEY_RESPONSE pKeyResponse = NULL;

    dwError = SMBAllocateMemory(
        sizeof(*pKeyResponse),
        (PVOID*) (PVOID) &pKeyResponse);
    BAIL_ON_SMB_ERROR(dwError);

    hFile = (PSMB_FILE_HANDLE)pRequest->object;

    dwError = IOMgrGetSessionKey(
        hFile,
        &pKeyResponse->dwSessionKeyLength,
        &pKeyResponse->pSessionKey
        );

    if (dwError)
    {
        dwCallError = dwError;

        dwError = SMBAllocateMemory(
            sizeof(*pStatusResponse),
            (PVOID*) (PVOID) &pStatusResponse);
        BAIL_ON_SMB_ERROR(dwError);

        pStatusResponse->dwError = dwCallError;
        pResponse->tag = SMB_GET_SESSION_KEY_FAILED;
        pResponse->object = pStatusResponse;
    }
    else
    {
        pResponse->tag = SMB_GET_SESSION_KEY_SUCCESS;
        pResponse->object = pKeyResponse;
    }

cleanup:

    return status;

error:

    SMB_SAFE_FREE_MEMORY(pStatusResponse);
    SMB_SAFE_FREE_MEMORY(pKeyResponse);

    goto cleanup;
}

static
VOID
IOMgrFreeSMBHandleObject(
    PVOID pHandle
    )
{
    if (pHandle)
    {
        IOMgrCloseFile((PSMB_FILE_HANDLE)pHandle);
    }
}
