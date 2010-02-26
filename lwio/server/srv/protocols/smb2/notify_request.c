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
 *        notify_request.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Change Notify Request
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildNotifyRequestState_SMB_V2(
    PSMB2_NOTIFY_CHANGE_HEADER         pRequestHeader,
    PSRV_NOTIFY_REQUEST_STATE_SMB_V2*  ppNotifyRequestState
    );

static
NTSTATUS
SrvExecuteChangeNotify_SMB_V2(
    PSRV_EXEC_CONTEXT        pExecContext,
    PSRV_NOTIFY_STATE_SMB_V2 pNotifyState
    );

static
NTSTATUS
SrvBuildNotifyResponse_SMB_V2(
    PSRV_EXEC_CONTEXT        pExecContext,
    PSRV_NOTIFY_STATE_SMB_V2 pNotifyState
    );

static
NTSTATUS
SrvMarshalNotifyResponse_SMB_V2(
    PBYTE  pNotifyResponse,
    ULONG  ulNotifyResponseLength,
    PBYTE* ppBuffer,
    PULONG pulBufferLength
    );

static
VOID
SrvReleaseNotifyRequestStateHandle_SMB_V2(
    HANDLE hNotifyRequest
    );

static
VOID
SrvReleaseNotifyRequestState_SMB_V2(
    PSRV_NOTIFY_REQUEST_STATE_SMB_V2 pNotifyRequestState
    );

static
VOID
SrvFreeNotifyRequestState_SMB_V2(
    PSRV_NOTIFY_REQUEST_STATE_SMB_V2 pNotifyRequestState
    );

