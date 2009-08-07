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
SrvGetFileInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileStandardInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileInternalInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileEAInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileAccessInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFilePositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileFullEAInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileModeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileAlignmentInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileAllInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileAlternateNameInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileStreamInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileCompressionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileNetworkOpenInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvGetFileAttributeTagInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetFileSystemInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetFileSystemVolumeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetFileSystemSizeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetFileSystemDeviceInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetFileSystemAttributeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetFileSystemQuotaInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetFileSystemFullInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetFileSystemObjectIdInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetSecurityInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

static
NTSTATUS
SrvGetFileSecurityBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    );

NTSTATUS
SrvProcessGetInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree = NULL;
    PLWIO_SRV_FILE_2    pFile = NULL;

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

    switch (pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = SrvGetFileInfo_SMB_V2(pExecContext, pRequestHeader);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = SrvGetFileSystemInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = SrvGetSecurityInfo_SMB_V2(pExecContext, pRequestHeader);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

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

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvGetFileInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_BASIC :

            ntStatus = SrvGetFileBasicInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_STANDARD :

            ntStatus = SrvGetFileStandardInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_INTERNAL :

            ntStatus = SrvGetFileInternalInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_EA :

            ntStatus = SrvGetFileEAInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_ACCESS :

            ntStatus = SrvGetFileAccessInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_POSITION :

            ntStatus = SrvGetFilePositionInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_FULL_EA :

            ntStatus = SrvGetFileFullEAInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_MODE :

            ntStatus = SrvGetFileModeInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_ALIGNMENT :

            ntStatus = SrvGetFileAlignmentInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_ALL :

            ntStatus = SrvGetFileAllInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_ALTERNATE_NAME :

            ntStatus = SrvGetFileAlternateNameInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_STREAM :

            ntStatus = SrvGetFileStreamInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_COMPRESSION :

            ntStatus = SrvGetFileCompressionInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_NETWORK_OPEN :

            ntStatus = SrvGetFileNetworkOpenInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FILE_INFO_CLASS_ATTRIBUTE_TAG :

            ntStatus = SrvGetFileAttributeTagInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

NTSTATUS
SrvGetFileBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileStandardInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileInternalInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    FILE_INTERNAL_INFORMATION   fileInternalInfo = {0};
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FILE_INTERNAL_INFO_HEADER pFileInternalInfoHeader = NULL;

    ntStatus = IoQueryInformationFile(
                    pCtxSmb2->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileInternalInfo,
                    sizeof(fileInternalInfo),
                    FileInternalInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    0,
                    1,
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

    ulTotalBytesUsed += sizeof(SMB_FILE_INTERNAL_INFO_HEADER);
    ulBytesAvailable -= sizeof(SMB_FILE_INTERNAL_INFO_HEADER);
    pOutBuffer += sizeof(SMB_FILE_INTERNAL_INFO_HEADER);

    pFileInternalInfoHeader->ullIndex = fileInternalInfo.IndexNumber;

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

NTSTATUS
SrvGetFileEAInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    FILE_EA_INFORMATION         fileEAInfo = {0};
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FILE_EA_INFO_HEADER       pFileEAInfoHeader = NULL;

    ntStatus = IoQueryInformationFile(
                    pCtxSmb2->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileEAInfo,
                    sizeof(fileEAInfo),
                    FileEaInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    0,
                    1,
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

    ulTotalBytesUsed += sizeof(SMB_FILE_EA_INFO_HEADER);
    ulBytesAvailable -= sizeof(SMB_FILE_EA_INFO_HEADER);
    pOutBuffer += sizeof(SMB_FILE_EA_INFO_HEADER);

    pFileEAInfoHeader->ulEaSize = fileEAInfo.EaSize;

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

NTSTATUS
SrvGetFileAccessInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFilePositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileFullEAInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileModeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileAlignmentInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileAllInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileAlternateNameInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileStreamInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileCompressionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvGetFileNetworkOpenInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
SrvGetFileAttributeTagInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvGetFileSystemInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (pRequestHeader->ucInfoClass)
    {
        case SMB2_FS_INFO_CLASS_VOLUME:

            ntStatus = SrvGetFileSystemVolumeInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FS_INFO_CLASS_SIZE:

            ntStatus = SrvGetFileSystemSizeInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FS_INFO_CLASS_DEVICE:

            ntStatus = SrvGetFileSystemDeviceInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FS_INFO_CLASS_ATTRIBUTE:

            ntStatus = SrvGetFileSystemAttributeInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FS_INFO_CLASS_QUOTA:

            ntStatus = SrvGetFileSystemQuotaInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB2_FS_INFO_CLASS_FULL_SIZE:

            ntStatus = SrvGetFileSystemFullInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        case SMB_FS_INFO_CLASS_OBJECTID:

            ntStatus = SrvGetFileSystemObjectIdInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

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
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PBYTE                       pResponseBuffer = NULL;
    size_t                      sAllocatedSize = 0;
    ULONG                       ulResponseBufferLen = 0;
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    PFILE_FS_VOLUME_INFORMATION pFSVolInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FS_VOLUME_INFO_HEADER  pFSVolInfoHeader = NULL;
    USHORT                      usVolumeLabelLen = 0;

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    pRequestHeader->ulOutputBufferLen,
                    &pResponseBuffer,
                    &sAllocatedSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ulResponseBufferLen = sAllocatedSize;

    ntStatus = IoQueryVolumeInformationFile(
                            pCtxSmb2->pFile->hFile,
                            NULL,
                            &ioStatusBlock,
                            pResponseBuffer,
                            ulResponseBufferLen,
                            FileFsVolumeInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ulResponseBufferLen = ioStatusBlock.BytesTransferred;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    0,
                    1,
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

    pFSVolInfo = (PFILE_FS_VOLUME_INFORMATION)pResponseBuffer;
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
    ulBytesAvailable -= sizeof(SMB_FS_VOLUME_INFO_HEADER);
    ulTotalBytesUsed += sizeof(SMB_FS_VOLUME_INFO_HEADER);

    pFSVolInfoHeader->bSupportsObjects = pFSVolInfo->SupportsObjects;
    pFSVolInfoHeader->llVolumeCreationTime = pFSVolInfo->VolumeCreationTime;
    pFSVolInfoHeader->ulVolumeSerialNumber = pFSVolInfo->VolumeSerialNumber;
    pFSVolInfoHeader->ulVolumeLabelLength = usVolumeLabelLen;

    if (usVolumeLabelLen)
    {
        memcpy(pOutBuffer, (PBYTE)pFSVolInfo->VolumeLabel, usVolumeLabelLen);
    }

    ulTotalBytesUsed += pGetInfoResponseHeader->ulOutBufferLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pResponseBuffer)
    {
        SMBPacketBufferFree(
                pConnection->hPacketAllocator,
                pResponseBuffer,
                sAllocatedSize);
    }

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
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvGetFileSystemDeviceInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvGetFileSystemAttributeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PBYTE                       pResponseBuffer = NULL;
    size_t                      sAllocatedSize = 0;
    ULONG                       ulResponseBufferLen = 0;
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    PFILE_FS_ATTRIBUTE_INFORMATION pFSAttrInfo = NULL;
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FS_ATTRIBUTE_INFO_HEADER  pFSAttrInfoHeader = NULL;
    USHORT                         usVolumeLabelLen = 0;

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    pRequestHeader->ulOutputBufferLen,
                    &pResponseBuffer,
                    &sAllocatedSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ulResponseBufferLen = sAllocatedSize;

    ntStatus = IoQueryVolumeInformationFile(
                            pCtxSmb2->pFile->hFile,
                            NULL,
                            &ioStatusBlock,
                            pResponseBuffer,
                            ulResponseBufferLen,
                            FileFsAttributeInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ulResponseBufferLen = ioStatusBlock.BytesTransferred;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    0,
                    1,
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

    pFSAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)pResponseBuffer;
    usVolumeLabelLen = wc16slen(pFSAttrInfo->FileSystemName) * sizeof(wchar16_t);

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
    pGetInfoResponseHeader->ulOutBufferLength += usVolumeLabelLen;

    if (ulBytesAvailable < pGetInfoResponseHeader->ulOutBufferLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFSAttrInfoHeader = (PSMB_FS_ATTRIBUTE_INFO_HEADER)pOutBuffer;

    ulTotalBytesUsed += sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
    ulBytesAvailable -= sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
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

    if (pResponseBuffer)
    {
        SMBPacketBufferFree(
                pConnection->hPacketAllocator,
                pResponseBuffer,
                sAllocatedSize);
    }

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
SrvGetFileSystemQuotaInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvGetFileSystemFullInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    FILE_FS_SIZE_INFORMATION    fSSizeInfo = {0};
    PSMB2_GET_INFO_RESPONSE_HEADER pGetInfoResponseHeader = NULL;
    PSMB_FS_FULL_INFO_HEADER       pFSFullInfoHeader = NULL;

    ntStatus = IoQueryVolumeInformationFile(
                        pCtxSmb2->pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        &fSSizeInfo,
                        sizeof(fSSizeInfo),
                        FileFsSizeInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_GETINFO,
                    0,
                    1,
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
    ulBytesAvailable -= sizeof(SMB_FS_FULL_INFO_HEADER);

    // TODO: Fill in the AllocationSize
    pFSFullInfoHeader->ullTotalAllocationUnits = fSSizeInfo.TotalAllocationUnits;
    pFSFullInfoHeader->ullCallerAvailableAllocationUnits = fSSizeInfo.AvailableAllocationUnits;
    pFSFullInfoHeader->ullAvailableAllocationUnits = fSSizeInfo.AvailableAllocationUnits;
    pFSFullInfoHeader->ulSectorsPerAllocationUnit = fSSizeInfo.SectorsPerAllocationUnit;
    pFSFullInfoHeader->ulBytesPerSector = fSSizeInfo.BytesPerSector;

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
SrvGetFileSystemObjectIdInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvGetSecurityInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (pRequestHeader->ucInfoClass)
    {
        case SMB2_SEC_INFO_CLASS_BASIC:

            ntStatus = SrvGetFileSecurityBasicInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

static
NTSTATUS
SrvGetFileSecurityBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_GET_INFO_REQUEST_HEADER pRequestHeader
    )
{
    return STATUS_NOT_SUPPORTED;
}
