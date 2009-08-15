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
SrvBuildFlushState(
    PFLUSH_REQUEST_HEADER    pRequestHeader,
    PLWIO_SRV_FILE           pFile,
    PSRV_FLUSH_STATE_SMB_V1* ppFlushState
    );

static
NTSTATUS
SrvBuildFlushResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareFlushStateAsync(
    PSRV_FLUSH_STATE_SMB_V1 pFlushState,
    PSRV_EXEC_CONTEXT       pExecContext
    );

static
VOID
SrvExecuteFlushAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseFlushStateAsync(
    PSRV_FLUSH_STATE_SMB_V1 pFlushState
    );

static
VOID
SrvReleaseFlushStateHandle(
    HANDLE hState
    );

static
VOID
SrvReleaseFlushState(
    PSRV_FLUSH_STATE_SMB_V1 pFlushState
    );

static
VOID
SrvFreeFlushState(
    PSRV_FLUSH_STATE_SMB_V1 pFlushState
    );

NTSTATUS
SrvProcessFlush(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus       = 0;
    PLWIO_SRV_CONNECTION       pConnection    = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1       = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION          pSession       = NULL;
    PLWIO_SRV_TREE             pTree          = NULL;
    PLWIO_SRV_FILE             pFile          = NULL;
    PSRV_FLUSH_STATE_SMB_V1    pFlushState    = NULL;
    BOOLEAN                    bInLock        = FALSE;

    pFlushState = (PSRV_FLUSH_STATE_SMB_V1)pCtxSmb1->hState;
    if (pFlushState)
    {
        InterlockedIncrement(&pFlushState->refCount);
    }
    else
    {
        ULONG                      iMsg           = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1        pSmbRequest    = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PFLUSH_REQUEST_HEADER pRequestHeader = NULL; // Do not free

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

        ntStatus = WireUnmarshallFlushRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvTreeFindFile_SMB_V1(
                        pCtxSmb1,
                        pTree,
                        pRequestHeader->usFid,
                        &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildFlushState(
                        pRequestHeader,
                        pFile,
                        &pFlushState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pFlushState;
        InterlockedIncrement(&pFlushState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseFlushStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pFlushState->mutex);

    switch (pFlushState->stage)
    {
        case SRV_FLUSH_STAGE_SMB_V1_INITIAL:

            pFlushState->stage = SRV_FLUSH_STAGE_SMB_V1_FLUSH_COMPLETED;

            SrvPrepareFlushStateAsync(pFlushState, pExecContext);

            ntStatus = IoFlushBuffersFile(
                            pFlushState->pFile->hFile,
                            pFlushState->pAcb,
                            &pFlushState->ioStatusBlock);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseFlushStateAsync(pFlushState); // completed synchronously

            // intentional fall through

        case SRV_FLUSH_STAGE_SMB_V1_FLUSH_COMPLETED:

            ntStatus = pFlushState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pFlushState->stage = SRV_FLUSH_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_FLUSH_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildFlushResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pFlushState->stage = SRV_FLUSH_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_FLUSH_STAGE_SMB_V1_DONE:

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

    if (pFlushState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pFlushState->mutex);

        SrvReleaseFlushState(pFlushState);
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

            if (pFlushState)
            {
                SrvReleaseFlushStateAsync(pFlushState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFlushState(
    PFLUSH_REQUEST_HEADER    pRequestHeader,
    PLWIO_SRV_FILE           pFile,
    PSRV_FLUSH_STATE_SMB_V1* ppFlushState
    )
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PSRV_FLUSH_STATE_SMB_V1 pFlushState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_FLUSH_STATE_SMB_V1),
                    (PVOID*)&pFlushState);
    BAIL_ON_NT_STATUS(ntStatus);

    pFlushState->refCount = 1;

    pthread_mutex_init(&pFlushState->mutex, NULL);
    pFlushState->pMutex = &pFlushState->mutex;

    pFlushState->stage = SRV_FLUSH_STAGE_SMB_V1_INITIAL;

    pFlushState->pRequestHeader = pRequestHeader;
    pFlushState->pFile          = pFile;
    InterlockedIncrement(&pFile->refcount);

    *ppFlushState = pFlushState;

cleanup:

    return ntStatus;

error:

    *ppFlushState = NULL;

    if (pFlushState)
    {
        SrvFreeFlushState(pFlushState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFlushResponse(
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
    PFLUSH_RESPONSE_HEADER     pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    USHORT usBytesUsed     = 0;
    ULONG ulTotalBytesUsed = 0;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_FLUSH,
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
                        COM_FLUSH,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 2;

    ntStatus = WireMarshallFlushResponse(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pResponseHeader,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->usByteCount = 0;

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

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
SrvPrepareFlushStateAsync(
    PSRV_FLUSH_STATE_SMB_V1 pFlushState,
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    pFlushState->acb.Callback        = &SrvExecuteFlushAsyncCB;

    pFlushState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pFlushState->acb.AsyncCancelContext = NULL;

    pFlushState->pAcb = &pFlushState->acb;
}

static
VOID
SrvExecuteFlushAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_FLUSH_STATE_SMB_V1    pFlushState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pFlushState =
            (PSRV_FLUSH_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pFlushState->mutex);

    if (pFlushState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pFlushState->pAcb->AsyncCancelContext);
    }

    pFlushState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pFlushState->mutex);

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
SrvReleaseFlushStateAsync(
    PSRV_FLUSH_STATE_SMB_V1 pFlushState
    )
{
    if (pFlushState->pAcb)
    {
        pFlushState->acb.Callback       = NULL;

        if (pFlushState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext =
                    (PSRV_EXEC_CONTEXT)pFlushState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pFlushState->pAcb->CallbackContext = NULL;
        }

        if (pFlushState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pFlushState->pAcb->AsyncCancelContext);
        }

        pFlushState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseFlushStateHandle(
    HANDLE hState
    )
{
    SrvReleaseFlushState((PSRV_FLUSH_STATE_SMB_V1)hState);
}

static
VOID
SrvReleaseFlushState(
    PSRV_FLUSH_STATE_SMB_V1 pFlushState
    )
{
    if (InterlockedDecrement(&pFlushState->refCount) == 0)
    {
        SrvFreeFlushState(pFlushState);
    }
}

static
VOID
SrvFreeFlushState(
    PSRV_FLUSH_STATE_SMB_V1 pFlushState
    )
{
    if (pFlushState->pAcb && pFlushState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pFlushState->pAcb->AsyncCancelContext);
    }

    if (pFlushState->pFile)
    {
        SrvFileRelease(pFlushState->pFile);
    }

    if (pFlushState->pMutex)
    {
        pthread_mutex_destroy(&pFlushState->mutex);
    }

    SrvFreeMemory(pFlushState);
}

