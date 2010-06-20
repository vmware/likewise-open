/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

/*
 * Copyright Likewise Software
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
 *        treeconnect.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Tree Connect
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */
#include "includes.h"

static
NTSTATUS
SrvBuildTreeConnectState_SMB_V2(
    PLWIO_SRV_CONNECTION              pConnection,
    PLWIO_SRV_SESSION_2               pSession,
    PSMB2_TREE_CONNECT_REQUEST_HEADER pTreeConnectHeader,
    PUNICODE_STRING                   pPath,
    PSRV_TREE_CONNECT_STATE_SMB_V2*   ppTConState
    );

static
VOID
SrvPrepareTreeConnectStateAsync_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState,
    PSRV_EXEC_CONTEXT              pExecContext
    );

static
VOID
SrvExecuteTreeConnectAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseTreeConnectStateAsync_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState
    );

static
NTSTATUS
SrvCreateTreeRootHandle_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvIoPrepareEcpList_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState, /* IN     */
    PIO_ECP_LIST*                  ppEcpList   /* IN OUT */
    );

static
VOID
SrvIoFreeEcpString_SMB_V2(
    IN PVOID pContext
    );

static
NTSTATUS
SrvBuildTreeConnectResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseTreeConnectStateHandle_SMB_V2(
    HANDLE hTConState
    );

static
VOID
SrvReleaseTreeConnectState_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState
    );

static
VOID
SrvFreeTreeConnectState_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState
    );

