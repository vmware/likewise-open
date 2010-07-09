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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV1
 *
 *        Process SET_INFORMATION2 request
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildSetInfo2State(
    PSET_INFO2_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE            pFile,
    PSRV_SET_INFO2_STATE_SMB_V1*  ppInfo2State
    );

static
NTSTATUS
SrvExecuteSetFileInfo2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareSetInfo2StateAsync(
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State,
    PSRV_EXEC_CONTEXT     pExecContext
    );

static
VOID
SrvExecuteSetInfo2AsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseSetInfo2StateAsync(
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State
    );

static
NTSTATUS
SrvBuildSetInfo2Response(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseSetInfo2StateHandle(
    HANDLE hInfo2State
    );

static
VOID
SrvReleaseSetInfo2State(
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State
    );

static
VOID
SrvFreeSetInfo2State(
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State
    );

NTSTATUS
SrvProcessSetInformation2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;
    PLWIO_SRV_FILE             pFile        = NULL;
    PSRV_SET_INFO2_STATE_SMB_V1    pInfo2State  = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pInfo2State = (PSRV_SET_INFO2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pInfo2State)
    {
        InterlockedIncrement(&pInfo2State->refCount);
    }
    else
    {
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PSET_INFO2_REQUEST_HEADER pRequestHeader = NULL; // Do not free

        if (*pSmbRequest->pWordCount != 7)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

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

        ntStatus = WireUnmarshalSetInfo2Request(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_DEBUG(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_1,
                pSmbRequest->pHeader->command,
                "Set Info2 Parameters: "
                "command(%u),uid(%u),mid(%u),pid(%u),tid(%u),"
                "file-id(%u),creation-date(%u),creation-time(%u),"
                "last-access-date(%u),last-access-time(%u),"
                "last-write-date(%u),last-write-time(%u)",
                pSmbRequest->pHeader->command,
                pSmbRequest->pHeader->uid,
                pSmbRequest->pHeader->mid,
                SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                pSmbRequest->pHeader->tid,
                pRequestHeader->usFid,
                pRequestHeader->creationDate,
                pRequestHeader->creationTime,
                pRequestHeader->lastAccessDate,
                pRequestHeader->lastAccessTime,
                pRequestHeader->lastWriteDate,
                pRequestHeader->lastWriteTime);

        ntStatus = SrvTreeFindFile_SMB_V1(
                        pCtxSmb1,
                        pTree,
                        pRequestHeader->usFid,
                        &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildSetInfo2State(
                        pRequestHeader,
                        pFile,
                        &pInfo2State);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pInfo2State;
        InterlockedIncrement(&pInfo2State->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseSetInfo2StateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pInfo2State->mutex);

    switch (pInfo2State->stage)
    {
        case SRV_SET_INFO2_STAGE_SMB_V1_INITIAL:

            ntStatus = WireSMBDateTimeToNTTime(
                            &pInfo2State->pRequestHeader->creationDate,
                            &pInfo2State->pRequestHeader->creationTime,
                            &pInfo2State->fileBasicInfo.CreationTime);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = WireSMBDateTimeToNTTime(
                            &pInfo2State->pRequestHeader->lastAccessDate,
                            &pInfo2State->pRequestHeader->lastAccessTime,
                            &pInfo2State->fileBasicInfo.LastAccessTime);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = WireSMBDateTimeToNTTime(
                            &pInfo2State->pRequestHeader->lastWriteDate,
                            &pInfo2State->pRequestHeader->lastWriteTime,
                            &pInfo2State->fileBasicInfo.LastWriteTime);
            BAIL_ON_NT_STATUS(ntStatus);

            pInfo2State->stage = SRV_SET_INFO2_STAGE_SMB_V1_ATTEMPT_SET;

            // intentional fall through

        case SRV_SET_INFO2_STAGE_SMB_V1_ATTEMPT_SET:

            pInfo2State->stage = SRV_SET_INFO2_STAGE_SMB_V1_BUILD_RESPONSE;

            ntStatus = SrvExecuteSetFileInfo2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            // intentional fall through

        case SRV_SET_INFO2_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = pInfo2State->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvBuildSetInfo2Response(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pInfo2State->stage = SRV_SET_INFO2_STAGE_SMB_V1_DONE;

        case SRV_SET_INFO2_STAGE_SMB_V1_DONE:

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

    if (pInfo2State)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pInfo2State->mutex);

        SrvReleaseSetInfo2State(pInfo2State);
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

            if (pInfo2State)
            {
                SrvReleaseSetInfo2StateAsync(pInfo2State);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildSetInfo2State(
    PSET_INFO2_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE             pFile,
    PSRV_SET_INFO2_STATE_SMB_V1*  ppInfo2State
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SET_INFO2_STATE_SMB_V1),
                    (PVOID*)&pInfo2State);
    BAIL_ON_NT_STATUS(ntStatus);

    pInfo2State->refCount = 1;

    pthread_mutex_init(&pInfo2State->mutex, NULL);
    pInfo2State->pMutex = &pInfo2State->mutex;

    pInfo2State->stage = SRV_SET_INFO2_STAGE_SMB_V1_INITIAL;

    pInfo2State->pRequestHeader = pRequestHeader;
    pInfo2State->pFile          = SrvFileAcquire(pFile);

    *ppInfo2State = pInfo2State;

cleanup:

    return ntStatus;

error:

    *ppInfo2State = NULL;

    if (pInfo2State)
    {
        SrvFreeSetInfo2State(pInfo2State);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteSetFileInfo2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_SET_INFO2_STATE_SMB_V1    pInfo2State  = NULL;

    pInfo2State = (PSRV_SET_INFO2_STATE_SMB_V1)pCtxSmb1->hState;

    SrvPrepareSetInfo2StateAsync(pInfo2State, pExecContext);

    ntStatus = IoSetInformationFile(
                    pInfo2State->pFile->hFile,
                    pInfo2State->pAcb,
                    &pInfo2State->ioStatusBlock,
                    &pInfo2State->fileBasicInfo,
                    sizeof(pInfo2State->fileBasicInfo),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseSetInfo2StateAsync(pInfo2State); // completed synchronously

error:

    return ntStatus;
}

static
VOID
SrvPrepareSetInfo2StateAsync(
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State,
    PSRV_EXEC_CONTEXT        pExecContext
    )
{
    pInfo2State->acb.Callback        = &SrvExecuteSetInfo2AsyncCB;

    pInfo2State->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pInfo2State->acb.AsyncCancelContext = NULL;

    pInfo2State->pAcb = &pInfo2State->acb;
}

static
VOID
SrvExecuteSetInfo2AsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_SET_INFO2_STATE_SMB_V1   pInfo2State      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pInfo2State =
        (PSRV_SET_INFO2_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pInfo2State->mutex);

    if (pInfo2State->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pInfo2State->pAcb->AsyncCancelContext);
    }

    pInfo2State->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pInfo2State->mutex);

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
SrvReleaseSetInfo2StateAsync(
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State
    )
{
    if (pInfo2State->pAcb)
    {
        pInfo2State->acb.Callback = NULL;

        if (pInfo2State->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pInfo2State->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pInfo2State->pAcb->CallbackContext = NULL;
        }

        if (pInfo2State->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pInfo2State->pAcb->AsyncCancelContext);
        }

        pInfo2State->pAcb = NULL;
    }
}

static
NTSTATUS
SrvBuildSetInfo2Response(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus     = 0;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                       iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1         pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1         pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PSET_INFO2_RESPONSE_HEADER      pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG ulTotalBytesUsed     = 0;
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State = NULL;

    pInfo2State = (PSRV_SET_INFO2_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_SET_INFORMATION2,
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
                        COM_SET_INFORMATION2,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 0;

    if (ulBytesAvailable < sizeof(SET_INFO2_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSET_INFO2_RESPONSE_HEADER)pOutBuffer;

    // pOutBuffer       += sizeof(SET_INFO2_RESPONSE_HEADER);
    // ulOffset         += sizeof(SET_INFO2_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(SET_INFO2_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SET_INFO2_RESPONSE_HEADER);

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
SrvReleaseSetInfo2StateHandle(
    HANDLE hInfo2State
    )
{
    SrvReleaseSetInfo2State((PSRV_SET_INFO2_STATE_SMB_V1)hInfo2State);
}

static
VOID
SrvReleaseSetInfo2State(
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State
    )
{
    if (InterlockedDecrement(&pInfo2State->refCount) == 0)
    {
        SrvFreeSetInfo2State(pInfo2State);
    }
}

static
VOID
SrvFreeSetInfo2State(
    PSRV_SET_INFO2_STATE_SMB_V1 pInfo2State
    )
{
    if (pInfo2State->pAcb && pInfo2State->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pInfo2State->pAcb->AsyncCancelContext);
    }

    if (pInfo2State->pFile)
    {
        SrvFileRelease(pInfo2State->pFile);
    }

    if (pInfo2State->pMutex)
    {
        pthread_mutex_destroy(&pInfo2State->mutex);
    }

    SrvFreeMemory(pInfo2State);
}
