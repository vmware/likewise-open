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
 *        read.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Read
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildReadState_SMB_V2(
    PSMB2_READ_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE_2          pFile,
    PSRV_READ_STATE_SMB_V2*   ppReadState
    );

static
NTSTATUS
SrvBuildZctReadState_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvAttemptReadIo_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSendZctReadResponse_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvCompleteZctRead_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareReadStateAsync_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT      pExecContext
    );

static
VOID
SrvExecuteReadAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvExecuteReadSendZctCB_SMB_V2(
    IN PVOID pContext,
    IN NTSTATUS Status
    );

static
VOID
SrvReleaseReadStateAsync_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState
    );

static
VOID
SrvReleaseReadStateHandle_SMB_V2(
    HANDLE hReadState
    );

static
VOID
SrvReleaseReadState_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState
    );

static
VOID
SrvFreeReadState_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState
    );

static
NTSTATUS
SrvBuildReadResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessRead_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PSRV_READ_STATE_SMB_V2     pReadState   = NULL;
    PLWIO_SRV_SESSION_2        pSession     = NULL;
    PLWIO_SRV_TREE_2           pTree        = NULL;
    PLWIO_SRV_FILE_2           pFile        = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pReadState = (PSRV_READ_STATE_SMB_V2)pCtxSmb2->hState;
    if (pReadState)
    {
        InterlockedIncrement(&pReadState->refCount);
    }
    else
    {
        ULONG                     iMsg           = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2       pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
        PSMB2_READ_REQUEST_HEADER pRequestHeader = NULL; // Do not free

        ntStatus = SrvConnection2FindSession_SMB_V2(
                        pCtxSmb2,
                        pConnection,
                        pSmbRequest->pHeader->ullSessionId,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSetStatSession2Info(pExecContext, pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSession2FindTree_SMB_V2(
                        pCtxSmb2,
                        pSession,
                        pSmbRequest->pHeader->ulTid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMB2UnmarshalReadRequest(pSmbRequest, &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_DEBUG(
            pExecContext->pLogContext,
            SMB_PROTOCOL_VERSION_2,
            pSmbRequest->pHeader->command,
            "Read request params: "
            "command(%u),uid(%llu),cmd-seq(%llu),pid(%u),tid(%u),"
            "credits(%u),flags(0x%x),chain-offset(%u),"
            "file-id(persistent:0x%x,volatile:0x%x),"
            "channel(%u),channel-info-length(%u),channel-info-offset(%u),"
            "minimum-count(%u),data-length(%u),data-offset(%llu)",
            pSmbRequest->pHeader->command,
            (long long)pSmbRequest->pHeader->ullSessionId,
            (long long)pSmbRequest->pHeader->ullCommandSequence,
            pSmbRequest->pHeader->ulPid,
            pSmbRequest->pHeader->ulTid,
            pSmbRequest->pHeader->usCredits,
            pSmbRequest->pHeader->ulFlags,
            pSmbRequest->pHeader->ulChainOffset,
            (long long)pRequestHeader->fid.ullPersistentId,
            (long long)pRequestHeader->fid.ullVolatileId,
            pRequestHeader->ulChannel,
            pRequestHeader->usReadChannelInfoLength,
            pRequestHeader->usReadChannelInfoOffset,
            pRequestHeader->ulMinimumCount,
            pRequestHeader->ulDataLength,
            (long long)pRequestHeader->ullFileOffset);

        ntStatus = SrvTree2FindFile_SMB_V2(
                        pCtxSmb2,
                        pTree,
                        &pRequestHeader->fid,
                        &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildReadState_SMB_V2(
                        pRequestHeader,
                        pFile,
                        &pReadState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pReadState;
        InterlockedIncrement(&pReadState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseReadStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pReadState->mutex);

    switch (pReadState->stage)
    {
        case SRV_READ_STAGE_SMB_V2_INITIAL:

            pReadState->llByteOffset =
                                pReadState->pRequestHeader->ullFileOffset;

            pReadState->ulBytesToRead = pReadState->pRequestHeader->ulDataLength;

            if (pReadState->ulBytesToRead > 0)
            {
                ntStatus = SrvAllocateMemory(
                            pReadState->pRequestHeader->ulDataLength,
                            (PVOID*)&pReadState->pData);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = SrvBuildZctReadState_SMB_V2(pReadState, pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pReadState->stage = SRV_READ_STAGE_SMB_V2_ATTEMPT_READ;

            // Intentional fall through

        case SRV_READ_STAGE_SMB_V2_ATTEMPT_READ:

            ntStatus = SrvAttemptReadIo_SMB_V2(pReadState, pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pReadState->stage = SRV_READ_STAGE_SMB_V2_ATTEMPT_READ_COMPLETED;

            // intentional fall through

        case SRV_READ_STAGE_SMB_V2_ATTEMPT_READ_COMPLETED:

            ntStatus = pReadState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pReadState->ulBytesRead = pReadState->ioStatusBlock.BytesTransferred;
            if (pReadState->ulBytesRead <
                            pReadState->pRequestHeader->ulMinimumCount)
            {
                ntStatus = STATUS_END_OF_FILE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pReadState->stage = SRV_READ_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_READ_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildReadResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            if (pReadState->pZct)
            {
                ntStatus = SrvSendZctReadResponse_SMB_V2(pReadState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pReadState->stage = SRV_READ_STAGE_SMB_V2_ZCT_COMPLETE;

            // intentional fall through

        case SRV_READ_STAGE_SMB_V2_ZCT_COMPLETE:

            if (pReadState->pZctCompletion)
            {
                ntStatus = SrvCompleteZctRead_SMB_V2(pReadState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pReadState->stage = SRV_READ_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_READ_STAGE_SMB_V2_DONE:

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

    if (pReadState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pReadState->mutex);

        SrvReleaseReadState_SMB_V2(pReadState);
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

        case STATUS_FILE_IS_A_DIRECTORY:

            ntStatus = STATUS_INVALID_DEVICE_REQUEST;

            // Intentional fall through

        default:

            if (pReadState)
            {
                SrvReleaseReadStateAsync_SMB_V2(pReadState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildReadState_SMB_V2(
    PSMB2_READ_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE_2          pFile,
    PSRV_READ_STATE_SMB_V2*   ppReadState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_READ_STATE_SMB_V2 pReadState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_READ_STATE_SMB_V2),
                    (PVOID*)&pReadState);
    BAIL_ON_NT_STATUS(ntStatus);

    pReadState->refCount = 1;

    pthread_mutex_init(&pReadState->mutex, NULL);
    pReadState->pMutex = &pReadState->mutex;

    pReadState->stage = SRV_READ_STAGE_SMB_V2_INITIAL;

    pReadState->pRequestHeader = pRequestHeader;

    pReadState->pFile = SrvFile2Acquire(pFile);

    *ppReadState = pReadState;

cleanup:

    return ntStatus;

error:

    *ppReadState = NULL;

    if (pReadState)
    {
        SrvFreeReadState_SMB_V2(pReadState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildZctReadState_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG ulZctThreshold = 0;
    LW_ZCT_ENTRY_MASK zctReadMask = 0;
    LW_ZCT_ENTRY_MASK zctSocketMask = 0;

    // Do not use ZCT if chained or if signing active.
    if ((pCtxSmb2->ulNumRequests != 1) ||
        SrvConnectionIsSigningActive(pConnection))
    {
        goto cleanup;
    }

    // Do not use ZCT if threshold disabled or the I/O size < threshold.
    ulZctThreshold = pConnection->serverProperties.ulZctReadThreshold;
    if ((ulZctThreshold == 0) ||
        (pReadState->ulBytesToRead < ulZctThreshold))
    {
        goto cleanup;
    }

    // Use ZCT only if file and system masks intersect.

    IoGetZctSupportMaskFile(
            pReadState->pFile->hFile,
            &zctReadMask,
            NULL);

    zctSocketMask = LwZctGetSystemSupportedMask(LW_ZCT_IO_TYPE_WRITE_SOCKET);
    if (zctReadMask & zctSocketMask)
    {
        ntStatus = LwZctCreate(
                        &pReadState->pZct,
                        LW_ZCT_IO_TYPE_WRITE_SOCKET);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    LwZctDestroy(&pReadState->pZct);

    goto cleanup;
}

static
NTSTATUS
SrvAttemptReadIo_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pReadState->pZct)
    {
        if (!pReadState->bStartedRead)
        {
            SrvPrepareReadStateAsync_SMB_V2(pReadState, pExecContext);
            pReadState->bStartedRead = TRUE;

            ntStatus = IoPrepareZctReadFile(
                            pReadState->pFile->hFile,
                            pReadState->pAcb,
                            &pReadState->ioStatusBlock,
                            0,
                            pReadState->pZct,
                            pReadState->ulBytesToRead,
                            &pReadState->llByteOffset,
                            &pReadState->ulKey,
                            &pReadState->pZctCompletion);
            if (ntStatus == STATUS_NOT_SUPPORTED)
            {
                SrvReleaseReadStateAsync_SMB_V2(pReadState); // Retry as non-ZCT
                pReadState->bStartedRead = FALSE;
                LwZctDestroy(&pReadState->pZct);
                pReadState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
            }
            else if (ntStatus == STATUS_END_OF_FILE)
            {
                pReadState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            // completed synchronously
            SrvReleaseReadStateAsync_SMB_V2(pReadState);
            pReadState->bStartedRead = FALSE;
        }

        ntStatus = pReadState->ioStatusBlock.Status;
        if (ntStatus == STATUS_NOT_SUPPORTED)
        {
            // Retry as non-ZCT
            SrvReleaseReadStateAsync_SMB_V2(pReadState);
            pReadState->bStartedRead = FALSE;
            LwZctDestroy(&pReadState->pZct);
            pReadState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
        }
        else if (ntStatus == STATUS_END_OF_FILE)
        {
            pReadState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // This will retry as non-ZCT if ZCT failed above
    if (!pReadState->pZct)
    {
        if (!pReadState->bStartedRead)
        {
            SrvPrepareReadStateAsync_SMB_V2(pReadState, pExecContext);
            pReadState->bStartedRead = TRUE;

            ntStatus = IoReadFile(
                            pReadState->pFile->hFile,
                            pReadState->pAcb,
                            &pReadState->ioStatusBlock,
                            pReadState->pData,
                            pReadState->ulBytesToRead,
                            &pReadState->llByteOffset,
                            &pReadState->ulKey);
            if (ntStatus == STATUS_END_OF_FILE)
            {
                pReadState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            // completed synchronously
            SrvReleaseReadStateAsync_SMB_V2(pReadState);
            pReadState->bStartedRead = FALSE;
        }

        ntStatus = pReadState->ioStatusBlock.Status;
        if (ntStatus == STATUS_END_OF_FILE)
        {
            ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pReadState->pZct && pReadState->ioStatusBlock.Status)
    {
        // No data, must treat this as non-ZCT for the rest of processing.
        LwZctDestroy(&pReadState->pZct);
    }

    pReadState->ulBytesRead = pReadState->ioStatusBlock.BytesTransferred;

cleanup:

    return ntStatus;

error:

    if (ntStatus != STATUS_PENDING)
    {
        pReadState->bStartedRead = FALSE;
    }

    goto cleanup;
}

static
NTSTATUS
SrvSendZctReadResponse_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbResponse = &pCtxSmb2->pResponses[iMsg];
    PSRV_EXEC_CONTEXT          pZctContext  = NULL;
    LW_ZCT_ENTRY entry = { 0 };

    pExecContext->pSmbResponse->bufferUsed += pSmbResponse->ulMessageSize;
    SMBPacketMarshallFooter(pExecContext->pSmbResponse);

    entry.Type = LW_ZCT_ENTRY_TYPE_MEMORY;
    entry.Length = pExecContext->pSmbResponse->bufferUsed - pReadState->ulBytesRead;
    entry.Data.Memory.Buffer = pExecContext->pSmbResponse->pRawBuffer;

    ntStatus = LwZctPrepend(pReadState->pZct, &entry, 1);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwZctPrepareIo(pReadState->pZct);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->ulZctMessageSize = pSmbResponse->ulMessageSize;

    pZctContext = SrvAcquireExecContext(pExecContext);

    ntStatus = SrvProtocolTransportSendZctResponse(
                    pConnection,
                    pReadState->pZct,
                    pExecContext->pStatInfo,
                    SrvExecuteReadSendZctCB_SMB_V2,
                    pZctContext);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseExecContext(pZctContext); // completed synchronously

cleanup:

    // TODO: Perhaps only if SrvProtocolTransportSendZctResponse() was
    // called and did not return STATUS_INSUFFICIENT_RESOURCES.
    // Never send out response via non-ZCT
    pSmbResponse->ulMessageSize = 0;
    pExecContext->pSmbResponse->bufferUsed = 0;

    return ntStatus;

error:

    if (pZctContext && (ntStatus != STATUS_PENDING))
    {
        SrvReleaseExecContext(pZctContext);
    }

    goto cleanup;
}

static
NTSTATUS
SrvCompleteZctRead_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pReadState->bStartedRead)
    {
        SrvPrepareReadStateAsync_SMB_V2(pReadState, pExecContext);
        pReadState->bStartedRead = TRUE;

        ntStatus = IoCompleteZctReadFile(
                        pReadState->pFile->hFile,
                        pReadState->pAcb,
                        &pReadState->ioStatusBlock,
                        0,
                        pReadState->pZctCompletion);
        BAIL_ON_NT_STATUS(ntStatus);

        // completed synchronously
        SrvReleaseReadStateAsync_SMB_V2(pReadState);
        pReadState->bStartedRead = FALSE;
    }

    ntStatus = pReadState->ioStatusBlock.Status;
    if (ntStatus)
    {
        LWIO_LOG_ERROR("Failed to complete ZCT read file [status:0x%x]",
                       ntStatus);
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    if (ntStatus != STATUS_PENDING)
    {
        SrvReleaseReadStateAsync_SMB_V2(pReadState);
        pReadState->bStartedRead = FALSE;
    }

    goto cleanup;
}

static
VOID
SrvPrepareReadStateAsync_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState,
    PSRV_EXEC_CONTEXT      pExecContext
    )
{
    pReadState->acb.Callback        = &SrvExecuteReadAsyncCB_SMB_V2;

    pReadState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pReadState->acb.AsyncCancelContext = NULL;

    pReadState->pAcb = &pReadState->acb;
}

static
VOID
SrvExecuteReadAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_READ_STATE_SMB_V2     pReadState       = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pReadState =
        (PSRV_READ_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pReadState->mutex);

    if (pReadState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pReadState->pAcb->AsyncCancelContext);
    }

    pReadState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pReadState->mutex);

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
SrvExecuteReadSendZctCB_SMB_V2(
    IN PVOID pContext,
    IN NTSTATUS Status
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_READ_STATE_SMB_V2     pReadState       = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pReadState =
        (PSRV_READ_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pReadState->mutex);
    pReadState->stage = SRV_READ_STAGE_SMB_V2_ZCT_COMPLETE;
    LWIO_UNLOCK_MUTEX(bInLock, &pReadState->mutex);

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
SrvReleaseReadStateAsync_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState
    )
{
    if (pReadState->pAcb)
    {
        pReadState->acb.Callback = NULL;

        if (pReadState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pReadState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pReadState->pAcb->CallbackContext = NULL;
        }

        if (pReadState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pReadState->pAcb->AsyncCancelContext);
        }

        pReadState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseReadStateHandle_SMB_V2(
    HANDLE hReadState
    )
{
    SrvReleaseReadState_SMB_V2((PSRV_READ_STATE_SMB_V2)hReadState);
}

static
VOID
SrvReleaseReadState_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState
    )
{
    if (InterlockedDecrement(&pReadState->refCount) == 0)
    {
        SrvFreeReadState_SMB_V2(pReadState);
    }
}

static
VOID
SrvFreeReadState_SMB_V2(
    PSRV_READ_STATE_SMB_V2 pReadState
    )
{
    if (pReadState->pAcb && pReadState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pReadState->pAcb->AsyncCancelContext);
    }

    if (pReadState->pFile)
    {
        SrvFile2Release(pReadState->pFile);
    }

    if (pReadState->pMutex)
    {
        pthread_mutex_destroy(&pReadState->mutex);
    }

    if (pReadState->pData)
    {
        SrvFreeMemory(pReadState->pData);
    }

    if (pReadState->pZct)
    {
        LwZctDestroy(&pReadState->pZct);
    }

    SrvFreeMemory(pReadState);
}

static
NTSTATUS
SrvBuildReadResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PSRV_READ_STATE_SMB_V2     pReadState       = NULL;
    ULONG                      ulDataOffset     = 0L;
    PBYTE                      pOutBuffer       = pSmbResponse->pBuffer;
    ULONG                      ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG                      ulOffset         = 0;
    ULONG                      ulBytesUsed      = 0;
    ULONG                      ulTotalBytesUsed = 0;

    pReadState = (PSRV_READ_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_READ,
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

    ntStatus = SMB2MarshalReadResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    NULL,
                    pReadState->pRequestHeader->ulDataLength,
                    0,
                    &ulDataOffset,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalReadResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pReadState->pZct ? NULL : pReadState->pData,
                    pReadState->ulBytesRead,
                    pReadState->ulRemaining,
                    &ulDataOffset,
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

