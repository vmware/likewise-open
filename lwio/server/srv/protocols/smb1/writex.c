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
SrvExecuteWriteAndX(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteWriteXZctIo(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteWriteXZctComplete(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareWriteXStateAsync(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT     pExecContext
    );

static
VOID
SrvExecuteWriteXAsyncCB(
    PVOID pContext
    );

static
VOID
SrvExecuteWriteXReceiveZctCB(
    IN PVOID pContext,
    IN NTSTATUS Status
    );

static
VOID
SrvReleaseWriteXStateAsync(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState
    );

static
NTSTATUS
SrvBuildWriteAndXResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseWriteXState(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState
    );

static
VOID
SrvFreeWriteXState(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState
    );

NTSTATUS
SrvProcessWriteAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;
    PLWIO_SRV_FILE             pFile        = NULL;
    PSRV_WRITEX_STATE_SMB_V1   pWriteState  = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pWriteState = (PSRV_WRITEX_STATE_SMB_V1)pCtxSmb1->hState;

    if (pWriteState)
    {
        InterlockedIncrement(&pWriteState->refCount);
    }
    else
    {
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PWRITE_ANDX_REQUEST_HEADER_WC_12 pRequestHeader_WC_12 = NULL; // Do not free
        PWRITE_ANDX_REQUEST_HEADER_WC_14 pRequestHeader_WC_14 = NULL; // Do not free
        PBYTE                      pData          = NULL; // Do not free

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

        switch (*pSmbRequest->pWordCount)
        {
            case 12:

                ntStatus = WireUnmarshallWriteAndXRequest_WC_12(
                                pBuffer,
                                ulBytesAvailable,
                                ulOffset,
                                &pRequestHeader_WC_12,
                                &pData);
                BAIL_ON_NT_STATUS(ntStatus);

                SRV_LOG_DEBUG(
                        pExecContext->pLogContext,
                        SMB_PROTOCOL_VERSION_1,
                        pSmbRequest->pHeader->command,
                        "WriteAndX(WC12) request params: "
                        "command(%u),uid(%u),mid(%u),pid(%u),tid(%u),"
                        "file-id(%u),data-length-high(%u),data-length(%u),"
                        "offset(%u)",
                        pSmbRequest->pHeader->command,
                        pSmbRequest->pHeader->uid,
                        pSmbRequest->pHeader->mid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pSmbRequest->pHeader->tid,
                        pRequestHeader_WC_12->fid,
                        pRequestHeader_WC_12->dataLengthHigh,
                        pRequestHeader_WC_12->dataLength,
                        pRequestHeader_WC_12->offset);

                ntStatus = SrvTreeFindFile_SMB_V1(
                                pCtxSmb1,
                                pTree,
                                pRequestHeader_WC_12->fid,
                                &pFile);
                BAIL_ON_NT_STATUS(ntStatus);

                break;

            case 14:

                ntStatus = WireUnmarshallWriteAndXRequest_WC_14(
                                pBuffer,
                                ulBytesAvailable,
                                ulOffset,
                                &pRequestHeader_WC_14,
                                &pData);
                BAIL_ON_NT_STATUS(ntStatus);

                SRV_LOG_DEBUG(
                        pExecContext->pLogContext,
                        SMB_PROTOCOL_VERSION_1,
                        pSmbRequest->pHeader->command,
                        "WriteAndX(WC14) request params: "
                        "command(%u),uid(%u),mid(%u),pid(%u),tid(%u),"
                        "file-id(%u),data-length-high(%u),data-length(%u),"
                        "offset-high(%u),offset(%u)",
                        pSmbRequest->pHeader->command,
                        pSmbRequest->pHeader->uid,
                        pSmbRequest->pHeader->mid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pSmbRequest->pHeader->tid,
                        pRequestHeader_WC_14->fid,
                        pRequestHeader_WC_14->dataLengthHigh,
                        pRequestHeader_WC_14->dataLength,
                        pRequestHeader_WC_14->offsetHigh,
                        pRequestHeader_WC_14->offset);

                ntStatus = SrvTreeFindFile_SMB_V1(
                                pCtxSmb1,
                                pTree,
                                pRequestHeader_WC_14->fid,
                                &pFile);
                BAIL_ON_NT_STATUS(ntStatus);

                break;

            default:

                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

        ntStatus = SrvBuildWriteXState(
                        *pSmbRequest->pWordCount,
                        pRequestHeader_WC_12,
                        pRequestHeader_WC_14,
                        pData,
                        pFile,
                        &pWriteState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pWriteState;
        InterlockedIncrement(&pWriteState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseWriteXStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);

    switch (pWriteState->stage)
    {
        case SRV_WRITEX_STAGE_SMB_V1_INITIAL:

            pWriteState->ulKey = pSmbRequest->pHeader->pid;

            pWriteState->stage = SRV_WRITEX_STAGE_SMB_V1_ATTEMPT_WRITE;

            // intentional fall through

        case SRV_WRITEX_STAGE_SMB_V1_ATTEMPT_WRITE:

            pWriteState->stage = SRV_WRITEX_STAGE_SMB_V1_ZCT_IO;

            ntStatus = SrvExecuteWriteAndX(pWriteState, pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            // intentional fall through

        case SRV_WRITEX_STAGE_SMB_V1_ZCT_IO:

            pWriteState->ulBytesWritten =
                pWriteState->ioStatusBlock.BytesTransferred;

            if (pWriteState->zct.pZct)
            {
                ntStatus = SrvExecuteWriteXZctIo(pWriteState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWriteState->stage = SRV_WRITEX_STAGE_SMB_V1_ZCT_COMPLETE;

            // intentional fall through

        case SRV_WRITEX_STAGE_SMB_V1_ZCT_COMPLETE:

            if (pWriteState->zct.pZct)
            {
                LwZctDestroy(&pWriteState->zct.pZct);

                SrvProtocolTransportResumeFromZct(pWriteState->zct.pPausedConnection);
                SrvConnectionRelease(pWriteState->zct.pPausedConnection);
                pWriteState->zct.pPausedConnection = NULL;

                ntStatus = SrvExecuteWriteXZctComplete(pWriteState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pWriteState->stage = SRV_WRITEX_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_WRITEX_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildWriteAndXResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteState->stage = SRV_WRITEX_STAGE_SMB_V1_DONE;

        case SRV_WRITEX_STAGE_SMB_V1_DONE:

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

        SrvReleaseWriteXState(pWriteState);
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
                SrvReleaseWriteXStateAsync(pWriteState);
            }

            break;
    }

    goto cleanup;
}

NTSTATUS
SrvBuildWriteXState(
    UCHAR                            ucWordCount,
    PWRITE_ANDX_REQUEST_HEADER_WC_12 pRequestHeader_WC_12,
    PWRITE_ANDX_REQUEST_HEADER_WC_14 pRequestHeader_WC_14,
    PBYTE                            pData,
    PLWIO_SRV_FILE                   pFile,
    PSRV_WRITEX_STATE_SMB_V1*        ppWriteState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_WRITEX_STATE_SMB_V1 pWriteState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_WRITEX_STATE_SMB_V1),
                    (PVOID*)&pWriteState);
    BAIL_ON_NT_STATUS(ntStatus);

    pWriteState->refCount = 1;

    pthread_mutex_init(&pWriteState->mutex, NULL);
    pWriteState->pMutex = &pWriteState->mutex;

    pWriteState->stage = SRV_WRITEX_STAGE_SMB_V1_INITIAL;

    pWriteState->pFile          = SrvFileAcquire(pFile);

    pWriteState->ucWordCount = ucWordCount;

    switch (ucWordCount)
    {
        case 12:

            pWriteState->pRequestHeader_WC_12 = pRequestHeader_WC_12;
            pWriteState->llOffset =
                ((LONG64)pWriteState->pRequestHeader_WC_12->offset);
            pWriteState->ulLength =
                (((LONG64)pWriteState->pRequestHeader_WC_12->dataLengthHigh) << 16) |
                ((LONG64)pWriteState->pRequestHeader_WC_12->dataLength);
            break;

        case 14:

            pWriteState->pRequestHeader_WC_14 = pRequestHeader_WC_14;
            pWriteState->llOffset =
                (((LONG64)pWriteState->pRequestHeader_WC_14->offsetHigh) << 32) |
                ((LONG64)pWriteState->pRequestHeader_WC_14->offset);
            pWriteState->ulLength =
                (((LONG64)pWriteState->pRequestHeader_WC_14->dataLengthHigh) << 16) |
                ((LONG64)pWriteState->pRequestHeader_WC_14->dataLength);
            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    pWriteState->pData          = pData;

    *ppWriteState = pWriteState;

cleanup:

    return ntStatus;

error:

    *ppWriteState = NULL;

    if (pWriteState)
    {
        SrvFreeWriteXState(pWriteState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteWriteAndX(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT        pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pWriteState->zct.pZct)
    {
        if (!pWriteState->bStartedIo)
        {
            SrvPrepareWriteXStateAsync(pWriteState, pExecContext);
            pWriteState->bStartedIo = TRUE;

            ntStatus = IoPrepareZctWriteFile(
                            pWriteState->pFile->hFile,
                            pWriteState->pAcb,
                            &pWriteState->ioStatusBlock,
                            0,
                            pWriteState->zct.pZct,
                            pWriteState->ulLength,
                            &pWriteState->llOffset,
                            &pWriteState->ulKey,
                            &pWriteState->zct.pZctCompletion);
            if (ntStatus == STATUS_NOT_SUPPORTED)
            {
                // Retry as non-ZCT
                LwZctDestroy(&pWriteState->zct.pZct);
                pWriteState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            // completed synchronously
            SrvReleaseWriteXStateAsync(pWriteState);
        }

        pWriteState->bStartedIo = FALSE;

        ntStatus = pWriteState->ioStatusBlock.Status;
        if (ntStatus == STATUS_NOT_SUPPORTED)
        {
            // Retry as non-ZCT
            LwZctDestroy(&pWriteState->zct.pZct);
            pWriteState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pWriteState->zct.pZct)
        {
            // Cannot do ZCT

            // Read rest of data into packet.
            ntStatus = SrvProtocolTransportContinueAsNonZct(
                            pExecContext->pConnection,
                            pExecContext);

            // For now, returns STATUS_PENDING.
            LWIO_ASSERT(STATUS_PENDING == ntStatus);

            SrvConnectionRelease(pWriteState->zct.pPausedConnection);
            pWriteState->zct.pPausedConnection = NULL;

            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            ULONG zctLength = LwZctGetLength(pWriteState->zct.pZct);

            LWIO_ASSERT(zctLength == pWriteState->ulLength);
        }
    }

    // This will also retry as non-ZCT if ZCT failed above.
    if (!pWriteState->zct.pZct)
    {
        if (!pWriteState->bStartedIo)
        {
            SrvPrepareWriteXStateAsync(pWriteState, pExecContext);

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
            SrvReleaseWriteXStateAsync(pWriteState);
        }

        pWriteState->bStartedIo = FALSE;

        ntStatus = pWriteState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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
SrvExecuteWriteXZctIo(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pZctContext = NULL;

    if (pWriteState->zct.ulSkipBytes)
    {
        // Skip extra padding
        LW_ZCT_ENTRY entry;

        ntStatus = SrvAllocateMemory(pWriteState->zct.ulSkipBytes, OUT_PPVOID(&pWriteState->zct.pPadding));
        BAIL_ON_NT_STATUS(ntStatus);

        entry.Type = LW_ZCT_ENTRY_TYPE_MEMORY;
        entry.Length = pWriteState->zct.ulSkipBytes;
        entry.Data.Memory.Buffer = pWriteState->zct.pPadding;

        ntStatus = LwZctPrepend(pWriteState->zct.pZct, &entry, 1);
        BAIL_ON_NT_STATUS(ntStatus);

        pWriteState->zct.ulPaddingSize = pWriteState->zct.ulSkipBytes;
        pWriteState->zct.ulSkipBytes = 0;
    }

    ntStatus = LwZctPrepareIo(pWriteState->zct.pZct);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pWriteState->zct.ulDataBytesResident)
    {
        // Copy any bytes we already have
        ntStatus = LwZctReadBufferIo(
                        pWriteState->zct.pZct,
                        pWriteState->pData,
                        pWriteState->zct.ulDataBytesResident,
                        &pWriteState->ulBytesWritten,
                        NULL);
        LWIO_ASSERT(STATUS_MORE_PROCESSING_REQUIRED != ntStatus);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_ASSERT(pWriteState->ulBytesWritten == pWriteState->zct.ulDataBytesResident);

        pWriteState->zct.ulDataBytesResident = 0;
    }

    LWIO_ASSERT(pWriteState->zct.ulDataBytesMissing > 0);

    pZctContext = SrvAcquireExecContext(pExecContext);

    // Do ZCT read from the transport for remaining bytes
    ntStatus = SrvProtocolTransportReceiveZct(
                    pExecContext->pConnection,
                    pWriteState->zct.pZct,
                    SrvExecuteWriteXReceiveZctCB,
                    pZctContext);
    BAIL_ON_NT_STATUS(ntStatus);

    // completed synchronously
    SrvReleaseExecContext(pZctContext);

    pWriteState->ulBytesWritten += pWriteState->zct.ulDataBytesMissing - pWriteState->zct.ulPaddingSize;

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
SrvExecuteWriteXZctComplete(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pWriteState->bStartedIo)
    {
        SrvPrepareWriteXStateAsync(pWriteState, pExecContext);
        pWriteState->bStartedIo = TRUE;

        ntStatus = IoCompleteZctWriteFile(
                        pWriteState->pFile->hFile,
                        pWriteState->pAcb,
                        &pWriteState->ioStatusBlock,
                        0,
                        pWriteState->zct.pZctCompletion,
                        pWriteState->ulBytesWritten);
        BAIL_ON_NT_STATUS(ntStatus);

        // completed synchronously
        SrvReleaseWriteXStateAsync(pWriteState);
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
        SrvReleaseWriteXStateAsync(pWriteState);
        pWriteState->bStartedIo = FALSE;
    }

    goto cleanup;
}

static
VOID
SrvPrepareWriteXStateAsync(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState,
    PSRV_EXEC_CONTEXT        pExecContext
    )
{
    pWriteState->acb.Callback        = &SrvExecuteWriteXAsyncCB;

    pWriteState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pWriteState->acb.AsyncCancelContext = NULL;

    pWriteState->pAcb = &pWriteState->acb;
}

static
VOID
SrvExecuteWriteXAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_WRITEX_STATE_SMB_V1   pWriteState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pWriteState =
        (PSRV_WRITEX_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

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
SrvExecuteWriteXReceiveZctCB(
    IN PVOID pContext,
    IN NTSTATUS Status
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_WRITEX_STATE_SMB_V1   pWriteState       = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pWriteState =
        (PSRV_WRITEX_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pWriteState->mutex);
    pWriteState->stage = SRV_WRITEX_STAGE_SMB_V1_ZCT_COMPLETE;
    if (Status == STATUS_SUCCESS)
    {
        pWriteState->ulBytesWritten += pWriteState->zct.ulDataBytesMissing - pWriteState->zct.ulPaddingSize;
    }
    LWIO_UNLOCK_MUTEX(bInLock, &pWriteState->mutex);

    LWIO_LOG_DEBUG("queue exec context = %p (conn = %p)", pContext, pExecContext->pConnection);

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
SrvReleaseWriteXStateAsync(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState
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
SrvBuildWriteAndXResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus     = 0;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                       iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1         pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1         pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PWRITE_ANDX_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG ulTotalBytesUsed     = 0;
    PSRV_WRITEX_STATE_SMB_V1 pWriteState = NULL;

    pWriteState = (PSRV_WRITEX_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_WRITE_ANDX,
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
                        COM_WRITE_ANDX,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 6;

    if (ulBytesAvailable < sizeof(WRITE_ANDX_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PWRITE_ANDX_RESPONSE_HEADER)pOutBuffer;

    // pOutBuffer       += sizeof(WRITE_ANDX_RESPONSE_HEADER);
    // ulOffset         += sizeof(WRITE_ANDX_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(WRITE_ANDX_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(WRITE_ANDX_RESPONSE_HEADER);

    pResponseHeader->remaining = 0;
    pResponseHeader->reserved = 0;
    pResponseHeader->count =
                    (USHORT) (pWriteState->ulBytesWritten & 0xFFFF);
    pResponseHeader->countHigh =
                    (USHORT) (pWriteState->ulBytesWritten >> 16);

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

VOID
SrvReleaseWriteXStateHandle(
    HANDLE hWriteState
    )
{
    SrvReleaseWriteXState((PSRV_WRITEX_STATE_SMB_V1)hWriteState);
}

static
VOID
SrvReleaseWriteXState(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState
    )
{
    if (InterlockedDecrement(&pWriteState->refCount) == 0)
    {
        SrvFreeWriteXState(pWriteState);
    }
}

static
VOID
SrvFreeWriteXState(
    PSRV_WRITEX_STATE_SMB_V1 pWriteState
    )
{
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



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
