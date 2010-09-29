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
VOID
SrvFreePendingLockStateList(
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList
    );

static
NTSTATUS
SrvBuildLockState(
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge,
    PLOCKING_ANDX_RANGE              pUnlockRange,
    PLOCKING_ANDX_RANGE              pLockRange,
    PLWIO_SRV_FILE                   pFile,
    ULONG                            ulPid,
    USHORT                           usMid,
    PSRV_LOCK_STATE_SMB_V1*          ppLockState
    );

static
NTSTATUS
SrvRegisterPendingLockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PSRV_LOCK_STATE_SMB_V1          pLockState
    );

static
NTSTATUS
SrvFindLargeUnlockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PLOCKING_ANDX_RANGE_LARGE_FILE  pRangeLarge,
    PSRV_LOCK_STATE_SMB_V1*         ppLockState,
    PUSHORT                         pusLockIndex
    );

static
BOOLEAN
SrvMatchesPendingLargeUnlockState(
    PLOCKING_ANDX_RANGE_LARGE_FILE pCandidateRange,
    PSRV_LOCK_STATE_SMB_V1         pLockState,
    PUSHORT                        pusLockIndex
    );

static
NTSTATUS
SrvFindLargeLockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PLOCKING_ANDX_RANGE_LARGE_FILE  pRangeLarge,
    PSRV_LOCK_STATE_SMB_V1*         ppLockState,
    PUSHORT                         pusLockIndex
    );

static
BOOLEAN
SrvMatchesPendingLargeLockState(
    PLOCKING_ANDX_RANGE_LARGE_FILE pCandidateRange,
    PSRV_LOCK_STATE_SMB_V1         pLockState,
    PUSHORT                        pusLockIndex
    );

static
NTSTATUS
SrvFindUnlockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PLOCKING_ANDX_RANGE             pRange,
    PSRV_LOCK_STATE_SMB_V1*         ppLockState,
    PUSHORT                         pusLockIndex
    );

static
BOOLEAN
SrvMatchesPendingUnlockState(
    PLOCKING_ANDX_RANGE    pCandidateRange,
    PSRV_LOCK_STATE_SMB_V1 pLockState,
    PUSHORT                pusLockIndex
    );

static
NTSTATUS
SrvFindLockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PLOCKING_ANDX_RANGE             pRange,
    PSRV_LOCK_STATE_SMB_V1*         ppLockState,
    PUSHORT                         pusLockIndex
    );

static
BOOLEAN
SrvMatchesPendingLockState(
    PLOCKING_ANDX_RANGE    pCandidateRange,
    PSRV_LOCK_STATE_SMB_V1 pLockState,
    PUSHORT                pusLockIndex
    );

static
NTSTATUS
SrvUnregisterBRLState(
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PSRV_LOCK_STATE_SMB_V1          pLockState
    );

