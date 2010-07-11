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
SrvBuildReadAndXResponseStart(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildReadAndXResponseFinish(
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
NTSTATUS
SrvBuildZctReadState(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvAttemptReadIo(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSendZctReadResponse(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvCompleteZctRead(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
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
SrvExecuteReadSendZctCB(
    IN PVOID pContext,
    IN NTSTATUS Status
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
    UINT16                 usFlags2 = 0;

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

        ntStatus = SrvSetStatSessionInfo(pExecContext, pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSessionFindTree_SMB_V1(
                        pCtxSmb1,
                        pSession,
                        pSmbRequest->pHeader->tid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        usFlags2 = pSmbRequest->pHeader->flags2;

        switch (*pSmbRequest->pWordCount)
        {
            case 10:

                ntStatus = WireUnmarshallReadAndXRequest_WC_10(
                                pBuffer,
                                ulBytesAvailable,
                                ulOffset,
                                &pRequestHeader_WC_10);
                BAIL_ON_NT_STATUS(ntStatus);

                SRV_LOG_DEBUG(
                        pExecContext->pLogContext,
                        SMB_PROTOCOL_VERSION_1,
                        pSmbRequest->pHeader->command,
                        "ReadAndX(WC10) request params: "
                        "file-id(%u),min-count(%u),"
                        "max-count-high(%u),max-count(%u),offset(%u)",
                        pRequestHeader_WC_10->fid,
                        pRequestHeader_WC_10->minCount,
                        pRequestHeader_WC_10->maxCountHigh,
                        pRequestHeader_WC_10->maxCount,
                        pRequestHeader_WC_10->offset);

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

                SRV_LOG_DEBUG(
                        pExecContext->pLogContext,
                        SMB_PROTOCOL_VERSION_1,
                        pSmbRequest->pHeader->command,
                        "ReadAndX(WC12) request params: "
                        "file-id(%u),min-count(%u),"
                        "max-count-high(%u),max-count(%u),"
                        "offset-high(%u),offset(%u)",
                        pRequestHeader_WC_12->fid,
                        pRequestHeader_WC_12->minCount,
                        pRequestHeader_WC_12->maxCountHigh,
                        pRequestHeader_WC_12->maxCount,
                        pRequestHeader_WC_12->offsetHigh,
                        pRequestHeader_WC_12->offset);

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
                        *pSmbRequest->pWordCount,
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

            if (pReadState->pRequestHeader_WC_12->maxCountHigh &&
                (pReadState->pRequestHeader_WC_12->maxCountHigh != UINT32_MAX))
            {
                pReadState->ullBytesToRead =
                    ((ULONG64)pReadState->pRequestHeader_WC_12->maxCountHigh) << 16;
            }

            pReadState->ullBytesToRead |=
                (ULONG64)pReadState->pRequestHeader_WC_12->maxCount;

            pReadState->bPagedIo = usFlags2 & FLAG2_PAGING_IO ? TRUE : FALSE;

            ntStatus = SrvBuildZctReadState(pReadState, pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pReadState->stage = SRV_READ_STAGE_SMB_V1_RESPONSE_START;

            // Intentional fall through

        case SRV_READ_STAGE_SMB_V1_RESPONSE_START:

            ntStatus = SrvBuildReadAndXResponseStart(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pReadState->stage = SRV_READ_STAGE_SMB_V1_ATTEMPT_READ;

            // Intentional fall through

        case SRV_READ_STAGE_SMB_V1_ATTEMPT_READ:

            ntStatus = SrvAttemptReadIo(pReadState, pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pReadState->stage = SRV_READ_STAGE_SMB_V1_RESPONSE_FINISH;

            // intentional fall through

        case SRV_READ_STAGE_SMB_V1_RESPONSE_FINISH:

            ntStatus = SrvBuildReadAndXResponseFinish(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            if (pReadState->pZct)
            {
                ntStatus = SrvSendZctReadResponse(pReadState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pReadState->stage = SRV_READ_STAGE_SMB_V1_ZCT_COMPLETE;

            // Intentional fall through

        case SRV_READ_STAGE_SMB_V1_ZCT_COMPLETE:

            if (pReadState->pZctCompletion)
            {
                ntStatus = SrvCompleteZctRead(pReadState, pExecContext);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pReadState->stage = SRV_READ_STAGE_SMB_V1_DONE;

            // Intentional fall through

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

    if (ntStatus != STATUS_PENDING)
    {
        if (pReadState)
        {
            SrvReleaseReadStateAsync(pReadState);
        }
    }

    goto cleanup;
}


static
NTSTATUS
SrvBuildReadAndXResponseStart(
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
    pReadState->pResponseHeader->dataLength =
        (pReadState->ulBytesRead & 0x000000000000FFFFLL);
    pReadState->pResponseHeader->dataLengthHigh =
        (pReadState->ulBytesRead & 0xFFFFFFFFFFFF0000LL) >> 16;

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

    if (pReadState->ullBytesToRead > UINT32_MAX) {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pReadState->ulBytesToRead = pReadState->ullBytesToRead;
    pReadState->ulKey = pSmbRequest->pHeader->pid;

    if (pReadState->ulBytesToRead > 0)
    {
        ntStatus = SrvAllocateMemory(
            pReadState->ulBytesToRead,
            (PVOID*)&pReadState->pData);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildReadAndXResponseFinish(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PSRV_READ_STATE_SMB_V1     pReadState   = NULL;

    pReadState = (PSRV_READ_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = WireMarshallReadResponseDataEx(
                    pReadState->pOutBuffer,
                    pReadState->ulBytesAvailable,
                    pReadState->ulOffset,
                    pReadState->pZct ? NULL : pReadState->pData,
                    pReadState->ulBytesRead,
                    &pReadState->ulDataOffset,
                    &pReadState->ulPackageByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    pReadState->pResponseHeader->dataLength =
        (pReadState->ulBytesRead & 0x000000000000FFFFLL);
    pReadState->pResponseHeader->dataLengthHigh =
        (pReadState->ulBytesRead & 0xFFFFFFFFFFFF0000LL) >> 16;

    // The data offset will fit in 16 bits
    assert((pReadState->ulDataOffset + pSmbResponse->ulOffset) <= UINT16_MAX);
    pReadState->pResponseHeader->dataOffset =
      (USHORT)(pSmbResponse->ulOffset + pReadState->ulDataOffset);

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

    pReadState->pFile = SrvFileAcquire(pFile);

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
NTSTATUS
SrvBuildZctReadState(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG ulZctThreshold = 0;
    LW_ZCT_ENTRY_MASK zctReadMask = 0;
    LW_ZCT_ENTRY_MASK zctSocketMask = 0;

    // Do not use ZCT if chained or if signing active.
    if ((pCtxSmb1->ulNumRequests != 1) ||
        SrvConnectionIsSigningActive(pConnection))
    {
        goto cleanup;
    }

    // Do not use ZCT if threshold disabled or the I/O size < threshold.
    ulZctThreshold = pConnection->serverProperties.ulZctReadThreshold;
    if ((ulZctThreshold == 0) ||
        (pReadState->ullBytesToRead < ulZctThreshold))
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
SrvAttemptReadIo(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pReadState->pZct)
    {
        if (!pReadState->bStartedRead)
        {
            SrvPrepareReadStateAsync(pReadState, pExecContext);
            pReadState->bStartedRead = TRUE;

            ntStatus = IoPrepareZctReadFile(
                            pReadState->pFile->hFile,
                            pReadState->pAcb,
                            &pReadState->ioStatusBlock,
                            pReadState->bPagedIo ? IO_FLAG_PAGING_IO : 0,
                            pReadState->pZct,
                            pReadState->ulBytesToRead,
                            &pReadState->llByteOffset,
                            &pReadState->ulKey,
                            &pReadState->pZctCompletion);

            if (ntStatus == STATUS_NOT_SUPPORTED)
            {
                SrvReleaseReadStateAsync(pReadState); // Retry as non-ZCT
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
            SrvReleaseReadStateAsync(pReadState);
            pReadState->bStartedRead = FALSE;
        }

        ntStatus = pReadState->ioStatusBlock.Status;
        if (ntStatus == STATUS_NOT_SUPPORTED)
        {
            // Retry as non-ZCT
            SrvReleaseReadStateAsync(pReadState);
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
            SrvPrepareReadStateAsync(pReadState, pExecContext);
            pReadState->bStartedRead = TRUE;

            if (pReadState->bPagedIo)
            {
                ntStatus = IoPagingReadFile(
                                pReadState->pFile->hFile,
                                pReadState->pAcb,
                                &pReadState->ioStatusBlock,
                                pReadState->pData,
                                pReadState->ulBytesToRead,
                                &pReadState->llByteOffset,
                                &pReadState->ulKey);
            }
            else
            {
                ntStatus = IoReadFile(
                                pReadState->pFile->hFile,
                                pReadState->pAcb,
                                &pReadState->ioStatusBlock,
                                pReadState->pData,
                                pReadState->ulBytesToRead,
                                &pReadState->llByteOffset,
                                &pReadState->ulKey);
            }
            if (ntStatus == STATUS_END_OF_FILE)
            {
                pReadState->ioStatusBlock.Status = ntStatus = STATUS_SUCCESS;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            // completed synchronously
            SrvReleaseReadStateAsync(pReadState);
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
SrvSendZctReadResponse(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
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
                    SrvExecuteReadSendZctCB,
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
SrvCompleteZctRead(
    PSRV_READ_STATE_SMB_V1 pReadState,
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pReadState->bStartedRead)
    {
        SrvPrepareReadStateAsync(pReadState, pExecContext);
        pReadState->bStartedRead = TRUE;

        ntStatus = IoCompleteZctReadFile(
                        pReadState->pFile->hFile,
                        pReadState->pAcb,
                        &pReadState->ioStatusBlock,
                        pReadState->bPagedIo ? IO_FLAG_PAGING_IO : 0,
                        pReadState->pZctCompletion);
        BAIL_ON_NT_STATUS(ntStatus);

        // completed synchronously
        SrvReleaseReadStateAsync(pReadState);
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
        SrvReleaseReadStateAsync(pReadState);
        pReadState->bStartedRead = FALSE;
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
SrvExecuteReadSendZctCB(
    IN PVOID pContext,
    IN NTSTATUS Status
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
    pReadState->stage = SRV_READ_STAGE_SMB_V1_ZCT_COMPLETE;
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

    if (pReadState->pZct)
    {
        LwZctDestroy(&pReadState->pZct);
    }

    SrvFreeMemory(pReadState);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
