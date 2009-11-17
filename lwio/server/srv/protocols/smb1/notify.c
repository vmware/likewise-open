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
 *        notify.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV1
 *
 *        Change Notify
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvNotifyRepositoryBuild(
    USHORT                             usTid,
    PSRV_TREE_NOTIFY_STATE_REPOSITORY* ppRepository
    );

static
int
SrvNotifyStateCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
int
SrvNotifyRepositoryCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvNotifyStateReleaseHandle(
    PVOID pNotifyState
    );

static
VOID
SrvNotifyRepositoryReleaseHandle(
    PVOID pNotifyRepository
    );

static
NTSTATUS
SrvNotifyStateBuild(
    PLWIO_SRV_CONNECTION             pConnection,
    PLWIO_SRV_SESSION                pSession,
    PLWIO_SRV_TREE                   pTree,
    PLWIO_SRV_FILE                   pFile,
    USHORT                           usMid,
    ULONG                            ulPid,
    ULONG                            ulRequestSequence,
    ULONG                            ulCompletionFilter,
    BOOLEAN                          bWatchTree,
    ULONG                            ulMaxBufferSize,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1* ppNotifyState
    );

static
ULONG64
SrvNotifyChangeGetId(
    ULONG  ulPid,
    USHORT usMid
    );

static
VOID
SrvNotifyRepositoryFree(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pNotifyRepository
    );

static
VOID
SrvNotifyAsyncCB(
    PVOID pContext
    );

static
NTSTATUS
SrvBuildNotifyExecContext(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState,
    PSRV_EXEC_CONTEXT*              ppExecContext
    );

static
VOID
SrvNotifyStateFree(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    );

NTSTATUS
SrvNotifyRepositoryInit(
    VOID
    )
{
    return LwRtlRBTreeCreate(
                    &SrvNotifyRepositoryCompare,
                    NULL,
                    &SrvNotifyRepositoryReleaseHandle,
                    &gProtocolGlobals_SMB_V1.pTreeNotifyStateCollection);
}

NTSTATUS
SrvNotifyCreateRepository(
    USHORT                             usTid,
    PSRV_TREE_NOTIFY_STATE_REPOSITORY* ppRepository
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pRepository = NULL;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    ntStatus = LwRtlRBTreeFind(
                        gProtocolGlobals_SMB_V1.pTreeNotifyStateCollection,
                        &usTid,
                        (PVOID*)&pRepository);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = SrvNotifyRepositoryBuild(
                        usTid,
                        &pRepository);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = LwRtlRBTreeAdd(
                        gProtocolGlobals_SMB_V1.pTreeNotifyStateCollection,
                        &pRepository->usTid,
                        pRepository);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pRepository->refCount);

    *ppRepository = pRepository;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    return ntStatus;

error:

    *ppRepository = pRepository;

    if (pRepository)
    {
        SrvNotifyRepositoryRelease(pRepository);
    }

    goto cleanup;
}

NTSTATUS
SrvNotifyRepositoryCreateState(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pRepository,
    PLWIO_SRV_CONNECTION              pConnection,
    PLWIO_SRV_SESSION                 pSession,
    PLWIO_SRV_TREE                    pTree,
    PLWIO_SRV_FILE                    pFile,
    USHORT                            usMid,
    ULONG                             ulPid,
    ULONG                             ulRequestSequence,
    ULONG                             ulCompletionFilter,
    BOOLEAN                           bWatchTree,
    ULONG                             ulMaxBufferSize,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1*  ppNotifyState
    )
{
    NTSTATUS ntStatus    = STATUS_SUCCESS;
    BOOLEAN  bInLock     = FALSE;
    ULONG64  ullNotifyId = 0;
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState = NULL;

    ullNotifyId = SrvNotifyChangeGetId(ulPid, usMid);

    LWIO_LOCK_MUTEX(bInLock, &pRepository->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pRepository->pNotifyStateCollection,
                    (PVOID)&ullNotifyId,
                    (PVOID*)&pNotifyState);
    if (ntStatus == STATUS_SUCCESS)
    {
        ntStatus = STATUS_OBJECT_NAME_COLLISION;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvNotifyStateBuild(
                    pConnection,
                    pSession,
                    pTree,
                    pFile,
                    usMid,
                    ulPid,
                    ulRequestSequence,
                    ulCompletionFilter,
                    bWatchTree,
                    ulMaxBufferSize,
                    &pNotifyState);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pRepository->pNotifyStateCollection,
                    &pNotifyState->ullNotifyId,
                    pNotifyState);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pNotifyState->refCount);

    *ppNotifyState = pNotifyState;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pRepository->mutex);

    return ntStatus;

error:

    *ppNotifyState = NULL;

    if (pNotifyState)
    {
        SrvNotifyStateRelease(pNotifyState);
    }

    goto cleanup;
}