static
NTSTATUS
SrvExecuteLockCancellation(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvCancelLockState_inlock(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

static
VOID
SrvFreeLockTimerRequest_inlock(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

static
VOID
SrvClearLockAsyncState_inlock(
    PLWIO_SRV_TREE         pTree,
    PSRV_LOCK_STATE_SMB_V1 pLockState
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

static
PSRV_LOCK_STATE_SMB_V1
SrvAcquireLockState(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

static
VOID
SrvReleaseLockState(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

static
VOID
SrvFreeLockState(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

static
VOID
SrvConvertLockTimeout(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    );

NTSTATUS
SrvCreatePendingLockStateList(
    PSRV_BYTE_RANGE_LOCK_STATE_LIST* ppBRLStateList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_BYTE_RANGE_LOCK_STATE_LIST),
                    (PVOID*)&pBRLStateList);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pBRLStateList->mutex, NULL);
    pBRLStateList->pMutex = &pBRLStateList->mutex;

    *ppBRLStateList = pBRLStateList;

cleanup:

    return ntStatus;

error:

    *ppBRLStateList = NULL;

    if (pBRLStateList)
    {
        SrvFreePendingLockStateList(pBRLStateList);
    }

    goto cleanup;
}

VOID
SrvFreePendingLockStateListHandle(
    HANDLE hBRLStateList
    )
{
    SrvFreePendingLockStateList((PSRV_BYTE_RANGE_LOCK_STATE_LIST)hBRLStateList);
}

static
VOID
SrvFreePendingLockStateList(
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList
    )
{
    while (pBRLStateList->pHead)
    {
        PSRV_BYTE_RANGE_LOCK_STATE pBRLState = pBRLStateList->pHead;

        pBRLStateList->pHead = pBRLStateList->pHead->pNext;

        // TODO: Cancel the asynchronous state?

        SrvFreeMemory(pBRLState);
    }

    if (pBRLStateList->pMutex)
    {
        pthread_mutex_destroy(&pBRLStateList->mutex);
    }

    SrvFreeMemory(pBRLStateList);
}

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
        SrvAcquireLockState(pLockState);
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

        ntStatus = SrvSetStatSessionInfo(pExecContext, pSession);
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

        SRV_LOG_DEBUG(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_1,
                pSmbRequest->pHeader->command,
                "Lock request: command(%u),uid(%u),mid(%u),pid(%u),tid(%u),"
                "file-id(%u),locks(%u),unlocks(%u),lockType(0x%x),"
                "oplock-level(0x%x),timeout(%u)",
                pSmbRequest->pHeader->command,
                pSmbRequest->pHeader->uid,
                pSmbRequest->pHeader->mid,
                SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                pSmbRequest->pHeader->tid,
                pRequestHeader->usFid,
                pRequestHeader->usNumLocks,
                pRequestHeader->usNumUnlocks,
                pRequestHeader->ucLockType,
                pRequestHeader->ucOplockLevel,
                pRequestHeader->ulTimeout);

        //
        // Check for cancel after logging but before any work to avoid
        // doing extra work in case the request was cancelled before
        // starting processing.
        //
        if (SrvMpxTrackerIsCancelledExecContext(pExecContext))
        {
            ntStatus = STATUS_CANCELLED;
            BAIL_ON_NT_STATUS(ntStatus);
        }

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
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pSmbRequest->pHeader->mid,
                        &pLockState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pLockState;
        pCtxSmb1->pfnStateRelease = &SrvReleaseLockStateHandle;

        SrvAcquireLockState(pLockState);
    }

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    switch (pLockState->stage)
    {
        case SRV_LOCK_STAGE_SMB_V1_INITIAL:

            if (pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_CANCEL_LOCK)
            {
                if (!pLockState->pRequestHeader->usNumLocks &&
                    !pLockState->pRequestHeader->usNumUnlocks)
                {
                    ntStatus = STATUS_INVALID_PARAMETER;
                    BAIL_ON_NT_STATUS(ntStatus);
                }
            }
            else if (pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_OPLOCK_RELEASE)
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

                    ntStatus = SrvAcknowledgeOplockBreak(
                                   pCtxSmb1,
                                   pOplockState,
                                   &pLockState->pRequestHeader->ucOplockLevel,
                                   FALSE);
                    BAIL_ON_NT_STATUS(ntStatus);
                }
                else
                {
                    // Client is trying to ack an oplock that is no longer
                    // valid (due to rundown or it never had one).

                    ntStatus = STATUS_INVALID_OPLOCK_PROTOCOL;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                goto cleanup;
            }
            else
            {
                PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList =
                        (PSRV_BYTE_RANGE_LOCK_STATE_LIST)pCtxSmb1->pFile->hCancellableBRLStateList;

                SrvConvertLockTimeout(pLockState);

                switch (pLockState->pRequestHeader->ulTimeout)
                {
                    case 0:           /* don't wait i.e. fail immediately */

                        break;

                    case ((ULONG)-1): /* wait indefinitely                */

                        ntStatus = SrvRegisterPendingLockState(
                                        pCtxSmb1->pTree,
                                        pBRLStateList,
                                        pLockState);
                        BAIL_ON_NT_STATUS(ntStatus);

                        break;

                    default:

                        {
                            LONG64 llExpiry = 0LL;

                            ntStatus = WireGetCurrentNTTime(&llExpiry);
                            BAIL_ON_NT_STATUS(ntStatus);

                            llExpiry +=
                                (pLockState->pRequestHeader->ulTimeout *
                                 WIRE_FACTOR_MILLISECS_TO_HUNDREDS_OF_NANOSECS);

                            ntStatus = SrvRegisterPendingLockState(
                                            pCtxSmb1->pTree,
                                            pBRLStateList,
                                            pLockState);
                            BAIL_ON_NT_STATUS(ntStatus);

                            ntStatus = SrvTimerPostRequest(
                                            llExpiry,
                                            pLockState,
                                            &SrvLockExpiredCB,
                                            &pLockState->pTimerRequest);
                            BAIL_ON_NT_STATUS(ntStatus);

                            SrvAcquireLockState(pLockState);
                        }

                        break;
                }

                //
                // Check for cancel a final time *after* adding the async state
                // so that a cancel is caught at least here or if async state
                // is cancelled.
                //
                if (SrvMpxTrackerIsCancelledExecContext(pExecContext))
                {
                    ntStatus = STATUS_CANCELLED;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                pLockState->stage = SRV_LOCK_STAGE_SMB_V1_ATTEMPT_LOCK;
            }

            // Intentional fall through

        case SRV_LOCK_STAGE_SMB_V1_ATTEMPT_LOCK:

            ntStatus = pLockState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            if (pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_CANCEL_LOCK)
            {
                ntStatus = SrvExecuteLockCancellation(pExecContext);
            }
            else
            {
                ntStatus = SrvExecuteLockRequest(pExecContext);
            }
            BAIL_ON_NT_STATUS(ntStatus);

            pLockState->stage = SRV_LOCK_STAGE_SMB_V1_DONE;

            // Intentional fall through

        case SRV_LOCK_STAGE_SMB_V1_DONE:

            if (!(pLockState->pRequestHeader->ucLockType &
                    LWIO_LOCK_TYPE_CANCEL_LOCK))
            {
                SrvClearLockAsyncState_inlock(pCtxSmb1->pTree, pLockState);
            }

            SrvFreeLockTimerRequest_inlock(pLockState);

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

        case STATUS_CANCELLED:

            ntStatus = STATUS_FILE_LOCK_CONFLICT;

            // intentional fall through

        case STATUS_LOCK_NOT_GRANTED:

            if (pLockState->pRequestHeader->ulTimeout != 0)
            {
                ntStatus = STATUS_FILE_LOCK_CONFLICT;
            }

            SrvFileSetLastFailedLockOffset(
                    pLockState->pFile,
                    pLockState->llOffset);

            // intentional fall through

        default:

            if (pLockState)
            {
                SrvClearLockAsyncState_inlock(pCtxSmb1->pTree, pLockState);

                SrvFreeLockTimerRequest_inlock(pLockState);

                SrvReleaseLockStateAsync(pLockState);

                SrvClearLocks(pLockState);
            }

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
    ULONG                            ulPid,
    USHORT                           usMid,
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

    pLockState->ulPid      = ulPid;
    pLockState->usMid      = usMid;
    pLockState->ullAsyncId = 0;

    pLockState->pRequestHeader    = pRequestHeader;
    pLockState->pUnlockRangeLarge = pUnlockRangeLarge;
    pLockState->pLockRangeLarge   = pLockRangeLarge;
    pLockState->pUnlockRange      = pUnlockRange;
    pLockState->pLockRange        = pLockRange;

    pLockState->bRequestExclusiveLock =
     !(pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_SHARED_LOCK);

    pLockState->pFile = SrvFileAcquire(pFile);

    pLockState->bExpired   = FALSE;
    pLockState->bCompleted = FALSE;

    LWIO_LOG_DEBUG("Created lock state [0x%08x]", pLockState);

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
SrvRegisterPendingLockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PSRV_LOCK_STATE_SMB_V1          pLockState
    )
{
    NTSTATUS ntStatus   = STATUS_SUCCESS;
    BOOLEAN  bInLock    = FALSE;
    PSRV_BYTE_RANGE_LOCK_STATE pBRLState   = NULL;
    PLWIO_ASYNC_STATE          pAsyncState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_BYTE_RANGE_LOCK_STATE),
                    (PVOID*)&pBRLState);
    BAIL_ON_NT_STATUS(ntStatus);

    pBRLState->ullAsyncId = SrvAsyncStateBuildId(
                                pLockState->ulPid,
                                pLockState->usMid);

    ntStatus = SrvAsyncStateCreate(
                    pBRLState->ullAsyncId,
                    COM_LOCKING_ANDX,
                    pLockState,
                    &SrvCancelLockState,
                    &SrvReleaseLockStateHandle,
                    &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvAcquireLockState(pLockState);

    pLockState->ullAsyncId = pBRLState->ullAsyncId; // publish for cancellation

    ntStatus = SrvTreeAddAsyncState(pTree, pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    if (!pBRLStateList->pHead)
    {
        pBRLStateList->pHead = pBRLStateList->pTail = pBRLState;
    }
    else
    {
        pBRLStateList->pTail->pNext = pBRLState;
        pBRLStateList->pTail = pBRLState;
    }

    pBRLState = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pBRLStateList->mutex);

cleanup:

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    return ntStatus;

error:

    if (pBRLState)
    {
        SrvFreeMemory(pBRLState);
    }

    if (pLockState)
    {
        pLockState->ullAsyncId = 0;
    }

    goto cleanup;
}

static
NTSTATUS
SrvFindLargeUnlockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PLOCKING_ANDX_RANGE_LARGE_FILE  pRangeLarge,
    PSRV_LOCK_STATE_SMB_V1*         ppLockState,
    PUSHORT                         pusLockIndex
    )
{
    NTSTATUS                   ntStatus            = STATUS_SUCCESS;
    BOOLEAN                    bInLock             = FALSE;
    USHORT                     usLockIdx           = 0;
    PLWIO_ASYNC_STATE          pAsyncState         = NULL;
    PSRV_BYTE_RANGE_LOCK_STATE pBRLStateCursor     = NULL;
    PSRV_LOCK_STATE_SMB_V1     pCandidateLockState = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    for (   pBRLStateCursor = pBRLStateList->pHead;
            pBRLStateCursor != NULL;
            pBRLStateCursor = pBRLStateCursor->pNext)
    {
        PSRV_LOCK_STATE_SMB_V1 pLockState = NULL;

        if (pAsyncState)
        {
            SrvAsyncStateRelease(pAsyncState);
            pAsyncState = NULL;
        }

        ntStatus = SrvTreeFindAsyncState(
                        pTree,
                        pBRLStateCursor->ullAsyncId,
                        &pAsyncState);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pLockState = (PSRV_LOCK_STATE_SMB_V1)pAsyncState->hAsyncState;

        if (SrvMatchesPendingLargeUnlockState(
                    pRangeLarge,
                    pLockState,
                    &usLockIdx))
        {
            pCandidateLockState = SrvAcquireLockState(pLockState);

            break;
        }
    }

    if (!pCandidateLockState)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppLockState  = pCandidateLockState;
    *pusLockIndex = usLockIdx;

cleanup:

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    return ntStatus;

error:

    *ppLockState  = NULL;
    *pusLockIndex = 0;

    goto cleanup;
}

static
BOOLEAN
SrvMatchesPendingLargeUnlockState(
    PLOCKING_ANDX_RANGE_LARGE_FILE pCandidateRange,
    PSRV_LOCK_STATE_SMB_V1         pLockState,
    PUSHORT                        pusLockIndex
    )
{
    BOOLEAN bResult     = FALSE;
    USHORT  usLockIndex = 0;

    if (pLockState->pUnlockRangeLarge)
    {
        USHORT iLock             = 0;
        LONG64 llCandidateOffset = 0LL;
        LONG64 llCandidateLength = 0LL;

        llCandidateOffset = (((LONG64)pCandidateRange->ulOffsetHigh) << 32) |
                            ((LONG64)pCandidateRange->ulOffsetLow);

        llCandidateLength = (((LONG64)pCandidateRange->ulLengthHigh) << 32) |
                            ((LONG64)pCandidateRange->ulLengthLow);

        for (iLock = 0; iLock < pLockState->pRequestHeader->usNumUnlocks; iLock++)
        {
            PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo =
                        &pLockState->pUnlockRangeLarge[iLock];
            LONG64 llOffset = 0LL;
            LONG64 llLength = 0LL;

            llOffset = (((LONG64)pLockInfo->ulOffsetHigh) << 32) |
                       ((LONG64)pLockInfo->ulOffsetLow);

            llLength = (((LONG64)pLockInfo->ulLengthHigh) << 32) |
                                   ((LONG64)pLockInfo->ulLengthLow);

            if ((pCandidateRange->usPid == pLockInfo->usPid) &&
                (llCandidateOffset == llOffset) &&
                (llCandidateLength == llLength))
            {
                usLockIndex = iLock;
                bResult = TRUE;
                break;
            }
        }
    }

    *pusLockIndex = usLockIndex;

    return bResult;
}

static
NTSTATUS
SrvFindLargeLockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PLOCKING_ANDX_RANGE_LARGE_FILE  pRangeLarge,
    PSRV_LOCK_STATE_SMB_V1*         ppLockState,
    PUSHORT                         pusLockIndex
    )
{
    NTSTATUS                   ntStatus            = STATUS_SUCCESS;
    BOOLEAN                    bInLock             = FALSE;
    USHORT                     usLockIdx           = 0;
    PLWIO_ASYNC_STATE          pAsyncState         = NULL;
    PSRV_BYTE_RANGE_LOCK_STATE pBRLStateCursor     = NULL;
    PSRV_LOCK_STATE_SMB_V1     pCandidateLockState = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    for (   pBRLStateCursor = pBRLStateList->pHead;
            pBRLStateCursor != NULL;
            pBRLStateCursor = pBRLStateCursor->pNext)
    {
        PSRV_LOCK_STATE_SMB_V1 pLockState = NULL;

        if (pAsyncState)
        {
            SrvAsyncStateRelease(pAsyncState);
            pAsyncState = NULL;
        }

        ntStatus = SrvTreeFindAsyncState(
                        pTree,
                        pBRLStateCursor->ullAsyncId,
                        &pAsyncState);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pLockState = (PSRV_LOCK_STATE_SMB_V1)pAsyncState->hAsyncState;

        if (SrvMatchesPendingLargeLockState(
                    pRangeLarge,
                    pLockState,
                    &usLockIdx))
        {
            pCandidateLockState = SrvAcquireLockState(pLockState);

            break;
        }
    }

    if (!pCandidateLockState)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppLockState  = pCandidateLockState;
    *pusLockIndex = usLockIdx;

cleanup:

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    return ntStatus;

error:

    *ppLockState  = NULL;
    *pusLockIndex = 0;

    goto cleanup;
}

