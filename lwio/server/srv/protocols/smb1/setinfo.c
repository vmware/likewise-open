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
 *        Process SET_INFORMATION request
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildSetInfoState(
    PSET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_SESSION        pSession,
    PLWIO_SRV_TREE           pTree,
    PWSTR                    pwszFilename,
    PSRV_SET_INFO_STATE_SMB_V1*  ppInfoState
    );

static
NTSTATUS
SrvExecuteSetFileInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareSetInfoStateAsync(
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState,
    PSRV_EXEC_CONTEXT     pExecContext
    );

static
VOID
SrvExecuteSetInfoAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseSetInfoStateAsync(
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState
    );

static
VOID
SrvLogSetInfo_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    );

static
NTSTATUS
SrvBuildSetInformationResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseSetInfoStateHandle(
    HANDLE hInfoState
    );

static
VOID
SrvReleaseSetInfoState(
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState
    );

static
VOID
SrvFreeSetInfoState(
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState
    );

NTSTATUS
SrvProcessSetInformation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState   = NULL;
    BOOLEAN                    bInLock      = FALSE;
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;

    pInfoState = (PSRV_SET_INFO_STATE_SMB_V1)pCtxSmb1->hState;

    if (pInfoState)
    {
        InterlockedIncrement(&pInfoState->refCount);
    }
    else
    {
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PSET_INFO_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PWSTR pwszFilename = NULL;   // Do not free

        if (*pSmbRequest->pWordCount != 8)
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

        ntStatus = WireUnmarshalSetInfoRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pwszFilename);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_CALL_DEBUG(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_1,
                pSmbRequest->pHeader->command,
                &SrvLogSetInfo_SMB_V1,
                pRequestHeader,
                pwszFilename);

        ntStatus = SrvBuildSetInfoState(
                        pRequestHeader,
                        pSession,
                        pTree,
                        pwszFilename,
                        &pInfoState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pInfoState;
        InterlockedIncrement(&pInfoState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseSetInfoStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pInfoState->mutex);

    switch (pInfoState->stage)
    {
        case SRV_SET_INFO_STAGE_SMB_V1_INITIAL:

            ntStatus = WireSMBDateTimeToNTTime(
                            &pInfoState->pRequestHeader->lastWriteDate,
                            &pInfoState->pRequestHeader->lastWriteTime,
                            &pInfoState->fileBasicInfo.LastWriteTime);
            BAIL_ON_NT_STATUS(ntStatus);

            pInfoState->fileBasicInfo.FileAttributes =
                         pInfoState->pRequestHeader->usFileAttributes;

            switch(pInfoState->fileBasicInfo.FileAttributes)
            {
                case FILE_ATTRIBUTE_NORMAL:

                    pInfoState->fileBasicInfo.FileAttributes = 0;
                    break;

                case 0:

                    pInfoState->fileBasicInfo.FileAttributes =
                                                FILE_ATTRIBUTE_NORMAL;
                    break;

                default:

                    break;
            }

            ntStatus = SrvBuildTreeRelativePath(
                            pInfoState->pTree,
                            pInfoState->pwszFilename,
                            &pInfoState->fileName);
            BAIL_ON_NT_STATUS(ntStatus);

            pInfoState->stage = SRV_SET_INFO_STAGE_SMB_V1_ATTEMPT_SET;

            SrvPrepareSetInfoStateAsync(pInfoState, pExecContext);

            ntStatus = SrvIoCreateFile(
                            pInfoState->pTree->pShareInfo,
                            &pInfoState->hFile,
                            pInfoState->pAcb,
                            &pInfoState->ioStatusBlock,
                            pInfoState->pSession->pIoSecurityContext,
                            &pInfoState->fileName,
                            NULL,
                            NULL,
                            FILE_WRITE_ATTRIBUTES,
                            0,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            FILE_OPEN,
                            0,
                            NULL, /* EA Buffer */
                            0,    /* EA Length */
                            pInfoState->pEcpList
                            );
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseSetInfoStateAsync(pInfoState); // completed synchronously

            // intentional fall through

        case SRV_SET_INFO_STAGE_SMB_V1_ATTEMPT_SET:

            pInfoState->stage = SRV_SET_INFO_STAGE_SMB_V1_BUILD_RESPONSE;

            ntStatus = SrvExecuteSetFileInfo(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            // intentional fall through

        case SRV_SET_INFO_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = pInfoState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvBuildSetInformationResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pInfoState->stage = SRV_SET_INFO_STAGE_SMB_V1_DONE;

        case SRV_SET_INFO_STAGE_SMB_V1_DONE:

            break;
    }

cleanup:

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pInfoState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pInfoState->mutex);

        SrvReleaseSetInfoState(pInfoState);
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

            if (pInfoState)
            {
                SrvReleaseSetInfoStateAsync(pInfoState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildSetInfoState(
    PSET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_SESSION        pSession,
    PLWIO_SRV_TREE           pTree,
    PWSTR                    pwszFilename,
    PSRV_SET_INFO_STATE_SMB_V1*  ppInfoState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SET_INFO_STATE_SMB_V1),
                    (PVOID*)&pInfoState);
    BAIL_ON_NT_STATUS(ntStatus);

    pInfoState->refCount = 1;

    pthread_mutex_init(&pInfoState->mutex, NULL);
    pInfoState->pMutex = &pInfoState->mutex;

    pInfoState->pSession = SrvSessionAcquire(pSession);
    pInfoState->pTree    = SrvTreeAcquire(pTree);
    pInfoState->pwszFilename = pwszFilename;

    pInfoState->stage = SRV_SET_INFO_STAGE_SMB_V1_INITIAL;

    pInfoState->pRequestHeader = pRequestHeader;

    *ppInfoState = pInfoState;

cleanup:

    return ntStatus;

error:

    *ppInfoState = NULL;

    if (pInfoState)
    {
        SrvFreeSetInfoState(pInfoState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteSetFileInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_SET_INFO_STATE_SMB_V1    pInfoState  = NULL;

    pInfoState = (PSRV_SET_INFO_STATE_SMB_V1)pCtxSmb1->hState;

    SrvPrepareSetInfoStateAsync(pInfoState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pInfoState->hFile,
                    pInfoState->pAcb,
                    &pInfoState->ioStatusBlock,
                    &pInfoState->fileBasicInfo,
                    sizeof(pInfoState->fileBasicInfo),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseSetInfoStateAsync(pInfoState); // completed synchronously

error:

    return ntStatus;
}

static
VOID
SrvPrepareSetInfoStateAsync(
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState,
    PSRV_EXEC_CONTEXT        pExecContext
    )
{
    pInfoState->acb.Callback        = &SrvExecuteSetInfoAsyncCB;

    pInfoState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pInfoState->acb.AsyncCancelContext = NULL;

    pInfoState->pAcb = &pInfoState->acb;
}

static
VOID
SrvExecuteSetInfoAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_SET_INFO_STATE_SMB_V1   pInfoState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pInfoState =
        (PSRV_SET_INFO_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pInfoState->mutex);

    if (pInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pInfoState->pAcb->AsyncCancelContext);
    }

    pInfoState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pInfoState->mutex);

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
SrvReleaseSetInfoStateAsync(
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState
    )
{
    if (pInfoState->pAcb)
    {
        pInfoState->acb.Callback = NULL;

        if (pInfoState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pInfoState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pInfoState->pAcb->CallbackContext = NULL;
        }

        if (pInfoState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pInfoState->pAcb->AsyncCancelContext);
        }

        pInfoState->pAcb = NULL;
    }
}

static
VOID
SrvLogSetInfo_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSET_INFO_REQUEST_HEADER pRequestHeader = NULL;
    PWSTR pwszFilename = NULL;
    PSTR  pszFilename  = NULL;
    va_list msgList;

    va_start(msgList, ulLine);
    pRequestHeader = va_arg(msgList, PSET_INFO_REQUEST_HEADER);
    pwszFilename   = va_arg(msgList, PWSTR);

    if (pwszFilename)
    {
        ntStatus = SrvWc16sToMbs(pwszFilename, &pszFilename);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (logLevel >= LWIO_LOG_LEVEL_DEBUG)
    {
        LWIO_LOG_ALWAYS_CUSTOM(
                logLevel,
                "[%s() %s:%u] SetInfo Parameters: last-write-date(%u),last-write-time(%u),file-attrs(0x%x),Filename(%s)",
                LWIO_SAFE_LOG_STRING(pszFunction),
                LWIO_SAFE_LOG_STRING(pszFile),
                ulLine,
                pRequestHeader->lastWriteDate,
                pRequestHeader->lastWriteTime,
                pRequestHeader->usFileAttributes,
                LWIO_SAFE_LOG_STRING(pszFilename));
    }
    else
    {
        LWIO_LOG_ALWAYS_CUSTOM(
                logLevel,
                "SetInfo Parameters: last-write-date(%u),last-write-time(%u),file-attrs(0x%x),Filename(%s)",
                pRequestHeader->lastWriteDate,
                pRequestHeader->lastWriteTime,
                pRequestHeader->usFileAttributes,
                LWIO_SAFE_LOG_STRING(pszFilename));
    }

error:

    va_end(msgList);

    SRV_SAFE_FREE_MEMORY(pszFilename);

    return;
}

static
NTSTATUS
SrvBuildSetInformationResponse(
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
    PSET_INFO_RESPONSE_HEADER      pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG ulTotalBytesUsed     = 0;
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState = NULL;

    pInfoState = (PSRV_SET_INFO_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_SET_INFORMATION,
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
                        COM_SET_INFORMATION,
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

    if (ulBytesAvailable < sizeof(SET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSET_INFO_RESPONSE_HEADER)pOutBuffer;

    // pOutBuffer       += sizeof(SET_INFO_RESPONSE_HEADER);
    // ulOffset         += sizeof(SET_INFO_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(SET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SET_INFO_RESPONSE_HEADER);

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
SrvReleaseSetInfoStateHandle(
    HANDLE hInfoState
    )
{
    SrvReleaseSetInfoState((PSRV_SET_INFO_STATE_SMB_V1)hInfoState);
}

static
VOID
SrvReleaseSetInfoState(
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState
    )
{
    if (InterlockedDecrement(&pInfoState->refCount) == 0)
    {
        SrvFreeSetInfoState(pInfoState);
    }
}

static
VOID
SrvFreeSetInfoState(
    PSRV_SET_INFO_STATE_SMB_V1 pInfoState
    )
{
    if (pInfoState->pAcb && pInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pInfoState->pAcb->AsyncCancelContext);
    }

    if (pInfoState->pEcpList)
    {
        IoRtlEcpListFree(&pInfoState->pEcpList);
    }

    if (pInfoState->pTree)
    {
        SrvTreeRelease(pInfoState->pTree);
    }

    if (pInfoState->pSession)
    {
        SrvSessionRelease(pInfoState->pSession);
    }

    if (pInfoState->fileName.FileName)
    {
        SrvFreeMemory(pInfoState->fileName.FileName);
    }

    if (pInfoState->hFile)
    {
        IoCloseFile(pInfoState->hFile);
    }

    if (pInfoState->pMutex)
    {
        pthread_mutex_destroy(&pInfoState->mutex);
    }

    SrvFreeMemory(pInfoState);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