NTSTATUS
SrvNotifyRepositoryFindState(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pRepository,
    ULONG                             ulPid,
    USHORT                            usMid,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1*  ppNotifyState
    )
{
    NTSTATUS ntStatus    = STATUS_SUCCESS;
    BOOLEAN  bInLock     = FALSE;
    ULONG64  ullNotifyId = 0;
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState = NULL;

    ullNotifyId = SrvNotifyChangeGetId(ulPid, usMid);

    LWIO_LOCK_MUTEX(bInLock, &pRepository->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pRepository->pNotifyStateCollection,
                    &ullNotifyId,
                    (PVOID*)&pNotifyState);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pNotifyState->refCount);

    *ppNotifyState = pNotifyState;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pRepository->mutex);

    return ntStatus;

error:

    *ppNotifyState = NULL;

    goto cleanup;
}

NTSTATUS
SrvNotifyRepositoryRemoveState(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pRepository,
    ULONG                             ulPid,
    USHORT                            usMid
    )
{
    NTSTATUS ntStatus    = STATUS_SUCCESS;
    BOOLEAN  bInLock     = FALSE;
    ULONG64  ullNotifyId = 0;

    ullNotifyId = SrvNotifyChangeGetId(ulPid, usMid);

    LWIO_LOCK_MUTEX(bInLock, &pRepository->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pRepository->pNotifyStateCollection,
                    &ullNotifyId);

    LWIO_UNLOCK_MUTEX(bInLock, &pRepository->mutex);

    return ntStatus;
}

NTSTATUS
SrvNotifyRemoveRepository(
    USHORT usTid
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    ntStatus = LwRtlRBTreeRemove(
                    gProtocolGlobals_SMB_V1.pTreeNotifyStateCollection,
                    &usTid);

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    return ntStatus;
}

VOID
SrvNotifyRepositoryShutdown(
    VOID
    )
{
    if (gProtocolGlobals_SMB_V1.pTreeNotifyStateCollection)
    {
        LwRtlRBTreeFree(gProtocolGlobals_SMB_V1.pTreeNotifyStateCollection);
    }
}

static
NTSTATUS
SrvNotifyRepositoryBuild(
    USHORT                             usTid,
    PSRV_TREE_NOTIFY_STATE_REPOSITORY* ppRepository
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pRepository = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_TREE_NOTIFY_STATE_REPOSITORY),
                    (PVOID*)&pRepository);
    BAIL_ON_NT_STATUS(ntStatus);

    pRepository->refCount = 1;

    pthread_mutex_init(&pRepository->mutex, NULL);
    pRepository->pMutex = &pRepository->mutex;

    pRepository->usTid = usTid;

    ntStatus = LwRtlRBTreeCreate(
                    &SrvNotifyStateCompare,
                    NULL,
                    &SrvNotifyStateReleaseHandle,
                    &pRepository->pNotifyStateCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppRepository = pRepository;

cleanup:

    return ntStatus;

error:

    *ppRepository = NULL;

    if (pRepository)
    {
        SrvNotifyRepositoryFree(pRepository);
    }

    goto cleanup;
}

