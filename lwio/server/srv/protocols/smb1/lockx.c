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
SrvBuildLockRequest(
    PSRV_EXEC_CONTEXT                pExecContext,
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge,
    PLOCKING_ANDX_RANGE              pUnlockRange,
    PLOCKING_ANDX_RANGE              pLockRange,
    PSRV_SMB_LOCK_REQUEST*           ppLockRequest
    );

static
VOID
SrvReleaseLockRequestState(
    HANDLE hState
    );

static
VOID
SrvFreeLockRequest(
    PSRV_SMB_LOCK_REQUEST pLockRequest
    );

static
NTSTATUS
SrvExecuteLockRequest(
    PSRV_EXEC_CONTEXT     pExecContext,
    PSRV_SMB_LOCK_REQUEST pLockRequest
    );

static
VOID
SrvLockRequestExpiredCB(
    PSRV_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    );

static
VOID
SrvExecuteLockContextAsyncCB(
    PVOID pContext
    );

static
VOID
SrvClearLocks(
    PSRV_SMB_LOCK_REQUEST pLockRequest
    );

static
VOID
SrvClearLocks_inlock(
    PSRV_SMB_LOCK_REQUEST pLockRequest
    );

static
NTSTATUS
SrvUnlockFile_inlock(
    PSRV_SMB_LOCK_CONTEXT pLockContext
    );

static
NTSTATUS
SrvLockFile_inlock(
    PSRV_SMB_LOCK_CONTEXT pLockContext
    );

