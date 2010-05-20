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
 *        lockasync.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Byte range locking (Asynchronous)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvExecuteAsyncLockRequest_SMB_V2(
    PSRV_EXEC_CONTEXT                    pExecContext,
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    );

static
VOID
SrvClearAsyncLocks_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    );

static
VOID
SrvClearAsyncLocks_SMB_V2_inlock(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    );

static
NTSTATUS
SrvBuildAsyncLockResponse_SMB_V2(
    PSRV_EXEC_CONTEXT      pExecContext
    );

static
VOID
SrvCancelAsyncLockState_SMB_V2_inlock(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    );

static
VOID
SrvPrepareAsyncLockStateAsync_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState,
    PSRV_EXEC_CONTEXT                    pExecContext
    );

static
VOID
SrvExecuteAsyncLockContextAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseAsyncLockStateAsync_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    );

static
VOID
SrvFreeAsyncLockState_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    );

NTSTATUS
SrvBuildAsyncLockState_SMB_V2(
    ULONG64                               ullAsyncId,
    PSRV_EXEC_CONTEXT                     pExecContext,
    PSRV_LOCK_REQUEST_STATE_SMB_V2        pLockRequestState,
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2* ppAsyncLockState
    )
{
    NTSTATUS  ntStatus = STATUS_SUCCESS;
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2),
                    (PVOID*)&pAsyncLockState);
    BAIL_ON_NT_STATUS(ntStatus);

    pAsyncLockState->refCount = 1;

    pthread_mutex_init(&pAsyncLockState->mutex, NULL);
    pAsyncLockState->pMutex = &pAsyncLockState->mutex;

    pAsyncLockState->stage  = SRV_NOTIFY_STAGE_SMB_V2_INITIAL;

    pAsyncLockState->ullAsyncId = ullAsyncId;

    pAsyncLockState->ulTid = pLockRequestState->ulTid;

    *ppAsyncLockState = pAsyncLockState;

cleanup:

    return ntStatus;

error:

    if (pAsyncLockState)
    {
        SrvReleaseAsyncLockState_SMB_V2(pAsyncLockState);
    }

    *ppAsyncLockState = NULL;

    goto cleanup;
}