static
BOOLEAN
SrvMatchesPendingLargeLockState(
    PLOCKING_ANDX_RANGE_LARGE_FILE pCandidateRange,
    PSRV_LOCK_STATE_SMB_V1         pLockState,
    PUSHORT                        pusLockIndex
    )
{
    BOOLEAN bResult     = FALSE;
    USHORT  usLockIndex = 0;

    if (pLockState->pLockRangeLarge)
    {
        USHORT iLock             = 0;
        LONG64 llCandidateOffset = 0LL;
        LONG64 llCandidateLength = 0LL;

        llCandidateOffset = (((LONG64)pCandidateRange->ulOffsetHigh) << 32) |
                            ((LONG64)pCandidateRange->ulOffsetLow);

        llCandidateLength = (((LONG64)pCandidateRange->ulLengthHigh) << 32) |
                            ((LONG64)pCandidateRange->ulLengthLow);

        for (iLock = 0; iLock < pLockState->pRequestHeader->usNumLocks; iLock++)
        {
            PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo =
                        &pLockState->pLockRangeLarge[iLock];
            LONG64 llOffset = 0LL;
            LONG64 llLength = 0LL;

            llOffset = (((LONG64)pLockInfo->ulOffsetHigh) << 32) |
                       ((LONG64)pLockInfo->ulOffsetLow);

            llLength = (((LONG64)pLockInfo->ulLengthHigh) << 32) |
                                   ((LONG64)pLockInfo->ulLengthLow);

            if ((pCandidateRange->usPid == pLockInfo->usPid) &&
                (llCandidateOffset == llOffset) &&
                (llCandidateLength == llLength))
            {
                usLockIndex = iLock;
                bResult = TRUE;
                break;
            }
        }
    }

    *pusLockIndex = usLockIndex;

    return bResult;
}