static
NTSTATUS
SrvBuildLockingAndXResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessLockAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader = NULL;  // Do not free
    PLOCKING_ANDX_RANGE              pUnlockRange = NULL;    // Do not free
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge = NULL; // Do not free
    PLOCKING_ANDX_RANGE              pLockRange = NULL;      // Do not free
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge = NULL; // Do not free
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    PLWIO_SRV_FILE    pFile = NULL;
    PSRV_SMB_LOCK_REQUEST pLockRequest = NULL;
    BOOLEAN bInLock = FALSE;

    pLockRequest = (PSRV_SMB_LOCK_REQUEST)(pCtxSmb1->hState);

    if (pLockRequest)
    {
        InterlockedIncrement(&pLockRequest->refCount);
    }
    else
    {
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

        ntStatus = SrvBuildLockRequest(
                            pExecContext,
                            pRequestHeader,
                            pUnlockRangeLarge,
                            pLockRangeLarge,
                            pUnlockRange,
                            pLockRange,
                            &pLockRequest);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pLockRequest;
        pCtxSmb1->pfnStateRelease = &SrvReleaseLockRequestState;
        InterlockedIncrement(&pLockRequest->refCount);

        ntStatus = SrvExecuteLockRequest(pExecContext, pLockRequest);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pLockRequest->bExpired && !pLockRequest->bResponseSent)
    {
        pLockRequest->bResponseSent = TRUE;

        ntStatus = STATUS_FILE_LOCK_CONFLICT;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

    if (pLockRequest->bCompleted && !pLockRequest->bResponseSent)
    {
        pLockRequest->bResponseSent = TRUE;

        ntStatus = pLockRequest->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

        ntStatus = SrvBuildLockingAndXResponse(pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);
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

    if (pLockRequest)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

        SrvReleaseLockRequest(pLockRequest);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildLockRequest(
    PSRV_EXEC_CONTEXT                pExecContext,
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pUnlockRangeLarge,
    PLOCKING_ANDX_RANGE_LARGE_FILE   pLockRangeLarge,
    PLOCKING_ANDX_RANGE              pUnlockRange,
    PLOCKING_ANDX_RANGE              pLockRange,
    PSRV_SMB_LOCK_REQUEST*           ppLockRequest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LONG lNumContexts = 0;
    LONG iContext = 0;
    LONG iLock = 0;
    PSRV_SMB_LOCK_REQUEST pLockRequest = NULL;

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

    ntStatus = SrvAllocateMemory(
                        sizeof(SRV_SMB_LOCK_REQUEST),
                        (PVOID*)&pLockRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pLockRequest->mutex, NULL);
    pLockRequest->pMutex = &pLockRequest->mutex;

    pLockRequest->refCount = 1;

    pLockRequest->ulTimeout = pRequestHeader->ulTimeout;

    pLockRequest->usNumUnlocks = pRequestHeader->usNumUnlocks;
    pLockRequest->usNumLocks = pRequestHeader->usNumLocks;

    lNumContexts = pLockRequest->usNumUnlocks + pLockRequest->usNumLocks;
    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SMB_LOCK_CONTEXT) * lNumContexts,
                    (PVOID*)&pLockRequest->pLockContexts);
    BAIL_ON_NT_STATUS(ntStatus);

    pLockRequest->lPendingContexts = lNumContexts;

    for (iLock = 0; iContext < pLockRequest->usNumUnlocks; iContext++, iLock++)
    {
        PSRV_SMB_LOCK_CONTEXT pContext = &pLockRequest->pLockContexts[iContext];

        pContext->operation = SRV_SMB_UNLOCK;

        pContext->bExclusiveLock =
            !(pRequestHeader->ucLockType & LWIO_LOCK_TYPE_SHARED_LOCK);

        if (pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
        {
            pContext->lockType = SRV_SMB_LOCK_TYPE_LARGE;

            pContext->lockInfo.largeFileRange = pUnlockRangeLarge[iLock];

            pContext->ulKey = pContext->lockInfo.largeFileRange.usPid;
        }
        else
        {
            pContext->lockType = SRV_SMB_LOCK_TYPE_REGULAR;

            pContext->lockInfo.regularRange = pUnlockRange[iLock];

            pContext->ulKey = pContext->lockInfo.regularRange.usPid;
        }

        pContext->acb.Callback = &SrvExecuteLockContextAsyncCB;
        pContext->acb.CallbackContext = pContext;

        if (pLockRequest->ulTimeout)
        {
            pContext->pAcb = &pContext->acb;
        }

        pContext->pLockRequest = pLockRequest;

        pContext->ioStatusBlock.Status = STATUS_NOT_LOCKED;
    }

    for (iLock = 0; iContext < lNumContexts; iContext++, iLock++)
    {
        PSRV_SMB_LOCK_CONTEXT pContext = &pLockRequest->pLockContexts[iContext];

        pContext->operation = SRV_SMB_LOCK;

        pContext->bExclusiveLock =
            !(pRequestHeader->ucLockType & LWIO_LOCK_TYPE_SHARED_LOCK);

        if (pRequestHeader->ucLockType & LWIO_LOCK_TYPE_LARGE_FILES)
        {
            pContext->lockType = SRV_SMB_LOCK_TYPE_LARGE;

            pContext->lockInfo.largeFileRange = pLockRangeLarge[iLock];

            pContext->ulKey = pContext->lockInfo.largeFileRange.usPid;
        }
        else
        {
            pContext->lockType = SRV_SMB_LOCK_TYPE_REGULAR;

            pContext->lockInfo.regularRange = pLockRange[iLock];

            pContext->ulKey = pContext->lockInfo.regularRange.usPid;
        }

        pContext->acb.Callback = &SrvExecuteLockContextAsyncCB;
        pContext->acb.CallbackContext = pContext;

        if (pLockRequest->ulTimeout)
        {
            pContext->pAcb = &pContext->acb;
        }

        pContext->pLockRequest = pLockRequest;

        pContext->ioStatusBlock.Status = STATUS_NOT_LOCKED;
    }

    pLockRequest->bExpired = FALSE;

    pLockRequest->pExecContext_async = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    *ppLockRequest = pLockRequest;

cleanup:

    return ntStatus;

error:

    *ppLockRequest = NULL;

    if (pLockRequest)
    {
        SrvReleaseLockRequest(pLockRequest);
    }

    goto cleanup;
}

static
VOID
SrvReleaseLockRequestState(
    HANDLE hState
    )
{
    SrvReleaseLockRequest((PSRV_SMB_LOCK_REQUEST)hState);
}

VOID
SrvReleaseLockRequest(
    PSRV_SMB_LOCK_REQUEST pLockRequest
    )
{
    if (InterlockedDecrement(&pLockRequest->refCount) == 0)
    {
        SrvFreeLockRequest(pLockRequest);
    }
}

static
VOID
SrvFreeLockRequest(
    PSRV_SMB_LOCK_REQUEST pLockRequest
    )
{
    if (pLockRequest->pMutex)
    {
        pthread_mutex_destroy(&pLockRequest->mutex);
    }

    if (pLockRequest->pTimerRequest)
    {
        SrvTimerCancelRequest(pLockRequest->pTimerRequest);
        SrvTimerRelease(pLockRequest->pTimerRequest);
    }

    if (pLockRequest->pExecContext_timer)
    {
        SrvReleaseExecContext(pLockRequest->pExecContext_timer);
    }

    if (pLockRequest->pExecContext_async)
    {
        SrvReleaseExecContext(pLockRequest->pExecContext_async);
    }

    SRV_SAFE_FREE_MEMORY(pLockRequest->pLockContexts);
    SRV_SAFE_FREE_MEMORY(pLockRequest);
}

static
NTSTATUS
SrvExecuteLockRequest(
    PSRV_EXEC_CONTEXT     pExecContext,
    PSRV_SMB_LOCK_REQUEST pLockRequest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bFailImmediately = (pLockRequest->ulTimeout == 0);
    BOOLEAN  bWaitIndefinitely = (pLockRequest->ulTimeout == (ULONG)-1);
    BOOLEAN  bAsync = FALSE;
    ULONG iLock = 0;
    LONG  lNumContexts = 0;
    BOOLEAN bInLock = FALSE;

    /*
     * Case 1: If requested to fail immediately, lock synchronously
     * Case 2: If requested to wait indefinitely, lock asynchronously
     * Case 3: If requested to wait for a specific time, lock asynchronously and
     *         register a timer call back
     */

    LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

    lNumContexts = pLockRequest->usNumUnlocks + pLockRequest->usNumLocks;

    for (iLock = 0; iLock < lNumContexts; iLock++)
    {
        PSRV_SMB_LOCK_CONTEXT pContext = &pLockRequest->pLockContexts[iLock];

        switch (pContext->operation)
        {
            case SRV_SMB_UNLOCK:

                    ntStatus = SrvUnlockFile_inlock(pContext);

                    break;

            case SRV_SMB_LOCK:

                    ntStatus = SrvLockFile_inlock(pContext);

                    break;
        }
        if (ntStatus == STATUS_PENDING)
        {
            // Asynchronous requests can complete synchronously.
            bAsync = TRUE;
            ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!bFailImmediately && !bWaitIndefinitely && bAsync)
    {
        LONG64 llExpiry = 0LL;

        llExpiry = (time(NULL) +
                    (pLockRequest->ulTimeout/1000) + 11644473600LL) * 10000000LL;

        pLockRequest->pExecContext_timer = pExecContext;
        InterlockedIncrement(&pExecContext->refCount);

        ntStatus = SrvTimerPostRequest(
                        llExpiry,
                        pExecContext,
                        &SrvLockRequestExpiredCB,
                        &pLockRequest->pTimerRequest);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = STATUS_PENDING;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (bFailImmediately || !bAsync)
    {
        pLockRequest->bCompleted = TRUE;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

    switch (ntStatus)
    {
        case STATUS_PENDING:

            break;

        default:

            SrvClearLocks(pLockRequest);

            break;
    }

    goto cleanup;
}

static
VOID
SrvLockRequestExpiredCB(
    PSRV_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pExecContext = (PSRV_EXEC_CONTEXT)pUserData;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_SMB_LOCK_REQUEST pLockRequest = (PSRV_SMB_LOCK_REQUEST)pCtxSmb1->hState;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

    if (!pLockRequest->bCompleted)
    {
        pLockRequest->bExpired = TRUE;
    }

    pLockRequest->pExecContext_timer = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

    ntStatus = SrvProdConsEnqueue(
                    gProtocolGlobals_SMB_V1.pWorkQueue,
                    pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return;

error:

    LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                   ntStatus);

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    goto cleanup;
}

static
VOID
SrvExecuteLockContextAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SMB_LOCK_CONTEXT pLockContext = (PSRV_SMB_LOCK_CONTEXT)pContext;
    PSRV_SMB_LOCK_REQUEST pLockRequest = pLockContext->pLockRequest;
    PSRV_EXEC_CONTEXT     pExecContext = NULL;
    BOOLEAN bInLock = FALSE;

    if (InterlockedDecrement(&pLockRequest->lPendingContexts) == 0)
    {
        BOOLEAN bSuccess = TRUE;
        LONG iCtx = 0;

        LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

        for (; iCtx < pLockRequest->usNumLocks; iCtx++)
        {
            PSRV_SMB_LOCK_CONTEXT pIter = &pLockRequest->pLockContexts[iCtx];

            if (pIter->ioStatusBlock.Status != STATUS_SUCCESS)
            {
                bSuccess = FALSE;

                pLockRequest->ioStatusBlock = pIter->ioStatusBlock;

                break;
            }
        }

        if (bSuccess)
        {
            pLockRequest->ioStatusBlock.Status = STATUS_SUCCESS;
        }
        else
        {
            SrvClearLocks_inlock(pLockRequest);
        }

        pLockRequest->bCompleted = TRUE;

        pExecContext = pLockRequest->pExecContext_async;
        pLockRequest->pExecContext_async = NULL;

        LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

        ntStatus = SrvProdConsEnqueue(
                        gProtocolGlobals_SMB_V1.pWorkQueue,
                        pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pLockContext->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pLockContext->pAcb->AsyncCancelContext);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

    SrvReleaseLockRequest(pLockRequest);

    return;

error:

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                     ntStatus);

    goto cleanup;
}

static
VOID
SrvClearLocks(
    PSRV_SMB_LOCK_REQUEST pLockRequest
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

    SrvClearLocks_inlock(pLockRequest);

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);
}

static
VOID
SrvClearLocks_inlock(
    PSRV_SMB_LOCK_REQUEST pLockRequest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG   iCtx = 0;

    if (InterlockedRead(&pLockRequest->lPendingContexts) == 0)
    {
        LWIO_LOG_ERROR("Attempt to clear lock request with pending contexts.");
        ntStatus = STATUS_INTERNAL_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Note: this routine must always be called on error, after all the actors
    //       are done processing the request.
    for (iCtx = pLockRequest->usNumUnlocks;
         iCtx < (pLockRequest->usNumUnlocks + pLockRequest->usNumLocks);
         iCtx++)
    {
        PSRV_SMB_LOCK_CONTEXT pLockContext = &pLockRequest->pLockContexts[iCtx];

        if (pLockContext->ioStatusBlock.Status == STATUS_SUCCESS)
        {
            NTSTATUS ntStatus1 = STATUS_SUCCESS;

            pLockContext->pAcb = NULL; // make it synchronous

            ntStatus1 = SrvUnlockFile_inlock(pLockContext);
            if (ntStatus1)
            {
                LWIO_LOG_ERROR("Failed in unlock. error code [%d]", ntStatus1);
            }
        }
    }

error:

    return;
}

static
NTSTATUS
SrvLockFile_inlock(
    PSRV_SMB_LOCK_CONTEXT pLockContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_EXEC_CONTEXT pExecContext = pLockContext->pLockRequest->pExecContext_async;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1 = pProtocolContext->pSmb1Context;
    LONG64   llOffset = 0;
    LONG64   llLength = 0;

    switch (pLockContext->lockType)
    {
        case SRV_SMB_LOCK_TYPE_LARGE:
            {
                PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo =
                                        &pLockContext->lockInfo.largeFileRange;

                llOffset = (((LONG64)pLockInfo->ulOffsetHigh) << 32) |
                           ((LONG64)pLockInfo->ulOffsetLow);

                llLength = (((LONG64)pLockInfo->ulLengthHigh) << 32) |
                           ((LONG64)pLockInfo->ulLengthLow);
            }

            break;

        case SRV_SMB_LOCK_TYPE_REGULAR:
            {
                PLOCKING_ANDX_RANGE pLockInfo =
                                        &pLockContext->lockInfo.regularRange;

                llOffset = pLockInfo->ulOffset;
                llLength = pLockInfo->ulLength;
            }

            break;

        case SRV_SMB_LOCK_TYPE_UNKNOWN:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    ntStatus = IoLockFile(
                    pCtxSmb1->pFile->hFile,
                    pLockContext->pAcb,
                    &pLockContext->ioStatusBlock,
                    llOffset,
                    llLength,
                    pLockContext->ulKey,
                    (pLockContext->pLockRequest->ulTimeout == 0),
                    pLockContext->bExclusiveLock);
    BAIL_ON_NT_STATUS(ntStatus);

    // Synchronous case
    InterlockedDecrement(&pLockContext->pLockRequest->lPendingContexts);

cleanup:

    return ntStatus;

error:

    // Asynchronous case
    if (pLockContext->pAcb && (ntStatus == STATUS_PENDING))
    {
        InterlockedIncrement(&pLockContext->pLockRequest->refCount);
    }

    goto cleanup;
}

static
NTSTATUS
SrvUnlockFile_inlock(
    PSRV_SMB_LOCK_CONTEXT pLockContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_EXEC_CONTEXT pExecContext = pLockContext->pLockRequest->pExecContext_async;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1 = pProtocolContext->pSmb1Context;
    LONG64   llOffset = 0;
    LONG64   llLength = 0;

    switch (pLockContext->lockType)
    {
        case SRV_SMB_LOCK_TYPE_LARGE:
            {
                PLOCKING_ANDX_RANGE_LARGE_FILE pLockInfo =
                                        &pLockContext->lockInfo.largeFileRange;

                llOffset = (((LONG64)pLockInfo->ulOffsetHigh) << 32) |
                           ((LONG64)pLockInfo->ulOffsetLow);

                llLength = (((LONG64)pLockInfo->ulLengthHigh) << 32) |
                           ((LONG64)pLockInfo->ulLengthLow);
            }

            break;

        case SRV_SMB_LOCK_TYPE_REGULAR:
            {
                PLOCKING_ANDX_RANGE pLockInfo =
                                        &pLockContext->lockInfo.regularRange;

                llOffset = pLockInfo->ulOffset;
                llLength = pLockInfo->ulLength;
            }

            break;

        case SRV_SMB_LOCK_TYPE_UNKNOWN:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    ntStatus = IoUnlockFile(
                    pCtxSmb1->pFile->hFile,
                    pLockContext->pAcb,
                    &pLockContext->ioStatusBlock,
                    llOffset,
                    llLength,
                    pLockContext->ulKey);
    BAIL_ON_NT_STATUS(ntStatus);

    // Synchronous case
    InterlockedDecrement(&pLockContext->pLockRequest->lPendingContexts);

cleanup:

    return ntStatus;

error:

    // Asynchronous case
    if (pLockContext->pAcb && (ntStatus == STATUS_PENDING))
    {
        InterlockedIncrement(&pLockContext->pLockRequest->refCount);
    }

    goto cleanup;
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
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;

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
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 2;

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