NTSTATUS
SrvBuildExecContextAsyncLock_SMB_V2(
    PSRV_EXEC_CONTEXT              pExecContext,
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState,
    ULONG64                        ullAsyncId,
    PSRV_EXEC_CONTEXT*             ppExecContextAsync
    )
{
    NTSTATUS                   ntStatus       = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection    = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2       = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg           = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
    PSRV_EXEC_CONTEXT pExecContextAsync   = NULL;
    PSMB_PACKET       pSmbRequest2        = NULL;
    PBYTE             pBuffer             = NULL;
    ULONG             ulBytesAvailable    = 0;
    ULONG             ulOffset            = 0;
    ULONG             ulBytesUsed         = 0;
    ULONG             ulTotalBytesUsed    = 0;
    PSMB2_HEADER      pHeader             = NULL; // Do not free
    PSMB2_LOCK_REQUEST_HEADER pLockHeader = NULL; // Do not free
    ULONG             iLock               = 0;

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbRequest2);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    pSmbRequest->ulMessageSize + sizeof(NETBIOS_HEADER),
                    &pSmbRequest2->pRawBuffer,
                    &pSmbRequest2->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2InitPacket(pSmbRequest2, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildExecContext(
                    pConnection,
                    pSmbRequest2,
                    TRUE,
                    &pExecContextAsync);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          = pSmbRequest2->pRawBuffer;
    ulBytesAvailable = pSmbRequest2->bufferLen;

    if (ulBytesAvailable < sizeof(NETBIOS_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBuffer          += sizeof(NETBIOS_HEADER);
    ulBytesAvailable -= sizeof(NETBIOS_HEADER);

    ntStatus = SMB2MarshalHeader(
                    pBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_LOCK,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pSmbRequest->pHeader->ulTid,
                    pSmbRequest->pHeader->ullSessionId,
                    ullAsyncId,
                    STATUS_SUCCESS,
                    FALSE,              /* is response */
                    FALSE,              /* chained message */
                    &pHeader,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    if (ulBytesAvailable < sizeof(SMB2_LOCK_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pLockHeader = (PSMB2_LOCK_REQUEST_HEADER)pBuffer;

    pLockHeader->usLength    = pLockRequestState->pRequestHeader->usLength;
    pLockHeader->usLockCount = pLockRequestState->pRequestHeader->usLockCount;
    pLockHeader->ulLockSequence =
                    pLockRequestState->pRequestHeader->ulLockSequence;
    pLockHeader->fid         = pLockRequestState->pRequestHeader->fid;

    pBuffer          += sizeof(SMB2_LOCK_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB2_LOCK_REQUEST_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_LOCK_REQUEST_HEADER);

    if (ulBytesAvailable < (sizeof(SMB2_LOCK) * (pLockHeader->usLockCount - 1)))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (iLock = 0; iLock < pLockHeader->usLockCount; iLock++)
    {
        pLockHeader->locks[iLock] =
                    pLockRequestState->pRequestHeader->locks[iLock];
    }

    // pBuffer          += (sizeof(SMB2_LOCK) * (pLockHeader->usLockCount - 1));
    // ulBytesAvailable -= (sizeof(SMB2_LOCK) * (pLockHeader->usLockCount - 1));
    ulTotalBytesUsed += (sizeof(SMB2_LOCK) * (pLockHeader->usLockCount - 1));

    pSmbRequest2->bufferUsed += ulTotalBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbRequest2);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppExecContextAsync = pExecContextAsync;

cleanup:

    if (pSmbRequest2)
    {
        SMBPacketRelease(pConnection->hPacketAllocator, pSmbRequest2);
    }

    return ntStatus;

error:

    *ppExecContextAsync = NULL;

    if (pExecContextAsync)
    {
        SrvReleaseExecContext(pExecContextAsync);
    }

    goto cleanup;
}

PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2
SrvAcquireAsyncLockState_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    )
{
    InterlockedIncrement(&pAsyncLockState->refCount);

    return pAsyncLockState;
}

NTSTATUS
SrvCancelLock_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    BOOLEAN                    bInLock      = FALSE;
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    ULONG64                    ullAsyncId   = 0LL;
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2     pLockState   = NULL;

    ntStatus = SMB2GetAsyncId(pSmbRequest->pHeader, &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnection2FindAsyncState(pConnection, ullAsyncId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pLockState = (PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2)pAsyncState->hAsyncState;

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    SrvCancelAsyncLockState_SMB_V2_inlock(pLockState);

cleanup:

    if (pLockState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pLockState->mutex);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    return ntStatus;

error:

    goto cleanup;
}

VOID
SrvCancelAsyncLockState_SMB_V2(
    HANDLE hLockState
    )
{
    BOOLEAN bInLock = FALSE;
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState =
            (PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2)hLockState;

    LWIO_LOCK_MUTEX(bInLock, &pAsyncLockState->mutex);

    SrvCancelAsyncLockState_SMB_V2_inlock(pAsyncLockState);

    LWIO_UNLOCK_MUTEX(bInLock, &pAsyncLockState->mutex);
}

NTSTATUS
SrvProcessAsyncLockRequest_SMB_V2(
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
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    ULONG64                    ullAsyncId   = 0LL;
    BOOLEAN                    bInLock      = FALSE;
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState = NULL;

    ntStatus = SrvConnection2FindSession_SMB_V2(
                    pCtxSmb2,
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2GetAsyncId(pSmbRequest->pHeader, &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnection2FindAsyncState(pConnection, ullAsyncId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pAsyncLockState =
            (PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2)pAsyncState->hAsyncState;

    LWIO_LOCK_MUTEX(bInLock, &pAsyncLockState->mutex);

    switch (pAsyncLockState->stage)
    {
        case SRV_LOCK_STAGE_SMB_V2_INITIAL:

            ntStatus = SrvSession2FindTree_SMB_V2(
                            pCtxSmb2,
                            pSession,
                            pAsyncLockState->ulTid,
                            &pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SMB2UnmarshalLockRequest(
                            pSmbRequest,
                            &pAsyncLockState->pRequestHeader);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvTree2FindFile_SMB_V2(
                                pCtxSmb2,
                                pTree,
                                &pAsyncLockState->pRequestHeader->fid,
                                LwIsSetFlag(
                                    pSmbRequest->pHeader->ulFlags,
                                    SMB2_FLAGS_RELATED_OPERATION),
                                &pAsyncLockState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvDetermineLocks_SMB_V2(
                            pAsyncLockState->pRequestHeader,
                            &pAsyncLockState->ppLockArray,
                            &pAsyncLockState->ulNumLocks);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvDetermineUnlocks_SMB_V2(
                            pAsyncLockState->pRequestHeader,
                            &pAsyncLockState->ppUnlockArray,
                            &pAsyncLockState->ulNumUnlocks);
            BAIL_ON_NT_STATUS(ntStatus);

            pAsyncLockState->bFailImmediately = FALSE;

            pCtxSmb2->hState          = pAsyncLockState;
            pCtxSmb2->pfnStateRelease = &SrvReleaseAsyncLockStateHandle_SMB_V2;
            SrvAcquireAsyncLockState_SMB_V2(pAsyncLockState);

            pAsyncLockState->stage = SRV_LOCK_STAGE_SMB_V2_ATTEMPT_LOCK;

            // intentional fall through

        case SRV_LOCK_STAGE_SMB_V2_ATTEMPT_LOCK:

            ntStatus = SrvExecuteAsyncLockRequest_SMB_V2(
                            pExecContext,
                            pAsyncLockState);
            BAIL_ON_NT_STATUS(ntStatus);

            pAsyncLockState->stage = SRV_LOCK_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_LOCK_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildAsyncLockResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pAsyncLockState->stage = SRV_LOCK_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_LOCK_STAGE_SMB_V2_DONE:

            ntStatus = SrvConnection2RemoveAsyncState(
                                pConnection,
                                pAsyncLockState->ullAsyncId);
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pAsyncLockState->mutex);

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

    switch (ntStatus)
    {
        case STATUS_PENDING:

            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed

            break;

        default:

            if (pAsyncLockState)
            {
                NTSTATUS ntStatus1 = STATUS_SUCCESS;

                ntStatus1 = SrvBuildErrorResponse_SMB_V2(
                                pExecContext,
                                pAsyncLockState->ullAsyncId,
                                ntStatus);
                if (ntStatus1 != STATUS_SUCCESS)
                {
                    LWIO_LOG_ERROR( "Failed to build error message for "
                                    "lock request [code:0x%X]",
                                    ntStatus1);
                }

                SrvReleaseAsyncLockStateAsync_SMB_V2(pAsyncLockState);

                if (bInLock)
                {
                    SrvClearAsyncLocks_SMB_V2_inlock(pAsyncLockState);
                }
                else
                {
                    SrvClearAsyncLocks_SMB_V2(pAsyncLockState);
                }

                if (pSession)
                {
                    ntStatus1 = SrvConnection2RemoveAsyncState(
                                        pConnection,
                                        pAsyncLockState->ullAsyncId);
                    if (ntStatus1 != STATUS_SUCCESS)
                    {
                        LWIO_LOG_ERROR( "Failed to remove async lock state"
                                        " from session [code:0x%X]",
                                        ntStatus1);
                    }
                }
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteAsyncLockRequest_SMB_V2(
    PSRV_EXEC_CONTEXT                    pExecContext,
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pAsyncLockRequestState->bUnlockPending)
    {
        ntStatus = pAsyncLockRequestState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        pAsyncLockRequestState->iUnlock++;
        pAsyncLockRequestState->bUnlockPending = FALSE;
    }

    if (pAsyncLockRequestState->bLockPending)
    {
        ntStatus = pAsyncLockRequestState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        pAsyncLockRequestState->iLock++;
        pAsyncLockRequestState->bLockPending = FALSE;
    }

    // Unlock requests
    for (;  pAsyncLockRequestState->iUnlock < pAsyncLockRequestState->ulNumUnlocks;
            pAsyncLockRequestState->iUnlock++)
    {
        PSMB2_LOCK pLock =
                pAsyncLockRequestState->ppUnlockArray[pAsyncLockRequestState->iUnlock];

        SrvPrepareAsyncLockStateAsync_SMB_V2(pAsyncLockRequestState, pExecContext);

        ntStatus = IoUnlockFile(
                        pAsyncLockRequestState->pFile->hFile,
                        pAsyncLockRequestState->pAcb,
                        &pAsyncLockRequestState->ioStatusBlock,
                        pLock->ullFileOffset,
                        pLock->ullByteRange,
                        pAsyncLockRequestState->pRequestHeader->ulLockSequence);
        if (ntStatus == STATUS_PENDING)
        {
            pAsyncLockRequestState->bUnlockPending = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseAsyncLockStateAsync_SMB_V2(pAsyncLockRequestState); // sync
    }

    // Lock requests
    for (;  pAsyncLockRequestState->iLock < pAsyncLockRequestState->ulNumLocks;
            pAsyncLockRequestState->iLock++)
    {
        PSMB2_LOCK pLock =
                pAsyncLockRequestState->ppLockArray[pAsyncLockRequestState->iLock];

        SrvPrepareAsyncLockStateAsync_SMB_V2(pAsyncLockRequestState, pExecContext);

        ntStatus = IoLockFile(
                        pAsyncLockRequestState->pFile->hFile,
                        pAsyncLockRequestState->pAcb,
                        &pAsyncLockRequestState->ioStatusBlock,
                        pLock->ullFileOffset,
                        pLock->ullByteRange,
                        pAsyncLockRequestState->pRequestHeader->ulLockSequence,
                        pAsyncLockRequestState->bFailImmediately,
                        LwIsSetFlag(pLock->ulFlags,
                                    SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK));
        if (ntStatus == STATUS_PENDING)
        {
            pAsyncLockRequestState->bLockPending = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseAsyncLockStateAsync_SMB_V2(pAsyncLockRequestState); // sync
    }

error:

    return ntStatus;
}

static
VOID
SrvClearAsyncLocks_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pAsyncLockRequestState->mutex);

    SrvClearAsyncLocks_SMB_V2_inlock(pAsyncLockRequestState);

    LWIO_UNLOCK_MUTEX(bInLock, &pAsyncLockRequestState->mutex);
}

static
VOID
SrvClearAsyncLocks_SMB_V2_inlock(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    )
{
    //
    // Unlock (backwards) locks that have already been acquired
    //
    while (pAsyncLockRequestState->iLock-- > 0)
    {
        NTSTATUS   ntStatus1 = STATUS_SUCCESS;
        PSMB2_LOCK pLock     =
                pAsyncLockRequestState->ppLockArray[pAsyncLockRequestState->iLock];

        pAsyncLockRequestState->pAcb = NULL;

        ntStatus1 = IoUnlockFile(
                        pAsyncLockRequestState->pFile->hFile,
                        pAsyncLockRequestState->pAcb,
                        &pAsyncLockRequestState->ioStatusBlock,
                        pLock->ullFileOffset,
                        pLock->ullByteRange,
                        pAsyncLockRequestState->pRequestHeader->ulLockSequence);
        if (ntStatus1)
        {
            LWIO_LOG_ERROR("Failed in unlock. error code [%d]", ntStatus1);
        }
    }

    return;
}

static
NTSTATUS
SrvBuildAsyncLockResponse_SMB_V2(
    PSRV_EXEC_CONTEXT      pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState = NULL;
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

    pAsyncLockRequestState =
                (PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM2_LOCK,
                pSmbRequest->pHeader->usEpoch,
                pSmbRequest->pHeader->usCredits,
                pSmbRequest->pHeader->ulPid,
                pSmbRequest->pHeader->ullCommandSequence,
                pCtxSmb2->pTree->ulTid,
                pCtxSmb2->pSession->ullUid,
                pAsyncLockRequestState->ullAsyncId,
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

    ntStatus = SMB2MarshalLockResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer += ulBytesUsed;
    // ulOffset += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

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
VOID
SrvCancelAsyncLockState_SMB_V2_inlock(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    )
{
    if (pAsyncLockState->pAcb && pAsyncLockState->pAcb->AsyncCancelContext)
    {
        IoCancelAsyncCancelContext(pAsyncLockState->pAcb->AsyncCancelContext);
    }
}

static
VOID
SrvPrepareAsyncLockStateAsync_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState,
    PSRV_EXEC_CONTEXT                    pExecContext
    )
{
    pAsyncLockRequestState->acb.Callback =
                &SrvExecuteAsyncLockContextAsyncCB_SMB_V2;

    pAsyncLockRequestState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pAsyncLockRequestState->acb.AsyncCancelContext = NULL;

    pAsyncLockRequestState->pAcb = &pAsyncLockRequestState->acb;
}

static
VOID
SrvExecuteAsyncLockContextAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState = NULL;
    BOOLEAN bInLock = FALSE;

    pAsyncLockRequestState =
            (PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2)pCtxSmb2->hState;

    LWIO_LOCK_MUTEX(bInLock, &pAsyncLockRequestState->mutex);

    if (pAsyncLockRequestState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pAsyncLockRequestState->pAcb->AsyncCancelContext);
    }

    pAsyncLockRequestState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pAsyncLockRequestState->mutex);

    ntStatus = SrvProdConsEnqueue(
                    gProtocolGlobals_SMB_V2.pWorkQueue,
                    pExecContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                        ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

static
VOID
SrvReleaseAsyncLockStateAsync_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockRequestState
    )
{
    if (pAsyncLockRequestState->pAcb)
    {
        pAsyncLockRequestState->acb.Callback        = NULL;

        if (pAsyncLockRequestState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pAsyncLockRequestState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pAsyncLockRequestState->pAcb->CallbackContext = NULL;
        }

        if (pAsyncLockRequestState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pAsyncLockRequestState->pAcb->AsyncCancelContext);
        }

        pAsyncLockRequestState->pAcb = NULL;
    }
}

VOID
SrvReleaseAsyncLockStateHandle_SMB_V2(
    HANDLE hAsyncLockState
    )
{
    SrvReleaseAsyncLockState_SMB_V2((PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2)hAsyncLockState);
}

VOID
SrvReleaseAsyncLockState_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    )
{
    if (InterlockedDecrement(&pAsyncLockState->refCount) == 0)
    {
        SrvFreeAsyncLockState_SMB_V2(pAsyncLockState);
    }
}

static
VOID
SrvFreeAsyncLockState_SMB_V2(
    PSRV_ASYNC_LOCK_REQUEST_STATE_SMB_V2 pAsyncLockState
    )
{
    if (pAsyncLockState->pAcb && pAsyncLockState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pAsyncLockState->pAcb->AsyncCancelContext);
    }

    if (pAsyncLockState->pFile)
    {
        SrvFile2Release(pAsyncLockState->pFile);
    }

    SRV_SAFE_FREE_MEMORY(pAsyncLockState->ppLockArray);
    SRV_SAFE_FREE_MEMORY(pAsyncLockState->ppUnlockArray);

    if (pAsyncLockState->pMutex)
    {
        pthread_mutex_destroy(&pAsyncLockState->mutex);
    }
}
