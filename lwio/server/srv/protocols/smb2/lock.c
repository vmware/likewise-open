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
SrvCancelLockRequestStateHandle_SMB_V2(
    HANDLE hLockState
    );

static
VOID
SrvCancelLockRequestStateHandle_SMB_V2_inlock(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockState
    );

static
NTSTATUS
SrvBuildLockRequestState_SMB_V2(
    PSRV_EXEC_CONTEXT               pExecContext,
    PSMB2_LOCK_REQUEST_HEADER       pLockRequestHeader,
    PLWIO_SRV_TREE_2                pTree,
    PLWIO_SRV_FILE_2                pFile,
    ULONG64                         ullAsyncId,
    PSRV_LOCK_REQUEST_STATE_SMB_V2* ppLockRequestState
    );

static
NTSTATUS
SrvProcessLockRequest_SMB_V2(
    PSRV_EXEC_CONTEXT              pExecContext,
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
NTSTATUS
SrvValidateLockRequest_SMB_V2(
    PSMB2_LOCK pLock
    );

static
VOID
SrvPrepareLockStateAsync_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState,
    PSRV_EXEC_CONTEXT              pExecContext
    );

static
VOID
SrvExecuteLockContextAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseLockStateAsync_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
VOID
SrvClearLocks_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
VOID
SrvClearLocks_SMB_V2_inlock(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
NTSTATUS
SrvBuildLockResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseLockRequestStateHandle_SMB_V2(
    HANDLE hLockRequestState
    );

static
PSRV_LOCK_REQUEST_STATE_SMB_V2
SrvAcquireLockRequestState_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
VOID
SrvReleaseLockRequestState_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    );

