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

#include "includes.h"

static
NTSTATUS
SrvBuildFSAllocationInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    );

static
NTSTATUS
SrvBuildFSInfoVolumeResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    );

static
NTSTATUS
SrvBuildFSVolumeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    );

static
NTSTATUS
SrvMarshallFSInfoVolume(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usDataOffset,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

static
NTSTATUS
SrvMarshallFSVolumeInfo(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

static
NTSTATUS
SrvBuildFSSizeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    );

static
NTSTATUS
SrvBuildFSAttributeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    );

static
NTSTATUS
SrvMarshallFSAttributeInfo(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

NTSTATUS
SrvProcessTrans2QueryFilesystemInformation(
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PBYTE                       pParameters,
    PBYTE                       pData
    )
{
    NTSTATUS          ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    USHORT            usInfoLevel = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    IO_FILE_HANDLE    hFile = NULL;
    IO_FILE_NAME      fileName = {0};
    PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock = NULL;
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    IO_STATUS_BLOCK     ioStatusBlock = {0};
    BOOLEAN             bInLock = FALSE;

    if ((pRequestHeader->parameterCount != 2) &&
        (pRequestHeader->parameterCount != 4))
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

    ntStatus = SrvSessionFindTree_SMB_V1(
                    pCtxSmb1,
                    pSession,
                    pSmbRequest->pHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    fileName.FileName = pTree->pShareInfo->pwszPath;

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSession->pIoSecurityContext,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    GENERIC_READ,
                    0,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_READ,
                    FILE_OPEN,
                    0,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    NULL  /* ECP List  */
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    usInfoLevel = *((PUSHORT)pParameters);

    switch (usInfoLevel)
    {
        case SMB_INFO_ALLOCATION:

            ntStatus = SrvBuildFSAllocationInfoResponse(
                            pExecContext,
                            hFile,
                            pRequestHeader->maxDataCount);

            break;

        case SMB_INFO_VOLUME:

            ntStatus = SrvBuildFSInfoVolumeResponse(
                            pExecContext,
                            hFile,
                            pRequestHeader->maxDataCount);

            break;

        case SMB_QUERY_FS_VOLUME_INFO:

            ntStatus = SrvBuildFSVolumeInfoResponse(
                            pExecContext,
                            hFile,
                            pRequestHeader->maxDataCount);

            break;

        case SMB_QUERY_FS_SIZE_INFO:

            ntStatus = SrvBuildFSSizeInfoResponse(
                            pExecContext,
                            hFile,
                            pRequestHeader->maxDataCount);

            break;

        case SMB_QUERY_FS_ATTRIBUTE_INFO:

            ntStatus = SrvBuildFSAttributeInfoResponse(
                            pExecContext,
                            hFile,
                            pRequestHeader->maxDataCount);

            break;

        case SMB_QUERY_FS_DEVICE_INFO:
        case SMB_QUERY_CIFS_UNIX_INFO:
        case SMB_QUERY_MAC_FS_INFO:

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    if (pTree)
    {
        LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildFSAllocationInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE    pOutBuffer            = pSmbResponse->pBuffer;
    ULONG    ulBytesAvailable      = pSmbResponse->ulBytesAvailable;
    ULONG    ulOffset              = 0;
    USHORT   usBytesUsed           = 0;
    ULONG    ulTotalBytesUsed      = 0;
    PUSHORT  pSetup                = NULL;
    BYTE     setupCount            = 0;
    USHORT   usDataOffset          = 0;
    USHORT   usParameterOffset     = 0;
    IO_STATUS_BLOCK          ioStatusBlock = {0};
    FILE_FS_SIZE_INFORMATION fSSizeInfo = {0};
    SMB_FS_INFO_ALLOCATION   fsAllocInfo = {0};

    ntStatus = IoQueryVolumeInformationFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        &fSSizeInfo,
                        sizeof(fSSizeInfo),
                        FileFsSizeInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: Resolve large integer values
    fsAllocInfo.ulFileSystemId = 0;
    fsAllocInfo.ulNumAllocationUnits =
                SMB_MIN(UINT32_MAX, fSSizeInfo.TotalAllocationUnits);
    fsAllocInfo.ulNumSectorsPerAllocationUnit =
                fSSizeInfo.SectorsPerAllocationUnit;
    fsAllocInfo.ulNumUnitsAvailable =
                SMB_MIN(UINT32_MAX, fSSizeInfo.AvailableAllocationUnits);
    fsAllocInfo.usNumBytesPerSector =
                SMB_MIN(UINT16_MAX, fSSizeInfo.BytesPerSector);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
                    pSmbRequest->pHeader->pid,
                    pCtxSmb1->pSession->uid,
                    pSmbRequest->pHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    (PBYTE)&fsAllocInfo,
                    sizeof(fsAllocInfo),
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

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
NTSTATUS
SrvBuildFSInfoVolumeResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE    pOutBuffer            = pSmbResponse->pBuffer;
    ULONG    ulBytesAvailable      = pSmbResponse->ulBytesAvailable;
    ULONG    ulOffset              = 0;
    USHORT   usBytesUsed           = 0;
    ULONG    ulTotalBytesUsed      = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PBYTE    pVolumeInfo = NULL;
    USHORT   usBytesAllocated = 0;

    usBytesAllocated = sizeof(FILE_FS_VOLUME_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = SrvAllocateMemory(usBytesAllocated, (PVOID*)&pVolumeInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ntStatus = IoQueryVolumeInformationFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        pVolumeInfo,
                        usBytesAllocated,
                        FileFsVolumeInformation);
        if (ntStatus == STATUS_SUCCESS)
        {
            break;
        }
        else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewSize = usBytesAllocated + 256 * sizeof(wchar16_t);

            ntStatus = SMBReallocMemory(
                            pVolumeInfo,
                            (PVOID*)&pVolumeInfo,
                            usNewSize);
            BAIL_ON_NT_STATUS(ntStatus);

            usBytesAllocated = usNewSize;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (TRUE);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
                    pSmbRequest->pHeader->pid,
                    pCtxSmb1->pSession->uid,
                    pSmbRequest->pHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    pData,
                    usDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallFSInfoVolume(
                    pVolumeInfo,
                    usBytesAllocated,
                    usDataOffset,
                    usMaxDataCount,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    pData,
                    usDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pVolumeInfo)
    {
        SrvFreeMemory(pVolumeInfo);
    }
    if (pData)
    {
        SrvFreeMemory(pData);
    }

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
NTSTATUS
SrvBuildFSVolumeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE    pOutBuffer            = pSmbResponse->pBuffer;
    ULONG    ulBytesAvailable      = pSmbResponse->ulBytesAvailable;
    ULONG    ulOffset              = 0;
    USHORT   usBytesUsed           = 0;
    ULONG    ulTotalBytesUsed      = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PBYTE    pVolumeInfo = NULL;
    USHORT   usBytesAllocated = 0;

    usBytesAllocated = sizeof(FILE_FS_VOLUME_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = SrvAllocateMemory(usBytesAllocated, (PVOID*)&pVolumeInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ntStatus = IoQueryVolumeInformationFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        pVolumeInfo,
                        usBytesAllocated,
                        FileFsVolumeInformation);
        if (ntStatus == STATUS_SUCCESS)
        {
            break;
        }
        else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewSize = usBytesAllocated + 256 * sizeof(wchar16_t);

            ntStatus = SMBReallocMemory(
                            pVolumeInfo,
                            (PVOID*)&pVolumeInfo,
                            usNewSize);
            BAIL_ON_NT_STATUS(ntStatus);

            usBytesAllocated = usNewSize;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (TRUE);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
                    pSmbRequest->pHeader->pid,
                    pCtxSmb1->pSession->uid,
                    pSmbRequest->pHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 10 + setupCount;

    ntStatus = SrvMarshallFSVolumeInfo(
                    pVolumeInfo,
                    usBytesAllocated,
                    usMaxDataCount,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    pData,
                    usDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pVolumeInfo)
    {
        SrvFreeMemory(pVolumeInfo);
    }
    if (pData)
    {
        SrvFreeMemory(pData);
    }

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
NTSTATUS
SrvMarshallFSInfoVolume(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usDataOffset,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usBytesRequired = 0;
    PFILE_FS_VOLUME_INFORMATION pFSVolInfo = NULL;
    PSMB_FS_INFO_VOLUME_HEADER pFSInfoVolHeader = NULL;
    USHORT   usVolumeLabelByteLen = 0;
    USHORT   usAlignment = 0;

    pFSVolInfo = (PFILE_FS_VOLUME_INFORMATION)pVolumeInfo;

    usBytesRequired = sizeof(SMB_FS_INFO_VOLUME_HEADER);
    usDataOffset += sizeof(SMB_FS_INFO_VOLUME_HEADER);

    if (pFSVolInfo->VolumeLabel && *pFSVolInfo->VolumeLabel)
    {
        usAlignment = usDataOffset % 2;

        usBytesRequired += usAlignment;
        // usDataOffset += usAlignment;

        usVolumeLabelByteLen = SMB_MIN(UINT8_MAX, wc16slen(pFSVolInfo->VolumeLabel) * sizeof(wchar16_t));

        usBytesRequired += usVolumeLabelByteLen;

        if (usBytesRequired > usMaxDataCount)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFSInfoVolHeader = (PSMB_FS_INFO_VOLUME_HEADER)pDataCursor;
    pFSInfoVolHeader->ulVolumeSerialNumber = pFSVolInfo->VolumeSerialNumber;
    pFSInfoVolHeader->ucNumLabelChars = usVolumeLabelByteLen;

    pDataCursor += sizeof(SMB_FS_INFO_VOLUME_HEADER);

    if (usVolumeLabelByteLen)
    {
        pDataCursor += usAlignment;

        memcpy(pDataCursor, (PBYTE)pFSVolInfo->VolumeLabel, usVolumeLabelByteLen);
    }

    *ppData = pData;
    *pusDataLen = usBytesRequired;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvMarshallFSVolumeInfo(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usBytesRequired = 0;
    PFILE_FS_VOLUME_INFORMATION pFSVolInfo = NULL;
    PSMB_FS_VOLUME_INFO_HEADER pFSVolInfoHeader = NULL;
    USHORT   usVolumeLabelLen = 0;

    pFSVolInfo = (PFILE_FS_VOLUME_INFORMATION)pVolumeInfo;

    usBytesRequired = sizeof(SMB_FS_VOLUME_INFO_HEADER);

    usVolumeLabelLen = wc16slen(pFSVolInfo->VolumeLabel) * sizeof(wchar16_t);

    usBytesRequired += usVolumeLabelLen;

    if (usBytesRequired > usMaxDataCount)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFSVolInfoHeader = (PSMB_FS_VOLUME_INFO_HEADER)pDataCursor;
    pFSVolInfoHeader->bSupportsObjects = pFSVolInfo->SupportsObjects;
    pFSVolInfoHeader->llVolumeCreationTime = pFSVolInfo->VolumeCreationTime;
    pFSVolInfoHeader->ulVolumeSerialNumber = pFSVolInfo->VolumeSerialNumber;
    pFSVolInfoHeader->ulVolumeLabelLength = usVolumeLabelLen;

    pDataCursor += sizeof(SMB_FS_VOLUME_INFO_HEADER);

    if (usVolumeLabelLen)
    {
        memcpy(pDataCursor, (PBYTE)pFSVolInfo->VolumeLabel, usVolumeLabelLen);
    }

    *ppData = pData;
    *pusDataLen = usBytesRequired;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFSSizeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE    pOutBuffer            = pSmbResponse->pBuffer;
    ULONG    ulBytesAvailable      = pSmbResponse->ulBytesAvailable;
    ULONG    ulOffset              = 0;
    USHORT   usBytesUsed           = 0;
    ULONG    ulTotalBytesUsed      = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    FILE_FS_SIZE_INFORMATION fSSizeInfo = {0};

    ntStatus = IoQueryVolumeInformationFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        &fSSizeInfo,
                        sizeof(fSSizeInfo),
                        FileFsSizeInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
                    pSmbRequest->pHeader->pid,
                    pCtxSmb1->pSession->uid,
                    pSmbRequest->pHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    (PBYTE)&fSSizeInfo,
                    sizeof(fSSizeInfo),
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

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
NTSTATUS
SrvBuildFSAttributeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    IO_FILE_HANDLE    hFile,
    USHORT            usMaxDataCount
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE    pOutBuffer            = pSmbResponse->pBuffer;
    ULONG    ulBytesAvailable      = pSmbResponse->ulBytesAvailable;
    ULONG    ulOffset              = 0;
    USHORT   usBytesUsed           = 0;
    ULONG    ulTotalBytesUsed      = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PBYTE    pVolumeInfo = NULL;
    USHORT   usBytesAllocated = 0;

    usBytesAllocated = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = SrvAllocateMemory(usBytesAllocated, (PVOID*)&pVolumeInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ntStatus = IoQueryVolumeInformationFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        pVolumeInfo,
                        usBytesAllocated,
                        FileFsAttributeInformation);
        if (ntStatus == STATUS_SUCCESS)
        {
            break;
        }
        else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewSize = usBytesAllocated + 256 * sizeof(wchar16_t);

            ntStatus = SMBReallocMemory(
                            pVolumeInfo,
                            (PVOID*)&pVolumeInfo,
                            usNewSize);
            BAIL_ON_NT_STATUS(ntStatus);

            usBytesAllocated = usNewSize;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (TRUE);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TRANSACTION2,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
                    pSmbRequest->pHeader->pid,
                    pCtxSmb1->pSession->uid,
                    pSmbRequest->pHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 10 + setupCount;

    ntStatus = SrvMarshallFSAttributeInfo(
                    pVolumeInfo,
                    usBytesAllocated,
                    usMaxDataCount,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    pData,
                    usDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pVolumeInfo)
    {
        SrvFreeMemory(pVolumeInfo);
    }
    if (pData)
    {
        SrvFreeMemory(pData);
    }

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
NTSTATUS
SrvMarshallFSAttributeInfo(
    PBYTE   pVolumeInfo,
    USHORT  usBytesAllocated,
    USHORT  usMaxDataCount,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usBytesRequired = 0;
    PFILE_FS_ATTRIBUTE_INFORMATION pFSAttrInfo = NULL;
    PSMB_FS_ATTRIBUTE_INFO_HEADER pFSAttrInfoHeader = NULL;
    USHORT   usVolumeNameLen = 0;

    pFSAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)pVolumeInfo;

    usBytesRequired = sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);

    usVolumeNameLen = wc16slen(pFSAttrInfo->FileSystemName);
    if (!usVolumeNameLen)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usBytesRequired += usVolumeNameLen * sizeof(wchar16_t);

    if (usBytesRequired > usMaxDataCount)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(usBytesRequired, (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFSAttrInfoHeader = (PSMB_FS_ATTRIBUTE_INFO_HEADER)pDataCursor;
    pFSAttrInfoHeader->ulFSAttributes = pFSAttrInfo->FileSystemAttributes;
    pFSAttrInfoHeader->lMaxFilenameLen = pFSAttrInfo->MaximumComponentNameLength;
    pFSAttrInfoHeader->ulFileSystemNameLen = usVolumeNameLen * sizeof(wchar16_t);

    pDataCursor += sizeof(SMB_FS_ATTRIBUTE_INFO_HEADER);

    if (usVolumeNameLen)
    {
        memcpy(pDataCursor, (PBYTE)pFSAttrInfo->FileSystemName, usVolumeNameLen * sizeof(wchar16_t));
    }

    *ppData = pData;
    *pusDataLen = usBytesRequired;

cleanup:

    return ntStatus;

error:

    *ppData = NULL;
    *pusDataLen = 0;

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    goto cleanup;
}