NTSTATUS
SrvProcessTreeConnect_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState = NULL;
    PLWIO_SRV_SESSION_2 pSession      = NULL;
    PWSTR               pwszSharename = NULL;
    PSRV_SHARE_INFO     pShareInfo    = NULL;
    BOOLEAN             bInLock       = FALSE;
    BOOLEAN             bShareInfoInLock = FALSE;

    pTConState = (PSRV_TREE_CONNECT_STATE_SMB_V2)pCtxSmb2->hState;
    if (pTConState)
    {
        InterlockedIncrement(&pTConState->refCount);
    }
    else
    {
        ULONG                      iMsg          = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
        PSMB2_TREE_CONNECT_REQUEST_HEADER pTreeConnectHeader = NULL;// Do not free
        UNICODE_STRING    wszPath = {0}; // Do not free

        if (pCtxSmb2->pTree)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SrvConnection2FindSession_SMB_V2(
                        pCtxSmb2,
                        pConnection,
                        pSmbRequest->pHeader->ullSessionId,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSetStatSession2Info(pExecContext, pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMB2UnmarshalTreeConnect(
                        pSmbRequest,
                        &pTreeConnectHeader,
                        &wszPath);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildTreeConnectState_SMB_V2(
                        pConnection,
                        pSession,
                        pTreeConnectHeader,
                        &wszPath,
                        &pTConState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pTConState;
        InterlockedIncrement(&pTConState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseTreeConnectStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pTConState->mutex);

    switch (pTConState->stage)
    {
        case SRV_TREE_CONNECT_STAGE_SMB_V2_INITIAL:

            LWIO_LOCK_RWMUTEX_SHARED(
                    bShareInfoInLock,
                    &pConnection->pHostinfo->mutex);

            ntStatus = SrvGetShareName(
                            pConnection->pHostinfo->pszHostname,
                            pConnection->pHostinfo->pszDomain,
                            pTConState->pwszPath,
                            &pwszSharename);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(bShareInfoInLock, &pConnection->pHostinfo->mutex);

            ntStatus = SrvShareFindByName(
                            pConnection->pShareList,
                            pwszSharename,
                            &pShareInfo);
            if (ntStatus == STATUS_NOT_FOUND)
            {
                ntStatus = STATUS_BAD_NETWORK_NAME;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSession2CreateTree(
                            pTConState->pSession,
                            pShareInfo,
                            &pTConState->pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            pTConState->bRemoveTreeFromSession = TRUE;

            // intentional fall through

        case SRV_TREE_CONNECT_STAGE_SMB_V2_CREATE_TREE_ROOT_HANDLE:

            if (pTConState->pTree->pShareInfo->service ==
                                SHARE_SERVICE_DISK_SHARE)
            {
                ntStatus = pTConState->ioStatusBlock.Status;
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = SrvCreateTreeRootHandle_SMB_V2(pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pTConState->stage = SRV_TREE_CONNECT_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_TREE_CONNECT_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildTreeConnectResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            // intentional fall through

        case SRV_TREE_CONNECT_STAGE_SMB_V2_DONE:

            pCtxSmb2->pTree = SrvTree2Acquire(pTConState->pTree);

            pTConState->bRemoveTreeFromSession = FALSE;

            break;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bShareInfoInLock, &pConnection->pHostinfo->mutex);

    if (pTConState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pTConState->mutex);

        SrvReleaseTreeConnectState_SMB_V2(pTConState);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    if (pShareInfo)
    {
        SrvShareReleaseInfo(pShareInfo);
    }

    SRV_SAFE_FREE_MEMORY(pwszSharename);

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed

            break;

        case STATUS_OBJECT_NAME_NOT_FOUND:

            ntStatus = STATUS_BAD_NETWORK_PATH;

            // Intentional fall through

        default:

            if (pTConState)
            {
                SrvReleaseTreeConnectStateAsync_SMB_V2(pTConState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildTreeConnectState_SMB_V2(
    PLWIO_SRV_CONNECTION              pConnection,
    PLWIO_SRV_SESSION_2               pSession,
    PSMB2_TREE_CONNECT_REQUEST_HEADER pTreeConnectHeader,
    PUNICODE_STRING                   pPath,
    PSRV_TREE_CONNECT_STATE_SMB_V2*   ppTConState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_TREE_CONNECT_STATE_SMB_V2),
                    (PVOID*)&pTConState);
    BAIL_ON_NT_STATUS(ntStatus);

    pTConState->refCount = 1;

    pthread_mutex_init(&pTConState->mutex, NULL);
    pTConState->pMutex = &pTConState->mutex;

    pTConState->stage = SRV_TREE_CONNECT_STAGE_SMB_V2_INITIAL;

    pTConState->pRequestHeader = pTreeConnectHeader;

    ntStatus = SrvAllocateMemory(
                        pPath->Length + sizeof(wchar16_t),
                        (PVOID*)&pTConState->pwszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy((PBYTE)pTConState->pwszPath, (PBYTE)pPath->Buffer, pPath->Length);

    pTConState->clientAddress         = pConnection->clientAddress;
    pTConState->ulClientAddressLength = pConnection->clientAddrLen;

    pTConState->pSession = SrvSession2Acquire(pSession);

    *ppTConState = pTConState;

cleanup:

    return ntStatus;

error:

    *ppTConState = NULL;

    if (pTConState)
    {
        SrvFreeTreeConnectState_SMB_V2(pTConState);
    }

    goto cleanup;
}

static
VOID
SrvPrepareTreeConnectStateAsync_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState,
    PSRV_EXEC_CONTEXT              pExecContext
    )
{
    pTConState->acb.Callback        = &SrvExecuteTreeConnectAsyncCB_SMB_V2;

    pTConState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pTConState->acb.AsyncCancelContext = NULL;

    pTConState->pAcb = &pTConState->acb;
}

static
VOID
SrvExecuteTreeConnectAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState   = NULL;
    BOOLEAN                        bInLock      = FALSE;

    pTConState =
        (PSRV_TREE_CONNECT_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pTConState->mutex);

    if (pTConState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pTConState->pAcb->AsyncCancelContext);
    }

    pTConState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pTConState->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V2.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

static
VOID
SrvReleaseTreeConnectStateAsync_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState
    )
{
    if (pTConState->pAcb)
    {
        pTConState->acb.Callback = NULL;

        if (pTConState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pTConState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pTConState->pAcb->CallbackContext = NULL;
        }

        if (pTConState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pTConState->pAcb->AsyncCancelContext);
        }

        pTConState->pAcb = NULL;
    }
}

static
NTSTATUS
SrvCreateTreeRootHandle_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState  = NULL;
    BOOLEAN                        bShareInLock     = FALSE;

    pTConState = (PSRV_TREE_CONNECT_STATE_SMB_V2)pCtxSmb2->hState;

    if (!pTConState->fileName.FileName)
    {
        LWIO_LOCK_RWMUTEX_SHARED(
                    bShareInLock,
                    &pTConState->pTree->pShareInfo->mutex);

        ntStatus = SrvAllocateStringW(
                        pTConState->pTree->pShareInfo->pwszPath,
                        &pTConState->fileName.FileName);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_RWMUTEX(
                    bShareInLock,
                    &pTConState->pTree->pShareInfo->mutex);

        ntStatus = SrvIoPrepareEcpList_SMB_V2(
                        pTConState,
                        &pTConState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!pTConState->pTree->hFile)
    {
        SrvPrepareTreeConnectStateAsync_SMB_V2(pTConState, pExecContext);

        ntStatus = SrvIoCreateFile(
                        pTConState->pTree->pShareInfo,
                        &pTConState->pTree->hFile,
                        pTConState->pAcb,
                        &pTConState->ioStatusBlock,
                        pTConState->pSession->pIoSecurityContext,
                        &pTConState->fileName,
                        pTConState->pSecurityDescriptor,
                        pTConState->pSecurityQOS,
                        FILE_READ_ATTRIBUTES,
                        0,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                        FILE_OPEN,
                        0,
                        NULL, /* EA Buffer */
                        0,    /* EA Length */
                        pTConState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTreeConnectStateAsync_SMB_V2(pTConState); // completed sync
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bShareInLock, &pTConState->pTree->pShareInfo->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvIoPrepareEcpList_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState, /* IN     */
    PIO_ECP_LIST*                  ppEcpList   /* IN OUT */
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PUNICODE_STRING pShareName = NULL;

    if (SrvElementsGetShareNameEcpEnabled())
    {
        if (!*ppEcpList)
        {
            ntStatus = IoRtlEcpListAllocate(ppEcpList);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = RTL_ALLOCATE(&pShareName, UNICODE_STRING, sizeof(*pShareName));
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlUnicodeStringAllocateFromWC16String(
                        pShareName,
                        pTConState->pTree->pShareInfo->pwszName);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoRtlEcpListInsert(
                        *ppEcpList,
                        SRV_ECP_TYPE_SHARE_NAME,
                        pShareName,
                        sizeof(*pShareName),
                        SrvIoFreeEcpString_SMB_V2);
        BAIL_ON_NT_STATUS(ntStatus);

        pShareName = NULL;
    }

    if (SrvElementsGetClientAddressEcpEnabled())
    {
        if (!*ppEcpList)
        {
            ntStatus = IoRtlEcpListAllocate(ppEcpList);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = IoRtlEcpListInsert(
                        *ppEcpList,
                        SRV_ECP_TYPE_CLIENT_ADDRESS,
                        &pTConState->clientAddress,
                        pTConState->ulClientAddressLength,
                        NULL);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (SrvElementsGetOEMSessionEcpEnabled() &&
        pTConState->pSession->pOEMSession)
    {
        if (!*ppEcpList)
        {
            ntStatus = IoRtlEcpListAllocate(ppEcpList);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = IoRtlEcpListInsert(
                        *ppEcpList,
                        SRV_ECP_TYPE_OEM_SESSION,
                        &pTConState->pSession->pOEMSession,
                        pTConState->pSession->ulOEMSessionLength,
                        NULL);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    RTL_FREE(&pShareName);

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvIoFreeEcpString_SMB_V2(
    IN PVOID pContext
    )
{
    PUNICODE_STRING pEcpString = (PUNICODE_STRING) pContext;

    if (pEcpString)
    {
        RtlUnicodeStringFree(pEcpString);
        RtlMemoryFree(pEcpString);
    }
}

static
NTSTATUS
SrvBuildTreeConnectResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse = &pCtxSmb2->pResponses[iMsg];
    PSMB2_TREE_CONNECT_RESPONSE_HEADER pTreeConnectResponseHeader = NULL; // Do not free
    PSRV_TREE_CONNECT_STATE_SMB_V2     pTConState  = NULL;
    PBYTE   pOutBuffer       = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset         = 0;
    ULONG   ulBytesUsed      = 0;
    ULONG   ulTotalBytesUsed = 0;

    pTConState = (PSRV_TREE_CONNECT_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_TREE_CONNECT,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pTConState->pTree->ulTid,
                    pTConState->pSession->ullUid,
                    0LL, /* Async Id */
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    ntStatus = SMB2MarshalTreeConnectResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pConnection,
                    pTConState->pTree,
                    &pTreeConnectResponseHeader,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pTreeConnectResponseHeader->ulShareFlags |=
    //                         SMB2_SHARE_FLAGS_DFS;
    // pTreeConnectResponseHeader->ulShareFlags |=
    //                         SMB2_SHARE_FLAGS_DFS_ROOT;
    // pTreeConnectResponseHeader->ulShareFlags |=
    //                         SMB2_SHARE_FLAGS_RESTRICT_EXCL_OPENS;
    // pTreeConnectResponseHeader->ulShareFlags |=
    //                         SMB2_SHARE_FLAGS_FORCE_SHARED_DELETE;
    // pTreeConnectResponseHeader->ulShareFlags |=
    //                         SMB2_SHARE_FLAGS_ALLOW_NS_CACHING;
    // pTreeConnectResponseHeader->ulShareFlags |=
    //                         SMB2_SHARE_FLAGS_ACCESS_BASED_DIR_ENUM;

    switch (pTreeConnectResponseHeader->usShareType)
    {
        case SMB2_SHARE_TYPE_DISK:

            // pTreeConnectResponseHeader->ulShareFlags |=
            //                         SMB2_SHARE_FLAGS_FORCE_LEVELII_OPLOCK;

            break;

        case SMB2_SHARE_TYPE_NAMED_PIPE:

            pTreeConnectResponseHeader->ulShareFlags |=
                                    SMB2_SHARE_FLAGS_CSC_NONE;

            break;

        default:

            break;

    }

    // pTreeConnectResponseHeader->ulShareFlags |=
    //                         SMB2_SHARE_FLAGS_ENABLE_HASH;

    // pTreeConnectRequestHeader->ulShareCapabilities |=
    //                             SMB2_SHARE_CAPABILITIES_DFS_AVAILABLE;

    // pOutBuffer += ulBytesUsed;
    // ulOffset += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
VOID
SrvReleaseTreeConnectStateHandle_SMB_V2(
    HANDLE hTConState
    )
{
    SrvReleaseTreeConnectState_SMB_V2((PSRV_TREE_CONNECT_STATE_SMB_V2)hTConState);
}

static
VOID
SrvReleaseTreeConnectState_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState
    )
{
    if (InterlockedDecrement(&pTConState->refCount) == 0)
    {
        SrvFreeTreeConnectState_SMB_V2(pTConState);
    }
}

static
VOID
SrvFreeTreeConnectState_SMB_V2(
    PSRV_TREE_CONNECT_STATE_SMB_V2 pTConState
    )
{
    if (pTConState->pAcb && pTConState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pTConState->pAcb->AsyncCancelContext);
    }

    if (pTConState->pEcpList)
    {
        IoRtlEcpListFree(&pTConState->pEcpList);
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pTConState->fileName.FileName)
    {
        SrvFreeMemory(pTConState->fileName.FileName);
    }

    if (pTConState->pwszPath)
    {
        SrvFreeMemory(pTConState->pwszPath);
    }

    if (pTConState->bRemoveTreeFromSession)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvSession2RemoveTree(
                        pTConState->pSession,
                        pTConState->pTree->ulTid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR(
                    "Failed to remove tid [%u] from session [uid=%u][code:%d]",
                    pTConState->pTree->ulTid,
                    pTConState->pSession->ullUid,
                    ntStatus2);
        }
    }

    if (pTConState->pSession)
    {
        SrvSession2Release(pTConState->pSession);
    }

    if (pTConState->pTree)
    {
        SrvTree2Release(pTConState->pTree);
    }

    if (pTConState->pMutex)
    {
        pthread_mutex_destroy(&pTConState->mutex);
    }

    SrvFreeMemory(pTConState);
}

