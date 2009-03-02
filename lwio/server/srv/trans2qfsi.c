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

typedef struct _SMB_FS_ATTRIBUTE_INFO_HEADER
{
    ULONG ulFSAttributes;
    LONG  lMaxFilenameLen;
    ULONG ulFileSystemNameLen;
} SMB_FS_ATTRIBUTE_INFO_HEADER, *PSMB_FS_ATTRIBUTE_INFO_HEADER;

typedef struct _SMB_FS_VOLUME_INFO_HEADER
{
    LONG64  llVolumeCreationTime;
    ULONG   ulVolumeSerialNumber;
    ULONG   ulVolumeLabelLength;
    BOOLEAN bSupportsObjects;
    UCHAR   pad;
} __attribute__((__packed__)) SMB_FS_VOLUME_INFO_HEADER, *PSMB_FS_VOLUME_INFO_HEADER;

static
NTSTATUS
SrvBuildFSAllocationInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildFSInfoVolumeResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildFSVolumeInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
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
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildFSDeviceInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildFSAttributeInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
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

static
NTSTATUS
SrvBuildFSCifsUnixInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvBuildMacFSInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessTrans2QueryFilesystemInformation(
    PSMB_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usInfoLevel = 0;
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE  pTree = NULL;
    PSMB_PACKET    pSmbResponse = NULL;
    IO_FILE_HANDLE hFile = NULL;
    IO_FILE_NAME   fileName = {0};
    PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock = NULL;
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    IO_STATUS_BLOCK     ioStatusBlock = {0};
    BOOLEAN             bInLock = FALSE;

    if ((pRequestHeader->parameterCount != 2) &&
        (pRequestHeader->parameterCount != 4))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree(
                    pSession,
                    pSmbRequest->pSMBHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    fileName.FileName = pTree->pShareInfo->pwszPath;

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSecurityContext,
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

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    usInfoLevel = *((PUSHORT)pParameters);

    switch (usInfoLevel)
    {
        case SMB_INFO_ALLOCATION:

            ntStatus = SrvBuildFSAllocationInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            pRequestHeader->maxDataCount,
                            &pSmbResponse);

            break;

        case SMB_INFO_VOLUME:

            ntStatus = SrvBuildFSInfoVolumeResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            pRequestHeader->maxDataCount,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FS_VOLUME_INFO:

            ntStatus = SrvBuildFSVolumeInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            pRequestHeader->maxDataCount,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FS_SIZE_INFO:

            ntStatus = SrvBuildFSSizeInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            pRequestHeader->maxDataCount,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FS_DEVICE_INFO:

            ntStatus = SrvBuildFSDeviceInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            pRequestHeader->maxDataCount,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FS_ATTRIBUTE_INFO:

            ntStatus = SrvBuildFSAttributeInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            pRequestHeader->maxDataCount,
                            &pSmbResponse);

            break;


        case SMB_QUERY_CIFS_UNIX_INFO:

            ntStatus = SrvBuildFSCifsUnixInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            pRequestHeader->maxDataCount,
                            &pSmbResponse);

            break;

        case SMB_QUERY_MAC_FS_INFO:

            ntStatus = SrvBuildMacFSInfoResponse(
                            pConnection,
                            pSmbRequest,
                            hFile,
                            pRequestHeader->maxDataCount,
                            &pSmbResponse);

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    if (pTree)
    {
        SMB_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFSAllocationInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    USHORT   usNumPackageBytesUsed = 0;
    PSMB_PACKET pSmbResponse = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    FILE_FS_SIZE_INFORMATION fSSizeInfo = {0};
    SMB_FS_INFO_ALLOCATION fsAllocInfo = {0};

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
    fsAllocInfo.ulNumAllocationUnits = SMB_MIN(UINT32_MAX, fSSizeInfo.TotalAllocationUnits);
    fsAllocInfo.ulNumSectorsPerAllocationUnit = fSSizeInfo.SectorsPerAllocationUnit;
    fsAllocInfo.ulNumUnitsAvailable = SMB_MIN(UINT32_MAX, fSSizeInfo.AvailableAllocationUnits);
    fsAllocInfo.usNumBytesPerSector = SMB_MIN(UINT16_MAX, fSSizeInfo.BytesPerSector);

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    (PBYTE)&fsAllocInfo,
                    sizeof(fsAllocInfo),
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFSInfoVolumeResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFSVolumeInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    USHORT   usNumPackageBytesUsed = 0;
    PSMB_PACKET pSmbResponse = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PBYTE    pVolumeInfo = NULL;
    USHORT   usBytesAllocated = 0;

    usBytesAllocated = sizeof(FILE_FS_VOLUME_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = LW_RTL_ALLOCATE(&pVolumeInfo, BYTE, usBytesAllocated);
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

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = SrvMarshallFSVolumeInfo(
                    pVolumeInfo,
                    usBytesAllocated,
                    usMaxDataCount,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    pData,
                    usDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pVolumeInfo)
    {
        LwRtlMemoryFree(pVolumeInfo);
    }
    if (pData)
    {
        LwRtlMemoryFree(pData);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
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

    usVolumeLabelLen = wc16slen(pFSVolInfo->VolumeLabel);

    usBytesRequired += usVolumeLabelLen * sizeof(wchar16_t);

    if (usBytesRequired > usMaxDataCount)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(&pData, BYTE, usBytesRequired);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFSVolInfoHeader = (PSMB_FS_VOLUME_INFO_HEADER)pDataCursor;
    pFSVolInfoHeader->bSupportsObjects = pFSVolInfo->SupportsObjects;
    pFSVolInfoHeader->llVolumeCreationTime = pFSVolInfo->VolumeCreationTime;
    pFSVolInfoHeader->ulVolumeSerialNumber = pFSVolInfo->VolumeSerialNumber;
    pFSVolInfoHeader->ulVolumeLabelLength = usVolumeLabelLen * sizeof(wchar16_t);

    pDataCursor += sizeof(SMB_FS_VOLUME_INFO_HEADER);

    if (usVolumeLabelLen)
    {
        memcpy(pDataCursor, (PBYTE)pFSVolInfo->VolumeLabel, usVolumeLabelLen * sizeof(wchar16_t));
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
        LwRtlMemoryFree(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFSSizeInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    USHORT   usNumPackageBytesUsed = 0;
    PSMB_PACKET pSmbResponse = NULL;
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

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    (PBYTE)&fSSizeInfo,
                    sizeof(fSSizeInfo),
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFSDeviceInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildFSAttributeInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    PUSHORT  pSetup = NULL;
    BYTE     setupCount = 0;
    USHORT   usDataOffset = 0;
    USHORT   usParameterOffset = 0;
    USHORT   usNumPackageBytesUsed = 0;
    PSMB_PACKET pSmbResponse = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PBYTE    pVolumeInfo = NULL;
    USHORT   usBytesAllocated = 0;

    usBytesAllocated = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = LW_RTL_ALLOCATE(&pVolumeInfo, BYTE, usBytesAllocated);
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

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_TRANSACTION2,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10 + setupCount;

    ntStatus = SrvMarshallFSAttributeInfo(
                    pVolumeInfo,
                    usBytesAllocated,
                    usMaxDataCount,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    NULL,
                    0,
                    pData,
                    usDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pVolumeInfo)
    {
        LwRtlMemoryFree(pVolumeInfo);
    }
    if (pData)
    {
        LwRtlMemoryFree(pData);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

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

    ntStatus = LW_RTL_ALLOCATE(&pData, BYTE, usBytesRequired);
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
        LwRtlMemoryFree(pData);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildFSCifsUnixInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
SrvBuildMacFSInfoResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    USHORT              usMaxDataCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

