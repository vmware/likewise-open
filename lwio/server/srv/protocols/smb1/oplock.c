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
 *        oplock.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV1
 *
 *        Opportunistic locks
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildOplockBreakResponse(
    PSRV_EXEC_CONTEXT        pExecContext,
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState,
    UCHAR                    ucOplockLevel
    );

static
VOID
SrvOplockExpiredCB(
    PSRV_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    );

static
NTSTATUS
SrvEnqueueOplockAckTask(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    );

static
VOID
SrvFreeOplockState(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    );

static
VOID
SrvOplockAsyncCB(
    PVOID pContext
    );

static
NTSTATUS
SrvBuildOplockExecContext(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState,
    USHORT                   usOplockAction,
    PSRV_EXEC_CONTEXT*       ppExecContext
    );

NTSTATUS
SrvBuildOplockState(
    PLWIO_SRV_CONNECTION      pConnection,
    PLWIO_SRV_SESSION         pSession,
    PLWIO_SRV_TREE            pTree,
    PLWIO_SRV_FILE            pFile,
    PSRV_OPLOCK_STATE_SMB_V1* ppOplockState
    )
{
    NTSTATUS                 ntStatus     = STATUS_SUCCESS;
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_OPLOCK_STATE_SMB_V1),
                    (PVOID*)&pOplockState);
    BAIL_ON_NT_STATUS(ntStatus);

    pOplockState->refCount = 1;

    pthread_mutex_init(&pOplockState->mutex, NULL);
    pOplockState->pMutex = &pOplockState->mutex;

    pOplockState->pConnection = pConnection;
    InterlockedIncrement(&pConnection->refCount);

    pOplockState->pSession = pSession;
    InterlockedIncrement(&pSession->refcount);

    pOplockState->pTree = pTree;
    InterlockedIncrement(&pTree->refcount);

    pOplockState->usFid = pFile->fid;

    *ppOplockState = pOplockState;

cleanup:

    return ntStatus;

error:

    *ppOplockState = NULL;

    if (pOplockState)
    {
        SrvFreeOplockState(pOplockState);
    }

    goto cleanup;
}