static
NTSTATUS
SrvFindUnlockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PLOCKING_ANDX_RANGE             pRange,
    PSRV_LOCK_STATE_SMB_V1*         ppLockState,
    PUSHORT                         pusLockIndex
    )
{
    NTSTATUS                   ntStatus            = STATUS_SUCCESS;
    BOOLEAN                    bInLock             = FALSE;
    USHORT                     usLockIdx           = 0;
    PLWIO_ASYNC_STATE          pAsyncState         = NULL;
    PSRV_BYTE_RANGE_LOCK_STATE pBRLStateCursor     = NULL;
    PSRV_LOCK_STATE_SMB_V1     pCandidateLockState = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    for (   pBRLStateCursor = pBRLStateList->pHead;
            pBRLStateCursor != NULL;
            pBRLStateCursor = pBRLStateCursor->pNext)
    {
        PSRV_LOCK_STATE_SMB_V1 pLockState = NULL;

        if (pAsyncState)
        {
            SrvAsyncStateRelease(pAsyncState);
            pAsyncState = NULL;
        }

        ntStatus = SrvTreeFindAsyncState(
                        pTree,
                        pBRLStateCursor->ullAsyncId,
                        &pAsyncState);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pLockState = (PSRV_LOCK_STATE_SMB_V1)pAsyncState->hAsyncState;

        if (SrvMatchesPendingUnlockState(pRange, pLockState, &usLockIdx))
        {
            pCandidateLockState = SrvAcquireLockState(pLockState);

            break;
        }
    }

    if (!pCandidateLockState)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppLockState  = pCandidateLockState;
    *pusLockIndex = usLockIdx;

cleanup:

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    return ntStatus;

error:

    *ppLockState  = NULL;
    *pusLockIndex = 0;

    goto cleanup;
}