static
int
SrvNotifyStateCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG64 pNotifyId1 = (PULONG64)pKey1;
    PULONG64 pNotifyId2 = (PULONG64)pKey2;

    assert (pNotifyId1 != NULL);
    assert (pNotifyId2 != NULL);

    if (*pNotifyId1 > *pNotifyId2)
    {
        return 1;
    }
    else if (*pNotifyId1 < *pNotifyId2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
int
SrvNotifyRepositoryCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PUSHORT pTid1 = (PUSHORT)pKey1;
    PUSHORT pTid2 = (PUSHORT)pKey2;

    assert (pTid1 != NULL);
    assert (pTid2 != NULL);

    if (*pTid1 > *pTid2)
    {
        return 1;
    }
    else if (*pTid1 < *pTid2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
SrvNotifyStateReleaseHandle(
    PVOID pNotifyState
    )
{
    SrvNotifyStateRelease((PSRV_CHANGE_NOTIFY_STATE_SMB_V1)pNotifyState);
}

static
VOID
SrvNotifyRepositoryReleaseHandle(
    PVOID pNotifyRepository
    )
{
    SrvNotifyRepositoryRelease(
                    (PSRV_TREE_NOTIFY_STATE_REPOSITORY)pNotifyRepository);
}

VOID
SrvNotifyRepositoryRelease(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pNotifyRepository
    )
{
    if (InterlockedDecrement(&pNotifyRepository->refCount) == 0)
    {
        SrvNotifyRepositoryFree(pNotifyRepository);
    }
}

static
VOID
SrvNotifyRepositoryFree(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pNotifyRepository
    )
{
    if (pNotifyRepository->pNotifyStateCollection)
    {
        LwRtlRBTreeFree(pNotifyRepository->pNotifyStateCollection);
    }

    if (pNotifyRepository->pMutex)
    {
        pthread_mutex_destroy(&pNotifyRepository->mutex);
    }

    SrvFreeMemory(pNotifyRepository);
}

static
NTSTATUS
SrvNotifyStateBuild(
    PLWIO_SRV_CONNECTION             pConnection,
    PLWIO_SRV_SESSION                pSession,
    PLWIO_SRV_TREE                   pTree,
    PLWIO_SRV_FILE                   pFile,
    USHORT                           usMid,
    ULONG                            ulPid,
    ULONG                            ulRequestSequence,
    ULONG                            ulCompletionFilter,
    BOOLEAN                          bWatchTree,
    ULONG                            ulMaxBufferSize,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1* ppNotifyState
    )
{
    NTSTATUS                        ntStatus     = STATUS_SUCCESS;
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_CHANGE_NOTIFY_STATE_SMB_V1),
                    (PVOID*)&pNotifyState);
    BAIL_ON_NT_STATUS(ntStatus);

    pNotifyState->refCount = 1;

    pthread_mutex_init(&pNotifyState->mutex, NULL);
    pNotifyState->pMutex = &pNotifyState->mutex;

    pNotifyState->ullNotifyId = SrvNotifyChangeGetId(ulPid, usMid);

    pNotifyState->pConnection = pConnection;
    InterlockedIncrement(&pNotifyState->refCount);

    pNotifyState->ulCompletionFilter = ulCompletionFilter;
    pNotifyState->bWatchTree         = bWatchTree;

    pNotifyState->usUid = pSession->uid;
    pNotifyState->usTid = pTree->tid;
    pNotifyState->usFid = pFile->fid;
    pNotifyState->usMid = usMid;
    pNotifyState->ulPid = ulPid;

    pNotifyState->ulRequestSequence = ulRequestSequence;

    pNotifyState->ulMaxBufferSize   = ulMaxBufferSize;

    if (ulMaxBufferSize)
    {
        ntStatus = SrvAllocateMemory(
                        ulMaxBufferSize,
                        (PVOID*)&pNotifyState->pBuffer);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNotifyState->ulBufferLength = ulMaxBufferSize;

    *ppNotifyState = pNotifyState;

cleanup:

    return ntStatus;

error:

    *ppNotifyState = NULL;

    if (pNotifyState)
    {
        SrvNotifyStateFree(pNotifyState);
    }

    goto cleanup;
}

static
ULONG64
SrvNotifyChangeGetId(
    ULONG  ulPid,
    USHORT usMid
    )
{
    return (((ULONG64)ulPid) << 32) | ((ULONG64)usMid);
}

VOID
SrvPrepareNotifyStateAsync(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    )
{
    pNotifyState->acb.Callback        = &SrvNotifyAsyncCB;

    pNotifyState->acb.CallbackContext = pNotifyState;
    InterlockedIncrement(&pNotifyState->refCount);

    pNotifyState->acb.AsyncCancelContext = NULL;

    pNotifyState->pAcb = &pNotifyState->acb;
}

static
VOID
SrvNotifyAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS          ntStatus     = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pExecContext = NULL;
    BOOLEAN           bInLock      = FALSE;
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState =
                            (PSRV_CHANGE_NOTIFY_STATE_SMB_V1)pContext;

    LWIO_LOCK_MUTEX(bInLock, &pNotifyState->mutex);

    if (pNotifyState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pNotifyState->pAcb->AsyncCancelContext);
    }

    pNotifyState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pNotifyState->mutex);

    ntStatus = SrvBuildNotifyExecContext(
                    pNotifyState,
                    &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProdConsEnqueue(
                    gProtocolGlobals_SMB_V1.pWorkQueue,
                    pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pExecContext = NULL;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pNotifyState->mutex);

    if (pNotifyState)
    {
        SrvNotifyStateRelease(pNotifyState);
    }

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    return;

error:

    LWIO_LOG_ERROR("Error: Failed processing change notify call back "
                   "[status:0x%x]",
                   ntStatus);

    // TODO: indicate error on file handle somehow

    goto cleanup;
}