NTSTATUS
SrvProcessOplock(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG               iMsg           = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1 pSmbRequest    = &pCtxSmb1->pRequests[iMsg];
    PLW_OPLOCK_HEADER   pRequestHeader = NULL; // Do not free
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PLWIO_SRV_SESSION        pSession      = NULL;
    PLWIO_SRV_TREE           pTree         = NULL;
    PLWIO_SRV_FILE           pFile         = NULL;
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState  = NULL;
    UCHAR                    ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;

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

    ntStatus = WireUnmarshallOplockRequest(
                    pBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pRequestHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
                    pTree,
                    pRequestHeader->usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pRequestHeader->usAction)
    {
        case LW_OPLOCK_ACTION_SEND_BREAK:

            pOplockState =
                    (PSRV_OPLOCK_STATE_SMB_V1)pFile->hOplockState;

            if (!pOplockState)
            {
                ntStatus = STATUS_INTERNAL_ERROR;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            switch (pOplockState->oplockBuffer_out.OplockBreakResult)
            {
                case IO_OPLOCK_BROKEN_TO_NONE:

                    ucOplockLevel = 0;

                    break;

                case IO_OPLOCK_BROKEN_TO_LEVEL_2:

                    ucOplockLevel = 1;

                    break;

                default:

                    ntStatus = STATUS_INTERNAL_ERROR;
                    BAIL_ON_NT_STATUS(ntStatus);

                    break;
            }

            ntStatus = SrvBuildOplockBreakResponse(
                            pExecContext,
                            pOplockState,
                            ucOplockLevel);
            BAIL_ON_NT_STATUS(ntStatus);

            pOplockState->bBreakRequestSent = TRUE;

            switch (SrvFileGetOplockLevel(pFile)) // current op-lock level
            {
                case SMB_OPLOCK_LEVEL_I:
                case SMB_OPLOCK_LEVEL_BATCH:
                    {
                        LONG64 llExpiry = 0LL;

                        llExpiry = (time(NULL) +
                                    (SrvConfigGetOplockTimeout_SMB_V1()/1000) +
                                    11644473600LL) * 10000000LL;

                        ntStatus = SrvTimerPostRequest(
                                        llExpiry,
                                        pOplockState,
                                        &SrvOplockExpiredCB,
                                        &pOplockState->pTimerRequest);
                        BAIL_ON_NT_STATUS(ntStatus);

                        InterlockedIncrement(&pOplockState->refCount);
                    }

                    break;

                case SMB_OPLOCK_LEVEL_II:

                    ntStatus = SrvEnqueueOplockAckTask(pOplockState);
                    BAIL_ON_NT_STATUS(ntStatus);

                    break;

                case SMB_OPLOCK_LEVEL_NONE:
                default:

                    ntStatus = STATUS_INTERNAL_ERROR;
                    BAIL_ON_NT_STATUS(ntStatus);

                    break;
            }

            break;

        case LW_OPLOCK_ACTION_PROCESS_ACK:

            pOplockState =
                    (PSRV_OPLOCK_STATE_SMB_V1)pFile->hOplockState;

            if (pOplockState)
            {
                ntStatus = SrvAcknowledgeOplockBreak(pOplockState, FALSE);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
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

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvAcknowledgeOplockBreak(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState,
    BOOLEAN bFileIsClosed
    )
{
    NTSTATUS ntStatus      = STATUS_SUCCESS;
    UCHAR    ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;
    PLWIO_SRV_FILE pFile   = NULL;

    ntStatus = SrvTreeFindFile(
                    pOplockState->pTree,
                    pOplockState->usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pOplockState->oplockBuffer_out.OplockBreakResult)
    {
        case IO_OPLOCK_BROKEN_TO_NONE:

            // ucOplockLevel = SMB_OPLOCK_LEVEL_NONE;

            break;

        case IO_OPLOCK_BROKEN_TO_LEVEL_2:

            ucOplockLevel = SMB_OPLOCK_LEVEL_II;

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    if (bFileIsClosed)
    {
        pOplockState->oplockBuffer_ack.Response = IO_OPLOCK_BREAK_CLOSE_PENDING;
    }
    else
    {
        pOplockState->oplockBuffer_ack.Response = IO_OPLOCK_BREAK_ACKNOWLEDGE;
    }

    SrvPrepareOplockStateAsync(pOplockState);

    ntStatus = IoFsControlFile(
                    pFile->hFile,
                    pOplockState->pAcb,
                    &pOplockState->ioStatusBlock,
                    IO_FSCTL_OPLOCK_BREAK_ACK,
                    &pOplockState->oplockBuffer_ack,
                    sizeof(pOplockState->oplockBuffer_ack),
                    &pOplockState->oplockBuffer_out,
                    sizeof(pOplockState->oplockBuffer_out));
    switch (ntStatus)
    {
        case STATUS_PENDING:

            SrvFileSetOplockLevel(pFile, ucOplockLevel);

            ntStatus = STATUS_SUCCESS;

            break;

        default:

            SrvReleaseOplockStateAsync(pOplockState); // completed sync

            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildOplockBreakResponse(
    PSRV_EXEC_CONTEXT        pExecContext,
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState,
    UCHAR                    ucOplockLevel
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PSMB_LOCKING_ANDX_REQUEST_HEADER pRequestHeader = NULL; // Do not free
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
                        COM_LOCKING_ANDX,
                        STATUS_SUCCESS,
                        FALSE,
                        pCtxSmb1->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pCtxSmb1->pSession->uid,
                        pSmbRequest->pHeader->mid,
                        FALSE, /* do not require signature */
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

    *pSmbResponse->pWordCount = 8;

    if (ulBytesAvailable < sizeof(SMB_LOCKING_ANDX_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRequestHeader = (PSMB_LOCKING_ANDX_REQUEST_HEADER)pOutBuffer;

    pRequestHeader->ucLockType    = LWIO_LOCK_TYPE_OPLOCK_RELEASE;
    pRequestHeader->ucOplockLevel = ucOplockLevel;
    pRequestHeader->usFid         = pOplockState->usFid;
    pRequestHeader->usNumLocks    = 0;
    pRequestHeader->usNumUnlocks  = 0;
    pRequestHeader->ulTimeout     = 0;
    pRequestHeader->usByteCount   = 0;

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += sizeof(SMB_LOCKING_ANDX_REQUEST_HEADER);

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
SrvOplockExpiredCB(
    PSRV_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    )
{
    NTSTATUS                 ntStatus     = STATUS_SUCCESS;
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState = (PSRV_OPLOCK_STATE_SMB_V1)pUserData;

    ntStatus = SrvEnqueueOplockAckTask(pOplockState);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pOplockState)
    {
        SrvReleaseOplockState(pOplockState);
    }

    return;

error:

    goto cleanup;
}

static
NTSTATUS
SrvEnqueueOplockAckTask(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    )
{
    NTSTATUS          ntStatus     = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pExecContext = NULL;

    ntStatus = SrvBuildOplockExecContext(
                    pOplockState,
                    LW_OPLOCK_ACTION_PROCESS_ACK,
                    &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProdConsEnqueue(
                    gProtocolGlobals_SMB_V1.pWorkQueue,
                    pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pExecContext = NULL;

cleanup:

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    return ntStatus;

error:

    goto cleanup;
}

VOID
SrvReleaseOplockStateHandle(
    HANDLE hOplockState
    )
{
    SrvReleaseOplockState((PSRV_OPLOCK_STATE_SMB_V1)hOplockState);
}

VOID
SrvReleaseOplockState(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    )
{
    if (InterlockedDecrement(&pOplockState->refCount) == 0)
    {
        SrvFreeOplockState(pOplockState);
    }
}

static
VOID
SrvFreeOplockState(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    )
{
    if (pOplockState->pAcb && pOplockState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pOplockState->pAcb->AsyncCancelContext);
    }

    if (pOplockState->pTree)
    {
        SrvTreeRelease(pOplockState->pTree);
    }

    if (pOplockState->pSession)
    {
        SrvSessionRelease(pOplockState->pSession);
    }

    if (pOplockState->pConnection)
    {
        SrvConnectionRelease(pOplockState->pConnection);
    }

    if (pOplockState->pTimerRequest)
    {
        SrvTimerRelease(pOplockState->pTimerRequest);
    }

    if (pOplockState->pMutex)
    {
        pthread_mutex_destroy(&pOplockState->mutex);
    }

    SrvFreeMemory(pOplockState);
}

VOID
SrvPrepareOplockStateAsync(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    )
{
    pOplockState->acb.Callback        = &SrvOplockAsyncCB;

    pOplockState->acb.CallbackContext = pOplockState;
    InterlockedIncrement(&pOplockState->refCount);

    pOplockState->acb.AsyncCancelContext = NULL;

    pOplockState->pAcb = &pOplockState->acb;
}

static
VOID
SrvOplockAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                 ntStatus     = STATUS_SUCCESS;
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState = (PSRV_OPLOCK_STATE_SMB_V1)pContext;
    PSRV_EXEC_CONTEXT        pExecContext = NULL;
    BOOLEAN                  bInLock      = FALSE;
    PLWIO_SRV_FILE           pFile        = NULL;

    /* Nothing to do if this was cancelled */

    if (pOplockState->ioStatusBlock.Status == STATUS_CANCELLED)
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    LWIO_LOCK_MUTEX(bInLock, &pOplockState->mutex);

    if (pOplockState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pOplockState->pAcb->AsyncCancelContext);
    }

    pOplockState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pOplockState->mutex);

    ntStatus = pOplockState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SrvTreeFindFile(
                    pOplockState->pTree,
                    pOplockState->usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildOplockExecContext(
                    pOplockState,
                    LW_OPLOCK_ACTION_SEND_BREAK,
                    &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProdConsEnqueue(
                    gProtocolGlobals_SMB_V1.pWorkQueue,
                    pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pExecContext = NULL;

cleanup:

    if (pOplockState)
    {
        SrvReleaseOplockState(pOplockState);
    }

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    return;

error:

    LWIO_LOG_ERROR("Error: Failed processing oplock break [status:0x%x]",
                   ntStatus);

    // TODO: indicate error on file handle somehow

    goto cleanup;
}

static
NTSTATUS
SrvBuildOplockExecContext(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState,
    USHORT                   usOplockAction,
    PSRV_EXEC_CONTEXT*       ppExecContext
    )
{
    NTSTATUS                 ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT        pExecContext     = NULL;
    PSMB_PACKET              pSmbRequest      = NULL;
    PBYTE                    pBuffer          = NULL;
    ULONG                    ulBytesAvailable = 0;
    ULONG                    ulOffset         = 0;
    USHORT                   usBytesUsed      = 0;
    USHORT                   usTotalBytesUsed = 0;
    PSMB_HEADER              pHeader          = NULL; // Do not free
    PBYTE                    pWordCount       = NULL; // Do not free
    PANDX_HEADER             pAndXHeader      = NULL; // Do not free
    PLW_OPLOCK_HEADER        pLwOplockHeader  = NULL; // Do not free

    ntStatus = SMBPacketAllocate(
                    pOplockState->pConnection->hPacketAllocator,
                    &pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pOplockState->pConnection->hPacketAllocator,
                    (64 * 1024) + 4096,
                    &pSmbRequest->pRawBuffer,
                    &pSmbRequest->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvInitPacket_SMB_V1(pSmbRequest, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildExecContext(
                    pOplockState->pConnection,
                    pSmbRequest,
                    TRUE,
                    &pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

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
                    COM_LW_OPLOCK,
                    STATUS_SUCCESS,
                    FALSE,  /* not a response */
                    pOplockState->pTree->tid,
                    0,      /* pid */
                    pOplockState->pSession->uid,
                    0xFFFF, /* mid = -1 */
                    pOplockState->pConnection->serverProperties.bRequireSecuritySignatures,
                    &pHeader,
                    &pWordCount,
                    &pAndXHeader,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += usBytesUsed;
    ulBytesAvailable -= usBytesUsed;
    usTotalBytesUsed += usBytesUsed;

    if (ulBytesAvailable < sizeof(LW_OPLOCK_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pLwOplockHeader = (PLW_OPLOCK_HEADER)pBuffer;

    pLwOplockHeader->usFid    = pOplockState->usFid;
    pLwOplockHeader->usAction = usOplockAction;

    *pWordCount = 1;

    // pBuffer          += sizeof(LW_OPLOCK_HEADER);
    // ulBytesAvailable -= sizeof(LW_OPLOCK_HEADER);
    usTotalBytesUsed += sizeof(LW_OPLOCK_HEADER);

    pSmbRequest->bufferUsed += usTotalBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppExecContext = pExecContext;

cleanup:

    if (pSmbRequest)
    {
        SMBPacketRelease(
                pOplockState->pConnection->hPacketAllocator,
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
SrvReleaseOplockStateAsync(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    )
{
    if (pOplockState->pAcb)
    {
        pOplockState->acb.Callback        = NULL;

        if (pOplockState->pAcb->CallbackContext)
        {
            InterlockedDecrement(&pOplockState->refCount);

            pOplockState->pAcb->CallbackContext = NULL;
        }

        if (pOplockState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pOplockState->pAcb->AsyncCancelContext);
        }

        pOplockState->pAcb = NULL;
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