static
BOOLEAN
SrvMatchesPendingUnlockState(
    PLOCKING_ANDX_RANGE    pCandidateRange,
    PSRV_LOCK_STATE_SMB_V1 pLockState,
    PUSHORT                pusLockIndex
    )
{
    BOOLEAN bResult     = FALSE;
    USHORT  usLockIndex = 0;

    if (pLockState->pUnlockRange)
    {
        USHORT iLock = 0;

        for (iLock = 0; iLock < pLockState->pRequestHeader->usNumUnlocks; iLock++)
        {
            PLOCKING_ANDX_RANGE pLockInfo =
                        &pLockState->pUnlockRange[iLock];

            if ((pCandidateRange->usPid == pLockInfo->usPid) &&
                (pCandidateRange->ulOffset == pLockInfo->ulOffset) &&
                (pCandidateRange->ulLength == pLockInfo->ulLength))
            {
                usLockIndex = iLock;
                bResult = TRUE;
                break;
            }
        }
    }

    *pusLockIndex = usLockIndex;

    return bResult;
}


static
NTSTATUS
SrvFindLockState(
    PLWIO_SRV_TREE                  pTree,
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PLOCKING_ANDX_RANGE             pRange,
    PSRV_LOCK_STATE_SMB_V1*         ppLockState,
    PUSHORT                         pusLockIndex
    )
{
    NTSTATUS                   ntStatus            = STATUS_SUCCESS;
    BOOLEAN                    bInLock             = FALSE;
    USHORT                     usLockIdx           = 0;
    PLWIO_ASYNC_STATE          pAsyncState         = NULL;
    PSRV_BYTE_RANGE_LOCK_STATE pBRLStateCursor     = NULL;
    PSRV_LOCK_STATE_SMB_V1     pCandidateLockState = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    for (   pBRLStateCursor = pBRLStateList->pHead;
            pBRLStateCursor != NULL;
            pBRLStateCursor = pBRLStateCursor->pNext)
    {
        PSRV_LOCK_STATE_SMB_V1 pLockState = NULL;

        if (pAsyncState)
        {
            SrvAsyncStateRelease(pAsyncState);
            pAsyncState = NULL;
        }

        ntStatus = SrvTreeFindAsyncState(
                        pTree,
                        pBRLStateCursor->ullAsyncId,
                        &pAsyncState);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pLockState = (PSRV_LOCK_STATE_SMB_V1)pAsyncState->hAsyncState;

        if (SrvMatchesPendingLockState(pRange, pLockState, &usLockIdx))
        {
            pCandidateLockState = SrvAcquireLockState(pLockState);

            break;
        }
    }

    if (!pCandidateLockState)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppLockState  = pCandidateLockState;
    *pusLockIndex = usLockIdx;

cleanup:

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    return ntStatus;

error:

    *ppLockState  = NULL;
    *pusLockIndex = 0;

    goto cleanup;
}

