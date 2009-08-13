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
SrvBuildWriteXState(
    PWRITE_ANDX_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_SRV_FILE             pFile,
    PSRV_WRITEX_STATE_SMB_V1*  ppWriteState
    );

static
NTSTATUS
SrvExecuteWriteAndX(
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
SrvReleaseWriteXStateHandle(
    HANDLE hWriteState
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
        PWRITE_ANDX_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PBYTE                      pData          = NULL; // Do not free

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

        ntStatus = WireUnmarshallWriteAndXRequest(
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

        ntStatus = SrvBuildWriteXState(
                        pRequestHeader,
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

            pWriteState->llDataOffset =
                (((LONG64)pWriteState->pRequestHeader->offsetHigh) << 32) |
                ((LONG64)pWriteState->pRequestHeader->offset);

            pWriteState->llDataLength =
                (((LONG64)pWriteState->pRequestHeader->dataLengthHigh) << 32) |
                ((LONG64)pWriteState->pRequestHeader->dataLength);

            pWriteState->ulKey = pSmbRequest->pHeader->pid;

            pWriteState->stage = SRV_WRITEX_STAGE_SMB_V1_ATTEMPT_WRITE;

            // intentional fall through

        case SRV_WRITEX_STAGE_SMB_V1_ATTEMPT_WRITE:

            ntStatus = SrvExecuteWriteAndX(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pWriteState->stage = SRV_WRITEX_STAGE_BUILD_RESPONSE;

            // intentional fall through

        case SRV_WRITEX_STAGE_BUILD_RESPONSE:

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

static
NTSTATUS
SrvBuildWriteXState(
    PWRITE_ANDX_REQUEST_HEADER pRequestHeader,
    PBYTE                      pData,
    PLWIO_SRV_FILE             pFile,
    PSRV_WRITEX_STATE_SMB_V1*  ppWriteState
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
        SrvFreeWriteXState(pWriteState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteWriteAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_WRITEX_STATE_SMB_V1   pWriteState  = NULL;

    pWriteState = (PSRV_WRITEX_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = pWriteState->ioStatusBlock.Status; // async response status
    BAIL_ON_NT_STATUS(ntStatus);

    pWriteState->llDataLength -=
                pWriteState->ioStatusBlock.BytesTransferred;
    pWriteState->ullBytesWritten +=
                pWriteState->ioStatusBlock.BytesTransferred;

    while (pWriteState->llDataLength > 0)
    {
        ULONG ulDataLength = 0;

        if (pWriteState->llDataLength > UINT32_MAX)
        {
            ulDataLength = UINT32_MAX;
        }
        else
        {
            ulDataLength = (ULONG)pWriteState->llDataLength;
        }

        SrvPrepareWriteXStateAsync(pWriteState, pExecContext);

        ntStatus = IoWriteFile(
                        pWriteState->pFile->hFile,
                        pWriteState->pAcb,
                        &pWriteState->ioStatusBlock,
                        pWriteState->pData + pWriteState->ullBytesWritten,
                        ulDataLength,
                        &pWriteState->llDataOffset,
                        &pWriteState->ulKey);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseWriteXStateAsync(pWriteState); // completed synchronously

        pWriteState->llDataLength -=
                    pWriteState->ioStatusBlock.BytesTransferred;
        pWriteState->ullBytesWritten +=
                    pWriteState->ioStatusBlock.BytesTransferred;
    }

cleanup:

    return ntStatus;

error:

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
                    (pWriteState->ullBytesWritten & 0x00000000FFFFFFFFLL);
    pResponseHeader->countHigh =
                    (pWriteState->ullBytesWritten & 0xFFFFFFFF00000000LL) >> 32;

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
