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
 *        close.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Close
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildCloseState_SMB_V2(
    PSMB2_FID                pFid,
    PLWIO_SRV_TREE_2         pTree,
    PLWIO_SRV_FILE_2         pFile,
    PSRV_CLOSE_STATE_SMB_V2* ppCloseState
    );

static
NTSTATUS
SrvGetFileInformation_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildCloseResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareCloseStateAsync_SMB_V2(
    PSRV_CLOSE_STATE_SMB_V2 pCloseState,
    PSRV_EXEC_CONTEXT       pExecContext
    );

static
VOID
SrvExecuteCloseAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseCloseStateAsync_SMB_V2(
    PSRV_CLOSE_STATE_SMB_V2 pCloseState
    );

static
VOID
SrvReleaseCloseStateHandle_SMB_V2(
    HANDLE hCloseState
    );

static
VOID
SrvReleaseCloseState_SMB_V2(
    PSRV_CLOSE_STATE_SMB_V2 pCloseState
    );

static
VOID
SrvFreeCloseState_SMB_V2(
    PSRV_CLOSE_STATE_SMB_V2 pCloseState
    );

NTSTATUS
SrvProcessClose_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection      = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2         = pProtocolContext->pSmb2Context;
    PSRV_CLOSE_STATE_SMB_V2    pCloseState      = NULL;
    PLWIO_SRV_SESSION_2        pSession         = NULL;
    PLWIO_SRV_TREE_2           pTree            = NULL;
    PLWIO_SRV_FILE_2           pFile            = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pCloseState = (PSRV_CLOSE_STATE_SMB_V2)pCtxSmb2->hState;
    if (pCloseState)
    {
        InterlockedIncrement(&pCloseState->refCount);
    }
    else
    {
        ULONG               iMsg           = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2 pSmbRequest    = &pCtxSmb2->pRequests[iMsg];
        PSMB2_FID           pFid     = NULL; // Do not free

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

        ntStatus = SMB2UnmarshalCloseRequest(pSmbRequest, &pFid);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvTree2FindFile_SMB_V2(
                        pCtxSmb2,
                        pTree,
                        pFid,
                        &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildCloseState_SMB_V2(
                        pFid,
                        pTree,
                        pFile,
                        &pCloseState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pCloseState;
        InterlockedIncrement(&pCloseState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseCloseStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pCloseState->mutex);

    switch (pCloseState->stage)
    {
        case SRV_CLOSE_STAGE_SMB_V2_INITIAL:

            ntStatus = SrvGetFileInformation_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCloseState->stage = SRV_CLOSE_STAGE_SMB_V2_ATTEMPT_IO;

            // intentional fall through

        case SRV_CLOSE_STAGE_SMB_V2_ATTEMPT_IO:

            SrvFile2ResetOplockState(pCloseState->pFile);

            ntStatus = SrvTree2RemoveFile(
                            pCloseState->pTree,
                            pCloseState->pFile->ullFid);
            BAIL_ON_NT_STATUS(ntStatus);

            pCloseState->stage = SRV_CLOSE_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_CLOSE_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildCloseResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCloseState->stage = SRV_CLOSE_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_CLOSE_STAGE_SMB_V2_DONE:

            if (pCtxSmb2->pFile)
            {
                SrvFile2Release(pCtxSmb2->pFile);
                pCtxSmb2->pFile = NULL;
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

    if (pCloseState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pCloseState->mutex);

        SrvReleaseCloseState_SMB_V2(pCloseState);
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

            if (pCloseState)
            {
                SrvReleaseCloseStateAsync_SMB_V2(pCloseState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildCloseState_SMB_V2(
    PSMB2_FID                pFid,
    PLWIO_SRV_TREE_2         pTree,
    PLWIO_SRV_FILE_2         pFile,
    PSRV_CLOSE_STATE_SMB_V2* ppCloseState
    )
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PSRV_CLOSE_STATE_SMB_V2 pCloseState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_CLOSE_STATE_SMB_V2),
                    (PVOID*)&pCloseState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCloseState->refCount = 1;

    pthread_mutex_init(&pCloseState->mutex, NULL);
    pCloseState->pMutex = &pCloseState->mutex;

    pCloseState->pFid = pFid;

    pCloseState->pTree = pTree;
    InterlockedIncrement(&pTree->refcount);

    pCloseState->pFile = SrvFile2Acquire(pFile);

    *ppCloseState = pCloseState;

cleanup:

    return ntStatus;

error:

    *ppCloseState = NULL;

    if (pCloseState)
    {
        SrvFreeCloseState_SMB_V2(pCloseState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetFileInformation_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2         = pProtocolContext->pSmb2Context;
    PSRV_CLOSE_STATE_SMB_V2    pCloseState      = NULL;

    pCloseState = (PSRV_CLOSE_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pCloseState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pCloseState->pFileBasicInfo)
    {
        pCloseState->pFileBasicInfo = &pCloseState->fileBasicInfo;

        SrvPrepareCloseStateAsync_SMB_V2(pCloseState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCloseState->pFile->hFile,
                        pCloseState->pAcb,
                        &pCloseState->ioStatusBlock,
                        pCloseState->pFileBasicInfo,
                        sizeof(pCloseState->fileBasicInfo),
                        FileBasicInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseCloseStateAsync_SMB_V2(pCloseState);
    }

    if (!pCloseState->pFileStdInfo)
    {
        pCloseState->pFileStdInfo = &pCloseState->fileStdInfo;

        SrvPrepareCloseStateAsync_SMB_V2(pCloseState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCloseState->pFile->hFile,
                        pCloseState->pAcb,
                        &pCloseState->ioStatusBlock,
                        pCloseState->pFileStdInfo,
                        sizeof(pCloseState->fileStdInfo),
                        FileStandardInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseCloseStateAsync_SMB_V2(pCloseState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildCloseResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2         = pProtocolContext->pSmb2Context;
    PSRV_CLOSE_STATE_SMB_V2    pCloseState      = NULL;
    ULONG                      iMsg             = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest      = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse     = &pCtxSmb2->pResponses[iMsg];
    PSMB2_CLOSE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE  pOutBuffer       = pSmbResponse->pBuffer;
    ULONG  ulOffset         = 0;
    ULONG  ulTotalBytesUsed = 0;
    ULONG  ulBytesUsed      = 0;
    ULONG  ulBytesAvailable = pSmbResponse->ulBytesAvailable;

    pCloseState = (PSRV_CLOSE_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_CLOSE,
                    pSmbRequest->pHeader->usEpoch,
                    pSmbRequest->pHeader->usCredits,
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

    if (ulBytesAvailable < sizeof(SMB2_CLOSE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_CLOSE_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulBytesUsed       = sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_CLOSE_RESPONSE_HEADER);

    pResponseHeader->ullCreationTime   =
                    pCloseState->fileBasicInfo.CreationTime;
    pResponseHeader->ullLastAccessTime =
                    pCloseState->fileBasicInfo.LastAccessTime;
    pResponseHeader->ullLastWriteTime  =
                    pCloseState->fileBasicInfo.LastWriteTime;
    pResponseHeader->ullLastChangeTime =
                    pCloseState->fileBasicInfo.ChangeTime;
    pResponseHeader->ulFileAttributes  =
                    pCloseState->fileBasicInfo.FileAttributes;
    pResponseHeader->ullAllocationSize =
                    pCloseState->fileStdInfo.AllocationSize;
    pResponseHeader->ullEndOfFile      =
                    pCloseState->fileStdInfo.EndOfFile;
    pResponseHeader->usLength          = ulBytesUsed;

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
SrvPrepareCloseStateAsync_SMB_V2(
    PSRV_CLOSE_STATE_SMB_V2 pCloseState,
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    pCloseState->acb.Callback        = &SrvExecuteCloseAsyncCB_SMB_V2;

    pCloseState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pCloseState->acb.AsyncCancelContext = NULL;

    pCloseState->pAcb = &pCloseState->acb;
}

static
VOID
SrvExecuteCloseAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_CLOSE_STATE_SMB_V2    pCloseState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pCloseState = (PSRV_CLOSE_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pCloseState->mutex);

    if (pCloseState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pCloseState->pAcb->AsyncCancelContext);
    }

    pCloseState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pCloseState->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V2.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

static
VOID
SrvReleaseCloseStateAsync_SMB_V2(
    PSRV_CLOSE_STATE_SMB_V2 pCloseState
    )
{
    if (pCloseState->pAcb)
    {
        pCloseState->acb.Callback        = NULL;

        if (pCloseState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pCloseState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pCloseState->pAcb->CallbackContext = NULL;
        }

        if (pCloseState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pCloseState->pAcb->AsyncCancelContext);
        }

        pCloseState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseCloseStateHandle_SMB_V2(
    HANDLE hCloseState
    )
{
    return SrvReleaseCloseState_SMB_V2((PSRV_CLOSE_STATE_SMB_V2)hCloseState);
}

static
VOID
SrvReleaseCloseState_SMB_V2(
    PSRV_CLOSE_STATE_SMB_V2 pCloseState
    )
{
    if (InterlockedDecrement(&pCloseState->refCount) == 0)
    {
        SrvFreeCloseState_SMB_V2(pCloseState);
    }
}

static
VOID
SrvFreeCloseState_SMB_V2(
    PSRV_CLOSE_STATE_SMB_V2 pCloseState
    )
{
    if (pCloseState->pAcb && pCloseState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pCloseState->pAcb->AsyncCancelContext);
    }

    if (pCloseState->pFile)
    {
        SrvFile2Release(pCloseState->pFile);
    }

    if (pCloseState->pTree)
    {
        SrvTree2Release(pCloseState->pTree);
    }

    if (pCloseState->pMutex)
    {
        pthread_mutex_destroy(&pCloseState->mutex);
    }

    SrvFreeMemory(pCloseState);
}