static
VOID
SrvFreeLockRequestState_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
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
    BOOLEAN                    bInLock        = FALSE;
    PLWIO_ASYNC_STATE          pAsyncState    = NULL;
    BOOLEAN                    bUnregisterAsync      = FALSE;
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState = NULL;

    pLockRequestState = (PSRV_LOCK_REQUEST_STATE_SMB_V2)pCtxSmb2->hState;

    if (pLockRequestState)
    {
        InterlockedIncrement(&pLockRequestState->refCount);
    }
    else
    {
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

        ntStatus = SMB2UnmarshalLockRequest(pSmbRequest, &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_DEBUG(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_2,
                pSmbRequest->pHeader->command,
                "Lock request params: file-id(persistent:0x%x,volatile:0x%x),"
                "lock-sequence(%u),lock-count(%u)",
                (long long)pRequestHeader->fid.ullPersistentId,
                (long long)pRequestHeader->fid.ullVolatileId,
                pRequestHeader->ulLockSequence,
                pRequestHeader->usLockCount);

        ntStatus = SrvTree2FindFile_SMB_V2(
                            pCtxSmb2,
                            pTree,
                            &pRequestHeader->fid,
                            &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnection2CreateAsyncState(
                                pConnection,
                                COM2_LOCK,
                                &SrvCancelLockRequestStateHandle_SMB_V2,
                                &SrvReleaseLockRequestStateHandle_SMB_V2,
                                &pAsyncState);
        BAIL_ON_NT_STATUS(ntStatus);

        bUnregisterAsync = TRUE;

        ntStatus = SrvBuildLockRequestState_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pTree,
                            pFile,
                            pAsyncState->ullAsyncId,
                            &pLockRequestState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = SrvAcquireLockRequestState_SMB_V2(pLockRequestState);
        pCtxSmb2->pfnStateRelease = &SrvReleaseLockRequestStateHandle_SMB_V2;

        pAsyncState->hAsyncState = SrvAcquireLockRequestState_SMB_V2(pLockRequestState);
    }

    LWIO_LOCK_MUTEX(bInLock, &pLockRequestState->mutex);

    switch (pLockRequestState->stage)
    {
        case SRV_LOCK_STAGE_SMB_V2_INITIAL:

            pLockRequestState->stage = SRV_LOCK_STAGE_SMB_V2_ATTEMPT_LOCK;

            // intentional fall through

        case SRV_LOCK_STAGE_SMB_V2_ATTEMPT_LOCK:

            ntStatus = SrvProcessLockRequest_SMB_V2(
                                pExecContext,
                                pLockRequestState);
            if ((ntStatus == STATUS_CANCELLED) &&
                !pLockRequestState->bCancelledByClient)
            {
                if (pLockRequestState->pFile &&
                    SrvFile2IsFileClosing(pLockRequestState->pFile))
                {
                    ntStatus = STATUS_RANGE_NOT_LOCKED;
                }
                else
                {
                    SrvClearLocks_SMB_V2_inlock(pLockRequestState);

                    // could be due to tree disconnect or logoff
                    ntStatus = STATUS_SUCCESS;
                }
            }
            BAIL_ON_NT_STATUS(ntStatus);

            pLockRequestState->stage = SRV_LOCK_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_LOCK_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildLockResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pLockRequestState->stage = SRV_LOCK_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_LOCK_STAGE_SMB_V2_DONE:

            if (pLockRequestState->ullAsyncId)
            {
                SrvConnection2RemoveAsyncState(
                        pConnection,
                        pLockRequestState->ullAsyncId);

                pLockRequestState->ullAsyncId = 0;
            }

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

    if (pLockRequestState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pLockRequestState->mutex);

        if (bUnregisterAsync)
        {
            SrvConnection2RemoveAsyncState(
                    pConnection,
                    pLockRequestState->ullAsyncId);
        }

        SrvReleaseLockRequestState_SMB_V2(pLockRequestState);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            if (!pLockRequestState->bInitInterimResponse)
            {

                NTSTATUS ntStatus2 = SrvBuildInterimResponse_SMB_V2(
                                            pExecContext,
                                            pLockRequestState->ullAsyncId);
                if (ntStatus2 != STATUS_SUCCESS)
                {
                    LWIO_LOG_ERROR(
                            "Failed to create interim response [code:0x%8x]",
                            ntStatus2);
                }
                else
                {
                    pLockRequestState->bInitInterimResponse = TRUE;

                    bUnregisterAsync = FALSE;
                }
            }

            break;

        case STATUS_NETWORK_NAME_DELETED:
        case STATUS_USER_SESSION_DELETED:

            ntStatus = STATUS_FILE_CLOSED;

            // intentional fall through

        default:

            if (ntStatus == STATUS_FILE_LOCK_CONFLICT)
            {
                ntStatus = STATUS_LOCK_NOT_GRANTED;
            }

            if (pLockRequestState)
            {
                if (bInLock)
                {
                    SrvClearLocks_SMB_V2_inlock(pLockRequestState);
                }
                else
                {
                    SrvClearLocks_SMB_V2(pLockRequestState);
                }

                if (pLockRequestState->ullAsyncId)
                {
                    SrvConnection2RemoveAsyncState(
                            pConnection,
                            pLockRequestState->ullAsyncId);

                    pLockRequestState->ullAsyncId = 0;

                    bUnregisterAsync = FALSE;
                }

                SrvReleaseLockStateAsync_SMB_V2(pLockRequestState);
            }

            break;
    }

    goto cleanup;
}

NTSTATUS
SrvCancelLockRequest_SMB_V2(
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
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockState = NULL;

    ntStatus = SMB2GetAsyncId(pSmbRequest->pHeader, &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnection2FindAsyncState(pConnection, ullAsyncId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pLockState = (PSRV_LOCK_REQUEST_STATE_SMB_V2)pAsyncState->hAsyncState;

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    pLockState->bCancelledByClient = TRUE;

    SrvCancelLockRequestStateHandle_SMB_V2_inlock(pLockState);

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

static
VOID
SrvCancelLockRequestStateHandle_SMB_V2(
    HANDLE hLockState
    )
{
    BOOLEAN bInLock = FALSE;
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockState =
                (PSRV_LOCK_REQUEST_STATE_SMB_V2)hLockState;

    LWIO_LOCK_MUTEX(bInLock, &pLockState->mutex);

    SrvCancelLockRequestStateHandle_SMB_V2_inlock(pLockState);

    LWIO_UNLOCK_MUTEX(bInLock, &pLockState->mutex);
}

static
VOID
SrvCancelLockRequestStateHandle_SMB_V2_inlock(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockState
    )
{
    if (pLockState->pAcb && pLockState->pAcb->AsyncCancelContext)
    {
        IoCancelAsyncCancelContext(pLockState->pAcb->AsyncCancelContext);
    }
}

static
NTSTATUS
SrvBuildLockRequestState_SMB_V2(
    PSRV_EXEC_CONTEXT               pExecContext,
    PSMB2_LOCK_REQUEST_HEADER       pLockRequestHeader,
    PLWIO_SRV_TREE_2                pTree,
    PLWIO_SRV_FILE_2                pFile,
    ULONG64                         ullAsyncId,
    PSRV_LOCK_REQUEST_STATE_SMB_V2* ppLockRequestState
    )
{
    NTSTATUS                       ntStatus          = STATUS_SUCCESS;
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState = NULL;

    ntStatus = SrvAllocateMemory(
                        sizeof(SRV_LOCK_REQUEST_STATE_SMB_V2),
                        (PVOID*)&pLockRequestState);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pLockRequestState->mutex, NULL);
    pLockRequestState->pMutex = &pLockRequestState->mutex;

    pLockRequestState->refCount = 1;

    pLockRequestState->stage = SRV_LOCK_STAGE_SMB_V2_INITIAL;

    pLockRequestState->ulTid          = pTree->ulTid;
    pLockRequestState->pFile          = SrvFile2Acquire(pFile);
    pLockRequestState->pRequestHeader = pLockRequestHeader;

    pLockRequestState->ullAsyncId     = ullAsyncId;

    *ppLockRequestState = pLockRequestState;

cleanup:

    return ntStatus;

error:

    *ppLockRequestState = NULL;

    if (pLockRequestState)
    {
        SrvReleaseLockRequestState_SMB_V2(pLockRequestState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvProcessLockRequest_SMB_V2(
    PSRV_EXEC_CONTEXT              pExecContext,
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pLockRequestState->bLockPending)
    {
        PSMB2_LOCK pLock =
            &pLockRequestState->pRequestHeader->locks[pLockRequestState->iLock];

        ntStatus = pLockRequestState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        if (LwIsSetFlag(pLock->ulFlags, SMB2_LOCK_FLAGS_UNLOCK))
        {
            SrvFile2RegisterUnlock(pLockRequestState->pFile);
        }
        else
        {
            SrvFile2RegisterLock(pLockRequestState->pFile);
        }

        pLockRequestState->iLock++;
        pLockRequestState->bLockPending = FALSE;
    }

    for (;  pLockRequestState->iLock < pLockRequestState->pRequestHeader->usLockCount;
            pLockRequestState->iLock++)
    {
        PSMB2_LOCK pLock =
            &pLockRequestState->pRequestHeader->locks[pLockRequestState->iLock];

        ntStatus = SrvValidateLockRequest_SMB_V2(pLock);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvPrepareLockStateAsync_SMB_V2(pLockRequestState, pExecContext);

        if (LwIsSetFlag(pLock->ulFlags, SMB2_LOCK_FLAGS_UNLOCK))
        {
            INT iPriorLock = 0;

            // No prior lock range must exist that matches this unlock
            for (; iPriorLock < pLockRequestState->iLock; iPriorLock++)
            {
                PSMB2_LOCK pPriorLock =
                    &pLockRequestState->pRequestHeader->locks[iPriorLock];

                if (!LwIsSetFlag(pPriorLock->ulFlags, SMB2_LOCK_FLAGS_UNLOCK))
                {
                    ntStatus = STATUS_INVALID_PARAMETER;
                    BAIL_ON_NT_STATUS(ntStatus);
                }
            }

            ntStatus = IoUnlockFile(
                            pLockRequestState->pFile->hFile,
                            pLockRequestState->pAcb,
                            &pLockRequestState->ioStatusBlock,
                            pLock->ullFileOffset,
                            pLock->ullByteRange,
                            pLockRequestState->pFile->bIsDurable ?
                                pLockRequestState->pRequestHeader->ulLockSequence :
                                0);
            if (ntStatus == STATUS_PENDING)
            {
                pLockRequestState->bLockPending = TRUE;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseLockStateAsync_SMB_V2(pLockRequestState); // sync

            SrvFile2RegisterUnlock(pLockRequestState->pFile);
        }
        else
        {
            INT iPriorLock = 0;

            // No prior lock range must exist that matches this unlock
            for (; iPriorLock < pLockRequestState->iLock; iPriorLock++)
            {
                PSMB2_LOCK pPriorLock =
                    &pLockRequestState->pRequestHeader->locks[iPriorLock];

                if (LwIsSetFlag(pPriorLock->ulFlags, SMB2_LOCK_FLAGS_UNLOCK))
                {
                    ntStatus = STATUS_INVALID_PARAMETER;
                    BAIL_ON_NT_STATUS(ntStatus);
                }
            }

            ntStatus = IoLockFile(
                            pLockRequestState->pFile->hFile,
                            pLockRequestState->pAcb,
                            &pLockRequestState->ioStatusBlock,
                            pLock->ullFileOffset,
                            pLock->ullByteRange,
                            pLockRequestState->pFile->bIsDurable ?
                                pLockRequestState->pRequestHeader->ulLockSequence :
                                0,
                            LwIsSetFlag(pLock->ulFlags,
                                        SMB2_LOCK_FLAGS_FAIL_IMMEDIATELY),
                            LwIsSetFlag(pLock->ulFlags,
                                        SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK));
            if (ntStatus == STATUS_PENDING)
            {
                pLockRequestState->bLockPending = TRUE;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseLockStateAsync_SMB_V2(pLockRequestState); // sync

            SrvFile2RegisterLock(pLockRequestState->pFile);
        }
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvValidateLockRequest_SMB_V2(
    PSMB2_LOCK pLock
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if ((pLock->ullFileOffset == UINT64_MAX) &&
        (pLock->ullByteRange == UINT64_MAX))
    {
        ntStatus = STATUS_INVALID_LOCK_RANGE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Only one of these must be set
    switch (pLock->ulFlags & (SMB2_LOCK_FLAGS_SHARED_LOCK |
                              SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK |
                              SMB2_LOCK_FLAGS_UNLOCK))
    {
        case SMB2_LOCK_FLAGS_SHARED_LOCK:
        case SMB2_LOCK_FLAGS_EXCLUSIVE_LOCK:
        case SMB2_LOCK_FLAGS_UNLOCK:

            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
    }

    if (LwIsSetFlag(pLock->ulFlags, SMB2_LOCK_FLAGS_UNLOCK) &&
        LwIsSetFlag(pLock->ulFlags, SMB2_LOCK_FLAGS_FAIL_IMMEDIATELY))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}


static
VOID
SrvPrepareLockStateAsync_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState,
    PSRV_EXEC_CONTEXT              pExecContext
    )
{
    pLockRequestState->acb.Callback = &SrvExecuteLockContextAsyncCB_SMB_V2;

    pLockRequestState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pLockRequestState->acb.AsyncCancelContext = NULL;

    pLockRequestState->pAcb = &pLockRequestState->acb;
}

static
VOID
SrvExecuteLockContextAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState =
                            (PSRV_LOCK_REQUEST_STATE_SMB_V2)pCtxSmb2->hState;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pLockRequestState->mutex);

    if (pLockRequestState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pLockRequestState->pAcb->AsyncCancelContext);
    }

    pLockRequestState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequestState->mutex);

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
SrvReleaseLockStateAsync_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    if (pLockRequestState->pAcb)
    {
        pLockRequestState->acb.Callback        = NULL;

        if (pLockRequestState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pLockRequestState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pLockRequestState->pAcb->CallbackContext = NULL;
        }

        if (pLockRequestState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pLockRequestState->pAcb->AsyncCancelContext);
        }

        pLockRequestState->pAcb = NULL;
    }
}

static
VOID
SrvClearLocks_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pLockRequestState->mutex);

    SrvClearLocks_SMB_V2_inlock(pLockRequestState);

    LWIO_UNLOCK_MUTEX(bInLock, &pLockRequestState->mutex);
}

static
VOID
SrvClearLocks_SMB_V2_inlock(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    //
    // Unlock (backwards) locks that have already been acquired
    //
    while (pLockRequestState->iLock-- > 0)
    {
        NTSTATUS   ntStatus1 = STATUS_SUCCESS;
        PSMB2_LOCK pLock     =
            &pLockRequestState->pRequestHeader->locks[pLockRequestState->iLock];

        pLockRequestState->pAcb = NULL;

        if (!LwIsSetFlag(pLock->ulFlags, SMB2_LOCK_FLAGS_UNLOCK))
        {
            ntStatus1 = IoUnlockFile(
                            pLockRequestState->pFile->hFile,
                            pLockRequestState->pAcb,
                            &pLockRequestState->ioStatusBlock,
                            pLock->ullFileOffset,
                            pLock->ullByteRange,
                            pLockRequestState->pFile->bIsDurable ?
                                pLockRequestState->pRequestHeader->ulLockSequence :
                                0);
            if (ntStatus1)
            {
                LWIO_LOG_ERROR("Failed in unlock. error code [%d]", ntStatus1);
            }
            else
            {
                SrvFile2RegisterUnlock(pLockRequestState->pFile);
            }
        }
    }

    return;
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
SrvReleaseLockRequestStateHandle_SMB_V2(
    HANDLE hLockRequestState
    )
{
    return SrvReleaseLockRequestState_SMB_V2(
                (PSRV_LOCK_REQUEST_STATE_SMB_V2)hLockRequestState);
}

static
PSRV_LOCK_REQUEST_STATE_SMB_V2
SrvAcquireLockRequestState_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    InterlockedIncrement(&pLockRequestState->refCount);

    return pLockRequestState;
}

static
VOID
SrvReleaseLockRequestState_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    if (InterlockedDecrement(&pLockRequestState->refCount) == 0)
    {
        SrvFreeLockRequestState_SMB_V2(pLockRequestState);
    }
}

static
VOID
SrvFreeLockRequestState_SMB_V2(
    PSRV_LOCK_REQUEST_STATE_SMB_V2 pLockRequestState
    )
{
    if (pLockRequestState->pAcb && pLockRequestState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pLockRequestState->pAcb->AsyncCancelContext);
    }

    if (pLockRequestState->pFile)
    {
        SrvFile2Release(pLockRequestState->pFile);
    }

    if (pLockRequestState->pMutex)
    {
        pthread_mutex_destroy(&pLockRequestState->mutex);
    }
}


