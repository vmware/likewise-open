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
SrvBuildReadAndXResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildReadState(
    UCHAR                           ucWordCount,
    PREAD_ANDX_REQUEST_HEADER_WC_10 pRequestHeader_WC_10,
    PREAD_ANDX_REQUEST_HEADER_WC_12 pRequestHeader_WC_12,
    PLWIO_SRV_FILE                  pFile,
    PSRV_READ_STATE_SMB_V1*         ppReadState
    );

static
VOID
SrvPrepareReadStateAsync(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT      pExecContext
    );

static
VOID
SrvExecuteReadAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseReadStateAsync(
    PSRV_READ_STATE_SMB_V1 pReadState
    );

static
VOID
SrvReleaseReadStateHandle(
    HANDLE hReadState
    );

static
VOID
SrvReleaseReadState(
    PSRV_READ_STATE_SMB_V1 pReadState
    );

static
VOID
SrvFreeReadState(
    PSRV_READ_STATE_SMB_V1 pReadState
    );

NTSTATUS
SrvProcessReadAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION      pSession   = NULL;
    PLWIO_SRV_TREE         pTree      = NULL;
    PLWIO_SRV_FILE         pFile      = NULL;
    PSRV_READ_STATE_SMB_V1 pReadState = NULL;
    BOOLEAN                bInLock = FALSE;

    pReadState = (PSRV_READ_STATE_SMB_V1)pCtxSmb1->hState;
    if (pReadState)
    {
        InterlockedIncrement(&pReadState->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer  = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PREAD_ANDX_REQUEST_HEADER_WC_10 pRequestHeader_WC_10 = NULL; // Do not free
        PREAD_ANDX_REQUEST_HEADER_WC_12 pRequestHeader_WC_12 = NULL; // Do not free

        ntStatus = SrvConnectionFindSession_SMB_V1(
                        pCtxSmb1,
                        pConnection,
                        pSmbRequest->pHeader->uid,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSessionFindTree_SMB_V1(
                        pCtxSmb1,
                        pSession,
                        pSmbRequest->pHeader->tid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        switch (pSmbRequest->pHeader->wordCount)
        {
            case 10:

                ntStatus = WireUnmarshallReadAndXRequest_WC_10(
                                pBuffer,
                                ulBytesAvailable,
                                ulOffset,
                                &pRequestHeader_WC_10);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = SrvTreeFindFile_SMB_V1(
                                pCtxSmb1,
                                pTree,
                                pRequestHeader_WC_10->fid,
                                &pFile);
                BAIL_ON_NT_STATUS(ntStatus);

                break;

            case 12:

                ntStatus = WireUnmarshallReadAndXRequest_WC_12(
                                pBuffer,
                                ulBytesAvailable,
                                ulOffset,
                                &pRequestHeader_WC_12);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = SrvTreeFindFile_SMB_V1(
                                pCtxSmb1,
                                pTree,
                                pRequestHeader_WC_12->fid,
                                &pFile);
                BAIL_ON_NT_STATUS(ntStatus);

                break;

            default:

                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

        ntStatus = SrvBuildReadState(
                        pSmbRequest->pHeader->wordCount,
                        pRequestHeader_WC_10,
                        pRequestHeader_WC_12,
                        pFile,
                        &pReadState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pReadState;
        InterlockedIncrement(&pReadState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseReadStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pReadState->mutex);

    switch (pReadState->stage)
    {
        case SRV_READ_STAGE_SMB_V1_INITIAL:

            switch (pReadState->ucWordCount)
            {
                case 10:

                    pReadState->llByteOffset =
                        pReadState->pRequestHeader_WC_10->offset;

                    break;

                case 12:

                    pReadState->llByteOffset =
                        (((LONG64)pReadState->pRequestHeader_WC_12->offsetHigh) << 32) |
                        ((LONG64)pReadState->pRequestHeader_WC_12->offset);

                    break;

                default:

                    break;
            }

            pReadState->ullBytesToRead =
                (((ULONG64)pReadState->pRequestHeader_WC_12->maxCountHigh) << 32)|
                ((ULONG64)pReadState->pRequestHeader_WC_12->maxCount);

            pReadState->stage = SRV_READ_STAGE_SMB_V1_ATTEMPT_READ;

            // Intentional fall through

        case SRV_READ_STAGE_SMB_V1_ATTEMPT_READ:

            ntStatus = SrvBuildReadAndXResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pReadState->stage = SRV_READ_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_READ_STAGE_SMB_V1_DONE:

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

    if (pReadState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pReadState->mutex);

        SrvReleaseReadState(pReadState);
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

            if (pReadState)
            {
                SrvReleaseReadStateAsync(pReadState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildReadAndXResponse(
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
    PSRV_READ_STATE_SMB_V1     pReadState   = NULL;

    pReadState = (PSRV_READ_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pReadState->pResponseHeader)
    {
        pReadState->pOutBuffer       = pSmbResponse->pBuffer;
        pReadState->ulBytesAvailable = pSmbResponse->ulBytesAvailable;

        if (!pSmbResponse->ulSerialNum)
        {
            ntStatus = SrvMarshalHeader_SMB_V1(
                            pReadState->pOutBuffer,
                            pReadState->ulOffset,
                            pReadState->ulBytesAvailable,
                            COM_READ_ANDX,
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
                            pReadState->pOutBuffer,
                            pReadState->ulOffset,
                            pReadState->ulBytesAvailable,
                            COM_READ_ANDX,
                            &pSmbResponse->pWordCount,
                            &pSmbResponse->pAndXHeader,
                            &pSmbResponse->usHeaderSize);
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pReadState->pOutBuffer       += pSmbResponse->usHeaderSize;
        pReadState->ulOffset         += pSmbResponse->usHeaderSize;
        pReadState->ulBytesAvailable -= pSmbResponse->usHeaderSize;
        pReadState->ulTotalBytesUsed += pSmbResponse->usHeaderSize;

        *pSmbResponse->pWordCount = 12;

        if (pReadState->ulBytesAvailable < sizeof(READ_ANDX_RESPONSE_HEADER))
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pReadState->pResponseHeader =
                    (PREAD_ANDX_RESPONSE_HEADER)pReadState->pOutBuffer;

        pReadState->pOutBuffer       += sizeof(READ_ANDX_RESPONSE_HEADER);
        pReadState->ulOffset         += sizeof(READ_ANDX_RESPONSE_HEADER);
        pReadState->ulBytesAvailable -= sizeof(READ_ANDX_RESPONSE_HEADER);
        pReadState->ulTotalBytesUsed += sizeof(READ_ANDX_RESPONSE_HEADER);

        pReadState->pResponseHeader->remaining = -1;
        pReadState->pResponseHeader->reserved = 0;
        memset( (PBYTE)&pReadState->pResponseHeader->reserved2,
                0,
                sizeof(pReadState->pResponseHeader->reserved2));
        pReadState->pResponseHeader->dataCompactionMode = 0;
        pReadState->pResponseHeader->dataLength = pReadState->ulBytesRead;
        // TODO: For large cap files
        pReadState->pResponseHeader->dataLengthHigh = 0;

        // Estimate how much data can fit in message
        ntStatus = WireMarshallReadResponseDataEx(
                        pReadState->pOutBuffer,
                        pReadState->ulBytesAvailable,
                        pReadState->ulOffset,
                        pReadState->pData,
                        pReadState->ulBytesRead,
                        &pReadState->ulDataOffset,
                        &pReadState->ulPackageByteCount);
        BAIL_ON_NT_STATUS(ntStatus);

        // Allow for alignment bytes
        pReadState->ulDataOffset += pReadState->ulDataOffset % 2;

        pReadState->ulBytesToRead =
            SMB_MIN(pReadState->ullBytesToRead,
                    pConnection->serverProperties.MaxBufferSize - pReadState->ulDataOffset);
        pReadState->ulKey = pSmbRequest->pHeader->pid;

        ntStatus = SrvAllocateMemory(
                        pReadState->ulBytesToRead,
                        (PVOID*)&pReadState->pData);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvPrepareReadStateAsync(pReadState, pExecContext);

        ntStatus = IoReadFile(
                        pReadState->pFile->hFile,
                        pReadState->pAcb,
                        &pReadState->ioStatusBlock,
                        pReadState->pData,
                        pReadState->ulBytesToRead,
                        &pReadState->llByteOffset,
                        &pReadState->ulKey);
        switch (ntStatus)
        {
            case STATUS_SUCCESS:

                pReadState->ulBytesRead =
                                pReadState->ioStatusBlock.BytesTransferred;

                break;

            case STATUS_END_OF_FILE:

                pReadState->ulBytesRead = 0;

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);
        }

        SrvReleaseReadStateAsync(pReadState); // completed synchronously
    }
    else
    {
        ntStatus = pReadState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_SUCCESS:

                pReadState->ulBytesRead =
                                pReadState->ioStatusBlock.BytesTransferred;

                break;

            case STATUS_END_OF_FILE:

                pReadState->ulBytesRead = 0;

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    ntStatus = WireMarshallReadResponseDataEx(
                    pReadState->pOutBuffer,
                    pReadState->ulBytesAvailable,
                    pReadState->ulOffset,
                    pReadState->pData,
                    pReadState->ulBytesRead,
                    &pReadState->ulDataOffset,
                    &pReadState->ulPackageByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    pReadState->pResponseHeader->dataLength = pReadState->ulBytesRead;
    // TODO: For large cap files
    pReadState->pResponseHeader->dataLengthHigh = 0;

    // The data offset will fit in 16 bits
    assert(pReadState->ulDataOffset <= UINT16_MAX);
    pReadState->pResponseHeader->dataOffset = (USHORT)pReadState->ulDataOffset;

    assert(pReadState->ulPackageByteCount <= UINT16_MAX);
    pReadState->pResponseHeader->byteCount = (USHORT)pReadState->ulPackageByteCount;

    // pOutBuffer       += pReadState->ulPackageByteCount;
    // ulOffset         += pReadState->ulPackageByteCount;
    // ulBytesAvailable -= pReadState->ulPackageByteCount;
    pReadState->ulTotalBytesUsed += pReadState->ulPackageByteCount;

    pSmbResponse->ulMessageSize = pReadState->ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            break;

        default:

            if (pReadState->ulTotalBytesUsed)
            {
                pSmbResponse->pHeader = NULL;
                pSmbResponse->pAndXHeader = NULL;
                memset(pSmbResponse->pBuffer, 0, pReadState->ulTotalBytesUsed);
            }

            pSmbResponse->ulMessageSize = 0;

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildReadState(
    UCHAR                           ucWordCount,
    PREAD_ANDX_REQUEST_HEADER_WC_10 pRequestHeader_WC_10,
    PREAD_ANDX_REQUEST_HEADER_WC_12 pRequestHeader_WC_12,
    PLWIO_SRV_FILE                  pFile,
    PSRV_READ_STATE_SMB_V1*         ppReadState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_READ_STATE_SMB_V1 pReadState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_READ_STATE_SMB_V1),
                    (PVOID*)&pReadState);
    BAIL_ON_NT_STATUS(ntStatus);

    pReadState->refCount = 1;

    pthread_mutex_init(&pReadState->mutex, NULL);
    pReadState->pMutex = &pReadState->mutex;

    pReadState->stage = SRV_READ_STAGE_SMB_V1_INITIAL;

    pReadState->ucWordCount    = ucWordCount;

    switch (ucWordCount)
    {
        case 10:

            pReadState->pRequestHeader_WC_10 = pRequestHeader_WC_10;

            break;

        case 12:

            pReadState->pRequestHeader_WC_12 = pRequestHeader_WC_12;

            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    pReadState->pFile = pFile;
    InterlockedIncrement(&pFile->refcount);

    *ppReadState = pReadState;

cleanup:

    return ntStatus;

error:

    *ppReadState = NULL;

    if (pReadState)
    {
        SrvFreeReadState(pReadState);
    }

    goto cleanup;
}

static
VOID
SrvPrepareReadStateAsync(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT      pExecContext
    )
{
    pReadState->acb.Callback        = &SrvExecuteReadAsyncCB;

    pReadState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pReadState->acb.AsyncCancelContext = NULL;

    pReadState->pAcb = &pReadState->acb;
}

static
VOID
SrvExecuteReadAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_READ_STATE_SMB_V1     pReadState       = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pReadState =
        (PSRV_READ_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pReadState->mutex);

    if (pReadState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pReadState->pAcb->AsyncCancelContext);
    }

    pReadState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pReadState->mutex);

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
SrvReleaseReadStateAsync(
    PSRV_READ_STATE_SMB_V1 pReadState
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
SrvReleaseReadStateHandle(
    HANDLE hReadState
    )
{
    SrvReleaseReadState((PSRV_READ_STATE_SMB_V1)hReadState);
}

static
VOID
SrvReleaseReadState(
    PSRV_READ_STATE_SMB_V1 pReadState
    )
{
    if (InterlockedDecrement(&pReadState->refCount) == 0)
    {
        SrvFreeReadState(pReadState);
    }
}

static
VOID
SrvFreeReadState(
    PSRV_READ_STATE_SMB_V1 pReadState
    )
{
    if (pReadState->pAcb && pReadState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pReadState->pAcb->AsyncCancelContext);
    }

    if (pReadState->pFile)
    {
        SrvFileRelease(pReadState->pFile);
    }

    if (pReadState->pMutex)
    {
        pthread_mutex_destroy(&pReadState->mutex);
    }

    if (pReadState->pData)
    {
        SrvFreeMemory(pReadState->pData);
    }

    SrvFreeMemory(pReadState);
}

