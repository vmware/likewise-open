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
 *        queryinfo.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Query Information
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildGetInfoState_SMB_V2(
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE_2              pFile,
    PSRV_GET_INFO_STATE_SMB_V2*   ppGetInfoState
    );

static
NTSTATUS
SrvQueryInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildGetInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseGetInfoStateHandle_SMB_V2(
    HANDLE hGetInfoState
    );

static
VOID
SrvExecuteGetInfoAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseGetInfoState_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState
    );

static
VOID
SrvFreeGetInfoState_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState
    );

NTSTATUS
SrvProcessGetInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    PLWIO_SRV_SESSION_2        pSession      = NULL;
    PLWIO_SRV_TREE_2           pTree         = NULL;
    PLWIO_SRV_FILE_2           pFile         = NULL;
    BOOLEAN                    bInLock       = FALSE;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    if (pGetInfoState)
    {
        InterlockedIncrement(&pGetInfoState->refCount);
    }
    else
    {
        ULONG                      iMsg          = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
        PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader = NULL; // Do not free

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

        ntStatus = SMB2UnmarshalGetInfoRequest(pSmbRequest, &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_DEBUG(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_2,
                pSmbRequest->pHeader->command,
                "Get Info request params: "
                "command(%u),uid(%llu),cmd-seq(%llu),pid(%u),tid(%u),"
                "credits(%u),flags(0x%x),chain-offset(%u),"
                "file-id(persistent:0x%x,volatile:0x%x),"
                "info-class(0x%x),info-type(0x%x),flags(0x%x),"
                "input-buffer-length(%u),input-buffer-offset(%u),"
                "output-buffer-length(%u),additional-info(%u)",
                pSmbRequest->pHeader->command,
                (long long)pSmbRequest->pHeader->ullSessionId,
                (long long)pSmbRequest->pHeader->ullCommandSequence,
                pSmbRequest->pHeader->ulPid,
                pSmbRequest->pHeader->ulTid,
                pSmbRequest->pHeader->usCredits,
                pSmbRequest->pHeader->ulFlags,
                pSmbRequest->pHeader->ulChainOffset,
                (long long)pRequestHeader->fid.ullPersistentId,
                (long long)pRequestHeader->fid.ullVolatileId,
                pRequestHeader->ucInfoClass,
                pRequestHeader->ucInfoType,
                pRequestHeader->ulFlags,
                pRequestHeader->ulInputBufferLen,
                pRequestHeader->usInputBufferOffset,
                pRequestHeader->ulOutputBufferLen,
                pRequestHeader->ulAdditionalInfo);

        ntStatus = SrvTree2FindFile_SMB_V2(
                            pCtxSmb2,
                            pTree,
                            &pRequestHeader->fid,
                            &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildGetInfoState_SMB_V2(
                            pRequestHeader,
                            pFile,
                            &pGetInfoState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pGetInfoState;
        InterlockedIncrement(&pGetInfoState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseGetInfoStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pGetInfoState->mutex);

    switch (pGetInfoState->stage)
    {
        case SRV_GET_INFO_STAGE_SMB_V2_INITIAL:

            pGetInfoState->stage = SRV_GET_INFO_STAGE_SMB_V2_ATTEMPT_IO;

            // Intentional fall through

        case SRV_GET_INFO_STAGE_SMB_V2_ATTEMPT_IO:

            ntStatus = SrvQueryInfo_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pGetInfoState->stage = SRV_GET_INFO_STAGE_SMB_V2_BUILD_RESPONSE;

            // Intentional fall through

        case SRV_GET_INFO_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildGetInfoResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pGetInfoState->stage = SRV_GET_INFO_STAGE_SMB_V2_DONE;

            // Intentional fall through

        case SRV_GET_INFO_STAGE_SMB_V2_DONE:

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

    if (pGetInfoState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pGetInfoState->mutex);

        SrvReleaseGetInfoState_SMB_V2(pGetInfoState);
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

            if (pGetInfoState)
            {
                SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildGetInfoState_SMB_V2(
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE_2              pFile,
    PSRV_GET_INFO_STATE_SMB_V2*   ppGetInfoState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_GET_INFO_STATE_SMB_V2),
                    (PVOID*)&pGetInfoState);
    BAIL_ON_NT_STATUS(ntStatus);

    pGetInfoState->refCount = 1;

    pthread_mutex_init(&pGetInfoState->mutex, NULL);
    pGetInfoState->pMutex = &pGetInfoState->mutex;

    pGetInfoState->stage = SRV_GET_INFO_STAGE_SMB_V2_INITIAL;

    pGetInfoState->pRequestHeader = pRequestHeader;

    pGetInfoState->pFile = SrvFile2Acquire(pFile);

    *ppGetInfoState = pGetInfoState;

cleanup:

    return ntStatus;

error:

    *ppGetInfoState = NULL;

    if (pGetInfoState)
    {
        SrvFreeGetInfoState_SMB_V2(pGetInfoState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvQueryInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = SrvGetFileInfo_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = SrvGetFileSystemInfo_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = SrvGetSecurityInfo_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvBuildGetInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = SrvBuildFileInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = SrvBuildFileSystemInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = SrvBuildSecurityInfoResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

VOID
SrvPrepareGetInfoStateAsync_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState,
    PSRV_EXEC_CONTEXT          pExecContext
    )
{
    pGetInfoState->acb.Callback        = &SrvExecuteGetInfoAsyncCB_SMB_V2;

    pGetInfoState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pGetInfoState->acb.AsyncCancelContext = NULL;

    pGetInfoState->pAcb = &pGetInfoState->acb;
}

static
VOID
SrvExecuteGetInfoAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState    = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pGetInfoState =
        (PSRV_GET_INFO_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pGetInfoState->mutex);

    if (pGetInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pGetInfoState->pAcb->AsyncCancelContext);
    }

    pGetInfoState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pGetInfoState->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V2.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

VOID
SrvReleaseGetInfoStateAsync_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState
    )
{
    if (pGetInfoState->pAcb)
    {
        pGetInfoState->acb.Callback = NULL;

        if (pGetInfoState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pGetInfoState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pGetInfoState->pAcb->CallbackContext = NULL;
        }

        if (pGetInfoState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pGetInfoState->pAcb->AsyncCancelContext);
        }

        pGetInfoState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseGetInfoStateHandle_SMB_V2(
    HANDLE hGetInfoState
    )
{
    return SrvReleaseGetInfoState_SMB_V2(
                    (PSRV_GET_INFO_STATE_SMB_V2)hGetInfoState);
}

static
VOID
SrvReleaseGetInfoState_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState
    )
{
    if (InterlockedDecrement(&pGetInfoState->refCount) == 0)
    {
        SrvFreeGetInfoState_SMB_V2(pGetInfoState);
    }
}

static
VOID
SrvFreeGetInfoState_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState
    )
{
    if (pGetInfoState->pAcb && pGetInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pGetInfoState->pAcb->AsyncCancelContext);
    }

    if (pGetInfoState->pData2)
    {
        SrvFreeMemory(pGetInfoState->pData2);
    }

    if (pGetInfoState->pFile)
    {
        SrvFile2Release(pGetInfoState->pFile);
    }

    if (pGetInfoState->pMutex)
    {
        pthread_mutex_destroy(&pGetInfoState->mutex);
    }

    SrvFreeMemory(pGetInfoState);
}

