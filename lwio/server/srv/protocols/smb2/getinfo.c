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
    PLWIO_SRV_CONNECTION          pConnection,
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
SrvGetFileInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileInternalInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileEAInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileStandardInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileNetworkOpenInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileAccessInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileAccessInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFilePositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFilePositionInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileModeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileModeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileAllInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileAllInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileAlignmentInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileAlignmentInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileAltNameInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileAltNameInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileAttrTagInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileAttrTagInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileStreamInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileStreamInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvMarshallFileStreamResponse(
    PBYTE  pOutBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulDataLength,
    PULONG pulAlignBytes,
    PULONG pulBytesUsed
    );

static
NTSTATUS
SrvGetFileFullEAInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileFullEAInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvMarshallFileFullEAResponse(
    PBYTE  pOutBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulDataLength,
    PULONG pulAlignBytes,
    PULONG pulBytesUsed
    );

static
NTSTATUS
SrvGetFileCompressionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileCompressionInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileSystemInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileSystemVolumeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileSystemVolumeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileSystemAttributeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileSystemAttributeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileSystemFullInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileSystemFullInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileSystemSizeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileSystemSizeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildGetInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileInternalInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileEAInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileBasicInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileStandardInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileNetworkOpenInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileSystemInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseGetInfoStateHandle_SMB_V2(
    HANDLE hGetInfoState
    );

static
NTSTATUS
SrvGetSecurityInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteQuerySecurityDescriptor_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildSecurityInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareGetInfoStateAsync_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState,
    PSRV_EXEC_CONTEXT          pExecContext
    );

static
VOID
SrvExecuteGetInfoAsyncCB_SMB_V2(
    PVOID pContext
    );