static
BOOLEAN
SrvMatchesPendingLockState(
    PLOCKING_ANDX_RANGE    pCandidateRange,
    PSRV_LOCK_STATE_SMB_V1 pLockState,
    PUSHORT                pusLockIndex
    )
{
    BOOLEAN bResult     = FALSE;
    USHORT  usLockIndex = 0;

    if (pLockState->pLockRange)
    {
        USHORT iLock = 0;

        for (iLock = 0; iLock < pLockState->pRequestHeader->usNumLocks; iLock++)
        {
            PLOCKING_ANDX_RANGE pLockInfo =
                        &pLockState->pLockRange[iLock];

            if ((pCandidateRange->usPid == pLockInfo->usPid) &&
                (pCandidateRange->ulOffset == pLockInfo->ulOffset) &&
                (pCandidateRange->ulLength == pLockInfo->ulLength))
            {
                usLockIndex = iLock;
                bResult = TRUE;
                break;
            }
        }
    }

    *pusLockIndex = usLockIndex;

    return bResult;
}

static
NTSTATUS
SrvUnregisterBRLState(
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList,
    PSRV_LOCK_STATE_SMB_V1          pLockState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PSRV_BYTE_RANGE_LOCK_STATE pCursor = NULL;
    PSRV_BYTE_RANGE_LOCK_STATE pPrev   = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    pCursor = pBRLStateList->pHead;
    while (pCursor && (pCursor->ullAsyncId != pLockState->ullAsyncId))
    {
        pPrev   = pCursor;
        pCursor = pCursor->pNext;
    }

    if (!pCursor)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pCursor == pBRLStateList->pHead)
    {
        if (pBRLStateList->pHead == pBRLStateList->pTail)
        {
            pBRLStateList->pTail = NULL;
        }

        pBRLStateList->pHead = pBRLStateList->pHead->pNext;
    }
    else if (pCursor == pBRLStateList->pTail)
    {
        pBRLStateList->pTail = pPrev;
        pPrev->pNext = NULL;
    }
    else
    {
        pPrev->pNext = pCursor->pNext;
    }

    pCursor->pNext = NULL;

    SrvFreeMemory(pCursor);

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pBRLStateList->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvExecuteLockCancellation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_LOCK_STATE_SMB_V1     pLockState   = NULL;
    USHORT                     usLockIndex  = 0;
    BOOLEAN                    bInLock      = FALSE;
    PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList = NULL;
    PSRV_LOCK_STATE_SMB_V1     pLockStateToCancel = NULL;

    pLockState = (PSRV_LOCK_STATE_SMB_V1)pCtxSmb1->hState;
    pBRLStateList =
            (PSRV_BYTE_RANGE_LOCK_STATE_LIST)pCtxSmb1->pFile->hCancellableBRLStateList;


    if (pLockState->pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
    {


























































































































        if (pLockState->pUnlockRangeLarge)
        {
            ntStatus = SrvFindLargeUnlockState(
                            pCtxSmb1->pTree,
                            pBRLStateList,
                            &pLockState->pUnlockRangeLarge[0],
                            &pLockStateToCancel,
                            &usLockIndex);
        }
        else if (pLockState->pLockRangeLarge)
        {
            ntStatus = SrvFindLargeLockState(
                            pCtxSmb1->pTree,
                            pBRLStateList,
                            &pLockState->pLockRangeLarge[0],
                            &pLockStateToCancel,
                            &usLockIndex);
        }
        else
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        if (pLockState->pUnlockRange)
        {
            ntStatus = SrvFindUnlockState(
                            pCtxSmb1->pTree,
                            pBRLStateList,
                            &pLockState->pUnlockRange[0],
                            &pLockStateToCancel,
                            &usLockIndex);
        }
        else if (pLockState->pLockRange)
        {
            ntStatus = SrvFindLockState(
                            pCtxSmb1->pTree,
                            pBRLStateList,
                            &pLockState->pLockRange[0],
                            &pLockStateToCancel,
                            &usLockIndex);
        }
        else
        {
            ntStatus = STATUS_INVALID_PARAMETER;
        }
    }
    BAIL_ON_NT_STATUS(ntStatus);

    SrvTreeRemoveAsyncState(
            pCtxSmb1->pTree,
            pLockStateToCancel->ullAsyncId);

    ntStatus = SrvUnregisterBRLState(pBRLStateList, pLockStateToCancel);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bInLock, &pLockStateToCancel->mutex);

    SrvFreeLockTimerRequest_inlock(pLockStateToCancel);

    if (pLockStateToCancel->bExpired)
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!pLockStateToCancel->bCompleted)
    {
        SrvCancelLockState_inlock(pLockStateToCancel);
    }
    else
    {
        ntStatus = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pLockStateToCancel)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pLockStateToCancel->mutex);

        SrvReleaseLockState(pLockStateToCancel);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_NOT_FOUND:

            ntStatus = ERROR_CANCEL_VIOLATION;

            break;

        default:

            break;
    }

    goto cleanup;
}

