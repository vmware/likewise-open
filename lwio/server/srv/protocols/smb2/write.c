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
 *        write.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Write
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildWriteState_SMB_V2(
    PSMB2_WRITE_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_SRV_FILE_2           pFile,
    ULONG                      ulKey,
    PSRV_WRITE_STATE_SMB_V2*   ppWriteState
    );

static
NTSTATUS
SrvExecuteWrite_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteWriteZctIo_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteWriteZctComplete_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareWriteStateAsync_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState,
    PSRV_EXEC_CONTEXT       pExecContext
    );

static
VOID
SrvExecuteWriteAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvExecuteWriteReceiveZctCB_SMB_V2(
    IN PVOID pContext,
    IN NTSTATUS Status
    );

static
VOID
SrvReleaseWriteStateAsync_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState
    );

static
VOID
SrvReleaseWriteStateHandle_SMB_V2(
    HANDLE hWriteState
    );

static
VOID
SrvReleaseWriteState_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState
    );

static
VOID
SrvFreeWriteState_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState
    );

static
NTSTATUS
SrvBuildWriteResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessWrite_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PSRV_WRITE_STATE_SMB_V2    pWriteState  = NULL;
    PLWIO_SRV_SESSION_2        pSession     = NULL;
    PLWIO_SRV_TREE_2           pTree        = NULL;
    PLWIO_SRV_FILE_2           pFile        = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pWriteState = (PSRV_WRITE_STATE_SMB_V2)pCtxSmb2->hState;
    if (pWriteState)
    {
        InterlockedIncrement(&pWriteState->refCount);
    }
    else
    {
        ULONG                      iMsg           = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
        PSMB2_WRITE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PBYTE                      pData          = NULL; // Do not free

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

        ntStatus = SMB2UnmarshalWriteRequest(
                        pSmbRequest,
                        &pRequestHeader,
                        &pData);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvTree2FindFile_SMB_V2(
                            pCtxSmb2,
                            pTree,
                            &pRequestHeader->fid,
                            LwIsSetFlag(
                                pSmbRequest->pHeader->ulFlags,
                                SMB2_FLAGS_RELATED_OPERATION),
                            &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildWriteState_SMB_V2(
                            pRequestHeader,
                            pData,
                            pFile,
                            pSmbRequest->pHeader->ulPid,
                            &pWriteState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pWriteState;
        InterlockedIncrement(&pWriteState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseWriteStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);

    switch (pWriteState->stage)
    {
        case SRV_WRITE_STAGE_SMB_V2_INITIAL:

            pWriteState->llOffset =
                            pWriteState->pRequestHeader->ullFileOffset;

            pWriteState->ulLength =
                            pWriteState->pRequestHeader->ulDataLength;

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V2_ATTEMPT_WRITE;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V2_ATTEMPT_WRITE:

            ntStatus = SrvExecuteWrite_SMB_V2(pWriteState, pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V2_ZCT_IO;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V2_ZCT_IO:

            if (pWriteState->Zct.pZct)
            {
                ntStatus = SrvExecuteWriteZctIo_SMB_V2(pWriteState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V2_ZCT_COMPLETE;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V2_ZCT_COMPLETE:

            if (pWriteState->Zct.pZct)
            {
                LwZctDestroy(&pWriteState->Zct.pZct);

                SrvProtocolTransportResumeFromZct(pWriteState->Zct.pPausedConnection);
                SrvConnectionRelease(pWriteState->Zct.pPausedConnection);
                pWriteState->Zct.pPausedConnection = NULL;

                ntStatus = SrvExecuteWriteZctComplete_SMB_V2(pWriteState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildWriteResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V2_DONE:

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

    if (pWriteState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pWriteState->mutex);

        SrvReleaseWriteState_SMB_V2(pWriteState);
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

            if (pWriteState)
            {
                SrvReleaseWriteStateAsync_SMB_V2(pWriteState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildWriteState_SMB_V2(
    PSMB2_WRITE_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_SRV_FILE_2           pFile,
    ULONG                      ulKey,
    PSRV_WRITE_STATE_SMB_V2*   ppWriteState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_WRITE_STATE_SMB_V2 pWriteState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_WRITE_STATE_SMB_V2),
                    (PVOID*)&pWriteState);
    BAIL_ON_NT_STATUS(ntStatus);

    pWriteState->refCount = 1;

    pthread_mutex_init(&pWriteState->mutex, NULL);
    pWriteState->pMutex = &pWriteState->mutex;

    pWriteState->stage = SRV_WRITE_STAGE_SMB_V2_INITIAL;

    pWriteState->pRequestHeader = pRequestHeader;
    pWriteState->pData          = pData;

    pWriteState->pFile          = SrvFile2Acquire(pFile);

    pWriteState->ulKey          = ulKey;

    *ppWriteState = pWriteState;

cleanup:

    return ntStatus;

error:

    *ppWriteState = NULL;

    if (pWriteState)
    {
        SrvFreeWriteState_SMB_V2(pWriteState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteWrite_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pWriteState->Zct.pZct)
    {
        if (!pWriteState->bStartedIo)
        {
            SrvPrepareWriteStateAsync_SMB_V2(pWriteState, pExecContext);
            pWriteState->bStartedIo = TRUE;

            ntStatus = IoPrepareZctWriteFile(
                            pWriteState->pFile->hFile,
                            pWriteState->pAcb,
                            &pWriteState->ioStatusBlock,
                            0,
                            pWriteState->Zct.pZct,
                            pWriteState->ulLength,
                            &pWriteState->llOffset,
                            &pWriteState->ulKey,
                            &pWriteState->Zct.pZctCompletion);
            if (ntStatus == STATUS_NOT_SUPPORTED)
            {
                // Retry as non-ZCT
                LwZctDestroy(&pWriteState->Zct.pZct);
                pWriteState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            // completed synchronously
            SrvReleaseWriteStateAsync_SMB_V2(pWriteState);
        }

        pWriteState->bStartedIo = FALSE;

        ntStatus = pWriteState->ioStatusBlock.Status;
        if (ntStatus == STATUS_NOT_SUPPORTED)
        {
            // Retry as non-ZCT
            LwZctDestroy(&pWriteState->Zct.pZct);
            pWriteState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pWriteState->Zct.pZct)
        {
            // Cannot do ZCT

            // Read rest of data into packet.
            ntStatus = SrvProtocolTransportContinueAsNonZct(
                    pExecContext->pConnection,
                    pExecContext);
            // For now, returns STATUS_PENDING.
            LWIO_ASSERT(STATUS_PENDING == ntStatus);

            SrvConnectionRelease(pWriteState->Zct.pPausedConnection);
            pWriteState->Zct.pPausedConnection = NULL;

            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            ULONG zctLength = LwZctGetLength(pWriteState->Zct.pZct);
            LWIO_ASSERT(zctLength == pWriteState->ulLength);
        }
    }

    // This will also retry as non-ZCT if ZCT failed above.
    if (!pWriteState->Zct.pZct)
    {
        if (!pWriteState->bStartedIo)
        {
            SrvPrepareWriteStateAsync_SMB_V2(pWriteState, pExecContext);

            ntStatus = IoWriteFile(
                            pWriteState->pFile->hFile,
                            pWriteState->pAcb,
                            &pWriteState->ioStatusBlock,
                            pWriteState->pData,
                            pWriteState->ulLength,
                            &pWriteState->llOffset,
                            &pWriteState->ulKey);
            BAIL_ON_NT_STATUS(ntStatus);

            // completed synchronously
            SrvReleaseWriteStateAsync_SMB_V2(pWriteState);
        }

        pWriteState->bStartedIo = FALSE;

        ntStatus = pWriteState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pWriteState->ulBytesWritten =
            pWriteState->ioStatusBlock.BytesTransferred;

cleanup:

    return ntStatus;

error:

    if (ntStatus != STATUS_PENDING)
    {
        pWriteState->bStartedIo = FALSE;
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteWriteZctIo_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pZctContext = NULL;

    if (pWriteState->Zct.ulSkipBytes)
    {
        // Skip extra padding
        LW_ZCT_ENTRY entry;

        ntStatus = SrvAllocateMemory(pWriteState->Zct.ulSkipBytes, OUT_PPVOID(&pWriteState->Zct.pPadding));
        BAIL_ON_NT_STATUS(ntStatus);

        entry.Type = LW_ZCT_ENTRY_TYPE_MEMORY;
        entry.Length = pWriteState->Zct.ulSkipBytes;
        entry.Data.Memory.Buffer = pWriteState->Zct.pPadding;

        ntStatus = LwZctPrepend(pWriteState->Zct.pZct, &entry, 1);
        BAIL_ON_NT_STATUS(ntStatus);

        pWriteState->Zct.ulSkipBytes = 0;
    }

    ntStatus = LwZctPrepareIo(pWriteState->Zct.pZct);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pWriteState->Zct.ulDataBytesResident)
    {
        // Copy any bytes we already have
        ntStatus = LwZctReadBufferIo(
                        pWriteState->Zct.pZct,
                        pWriteState->pData,
                        pWriteState->Zct.ulDataBytesResident,
                        &pWriteState->ulBytesWritten,
                        NULL);
        LWIO_ASSERT(STATUS_MORE_PROCESSING_REQUIRED != ntStatus);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_ASSERT(pWriteState->ulBytesWritten == pWriteState->Zct.ulDataBytesResident);

        pWriteState->Zct.ulDataBytesResident = 0;
    }

    LWIO_ASSERT(pWriteState->Zct.ulDataBytesMissing > 0);

    pZctContext = SrvAcquireExecContext(pExecContext);

    // Do ZCT read from the transport for remaining bytes
    ntStatus = SrvProtocolTransportReceiveZct(
                    pExecContext->pConnection,
                    pWriteState->Zct.pZct,
                    SrvExecuteWriteReceiveZctCB_SMB_V2,
                    pZctContext);
    BAIL_ON_NT_STATUS(ntStatus);

    // completed synchronously
    SrvReleaseExecContext(pZctContext);

    pWriteState->ulBytesWritten += pWriteState->Zct.ulDataBytesMissing;

cleanup:

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
SrvExecuteWriteZctComplete_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pWriteState->bStartedIo)
    {
        SrvPrepareWriteStateAsync_SMB_V2(pWriteState, pExecContext);
        pWriteState->bStartedIo = TRUE;

        ntStatus = IoCompleteZctWriteFile(
                        pWriteState->pFile->hFile,
                        pWriteState->pAcb,
                        &pWriteState->ioStatusBlock,
                        0,
                        pWriteState->Zct.pZctCompletion,
                        pWriteState->ulBytesWritten);
        BAIL_ON_NT_STATUS(ntStatus);

        // completed synchronously
        SrvReleaseWriteStateAsync_SMB_V2(pWriteState);
        pWriteState->bStartedIo = FALSE;
    }

    ntStatus = pWriteState->ioStatusBlock.Status;
    if (ntStatus)
    {
        LWIO_LOG_ERROR("Failed to complete ZCT write file (status = 0x%08x)",
                       ntStatus);
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    if (ntStatus != STATUS_PENDING)
    {
        SrvReleaseWriteStateAsync_SMB_V2(pWriteState);
        pWriteState->bStartedIo = FALSE;
    }

    goto cleanup;
}

static
VOID
SrvPrepareWriteStateAsync_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState,
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    pWriteState->acb.Callback        = &SrvExecuteWriteAsyncCB_SMB_V2;

    pWriteState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pWriteState->acb.AsyncCancelContext = NULL;

    pWriteState->pAcb = &pWriteState->acb;
}

static
VOID
SrvExecuteWriteAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_WRITE_STATE_SMB_V2    pWriteState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pWriteState =
        (PSRV_WRITE_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);

    if (pWriteState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pWriteState->pAcb->AsyncCancelContext);
    }

    pWriteState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pWriteState->mutex);

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
SrvExecuteWriteReceiveZctCB_SMB_V2(
    IN PVOID pContext,
    IN NTSTATUS Status
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_WRITE_STATE_SMB_V2    pWriteState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pWriteState =
        (PSRV_WRITE_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);
    pWriteState->stage = SRV_WRITE_STAGE_SMB_V2_ZCT_COMPLETE;
    if (Status == STATUS_SUCCESS)
    {
        pWriteState->ulBytesWritten += pWriteState->Zct.ulDataBytesMissing;
    }
    LWIO_UNLOCK_MUTEX(bInLock, &pWriteState->mutex);

    LWIO_LOG_DEBUG("queue exec context = %p (conn = %p)", pContext, pExecContext->pConnection);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V2.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context (status = 0x%08x)",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

static
VOID
SrvReleaseWriteStateAsync_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState
    )
{
    if (pWriteState->pAcb)
    {
        pWriteState->acb.Callback = NULL;

        if (pWriteState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pWriteState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pWriteState->pAcb->CallbackContext = NULL;
        }

        if (pWriteState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pWriteState->pAcb->AsyncCancelContext);
        }

        pWriteState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseWriteStateHandle_SMB_V2(
    HANDLE hWriteState
    )
{
    SrvReleaseWriteState_SMB_V2((PSRV_WRITE_STATE_SMB_V2)hWriteState);
}

static
VOID
SrvReleaseWriteState_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState
    )
{
    if (InterlockedDecrement(&pWriteState->refCount) == 0)
    {
        SrvFreeWriteState_SMB_V2(pWriteState);
    }
}

static
VOID
SrvFreeWriteState_SMB_V2(
    PSRV_WRITE_STATE_SMB_V2 pWriteState
    )
{
    if (pWriteState->Zct.pZct)
    {
        LwZctDestroy(&pWriteState->Zct.pZct);
    }

    if (pWriteState->Zct.pPadding)
    {
        SrvFreeMemory(pWriteState->Zct.pPadding);
    }

    if (pWriteState->Zct.pPausedConnection)
    {
        SrvProtocolTransportResumeFromZct(pWriteState->Zct.pPausedConnection);
        SrvConnectionRelease(pWriteState->Zct.pPausedConnection);
    }

    if (pWriteState->pAcb && pWriteState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pWriteState->pAcb->AsyncCancelContext);
    }

    if (pWriteState->pFile)
    {
        SrvFile2Release(pWriteState->pFile);
    }

    if (pWriteState->pMutex)
    {
        pthread_mutex_destroy(&pWriteState->mutex);
    }

    SrvFreeMemory(pWriteState);
}

static
NTSTATUS
SrvBuildWriteResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_WRITE_STATE_SMB_V2    pWriteState   = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

    pWriteState = (PSRV_WRITE_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_WRITE,
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

    ntStatus = SMB2MarshalWriteResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pWriteState->ulBytesWritten,
                    (pWriteState->pRequestHeader->ulDataLength -
                     pWriteState->ulBytesWritten),
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

NTSTATUS
SrvDetectZctWrite_SMB_V2(
    IN PSRV_CONNECTION pConnection,
    IN PSMB_PACKET pPacket,
    OUT PSRV_EXEC_CONTEXT* ppZctExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NETBIOS_HEADER* pNetBiosHeader = (NETBIOS_HEADER*) pPacket->pRawBuffer;
    PBYTE pBuffer          = pPacket->pRawBuffer + sizeof(NETBIOS_HEADER);
    ULONG ulBytesAvailable = pPacket->bufferUsed - sizeof(NETBIOS_HEADER);
    ULONG ulRequestOffset = 0;
    PSMB2_WRITE_REQUEST_HEADER pWrite = NULL;
    PBYTE pData = NULL;
    SMB2_FID fid = { 0 };
    LONG64 llOffset = 0;
    ULONG ulLength = 0;
    ULONG ulDataOffset = 0;
    ULONG ulZctThreshold = 0;
    LW_ZCT_ENTRY_MASK zctWriteMask = 0;
    LW_ZCT_ENTRY_MASK zctSocketMask = 0;
    SRV_ZCT_WRITE_STATE_SMB_V2 zctState = { 0 };
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2 pTree = NULL;
    PLWIO_SRV_FILE_2 pFile = NULL;
    BOOLEAN bCanTryZct = FALSE;
    PSRV_EXEC_CONTEXT pExecContext = NULL;
    PSRV_WRITE_STATE_SMB_V2 pWriteState = NULL;

    if (ulBytesAvailable < sizeof(SMB2_HEADER))
    {
        ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pPacket->pSMB2Header = (PSMB2_HEADER)pBuffer;
    pBuffer += sizeof(SMB2_HEADER);
    ulBytesAvailable -= sizeof(SMB2_HEADER);

    if (pPacket->pSMB2Header->smb[0] != 0xFE)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pPacket->protocolVer = SMB_PROTOCOL_VERSION_2;

    // TODO: Support chaining.
    if (pPacket->pSMB2Header->ulChainOffset)
    {
        // Cannot do ZCT on chained command
        bCanTryZct = FALSE;
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    ulRequestOffset = LwRtlPointerToOffset(pPacket->pSMB2Header, pBuffer);

    switch (pPacket->pSMB2Header->command)
    {
        case COM2_WRITE:
        {
            ULONG ulRequired = sizeof(SMB2_WRITE_REQUEST_HEADER);

            if (ulBytesAvailable < ulRequired)
            {
                ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWrite = (PSMB2_WRITE_REQUEST_HEADER) pBuffer;

            fid = pWrite->fid;
            llOffset = pWrite->ullFileOffset;
            ulLength = pWrite->ulDataLength;
            ulDataOffset = pWrite->usDataOffset;

            if (ulDataOffset < ulRequestOffset)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            ulDataOffset -= ulRequestOffset;

            if (ulDataOffset < sizeof(SMB2_WRITE_REQUEST_HEADER))
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;
        }
        default:
            bCanTryZct = FALSE;
            ntStatus = STATUS_SUCCESS;
            goto cleanup;
    }

    // TODO: guard against overflow
    if ((ulRequestOffset + ulDataOffset + ulLength) > pNetBiosHeader->len)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulBytesAvailable >= (ulDataOffset + ulLength))
    {
        // Have all the data, so do not use ZCT
        bCanTryZct = FALSE;
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    // Do not use ZCT if threshold disabled or the I/O size < threshold.
    ulZctThreshold = pConnection->serverProperties.ulZctWriteThreshold;
    if ((ulZctThreshold == 0) ||
        (ulLength < ulZctThreshold))
    {
        // Should not use ZCT
        bCanTryZct = FALSE;
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    // Compute what's missing, present, and what can be skipped.
    zctState.ulDataBytesMissing = (ulDataOffset + ulLength) - ulBytesAvailable;
    if (ulBytesAvailable > ulDataOffset)
    {
        zctState.ulDataBytesResident = ulBytesAvailable - ulDataOffset;
        LWIO_ASSERT(zctState.ulDataBytesResident == (ulLength - zctState.ulDataBytesMissing));
    }
    else
    {
        zctState.ulSkipBytes = ulDataOffset - ulBytesAvailable;
    }

    // TODO-Do not do ZCT if very few bytes missing?

    ntStatus = SrvConnection2FindSession_inlock(
                    pConnection,
                    pPacket->pSMB2Header->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindTree(
                    pSession,
                    pPacket->pSMB2Header->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2FindFile(
                    pTree,
                    &fid,
                    &pFile);
    if (STATUS_INVALID_HANDLE == ntStatus)
    {
        // TODO: Perhaps short-circuit
        bCanTryZct = FALSE;
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    IoGetZctSupportMaskFile(
            pFile->hFile,
            NULL,
            &zctWriteMask);

    zctSocketMask = LwZctGetSystemSupportedMask(LW_ZCT_IO_TYPE_READ_SOCKET);
    if (!(zctWriteMask & zctSocketMask))
    {
        bCanTryZct = FALSE;
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    bCanTryZct = TRUE;

    ntStatus = LwZctCreate(&zctState.pZct, LW_ZCT_IO_TYPE_READ_SOCKET);
    BAIL_ON_NT_STATUS(ntStatus);

    // Note that building the context takes its own reference.
    ntStatus = SrvBuildExecContext(pConnection, pPacket, FALSE, &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProtocolAddContext(pExecContext, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildExecContext_SMB_V2(
                    pExecContext->pConnection,
                    pExecContext->pSmbRequest,
                    &pExecContext->pProtocolContext->pSmb2Context);
    BAIL_ON_NT_STATUS(ntStatus);

    pExecContext->pProtocolContext->pSmb2Context->pSession = pSession;
    pSession = NULL;

    pExecContext->pProtocolContext->pSmb2Context->pTree = pTree;
    pTree = NULL;

    pExecContext->pProtocolContext->pSmb2Context->pFile = SrvFile2Acquire(pFile);

    pData = (PBYTE) LwRtlOffsetToPointer(pBuffer, ulDataOffset);

    switch (pPacket->pSMB2Header->command)
    {
        case COM2_WRITE:
        {
            // gets reference to file.
            ntStatus = SrvBuildWriteState_SMB_V2(
                            pWrite,
                            pData,
                            pFile,
                            pPacket->pSMB2Header->ulPid,
                            &pWriteState);
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteState->Zct = zctState;
            zctState.pZct = NULL;

            pWriteState->Zct.pPausedConnection = SrvConnectionAcquire(pConnection);

            pExecContext->pProtocolContext->pSmb2Context->hState = pWriteState;
            pExecContext->pProtocolContext->pSmb2Context->pfnStateRelease = SrvReleaseWriteStateHandle_SMB_V2;
            pWriteState = NULL;

            break;
        }

        default:
        {
            LWIO_LOG_ERROR("Unexpected SMB command while processing SMB2 ZCT (0x%04x)",
                           pPacket->pSMB2Header->command);
            ntStatus = STATUS_ASSERTION_FAILURE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *ppZctExecContext = pExecContext;

    ntStatus = STATUS_SUCCESS;

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

    if (pWriteState)
    {
        SrvReleaseWriteStateHandle_SMB_V2(pWriteState);
    }

    return ntStatus;

error:

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    goto cleanup;
}
