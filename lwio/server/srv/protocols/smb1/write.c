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

            pWriteState->ulDataOffset = pWriteState->pRequestHeader->offset;
            pWriteState->llDataOffset = pWriteState->ulDataOffset;
            pWriteState->usDataLength = pWriteState->pRequestHeader->dataLength;

            pWriteState->ulKey = pSmbRequest->pHeader->pid;

            pWriteState->stage = SRV_WRITE_STAGE_SMB_V1_ATTEMPT_WRITE;

            // intentional fall through

        case SRV_WRITE_STAGE_SMB_V1_ATTEMPT_WRITE:

            ntStatus = SrvExecuteWrite(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

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

    pWriteState->pRequestHeader = pRequestHeader;
    pWriteState->pData          = pData;
    pWriteState->pFile          = pFile;
    InterlockedIncrement(&pFile->refcount);

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
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_WRITE_STATE_SMB_V1    pWriteState  = NULL;

    pWriteState = (PSRV_WRITE_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = pWriteState->ioStatusBlock.Status; // async response status
    BAIL_ON_NT_STATUS(ntStatus);

    if (pWriteState->pData)
    {
        pWriteState->usBytesWritten +=
                        pWriteState->ioStatusBlock.BytesTransferred;
        pWriteState->usDataLength -=
                        pWriteState->ioStatusBlock.BytesTransferred;

        if (pWriteState->usDataLength > 0)
        {
            SrvPrepareWriteStateAsync(pWriteState, pExecContext);

            ntStatus = IoWriteFile(
                            pWriteState->pFile->hFile,
                            pWriteState->pAcb,
                            &pWriteState->ioStatusBlock,
                            pWriteState->pData + pWriteState->usBytesWritten,
                            pWriteState->usDataLength,
                            &pWriteState->llDataOffset,
                            &pWriteState->ulKey);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseWriteStateAsync(pWriteState); // completed synchronously

            pWriteState->usBytesWritten =
                    pWriteState->ioStatusBlock.BytesTransferred;
        }
    }
    else
    {
        if (!pWriteState->pFileEofInfo)
        {
            pWriteState->fileEofInfo.EndOfFile = pWriteState->llDataOffset;
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
    }

cleanup:

    return ntStatus;

error:

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

    pResponseHeader->count = pWriteState->usBytesWritten;
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