VOID
SrvCancelLockState(
    HANDLE hLockState
    )
{
    BOOLEAN bInLock = FALSE;
    PSRV_LOCK_STATE_SMB_V1 pLockState = (PSRV_LOCK_STATE_SMB_V1)hLockState;

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    SrvCancelLockState_inlock(pLockState);

    SrvFreeLockTimerRequest_inlock(pLockState);

    LWIO_UNLOCK_MUTEX(bInLock, &pLockState->mutex);
}

static
VOID
SrvCancelLockState_inlock(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
    pLockState->bCancelled = TRUE;

    if (pLockState->pAcb &&
            pLockState->pAcb->AsyncCancelContext)
    {
        IoCancelAsyncCancelContext(
                pLockState->pAcb->AsyncCancelContext);
    }
}

static
VOID
SrvFreeLockTimerRequest_inlock(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pLockState->pTimerRequest)
    {
        PSRV_LOCK_STATE_SMB_V1 pTmpLockState = NULL;

        ntStatus = SrvTimerCancelRequest(pLockState->pTimerRequest,
                                         (PVOID*)&pTmpLockState);

        if (ntStatus != STATUS_NOT_FOUND)
        {
            SrvReleaseLockState(pLockState);
        }

        SrvTimerRelease(pLockState->pTimerRequest);
        pLockState->pTimerRequest = NULL;
    }
}

