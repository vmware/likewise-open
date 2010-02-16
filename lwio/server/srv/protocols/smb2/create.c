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
 *        create.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Create
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildCreateState_SMB_V2(
    PSRV_EXEC_CONTEXT           pExecContext,
    PSMB2_CREATE_REQUEST_HEADER pRequestHeader,
    PUNICODE_STRING             pwszFilename,
    PSRV_CREATE_CONTEXT*        ppCreateContexts,
    ULONG                       ulNumContexts,
    PSRV_CREATE_STATE_SMB_V2*   ppCreateState
    );

static
NTSTATUS
SrvQueryFileInformation_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvProcessCreateContexts_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvRequestCreateOplocks_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareCreateStateAsync_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState,
    PSRV_EXEC_CONTEXT        pExecContext
    );

static
VOID
SrvReleaseCreateStateAsync_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState
    );

static
VOID
SrvExecuteCreateAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseCreateStateHandle_SMB_V2(
    HANDLE hState
    );

static
PSRV_CREATE_STATE_SMB_V2
SrvAcquireCreateState_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState
    );

static
VOID
SrvReleaseCreateState_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState
    );

static
VOID
SrvFreeCreateState_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState
    );

static
NTSTATUS
SrvBuildCreateResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvWriteCreateContext(
    PBYTE                 pOutBuffer,
    ULONG                 ulOffset,
    ULONG                 ulBytesAvailable,
    PBYTE                 pName,
    USHORT                usNameSize,
    PBYTE                 pData,
    ULONG                 ulDataSize,
    PULONG                pulAlignBytesUsed,
    PULONG                pulCCBytesUsed,
    PSMB2_CREATE_CONTEXT* ppCreateContext
    );

