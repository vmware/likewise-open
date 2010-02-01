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
 *        ioctl.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        IOCTL
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildIOCTLState_SMB_V2(
    PLWIO_SRV_CONNECTION       pConnection,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_SRV_FILE_2           pFile,
    PSRV_IOCTL_STATE_SMB_V2*   ppIOCTLState
    );

static
NTSTATUS
SrvExecuteIOCTL_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildIOCTLResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareIOCTLStateAsync_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState,
    PSRV_EXEC_CONTEXT       pExecContext
    );

static
VOID
SrvExecuteIOCTLAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseIOCTLStateAsync_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    );

static
VOID
SrvReleaseIOCTLStateHandle_SMB_V2(
    HANDLE hIOCTLState
    );

static
VOID
SrvReleaseIOCTLState_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    );

static
VOID
SrvFreeIOCTLState_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    );

NTSTATUS
SrvProcessIOCTL_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;
    PLWIO_SRV_SESSION_2        pSession      = NULL;
    PLWIO_SRV_TREE_2           pTree         = NULL;
    PLWIO_SRV_FILE_2           pFile         = NULL;
    BOOLEAN                    bInLock       = FALSE;

    pIOCTLState = (PSRV_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;

    if (pIOCTLState)
    {
        InterlockedIncrement(&pIOCTLState->refCount);
    }
    else
    {
        ULONG                      iMsg           = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
        PSMB2_IOCTL_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PBYTE                      pData          = NULL; // Do not free

        ntStatus = SrvConnection2FindSession_SMB_V2(
                        pCtxSmb2,
                        pConnection,
                        pSmbRequest->pHeader->ullSessionId,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSession2FindTree_SMB_V2(
                        pCtxSmb2,
                        pSession,
                        pSmbRequest->pHeader->ulTid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMB2UnmarshalIOCTLRequest(
                        pSmbRequest,
                        &pRequestHeader,
                        &pData);
        BAIL_ON_NT_STATUS(ntStatus);

        switch (pRequestHeader->ulFunctionCode)
        {
            case IO_FSCTL_GET_DFS_REFERRALS:

                ntStatus = STATUS_FS_DRIVER_REQUIRED;

                break;

            case IO_FSCTL_PIPE_WAIT:

                ntStatus = STATUS_NOT_SUPPORTED;

                break;

            default:

                ntStatus = SrvTree2FindFile_SMB_V2(
                                pCtxSmb2,
                                pTree,
                                &pRequestHeader->fid,
                                LwIsSetFlag(
                                    pSmbRequest->pHeader->ulFlags,
                                    SMB2_FLAGS_RELATED_OPERATION),
                                &pFile);

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildIOCTLState_SMB_V2(
                        pConnection,
                        pRequestHeader,
                        pData,
                        pFile,
                        &pIOCTLState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pIOCTLState;
        InterlockedIncrement(&pIOCTLState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseIOCTLStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pIOCTLState->mutex);

    switch (pIOCTLState->stage)
    {
        case SRV_IOCTL_STAGE_SMB_V2_INITIAL:

            pIOCTLState->stage = SRV_IOCTL_STAGE_SMB_V2_ATTEMPT_IO;

            // intentional fall through

        case SRV_IOCTL_STAGE_SMB_V2_ATTEMPT_IO:

            ntStatus = SrvExecuteIOCTL_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pIOCTLState->stage = SRV_IOCTL_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_IOCTL_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildIOCTLResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pIOCTLState->stage = SRV_IOCTL_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_IOCTL_STAGE_SMB_V2_DONE:

            break;
    }

cleanup:

    if (pFile)
    {
        SrvFile2Release(pFile);
    }

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    if (pIOCTLState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pIOCTLState->mutex);

        SrvReleaseIOCTLState_SMB_V2(pIOCTLState);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed

            break;

        default:

            if (pIOCTLState)
            {
                SrvReleaseIOCTLStateAsync_SMB_V2(pIOCTLState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildIOCTLState_SMB_V2(
    PLWIO_SRV_CONNECTION       pConnection,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_SRV_FILE_2           pFile,
    PSRV_IOCTL_STATE_SMB_V2*   ppIOCTLState
    )
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_IOCTL_STATE_SMB_V2),
                    (PVOID*)&pIOCTLState);
    BAIL_ON_NT_STATUS(ntStatus);

    pIOCTLState->refCount = 1;

    pthread_mutex_init(&pIOCTLState->mutex, NULL);
    pIOCTLState->pMutex = &pIOCTLState->mutex;

    pIOCTLState->pConnection = SrvConnectionAcquire(pConnection);

    pIOCTLState->pFile = SrvFile2Acquire(pFile);

    pIOCTLState->pRequestHeader = pRequestHeader;
    pIOCTLState->pData          = pData;

    *ppIOCTLState = pIOCTLState;

cleanup:

    return ntStatus;

error:

    *ppIOCTLState = NULL;

    if (pIOCTLState)
    {
        SrvFreeIOCTLState_SMB_V2(pIOCTLState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteIOCTL_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;

    pIOCTLState = (PSRV_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pIOCTLState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pIOCTLState->pResponseBuffer)
    {
        // TODO: Should we just allocate 64 * 1024 bytes in this buffer?

        ntStatus = SMBPacketBufferAllocate(
                        pConnection->hPacketAllocator,
                        pIOCTLState->pRequestHeader->ulMaxOutLength,
                        &pIOCTLState->pResponseBuffer,
                        &pIOCTLState->sResponseBufferLen);
        BAIL_ON_NT_STATUS(ntStatus);

        pIOCTLState->ulResponseBufferLen = pIOCTLState->sResponseBufferLen;

        SrvPrepareIOCTLStateAsync_SMB_V2(pIOCTLState, pExecContext);

        if (pIOCTLState->pRequestHeader->ulFlags & 0x1)
        {
            ntStatus = IoFsControlFile(
                                    pIOCTLState->pFile->hFile,
                                    pIOCTLState->pAcb,
                                    &pIOCTLState->ioStatusBlock,
                                    pIOCTLState->pRequestHeader->ulFunctionCode,
                                    pIOCTLState->pData,
                                    pIOCTLState->pRequestHeader->ulInLength,
                                    pIOCTLState->pResponseBuffer,
                                    pIOCTLState->ulResponseBufferLen);
        }
        else
        {
            ntStatus = IoDeviceIoControlFile(
                                    pIOCTLState->pFile->hFile,
                                    pIOCTLState->pAcb,
                                    &pIOCTLState->ioStatusBlock,
                                    pIOCTLState->pRequestHeader->ulFunctionCode,
                                    pIOCTLState->pData,
                                    pIOCTLState->pRequestHeader->ulInLength,
                                    pIOCTLState->pResponseBuffer,
                                    pIOCTLState->ulResponseBufferLen);
        }
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseIOCTLStateAsync_SMB_V2(pIOCTLState); // completed sync
    }

    pIOCTLState->ulResponseBufferLen =
                                    pIOCTLState->ioStatusBlock.BytesTransferred;

cleanup:

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_NOT_SUPPORTED:

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

            break;

        default:

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildIOCTLResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_IOCTL_STATE_SMB_V2    pIOCTLState   = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE                     pOutBuffer       = pSmbResponse->pBuffer;
    ULONG                     ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG                     ulOffset         = 0;
    ULONG                     ulBytesUsed      = 0;
    ULONG                     ulTotalBytesUsed = 0;

    pIOCTLState = (PSRV_IOCTL_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_IOCTL,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
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

    ntStatus = SMB2MarshalIOCTLResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pIOCTLState->pRequestHeader,
                    pIOCTLState->pResponseBuffer,
                    SMB_MIN(pIOCTLState->ulResponseBufferLen,
                            pIOCTLState->pRequestHeader->ulMaxOutLength),
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += ulBytesUsed;
    // ulOffset         += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
VOID
SrvPrepareIOCTLStateAsync_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState,
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    pIOCTLState->acb.Callback        = &SrvExecuteIOCTLAsyncCB_SMB_V2;

    pIOCTLState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pIOCTLState->acb.AsyncCancelContext = NULL;

    pIOCTLState->pAcb = &pIOCTLState->acb;
}

static
VOID
SrvExecuteIOCTLAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_IOCTL_STATE_SMB_V2    pIOCTLState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pIOCTLState = (PSRV_IOCTL_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pIOCTLState->mutex);

    if (pIOCTLState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pIOCTLState->pAcb->AsyncCancelContext);
    }

    pIOCTLState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pIOCTLState->mutex);

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
SrvReleaseIOCTLStateAsync_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    )
{
    if (pIOCTLState->pAcb)
    {
        pIOCTLState->acb.Callback        = NULL;

        if (pIOCTLState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pIOCTLState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pIOCTLState->pAcb->CallbackContext = NULL;
        }

        if (pIOCTLState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pIOCTLState->pAcb->AsyncCancelContext);
        }

        pIOCTLState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseIOCTLStateHandle_SMB_V2(
    HANDLE hIOCTLState
    )
{
    return SrvReleaseIOCTLState_SMB_V2((PSRV_IOCTL_STATE_SMB_V2)hIOCTLState);
}

static
VOID
SrvReleaseIOCTLState_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    )
{
    if (InterlockedDecrement(&pIOCTLState->refCount) == 0)
    {
        SrvFreeIOCTLState_SMB_V2(pIOCTLState);
    }
}

static
VOID
SrvFreeIOCTLState_SMB_V2(
    PSRV_IOCTL_STATE_SMB_V2 pIOCTLState
    )
{
    if (pIOCTLState->pAcb && pIOCTLState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pIOCTLState->pAcb->AsyncCancelContext);
    }

    if (pIOCTLState->pFile)
    {
        SrvFile2Release(pIOCTLState->pFile);
    }

    if (pIOCTLState->pResponseBuffer)
    {
        SMBPacketBufferFree(
            pIOCTLState->pConnection->hPacketAllocator,
            pIOCTLState->pResponseBuffer,
            pIOCTLState->sResponseBufferLen);
    }

    if (pIOCTLState->pConnection)
    {
        SrvConnectionRelease(pIOCTLState->pConnection);
    }

    if (pIOCTLState->pMutex)
    {
        pthread_mutex_destroy(&pIOCTLState->mutex);
    }

    SrvFreeMemory(pIOCTLState);
}