static
VOID
SrvReleaseGetInfoStateAsync_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState
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

        ntStatus = SrvSession2FindTree_SMB_V2(
                        pCtxSmb2,
                        pSession,
                        pSmbRequest->pHeader->ulTid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMB2UnmarshalGetInfoRequest(pSmbRequest, &pRequestHeader);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvTree2FindFile_SMB_V2(
                            pCtxSmb2,
                            pTree,
                            &pRequestHeader->fid,
                            &pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildGetInfoState_SMB_V2(
                            pRequestHeader,
                            pConnection,
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
    PLWIO_SRV_CONNECTION          pConnection,
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

    pGetInfoState->pConnection = pConnection;
    InterlockedIncrement(&pConnection->refCount);

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

static
NTSTATUS
SrvBuildFileInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_INTERNAL :

            ntStatus = SrvBuildFileInternalInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_EA :

            ntStatus = SrvBuildFileEAInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_BASIC :

            ntStatus = SrvBuildFileBasicInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_STANDARD :

            ntStatus = SrvBuildFileStandardInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_NETWORK_OPEN :

            ntStatus = SrvBuildFileNetworkOpenInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ACCESS :

            ntStatus = SrvBuildFileAccessInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_POSITION :

            ntStatus = SrvBuildFilePositionInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_MODE :

            ntStatus = SrvBuildFileModeInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALL :

            ntStatus = SrvBuildFileAllInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALIGNMENT :

            ntStatus = SrvBuildFileAlignmentInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALTERNATE_NAME :

            ntStatus = SrvBuildFileAltNameInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ATTRIBUTE_TAG :

            ntStatus = SrvBuildFileAttrTagInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_STREAM :

            ntStatus = SrvBuildFileStreamInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_FULL_EA :

            ntStatus = SrvBuildFileFullEAInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_COMPRESSION :

            ntStatus = SrvBuildFileCompressionInfoResponse_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileSystemInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FS_INFO_CLASS_VOLUME:

            ntStatus = SrvBuildFileSystemVolumeInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FS_INFO_CLASS_ATTRIBUTE:

            ntStatus = SrvBuildFileSystemAttributeInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FS_INFO_CLASS_FULL_SIZE:

            ntStatus = SrvBuildFileSystemFullInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB2_FS_INFO_CLASS_SIZE:

            ntStatus = SrvBuildFileSystemSizeInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB_FS_INFO_CLASS_OBJECTID:
        case SMB2_FS_INFO_CLASS_DEVICE:
        case SMB2_FS_INFO_CLASS_QUOTA:

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvGetFileInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_INTERNAL :

            ntStatus = SrvGetFileInternalInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_EA :

            ntStatus = SrvGetFileEAInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_BASIC :

            ntStatus = SrvGetFileBasicInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_STANDARD :

            ntStatus = SrvGetFileStandardInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_NETWORK_OPEN :

            if (SrvTree2IsNamedPipe(pCtxSmb2->pTree))
            {
                ntStatus = STATUS_INVALID_PARAMETER;
            }
            else
            {
                ntStatus = SrvGetFileNetworkOpenInfo_SMB_V2(pExecContext);
            }

            break;

        case SMB2_FILE_INFO_CLASS_ACCESS :

            ntStatus = SrvGetFileAccessInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_POSITION :

            ntStatus = SrvGetFilePositionInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_MODE :

            ntStatus = SrvGetFileModeInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALL :

            ntStatus = SrvGetFileAllInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALIGNMENT :

            ntStatus = SrvGetFileAlignmentInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ALTERNATE_NAME :

            ntStatus = SrvGetFileAltNameInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_ATTRIBUTE_TAG :

            ntStatus = SrvGetFileAttrTagInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_STREAM :

            ntStatus = SrvGetFileStreamInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_FULL_EA :

            ntStatus = SrvGetFileFullEAInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FILE_INFO_CLASS_COMPRESSION :

            ntStatus = SrvGetFileCompressionInfo_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvGetFileInternalInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_INTERNAL_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_INTERNAL_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileInternalInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState); // sync completion
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileInternalInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_INTERNAL_INFORMATION pFileInternalInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FILE_INTERNAL_INFO_HEADER pFileInternalInfoHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    pFileInternalInfo = (PFILE_INTERNAL_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FILE_INTERNAL_INFO_HEADER);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileInternalInfoHeader = (PSMB_FILE_INTERNAL_INFO_HEADER)pOutBuffer;

    // ulTotalBytesUsed += sizeof(SMB_FILE_INTERNAL_INFO_HEADER);
    // ulBytesAvailable -= sizeof(SMB_FILE_INTERNAL_INFO_HEADER);
    // pOutBuffer += sizeof(SMB_FILE_INTERNAL_INFO_HEADER);

    pFileInternalInfoHeader->ullIndex = pFileInternalInfo->IndexNumber;

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileEAInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_EA_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_EA_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileEaInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileEAInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_EA_INFORMATION           pFileEAInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FILE_EA_INFO_HEADER       pFileEAInfoHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileEAInfo = (PFILE_EA_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FILE_EA_INFO_HEADER);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileEAInfoHeader = (PSMB_FILE_EA_INFO_HEADER)pOutBuffer;

    // ulTotalBytesUsed += sizeof(SMB_FILE_EA_INFO_HEADER);
    // ulBytesAvailable -= sizeof(SMB_FILE_EA_INFO_HEADER);
    // pOutBuffer += sizeof(SMB_FILE_EA_INFO_HEADER);

    pFileEAInfoHeader->ulEaSize = pFileEAInfo->EaSize;

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_BASIC_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_BASIC_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileBasicInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileBasicInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_BASIC_INFORMATION        pFileBasicInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileBasicInfo = (PFILE_BASIC_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(FILE_BASIC_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileBasicInfo, sizeof(FILE_BASIC_INFORMATION));

    // pOutBuffer += sizeof(FILE_BASIC_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_BASIC_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileStandardInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_STANDARD_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_STANDARD_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileStandardInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileStandardInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_STANDARD_INFORMATION     pFileStandardInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileStandardInfo = (PFILE_STANDARD_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                            sizeof(FILE_STANDARD_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileStandardInfo, sizeof(FILE_STANDARD_INFORMATION));

    // pOutBuffer += sizeof(FILE_STANDARD_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_STANDARD_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileNetworkOpenInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_NETWORK_OPEN_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_NETWORK_OPEN_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileNetworkOpenInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileNetworkOpenInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_NETWORK_OPEN_INFORMATION pFileNetworkOpenInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileNetworkOpenInfo = (PFILE_NETWORK_OPEN_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                            sizeof(FILE_NETWORK_OPEN_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileNetworkOpenInfo, sizeof(FILE_NETWORK_OPEN_INFORMATION));

    // pOutBuffer += sizeof(FILE_NETWORK_OPEN_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_NETWORK_OPEN_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileAccessInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_ACCESS_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_ACCESS_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileAccessInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileAccessInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_ACCESS_INFORMATION pFileAccessInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileAccessInfo = (PFILE_ACCESS_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(FILE_ACCESS_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileAccessInfo, sizeof(FILE_ACCESS_INFORMATION));

    // pOutBuffer += sizeof(FILE_ACCESS_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_ACCESS_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFilePositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_POSITION_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_POSITION_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FilePositionInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFilePositionInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_POSITION_INFORMATION pFilePositionInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFilePositionInfo = (PFILE_POSITION_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    sizeof(FILE_POSITION_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFilePositionInfo, sizeof(FILE_POSITION_INFORMATION));

    // pOutBuffer += sizeof(FILE_POSITION_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_POSITION_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileModeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_MODE_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_MODE_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileModeInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileModeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_MODE_INFORMATION pFileModeInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileModeInfo = (PFILE_MODE_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    sizeof(FILE_MODE_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileModeInfo, sizeof(FILE_MODE_INFORMATION));

    // pOutBuffer += sizeof(FILE_MODE_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_MODE_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileAllInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    BOOLEAN                    bContinue     = TRUE;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    do
    {
        ntStatus = pGetInfoState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                {
                    ULONG ulNewSize =  0;

                    if (!pGetInfoState->ulDataLength)
                    {
                        ulNewSize = pGetInfoState->ulDataLength +
                                        sizeof(FILE_ALL_INFORMATION) +
                                        256 * sizeof(wchar16_t);
                    }
                    else
                    {
                        ulNewSize = pGetInfoState->ulDataLength +
                                        256 * sizeof(wchar16_t);
                    }

                    ntStatus = SMBReallocMemory(
                                    pGetInfoState->pData2,
                                    (PVOID*)&pGetInfoState->pData2,
                                    ulNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pGetInfoState->ulDataLength = ulNewSize;

                    SrvPrepareGetInfoStateAsync_SMB_V2(
                                    pGetInfoState,
                                    pExecContext);

                    ntStatus = IoQueryInformationFile(
                                            pCtxSmb2->pFile->hFile,
                                            pGetInfoState->pAcb,
                                            &pGetInfoState->ioStatusBlock,
                                            pGetInfoState->pData2,
                                            pGetInfoState->ulDataLength,
                                            FileAllInformation);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

                            pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;

                            // intentional fall through

                        case STATUS_BUFFER_TOO_SMALL:

                            // synchronous completion
                            SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);

                            break;

                        default:

                            BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            case STATUS_SUCCESS:

                if (!pGetInfoState->pData2)
                {
                    pGetInfoState->ioStatusBlock.Status =
                                            STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;

                    bContinue = FALSE;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileAllInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_ALL_INFORMATION pFileAllInfo = NULL;
    PSMB2_FILE_ALL_INFORMATION_HEADER pFileAllInfoHeader = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileAllInfo = (PFILE_ALL_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                    sizeof(SMB2_FILE_ALL_INFORMATION_HEADER) +
                    pFileAllInfo->NameInformation.FileNameLength;

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileAllInfoHeader = (PSMB2_FILE_ALL_INFORMATION_HEADER)pOutBuffer;
    pFileAllInfoHeader->llChangeTime =
                            pFileAllInfo->BasicInformation.ChangeTime;
    pFileAllInfoHeader->llCreationTime =
                            pFileAllInfo->BasicInformation.CreationTime;
    pFileAllInfoHeader->llLastAccessTime =
                            pFileAllInfo->BasicInformation.LastAccessTime;
    pFileAllInfoHeader->llLastWriteTime =
                            pFileAllInfo->BasicInformation.LastWriteTime;
    pFileAllInfoHeader->ulFileAttributes =
                            pFileAllInfo->BasicInformation.FileAttributes;
    pFileAllInfoHeader->ullAllocationSize =
                            pFileAllInfo->StandardInformation.AllocationSize;
    pFileAllInfoHeader->ullEndOfFile =
                            pFileAllInfo->StandardInformation.EndOfFile;
    pFileAllInfoHeader->ulNumberOfLinks =
                            pFileAllInfo->StandardInformation.NumberOfLinks;
    pFileAllInfoHeader->ucDeletePending =
                            pFileAllInfo->StandardInformation.DeletePending;
    pFileAllInfoHeader->ucIsDirectory =
                            pFileAllInfo->StandardInformation.Directory;
    pFileAllInfoHeader->ullIndexNumber =
                            pFileAllInfo->InternalInformation.IndexNumber;
    pFileAllInfoHeader->ulEaSize =
                            pFileAllInfo->EaInformation.EaSize;
    pFileAllInfoHeader->ulAccessMask =
                            pFileAllInfo->AccessInformation.AccessFlags;
    pFileAllInfoHeader->ullCurrentByteOffset =
                            pFileAllInfo->PositionInformation.CurrentByteOffset;
    pFileAllInfoHeader->ulMode =
                            pFileAllInfo->ModeInformation.Mode;
    pFileAllInfoHeader->ulAlignment =
                        pFileAllInfo->AlignmentInformation.AlignmentRequirement;
    pFileAllInfoHeader->ulFilenameLength =
                            pFileAllInfo->NameInformation.FileNameLength;

    if (pFileAllInfoHeader->ulFilenameLength)
    {
        pOutBuffer += sizeof(SMB2_FILE_ALL_INFORMATION_HEADER);

        memcpy( pOutBuffer,
                (PBYTE)pFileAllInfo->NameInformation.FileName,
                pFileAllInfoHeader->ulFilenameLength);
    }

    // pOutBuffer += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileAlignmentInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_ALIGNMENT_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_ALIGNMENT_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileAlignmentInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileAlignmentInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_ALIGNMENT_INFORMATION pFileAlignmentInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileAlignmentInfo = (PFILE_ALIGNMENT_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    sizeof(FILE_ALIGNMENT_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileAlignmentInfo, sizeof(FILE_ALIGNMENT_INFORMATION));

    // pOutBuffer += sizeof(FILE_ALIGNMENT_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_ALIGNMENT_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileAltNameInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    BOOLEAN                    bContinue    = TRUE;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    do
    {
        ntStatus = pGetInfoState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                {
                    ULONG ulNewSize =  0;

                    if (!pGetInfoState->ulDataLength)
                    {
                        ulNewSize = sizeof(FILE_NAME_INFORMATION) +
                                        256 * sizeof(wchar16_t);
                    }
                    else
                    {
                        ulNewSize = pGetInfoState->ulDataLength +
                                        256 * sizeof(wchar16_t);
                    }

                    ntStatus = SMBReallocMemory(
                                    pGetInfoState->pData2,
                                    (PVOID*)&pGetInfoState->pData2,
                                    ulNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pGetInfoState->ulDataLength = ulNewSize;

                    SrvPrepareGetInfoStateAsync_SMB_V2(
                                    pGetInfoState,
                                    pExecContext);

                    ntStatus = IoQueryInformationFile(
                                            pGetInfoState->pFile->hFile,
                                            pGetInfoState->pAcb,
                                            &pGetInfoState->ioStatusBlock,
                                            pGetInfoState->pData2,
                                            pGetInfoState->ulDataLength,
                                            FileAlternateNameInformation);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

                            pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;

                            // intentional fall through

                        case STATUS_BUFFER_TOO_SMALL:

                            // synchronous completion
                            SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);

                            break;

                        default:

                            BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            case STATUS_SUCCESS:

                if (!pGetInfoState->pData2)
                {
                    pGetInfoState->ioStatusBlock.Status =
                                            STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    pGetInfoState->ulActualDataLength =
                        pGetInfoState->ioStatusBlock.BytesTransferred;

                    bContinue = FALSE;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileAltNameInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_NAME_INFORMATION pFileNameInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileNameInfo = (PFILE_NAME_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    pGetInfoState->ulActualDataLength;

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pGetInfoState->ulActualDataLength)
    {
        memcpy( pOutBuffer,
                pGetInfoState->pData2,
                pGetInfoState->ulActualDataLength);
    }

    // pOutBuffer += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileAttrTagInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_ATTRIBUTE_TAG_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileAttributeTagInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileAttrTagInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_ATTRIBUTE_TAG_INFORMATION pFileAttrTagInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileAttrTagInfo = (PFILE_ATTRIBUTE_TAG_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                                    sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memcpy(pOutBuffer, pFileAttrTagInfo, sizeof(FILE_ATTRIBUTE_TAG_INFORMATION));

    // pOutBuffer += sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
    // ulBytesAvailable -= sizeof(FILE_ATTRIBUTE_TAG_INFORMATION);
    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileStreamInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    BOOLEAN                    bContinue    = TRUE;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    do
    {
        ntStatus = pGetInfoState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                {
                    ULONG ulNewSize =  0;

                    if (pGetInfoState->ulDataLength >=
                             pGetInfoState->pRequestHeader->ulOutputBufferLen)
                    {
                        bContinue = FALSE;
                    }
                    else if (!pGetInfoState->ulDataLength)
                    {
                        ulNewSize = sizeof(FILE_STREAM_INFORMATION) +
                                        256 * sizeof(wchar16_t);
                    }
                    else
                    {
                        ulNewSize = pGetInfoState->ulDataLength +
                                        256 * sizeof(wchar16_t);
                    }

                    ntStatus = SMBReallocMemory(
                                    pGetInfoState->pData2,
                                    (PVOID*)&pGetInfoState->pData2,
                                    ulNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pGetInfoState->ulDataLength = ulNewSize;

                    SrvPrepareGetInfoStateAsync_SMB_V2(
                                    pGetInfoState,
                                    pExecContext);

                    ntStatus = IoQueryInformationFile(
                                            pGetInfoState->pFile->hFile,
                                            pGetInfoState->pAcb,
                                            &pGetInfoState->ioStatusBlock,
                                            pGetInfoState->pData2,
                                            pGetInfoState->ulDataLength,
                                            FileStreamInformation);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

                            pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;

                            // intentional fall through

                        case STATUS_BUFFER_TOO_SMALL:

                            // synchronous completion
                            SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);

                            break;

                        default:

                            BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            case STATUS_SUCCESS:

                if (!pGetInfoState->pData2)
                {
                    pGetInfoState->ioStatusBlock.Status =
                                            STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    bContinue = FALSE;

                    pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileStreamInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_STREAM_INFORMATION pFileStreamInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileStreamInfo = (PFILE_STREAM_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    if (pGetInfoState->ulActualDataLength)
    {
        ULONG ulAlignBytes = 0;

        ntStatus = SrvMarshallFileStreamResponse(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        pGetInfoState->pData2,
                        pGetInfoState->ulActualDataLength,
                        &ulAlignBytes,
                        &pGetInfoResponseHeader->ulOutBufferLength);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoResponseHeader->usOutBufferOffset += ulAlignBytes;
    }
    else
    {
        pGetInfoResponseHeader->ulOutBufferLength = 0;
    }

    // pOutBuffer       += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed    += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvMarshallFileStreamResponse(
    PBYTE  pOutBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulDataLength,
    PULONG pulAlignBytes,
    PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus          = 0;
    ULONG    ulBytesUsed       = 0;
    ULONG    iInfoCount        = 0;
    ULONG    ulInfoCount       = 0;
    ULONG    ulOffset1         = 0;
    ULONG    ulBytesAvailable1 = 0;
    ULONG    ulAlignBytes      = 0;
    PFILE_STREAM_INFORMATION             pFileStreamInfoCursor = NULL;
    PSMB2_FILE_STREAM_INFORMATION_HEADER pInfoHeaderPrev       = NULL;
    PSMB2_FILE_STREAM_INFORMATION_HEADER pInfoHeaderCur        = NULL;

    if (ulOffset % 4)
    {
        USHORT usAlign = 4 - (ulOffset % 4);

        if (ulBytesAvailable < usAlign)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulAlignBytes       = usAlign;
        ulOffset          += usAlign;
        ulBytesUsed       += usAlign;
        ulBytesAvailable  -= usAlign;
    }

    if (ulBytesAvailable < ulDataLength)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable1 = ulDataLength;

    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pData;
    while (pFileStreamInfoCursor && (ulBytesAvailable1 > 0))
    {
        ULONG  ulInfoBytesRequired = 0;

        ulInfoBytesRequired += sizeof(SMB2_FILE_STREAM_INFORMATION_HEADER);
        ulInfoBytesRequired += pFileStreamInfoCursor->StreamNameLength;

        /* Null terminate all stream names but the last. */
        if (pFileStreamInfoCursor->NextEntryOffset != 0)
        {
            ulInfoBytesRequired += sizeof(wchar16_t);

            if (ulInfoBytesRequired % 8)
            {
                ulInfoBytesRequired += 8 - (ulInfoBytesRequired % 8);
            }
        }

        if (ulBytesAvailable1 < ulInfoBytesRequired)
        {
            break;
        }

        ulInfoCount++;

        ulBytesAvailable1 -= ulInfoBytesRequired;

        if (pFileStreamInfoCursor->NextEntryOffset)
        {
            pFileStreamInfoCursor =
                (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfoCursor) +
                                        pFileStreamInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileStreamInfoCursor = NULL;
        }
    }

    pOutBuffer += ulAlignBytes;
    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pData;

    for (; iInfoCount < ulInfoCount; iInfoCount++)
    {
        pInfoHeaderPrev = pInfoHeaderCur;
        pInfoHeaderCur = (PSMB2_FILE_STREAM_INFORMATION_HEADER)pOutBuffer;

        /* Update next entry offset for previous entry. */
        if (pInfoHeaderPrev != NULL)
        {
            pInfoHeaderPrev->ulNextEntryOffset = ulOffset1;
        }

        /* Reset the offset to 0 since it's relative. */
        ulOffset1 = 0;

        /* Add the header info. */
        pInfoHeaderCur->ulNextEntryOffset = 0;
        pInfoHeaderCur->llStreamAllocationSize =
                            pFileStreamInfoCursor->StreamAllocationSize;
        pInfoHeaderCur->ullStreamSize = pFileStreamInfoCursor->StreamSize;
        pInfoHeaderCur->ulStreamNameLength =
                            pFileStreamInfoCursor->StreamNameLength;

        pOutBuffer  += sizeof(SMB2_FILE_STREAM_INFORMATION_HEADER);
        ulBytesUsed += sizeof(SMB2_FILE_STREAM_INFORMATION_HEADER);
        ulOffset1   += sizeof(SMB2_FILE_STREAM_INFORMATION_HEADER);

        memcpy( pOutBuffer,
                pFileStreamInfoCursor->StreamName,
                pFileStreamInfoCursor->StreamNameLength);

        pOutBuffer  += pFileStreamInfoCursor->StreamNameLength;
        ulBytesUsed += pFileStreamInfoCursor->StreamNameLength;
        ulOffset1   += pFileStreamInfoCursor->StreamNameLength;

        /* Null terminate all streams names but the last. */
        if (pFileStreamInfoCursor->NextEntryOffset != 0)
        {
            pOutBuffer  += sizeof(wchar16_t);
            ulBytesUsed += sizeof(wchar16_t);
            ulOffset1   += sizeof(wchar16_t);

            if (ulOffset1 % 8)
            {
                USHORT usAlign = 8 - (ulOffset1 % 8);

                pOutBuffer  += usAlign;
                ulBytesUsed += usAlign;
                ulOffset1   += usAlign;
            }
        }

        pFileStreamInfoCursor =
                    (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfoCursor) +
                                    pFileStreamInfoCursor->NextEntryOffset);
    }

    *pulAlignBytes = ulAlignBytes;
    *pulBytesUsed  = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulAlignBytes = 0;
    *pulBytesUsed  = 0;

    if (ulBytesUsed)
    {
        memset(pOutBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetFileFullEAInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    BOOLEAN                    bContinue    = TRUE;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    do
    {
        ntStatus = pGetInfoState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                {
                    ULONG ulNewSize =  0;

                    if (pGetInfoState->ulDataLength >=
                             pGetInfoState->pRequestHeader->ulOutputBufferLen)
                    {
                        bContinue = FALSE;
                    }
                    else if (!pGetInfoState->ulDataLength)
                    {
                        ulNewSize = sizeof(FILE_FULL_EA_INFORMATION) +
                                        256 * sizeof(wchar16_t);
                    }
                    else
                    {
                        ulNewSize = pGetInfoState->ulDataLength +
                                        256 * sizeof(wchar16_t);
                    }

                    ntStatus = SMBReallocMemory(
                                    pGetInfoState->pData2,
                                    (PVOID*)&pGetInfoState->pData2,
                                    ulNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pGetInfoState->ulDataLength = ulNewSize;

                    SrvPrepareGetInfoStateAsync_SMB_V2(
                                    pGetInfoState,
                                    pExecContext);

                    ntStatus = IoQueryInformationFile(
                                            pGetInfoState->pFile->hFile,
                                            pGetInfoState->pAcb,
                                            &pGetInfoState->ioStatusBlock,
                                            pGetInfoState->pData2,
                                            pGetInfoState->ulDataLength,
                                            FileFullEaInformation);
                    switch (ntStatus)
                    {
                        case STATUS_SUCCESS:

                            bContinue = FALSE;

                            pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;

                            // intentional fall through

                        case STATUS_BUFFER_TOO_SMALL:

                            // synchronous completion
                            SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);

                            break;

                        default:

                            BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            case STATUS_SUCCESS:

                if (!pGetInfoState->pData2)
                {
                    pGetInfoState->ioStatusBlock.Status =
                                            STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    bContinue = FALSE;

                    pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileFullEAInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_FULL_EA_INFORMATION pFileFullEaInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileFullEaInfo = (PFILE_FULL_EA_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    if (pGetInfoState->ulActualDataLength)
    {
        ULONG ulAlignBytes = 0;

        ntStatus = SrvMarshallFileFullEAResponse(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        pGetInfoState->pData2,
                        pGetInfoState->ulActualDataLength,
                        &ulAlignBytes,
                        &pGetInfoResponseHeader->ulOutBufferLength);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoResponseHeader->usOutBufferOffset += ulAlignBytes;
    }
    else
    {
        pGetInfoResponseHeader->ulOutBufferLength = 0;
    }

    // pOutBuffer       += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed    += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvMarshallFileFullEAResponse(
    PBYTE  pOutBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulDataLength,
    PULONG pulAlignBytes,
    PULONG pulBytesUsed
    )
{
    NTSTATUS ntStatus          = 0;
    ULONG    ulBytesUsed       = 0;
    ULONG    iInfoCount        = 0;
    ULONG    ulInfoCount       = 0;
    ULONG    ulOffset1         = 0;
    ULONG    ulBytesAvailable1 = 0;
    ULONG    ulAlignBytes      = 0;
    PFILE_FULL_EA_INFORMATION             pFileFullEAInfoCursor = NULL;
    PSMB2_FILE_FULL_EA_INFORMATION_HEADER pInfoHeaderPrev       = NULL;
    PSMB2_FILE_FULL_EA_INFORMATION_HEADER pInfoHeaderCur        = NULL;

    if (ulOffset % 4)
    {
        USHORT usAlign = 4 - (ulOffset % 4);

        if (ulBytesAvailable < usAlign)
        {
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulAlignBytes       = usAlign;
        ulOffset          += usAlign;
        ulBytesUsed       += usAlign;
        ulBytesAvailable  -= usAlign;
    }

    if (ulBytesAvailable < ulDataLength)
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ulBytesAvailable1 = ulDataLength;

    pFileFullEAInfoCursor = (PFILE_FULL_EA_INFORMATION)pData;
    while (pFileFullEAInfoCursor && (ulBytesAvailable1 > 0))
    {
        ULONG  ulInfoBytesRequired = 0;

        ulInfoBytesRequired += sizeof(SMB2_FILE_FULL_EA_INFORMATION_HEADER);
        ulInfoBytesRequired += pFileFullEAInfoCursor->EaNameLength + 1;
        ulInfoBytesRequired += pFileFullEAInfoCursor->EaValueLength;

		if (pFileFullEAInfoCursor->NextEntryOffset)
		{
	    if (ulInfoBytesRequired % 8)
		{
		    ulInfoBytesRequired += 8 - (ulInfoBytesRequired % 8);
		}
		}

        if (ulBytesAvailable1 < ulInfoBytesRequired)
        {
            break;
        }

        ulInfoCount++;

        ulBytesAvailable1 -= ulInfoBytesRequired;

        if (pFileFullEAInfoCursor->NextEntryOffset)
        {
            pFileFullEAInfoCursor =
                (PFILE_FULL_EA_INFORMATION)(((PBYTE)pFileFullEAInfoCursor) +
                                pFileFullEAInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileFullEAInfoCursor = NULL;
        }
    }

    pOutBuffer += ulAlignBytes;
    pFileFullEAInfoCursor = (PFILE_FULL_EA_INFORMATION)pData;

    for (; iInfoCount < ulInfoCount; iInfoCount++)
    {
        pInfoHeaderPrev = pInfoHeaderCur;
        pInfoHeaderCur = (PSMB2_FILE_FULL_EA_INFORMATION_HEADER)pOutBuffer;

        /* Update next entry offset for previous entry. */
        if (pInfoHeaderPrev != NULL)
        {
            pInfoHeaderPrev->ulNextEntryOffset = ulOffset1;
        }

        /* Reset the offset to 0 since it's relative. */
        ulOffset1 = 0;

        /* Add the header info. */
        pInfoHeaderCur->ulNextEntryOffset = 0;
        pInfoHeaderCur->ucFlags         = pFileFullEAInfoCursor->Flags;
        pInfoHeaderCur->ucEaNameLength  = pFileFullEAInfoCursor->EaNameLength;
        pInfoHeaderCur->usEaValueLength = pFileFullEAInfoCursor->EaValueLength;

        pOutBuffer  += sizeof(SMB2_FILE_FULL_EA_INFORMATION_HEADER);
        ulBytesUsed += sizeof(SMB2_FILE_FULL_EA_INFORMATION_HEADER);
        ulOffset1   += sizeof(SMB2_FILE_FULL_EA_INFORMATION_HEADER);

        if (pFileFullEAInfoCursor->EaNameLength)
        {
            memcpy( pOutBuffer,
                    &pFileFullEAInfoCursor->EaName[0],
                    pFileFullEAInfoCursor->EaNameLength);
        }

        pOutBuffer  += pFileFullEAInfoCursor->EaNameLength + 1;
        ulBytesUsed += pFileFullEAInfoCursor->EaNameLength + 1;
        ulOffset1   += pFileFullEAInfoCursor->EaNameLength + 1;

        if (pFileFullEAInfoCursor->EaValueLength)
        {
            PBYTE pEaValue = (PBYTE)&pFileFullEAInfoCursor->EaName[0] +
                                    pFileFullEAInfoCursor->EaNameLength;

            memcpy(pOutBuffer, pEaValue, pFileFullEAInfoCursor->EaValueLength);

            pOutBuffer  += pFileFullEAInfoCursor->EaValueLength;
            ulBytesUsed += pFileFullEAInfoCursor->EaValueLength;
            ulOffset1   += pFileFullEAInfoCursor->EaValueLength;
        }

        if (ulOffset1 % 8)
        {
            USHORT usAlign = 8 - (ulOffset1 % 8);

            pOutBuffer  += usAlign;
            ulOffset1   += usAlign;
            ulBytesUsed += usAlign;
        }

        pFileFullEAInfoCursor =
                    (PFILE_FULL_EA_INFORMATION)(((PBYTE)pFileFullEAInfoCursor) +
                                    pFileFullEAInfoCursor->NextEntryOffset);
    }

    *pulAlignBytes = ulAlignBytes;
    *pulBytesUsed  = ulBytesUsed;

cleanup:

    return ntStatus;

error:

    *pulAlignBytes = 0;
    *pulBytesUsed  = 0;

    if (ulBytesUsed)
    {
        memset(pOutBuffer, 0, ulBytesUsed);
    }

    goto cleanup;
}

static
NTSTATUS
SrvGetFileCompressionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_COMPRESSION_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_COMPRESSION_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb2->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        FileCompressionInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileCompressionInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_COMPRESSION_INFORMATION pFileCompressionInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB2_FILE_COMPRESSION_INFORMATION_HEADER pFileCompressionInfoHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;
    pFileCompressionInfo = (PFILE_COMPRESSION_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength =
                    sizeof(SMB2_FILE_COMPRESSION_INFORMATION_HEADER);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileCompressionInfoHeader =
                    (PSMB2_FILE_COMPRESSION_INFORMATION_HEADER)pOutBuffer;
    pFileCompressionInfoHeader->llCompressedFileSize =
                    pFileCompressionInfo->CompressedFileSize;
    pFileCompressionInfoHeader->ucChunkShift =
                    pFileCompressionInfo->ChunkShift;
    pFileCompressionInfoHeader->ucClusterShift =
                    pFileCompressionInfo->ClusterShift;
    pFileCompressionInfoHeader->ucCompressionUnitShift =
                    pFileCompressionInfo->CompressionUnitShift;
    pFileCompressionInfoHeader->usCompressionFormat =
                    pFileCompressionInfo->CompressionFormat;

    // pOutBuffer       += pGetInfoResponseHeader->ulOutBufferLength;
    // ulBytesAvailable -= pGetInfoResponseHeader->ulOutBufferLength;
    ulTotalBytesUsed    += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileSystemInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_FS_INFO_CLASS_VOLUME:

            ntStatus = SrvGetFileSystemVolumeInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FS_INFO_CLASS_ATTRIBUTE:

            ntStatus = SrvGetFileSystemAttributeInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FS_INFO_CLASS_FULL_SIZE:

            ntStatus = SrvGetFileSystemFullInfo_SMB_V2(pExecContext);

            break;

        case SMB2_FS_INFO_CLASS_SIZE:

            ntStatus = SrvGetFileSystemSizeInfo_SMB_V2(pExecContext);

            break;

        case SMB_FS_INFO_CLASS_OBJECTID:
        case SMB2_FS_INFO_CLASS_DEVICE:
        case SMB2_FS_INFO_CLASS_QUOTA:

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvGetFileSystemVolumeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pResponseBuffer)
    {
        ntStatus = SMBPacketBufferAllocate(
                        pConnection->hPacketAllocator,
                        pGetInfoState->pRequestHeader->ulOutputBufferLen,
                        &pGetInfoState->pResponseBuffer,
                        &pGetInfoState->sAllocatedSize);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulResponseBufferLen = pGetInfoState->sAllocatedSize;

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryVolumeInformationFile(
                                pCtxSmb2->pFile->hFile,
                                pGetInfoState->pAcb,
                                &pGetInfoState->ioStatusBlock,
                                pGetInfoState->pResponseBuffer,
                                pGetInfoState->ulResponseBufferLen,
                                FileFsVolumeInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState); // completed sync
    }

    pGetInfoState->ulResponseBufferLen =
                    pGetInfoState->ioStatusBlock.BytesTransferred;

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileSystemVolumeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_FS_VOLUME_INFORMATION    pFSVolInfo             = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FS_VOLUME_INFO_HEADER     pFSVolInfoHeader       = NULL;
    USHORT                         usVolumeLabelLen       = 0;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pFSVolInfo = (PFILE_FS_VOLUME_INFORMATION)pGetInfoState->pResponseBuffer;
    usVolumeLabelLen = wc16slen(pFSVolInfo->VolumeLabel) * sizeof(wchar16_t);

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FS_VOLUME_INFO_HEADER);
    pGetInfoResponseHeader->ulOutBufferLength += usVolumeLabelLen;

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFSVolInfoHeader = (PSMB_FS_VOLUME_INFO_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB_FS_VOLUME_INFO_HEADER);
    // ulBytesAvailable -= sizeof(SMB_FS_VOLUME_INFO_HEADER);
    // ulTotalBytesUsed += sizeof(SMB_FS_VOLUME_INFO_HEADER);

    pFSVolInfoHeader->bSupportsObjects     = pFSVolInfo->SupportsObjects;
    pFSVolInfoHeader->llVolumeCreationTime = pFSVolInfo->VolumeCreationTime;
    pFSVolInfoHeader->ulVolumeSerialNumber = pFSVolInfo->VolumeSerialNumber;
    pFSVolInfoHeader->ulVolumeLabelLength  = usVolumeLabelLen;

    if (usVolumeLabelLen)
    {
        memcpy(pOutBuffer, (PBYTE)pFSVolInfo->VolumeLabel, usVolumeLabelLen);
    }

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileSystemAttributeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pResponseBuffer)
    {
        ntStatus = SMBPacketBufferAllocate(
                        pConnection->hPacketAllocator,
                        pGetInfoState->pRequestHeader->ulOutputBufferLen,
                        &pGetInfoState->pResponseBuffer,
                        &pGetInfoState->sAllocatedSize);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulResponseBufferLen = pGetInfoState->sAllocatedSize;

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryVolumeInformationFile(
                                pCtxSmb2->pFile->hFile,
                                pGetInfoState->pAcb,
                                &pGetInfoState->ioStatusBlock,
                                pGetInfoState->pResponseBuffer,
                                pGetInfoState->ulResponseBufferLen,
                                FileFsAttributeInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState); // completed sync
    }

    pGetInfoState->ulResponseBufferLen =
                    pGetInfoState->ioStatusBlock.BytesTransferred;

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileSystemAttributeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_FS_ATTRIBUTE_INFORMATION pFSAttrInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FS_ATTRIBUTE_INFO_HEADER  pFSAttrInfoHeader = NULL;
    USHORT                         usVolumeLabelLen = 0;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pOutBuffer += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pFSAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)pGetInfoState->pResponseBuffer;
    usVolumeLabelLen = wc16slen(pFSAttrInfo->FileSystemName) * sizeof(wchar16_t);

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
    pGetInfoResponseHeader->ulOutBufferLength += usVolumeLabelLen;

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFSAttrInfoHeader = (PSMB_FS_ATTRIBUTE_INFO_HEADER)pOutBuffer;

    // ulTotalBytesUsed += sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
    // ulBytesAvailable -= sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
    pOutBuffer += sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);

    pFSAttrInfoHeader->ulFSAttributes = pFSAttrInfo->FileSystemAttributes;
    pFSAttrInfoHeader->lMaxFilenameLen = pFSAttrInfo->MaximumComponentNameLength;
    pFSAttrInfoHeader->ulFileSystemNameLen = usVolumeLabelLen;

    if (usVolumeLabelLen)
    {
        memcpy(pOutBuffer, (PBYTE)pFSAttrInfo->FileSystemName, usVolumeLabelLen);
    }

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileSystemFullInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_FS_SIZE_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_FS_SIZE_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryVolumeInformationFile(
                                pCtxSmb2->pFile->hFile,
                                pGetInfoState->pAcb,
                                &pGetInfoState->ioStatusBlock,
                                pGetInfoState->pData2,
                                pGetInfoState->ulDataLength,
                                FileFsSizeInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState); // completed sync
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileSystemFullInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_FS_SIZE_INFORMATION      pFSSizeInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FS_FULL_INFO_HEADER       pFSFullInfoHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    pFSSizeInfo = (PFILE_FS_SIZE_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FS_FULL_INFO_HEADER);

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFSFullInfoHeader = (PSMB_FS_FULL_INFO_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB_FS_FULL_INFO_HEADER);
    // ulBytesAvailable -= sizeof(SMB_FS_FULL_INFO_HEADER);

    // TODO: Fill in the AllocationSize
    pFSFullInfoHeader->ullTotalAllocationUnits =
                                        pFSSizeInfo->TotalAllocationUnits;
    pFSFullInfoHeader->ullCallerAvailableAllocationUnits =
                                        pFSSizeInfo->AvailableAllocationUnits;
    pFSFullInfoHeader->ullAvailableAllocationUnits =
                                        pFSSizeInfo->AvailableAllocationUnits;
    pFSFullInfoHeader->ulSectorsPerAllocationUnit =
                                        pFSSizeInfo->SectorsPerAllocationUnit;
    pFSFullInfoHeader->ulBytesPerSector = pFSSizeInfo->BytesPerSector;

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetFileSystemSizeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = pGetInfoState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pGetInfoState->pData2)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(FILE_FS_SIZE_INFORMATION),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength = sizeof(FILE_FS_SIZE_INFORMATION);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryVolumeInformationFile(
                                pCtxSmb2->pFile->hFile,
                                pGetInfoState->pAcb,
                                &pGetInfoState->ioStatusBlock,
                                pGetInfoState->pData2,
                                pGetInfoState->ulDataLength,
                                FileFsSizeInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState); // completed sync
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildFileSystemSizeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_FS_SIZE_INFORMATION      pFSSizeInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    pFSSizeInfo = (PFILE_FS_SIZE_INFORMATION)pGetInfoState->pData2;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    pGetInfoResponseHeader->ulOutBufferLength = pGetInfoState->ulDataLength;

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pGetInfoResponseHeader->ulOutBufferLength)
    {
        memcpy(pOutBuffer, pGetInfoState->pData2, pGetInfoState->ulDataLength);
    }

    // pOutBuffer       += pGetInfoState->ulDataLength;
    // ulBytesAvailable -= pGetInfoState->ulDataLength;
    // ulOffset         += pGetInfoState->ulDataLength;

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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
SrvGetSecurityInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    switch (pGetInfoState->pRequestHeader->ucInfoClass)
    {
        case SMB2_SEC_INFO_CLASS_BASIC:

            if (pGetInfoState->pRequestHeader->ulOutputBufferLen >
                        SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE)
            {
                ntStatus = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = SrvExecuteQuerySecurityDescriptor_SMB_V2(pExecContext);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvExecuteQuerySecurityDescriptor_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    BOOLEAN                    bContinue     = TRUE;
    PBYTE                      pErrorMessage = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    do
    {
        ntStatus = pGetInfoState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                if (pGetInfoState->ulDataLength >=
                         pGetInfoState->pRequestHeader->ulOutputBufferLen)
                {
                    bContinue = FALSE;
                }
                else if (!pGetInfoState->pData2)
                {
                    ULONG ulSecurityDescInitialLen = 256;

                    ntStatus = SrvAllocateMemory(
                                    ulSecurityDescInitialLen,
                                    (PVOID*)&pGetInfoState->pData2);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pGetInfoState->ulDataLength = ulSecurityDescInitialLen;
                }
                else if (pGetInfoState->ulDataLength !=
                                    SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE)
                {
                    PBYTE pNewMemory = NULL;
                    ULONG ulNewLen =
                        LW_MIN( SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE,
                                        pGetInfoState->ulDataLength + 4096);

                    ntStatus = SrvAllocateMemory(ulNewLen, (PVOID*)&pNewMemory);
                    BAIL_ON_NT_STATUS(ntStatus);

                    if (pGetInfoState->pData2)
                    {
                        SrvFreeMemory(pGetInfoState->pData2);
                    }

                    pGetInfoState->pData2 = pNewMemory;
                    pGetInfoState->ulDataLength = ulNewLen;
                }
                else
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
                BAIL_ON_NT_STATUS(ntStatus);

                SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

                ntStatus = IoQuerySecurityFile(
                                pGetInfoState->pFile->hFile,
                                pGetInfoState->pAcb,
                                &pGetInfoState->ioStatusBlock,
                                pGetInfoState->pRequestHeader->ulAdditionalInfo,
                                (PSECURITY_DESCRIPTOR_RELATIVE)pGetInfoState->pData2,
                                pGetInfoState->ulDataLength);
                switch (ntStatus)
                {
                    case STATUS_SUCCESS:
                    case STATUS_BUFFER_TOO_SMALL:

                        // completed synchronously
                        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState);

                        break;

                    default:

                        BAIL_ON_NT_STATUS(ntStatus);
                }

                break;

            case STATUS_SUCCESS:

                if (!pGetInfoState->pData2)
                {
                    pGetInfoState->ioStatusBlock.Status =
                                                    STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    pGetInfoState->ulActualDataLength =
                                pGetInfoState->ioStatusBlock.BytesTransferred;

                    bContinue = FALSE;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

cleanup:

    if (pErrorMessage)
    {
        SrvFreeMemory(pErrorMessage);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_BUFFER_TOO_SMALL:

            {
                NTSTATUS ntStatus2 = STATUS_SUCCESS;
                ULONG    ulLength  = 0xE0;

                ntStatus2 = SrvAllocateMemory(
                                sizeof(ULONG),
                                (PVOID*)&pErrorMessage);
                if (ntStatus2)
                {
                    LWIO_LOG_ERROR(
                        "Failed to allocate buffer for error message "
                        "[error:0x%08x]",
                        ntStatus2);
                }
                else
                {
                    memcpy(pErrorMessage, (PBYTE)&ulLength, sizeof(ulLength));

                    ntStatus2 = SrvSetErrorMessage_SMB_V2(
                                    pCtxSmb2,
                                    pErrorMessage,
                                    sizeof(ulLength));
                    if (ntStatus2 == STATUS_SUCCESS)
                    {
                        pErrorMessage = NULL;
                    }
                    else
                    {
                        LWIO_LOG_ERROR(
                        "Failed to set error message in exec context "
                        "[error:0x%08x]",
                        ntStatus2);
                    }
                }
            }

            break;

        default:

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildSecurityInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState = NULL;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;

    pGetInfoState = (PSRV_GET_INFO_STATE_SMB_V2)pCtxSmb2->hState;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_GET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pGetInfoResponseHeader = (PSMB2_GET_INFO_RESPONSE_HEADER)pOutBuffer;

    ulBytesAvailable -= sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    pOutBuffer       += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_GET_INFO_RESPONSE_HEADER);

    pGetInfoResponseHeader->usLength = sizeof(SMB2_GET_INFO_RESPONSE_HEADER)+1;
    pGetInfoResponseHeader->usOutBufferOffset = ulOffset;

    if (ulBytesAvailable < pGetInfoState->ulActualDataLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pGetInfoState->ulActualDataLength)
    {
        memcpy(pOutBuffer,
               pGetInfoState->pData2,
               pGetInfoState->ulActualDataLength);
    }

    pGetInfoResponseHeader->ulOutBufferLength =
                                        pGetInfoState->ulActualDataLength;

    // pOutBuffer       += pGetInfoState->ulDataLength;
    // ulBytesAvailable -= pGetInfoState->ulDataLength;
    // ulOffset         += pGetInfoState->ulDataLength;

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

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

static
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

    if (pGetInfoState->pResponseBuffer)
    {
        SMBPacketBufferFree(
                    pGetInfoState->pConnection->hPacketAllocator,
                    pGetInfoState->pResponseBuffer,
                    pGetInfoState->sAllocatedSize);
    }

    if (pGetInfoState->pConnection)
    {
        SrvConnectionRelease(pGetInfoState->pConnection);
    }

    if (pGetInfoState->pMutex)
    {
        pthread_mutex_destroy(&pGetInfoState->mutex);
    }

    SrvFreeMemory(pGetInfoState);
}

