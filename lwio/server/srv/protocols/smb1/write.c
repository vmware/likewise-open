/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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

#include "includes.h"

static
NTSTATUS
SrvBuildWriteState(
    PWRITE_REQUEST_HEADER    pRequestHeader,
    PBYTE                    pData,
    PLWIO_SRV_FILE           pFile,
    PSRV_WRITE_STATE_SMB_V1* ppWriteState
    );

static
NTSTATUS
SrvExecuteWrite(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteWriteSetEndOfFile(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteWriteZctIo(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteWriteZctComplete(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareWriteStateAsync(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT       pExecContext
    );

static
VOID
SrvExecuteWriteAsyncCB(
    PVOID pContext
    );

static
VOID
SrvExecuteWriteReceiveZctCB(
    IN PVOID pContext,
    IN NTSTATUS Status
    );

static
VOID
SrvReleaseWriteStateAsync(
    PSRV_WRITE_STATE_SMB_V1 pWriteState
    );

static
NTSTATUS
SrvBuildWriteResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseWriteStateHandle(
    HANDLE hWriteState
    );

static
VOID
SrvReleaseWriteState(
    PSRV_WRITE_STATE_SMB_V1 pWriteState
    );

static
VOID
SrvFreeWriteState(
    PSRV_WRITE_STATE_SMB_V1 pWriteState
    );

NTSTATUS
SrvProcessWrite(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_WRITE_STATE_SMB_V1    pWriteState  = NULL;
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;
    PLWIO_SRV_FILE             pFile        = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pWriteState = (PSRV_WRITE_STATE_SMB_V1)pCtxSmb1->hState;

    if (pWriteState)
    {
        InterlockedIncrement(&pWriteState->refCount);
    }
    else
    {
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PWRITE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PBYTE pData = NULL; // Do not free

        ntStatus = SrvConnectionFindSession_SMB_V1(
                        pCtxSmb1,
                        pConnection,
                        pSmbRequest->pHeader->uid,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSetStatSessionInfo(pExecContext, pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSessionFindTree_SMB_V1(
                        pCtxSmb1,
                        pSession,
                        pSmbRequest->pHeader->tid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = WireUnmarshallWriteRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pData);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvTreeFindFile_SMB_V1(
                        pCtxSmb1,
                        pTree,
                        pRequestHeader->fid,
                        &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildWriteState(
                        pRequestHeader,
                        pData,
                        pFile,
                        &pWriteState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pWriteState;
        InterlockedIncrement(&pWriteState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseWriteStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);

    switch (pWriteState->stage)
    {
        case SRV_WRITE_STAGE_SMB_V1_INITIAL:

            pWriteState->ulKey = pSmbRequest->pHeader->pid;

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V1_ATTEMPT_WRITE;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V1_ATTEMPT_WRITE:

            if (pWriteState->ulLength)
            {
                ntStatus = SrvExecuteWrite(pWriteState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                ntStatus = SrvExecuteWriteSetEndOfFile(
                                pWriteState,
                                pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V1_ZCT_IO;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V1_ZCT_IO:

            if (pWriteState->Zct.pZct)
            {
                ntStatus = SrvExecuteWriteZctIo(pWriteState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V1_ZCT_COMPLETE;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V1_ZCT_COMPLETE:

            if (pWriteState->Zct.pZct)
            {
                LwZctDestroy(&pWriteState->Zct.pZct);

                SrvProtocolTransportResumeFromZct(pWriteState->Zct.pPausedConnection);
                SrvConnectionRelease(pWriteState->Zct.pPausedConnection);
                pWriteState->Zct.pPausedConnection = NULL;

                ntStatus = SrvExecuteWriteZctComplete(pWriteState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildWriteResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V1_DONE:

            break;
    }

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pWriteState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pWriteState->mutex);

        SrvReleaseWriteState(pWriteState);
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
                SrvReleaseWriteStateAsync(pWriteState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildWriteState(
    PWRITE_REQUEST_HEADER    pRequestHeader,
    PBYTE                    pData,
    PLWIO_SRV_FILE           pFile,
    PSRV_WRITE_STATE_SMB_V1* ppWriteState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_WRITE_STATE_SMB_V1 pWriteState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_WRITE_STATE_SMB_V1),
                    (PVOID*)&pWriteState);
    BAIL_ON_NT_STATUS(ntStatus);

    pWriteState->refCount = 1;

    pthread_mutex_init(&pWriteState->mutex, NULL);
    pWriteState->pMutex = &pWriteState->mutex;

    pWriteState->stage = SRV_WRITE_STAGE_SMB_V1_INITIAL;

    pWriteState->pFile          = SrvFileAcquire(pFile);

    pWriteState->pRequestHeader = pRequestHeader;
    pWriteState->pData          = pData;
    pWriteState->llOffset = pWriteState->pRequestHeader->offset;
    pWriteState->ulLength = pWriteState->pRequestHeader->dataLength;

    *ppWriteState = pWriteState;

cleanup:

    return ntStatus;

error:

    *ppWriteState = NULL;

    if (pWriteState)
    {
        SrvFreeWriteState(pWriteState);
    }

    goto cleanup;
}


static
NTSTATUS
SrvExecuteWrite(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pWriteState->Zct.pZct)
    {
        if (!pWriteState->bStartedIo)
        {
            SrvPrepareWriteStateAsync(pWriteState, pExecContext);
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
            SrvReleaseWriteStateAsync(pWriteState);
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
            SrvPrepareWriteStateAsync(pWriteState, pExecContext);

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
            SrvReleaseWriteStateAsync(pWriteState);
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
SrvExecuteWriteSetEndOfFile(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = pWriteState->ioStatusBlock.Status; // async response status
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pWriteState->pFileEofInfo)
    {
        pWriteState->fileEofInfo.EndOfFile = pWriteState->llOffset;
        pWriteState->pFileEofInfo = &pWriteState->fileEofInfo;

        SrvPrepareWriteStateAsync(pWriteState, pExecContext);

        ntStatus = IoSetInformationFile(
                        pWriteState->pFile->hFile,
                        pWriteState->pAcb,
                        &pWriteState->ioStatusBlock,
                        pWriteState->pFileEofInfo,
                        sizeof(pWriteState->fileEofInfo),
                        FileEndOfFileInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseWriteStateAsync(pWriteState); // completed synchronously
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvExecuteWriteZctIo(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
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
                    SrvExecuteWriteReceiveZctCB,
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
SrvExecuteWriteZctComplete(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pWriteState->bStartedIo)
    {
        SrvPrepareWriteStateAsync(pWriteState, pExecContext);
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
        SrvReleaseWriteStateAsync(pWriteState);
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
        SrvReleaseWriteStateAsync(pWriteState);
        pWriteState->bStartedIo = FALSE;
    }

    goto cleanup;
}

static
VOID
SrvPrepareWriteStateAsync(
    PSRV_WRITE_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    pWriteState->acb.Callback        = &SrvExecuteWriteAsyncCB;

    pWriteState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pWriteState->acb.AsyncCancelContext = NULL;

    pWriteState->pAcb = &pWriteState->acb;
}

static
VOID
SrvExecuteWriteAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_WRITE_STATE_SMB_V1    pWriteState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pWriteState =
        (PSRV_WRITE_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);

    if (pWriteState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pWriteState->pAcb->AsyncCancelContext);
    }

    pWriteState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pWriteState->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V1.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

static
VOID
SrvExecuteWriteReceiveZctCB(
    IN PVOID pContext,
    IN NTSTATUS Status
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_WRITE_STATE_SMB_V1    pWriteState       = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pWriteState =
        (PSRV_WRITE_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);
    pWriteState->stage = SRV_WRITE_STAGE_SMB_V1_ZCT_COMPLETE;
    if (Status == STATUS_SUCCESS)
    {
        pWriteState->ulBytesWritten += pWriteState->Zct.ulDataBytesMissing;
    }
    LWIO_UNLOCK_MUTEX(bInLock, &pWriteState->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V1.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context (status = 0x%08x)",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

static
VOID
SrvReleaseWriteStateAsync(
    PSRV_WRITE_STATE_SMB_V1 pWriteState
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
NTSTATUS
SrvBuildWriteResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PWRITE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG ulTotalBytesUsed     = 0;
    PSRV_WRITE_STATE_SMB_V1    pWriteState = NULL;

    pWriteState = (PSRV_WRITE_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_WRITE,
                        STATUS_SUCCESS,
                        TRUE,
                        pCtxSmb1->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pCtxSmb1->pSession->uid,
                        pSmbRequest->pHeader->mid,
                        pConnection->serverProperties.bRequireSecuritySignatures,
                        &pSmbResponse->pHeader,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    else
    {
        ntStatus = SrvMarshalHeaderAndX_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_WRITE,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 1;

    if (ulBytesAvailable < sizeof(WRITE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PWRITE_RESPONSE_HEADER)pOutBuffer;

    // pOutBuffer       += sizeof(WRITE_RESPONSE_HEADER);
    // ulOffset         += sizeof(WRITE_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(WRITE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(WRITE_RESPONSE_HEADER);

    LWIO_ASSERT(pWriteState->ulBytesWritten <= MAXUSHORT);

    pResponseHeader->count = (USHORT) pWriteState->ulBytesWritten;
    pResponseHeader->byteCount = 0;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->pAndXHeader = NULL;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
VOID
SrvReleaseWriteStateHandle(
    HANDLE hWriteState
    )
{
    SrvReleaseWriteState((PSRV_WRITE_STATE_SMB_V1)hWriteState);
}

static
VOID
SrvReleaseWriteState(
    PSRV_WRITE_STATE_SMB_V1 pWriteState
    )
{
    if (InterlockedDecrement(&pWriteState->refCount) == 0)
    {
        SrvFreeWriteState(pWriteState);
    }
}

static
VOID
SrvFreeWriteState(
    PSRV_WRITE_STATE_SMB_V1 pWriteState
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
        SrvFileRelease(pWriteState->pFile);
    }

    if (pWriteState->pMutex)
    {
        pthread_mutex_destroy(&pWriteState->mutex);
    }

    SrvFreeMemory(pWriteState);
}

NTSTATUS
SrvDetectZctWrite_SMB_V1(
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
    PWRITE_REQUEST_HEADER pWrite = NULL;
    PWRITE_ANDX_REQUEST_HEADER pWriteAndX = NULL;
    PBYTE pData = NULL;
    uint16_t fid = 0;
    LONG64 llOffset = 0;
    ULONG ulLength = 0;
    ULONG ulDataOffset = 0;
    ULONG ulZctThreshold = 0;
    LW_ZCT_ENTRY_MASK zctWriteMask = 0;
    LW_ZCT_ENTRY_MASK zctSocketMask = 0;
    SRV_ZCT_WRITE_STATE zctState = { 0 };
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    BOOLEAN bCanTryZct = FALSE;
    PSRV_EXEC_CONTEXT pExecContext = NULL;
    PSRV_WRITE_STATE_SMB_V1 pWriteState = NULL;
    PSRV_WRITEX_STATE_SMB_V1 pWriteXState = NULL;

    if (ulBytesAvailable < sizeof(SMB_HEADER))
    {
        ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pPacket->pSMBHeader = (PSMB_HEADER)pBuffer;
    pBuffer += sizeof(SMB_HEADER);
    ulBytesAvailable -= sizeof(SMB_HEADER);

    if (pPacket->pSMBHeader->smb[0] != 0xFF)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pPacket->protocolVer = SMB_PROTOCOL_VERSION_1;

    if (SMBIsAndXCommand(pPacket->pSMBHeader->command))
    {
        if (ulBytesAvailable < sizeof(ANDX_HEADER))
        {
            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pPacket->pAndXHeader = (PANDX_HEADER)pBuffer;

        // TODO: Support chaining.
        if (pPacket->pAndXHeader->andXCommand != 0xFF)
        {
            // Cannot do ZCT on chained command
            bCanTryZct = FALSE;
            ntStatus = STATUS_SUCCESS;
            goto cleanup;
        }

        pBuffer             += sizeof(ANDX_HEADER);
        ulBytesAvailable    -= sizeof(ANDX_HEADER);
    }

    ulRequestOffset = LwRtlPointerToOffset(pPacket->pSMBHeader, pBuffer);

    switch (pPacket->pSMBHeader->command)
    {
        case COM_WRITE:
        {
            ULONG ulRequired = LW_FIELD_OFFSET(WRITE_REQUEST_HEADER, dataLength) + LW_FIELD_SIZE(WRITE_REQUEST_HEADER, dataLength);

            if (ulBytesAvailable < ulRequired)
            {
                ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWrite = (PWRITE_REQUEST_HEADER) pBuffer;

            fid = pWrite->fid;
            llOffset = pWrite->offset;
            ulLength = pWrite->dataLength;
            ulDataOffset = sizeof(WRITE_REQUEST_HEADER);

            break;
        }
        case COM_WRITE_ANDX:
        {
            ULONG ulRequired = LW_FIELD_OFFSET(WRITE_ANDX_REQUEST_HEADER, offsetHigh) + LW_FIELD_SIZE(WRITE_ANDX_REQUEST_HEADER, offsetHigh);

            if (ulBytesAvailable < ulRequired)
            {
                ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWriteAndX = (PWRITE_ANDX_REQUEST_HEADER) pBuffer;

            fid = pWriteAndX->fid;
            llOffset = (((LONG64)pWriteAndX->offsetHigh) << 32) | ((LONG64)pWriteAndX->offset);
            ulLength = (((ULONG)pWriteAndX->dataLengthHigh) << 16) | ((ULONG)pWriteAndX->dataLength);
            ulDataOffset = pWriteAndX->dataOffset;
            if (ulDataOffset < ulRequestOffset)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            ulDataOffset -= ulRequestOffset;

            if (ulDataOffset < LW_FIELD_OFFSET(WRITE_ANDX_REQUEST_HEADER, pad))
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

    ntStatus = SrvConnectionFindSession_inlock(
                    pConnection,
                    pPacket->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree(
                    pSession,
                    pPacket->pSMBHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeFindFile(
                    pTree,
                    fid,
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

    ntStatus = SrvBuildExecContext_SMB_V1(
                    pExecContext->pConnection,
                    pExecContext->pSmbRequest,
                    &pExecContext->pProtocolContext->pSmb1Context);
    BAIL_ON_NT_STATUS(ntStatus);

    pExecContext->pProtocolContext->pSmb1Context->pSession = pSession;
    pSession = NULL;

    pExecContext->pProtocolContext->pSmb1Context->pTree = pTree;
    pTree = NULL;

    pExecContext->pProtocolContext->pSmb1Context->pFile = SrvFileAcquire(pFile);

    pData = (PBYTE) LwRtlOffsetToPointer(pBuffer, ulDataOffset);

    switch (pPacket->pSMBHeader->command)
    {
        case COM_WRITE:
        {
            // gets reference to file.
            ntStatus = SrvBuildWriteState(
                            pWrite,
                            pData,
                            pFile,
                            &pWriteState);
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteState->Zct = zctState;
            zctState.pZct = NULL;

            pWriteState->Zct.pPausedConnection = SrvConnectionAcquire(pConnection);

            pExecContext->pProtocolContext->pSmb1Context->hState = pWriteState;
            pExecContext->pProtocolContext->pSmb1Context->pfnStateRelease = SrvReleaseWriteStateHandle;
            pWriteState = NULL;

            break;
        }

        case COM_WRITE_ANDX:
        {
            // gets reference to file.
            ntStatus = SrvBuildWriteXState(
                            pWriteAndX,
                            pData,
                            pFile,
                            &pWriteXState);
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteXState->zct = zctState;
            zctState.pZct = NULL;

            pWriteXState->zct.pPausedConnection = SrvConnectionAcquire(pConnection);

            pExecContext->pProtocolContext->pSmb1Context->hState = pWriteXState;
            pExecContext->pProtocolContext->pSmb1Context->pfnStateRelease = SrvReleaseWriteXStateHandle;
            pWriteXState = NULL;

            break;
        }

        default:
        {
            LWIO_LOG_ERROR("Unexpected SMB command while processing SMB1 ZCT (0x%02x)",
                           pPacket->pSMBHeader->command);
            ntStatus = STATUS_ASSERTION_FAILURE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *ppZctExecContext = pExecContext;

    ntStatus = STATUS_SUCCESS;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pWriteState)
    {
        SrvReleaseWriteStateHandle(pWriteState);
    }

    if (pWriteXState)
    {
        SrvReleaseWriteXStateHandle(pWriteXState);
    }

    return ntStatus;

error:

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    goto cleanup;
}