NTSTATUS
SrvProcessCreate_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PLWIO_SRV_SESSION_2        pSession = NULL;
    PLWIO_SRV_TREE_2           pTree = NULL;
    PSRV_CREATE_STATE_SMB_V2   pCreateState = NULL;
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    BOOLEAN                    bUnregisterAsync = FALSE;
    PSRV_CREATE_CONTEXT        pCreateContexts = NULL;
    ULONG                      ulNumContexts = 0;
    BOOLEAN                    bInLock = FALSE;

    pCreateState = (PSRV_CREATE_STATE_SMB_V2)pCtxSmb2->hState;
    if (pCreateState)
    {
        InterlockedIncrement(&pCreateState->refCount);
    }
    else
    {
        ULONG               iMsg        = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2 pSmbRequest = &pCtxSmb2->pRequests[iMsg];
        PSMB2_CREATE_REQUEST_HEADER pCreateRequestHeader = NULL;// Do not free
        UNICODE_STRING              wszFileName = {0};          // Do not free
        wchar16_t                   wszEmpty[] = {0};

        if (pCtxSmb2->pFile)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

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

        ntStatus = SMB2UnmarshalCreateRequest(
                        pSmbRequest,
                        &pCreateRequestHeader,
                        &wszFileName,
                        &pCreateContexts,
                        &ulNumContexts);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!wszFileName.Length)
        {
            wszFileName.Buffer = &wszEmpty[0];
            wszFileName.Length = 0;
            wszFileName.MaximumLength = sizeof(wszEmpty);
        }

        ntStatus = SrvBuildCreateState_SMB_V2(
                        pExecContext,
                        pCreateRequestHeader,
                        &wszFileName,
                        &pCreateContexts,
                        ulNumContexts,
                        &pCreateState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pCreateState;
        InterlockedIncrement(&pCreateState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseCreateStateHandle_SMB_V2;

        ntStatus = SrvAsyncBuildUniqueId(
                        pExecContext,
                        &pCreateState->ullAsyncId);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvAsyncStateCreate(
                     pCreateState->ullAsyncId,
                     COM2_CREATE,
                     pCreateState,
                     &SrvReleaseCreateStateHandle_SMB_V2,
                     &pAsyncState);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvAcquireCreateState_SMB_V2(pCreateState);

        ntStatus = SrvSession2AddAsyncState(
                     pCtxSmb2->pSession,
                     pAsyncState);
        BAIL_ON_NT_STATUS(ntStatus);

        bUnregisterAsync = TRUE;
    }

    LWIO_LOCK_MUTEX(bInLock, &pCreateState->mutex);

    switch (pCreateState->stage)
    {
        case SRV_CREATE_STAGE_SMB_V2_INITIAL:

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V2_CREATE_FILE_COMPLETED;

            SrvPrepareCreateStateAsync_SMB_V2(pCreateState, pExecContext);

            ntStatus = SrvIoCreateFile(
                            pCreateState->pTree->pShareInfo,
                            &pCreateState->hFile,
                            pCreateState->pAcb,
                            &pCreateState->ioStatusBlock,
                            pSession->pIoSecurityContext,
                            pCreateState->pFilename,
                            pCreateState->pSecurityDescriptor,
                            pCreateState->pSecurityQOS,
                            pCreateState->pRequestHeader->ulDesiredAccess,
                            0LL,
                            pCreateState->pRequestHeader->ulFileAttributes,
                            pCreateState->pRequestHeader->ulShareAccess,
                            pCreateState->pRequestHeader->ulCreateDisposition,
                            pCreateState->pRequestHeader->ulCreateOptions,
                            (pCreateState->pExtAContext ?
                                pCreateState->pExtAContext->pData : NULL),
                            (pCreateState->pExtAContext ?
                                pCreateState->pExtAContext->ulDataLength : 0),
                            &pCreateState->pEcpList);
            switch (ntStatus)
            {
                case STATUS_PENDING:
                {
                    // TODO: Might have to cancel the entire operation
                    //
                    NTSTATUS ntStatus2 = SrvBuildInterimResponse_SMB_V2(
                                                pExecContext,
                                                pCreateState->ullAsyncId);
                    if (ntStatus2 != STATUS_SUCCESS)
                    {
                        LWIO_LOG_ERROR(
                                "Failed to create interim response [code:0x%8x]",
                                ntStatus2);
                    }

                    bUnregisterAsync = FALSE;
                }
                    break;

                case STATUS_SUCCESS:

                    // completed synchronously; remove asynchronous state
                    //
                    ntStatus = SrvSession2RemoveAsyncState(
                                    pCtxSmb2->pSession,
                                    pCreateState->ullAsyncId);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pCreateState->ullAsyncId = 0LL;

                    bUnregisterAsync = FALSE;

                default:

                    break;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseCreateStateAsync_SMB_V2(pCreateState); // completed sync

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V2_CREATE_FILE_COMPLETED:

            ntStatus = pCreateState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pCreateState->ulCreateAction =
                            pCreateState->ioStatusBlock.CreateResult;

            ntStatus = SrvTree2CreateFile(
                            pCreateState->pTree,
                            pCreateState->pwszFilename,
                            &pCreateState->hFile,
                            &pCreateState->pFilename,
                            pCreateState->pRequestHeader->ulDesiredAccess,
                            0LL,
                            pCreateState->pRequestHeader->ulFileAttributes,
                            pCreateState->pRequestHeader->ulShareAccess,
                            pCreateState->pRequestHeader->ulCreateDisposition,
                            pCreateState->pRequestHeader->ulCreateOptions,
                            &pCreateState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pCreateState->bRemoveFileFromTree = TRUE;

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V2_ATTEMPT_QUERY_INFO;

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V2_ATTEMPT_QUERY_INFO:

            ntStatus = SrvQueryFileInformation_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V2_QUERY_CREATE_CONTEXTS;

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V2_QUERY_CREATE_CONTEXTS:

            ntStatus = SrvProcessCreateContexts_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V2_REQUEST_OPLOCK;

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V2_REQUEST_OPLOCK:

            ntStatus = SrvRequestCreateOplocks_SMB_V2(pExecContext);
            // Don't fail if the op-lock cannot be granted

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V2_QUERY_INFO_COMPLETED;

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V2_QUERY_INFO_COMPLETED:

            ntStatus = pCreateState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvBuildCreateResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V2_DONE;

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V2_DONE:

            if (pCreateState->ullAsyncId)
            {
                ntStatus = SrvSession2RemoveAsyncState(
                                pCtxSmb2->pSession,
                                pCreateState->ullAsyncId);
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pCreateState->bRemoveFileFromTree = FALSE;

            pCtxSmb2->pFile = SrvFile2Acquire(pCreateState->pFile);
            pCtxSmb2->llNumSuccessfulCreates++;

            break;
    }

cleanup:

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    if (pCreateState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pCreateState->mutex);

        if (bUnregisterAsync)
        {
            SrvSession2RemoveAsyncState(
                    pCtxSmb2->pSession,
                    pCreateState->ullAsyncId);
        }

        SrvReleaseCreateState_SMB_V2(pCreateState);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    SRV_SAFE_FREE_MEMORY(pCreateContexts);

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

            if (pCreateState)
            {
                SrvReleaseCreateStateAsync_SMB_V2(pCreateState);
            }

            break;
    }

    goto cleanup;
}

NTSTATUS
SrvCancelCreate_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PLWIO_SRV_SESSION_2        pSession     = NULL;
    BOOLEAN                    bInLock      = FALSE;
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    ULONG64                    ullAsyncId   = 0LL;
    PSRV_CREATE_STATE_SMB_V2   pCreateState = NULL;

    ntStatus = SrvConnection2FindSession_SMB_V2(
                            pCtxSmb2,
                            pConnection,
                            pSmbRequest->pHeader->ullSessionId,
                            &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2GetAsyncId(pSmbRequest->pHeader, &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindAsyncState(pSession, ullAsyncId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreateState = (PSRV_CREATE_STATE_SMB_V2)pAsyncState->hAsyncState;

    LWIO_LOCK_MUTEX(bInLock, &pCreateState->mutex);

    if (pCreateState->pAcb && pCreateState->pAcb->AsyncCancelContext)
    {
        IoCancelAsyncCancelContext(pCreateState->pAcb->AsyncCancelContext);
    }

cleanup:

    if (pCreateState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pCreateState->mutex);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildCreateState_SMB_V2(
    PSRV_EXEC_CONTEXT           pExecContext,
    PSMB2_CREATE_REQUEST_HEADER pRequestHeader,
    PUNICODE_STRING             pwszFilename,
    PSRV_CREATE_CONTEXT*        ppCreateContexts,
    ULONG                       ulNumContexts,
    PSRV_CREATE_STATE_SMB_V2*   ppCreateState
    )
{
    NTSTATUS                   ntStatus       = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection    = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2       = pCtxProtocol->pSmb2Context;
    PSRV_CREATE_STATE_SMB_V2   pCreateState   = NULL;
    BOOLEAN                    bTreeInLock    = FALSE;
    ULONG                      iCtx       = 0;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_CREATE_STATE_SMB_V2),
                    (PVOID*)&pCreateState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreateState->refCount = 1;

    pthread_mutex_init(&pCreateState->mutex, NULL);
    pCreateState->pMutex = &pCreateState->mutex;

    pCreateState->stage = SRV_CREATE_STAGE_SMB_V2_INITIAL;

    ntStatus = SrvAllocateMemory(
                    pwszFilename->Length + sizeof(wchar16_t),
                    (PVOID*)&pCreateState->pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pwszFilename->Length)
    {
        memcpy((PBYTE)pCreateState->pwszFilename,
               (PBYTE)pwszFilename->Buffer,
               pwszFilename->Length);
    }

    // TODO: Handle root fids
    ntStatus = SrvAllocateMemory(
                    sizeof(IO_FILE_NAME),
                    (PVOID*)&pCreateState->pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreateState->pTree = SrvTree2Acquire(pCtxSmb2->pTree);

    LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pCtxSmb2->pTree->mutex);

    ntStatus = SrvBuildFilePath(
                    pCtxSmb2->pTree->pShareInfo->pwszPath,
                    pCreateState->pwszFilename,
                    &pCreateState->pFilename->FileName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb2->pTree->mutex);

    /* For named pipes, we need to pipe some extra data into the npfs driver:
     *  - Session key
     *  - Client principal name
     *  - Client address
     */
    if (SrvTree2IsNamedPipe(pCtxSmb2->pTree))
    {
        ntStatus = IoRtlEcpListAllocate(&pCreateState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeSessionKey(
                       pConnection,
                       pCreateState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeClientAddress(
                       pConnection,
                       pCreateState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pCreateState->pCreateContexts = *ppCreateContexts;
    *ppCreateContexts = NULL;

    pCreateState->ulNumContexts   = ulNumContexts;

    for (iCtx = 0; iCtx < pCreateState->ulNumContexts; iCtx++)
    {
        PSRV_CREATE_CONTEXT pContext = &pCreateState->pCreateContexts[iCtx];

        switch (pContext->contextItemType)
        {
            case SMB2_CONTEXT_ITEM_TYPE_EXT_ATTRS:

                pCreateState->pExtAContext = pContext;

                break;

            case SMB2_CONTEXT_ITEM_TYPE_SEC_DESC:

                pCreateState->pSecDescContext = pContext;

                break;

            default:

                break;
        }
    }

    if (pCreateState->pSecDescContext)
    {
        SECURITY_INFORMATION secInfoAll = DACL_SECURITY_INFORMATION;

        if (!pCreateState->pSecDescContext->ulDataLength ||
            !RtlValidRelativeSecurityDescriptor(
                (PSECURITY_DESCRIPTOR_RELATIVE)pCreateState->pSecDescContext->pData,
                pCreateState->pSecDescContext->ulDataLength,
                secInfoAll))
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pCreateState->pSecurityDescriptor =
            (PSECURITY_DESCRIPTOR_RELATIVE)pCreateState->pSecDescContext->pData;
    }

    pCreateState->pRequestHeader = pRequestHeader;

    *ppCreateState = pCreateState;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb2->pTree->mutex);

    return ntStatus;

error:

    *ppCreateState = NULL;

    if (pCreateState)
    {
        SrvFreeCreateState_SMB_V2(pCreateState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvQueryFileInformation_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2    pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PSRV_CREATE_STATE_SMB_V2    pCreateState = NULL;

    pCreateState = (PSRV_CREATE_STATE_SMB_V2)pCtxSmb2->hState;

    if (!pCreateState->pFileBasicInfo)
    {
        pCreateState->pFileBasicInfo = &pCreateState->fileBasicInfo;

        SrvPrepareCreateStateAsync_SMB_V2(pCreateState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCreateState->pFile->hFile,
                        pCreateState->pAcb,
                        &pCreateState->ioStatusBlock,
                        pCreateState->pFileBasicInfo,
                        sizeof(pCreateState->fileBasicInfo),
                        FileBasicInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseCreateStateAsync_SMB_V2(pCreateState); // completed sync
    }

    if (!pCreateState->pFileStdInfo)
    {
        pCreateState->pFileStdInfo = &pCreateState->fileStdInfo;

        SrvPrepareCreateStateAsync_SMB_V2(pCreateState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCreateState->pFile->hFile,
                        pCreateState->pAcb,
                        &pCreateState->ioStatusBlock,
                        pCreateState->pFileStdInfo,
                        sizeof(pCreateState->fileStdInfo),
                        FileStandardInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseCreateStateAsync_SMB_V2(pCreateState); // completed sync
    }

    if (SrvTree2IsNamedPipe(pCreateState->pTree))
    {
        if (!pCreateState->pFilePipeInfo)
        {
            pCreateState->pFilePipeInfo = &pCreateState->filePipeInfo;

            SrvPrepareCreateStateAsync_SMB_V2(pCreateState, pExecContext);

            ntStatus = IoQueryInformationFile(
                            pCreateState->pFile->hFile,
                            pCreateState->pAcb,
                            &pCreateState->ioStatusBlock,
                            pCreateState->pFilePipeInfo,
                            sizeof(pCreateState->filePipeInfo),
                            FilePipeInformation);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseCreateStateAsync_SMB_V2(pCreateState); // completed sync
        }

        if (!pCreateState->pFilePipeLocalInfo)
        {
            pCreateState->pFilePipeLocalInfo = &pCreateState->filePipeLocalInfo;

            SrvPrepareCreateStateAsync_SMB_V2(pCreateState, pExecContext);

            ntStatus = IoQueryInformationFile(
                            pCreateState->pFile->hFile,
                            pCreateState->pAcb,
                            &pCreateState->ioStatusBlock,
                            pCreateState->pFilePipeLocalInfo,
                            sizeof(pCreateState->filePipeLocalInfo),
                            FilePipeLocalInformation);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseCreateStateAsync_SMB_V2(pCreateState); // completed sync
        }
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvProcessCreateContexts_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2    pCtxSmb2     = pCtxProtocol->pSmb2Context;
    PSRV_CREATE_STATE_SMB_V2    pCreateState = NULL;

    pCreateState = (PSRV_CREATE_STATE_SMB_V2)pCtxSmb2->hState;

    for (;
         pCreateState->iContext < pCreateState->ulNumContexts;
         pCreateState->iContext++)
    {
        PSRV_CREATE_CONTEXT pCreateContext =
                        &pCreateState->pCreateContexts[pCreateState->iContext];

        switch (pCreateContext->contextItemType)
        {
            case SMB2_CONTEXT_ITEM_TYPE_MAX_ACCESS:

                // TODO: Find the real max access mask on the file
                pCreateState->ulMaximalAccessMask = 0x1F01FF;

                break;

            case SMB2_CONTEXT_ITEM_TYPE_DURABLE_HANDLE:
            case SMB2_CONTEXT_ITEM_TYPE_QUERY_DISK_ID:
            case SMB2_CONTEXT_ITEM_TYPE_EXT_ATTRS:
            case SMB2_CONTEXT_ITEM_TYPE_SHADOW_COPY:
            default:

                // TODO:

                break;
        }
    }

    return ntStatus;
}

static
NTSTATUS
SrvRequestCreateOplocks_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS        ntStatus           = STATUS_SUCCESS;
    SRV_OPLOCK_INFO batchOplockChain[] =
            {
               { IO_OPLOCK_REQUEST_OPLOCK_BATCH,   SMB_OPLOCK_LEVEL_BATCH },
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2, SMB_OPLOCK_LEVEL_II    },
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    SRV_OPLOCK_INFO exclOplockChain[] =
            {
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1, SMB_OPLOCK_LEVEL_I     },
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2, SMB_OPLOCK_LEVEL_II    },
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    SRV_OPLOCK_INFO noOplockChain[] =
            {
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    PSRV_OPLOCK_INFO           pOplockCursor = NULL;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_CREATE_STATE_SMB_V2   pCreateState  = NULL;
    PSRV_OPLOCK_STATE_SMB_V2   pOplockState  = NULL;
    BOOLEAN                    bContinue     = TRUE;

    pCreateState = (PSRV_CREATE_STATE_SMB_V2)pCtxSmb2->hState;

    if (SrvTree2IsNamedPipe(pCreateState->pTree) ||
        pCreateState->fileStdInfo.Directory)
    {
        pOplockCursor = &noOplockChain[0];

        goto done;
    }

    ntStatus = SrvBuildOplockState_SMB_V2(
                    pExecContext->pConnection,
                    pCtxSmb2->pSession,
                    pCtxSmb2->pTree,
                    pCreateState->pFile,
                    &pOplockState);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pCreateState->pRequestHeader->ucOplockLevel)
    {
        case SMB2_OPLOCK_LEVEL_BATCH:

            pOplockCursor = &batchOplockChain[0];

            break;

        case SMB2_OPLOCK_LEVEL_I:

            pOplockCursor = &exclOplockChain[0];

            break;

        default:

            pOplockCursor = &noOplockChain[0];

            break;
    }

    while (bContinue && (pOplockCursor->oplockRequest != SMB_OPLOCK_LEVEL_NONE))
    {
        pOplockState->oplockBuffer_in.OplockRequestType =
                        pOplockCursor->oplockRequest;

        SrvPrepareOplockStateAsync_SMB_V2(pOplockState);

        ntStatus = IoFsControlFile(
                        pCreateState->pFile->hFile,
                        pOplockState->pAcb,
                        &pOplockState->ioStatusBlock,
                        IO_FSCTL_OPLOCK_REQUEST,
                        &pOplockState->oplockBuffer_in,
                        sizeof(pOplockState->oplockBuffer_in),
                        &pOplockState->oplockBuffer_out,
                        sizeof(pOplockState->oplockBuffer_out));
        switch (ntStatus)
        {
            case STATUS_OPLOCK_NOT_GRANTED:

                SrvReleaseOplockStateAsync_SMB_V2(pOplockState); // completed sync

                pOplockCursor++;

                break;

            case STATUS_PENDING:

                ntStatus = SrvFile2SetOplockState(
                               pCreateState->pFile,
                               pOplockState,
                               &SrvReleaseOplockStateHandle_SMB_V2);
                BAIL_ON_NT_STATUS(ntStatus);

                InterlockedIncrement(&pOplockState->refCount);

                SrvFile2SetOplockLevel(
                        pCreateState->pFile,
                        pOplockCursor->oplockLevel);

                ntStatus = STATUS_SUCCESS;

                bContinue = FALSE;

                break;

            default:

                SrvReleaseOplockStateAsync_SMB_V2(pOplockState); // completed sync

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }
    }

done:

    pCreateState->ucOplockLevel = pOplockCursor->oplockLevel;

cleanup:

    if (pOplockState)
    {
        SrvReleaseOplockState_SMB_V2(pOplockState);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvPrepareCreateStateAsync_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState,
    PSRV_EXEC_CONTEXT        pExecContext
    )
{
    pCreateState->acb.Callback        = &SrvExecuteCreateAsyncCB_SMB_V2;

    pCreateState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pCreateState->acb.AsyncCancelContext = NULL;

    pCreateState->pAcb = &pCreateState->acb;
}

static
VOID
SrvExecuteCreateAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_CREATE_STATE_SMB_V2   pCreateState     = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pCreateState = (PSRV_CREATE_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pCreateState->mutex);

    if (pCreateState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pCreateState->pAcb->AsyncCancelContext);
    }

    pCreateState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pCreateState->mutex);

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
SrvReleaseCreateStateAsync_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState
    )
{
    if (pCreateState->pAcb)
    {
        pCreateState->acb.Callback        = NULL;

        if (pCreateState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pCreateState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pCreateState->pAcb->CallbackContext = NULL;
        }

        if (pCreateState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pCreateState->pAcb->AsyncCancelContext);
        }

        pCreateState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseCreateStateHandle_SMB_V2(
    HANDLE hState
    )
{
    SrvReleaseCreateState_SMB_V2((PSRV_CREATE_STATE_SMB_V2)hState);
}

static
PSRV_CREATE_STATE_SMB_V2
SrvAcquireCreateState_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState
    )
{
    InterlockedIncrement(&pCreateState->refCount);

    return pCreateState;
}

static
VOID
SrvReleaseCreateState_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState
    )
{
    if (InterlockedDecrement(&pCreateState->refCount) == 0)
    {
        SrvFreeCreateState_SMB_V2(pCreateState);
    }
}

static
VOID
SrvFreeCreateState_SMB_V2(
    PSRV_CREATE_STATE_SMB_V2 pCreateState
    )
{
    if (pCreateState->pEcpList)
    {
        IoRtlEcpListFree(&pCreateState->pEcpList);
    }

    if (pCreateState->pAcb && pCreateState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pCreateState->pAcb->AsyncCancelContext);
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pCreateState->pFilename)
    {
        if (pCreateState->pFilename->FileName)
        {
            SrvFreeMemory(pCreateState->pFilename->FileName);
        }

        SrvFreeMemory(pCreateState->pFilename);
    }

    if (pCreateState->hFile)
    {
        IoCloseFile(pCreateState->hFile);
    }

    if (pCreateState->bRemoveFileFromTree)
    {
        NTSTATUS ntStatus2 = 0;

        SrvFile2ResetOplockState(pCreateState->pFile);

        ntStatus2 = SrvTree2RemoveFile(
                        pCreateState->pTree,
                        &pCreateState->pFile->fid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove file from tree [Tid:%u]"
                            "[Fid:persistent(%0x08X) volatile(%0x08X)][code:%d]",
                           pCreateState->pTree->ulTid,
                           pCreateState->pFile->fid.ullPersistentId,
                           pCreateState->pFile->fid.ullVolatileId,
                           ntStatus2);
        }
    }

    SRV_SAFE_FREE_MEMORY(pCreateState->pwszFilename);
    SRV_SAFE_FREE_MEMORY(pCreateState->pCreateContexts);

    if (pCreateState->pFile)
    {
        SrvFile2Release(pCreateState->pFile);
    }

    if (pCreateState->pTree)
    {
        SrvTree2Release(pCreateState->pTree);
    }

    if (pCreateState->pMutex)
    {
        pthread_mutex_destroy(&pCreateState->mutex);
    }

    SrvFreeMemory(pCreateState);
}

static
NTSTATUS
SrvBuildCreateResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PSRV_CREATE_STATE_SMB_V2   pCreateState  = NULL;
    ULONG                      iContext      = 0;
    ULONG                      ulCreateContextOffset = 0;
    ULONG                      ulCreateContextLength = 0;
    PSMB2_CREATE_CONTEXT       pPrevCreateContext    = NULL;
    PSMB2_CREATE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulBytesUsed      = 0;
    ULONG ulTotalBytesUsed = 0;

    pCreateState = (PSRV_CREATE_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_CREATE,
                    pSmbRequest->pHeader->usEpoch,
                    (pCreateState->ullAsyncId ? 0 : pSmbRequest->pHeader->usCredits),
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    pCreateState->ullAsyncId,
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

    if (ulBytesAvailable < sizeof(SMB2_CREATE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_CREATE_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulBytesUsed       = sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_CREATE_RESPONSE_HEADER);

    switch (pCreateState->ucOplockLevel)
    {
        case SMB_OPLOCK_LEVEL_BATCH:

            pResponseHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_BATCH;

            break;

        case SMB_OPLOCK_LEVEL_I:

            pResponseHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_I;

            break;

        case SMB_OPLOCK_LEVEL_II:

            pResponseHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_II;

            break;

        default:

            pResponseHeader->ucOplockLevel = SMB2_OPLOCK_LEVEL_NONE;

            break;
    }

    pResponseHeader->fid               = pCreateState->pFile->fid;
    pResponseHeader->ulCreateAction    = pCreateState->ulCreateAction;
    pResponseHeader->ullCreationTime   =
                                pCreateState->pFileBasicInfo->CreationTime;
    pResponseHeader->ullLastAccessTime =
                                pCreateState->pFileBasicInfo->LastAccessTime;
    pResponseHeader->ullLastWriteTime  =
                                pCreateState->pFileBasicInfo->LastWriteTime;
    pResponseHeader->ullLastChangeTime =
                                pCreateState->pFileBasicInfo->ChangeTime;
    pResponseHeader->ulFileAttributes  =
                                pCreateState->pFileBasicInfo->FileAttributes;
    pResponseHeader->ullAllocationSize =
                                pCreateState->pFileStdInfo->AllocationSize;
    pResponseHeader->ullEndOfFile      = pCreateState->pFileStdInfo->EndOfFile;
    pResponseHeader->usLength          = ulBytesUsed + 1;

    for (; iContext < pCreateState->ulNumContexts; iContext++)
    {
        PSMB2_CREATE_CONTEXT pCurCreateContext     = NULL;
        ULONG                ulCCBytesUsed         = 0;
        ULONG                ulAlignBytesUsed      = 0;
        PSRV_CREATE_CONTEXT  pCreateContextRequest =
                        &pCreateState->pCreateContexts[iContext];

        switch (pCreateContextRequest->contextItemType)
        {
            case SMB2_CONTEXT_ITEM_TYPE_MAX_ACCESS:

                {
                    CHAR szName[] = SMB2_CONTEXT_NAME_MAX_ACCESS;
                    SMB2_MAXIMAL_ACCESS_MASK_CREATE_CONTEXT maxAcCC = {0};

                    maxAcCC.accessMask = pCreateState->ulMaximalAccessMask;

                    ntStatus = SrvWriteCreateContext(
                                    pOutBuffer,
                                    ulOffset,
                                    ulBytesAvailable,
                                    (PBYTE)&szName[0],
                                    strlen(szName),
                                    (PBYTE)&maxAcCC,
                                    sizeof(maxAcCC),
                                    &ulAlignBytesUsed,
                                    &ulCCBytesUsed,
                                    &pCurCreateContext);
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                break;

            case SMB2_CONTEXT_ITEM_TYPE_DURABLE_HANDLE:
            case SMB2_CONTEXT_ITEM_TYPE_QUERY_DISK_ID:
            case SMB2_CONTEXT_ITEM_TYPE_EXT_ATTRS:
            case SMB2_CONTEXT_ITEM_TYPE_SHADOW_COPY:
            case SMB2_CONTEXT_ITEM_TYPE_SEC_DESC:
            default:

                // TODO:

                break;
        }

        if (pCurCreateContext)
        {
            if (!ulCreateContextOffset)
            {
                // First context
                ulCreateContextOffset = ulOffset + ulAlignBytesUsed;
            }

            if (ulAlignBytesUsed)
            {
                pOutBuffer       += ulAlignBytesUsed;
                ulBytesUsed       = ulAlignBytesUsed;
                ulOffset         += ulAlignBytesUsed;
                ulBytesAvailable -= ulAlignBytesUsed;
                ulTotalBytesUsed += ulAlignBytesUsed;

                if (pPrevCreateContext)
                {
                    pPrevCreateContext->ulNextContextOffset += ulAlignBytesUsed;
                    ulCreateContextLength += ulAlignBytesUsed;
                }
            }

            pOutBuffer       += ulCCBytesUsed;
            ulBytesUsed       = ulCCBytesUsed;
            ulOffset         += ulCCBytesUsed;
            ulBytesAvailable -= ulCCBytesUsed;
            ulTotalBytesUsed += ulCCBytesUsed;

            ulCreateContextLength += ulCCBytesUsed;

            pPrevCreateContext = pCurCreateContext;
        }
    }

    pResponseHeader->ulCreateContextOffset = ulCreateContextOffset;
    pResponseHeader->ulCreateContextLength = ulCreateContextLength;

    if (pPrevCreateContext)
    {
        pPrevCreateContext->ulNextContextOffset = 0;
    }

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader      = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
SrvWriteCreateContext(
    PBYTE                 pOutBuffer,
    ULONG                 ulOffset,
    ULONG                 ulBytesAvailable,
    PBYTE                 pName,
    USHORT                usNameSize,
    PBYTE                 pData,
    ULONG                 ulDataSize,
    PULONG                pulAlignBytesUsed,
    PULONG                pulCCBytesUsed,
    PSMB2_CREATE_CONTEXT* ppCreateContext
    )
{
    NTSTATUS             ntStatus          = STATUS_SUCCESS;
    PBYTE                pDataCursor       = pOutBuffer;
    ULONG                ulBytesAvailable1 = ulBytesAvailable;
    ULONG                ulCCBytesUsed     = 0;
    ULONG                ulAlignBytesUsed  = 0;
    PSMB2_CREATE_CONTEXT pCreateContext    = NULL;

    if (ulOffset % 4)
    {
        USHORT usAlign = 4 - (ulOffset % 4);

        if (ulBytesAvailable1 < usAlign)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulOffset          += usAlign;
        ulAlignBytesUsed  += usAlign;
        ulBytesAvailable1 -= usAlign;
        pDataCursor       += usAlign;
    }

    ntStatus = SMB2MarshalCreateContext(
                    pDataCursor,
                    ulOffset,
                    pName,
                    usNameSize,
                    pData,
                    ulDataSize,
                    ulBytesAvailable1,
                    &ulCCBytesUsed,
                    &pCreateContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreateContext->ulNextContextOffset = ulCCBytesUsed;

    *pulAlignBytesUsed = ulAlignBytesUsed;
    *pulCCBytesUsed    = ulCCBytesUsed;
    *ppCreateContext   = pCreateContext;

cleanup:

    return ntStatus;

error:

    *pulAlignBytesUsed = 0;
    *pulCCBytesUsed    = 0;
    *ppCreateContext   = NULL;

    goto cleanup;
}
