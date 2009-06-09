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

typedef struct _SMB_FILE_STREAM_INFO_RESPONSE_HEADER
{
    ULONG  ulNextEntryOffset;
    ULONG  ulStreamNameLength;
    LONG64 llStreamSize;
    LONG64 llStreamAllocationSize;
} __attribute__((__packed__)) SMB_FILE_STREAM_INFO_RESPONSE_HEADER, *PSMB_FILE_STREAM_INFO_RESPONSE_HEADER;

static
NTSTATUS
SrvUnmarshallQueryFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    );

static
NTSTATUS
SrvBuildQueryFileInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usFid,
    SMB_INFO_LEVEL      smbInfoLevel,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvMarshallFileStreamInfo(
    PBYTE   pFileStreamInfo,
    USHORT  usBytesAvailable,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    );

NTSTATUS
SrvProcessTrans2QueryFileInformation(
    PLWIO_SRV_CONNECTION         pConnection,
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
    USHORT usFid = 0;
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL; // Do not free
    PSMB_PACKET pSmbResponse = NULL;

    ntStatus = SrvUnmarshallQueryFileInfoParams(
                    pParameters,
                    pRequestHeader->parameterCount,
                    &usFid,
                    &pSmbInfoLevel);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildQueryFileInfoResponse(
                    pConnection,
                    pSmbRequest,
                    usFid,
                    *pSmbInfoLevel,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallQueryFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL;
    USHORT   usFid = 0;
    PBYTE    pDataCursor = pParams;

    // FID
    if (ulBytesAvailable < sizeof(USHORT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usFid = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(USHORT);
    ulBytesAvailable -= sizeof(USHORT);

    // Info level
    if (ulBytesAvailable < sizeof(SMB_INFO_LEVEL))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbInfoLevel = (PSMB_INFO_LEVEL)pDataCursor;
    pDataCursor += sizeof(SMB_INFO_LEVEL);
    ulBytesAvailable -= sizeof(SMB_INFO_LEVEL);

    *pusFid = usFid;
    *ppSmbInfoLevel = pSmbInfoLevel;

cleanup:

    return ntStatus;

error:

    *pusFid = 0;
    *ppSmbInfoLevel = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvBuildQueryFileInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    USHORT              usFid,
    SMB_INFO_LEVEL      smbInfoLevel,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    PLWIO_SRV_FILE    pFile = NULL;
    PSMB_PACKET pSmbResponse = NULL;

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

    ntStatus = SrvTreeFindFile(
                    pTree,
                    usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (smbInfoLevel)
    {
        case SMB_INFO_STANDARD :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_INFO_QUERY_EA_SIZE :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_INFO_QUERY_EAS_FROM_LIST :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_INFO_QUERY_ALL_EAS :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_INFO_IS_NAME_VALID :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_BASIC_INFO :

            ntStatus = SrvBuildQueryFileBasicInfoResponse(
                            pConnection,
                            pSmbRequest,
                            pFile->hFile,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FILE_STANDARD_INFO :

            ntStatus = SrvBuildQueryFileStandardInfoResponse(
                            pConnection,
                            pSmbRequest,
                            pFile->hFile,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FILE_EA_INFO :

            ntStatus = SrvBuildQueryFileEAInfoResponse(
                            pConnection,
                            pSmbRequest,
                            pFile->hFile,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FILE_NAME_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_ALL_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_ALT_NAME_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_STREAM_INFO :

            ntStatus = SrvBuildQueryFileStreamInfoResponse(
                            pConnection,
                            pSmbRequest,
                            pFile->hFile,
                            &pSmbResponse);

            break;

        case SMB_QUERY_FILE_COMPRESSION_INFO :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_UNIX_BASIC :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_QUERY_FILE_UNIX_LINK :

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;


        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    if (pTree)
    {
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
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}


NTSTATUS
SrvBuildQueryFileBasicInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    FILE_BASIC_INFORMATION fileBasicInfo = {0};
    TRANS2_FILE_BASIC_INFORMATION fileBasicInfoPacked = {0};
    USHORT              usParam = 0;
    PUSHORT             pSetup = NULL;
    BYTE                setupCount = 0;
    USHORT              usDataOffset = 0;
    USHORT              usParameterOffset = 0;
    USHORT              usNumPackageBytesUsed = 0;

    ntStatus = IoQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileBasicInfo,
                    sizeof(fileBasicInfo),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
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

    fileBasicInfoPacked.ChangeTime = fileBasicInfo.ChangeTime;
    fileBasicInfoPacked.CreationTime = fileBasicInfo.CreationTime;
    fileBasicInfoPacked.FileAttributes = fileBasicInfo.FileAttributes;
    fileBasicInfoPacked.LastAccessTime = fileBasicInfo.LastAccessTime;
    fileBasicInfoPacked.LastWriteTime = fileBasicInfo.LastWriteTime;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    (PBYTE)&fileBasicInfoPacked,
                    sizeof(fileBasicInfoPacked),
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
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

NTSTATUS
SrvBuildQueryFileStandardInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    FILE_STANDARD_INFORMATION fileStandardInfo = {0};
    TRANS2_FILE_STANDARD_INFORMATION fileStandardInfoPacked = {0};
    USHORT              usParam = 0;
    PUSHORT             pSetup = NULL;
    BYTE                setupCount = 0;
    USHORT              usDataOffset = 0;
    USHORT              usParameterOffset = 0;
    USHORT              usNumPackageBytesUsed = 0;

    ntStatus = IoQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileStandardInfo,
                    sizeof(fileStandardInfo),
                    FileStandardInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
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

    fileStandardInfoPacked.AllocationSize = fileStandardInfo.AllocationSize;
    fileStandardInfoPacked.EndOfFile = fileStandardInfo.EndOfFile;
    fileStandardInfoPacked.NumberOfLinks = fileStandardInfo.NumberOfLinks;
    fileStandardInfoPacked.bDeletePending = fileStandardInfo.DeletePending;
    fileStandardInfoPacked.bDirectory = fileStandardInfo.Directory;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    (PBYTE)&fileStandardInfoPacked,
                    sizeof(fileStandardInfoPacked),
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
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

NTSTATUS
SrvBuildQueryFileEAInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    FILE_EA_INFORMATION fileEAInfo = {0};
    USHORT              usParam = 0;
    PUSHORT             pSetup = NULL;
    BYTE                setupCount = 0;
    USHORT              usDataOffset = 0;
    USHORT              usParameterOffset = 0;
    USHORT              usNumPackageBytesUsed = 0;

    ntStatus = IoQueryInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileEAInfo,
                    sizeof(fileEAInfo),
                    FileEaInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
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
                    (PBYTE)&usParam,
                    sizeof(usParam),
                    (PBYTE)&fileEAInfo,
                    sizeof(fileEAInfo),
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
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

NTSTATUS
SrvBuildQueryFileStreamInfoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    IO_FILE_HANDLE      hFile,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PBYTE   pFileStreamInfo = NULL;
    USHORT  usBytesAllocated = 0;
    USHORT  usParam = 0;
    PUSHORT pSetup = NULL;
    BYTE    setupCount = 0;
    USHORT  usDataOffset = 0;
    USHORT  usParameterOffset = 0;
    USHORT  usNumPackageBytesUsed = 0;
    PBYTE   pData = NULL;
    USHORT  usDataLen = 0;

    usBytesAllocated = sizeof(FILE_STREAM_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = LW_RTL_ALLOCATE(
                    &pFileStreamInfo,
                    BYTE,
                    usBytesAllocated);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ntStatus = IoQueryInformationFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        pFileStreamInfo,
                        usBytesAllocated,
                        FileStreamInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        if (ntStatus == STATUS_SUCCESS)
        {
            break;
        }
        else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewSize = usBytesAllocated + 256 * sizeof(wchar16_t);

            ntStatus = SMBReallocMemory(
                            pFileStreamInfo,
                            (PVOID*)&pFileStreamInfo,
                            usNewSize);
            BAIL_ON_NT_STATUS(ntStatus);

            usBytesAllocated = usNewSize;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (TRUE);

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
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

    ntStatus = SrvMarshallFileStreamInfo(
                    pFileStreamInfo,
                    usBytesAllocated,
                    &pData,
                    &usDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    setupCount,
                    (PBYTE)&usParam,
                    sizeof(usParam),
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

    if (pFileStreamInfo)
    {
        LwRtlMemoryFree(pFileStreamInfo);
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
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvMarshallFileStreamInfo(
    PBYTE   pFileStreamInfo,
    USHORT  usBytesAvailable,
    PBYTE*  ppData,
    PUSHORT pusDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pData = NULL;
    PBYTE    pDataCursor = NULL;
    USHORT   usBytesRequired = 0;
    USHORT   iInfoCount = 0;
    USHORT   usInfoCount = 0;
    USHORT   usOffset = 0;
    PFILE_STREAM_INFORMATION pFileStreamInfoCursor = NULL;

    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pFileStreamInfo;
    while (pFileStreamInfoCursor && (usBytesAvailable > 0))
    {
        USHORT usInfoBytesRequired = 0;

        usInfoBytesRequired = sizeof(SMB_FILE_STREAM_INFO_RESPONSE_HEADER);
        usInfoBytesRequired += wc16slen(pFileStreamInfoCursor->StreamName) * sizeof(wchar16_t);
        usInfoBytesRequired += sizeof(wchar16_t);

        if (usBytesAvailable < usInfoBytesRequired)
        {
            break;
        }

        usInfoCount++;

        usBytesAvailable -= usInfoBytesRequired;
        usBytesRequired += usInfoBytesRequired;

        if (pFileStreamInfoCursor->NextEntryOffset)
        {
            pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfo) + pFileStreamInfoCursor->NextEntryOffset);
        }
        else
        {
            pFileStreamInfoCursor = NULL;
        }
    }

    ntStatus = LW_RTL_ALLOCATE(
                    &pData,
                    BYTE,
                    usBytesRequired);
    BAIL_ON_NT_STATUS(ntStatus);

    pDataCursor = pData;
    pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)pFileStreamInfo;
    for (; iInfoCount < usInfoCount; iInfoCount++)
    {
        PSMB_FILE_STREAM_INFO_RESPONSE_HEADER pInfoHeader = NULL;
        USHORT usStreamNameLen = 0;

        pInfoHeader = (PSMB_FILE_STREAM_INFO_RESPONSE_HEADER)pDataCursor;

        pInfoHeader->ulNextEntryOffset = usOffset;
        pInfoHeader->llStreamAllocationSize = pFileStreamInfoCursor->StreamAllocationSize;
        pInfoHeader->llStreamSize = pFileStreamInfoCursor->StreamSize;
        pInfoHeader->ulStreamNameLength = pFileStreamInfoCursor->StreamNameLength * sizeof(wchar16_t);

        pDataCursor += sizeof(SMB_FILE_STREAM_INFO_RESPONSE_HEADER);
        usOffset += sizeof(SMB_FILE_STREAM_INFO_RESPONSE_HEADER);

        usStreamNameLen = wc16slen(pFileStreamInfoCursor->StreamName);
        if (usStreamNameLen)
        {
            memcpy(pDataCursor, (PBYTE)pFileStreamInfoCursor->StreamName, usStreamNameLen * sizeof(wchar16_t));
            pDataCursor += usStreamNameLen * sizeof(wchar16_t);
            usOffset += usStreamNameLen * sizeof(wchar16_t);
        }

        pDataCursor += sizeof(wchar16_t);
        usOffset += sizeof(wchar16_t);

        pFileStreamInfoCursor = (PFILE_STREAM_INFORMATION)(((PBYTE)pFileStreamInfo) + pFileStreamInfoCursor->NextEntryOffset);
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
