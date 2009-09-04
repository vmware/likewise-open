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

// Case 1: Fail immediately
//
//         All lock requests are posted synchronously
//
// Case 2: Wait indefinitely
//
//         All lock requests are posted asynchronously
//         When all the callbacks are received, a response is sent to the client
//
// Case 3: Wait on a timeout
//
//         All lock requests are posted asynchronously.
//
//         If the timeout happens first, the lock request is set to be expired.
//         And a failure message is sent immediately. An attempt is made to
//         cancel any asynchronous requests.
//
//         If all requests are fulfilled before the timer expires, a response
//         is sent to the client immediately. An attempt is made to cancel the
//         timer.

static
NTSTATUS
SrvBuildLockState(
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge,
    PLOCKING_ANDX_RANGE              pUnlockRange,
    PLOCKING_ANDX_RANGE              pLockRange,
    PLWIO_SRV_FILE                   pFile,
    PSRV_LOCK_STATE_SMB_V1*          ppLockState
    );

static
NTSTATUS
SrvExecuteLockRequest(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvLockExpiredCB(
    PSRV_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    );

static
VOID
SrvPrepareLockStateAsync(
    PSRV_LOCK_STATE_SMB_V1 pLockState,
    PSRV_EXEC_CONTEXT      pExecContext
    );

static
VOID
SrvExecuteLockAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseLockStateAsync(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

static
VOID
SrvClearLocks(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

static
NTSTATUS
SrvBuildLockingAndXResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseLockStateHandle(
    HANDLE hState
    );

VOID
SrvReleaseLockState(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

static
VOID
SrvFreeLockState(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

NTSTATUS
SrvProcessLockAndX(
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
    PSRV_LOCK_STATE_SMB_V1     pLockState   = NULL;
    BOOLEAN                    bInLock      = FALSE;
    PSRV_OPLOCK_STATE_SMB_V1   pOplockState = NULL;

    pLockState = (PSRV_LOCK_STATE_SMB_V1)pCtxSmb1->hState;

    if (pLockState)
    {
        InterlockedIncrement(&pLockState->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader = NULL;  // Do not free
        PLOCKING_ANDX_RANGE              pUnlockRange = NULL;    // Do not free
        PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge = NULL;// Do not free
        PLOCKING_ANDX_RANGE              pLockRange = NULL;      // Do not free
        PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge = NULL; // Do not free

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

        ntStatus = WireUnmarshallLockingAndXRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pUnlockRange,
                        &pUnlockRangeLarge,
                        &pLockRange,
                        &pLockRangeLarge);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvTreeFindFile_SMB_V1(
                        pCtxSmb1,
                        pTree,
                        pRequestHeader->usFid,
                        &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildLockState(
                        pRequestHeader,
                        pUnlockRangeLarge,
                        pLockRangeLarge,
                        pUnlockRange,
                        pLockRange,
                        pFile,
                        &pLockState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pLockState;
        pCtxSmb1->pfnStateRelease = &SrvReleaseLockStateHandle;
        InterlockedIncrement(&pLockState->refCount);
    }

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    switch (pLockState->stage)
    {
        case SRV_LOCK_STAGE_SMB_V1_INITIAL:

            if (pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_OPLOCK_RELEASE)
            {
                pOplockState =
                    (PSRV_OPLOCK_STATE_SMB_V1)SrvFileRemoveOplockState(pLockState->pFile);

                if (pOplockState)
                {
                    if (pOplockState->pTimerRequest)
                    {
                        PSRV_OPLOCK_STATE_SMB_V1 pOplockState2 = NULL;

                        SrvTimerCancelRequest(
                                        pOplockState->pTimerRequest,
                                        (PVOID*)&pOplockState2);

                        if (pOplockState2)
                        {
                            SrvReleaseOplockState(pOplockState2);
                        }

                        SrvTimerRelease(pOplockState->pTimerRequest);
                        pOplockState->pTimerRequest = NULL;
                    }

                    ntStatus = SrvAcknowledgeOplockBreak(pOplockState, FALSE);
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                goto cleanup;
            }
            else
            {
                switch (pLockState->pRequestHeader->ulTimeout)
                {
                    case 0:           /* don't wait i.e. fail immediately */
                    case ((ULONG)-1): /* wait indefinitely                */

                        break;

                    default:

                        {
                            LONG64 llExpiry = 0LL;

                            llExpiry =
                                (time(NULL) +
                                 (pLockState->pRequestHeader->ulTimeout/1000) +
                                 11644473600LL) * 10000000LL;

                            ntStatus = SrvTimerPostRequest(
                                            llExpiry,
                                            pExecContext,
                                            &SrvLockExpiredCB,
                                            &pLockState->pTimerRequest);
                            BAIL_ON_NT_STATUS(ntStatus);

                            InterlockedIncrement(&pExecContext->refCount);
                        }

                        break;
                }

                pLockState->stage = SRV_LOCK_STAGE_SMB_V1_ATTEMPT_LOCK;
            }

            // Intentional fall through

        case SRV_LOCK_STAGE_SMB_V1_ATTEMPT_LOCK:

            ntStatus = SrvExecuteLockRequest(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pLockState->stage = SRV_LOCK_STAGE_SMB_V1_DONE;

            // Intentional fall through

        case SRV_LOCK_STAGE_SMB_V1_DONE:

            ntStatus = SrvBuildLockingAndXResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

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

    if (pLockState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pLockState->mutex);

        SrvReleaseLockState(pLockState);
    }

    if (pOplockState)
    {
        SrvReleaseOplockState(pOplockState);
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

            if (pLockState)
            {
                SrvReleaseLockStateAsync(pLockState);
            }

            SrvClearLocks(pLockState);

            ntStatus = STATUS_LOCK_NOT_GRANTED;

            break;
    }


    goto cleanup;
}

static
NTSTATUS
SrvBuildLockState(
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge,
    PLOCKING_ANDX_RANGE              pUnlockRange,
    PLOCKING_ANDX_RANGE              pLockRange,
    PLWIO_SRV_FILE                   pFile,
    PSRV_LOCK_STATE_SMB_V1*          ppLockState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_LOCK_STATE_SMB_V1 pLockState = NULL;

    if (!(pRequestHeader->ucLockType & LWIO_LOCK_TYPE_OPLOCK_RELEASE))
    {
        if (pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
        {
            if ((pRequestHeader->usNumUnlocks && !pUnlockRangeLarge) ||
                (pRequestHeader->usNumLocks && !pLockRangeLarge))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            }
        }
        else
        {
            if ((pRequestHeader->usNumUnlocks && !pUnlockRange) ||
                (pRequestHeader->usNumLocks && !pLockRange))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            }
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                        sizeof(SRV_LOCK_STATE_SMB_V1),
                        (PVOID*)&pLockState);
    BAIL_ON_NT_STATUS(ntStatus);

    pLockState->refCount = 1;

    pthread_mutex_init(&pLockState->mutex, NULL);
    pLockState->pMutex = &pLockState->mutex;

    pLockState->stage = SRV_LOCK_STAGE_SMB_V1_INITIAL;

    pLockState->pRequestHeader    = pRequestHeader;
    pLockState->pUnlockRangeLarge = pUnlockRangeLarge;
    pLockState->pLockRangeLarge   = pLockRangeLarge;
    pLockState->pUnlockRange      = pUnlockRange;
    pLockState->pLockRange        = pLockRange;

    pLockState->bRequestExclusiveLock =
     !(pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_SHARED_LOCK);

    pLockState->pFile = pFile;
    InterlockedIncrement(&pFile->refcount);

    pLockState->bExpired   = FALSE;
    pLockState->bCompleted = FALSE;

    *ppLockState = pLockState;

cleanup:

    return ntStatus;

error:

    *ppLockState = NULL;

    if (pLockState)
    {
        SrvFreeLockState(pLockState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteLockRequest(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_LOCK_STATE_SMB_V1     pLockState   = NULL;
    BOOLEAN                    bFailImmediately  = FALSE;
    BOOLEAN                    bWaitIndefinitely = FALSE;

    pLockState        = (PSRV_LOCK_STATE_SMB_V1)pCtxSmb1->hState;
    bFailImmediately  = (pLockState->pRequestHeader->ulTimeout == 0);
    bWaitIndefinitely = (pLockState->pRequestHeader->ulTimeout == (ULONG)-1);

    if (pLockState->bExpired)
    {
        SrvReleaseLockStateAsync(pLockState);

        ntStatus = STATUS_IO_TIMEOUT;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pLockState->bUnlockPending)
    {
        ntStatus = pLockState->ioStatusBlock.Status; // async response status
        BAIL_ON_NT_STATUS(ntStatus);

        pLockState->iUnlock++;
        pLockState->bUnlockPending = FALSE;
    }

    for (; pLockState->iUnlock < pLockState->pRequestHeader->usNumUnlocks;
           pLockState->iUnlock++)
    {
        if (pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
        {
            PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo =
                        &pLockState->pUnlockRangeLarge[pLockState->iUnlock];

            pLockState->llOffset = (((LONG64)pLockInfo->ulOffsetHigh) << 32) |
                                   ((LONG64)pLockInfo->ulOffsetLow);

            pLockState->llLength = (((LONG64)pLockInfo->ulLengthHigh) << 32) |
                                   ((LONG64)pLockInfo->ulLengthLow);

            pLockState->ulKey    = pLockInfo->usPid;
        }
        else
        {
            PLOCKING_ANDX_RANGE pLockInfo =
                        &pLockState->pUnlockRange[pLockState->iUnlock];

            pLockState->llOffset = pLockInfo->ulOffset;
            pLockState->llLength = pLockInfo->ulLength;
            pLockState->ulKey    = pLockInfo->usPid;
        }

        if (!bFailImmediately)
        {
            SrvPrepareLockStateAsync(pLockState, pExecContext);
        }

        ntStatus = IoUnlockFile(
                        pLockState->pFile->hFile,
                        pLockState->pAcb,
                        &pLockState->ioStatusBlock,
                        pLockState->llOffset,
                        pLockState->llLength,
                        pLockState->ulKey);
        if (ntStatus == STATUS_PENDING)
        {
            pLockState->bUnlockPending = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (!bFailImmediately)
        {
            SrvReleaseLockStateAsync(pLockState); // completed synchronously
        }
    }

    if (pLockState->bLockPending)
    {
        ntStatus = pLockState->ioStatusBlock.Status; // async response status
        BAIL_ON_NT_STATUS(ntStatus);

        pLockState->iLock++;
        pLockState->bLockPending = FALSE;
    }

    for (; pLockState->iLock < pLockState->pRequestHeader->usNumLocks;
           pLockState->iLock++)
    {
        if (pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
        {
            PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo =
                        &pLockState->pLockRangeLarge[pLockState->iLock];

            pLockState->llOffset = (((LONG64)pLockInfo->ulOffsetHigh) << 32) |
                                   ((LONG64)pLockInfo->ulOffsetLow);

            pLockState->llLength = (((LONG64)pLockInfo->ulLengthHigh) << 32) |
                                   ((LONG64)pLockInfo->ulLengthLow);

            pLockState->ulKey    = pLockInfo->usPid;
        }
        else
        {
            PLOCKING_ANDX_RANGE pLockInfo =
                        &pLockState->pLockRange[pLockState->iLock];

            pLockState->llOffset = pLockInfo->ulOffset;
            pLockState->llLength = pLockInfo->ulLength;
            pLockState->ulKey    = pLockInfo->usPid;
        }

        if (!bFailImmediately)
        {
            SrvPrepareLockStateAsync(pLockState, pExecContext);
        }

        ntStatus = IoLockFile(
                        pLockState->pFile->hFile,
                        pLockState->pAcb,
                        &pLockState->ioStatusBlock,
                        pLockState->llOffset,
                        pLockState->llLength,
                        pLockState->ulKey,
                        bFailImmediately,
                        pLockState->bRequestExclusiveLock);
        if (ntStatus == STATUS_PENDING)
        {
            pLockState->bLockPending = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (!bFailImmediately)
        {
            SrvReleaseLockStateAsync(pLockState); // completed synchronously
        }
    }

    pLockState->bCompleted = TRUE;

    if (pLockState->pTimerRequest)
    {
        PSRV_EXEC_CONTEXT pExecContext = NULL;

        SrvTimerCancelRequest(pLockState->pTimerRequest, (PVOID*)&pExecContext);
        if (pExecContext)
        {
            SrvReleaseExecContext(pExecContext);
        }

        SrvTimerRelease(pLockState->pTimerRequest);
        pLockState->pTimerRequest = NULL;
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvLockExpiredCB(
    PSRV_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pExecContext = (PSRV_EXEC_CONTEXT)pUserData;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_LOCK_STATE_SMB_V1     pLockState   = NULL;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bEnqueueRequest = FALSE;

    pLockState = (PSRV_LOCK_STATE_SMB_V1)pCtxSmb1->hState;

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    if (!pLockState->bCompleted)
    {
        pLockState->bExpired = TRUE;
        bEnqueueRequest = TRUE;

        SrvReleaseLockStateAsync(pLockState); // cancel a pending async request
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pLockState->mutex);

    if (bEnqueueRequest)
    {
        ntStatus = SrvProdConsEnqueue(
                        gProtocolGlobals_SMB_V1.pWorkQueue,
                        pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);

        pExecContext = NULL;
    }

cleanup:

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    return;

error:

    LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                   ntStatus);

    goto cleanup;
}

static
VOID
SrvPrepareLockStateAsync(
    PSRV_LOCK_STATE_SMB_V1 pLockState,
    PSRV_EXEC_CONTEXT      pExecContext
    )
{
    pLockState->acb.Callback          = &SrvExecuteLockAsyncCB;

    pLockState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pLockState->acb.AsyncCancelContext = NULL;

    pLockState->pAcb = &pLockState->acb;
}

static
VOID
SrvExecuteLockAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_LOCK_STATE_SMB_V1     pLockState       = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pLockState = (PSRV_LOCK_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    if (pLockState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pLockState->pAcb->AsyncCancelContext);
    }

    pLockState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pLockState->mutex);

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
SrvReleaseLockStateAsync(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
    if (pLockState->pAcb)
    {
        pLockState->acb.Callback        = NULL;

        if (pLockState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pLockState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pLockState->pAcb->CallbackContext = NULL;
        }

        if (pLockState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pLockState->pAcb->AsyncCancelContext);
        }

        pLockState->pAcb = NULL;
    }
}

static
VOID
SrvClearLocks(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
    // Unlock in reverse order
    while (pLockState->iLock-- > 0)
    {
        NTSTATUS        ntStatus      = STATUS_SUCCESS;
        IO_STATUS_BLOCK ioStatusBlock = {0};
        LONG64          llOffset      = 0;
        LONG64          llLength      = 0;
        ULONG           ulKey         = 0;

        if (pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
        {
            PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo =
                        &pLockState->pLockRangeLarge[pLockState->iLock];

            llOffset = (((LONG64)pLockInfo->ulOffsetHigh) << 32) |
                       ((LONG64)pLockInfo->ulOffsetLow);

            llLength = (((LONG64)pLockInfo->ulLengthHigh) << 32) |
                       ((LONG64)pLockInfo->ulLengthLow);

            ulKey    = pLockInfo->usPid;
        }
        else
        {
            PLOCKING_ANDX_RANGE pLockInfo =
                        &pLockState->pLockRange[pLockState->iLock];

            llOffset = pLockInfo->ulOffset;
            llLength = pLockInfo->ulLength;
            ulKey    = pLockInfo->usPid;
        }

        ntStatus = IoUnlockFile(
                        pLockState->pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        llOffset,
                        llLength,
                        ulKey);
        if (ntStatus != STATUS_SUCCESS)
        {
            LWIO_LOG_ERROR("Failed to unlock range "
                           "[fid:%d; offset:%lld; length:%lld][status:0x%x]",
                            pLockState->pFile->fid,
                            llOffset,
                            llLength,
                            ntStatus);
        }
    }
}

static
NTSTATUS
SrvBuildLockingAndXResponse(
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
    PSMB_LOCKING_ANDX_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    USHORT usBytesUsed         = 0;
    ULONG ulTotalBytesUsed     = 0;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_LOCKING_ANDX,
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
                        COM_LOCKING_ANDX,
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

    ntStatus = WireMarshallLockingAndXResponse(
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
SrvReleaseLockStateHandle(
    HANDLE hState
    )
{
    SrvReleaseLockState((PSRV_LOCK_STATE_SMB_V1)hState);
}

VOID
SrvReleaseLockState(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
    if (InterlockedDecrement(&pLockState->refCount) == 0)
    {
        SrvFreeLockState(pLockState);
    }
}

static
VOID
SrvFreeLockState(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
    if (pLockState->pAcb && pLockState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pLockState->pAcb->AsyncCancelContext);
    }

    if (pLockState->pTimerRequest)
    {
        PSRV_EXEC_CONTEXT pExecContext = NULL;

        SrvTimerCancelRequest(pLockState->pTimerRequest, (PVOID*)&pExecContext);
        if (pExecContext)
        {
            SrvReleaseExecContext(pExecContext);
        }

        SrvTimerRelease(pLockState->pTimerRequest);
    }

    if (pLockState->pFile)
    {
        SrvFileRelease(pLockState->pFile);
    }

    if (pLockState->pMutex)
    {
        pthread_mutex_destroy(&pLockState->mutex);
    }

    SrvFreeMemory(pLockState);
}

