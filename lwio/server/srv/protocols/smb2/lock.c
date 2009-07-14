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
    PLWIO_SRV_CONNECTION      pConnection,
    PLWIO_SRV_FILE_2          pFile,
    PSMB2_MESSAGE             pSmbRequest,
    PSMB2_LOCK_REQUEST_HEADER pRequestHeader,
    PSRV_SMB2_LOCK_REQUEST*   ppLockRequest
    );

static
NTSTATUS
SrvExecuteLockRequest_SMB_V2(
    PSRV_SMB2_LOCK_REQUEST pLockRequest,
    PSMB_PACKET            pSmbResponse
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
    PSRV_SMB2_LOCK_REQUEST pLockRequest,
    PSMB_PACKET            pSmbResponse
    );

NTSTATUS
SrvProcessLock_SMB_V2(
    IN     PLWIO_SRV_CONNECTION pConnection,
    IN     PSMB2_MESSAGE        pSmbRequest,
    IN OUT PSMB_PACKET          pSmbResponse
    )
{
    NTSTATUS                  ntStatus = STATUS_SUCCESS;
    PSMB2_LOCK_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PLWIO_SRV_SESSION_2       pSession = NULL;
    PLWIO_SRV_TREE_2          pTree = NULL;
    PLWIO_SRV_FILE_2          pFile = NULL;
    PSRV_SMB2_LOCK_REQUEST    pLockRequest = NULL;

    ntStatus = SrvConnection2FindSession(
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindTree(
                    pSession,
                    pSmbRequest->pHeader->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2UnmarshalLockRequest(
                    pSmbRequest,
                    &pRequestHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pRequestHeader->usLockCount != 1)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvTree2FindFile(
                        pTree,
                        pRequestHeader->fid.ullVolatileId,
                        &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildLockRequest_SMB_V2(
                        pConnection,
                        pFile,
                        pSmbRequest,
                        pRequestHeader,
                        &pLockRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvExecuteLockRequest_SMB_V2(pLockRequest, pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

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
        SrvReleaseLockRequest_SMB_V2(pLockRequest);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildLockRequest_SMB_V2(
    PLWIO_SRV_CONNECTION      pConnection,
    PLWIO_SRV_FILE_2          pFile,
    PSMB2_MESSAGE             pSmbRequest,
    PSMB2_LOCK_REQUEST_HEADER pRequestHeader,
    PSRV_SMB2_LOCK_REQUEST*   ppLockRequest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LONG iCtx = 0;
    LONG iLock = 0;
    PSMB2_LOCK pLockArray = NULL;
    PSRV_SMB2_LOCK_REQUEST pLockRequest = NULL;

    ntStatus = SrvAllocateMemory(
                        sizeof(SRV_SMB2_LOCK_REQUEST),
                        (PVOID*)&pLockRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pLockRequest->mutex, NULL);
    pLockRequest->pMutex = &pLockRequest->mutex;

    pLockRequest->refCount = 1;

    pLockRequest->pFile = pFile;
    InterlockedIncrement(&pFile->refcount);

    pLockRequest->pConnection = pConnection;
    InterlockedIncrement(&pConnection->refCount);

    pLockRequest->ulTid = pSmbRequest->pHeader->ulTid;
    pLockRequest->ullCommandSequence = pSmbRequest->pHeader->ullCommandSequence;
    pLockRequest->ullSessionId = pSmbRequest->pHeader->ullSessionId;
    pLockRequest->ulPid = pSmbRequest->pHeader->ulPid;

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

    if (pLockRequest->pFile)
    {
        SrvFile2Release(pLockRequest->pFile);
    }

    if (pLockRequest->pConnection)
    {
        SrvConnectionRelease(pLockRequest->pConnection);
    }

    SRV_SAFE_FREE_MEMORY(pLockRequest->pLockContexts);
}

static
NTSTATUS
SrvExecuteLockRequest_SMB_V2(
    PSRV_SMB2_LOCK_REQUEST pLockRequest,
    PSMB_PACKET            pSmbResponse
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

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

    if (!bAsync)
    {
        ntStatus = SrvBuildLockResponse_SMB_V2(pLockRequest, pSmbResponse);
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
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SMB2_LOCK_CONTEXT pLockContext = (PSRV_SMB2_LOCK_CONTEXT)pContext;
    PSRV_SMB2_LOCK_REQUEST pLockRequest = pLockContext->pLockRequest;
    BOOLEAN bInLock = FALSE;
    PSMB_PACKET pSmbResponse = NULL;

    if (InterlockedDecrement(&pLockRequest->lPendingContexts) == 0)
    {
        BOOLEAN bSuccess = TRUE;
        LONG iCtx = 0;

        LWIO_LOCK_MUTEX(bInLock, &pLockRequest->mutex);

        if (!pLockRequest->bResponseSent)
        {
            for (; iCtx < pLockRequest->ulNumContexts; iCtx++)
            {
                PSRV_SMB2_LOCK_CONTEXT pIter = &pLockRequest->pLockContexts[iCtx];

                if (pIter->ioStatusBlock.Status != STATUS_SUCCESS)
                {
                    bSuccess = FALSE;
                    break;
                }
            }

            if (bSuccess)
            {
                ntStatus = SrvBuildLockResponse_SMB_V2(
                                pLockRequest,
                                pSmbResponse);
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                SMB2_HEADER header;

                SrvClearLocks_SMB_V2_inlock(pLockRequest);

                memset(&header, 0, sizeof(SMB2_HEADER));

                header.command = COM2_LOCK;
                header.ulTid   = pLockRequest->ulTid;
                header.ulPid   = pLockRequest->ulPid;
                header.ullSessionId = pLockRequest->ullSessionId;
                header.ullCommandSequence = pLockRequest->ullCommandSequence;

                ntStatus = SrvBuildErrorResponse_SMB_V2(
                                pLockRequest->pConnection,
                                &header,
                                STATUS_FILE_LOCK_CONFLICT,
                                NULL,
                                0,
                                pSmbResponse);
            }

            ntStatus = SrvTransportSendResponse(
                                pLockRequest->pConnection,
                                pSmbResponse);
            BAIL_ON_NT_STATUS(ntStatus);

            pLockRequest->bResponseSent = TRUE;
        }
    }

cleanup:

    if (pLockContext->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pLockContext->pAcb->AsyncCancelContext);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequest->mutex);

    if (pSmbResponse)
    {
        SMBPacketFree(
                pLockRequest->pConnection->hPacketAllocator,
                pSmbResponse);
    }

    SrvReleaseLockRequest_SMB_V2(pLockRequest);

    return;

error:

    goto cleanup;
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
    LONG64   llOffset = pLockContext->lockInfo.ullFileOffset;
    LONG64   llLength = pLockContext->lockInfo.ullByteRange;

    ntStatus = IoLockFile(
                    pLockContext->pLockRequest->pFile->hFile,
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
    LONG64   llOffset = pLockContext->lockInfo.ullFileOffset;
    LONG64   llLength = pLockContext->lockInfo.ullByteRange;

    ntStatus = IoUnlockFile(
                    pLockContext->pLockRequest->pFile->hFile,
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
    PSRV_SMB2_LOCK_REQUEST pLockRequest,
    PSMB_PACKET            pSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE pOutBufferRef = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    PBYTE pOutBuffer = pOutBufferRef;
    ULONG ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    ULONG ulOffset    = pSmbResponse->bufferUsed - sizeof(NETBIOS_HEADER);
    ULONG ulBytesUsed = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SMB2MarshalHeader(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM2_LOCK,
                0,
                1,
                pLockRequest->ulPid,
                pLockRequest->ullCommandSequence,
                pLockRequest->ulTid,
                pLockRequest->ullSessionId,
                STATUS_SUCCESS,
                TRUE,
                NULL,
                &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    pOutBuffer += ulBytesUsed;
    ulOffset += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;

    ntStatus = SMB2MarshalLockResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pLockRequest,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    // pOutBuffer += ulBytesUsed;
    // ulOffset += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;

    pSmbResponse->bufferUsed += ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        memset(pOutBufferRef, 0, ulTotalBytesUsed);
    }

    goto cleanup;
}