static
NTSTATUS
SrvBuildNotifyExecContext(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState,
    PSRV_EXEC_CONTEXT*              ppExecContext
    )
{
    NTSTATUS                 ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT        pExecContext     = NULL;
    PSMB_PACKET              pSmbRequest      = NULL;
    PBYTE                    pParams          = NULL;
    ULONG                    ulParamLength    = 0;
    PBYTE                    pData            = NULL;
    ULONG                    ulDataLen        = 0;
    ULONG                    ulDataOffset     = 0;
    PBYTE                    pBuffer          = NULL;
    ULONG                    ulBytesAvailable = 0;
    ULONG                    ulOffset         = 0;
    USHORT                   usBytesUsed      = 0;
    USHORT                   usTotalBytesUsed = 0;
    PSMB_HEADER              pHeader          = NULL; // Do not free
    PBYTE                    pWordCount       = NULL; // Do not free
    PANDX_HEADER             pAndXHeader      = NULL; // Do not free
    PUSHORT                  pSetup           = NULL;
    UCHAR                    ucSetupCount     = 0;
    ULONG                    ulParameterOffset     = 0;
    ULONG                    ulNumPackageBytesUsed = 0;
    SMB_NOTIFY_CHANGE_HEADER notifyRequestHeader   = {0};

    ntStatus = SMBPacketAllocate(
                    pNotifyState->pConnection->hPacketAllocator,
                    &pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pNotifyState->pConnection->hPacketAllocator,
                    (64 * 1024) + 4096,
                    &pSmbRequest->pRawBuffer,
                    &pSmbRequest->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvInitPacket_SMB_V1(pSmbRequest, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildExecContext(
                    pNotifyState->pConnection,
                    pSmbRequest,
                    TRUE,
                    &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbRequest->sequence = pNotifyState->ulRequestSequence;

    pBuffer = pSmbRequest->pRawBuffer;
    ulBytesAvailable = pSmbRequest->bufferLen;

    if (ulBytesAvailable < sizeof(NETBIOS_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBuffer += sizeof(NETBIOS_HEADER);
    ulBytesAvailable -= sizeof(NETBIOS_HEADER);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_NT_TRANSACT,
                    pNotifyState->ioStatusBlock.Status,
                    FALSE,  /* not a response */
                    pNotifyState->usTid,
                    pNotifyState->ulPid,
                    pNotifyState->usUid,
                    pNotifyState->usMid,
                    pNotifyState->pConnection->serverProperties.bRequireSecuritySignatures,
                    &pHeader,
                    &pWordCount,
                    &pAndXHeader,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += usBytesUsed;
    ulOffset         += usBytesUsed;
    ulBytesAvailable -= usBytesUsed;
    usTotalBytesUsed += usBytesUsed;

    notifyRequestHeader.usFid = pNotifyState->usFid;

    pSetup       = (PUSHORT)&notifyRequestHeader;
    ucSetupCount = sizeof(notifyRequestHeader)/sizeof(USHORT);

    *pWordCount = 18 + ucSetupCount;

    ntStatus = WireMarshallNtTransactionRequest(
                    pBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    SMB_SUB_COMMAND_NT_TRANSACT_NOTIFY_CHANGE,
                    pSetup,
                    ucSetupCount,
                    pParams,
                    ulParamLength,
                    pData,
                    ulDataLen,
                    &ulDataOffset,
                    &ulParameterOffset,
                    &ulNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pBuffer          += ulNumPackageBytesUsed;
    // ulOffset         += ulNumPackageBytesUsed;
    // ulBytesAvailable -= ulNumPackageBytesUsed;
    usTotalBytesUsed += ulNumPackageBytesUsed;

    pSmbRequest->bufferUsed += usTotalBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppExecContext = pExecContext;

cleanup:

    if (pSmbRequest)
    {
        SMBPacketRelease(
                pNotifyState->pConnection->hPacketAllocator,
                pSmbRequest);
    }

    return ntStatus;

error:

    *ppExecContext = NULL;

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    goto cleanup;
}

VOID
SrvReleaseNotifyStateAsync(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    )
{
    if (pNotifyState->pAcb)
    {
        pNotifyState->acb.Callback        = NULL;

        if (pNotifyState->pAcb->CallbackContext)
        {
            InterlockedDecrement(&pNotifyState->refCount);

            pNotifyState->pAcb->CallbackContext = NULL;
        }

        if (pNotifyState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pNotifyState->pAcb->AsyncCancelContext);
        }

        pNotifyState->pAcb = NULL;
    }
}

VOID
SrvNotifyStateRelease(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    )
{
    if (InterlockedDecrement(&pNotifyState->refCount) == 0)
    {
        SrvNotifyStateFree(pNotifyState);
    }
}

static
VOID
SrvNotifyStateFree(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    )
{
    if (pNotifyState->pAcb && pNotifyState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pNotifyState->pAcb->AsyncCancelContext);
    }

    if (pNotifyState->pConnection)
    {
        SrvConnectionRelease(pNotifyState->pConnection);
    }

    if (pNotifyState->pBuffer)
    {
        SrvFreeMemory(pNotifyState->pBuffer);
    }

    if (pNotifyState->pMutex)
    {
        pthread_mutex_destroy(&pNotifyState->mutex);
    }

    SrvFreeMemory(pNotifyState);
}
