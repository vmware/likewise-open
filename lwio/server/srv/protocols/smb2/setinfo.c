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
 *        setinfo.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Set Information
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildSetInfoState_SMB_V2(
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE_2              pFile,
    PBYTE                         pData,
    PSRV_SET_INFO_STATE_SMB_V2*   ppSetInfoState
    );

static
NTSTATUS
SrvSetInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildSetInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildSetFileInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileEOFInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileDispositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileRenameInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvUnmarshalRenameHeader_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileAllocationInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildSetInfoCommonResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetFileSystemInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildSetFileSystemInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetSecurityInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetSecurityDescriptor_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildSetSecurityInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareSetInfoStateAsync_SMB_V2(
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState,
    PSRV_EXEC_CONTEXT          pExecContext
    );

static
VOID
SrvExecuteSetInfoAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseSetInfoStateAsync_SMB_V2(
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState
    );

static
VOID
SrvReleaseSetInfoStateHandle_SMB_V2(
    HANDLE hSetInfoState
    );

static
VOID
SrvReleaseSetInfoState_SMB_V2(
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState
    );

static
VOID
SrvFreeSetInfoState_SMB_V2(
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState
    );

NTSTATUS
SrvProcessSetInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;
    PLWIO_SRV_SESSION_2        pSession      = NULL;
    PLWIO_SRV_TREE_2           pTree         = NULL;
    PLWIO_SRV_FILE_2           pFile         = NULL;
    BOOLEAN                    bInLock       = FALSE;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    if (pSetInfoState)
    {
        InterlockedIncrement(&pSetInfoState->refCount);
    }
    else
    {
        ULONG                      iMsg          = pCtxSmb2->iMsg;
        PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
        PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PBYTE                         pData          = NULL; // Do not free

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

        ntStatus = SMB2UnmarshalSetInfoRequest(
                        pSmbRequest,
                        &pRequestHeader,
                        &pData);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_DEBUG(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_2,
                pSmbRequest->pHeader->command,
                "Set Info request params: "
                "command(%u),uid(%llu),cmd-seq(%llu),pid(%u),tid(%u),"
                "credits(%u),flags(0x%x),chain-offset(%u),"
                "file-id(persistent:0x%x,volatile:0x%x),"
                "info-class(0x%x),info-type(0x%x)",
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
                pRequestHeader->ucInfoType);

        ntStatus = SrvTree2FindFile_SMB_V2(
                            pCtxSmb2,
                            pTree,
                            &pRequestHeader->fid,
                            &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildSetInfoState_SMB_V2(
                            pRequestHeader,
                            pFile,
                            pData,
                            &pSetInfoState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb2->hState = pSetInfoState;
        InterlockedIncrement(&pSetInfoState->refCount);
        pCtxSmb2->pfnStateRelease = &SrvReleaseSetInfoStateHandle_SMB_V2;
    }

    LWIO_LOCK_MUTEX(bInLock, &pSetInfoState->mutex);

    switch (pSetInfoState->stage)
    {
        case SRV_SET_INFO_STAGE_SMB_V2_INITIAL:

            pSetInfoState->stage = SRV_GET_INFO_STAGE_SMB_V2_ATTEMPT_IO;

            // intentional fall through

        case SRV_SET_INFO_STAGE_SMB_V2_ATTEMPT_IO:

            ntStatus = SrvSetInfo_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pSetInfoState->stage = SRV_SET_INFO_STAGE_SMB_V2_BUILD_RESPONSE;

            // intentional fall through

        case SRV_SET_INFO_STAGE_SMB_V2_BUILD_RESPONSE:

            ntStatus = SrvBuildSetInfoResponse_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pSetInfoState->stage = SRV_SET_INFO_STAGE_SMB_V2_DONE;

        case SRV_SET_INFO_STAGE_SMB_V2_DONE:

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

    if (pSetInfoState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pSetInfoState->mutex);

        SrvReleaseSetInfoState_SMB_V2(pSetInfoState);
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

            if (pSetInfoState)
            {
                SrvReleaseSetInfoStateAsync_SMB_V2(pSetInfoState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildSetInfoState_SMB_V2(
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE_2              pFile,
    PBYTE                         pData,
    PSRV_SET_INFO_STATE_SMB_V2*   ppSetInfoState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SET_INFO_STATE_SMB_V2),
                    (PVOID*)&pSetInfoState);
    BAIL_ON_NT_STATUS(ntStatus);

    pSetInfoState->refCount = 1;

    pthread_mutex_init(&pSetInfoState->mutex, NULL);
    pSetInfoState->pMutex = &pSetInfoState->mutex;

    pSetInfoState->stage = SRV_SET_INFO_STAGE_SMB_V2_INITIAL;

    pSetInfoState->pRequestHeader = pRequestHeader;
    pSetInfoState->pData          = pData;

    pSetInfoState->pFile = SrvFile2Acquire(pFile);

    *ppSetInfoState = pSetInfoState;

cleanup:

    return ntStatus;

error:

    *ppSetInfoState = NULL;

    if (pSetInfoState)
    {
        SrvFreeSetInfoState_SMB_V2(pSetInfoState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvSetInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = SrvSetFileInfo_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = SrvSetFileSystemInfo_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = SrvSetSecurityInfo_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvBuildSetInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = SrvBuildSetFileInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = SrvBuildSetFileSystemInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = SrvBuildSetSecurityInfoResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvSetFileInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_POSITION :
        case SMB2_FILE_INFO_CLASS_MODE :
        case SMB2_FILE_INFO_CLASS_PIPE :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        case SMB2_FILE_INFO_CLASS_EOF :

            ntStatus = SrvSetFileEOFInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_DISPOSITION :

            ntStatus = SrvSetFileDispositionInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_RENAME :

            ntStatus = SrvSetFileRenameInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_BASIC :

            ntStatus = SrvSetFileBasicInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALLOCATION :

            ntStatus = SrvSetFileAllocationInfo_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvBuildSetFileInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_POSITION :
        case SMB2_FILE_INFO_CLASS_MODE :
        case SMB2_FILE_INFO_CLASS_PIPE :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        case SMB2_FILE_INFO_CLASS_EOF :
        case SMB2_FILE_INFO_CLASS_DISPOSITION :
        case SMB2_FILE_INFO_CLASS_RENAME :
        case SMB2_FILE_INFO_CLASS_BASIC :
        case SMB2_FILE_INFO_CLASS_ALLOCATION :

            ntStatus = SrvBuildSetInfoCommonResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvSetFileEOFInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                        sizeof(FILE_END_OF_FILE_INFORMATION))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        SrvPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

        ntStatus = IoSetInformationFile(
                        pSetInfoState->pFile->hFile,
                        pSetInfoState->pAcb,
                        &pSetInfoState->ioStatusBlock,
                        (PFILE_END_OF_FILE_INFORMATION)pSetInfoState->pData,
                        sizeof(FILE_END_OF_FILE_INFORMATION),
                        FileEndOfFileInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
SrvSetFileDispositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;
    PFILE_DISPOSITION_INFORMATION pFileDispositionInfo = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                    sizeof(FILE_DISPOSITION_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileDispositionInfo = (PFILE_DISPOSITION_INFORMATION)pSetInfoState->pData;

    SMB2UnmarshallBoolean(&pFileDispositionInfo->DeleteFile);

    SrvPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    pFileDispositionInfo,
                    sizeof(FILE_DISPOSITION_INFORMATION),
                    FileDispositionInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
SrvSetFileRenameInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (!pSetInfoState->pData2)
    {
        ntStatus = SrvUnmarshalRenameHeader_SMB_V2(pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pSetInfoState->pRootDir)
    {
        ((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->RootDirectory =
                                                pSetInfoState->pRootDir->hFile;
    }
    else if (!pSetInfoState->hDir)
    {
        ntStatus = SrvBuildTreeRelativePath_SMB_V2(
                        pCtxSmb2->pTree,
                        NULL,
                        &pSetInfoState->dirPath);
        BAIL_ON_NT_STATUS(ntStatus);

        // Catch failed CreateFile calls when they come back around

        ntStatus = pSetInfoState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        SrvPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

        ntStatus = SrvIoCreateFile(
                                pCtxSmb2->pTree->pShareInfo,
                                &pSetInfoState->hDir,
                                pSetInfoState->pAcb,
                                &pSetInfoState->ioStatusBlock,
                                pCtxSmb2->pSession->pIoSecurityContext,
                                &pSetInfoState->dirPath,
                                (PSECURITY_DESCRIPTOR_RELATIVE)pSetInfoState->pSecurityDescriptor,
                                pSetInfoState->pSecurityQOS,
                                GENERIC_READ,
                                0,
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                FILE_OPEN,
                                FILE_DIRECTORY_FILE,
                                NULL, /* EA Buffer */
                                0,    /* EA Length */
                                pSetInfoState->pEcpList
                                );
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync
    }

    if (!pSetInfoState->pRootDir)
    {
        ((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->RootDirectory =
                                                pSetInfoState->hDir;
    }

    SrvPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    (PFILE_RENAME_INFORMATION)pSetInfoState->pData2,
                    pSetInfoState->ulData2Length,
                    FileRenameInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
SrvUnmarshalRenameHeader_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;
    BOOLEAN                    bTreeInLock   = FALSE;
    PSMB2_FILE_RENAME_INFO_HEADER pRenameInfoHeader = NULL;
    ULONG                         ulBytesAvailable  = 0;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                    sizeof(SMB2_FILE_RENAME_INFO_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pRenameInfoHeader = (PSMB2_FILE_RENAME_INFO_HEADER)pSetInfoState->pData;

    ulBytesAvailable = pSetInfoState->pRequestHeader->ulInputBufferLen;
    ulBytesAvailable -= sizeof(SMB2_FILE_RENAME_INFO_HEADER);
    ulBytesAvailable += sizeof(wchar16_t);

    if (ulBytesAvailable < pRenameInfoHeader->ulFileNameLength)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pRenameInfoHeader->ullRootDir)
    {
        // TODO: Figure out if this one is a persistent or volatile fid
        SMB2_FID rootDirFid = { .ullPersistentId = 0xFFFFFFFFFFFFFFFFLL,
                                .ullVolatileId   = pRenameInfoHeader->ullRootDir
                              };

        LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pCtxSmb2->pTree->mutex);

        ntStatus = SrvTree2FindFile(
                        pCtxSmb2->pTree,
                        &rootDirFid,
                        &pSetInfoState->pRootDir);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb2->pTree->mutex);
    }

    pSetInfoState->ulData2Length =
          sizeof(FILE_RENAME_INFORMATION) + pRenameInfoHeader->ulFileNameLength;

    ntStatus = SrvAllocateMemory(
                    pSetInfoState->ulData2Length,
                    (PVOID*)&pSetInfoState->pData2);
    BAIL_ON_NT_STATUS(ntStatus);

    ((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->ReplaceIfExists =
                    pRenameInfoHeader->ucReplaceIfExists ? TRUE : FALSE;
    ((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->FileNameLength =
                    pRenameInfoHeader->ulFileNameLength;

    memcpy((PBYTE)((PFILE_RENAME_INFORMATION)pSetInfoState->pData2)->FileName,
           (PBYTE)pRenameInfoHeader->wszFileName,
           pRenameInfoHeader->ulFileNameLength);

error:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb2->pTree->mutex);

    return ntStatus;
}

static
NTSTATUS
SrvSetFileBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                    sizeof(FILE_BASIC_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SrvPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    (PFILE_BASIC_INFORMATION)pSetInfoState->pData,
                    sizeof(FILE_BASIC_INFORMATION),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
SrvSetFileAllocationInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (pSetInfoState->pRequestHeader->ulInputBufferLen <
                    sizeof(FILE_ALLOCATION_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SrvPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    (PFILE_ALLOCATION_INFORMATION)pSetInfoState->pData,
                    sizeof(FILE_ALLOCATION_INFORMATION),
                    FileAllocationInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildSetInfoCommonResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PSMB2_SET_INFO_RESPONSE_HEADER pResponseHeader = NULL; // Do not free

    ntStatus = SrvCreditorAdjustCredits(
                    pExecContext->pConnection->pCreditor,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pExecContext->ullAsyncId,
                    pSmbRequest->pHeader->usCredits,
                    &pExecContext->usCreditsGranted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_SETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pExecContext->usCreditsGranted,
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

    if (ulBytesAvailable < sizeof(SMB2_SET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_SET_INFO_RESPONSE_HEADER)pOutBuffer;
    pResponseHeader->usLength = sizeof(SMB2_SET_INFO_RESPONSE_HEADER);

    // pOutBuffer += sizeof(SMB2_SET_INFO_RESPONSE_HEADER);
    // ulOffset += sizeof(SMB2_SET_INFO_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(SMB2_SET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_SET_INFO_RESPONSE_HEADER);

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
NTSTATUS
SrvSetFileSystemInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvBuildSetFileSystemInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvSetSecurityInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_SEC_INFO_CLASS_BASIC:

            ntStatus = SrvSetSecurityDescriptor_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvSetSecurityDescriptor_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2     pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2   pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    if (!pSetInfoState->pRequestHeader->ulAdditionalInfo ||
        !pSetInfoState->pRequestHeader->ulInputBufferLen)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SrvPrepareSetInfoStateAsync_SMB_V2(pSetInfoState, pExecContext);

    ntStatus = IoSetSecurityFile(
                    pSetInfoState->pFile->hFile,
                    pSetInfoState->pAcb,
                    &pSetInfoState->ioStatusBlock,
                    pSetInfoState->pRequestHeader->ulAdditionalInfo,
                    (PSECURITY_DESCRIPTOR_RELATIVE)pSetInfoState->pData,
                    pSetInfoState->pRequestHeader->ulInputBufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseSetInfoStateAsync_SMB_V2(pSetInfoState); // completed sync

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildSetSecurityInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState = NULL;

    pSetInfoState = (PSRV_SET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pSetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_SEC_INFO_CLASS_BASIC:

            ntStatus = SrvBuildSetInfoCommonResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
VOID
SrvPrepareSetInfoStateAsync_SMB_V2(
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState,
    PSRV_EXEC_CONTEXT          pExecContext
    )
{
    pSetInfoState->acb.Callback        = &SrvExecuteSetInfoAsyncCB_SMB_V2;

    pSetInfoState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pSetInfoState->acb.AsyncCancelContext = NULL;

    pSetInfoState->pAcb = &pSetInfoState->acb;
}

static
VOID
SrvExecuteSetInfoAsyncCB_SMB_V2(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState    = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pSetInfoState =
        (PSRV_SET_INFO_STATE_SMB_V2)pProtocolContext->pSmb2Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pSetInfoState->mutex);

    if (pSetInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pSetInfoState->pAcb->AsyncCancelContext);
    }

    pSetInfoState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pSetInfoState->mutex);

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
SrvReleaseSetInfoStateAsync_SMB_V2(
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState
    )
{
    if (pSetInfoState->pAcb)
    {
        pSetInfoState->acb.Callback = NULL;

        if (pSetInfoState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pSetInfoState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pSetInfoState->pAcb->CallbackContext = NULL;
        }

        if (pSetInfoState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pSetInfoState->pAcb->AsyncCancelContext);
        }

        pSetInfoState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseSetInfoStateHandle_SMB_V2(
    HANDLE hSetInfoState
    )
{
    return SrvReleaseSetInfoState_SMB_V2(
                    (PSRV_SET_INFO_STATE_SMB_V2)hSetInfoState);
}

static
VOID
SrvReleaseSetInfoState_SMB_V2(
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState
    )
{
    if (InterlockedDecrement(&pSetInfoState->refCount) == 0)
    {
        SrvFreeSetInfoState_SMB_V2(pSetInfoState);
    }
}

static
VOID
SrvFreeSetInfoState_SMB_V2(
    PSRV_SET_INFO_STATE_SMB_V2 pSetInfoState
    )
{
    if (pSetInfoState->pAcb && pSetInfoState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pSetInfoState->pAcb->AsyncCancelContext);
    }

    if (pSetInfoState->pEcpList)
    {
        IoRtlEcpListFree(&pSetInfoState->pEcpList);
    }

    if (pSetInfoState->pFile)
    {
        SrvFile2Release(pSetInfoState->pFile);
    }

    if (pSetInfoState->pRootDir)
    {
        SrvFile2Release(pSetInfoState->pRootDir);
    }

    if (pSetInfoState->hDir)
    {
        IoCloseFile(pSetInfoState->hDir);
    }

    if (pSetInfoState->dirPath.FileName)
    {
        SrvFreeMemory(pSetInfoState->dirPath.FileName);
    }

    if (pSetInfoState->pData2)
    {
        SrvFreeMemory(pSetInfoState->pData2);
    }

    if (pSetInfoState->pMutex)
    {
        pthread_mutex_destroy(&pSetInfoState->mutex);
    }

    SrvFreeMemory(pSetInfoState);
}
