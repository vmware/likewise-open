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
SrvBuildCloseState(
    PCLOSE_REQUEST_HEADER    pRequestHeader,
    PLWIO_SRV_FILE           pFile,
    PSRV_CLOSE_STATE_SMB_V1* ppCloseState
    );

static
NTSTATUS
SrvBuildCloseResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareCloseStateAsync(
    PSRV_CLOSE_STATE_SMB_V1 pCloseState,
    PSRV_EXEC_CONTEXT       pExecContext
    );

static
VOID
SrvExecuteCloseAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseCloseStateAsync(
    PSRV_CLOSE_STATE_SMB_V1 pCloseState
    );

static
VOID
SrvReleaseCloseStateHandle(
    HANDLE hState
    );

static
VOID
SrvReleaseCloseState(
    PSRV_CLOSE_STATE_SMB_V1 pCloseState
    );

static
VOID
SrvFreeCloseState(
    PSRV_CLOSE_STATE_SMB_V1 pCloseState
    );

NTSTATUS
SrvProcessCloseAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;
    PLWIO_SRV_FILE             pFile        = NULL;
    PSRV_CLOSE_STATE_SMB_V1    pCloseState  = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pCloseState = (PSRV_CLOSE_STATE_SMB_V1)pCtxSmb1->hState;
    if (pCloseState)
    {
        InterlockedIncrement(&pCloseState->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PCLOSE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PSRV_OPLOCK_STATE_SMB_V1 pOplockState = NULL;

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

        ntStatus = WireUnmarshallCloseRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvTreeFindFile_SMB_V1(
                        pCtxSmb1,
                        pTree,
                        pRequestHeader->fid,
                        &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        pOplockState =
            (PSRV_OPLOCK_STATE_SMB_V1)SrvFileRemoveOplockState(pFile);

        /* Deal with any outstanding oplock issues */

        if (pOplockState)
        {
            /* This is an implicit acknowlegdement to any outstanding
               break requests we sent the client */

            if (pOplockState->bBreakRequestSent)
            {
                PSRV_OPLOCK_STATE_SMB_V1 pOplockState2 = NULL;

                if (pOplockState->pTimerRequest)
                {
                    SrvTimerCancelRequest(
                        pOplockState->pTimerRequest,
                        (PVOID*)&pOplockState2);

                    SrvTimerRelease(pOplockState->pTimerRequest);
                    pOplockState->pTimerRequest = NULL;
                }

                ntStatus = SrvAcknowledgeOplockBreak(pOplockState, TRUE);
                BAIL_ON_NT_STATUS(ntStatus);

                SrvReleaseOplockState(pOplockState);
                pOplockState = NULL;

            }
            else
            {
                /* Cancel any registered oplock on the file */
                if (pOplockState->acb.AsyncCancelContext)
                {
                    IoCancelAsyncCancelContext(
                        pOplockState->acb.AsyncCancelContext);
                }
            }
        }

        ntStatus = SrvBuildCloseState(
                        pRequestHeader,
                        pFile,
                        &pCloseState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pCloseState;
        InterlockedIncrement(&pCloseState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseCloseStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pCloseState->mutex);

    switch (pCloseState->stage)
    {
        case SRV_CLOSE_STAGE_SMB_V1_INITIAL:

            pCloseState->stage = SRV_CLOSE_STAGE_SMB_V1_SET_INFO_COMPLETED;

            switch (pCloseState->pRequestHeader->ulLastWriteTime)
            {
                case 0:
                case (ULONG)-1:

                    break;

                default:

                    pCloseState->fileBasicInfo.LastWriteTime =
                        (pCloseState->pRequestHeader->ulLastWriteTime +
                         11644473600LL) * 10000000LL;

                    SrvPrepareCloseStateAsync(pCloseState, pExecContext);

                    ntStatus = IoSetInformationFile(
                                    pCloseState->pFile->hFile,
                                    pCloseState->pAcb,
                                    &pCloseState->ioStatusBlock,
                                    &pCloseState->fileBasicInfo,
                                    sizeof(pCloseState->fileBasicInfo),
                                    FileBasicInformation);
                    if (ntStatus == STATUS_PENDING)
                    {
                        BAIL_ON_NT_STATUS(ntStatus);
                    }

                    SrvReleaseCloseStateAsync(pCloseState);

                    break;
            }

            // intentional fall through

        case SRV_CLOSE_STAGE_SMB_V1_SET_INFO_COMPLETED:

            switch (pCloseState->ioStatusBlock.Status)
            {
                case STATUS_SUCCESS:

                    break;

                default:

                    LWIO_LOG_ERROR(
                        "Failed to set the last write time for file "
                        " [fid:%u][code:%d]",
                        pCloseState->pFile->fid,
                        pCloseState->ioStatusBlock.Status);

                    break;
            }

            pCloseState->stage = SRV_CLOSE_STAGE_SMB_V1_ATTEMPT_CLOSE;

            // intentional fall through

        case SRV_CLOSE_STAGE_SMB_V1_ATTEMPT_CLOSE:

            SrvFileResetOplockState(pCloseState->pFile);

            ntStatus = SrvTreeRemoveFile(
                            pCtxSmb1->pTree,
                            pCloseState->pFile->fid);
            BAIL_ON_NT_STATUS(ntStatus);

            pCloseState->stage = SRV_CLOSE_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_CLOSE_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildCloseResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCloseState->stage = SRV_CLOSE_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_CLOSE_STAGE_SMB_V1_DONE:

            if (pCtxSmb1->pFile)
            {
                SrvFileRelease(pCtxSmb1->pFile);
                pCtxSmb1->pFile = NULL;
            }

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

    if (pCloseState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pCloseState->mutex);

        SrvReleaseCloseState(pCloseState);
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

            if (pCloseState)
            {
                SrvReleaseCloseStateAsync(pCloseState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildCloseState(
    PCLOSE_REQUEST_HEADER    pRequestHeader,
    PLWIO_SRV_FILE           pFile,
    PSRV_CLOSE_STATE_SMB_V1* ppCloseState
    )
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PSRV_CLOSE_STATE_SMB_V1 pCloseState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_CLOSE_STATE_SMB_V1),
                    (PVOID*)&pCloseState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCloseState->refCount = 1;

    pthread_mutex_init(&pCloseState->mutex, NULL);
    pCloseState->pMutex = &pCloseState->mutex;

    pCloseState->stage = SRV_CLOSE_STAGE_SMB_V1_INITIAL;

    pCloseState->pRequestHeader = pRequestHeader;

    pCloseState->pFile          = pFile;
    InterlockedIncrement(&pFile->refcount);

    *ppCloseState = pCloseState;

cleanup:

    return ntStatus;

error:

    *ppCloseState = NULL;

    if (pCloseState)
    {
        SrvFreeCloseState(pCloseState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildCloseResponse(
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
    PCLOSE_RESPONSE_HEADER     pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG ulTotalBytesUsed     = 0;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_CLOSE,
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
                        COM_CLOSE,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 0;

    if (ulBytesAvailable < sizeof(CLOSE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PCLOSE_RESPONSE_HEADER)pOutBuffer;

    // pOutBuffer       += sizeof(CLOSE_RESPONSE_HEADER);
    // ulOffset         += sizeof(CLOSE_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(CLOSE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(CLOSE_RESPONSE_HEADER);

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
SrvPrepareCloseStateAsync(
    PSRV_CLOSE_STATE_SMB_V1 pCloseState,
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    pCloseState->acb.Callback        = &SrvExecuteCloseAsyncCB;

    pCloseState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pCloseState->acb.AsyncCancelContext = NULL;

    pCloseState->pAcb = &pCloseState->acb;
}

static
VOID
SrvExecuteCloseAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_CLOSE_STATE_SMB_V1    pCloseState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pCloseState =
            (PSRV_CLOSE_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pCloseState->mutex);

    if (pCloseState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pCloseState->pAcb->AsyncCancelContext);
    }

    pCloseState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pCloseState->mutex);

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
SrvReleaseCloseStateAsync(
    PSRV_CLOSE_STATE_SMB_V1 pCloseState
    )
{
    if (pCloseState->pAcb)
    {
        pCloseState->acb.Callback       = NULL;

        if (pCloseState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext =
                    (PSRV_EXEC_CONTEXT)pCloseState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pCloseState->pAcb->CallbackContext = NULL;
        }

        if (pCloseState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pCloseState->pAcb->AsyncCancelContext);
        }

        pCloseState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseCloseStateHandle(
    HANDLE hState
    )
{
    SrvReleaseCloseState((PSRV_CLOSE_STATE_SMB_V1)hState);
}

static
VOID
SrvReleaseCloseState(
    PSRV_CLOSE_STATE_SMB_V1 pCloseState
    )
{
    if (InterlockedDecrement(&pCloseState->refCount) == 0)
    {
        SrvFreeCloseState(pCloseState);
    }
}

static
VOID
SrvFreeCloseState(
    PSRV_CLOSE_STATE_SMB_V1 pCloseState
    )
{
    if (pCloseState->pAcb && pCloseState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pCloseState->pAcb->AsyncCancelContext);
    }

    if (pCloseState->pFile)
    {
        SrvFileRelease(pCloseState->pFile);
    }

    if (pCloseState->pMutex)
    {
        pthread_mutex_destroy(&pCloseState->mutex);
    }

    SrvFreeMemory(pCloseState);
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
