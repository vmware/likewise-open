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
SrvGetFileSystemControlInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileSystemSizeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileSystemInfoResponseGeneric_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildFileSystemControlInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetFileSystemInfoGeneric_SMB_V2(
    PSRV_EXEC_CONTEXT      pExecContext,
    ULONG                  ulStructSize,
    FILE_INFORMATION_CLASS infoClass
    );

static
NTSTATUS
SrvGetFileSystemInfoGeneric_WithString_SMB_V2(
    PSRV_EXEC_CONTEXT      pExecContext,
    ULONG                  ulStructSize,
    FILE_INFORMATION_CLASS infoClass
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

        case SMB2_FS_INFO_CLASS_CONTROL:

            ntStatus = SrvGetFileSystemControlInfo_SMB_V2(pExecContext);

            break;

        case SMB_FS_INFO_CLASS_OBJECTID:
        case SMB2_FS_INFO_CLASS_DEVICE:

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

        case SMB2_FS_INFO_CLASS_CONTROL:

            ntStatus = SrvBuildFileSystemControlInfoResponse_SMB_V2(pExecContext);

            break;

        case SMB_FS_INFO_CLASS_OBJECTID:
        case SMB2_FS_INFO_CLASS_DEVICE:

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
    return SrvGetFileSystemInfoGeneric_WithString_SMB_V2(
                pExecContext,
                sizeof(FILE_FS_VOLUME_INFORMATION),
                FileFsVolumeInformation);
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
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pExecContext->usCreditsGranted,
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

    pFSVolInfo = (PFILE_FS_VOLUME_INFORMATION)pGetInfoState->pData2;

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
    return SrvGetFileSystemInfoGeneric_WithString_SMB_V2(
                pExecContext,
                sizeof(FILE_FS_ATTRIBUTE_INFORMATION),
                FileFsAttributeInformation);
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
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pExecContext->usCreditsGranted,
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

    pFSAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)pGetInfoState->pData2;

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
    return SrvGetFileSystemInfoGeneric_SMB_V2(
                pExecContext,
                sizeof(FILE_FS_SIZE_INFORMATION),
                FileFsSizeInformation);
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
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pExecContext->usCreditsGranted,
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
    return SrvGetFileSystemInfoGeneric_SMB_V2(
            pExecContext,
            sizeof(FILE_FS_SIZE_INFORMATION),
            FileFsSizeInformation);
}

static
NTSTATUS
SrvBuildFileSystemSizeInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    return SrvBuildFileSystemInfoResponseGeneric_SMB_V2(pExecContext);
}

static
NTSTATUS
SrvBuildFileSystemInfoResponseGeneric_SMB_V2(
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
                    COM2_GETINFO,
                    pSmbRequest->pHeader->usEpoch,
                    pExecContext->usCreditsGranted,
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

static
NTSTATUS
SrvGetFileSystemControlInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    return SrvGetFileSystemInfoGeneric_SMB_V2(
            pExecContext,
            sizeof(FILE_FS_CONTROL_INFORMATION),
            FileFsControlInformation);
}

static
NTSTATUS
SrvBuildFileSystemControlInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    return SrvBuildFileSystemInfoResponseGeneric_SMB_V2(pExecContext);
}

static
NTSTATUS
SrvGetFileSystemInfoGeneric_SMB_V2(
    PSRV_EXEC_CONTEXT      pExecContext,
    ULONG                  ulStructSize,
    FILE_INFORMATION_CLASS infoClass
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
        if (pGetInfoState->pRequestHeader->ulOutputBufferLen < ulStructSize)
        {
            ntStatus = STATUS_INFO_LENGTH_MISMATCH;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SrvAllocateMemory(
                        SMB_MIN(pGetInfoState->pRequestHeader->ulOutputBufferLen,
                                ulStructSize),
                        (PVOID*)&pGetInfoState->pData2);
        BAIL_ON_NT_STATUS(ntStatus);

        pGetInfoState->ulDataLength =
                SMB_MIN(pGetInfoState->pRequestHeader->ulOutputBufferLen,
                        ulStructSize);

        SrvPrepareGetInfoStateAsync_SMB_V2(pGetInfoState, pExecContext);

        ntStatus = IoQueryVolumeInformationFile(
                        pGetInfoState->pFile->hFile,
                        pGetInfoState->pAcb,
                        &pGetInfoState->ioStatusBlock,
                        pGetInfoState->pData2,
                        pGetInfoState->ulDataLength,
                        infoClass);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseGetInfoStateAsync_SMB_V2(pGetInfoState); // sync completion
    }

    pGetInfoState->ulActualDataLength =
                            pGetInfoState->ioStatusBlock.BytesTransferred;

error:

    return ntStatus;
}

static
NTSTATUS
SrvGetFileSystemInfoGeneric_WithString_SMB_V2(
    PSRV_EXEC_CONTEXT      pExecContext,
    ULONG                  ulStructSize,
    FILE_INFORMATION_CLASS infoClass
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
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

                {
                    if (pGetInfoState->ulDataLength >=
                            pGetInfoState->pRequestHeader->ulOutputBufferLen)
                    {
                        bContinue = FALSE;
                    }
                    else
                    {
                        ULONG ulNewSize =  0;

                        if (!pGetInfoState->ulDataLength)
                        {
                            if (pGetInfoState->pRequestHeader->ulOutputBufferLen < ulStructSize)
                            {
                                ntStatus = STATUS_INFO_LENGTH_MISMATCH;
                                BAIL_ON_NT_STATUS(ntStatus);
                            }

                            ulNewSize = ulStructSize + 256 * sizeof(wchar16_t);
                        }
                        else
                        {
                            ulNewSize = pGetInfoState->ulDataLength +
                                            256 * sizeof(wchar16_t);
                        }

                        ulNewSize = SMB_MIN(ulNewSize,
                                            pGetInfoState->pRequestHeader->ulOutputBufferLen);

                        ntStatus = SMBReallocMemory(
                                        pGetInfoState->pData2,
                                        (PVOID*)&pGetInfoState->pData2,
                                        ulNewSize);
                        BAIL_ON_NT_STATUS(ntStatus);

                        pGetInfoState->ulDataLength = ulNewSize;
                    }

                    SrvPrepareGetInfoStateAsync_SMB_V2(
                                    pGetInfoState,
                                    pExecContext);

                    ntStatus = IoQueryVolumeInformationFile(
                                            pGetInfoState->pFile->hFile,
                                            pGetInfoState->pAcb,
                                            &pGetInfoState->ioStatusBlock,
                                            pGetInfoState->pData2,
                                            pGetInfoState->ulDataLength,
                                            infoClass);
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

    BAIL_ON_NT_STATUS(ntStatus);

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
                ULONG    ulLength  = 0;

                if (!pGetInfoState->ulDataLength)
                {
                    ulLength = ulStructSize + sizeof(wchar16_t) * 256;
                }
                else if (pGetInfoState->ulDataLength ==
                            pGetInfoState->pRequestHeader->ulOutputBufferLen)
                {
                    ulLength = pGetInfoState->ulDataLength +
                                        sizeof(wchar16_t) * 256;
                }
                else
                {
                    ulLength = pGetInfoState->ulDataLength;
                }

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
                if (infoClass == FileFsVolumeInformation)
                {
                    ntStatus = STATUS_BUFFER_OVERFLOW;
                }
            }

            break;

        default:

            break;
    }

    goto cleanup;
}

