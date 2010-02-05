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
 *        getfsinfo.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Query File System Information
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

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
                                pGetInfoState->pFile->hFile,
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
                    0LL,
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

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FS_VOLUME_INFO_HEADER);
    // pGetInfoResponseHeader->ulOutBufferLength += pFSVolInfo->VolumeLabelLength;

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
    pFSVolInfoHeader->ulVolumeLabelLength = 0;

    // pFSVolInfoHeader->ulVolumeLabelLength  = pFSVolInfo->VolumeLabelLength;
    // if (pFSVolInfo->VolumeLabelLength)
    // {
    //     memcpy( pOutBuffer,
    //             (PBYTE)pFSVolInfo->VolumeLabel,
    //             pFSVolInfo->VolumeLabelLength);
    // }

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
                                pGetInfoState->pFile->hFile,
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
                    0LL,
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

    pGetInfoResponseHeader->ulOutBufferLength = sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);
    pGetInfoResponseHeader->ulOutBufferLength += pFSAttrInfo->FileSystemNameLength;

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
    pFSAttrInfoHeader->ulFileSystemNameLen = pFSAttrInfo->FileSystemNameLength;

    if (pFSAttrInfo->FileSystemNameLength)
    {
        memcpy( pOutBuffer,
                (PBYTE)pFSAttrInfo->FileSystemName,
                pFSAttrInfo->FileSystemNameLength);
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
                                pGetInfoState->pFile->hFile,
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
                    0LL,
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
                                pGetInfoState->pFile->hFile,
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
                    0LL,
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
