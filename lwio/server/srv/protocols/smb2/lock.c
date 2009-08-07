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
 *        lock.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Byte range locking
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
VOID
SrvReleaseLockRequestHandle_SMB_V2(
    HANDLE hState
    );

static
VOID
SrvReleaseLockRequest_SMB_V2(
    PSRV_SMB2_LOCK_REQUEST pLockRequest
    );

static
VOID
SrvFreeLockRequest_SMB_V2(
    PSRV_SMB2_LOCK_REQUEST pLockRequest
    );

static
NTSTATUS
SrvBuildLockRequest_SMB_V2(
    PSRV_EXEC_CONTEXT         pExecContext,
    PSMB2_LOCK_REQUEST_HEADER pRequestHeader,
    PSRV_SMB2_LOCK_REQUEST*   ppLockRequest
    );

static
NTSTATUS
SrvExecuteLockRequest_SMB_V2(
    PSRV_EXEC_CONTEXT      pExecContext,
    PSRV_SMB2_LOCK_REQUEST pLockRequest
    );

static
VOID
SrvExecuteLockContextAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvClearLocks_SMB_V2(
    PSRV_SMB2_LOCK_REQUEST pLockRequest
    );

static
VOID
SrvClearLocks_SMB_V2_inlock(
    PSRV_SMB2_LOCK_REQUEST pLockRequest
    );

static
NTSTATUS
SrvLockFile_SMB_V2_inlock(
    PSRV_SMB2_LOCK_CONTEXT pLockContext
    );

static
NTSTATUS
SrvUnlockFile_SMB_V2_inlock(
    PSRV_SMB2_LOCK_CONTEXT pLockContext
    );

static
NTSTATUS
SrvBuildLockResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessLock_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus       = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection    = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2       = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg           = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
    PSMB2_LOCK_REQUEST_HEADER  pRequestHeader = NULL; // Do not free
    PLWIO_SRV_SESSION_2        pSession       = NULL;
    PLWIO_SRV_TREE_2           pTree          = NULL;
    PLWIO_SRV_FILE_2           pFile          = NULL;
    PSRV_SMB2_LOCK_REQUEST     pLockRequest   = NULL;
    BOOLEAN                    bInLock        = FALSE;

    pLockRequest = (PSRV_SMB2_LOCK_REQUEST)pCtxSmb2->hState;

    if (pLockRequest)
    {
        InterlockedIncrement(&pLockRequest->refCount);
    }
    else
    {
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

        ntStatus = SMB2UnmarshalLockRequest(pSmbRequest, &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pRequestHeader->usLockCount != 1)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SrvTree2FindFile_SMB_V2(
                            pCtxSmb2,
                            pTree,
                            &pRequestHeader->fid,
                            &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildLockRequest_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            &pLockRequest);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pLockRequest;
        pCtxSmb2->pfnStateRelease = &SrvReleaseLockRequestHandle_SMB_V2;
        InterlockedIncrement(&pLockRequest->refCount);

        ntStatus = SrvExecuteLockRequest_SMB_V2(pExecContext, pLockRequest);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

    if (pLockRequest->bComplete)
    {
        ntStatus = pLockRequest->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

        ntStatus = SrvBuildLockResponse_SMB_V2(pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);
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

    if (pLockRequest)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

        SrvReleaseLockRequest_SMB_V2(pLockRequest);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildLockRequest_SMB_V2(
    PSRV_EXEC_CONTEXT         pExecContext,
    PSMB2_LOCK_REQUEST_HEADER pRequestHeader,
    PSRV_SMB2_LOCK_REQUEST*   ppLockRequest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2       = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg           = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
    LONG                       iCtx           = 0;
    LONG                       iLock          = 0;
    PSMB2_LOCK                 pLockArray     = NULL;
    PSRV_SMB2_LOCK_REQUEST     pLockRequest   = NULL;

    ntStatus = SrvAllocateMemory(
                        sizeof(SRV_SMB2_LOCK_REQUEST),
                        (PVOID*)&pLockRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pLockRequest->mutex, NULL);
    pLockRequest->pMutex = &pLockRequest->mutex;

    pLockRequest->refCount = 1;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SMB2_LOCK_CONTEXT) * pRequestHeader->usLockCount,
                    (PVOID*)&pLockRequest->pLockContexts);
    BAIL_ON_NT_STATUS(ntStatus);

    pLockRequest->lPendingContexts = pRequestHeader->usLockCount;
    pLockRequest->ulNumContexts = pRequestHeader->usLockCount;

    pLockArray = &pRequestHeader->locks[0];
    for (iLock = 0; iCtx < pLockRequest->ulNumContexts; iCtx++, iLock++)
    {
        PSMB2_LOCK pLock = &pLockArray[iLock];
        PSRV_SMB2_LOCK_CONTEXT pContext = &pLockRequest->pLockContexts[iCtx++];

        pContext->lockInfo = *pLock;

        pContext->ulKey = pSmbRequest->pHeader->ulPid;

        pContext->acb.Callback = &SrvExecuteLockContextAsyncCB_SMB_V2;
        pContext->acb.CallbackContext = pContext;

        if (!(pLock->ulFlags & SMB2_LOCK_FLAGS_FAIL_IMMEDIATELY))
        {
            pContext->pAcb = &pContext->acb;
        }

        pContext->pLockRequest = pLockRequest;
        pContext->pExecContext = pExecContext;

        pContext->ioStatusBlock.Status = STATUS_NOT_LOCKED;
    }

    *ppLockRequest = pLockRequest;

cleanup:

    return ntStatus;

error:

    *ppLockRequest = NULL;

    if (pLockRequest)
    {
        SrvReleaseLockRequest_SMB_V2(pLockRequest);
    }

    goto cleanup;
}

