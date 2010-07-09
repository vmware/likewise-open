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
 *        flush.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Flush
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildFlushState_SMB_V2(
    PSMB2_FID                pFid,
    PLWIO_SRV_FILE_2         pFile,
    PSRV_FLUSH_STATE_SMB_V2* ppFlushState
    );

static
NTSTATUS
SrvBuildFlushResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareFlushStateAsync_SMB_V2(
    PSRV_FLUSH_STATE_SMB_V2 pFlushState,
    PSRV_EXEC_CONTEXT       pExecContext
    );

static
VOID
SrvExecuteFlushAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseFlushStateAsync_SMB_V2(
    PSRV_FLUSH_STATE_SMB_V2 pFlushState
    );

static
VOID
SrvReleaseFlushStateHandle_SMB_V2(
    HANDLE hState
    );

static
VOID
SrvReleaseFlushState_SMB_V2(
    PSRV_FLUSH_STATE_SMB_V2 pFlushState
    );

static
VOID
SrvFreeFlushState_SMB_V2(
    PSRV_FLUSH_STATE_SMB_V2 pFlushState
    );

NTSTATUS
SrvProcessFlush_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PSRV_FLUSH_STATE_SMB_V2    pFlushState  = NULL;
    PLWIO_SRV_SESSION_2        pSession     = NULL;
    PLWIO_SRV_TREE_2           pTree        = NULL;
    PLWIO_SRV_FILE_2           pFile        = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pFlushState = (PSRV_FLUSH_STATE_SMB_V2)pCtxSmb2->hState;

    if (pFlushState)
    {
        InterlockedIncrement(&pFlushState->refCount);
    }
    else
    {
        ULONG               iMsg          = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2 pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
        PSMB2_FID           pFid = NULL; // Do not free

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

        ntStatus = SMB2UnmarshalFlushRequest(pSmbRequest, &pFid);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_DEBUG(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_2,
                pSmbRequest->pHeader->command,
                "Flush request params: file-id(persistent:0x%x,volatile:0x%x)",
                (long long)pFid->ullPersistentId,
                (long long)pFid->ullVolatileId);

        ntStatus = SrvTree2FindFile_SMB_V2(
                        pCtxSmb2,
                        pTree,
                        pFid,
                        &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildFlushState_SMB_V2(
                        pFid,
                        pFile,
                        &pFlushState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pFlushState;
        InterlockedIncrement(&pFlushState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseFlushStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pFlushState->mutex);

    switch (pFlushState->stage)
    {
        case SRV_FLUSH_STAGE_SMB_V2_INITIAL:

            pFlushState->stage = SRV_FLUSH_STAGE_SMB_V2_FLUSH_COMPLETED;

            SrvPrepareFlushStateAsync_SMB_V2(pFlushState, pExecContext);

            ntStatus = IoFlushBuffersFile(
                            pFlushState->pFile->hFile,
                            pFlushState->pAcb,
                            &pFlushState->ioStatusBlock);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseFlushStateAsync_SMB_V2(pFlushState); // completed synchronously

            // intentional fall through

        case SRV_FLUSH_STAGE_SMB_V2_FLUSH_COMPLETED:

            ntStatus = pFlushState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pFlushState->stage = SRV_FLUSH_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_FLUSH_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildFlushResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pFlushState->stage = SRV_FLUSH_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_FLUSH_STAGE_SMB_V2_DONE:

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

    if (pFlushState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pFlushState->mutex);

        SrvReleaseFlushState_SMB_V2(pFlushState);
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
                SrvReleaseFlushStateAsync_SMB_V2(pFlushState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFlushState_SMB_V2(
    PSMB2_FID                pFid,
    PLWIO_SRV_FILE_2         pFile,
    PSRV_FLUSH_STATE_SMB_V2* ppFlushState
    )
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PSRV_FLUSH_STATE_SMB_V2 pFlushState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_FLUSH_STATE_SMB_V2),
                    (PVOID*)&pFlushState);
    BAIL_ON_NT_STATUS(ntStatus);

    pFlushState->refCount = 1;

    pthread_mutex_init(&pFlushState->mutex, NULL);
    pFlushState->pMutex = &pFlushState->mutex;

    pFlushState->stage = SRV_FLUSH_STAGE_SMB_V2_INITIAL;

    pFlushState->pFid  = pFid;
    pFlushState->pFile = SrvFile2Acquire(pFile);

    *ppFlushState = pFlushState;

cleanup:

    return ntStatus;

error:

    *ppFlushState = NULL;

    if (pFlushState)
    {
        SrvFreeFlushState_SMB_V2(pFlushState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFlushResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset    = 0;
    ULONG ulBytesUsed = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_FLUSH,
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

    ntStatus = SMB2MarshalFlushResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
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
        pSmbResponse->pHeader       = NULL;
        pSmbResponse->ulMessageSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
VOID
SrvPrepareFlushStateAsync_SMB_V2(
    PSRV_FLUSH_STATE_SMB_V2 pFlushState,
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    pFlushState->acb.Callback        = &SrvExecuteFlushAsyncCB_SMB_V2;

    pFlushState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pFlushState->acb.AsyncCancelContext = NULL;

    pFlushState->pAcb = &pFlushState->acb;
}

static
VOID
SrvExecuteFlushAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_FLUSH_STATE_SMB_V2    pFlushState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pFlushState =
            (PSRV_FLUSH_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pFlushState->mutex);

    if (pFlushState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pFlushState->pAcb->AsyncCancelContext);
    }

    pFlushState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pFlushState->mutex);

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
SrvReleaseFlushStateAsync_SMB_V2(
    PSRV_FLUSH_STATE_SMB_V2 pFlushState
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
SrvReleaseFlushStateHandle_SMB_V2(
    HANDLE hState
    )
{
    SrvReleaseFlushState_SMB_V2((PSRV_FLUSH_STATE_SMB_V2)hState);
}

static
VOID
SrvReleaseFlushState_SMB_V2(
    PSRV_FLUSH_STATE_SMB_V2 pFlushState
    )
{
    if (InterlockedDecrement(&pFlushState->refCount) == 0)
    {
        SrvFreeFlushState_SMB_V2(pFlushState);
    }
}

static
VOID
SrvFreeFlushState_SMB_V2(
    PSRV_FLUSH_STATE_SMB_V2 pFlushState
    )
{
    if (pFlushState->pAcb && pFlushState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pFlushState->pAcb->AsyncCancelContext);
    }

    if (pFlushState->pFile)
    {
        SrvFile2Release(pFlushState->pFile);
    }

    if (pFlushState->pMutex)
    {
        pthread_mutex_destroy(&pFlushState->mutex);
    }

    SrvFreeMemory(pFlushState);
}