NTSTATUS
SrvProcessNotify_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PLWIO_SRV_SESSION_2        pSession     = NULL;
    PLWIO_SRV_TREE_2           pTree        = NULL;
    PLWIO_SRV_FILE_2           pFile        = NULL;
    BOOLEAN                    bInLock      = FALSE;
    PSRV_NOTIFY_STATE_SMB_V2   pNotifyState = NULL;
    PLWIO_ASYNC_STATE          pAsyncState       = NULL;
    BOOLEAN                    bUnregisterAsync          = FALSE;
    PSRV_NOTIFY_REQUEST_STATE_SMB_V2 pNotifyRequestState = NULL;

    pNotifyRequestState = (PSRV_NOTIFY_REQUEST_STATE_SMB_V2)pCtxSmb2->hState;
    if (pNotifyRequestState)
    {
        InterlockedIncrement(&pNotifyRequestState->refCount);
    }
    else
    {
        PSMB2_NOTIFY_CHANGE_HEADER pRequestHeader = NULL; // Do not free

        ntStatus = SrvConnection2FindSession_SMB_V2(
                        pCtxSmb2,
                        pConnection,
                        pSmbRequest->pHeader->ullSessionId,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSession2FindTree_SMB_V2(
                        pCtxSmb2,
                        pSession,
                        pSmbRequest->pHeader->ulTid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMB2UnmarshalNotifyRequest(pSmbRequest, &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildNotifyRequestState_SMB_V2(
                        pRequestHeader,
                        &pNotifyRequestState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pNotifyRequestState;
        InterlockedIncrement(&pNotifyRequestState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseNotifyRequestStateHandle_SMB_V2;

        ntStatus = SrvTree2FindFile_SMB_V2(
                        pCtxSmb2,
                        pTree,
                        &pRequestHeader->fid,
                        LwIsSetFlag(
                            pSmbRequest->pHeader->ulFlags,
                            SMB2_FLAGS_RELATED_OPERATION),
                        &pNotifyRequestState->pFile);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_MUTEX(bInLock, &pNotifyRequestState->mutex);

    switch (pNotifyRequestState->stage)
    {
        case SRV_NOTIFY_STAGE_SMB_V2_INITIAL:

             if (pNotifyRequestState->pRequestHeader->ulOutputBufferLength >
                 SMB_CN_MAX_BUFFER_SIZE)
             {
                 ntStatus = STATUS_INVALID_BUFFER_SIZE;
                 BAIL_ON_NT_STATUS(ntStatus);
             }

             // A change notify request can occur as the only message in
             // a request, or the last in a chained request
             if (pCtxSmb2->iMsg != (pCtxSmb2->ulNumRequests - 1))
             {
                 ntStatus = STATUS_INTERNAL_ERROR;
                 BAIL_ON_NT_STATUS(ntStatus);
             }

             ntStatus = SrvSession2CreateAsyncState(
                             pCtxSmb2->pSession,
                             COM2_NOTIFY,
                             &SrvNotifyStateReleaseHandle_SMB_V2,
                             &pAsyncState);
             BAIL_ON_NT_STATUS(ntStatus);

             bUnregisterAsync = TRUE;

             ntStatus = SrvNotifyCreateState_SMB_V2(
                             pAsyncState->ullAsyncId,
                             pExecContext->pConnection,
                             pCtxSmb2->pSession,
                             pCtxSmb2->pTree,
                             pNotifyRequestState->pFile,
                             pSmbRequest->pHeader->usEpoch,
                             pSmbRequest->pHeader->ullCommandSequence,
                             pSmbRequest->pHeader->ulPid,
                             pNotifyRequestState->pRequestHeader->ulCompletionFilter,
                             (pNotifyRequestState->pRequestHeader->usFlags &
                                     SMB2_NOTIFY_FLAGS_WATCH_TREE),
                             pNotifyRequestState->pRequestHeader->ulOutputBufferLength,
                             &pNotifyState);
             BAIL_ON_NT_STATUS(ntStatus);

             pAsyncState->hAsyncState =
                                 SrvNotifyStateAcquire_SMB_V2(pNotifyState);

             pNotifyRequestState->stage = SRV_NOTIFY_STAGE_SMB_V2_ATTEMPT_IO;

             // intentional fall through

        case SRV_NOTIFY_STAGE_SMB_V2_ATTEMPT_IO:

            ntStatus = SrvExecuteChangeNotify_SMB_V2(
                            pExecContext,
                            pNotifyState);
            switch (ntStatus)
            {
                case STATUS_PENDING:

                {
                    // TODO: Might have to cancel the entire operation
                    //
                    NTSTATUS ntStatus2 = SrvBuildInterimResponse_SMB_V2(
                                                pExecContext,
                                                pNotifyState->ullAsyncId);
                    if (ntStatus2 != STATUS_SUCCESS)
                    {
                        LWIO_LOG_ERROR(
                                "Failed to create interim response [code:0x%8x]",
                                ntStatus2);
                    }

                    bUnregisterAsync = FALSE;
                }

                    break;

                case STATUS_SUCCESS:

                    // completed synchronously; remove asynchronous state
                    //
                    ntStatus = SrvSession2RemoveAsyncState(
                                    pCtxSmb2->pSession,
                                    pAsyncState->ullAsyncId);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pNotifyState->ullAsyncId = 0LL;

                    bUnregisterAsync = FALSE;

                    break;

                default:

                    break;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            pNotifyRequestState->stage = SRV_NOTIFY_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_NOTIFY_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildNotifyResponse_SMB_V2(
                            pExecContext,
                            pNotifyState);
            BAIL_ON_NT_STATUS(ntStatus);

            pNotifyRequestState->stage = SRV_NOTIFY_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_NOTIFY_STAGE_SMB_V2_DONE:

            break;
    }

cleanup:

    if (pNotifyRequestState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pNotifyRequestState->mutex);

        SrvReleaseNotifyRequestState_SMB_V2(pNotifyRequestState);
    }

    if (pNotifyState)
    {
        if (bUnregisterAsync)
        {
            SrvSession2RemoveAsyncState(
                    pCtxSmb2->pSession,
                    pNotifyState->ullAsyncId);
        }

        SrvNotifyStateRelease_SMB_V2(pNotifyState);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

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

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            break;

        default:

            if (pNotifyState)
            {
                SrvReleaseNotifyStateAsync_SMB_V2(pNotifyState);
            }

            break;
    }

    goto cleanup;
}

NTSTATUS
SrvProcessNotifyCompletion_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PLWIO_SRV_SESSION_2        pSession     = NULL;
    PLWIO_SRV_TREE_2           pTree        = NULL;
    BOOLEAN                    bInLock      = FALSE;
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    ULONG64                    ullAsyncId   = 0LL;
    PSRV_NOTIFY_STATE_SMB_V2   pNotifyState = NULL;

    ntStatus = SrvConnection2FindSession_SMB_V2(
                            pCtxSmb2,
                            pConnection,
                            pSmbRequest->pHeader->ullSessionId,
                            &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2GetAsyncId(pSmbRequest->pHeader, &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindAsyncState(pSession, ullAsyncId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2RemoveAsyncState(pSession, ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    pNotifyState = (PSRV_NOTIFY_STATE_SMB_V2)pAsyncState->hAsyncState;

    ntStatus = SrvSession2FindTree_SMB_V2(
                    pCtxSmb2,
                    pSession,
                    pNotifyState->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bInLock, &pNotifyState->mutex);

    switch (pSmbRequest->pHeader->error)
    {
        case STATUS_CANCELLED:

            ntStatus = SrvBuildErrorResponse_SMB_V2(
                            pExecContext,
                            pNotifyState->ullAsyncId,
                            pSmbRequest->pHeader->error);

            break;

        case STATUS_NOTIFY_ENUM_DIR:
        case STATUS_SUCCESS:

            pNotifyState->ulBytesUsed =
                                pNotifyState->ioStatusBlock.BytesTransferred;

            ntStatus = SrvBuildNotifyResponse_SMB_V2(
                            pExecContext,
                            pNotifyState);

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pNotifyState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pNotifyState->mutex);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvCancelChangeNotify_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PLWIO_SRV_SESSION_2        pSession     = NULL;
    BOOLEAN                    bInLock      = FALSE;
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    ULONG64                    ullAsyncId   = 0LL;
    PSRV_NOTIFY_STATE_SMB_V2   pNotifyState = NULL;

    ntStatus = SrvConnection2FindSession_SMB_V2(
                            pCtxSmb2,
                            pConnection,
                            pSmbRequest->pHeader->ullSessionId,
                            &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2GetAsyncId(pSmbRequest->pHeader, &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindAsyncState(pSession, ullAsyncId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pNotifyState = (PSRV_NOTIFY_STATE_SMB_V2)pAsyncState->hAsyncState;

    LWIO_LOCK_MUTEX(bInLock, &pNotifyState->mutex);

    if (pNotifyState->pAcb && pNotifyState->pAcb->AsyncCancelContext)
    {
        IoCancelAsyncCancelContext(pNotifyState->pAcb->AsyncCancelContext);
    }

cleanup:

    if (pNotifyState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pNotifyState->mutex);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildNotifyRequestState_SMB_V2(
    PSMB2_NOTIFY_CHANGE_HEADER         pRequestHeader,
    PSRV_NOTIFY_REQUEST_STATE_SMB_V2*  ppNotifyRequestState
    )
{
    NTSTATUS ntStatus     = STATUS_SUCCESS;
    PSRV_NOTIFY_REQUEST_STATE_SMB_V2 pNotifyRequestState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_NOTIFY_REQUEST_STATE_SMB_V2),
                    (PVOID*)&pNotifyRequestState);
    BAIL_ON_NT_STATUS(ntStatus);

    pNotifyRequestState->refCount = 1;

    pthread_mutex_init(&pNotifyRequestState->mutex, NULL);
    pNotifyRequestState->pMutex = &pNotifyRequestState->mutex;

    pNotifyRequestState->stage = SRV_NOTIFY_STAGE_SMB_V2_INITIAL;

    pNotifyRequestState->pRequestHeader = pRequestHeader;

    *ppNotifyRequestState = pNotifyRequestState;

cleanup:

    return ntStatus;

error:

    *ppNotifyRequestState = NULL;

    if (pNotifyRequestState)
    {
        SrvFreeNotifyRequestState_SMB_V2(pNotifyRequestState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteChangeNotify_SMB_V2(
    PSRV_EXEC_CONTEXT        pExecContext,
    PSRV_NOTIFY_STATE_SMB_V2 pNotifyState
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;

    SrvPrepareNotifyStateAsync_SMB_V2(pNotifyState);

    ntStatus = IoReadDirectoryChangeFile(
                    pNotifyState->pFile->hFile,
                    pNotifyState->pAcb,
                    &pNotifyState->ioStatusBlock,
                    pNotifyState->pBuffer,
                    pNotifyState->ulBufferLength,
                    pNotifyState->bWatchTree,
                    pNotifyState->ulCompletionFilter,
                    &pNotifyState->ulMaxBufferSize);

    if (ntStatus == STATUS_NOT_SUPPORTED)
    {
        //
        // The file system driver does not have support
        // to keep accumulating file change notifications
        //
        ntStatus = IoReadDirectoryChangeFile(
                        pNotifyState->pFile->hFile,
                        pNotifyState->pAcb,
                        &pNotifyState->ioStatusBlock,
                        pNotifyState->pBuffer,
                        pNotifyState->ulBufferLength,
                        pNotifyState->bWatchTree,
                        pNotifyState->ulCompletionFilter,
                        NULL);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseNotifyStateAsync_SMB_V2(pNotifyState); // Completed synchronously

    pNotifyState->ulBytesUsed = pNotifyState->ioStatusBlock.BytesTransferred;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildNotifyResponse_SMB_V2(
    PSRV_EXEC_CONTEXT        pExecContext,
    PSRV_NOTIFY_STATE_SMB_V2 pNotifyState
    )
{
    NTSTATUS                     ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2     pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                        iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2          pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2          pSmbResponse = &pCtxSmb2->pResponses[iMsg];
    PSMB2_NOTIFY_RESPONSE_HEADER pNotifyResponseHeader = NULL; // do not free
    PBYTE pData            = NULL;
    ULONG ulDataLength     = 0;
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulDataOffset     = 0;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SMB2MarshalHeader(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM2_NOTIFY,
                pSmbRequest->pHeader->usEpoch,
                (pNotifyState->ullAsyncId ? 0 : pSmbRequest->pHeader->usCredits),
                pSmbRequest->pHeader->ulPid,
                pSmbRequest->pHeader->ullCommandSequence,
                pCtxSmb2->pTree->ulTid,
                pCtxSmb2->pSession->ullUid,
                pNotifyState->ullAsyncId,
                pSmbRequest->pHeader->error,
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

    if ((pNotifyState->ioStatusBlock.Status == STATUS_SUCCESS) &&
        pNotifyState->ulBytesUsed > 0)
    {
        ntStatus = SrvMarshalNotifyResponse_SMB_V2(
                        pNotifyState->pBuffer,
                        pNotifyState->ulBytesUsed,
                        &pData,
                        &ulDataLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMB2MarshalNotifyResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pData,
                    ulDataLength,
                    &ulDataOffset,
                    &pNotifyResponseHeader,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += ulBytesUsed;
    // ulOffset         += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pData)
    {
        SrvFreeMemory(pData);
    }

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

static
NTSTATUS
SrvMarshalNotifyResponse_SMB_V2(
    PBYTE  pNotifyResponse,
    ULONG  ulNotifyResponseLength,
    PBYTE* ppBuffer,
    PULONG pulBufferLength
    )
{
    NTSTATUS ntStatus         = STATUS_SUCCESS;
    PBYTE    pBuffer          = NULL;
    ULONG    ulBufferLength   = 0;
    ULONG    ulOffset         = 0;
    ULONG    ulNumRecords     = 0;
    ULONG    iRecord          = 0;
    ULONG    ulBytesAvailable = ulNotifyResponseLength;
    PBYTE    pDataCursor      = NULL;
    PFILE_NOTIFY_INFORMATION pNotifyCursor = NULL;
    PFILE_NOTIFY_INFORMATION pPrevHeader   = NULL;
    PFILE_NOTIFY_INFORMATION pCurHeader    = NULL;

    pNotifyCursor = (PFILE_NOTIFY_INFORMATION)pNotifyResponse;

    while (pNotifyCursor && (ulBytesAvailable > 0))
    {
        ULONG ulInfoBytesRequired = 0;

        if (pNotifyCursor->NextEntryOffset != 0)
        {
            if (ulBufferLength % 4)
            {
                USHORT usAlignment = (4 - (ulBufferLength % 4));

                ulInfoBytesRequired += usAlignment;
            }
        }

        ulInfoBytesRequired += offsetof(FILE_NOTIFY_INFORMATION, FileName);
        ulOffset            += offsetof(FILE_NOTIFY_INFORMATION, FileName);

        if (!pNotifyCursor->FileNameLength)
        {
            ulInfoBytesRequired += sizeof(wchar16_t);
        }
        else
        {
            ulInfoBytesRequired += pNotifyCursor->FileNameLength;
        }

        ulNumRecords++;

        ulBytesAvailable -= ulInfoBytesRequired;
        ulBufferLength   += ulInfoBytesRequired;

        if (pNotifyCursor->NextEntryOffset)
        {
            pNotifyCursor =
                (PFILE_NOTIFY_INFORMATION)(((PBYTE)pNotifyCursor) +
                                            pNotifyCursor->NextEntryOffset);
        }
        else
        {
            pNotifyCursor = NULL;
        }
    }

    if (ulBufferLength)
    {
        ntStatus = SrvAllocateMemory(ulBufferLength, (PVOID*)&pBuffer);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNotifyCursor = (PFILE_NOTIFY_INFORMATION)pNotifyResponse;
    pDataCursor   = pBuffer;

    for (; iRecord < ulNumRecords; iRecord++)
    {
        pPrevHeader = pCurHeader;
        pCurHeader = (PFILE_NOTIFY_INFORMATION)pDataCursor;

        /* Update next entry offset for previous entry. */
        if (pPrevHeader != NULL)
        {
            pPrevHeader->NextEntryOffset = ulOffset;
        }

        ulOffset = 0;

        pCurHeader->NextEntryOffset = 0;
        pCurHeader->Action = pNotifyCursor->Action;
        pCurHeader->FileNameLength = pNotifyCursor->FileNameLength;

        pDataCursor += offsetof(FILE_NOTIFY_INFORMATION, FileName);
        ulOffset    += offsetof(FILE_NOTIFY_INFORMATION, FileName);

        if (pNotifyCursor->FileNameLength)
        {
            memcpy( pDataCursor,
                    (PBYTE)pNotifyCursor->FileName,
                    pNotifyCursor->FileNameLength);

            pDataCursor += pNotifyCursor->FileNameLength;
            ulOffset    += pNotifyCursor->FileNameLength;
        }
        else
        {
            pDataCursor += sizeof(wchar16_t);
            ulOffset    += sizeof(wchar16_t);
        }

        if (pNotifyCursor->NextEntryOffset != 0)
        {
            if (ulOffset % 4)
            {
                USHORT usAlign = 4 - (ulOffset % 4);

                pDataCursor += usAlign;
                ulOffset    += usAlign;
            }
        }

        pNotifyCursor =
                    (PFILE_NOTIFY_INFORMATION)(((PBYTE)pNotifyCursor) +
                                    pNotifyCursor->NextEntryOffset);
    }

    *ppBuffer        = pBuffer;
    *pulBufferLength = ulBufferLength;

cleanup:

    return ntStatus;

error:

    *ppBuffer        = NULL;
    *pulBufferLength = 0;

    if (pBuffer)
    {
        SrvFreeMemory(pBuffer);
    }

    goto cleanup;
}

static
VOID
SrvReleaseNotifyRequestStateHandle_SMB_V2(
    HANDLE hNotifyRequest
    )
{
    return SrvReleaseNotifyRequestState_SMB_V2(
                (PSRV_NOTIFY_REQUEST_STATE_SMB_V2)hNotifyRequest);
}

static
VOID
SrvReleaseNotifyRequestState_SMB_V2(
    PSRV_NOTIFY_REQUEST_STATE_SMB_V2 pNotifyRequestState
    )
{
    if (InterlockedDecrement(&pNotifyRequestState->refCount) == 0)
    {
        SrvFreeNotifyRequestState_SMB_V2(pNotifyRequestState);
    }
}

static
VOID
SrvFreeNotifyRequestState_SMB_V2(
    PSRV_NOTIFY_REQUEST_STATE_SMB_V2 pNotifyRequestState
    )
{
    if (pNotifyRequestState->pFile)
    {
        SrvFile2Release(pNotifyRequestState->pFile);
    }

    SrvFreeMemory(pNotifyRequestState);
}