static
VOID
SrvClearLockAsyncState_inlock(
    PLWIO_SRV_TREE         pTree,
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
    if (pLockState->ullAsyncId)
    {
        NTSTATUS       ntStatus2 = STATUS_SUCCESS;
        PLWIO_SRV_FILE pFile     = pLockState->pFile;

        PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList =
            (PSRV_BYTE_RANGE_LOCK_STATE_LIST)pFile->hCancellableBRLStateList;

        SrvTreeRemoveAsyncState(pTree, pLockState->ullAsyncId);

        pLockState->ullAsyncId = 0;

        ntStatus2 = SrvUnregisterBRLState(pBRLStateList, pLockState);
    }
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

    if (pLockState->bUnlockPending)
    {
        ntStatus = pLockState->ioStatusBlock.Status; // async response status
        BAIL_ON_NT_STATUS(ntStatus);

        pLockState->iUnlock++;
        pLockState->bUnlockPending = FALSE;

        SrvFileRegisterUnlock(pLockState->pFile);
    }

    if (pLockState->bLockPending)
    {
        ntStatus = pLockState->ioStatusBlock.Status; // async response status
        BAIL_ON_NT_STATUS(ntStatus);

        pLockState->iLock++;
        pLockState->bLockPending = FALSE;

        SrvFileRegisterLock(pLockState->pFile);
    }

    if (pLockState->bCancelled || pLockState->bExpired)
    {
        SrvReleaseLockStateAsync(pLockState);

        ntStatus = STATUS_FILE_LOCK_CONFLICT;
        BAIL_ON_NT_STATUS(ntStatus);
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

        SrvFileRegisterUnlock(pLockState->pFile);

        if (!bFailImmediately)
        {
            SrvReleaseLockStateAsync(pLockState); // completed synchronously
        }
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

        SrvFileRegisterLock(pLockState->pFile);

        if (!bFailImmediately)
        {
            SrvReleaseLockStateAsync(pLockState); // completed synchronously
        }
    }

    pLockState->bCompleted = TRUE;

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
    PSRV_LOCK_STATE_SMB_V1 pLockState = (PSRV_LOCK_STATE_SMB_V1)pUserData;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    if (!pLockState->bCompleted)
    {
        pLockState->bExpired = TRUE;

        SrvCancelLockState_inlock(pLockState);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pLockState->mutex);

    SrvReleaseLockState(pLockState);
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

    if (pLockState->pAcb && pLockState->pAcb->AsyncCancelContext)
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
        else
        {
            SrvFileRegisterUnlock(pLockState->pFile);
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
                        pConnection->serverProperties.Capabilities,
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

static
PSRV_LOCK_STATE_SMB_V1
SrvAcquireLockState(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
    LONG NewRefCount = InterlockedIncrement(&pLockState->refCount);

    LWIO_LOG_DEBUG("Acquiring lock state [0x%08x]. New RefCount: %d",
        pLockState, NewRefCount);

    return pLockState;
}

static
VOID
SrvReleaseLockState(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
    LONG NewRefCount = InterlockedDecrement(&pLockState->refCount);

    assert(NewRefCount >= 0);

    LWIO_LOG_DEBUG("Releasing lock state [0x%08x]. New RefCount: %d",
        pLockState, NewRefCount);

    if (NewRefCount == 0)
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
    LWIO_LOG_DEBUG("Freeing lock state [0x%08x]", pLockState);

    if (pLockState->pAcb && pLockState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pLockState->pAcb->AsyncCancelContext);
    }

    SrvFreeLockTimerRequest_inlock(pLockState);

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


static
VOID
SrvConvertLockTimeout(
    PSRV_LOCK_STATE_SMB_V1 pLockState
    )
{
        int i = 0;
        ULONG64 ullOffset = 0;
        const ULONG64 ullMaxNonBlockingLockOffset = 0xEF000000;
        const ULONG ulLockConflictTimeout = 250;
        ULONG64 ullLastFailedLockOffset = -1;

        // If a requested lock has previously conflicted with a held lock
        // we save the offset. If it is requested again in a non-blocking
        // manner we instead give it a small timeout. This throttles clients
        // which are repeatedly polling for the same lock and overloading the
        // server. In addition locks over a certain offset, when the 64-bit is
        // not set, are always given a short timeout.

        if (pLockState->pRequestHeader->ulTimeout != 0)
        {
                goto cleanup;
        }

        ullLastFailedLockOffset =
                SrvFileGetLastFailedLockOffset(pLockState->pFile);

        for (i = 0; i < pLockState->pRequestHeader->usNumLocks; i++)
        {
            if (pLockState->pRequestHeader->ucLockType &
                LWIO_LOCK_TYPE_LARGE_FILES)
            {
                PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo =
                            &pLockState->pLockRangeLarge[i];

                ullOffset = (((ULONG64)pLockInfo->ulOffsetHigh) << 32) |
                            ((ULONG64)pLockInfo->ulOffsetLow);
            }
            else
            {
                PLOCKING_ANDX_RANGE pLockInfo = &pLockState->pLockRange[i];

                ullOffset = pLockInfo->ulOffset;
            }

            if (ullOffset == ullLastFailedLockOffset ||
                (ullOffset >= ullMaxNonBlockingLockOffset &&
                (ullOffset & 0x8000000000000000LL) == 0))

            {
                pLockState->pRequestHeader->ulTimeout = ulLockConflictTimeout;
                break;
            }
        }

cleanup:
        return;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