static
VOID
SrvReleaseLockRequestHandle_SMB_V2(
    HANDLE hState
    )
{
    SrvReleaseLockRequest_SMB_V2((PSRV_SMB2_LOCK_REQUEST)hState);
}

static
VOID
SrvReleaseLockRequest_SMB_V2(
    PSRV_SMB2_LOCK_REQUEST pLockRequest
    )
{
    if (InterlockedDecrement(&pLockRequest->refCount) == 0)
    {
        SrvFreeLockRequest_SMB_V2(pLockRequest);
    }
}

static
VOID
SrvFreeLockRequest_SMB_V2(
    PSRV_SMB2_LOCK_REQUEST pLockRequest
    )
{
    if (pLockRequest->pMutex)
    {
        pthread_mutex_destroy(&pLockRequest->mutex);
    }

    SRV_SAFE_FREE_MEMORY(pLockRequest->pLockContexts);
}

static
NTSTATUS
SrvExecuteLockRequest_SMB_V2(
    PSRV_EXEC_CONTEXT      pExecContext,
    PSRV_SMB2_LOCK_REQUEST pLockRequest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bAsync = FALSE;
    ULONG iLock = 0;
    BOOLEAN bInLock = FALSE;

    /*
     * Case 1: If requested to fail immediately, lock synchronously
     * Case 2: If requested to wait indefinitely, lock asynchronously
     */

    LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

    // Unlock requests
    for (iLock = 0; iLock < pLockRequest->ulNumContexts; iLock++)
    {
        PSRV_SMB2_LOCK_CONTEXT pContext = &pLockRequest->pLockContexts[iLock];

        if (pContext->lockInfo.ulFlags & SMB2_LOCK_FLAGS_UNLOCK)
        {
            ntStatus = SrvUnlockFile_SMB_V2_inlock(pContext);
        }
        if (ntStatus == STATUS_PENDING)
        {
            // Asynchronous requests can complete synchronously.
            bAsync = TRUE;
            ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Lock requests
    for (iLock = 0; iLock < pLockRequest->ulNumContexts; iLock++)
    {
        PSRV_SMB2_LOCK_CONTEXT pContext = &pLockRequest->pLockContexts[iLock];

        // Only one of these must be set
        if (!((pContext->lockInfo.ulFlags & SMB2_LOCK_FLAGS_SHARED_LOCK)^
              (pContext->lockInfo.ulFlags & SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK)))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (!(pContext->lockInfo.ulFlags & SMB2_LOCK_FLAGS_UNLOCK))
        {
            ntStatus = SrvLockFile_SMB_V2_inlock(pContext);
        }
        if (ntStatus == STATUS_PENDING)
        {
            // Asynchronous requests can complete synchronously.
            bAsync = TRUE;
            ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!bAsync)
    {
        pLockRequest->bComplete = TRUE;
        pLockRequest->ioStatusBlock.Status = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = STATUS_PENDING;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

    if (!bAsync)
    {
        SrvClearLocks_SMB_V2(pLockRequest);
    }

    goto cleanup;
}

static
VOID
SrvExecuteLockContextAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    PSRV_SMB2_LOCK_CONTEXT pLockContext = (PSRV_SMB2_LOCK_CONTEXT)pContext;
    PSRV_SMB2_LOCK_REQUEST pLockRequest = pLockContext->pLockRequest;
    BOOLEAN bInLock = FALSE;

    if (InterlockedDecrement(&pLockRequest->lPendingContexts) == 0)
    {
        NTSTATUS ntStatus = STATUS_SUCCESS;
        BOOLEAN bSuccess = TRUE;
        LONG iCtx = 0;

        LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

        for (; iCtx < pLockRequest->ulNumContexts; iCtx++)
        {
            PSRV_SMB2_LOCK_CONTEXT pIter = &pLockRequest->pLockContexts[iCtx];

            if (pIter->ioStatusBlock.Status != STATUS_SUCCESS)
            {
                bSuccess = FALSE;
                pLockRequest->ioStatusBlock = pIter->ioStatusBlock;

                break;
            }
        }

        pLockRequest->bComplete = TRUE;

        LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

        ntStatus = SrvProdConsEnqueue(
                        gProtocolGlobals_SMB_V2.pWorkQueue,
                        pLockContext->pExecContext);
        if (ntStatus != STATUS_SUCCESS)
        {
            LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                            ntStatus);
        }
    }

    if (pLockContext->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pLockContext->pAcb->AsyncCancelContext);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

    SrvReleaseLockRequest_SMB_V2(pLockRequest);
}

static
VOID
SrvClearLocks_SMB_V2(
    PSRV_SMB2_LOCK_REQUEST pLockRequest
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

    SrvClearLocks_SMB_V2_inlock(pLockRequest);

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);
}

static
VOID
SrvClearLocks_SMB_V2_inlock(
    PSRV_SMB2_LOCK_REQUEST pLockRequest
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
    for (iCtx = 0; iCtx < pLockRequest->ulNumContexts; iCtx++)
    {
        PSRV_SMB2_LOCK_CONTEXT pLockContext = &pLockRequest->pLockContexts[iCtx];

        if (!(pLockContext->lockInfo.ulFlags & SMB2_LOCK_FLAGS_UNLOCK) &&
            (pLockContext->ioStatusBlock.Status == STATUS_SUCCESS))
        {
            NTSTATUS ntStatus1 = STATUS_SUCCESS;

            pLockContext->pAcb = NULL; // make it synchronous

            ntStatus1 = SrvUnlockFile_SMB_V2_inlock(pLockContext);
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
SrvLockFile_SMB_V2_inlock(
    PSRV_SMB2_LOCK_CONTEXT pLockContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_EXEC_CONTEXT pExecContext = pLockContext->pExecContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    LONG64   llOffset = pLockContext->lockInfo.ullFileOffset;
    LONG64   llLength = pLockContext->lockInfo.ullByteRange;

    ntStatus = IoLockFile(
                    pCtxSmb2->pFile->hFile,
                    pLockContext->pAcb,
                    &pLockContext->ioStatusBlock,
                    llOffset,
                    llLength,
                    pLockContext->ulKey,
                    (pLockContext->lockInfo.ulFlags &
                                            SMB2_LOCK_FLAGS_FAIL_IMMEDIATELY),
                    (pLockContext->lockInfo.ulFlags &
                                    SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK));
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
SrvUnlockFile_SMB_V2_inlock(
    PSRV_SMB2_LOCK_CONTEXT pLockContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_EXEC_CONTEXT pExecContext = pLockContext->pExecContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    LONG64   llOffset = pLockContext->lockInfo.ullFileOffset;
    LONG64   llLength = pLockContext->lockInfo.ullByteRange;

    ntStatus = IoUnlockFile(
                    pCtxSmb2->pFile->hFile,
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
SrvBuildLockResponse_SMB_V2(
    PSRV_EXEC_CONTEXT      pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SMB2MarshalHeader(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM2_LOCK,
                0,
                1,
                pSmbRequest->pHeader->ulPid,
                pSmbRequest->pHeader->ullCommandSequence,
                pCtxSmb2->pTree->ulTid,
                pCtxSmb2->pSession->ullUid,
                STATUS_SUCCESS,
                TRUE,
                pSmbRequest->pHeader->ulFlags & SMB2_FLAGS_RELATED_OPERATION,
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
                    (PSRV_SMB2_LOCK_REQUEST)pCtxSmb2->hState,
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

