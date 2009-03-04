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

#include "rdr.h"

static
NTSTATUS
RdrUnmarshalQueryFileInfoReply(
    SMB_INFO_LEVEL infoLevel,
    PBYTE pData,
    USHORT usDataCount,
    PVOID pInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );

static
NTSTATUS
RdrUnmarshalQueryFileBasicInfoReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_BASIC_INFORMATION pBasicInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );

static
NTSTATUS
RdrUnmarshalQueryFileStandardInfoReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_STANDARD_INFORMATION pStandardInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );

static
NTSTATUS
RdrTransactQueryInfoFile(
    PSMB_TREE pTree,
    USHORT usFid,
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    );

NTSTATUS
RdrCallQueryInformationFile(
    HANDLE hFile,
    PVOID fileInformation,
    ULONG ulLength,
    FILE_INFORMATION_CLASS fileInformationClass,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_INFO_LEVEL infoLevel = 0;
    PSMB_CLIENT_FILE_HANDLE pFile = hFile;

    switch (fileInformationClass)
    {
    case FileBasicInformation:
        infoLevel = SMB_QUERY_FILE_BASIC_INFO;
        break;
    case FileStandardInformation:
        infoLevel = SMB_QUERY_FILE_STANDARD_INFO;
        break;
    default:
        ntStatus = STATUS_NOT_IMPLEMENTED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

    ntStatus = RdrTransactQueryInfoFile(
        pFile->pTree,
        pFile->fid,
        infoLevel,
        fileInformation,
        ulLength,
        pulInfoLengthUsed);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
RdrTransactQueryInfoFile(
    PSMB_TREE pTree,
    USHORT usFid,
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    TRANSACTION_SECONDARY_RESPONSE_HEADER *pResponseHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_QUERY_FILE_INFORMATION;
    SMB_QUERY_FILE_INFO_HEADER queryHeader = {0};
    USHORT usQueryHeaderOffset = 0;
    USHORT usQueryDataOffset = 0;
    ULONG ulOffset = 0;
    PUSHORT pusReplySetup = NULL;
    PUSHORT pusReplyByteCount = NULL;
    PBYTE pReplyParameters = NULL;
    PBYTE pReplyData = NULL;


    ntStatus = SMBPacketBufferAllocate(
        pTree->pSession->pSocket->hPacketAllocator,
        1024*64,
        &packet.pRawBuffer,
        &packet.bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeAcquireMid(
        pTree,
        &usMid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
        packet.pRawBuffer,
        packet.bufferLen,
        COM_TRANSACTION2,
        0,
        0,
        pTree->tid,
        0,
        pTree->pSession->uid,
        usMid,
        TRUE,
        &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    packet.pData = packet.pParams + sizeof(TRANSACTION_REQUEST_HEADER);

    packet.bufferUsed += sizeof(TRANSACTION_REQUEST_HEADER);

    packet.pSMBHeader->wordCount = 14 + sizeof(usSetup)/sizeof(USHORT);

    pHeader = (TRANSACTION_REQUEST_HEADER *) packet.pParams;

    queryHeader.usFid = usFid;
    queryHeader.infoLevel = infoLevel;

    ntStatus = WireMarshallTransactionRequestData(
        packet.pData,
        packet.bufferLen - packet.bufferUsed,
        &packetByteCount,
        &usSetup,
        sizeof(usSetup)/sizeof(USHORT),
        NULL,
        (PBYTE) &queryHeader,
        sizeof(queryHeader),
        &usQueryHeaderOffset,
        NULL,
        0,
        &usQueryDataOffset);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(packetByteCount <= UINT16_MAX);
    packet.bufferUsed += packetByteCount;

    pHeader->totalParameterCount = sizeof(queryHeader);
    pHeader->totalDataCount = 0;
    pHeader->maxParameterCount = sizeof(queryHeader);
    pHeader->maxDataCount = ulInfoLength;
    pHeader->maxSetupCount = sizeof(usSetup)/sizeof(USHORT);
    pHeader->flags = 0;
    pHeader->parameterCount = sizeof(queryHeader);
    pHeader->parameterOffset = usQueryHeaderOffset + (packet.pData - (PBYTE) packet.pSMBHeader);
    pHeader->dataCount = 0;
    pHeader->dataOffset = usQueryDataOffset + (packet.pData - (PBYTE) packet.pSMBHeader);
    pHeader->setupCount = sizeof(usSetup)/sizeof(USHORT);

    // byte order conversions
    SMB_HTOL16_INPLACE(pHeader->totalParameterCount);
    SMB_HTOL16_INPLACE(pHeader->totalDataCount);
    SMB_HTOL16_INPLACE(pHeader->maxParameterCount);
    SMB_HTOL16_INPLACE(pHeader->maxDataCount);
    SMB_HTOL8_INPLACE(pHeader->maxSetupCount);
    SMB_HTOL16_INPLACE(pHeader->flags);
    SMB_HTOL32_INPLACE(pHeader->timeout);
    SMB_HTOL16_INPLACE(pHeader->parameterCount);
    SMB_HTOL16_INPLACE(pHeader->parameterOffset);
    SMB_HTOL16_INPLACE(pHeader->dataCount);
    SMB_HTOL16_INPLACE(pHeader->dataOffset);
    SMB_HTOL8_INPLACE(pHeader->setupCount);

    ntStatus = SMBPacketMarshallFooter(&packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBResponseCreate(usMid, &pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvClientTreeAddResponse(pTree, pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSocketSend(pTree->pSession->pSocket, &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBTreeReceiveResponse(
        pTree,
        packet.haveSignature,
        packet.sequence + 1,
        pResponse,
        &pResponsePacket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(ntStatus);

    ulOffset = (PBYTE)pResponsePacket->pParams - (PBYTE)pResponsePacket->pSMBHeader;

    ntStatus = WireUnmarshallTransactionSecondaryResponse(
        pResponsePacket->pParams,
        pResponsePacket->pNetBIOSHeader->len - ulOffset,
        ulOffset,
        &pResponseHeader,
        &pusReplySetup,
        &pusReplyByteCount,
        NULL,
        &pReplyParameters,
        &pReplyData,
        0);
    BAIL_ON_NT_STATUS(ntStatus);

    /* @todo verify response setup/parameters match requested info level */
    ntStatus = RdrUnmarshalQueryFileInfoReply(
        infoLevel,
        pReplyData,
        pResponseHeader->totalDataCount,
        pInfo,
        ulInfoLength,
        pulInfoLengthUsed);

cleanup:

    if (pResponsePacket)
    {
        SMBPacketFree(
            pTree->pSession->pSocket->hPacketAllocator,
            pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBPacketBufferFree(pTree->pSession->pSocket->hPacketAllocator,
                            packet.pRawBuffer,
                            packet.bufferLen);
    }

    if (pResponse)
    {
        SMBResponseFree(pResponse);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
RdrUnmarshalQueryFileInfoReply(
    SMB_INFO_LEVEL infoLevel,
    PBYTE pData,
    USHORT usDataCount,
    PVOID pInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (infoLevel)
    {
    case SMB_QUERY_FILE_BASIC_INFO:
        ntStatus = RdrUnmarshalQueryFileBasicInfoReply(
            pData,
            usDataCount,
            pInfo,
            ulInfoLength,
            pulInfoLengthUsed);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case SMB_QUERY_FILE_STANDARD_INFO:
        ntStatus = RdrUnmarshalQueryFileStandardInfoReply(
            pData,
            usDataCount,
            pInfo,
            ulInfoLength,
            pulInfoLengthUsed);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

error:

    return ntStatus;
}


static
NTSTATUS
RdrUnmarshalQueryFileBasicInfoReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_BASIC_INFORMATION pBasicInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PTRANS2_FILE_BASIC_INFORMATION pBasicInfoPacked = NULL;

    pBasicInfoPacked = (PTRANS2_FILE_BASIC_INFORMATION) pData;

    if (usDataCount != sizeof(*pBasicInfoPacked))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulInfoLength < sizeof(*pBasicInfo))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBasicInfo->ChangeTime = pBasicInfoPacked->ChangeTime;
    pBasicInfo->FileAttributes = pBasicInfoPacked->FileAttributes;
    pBasicInfo->LastAccessTime = pBasicInfoPacked->LastAccessTime;
    pBasicInfo->LastWriteTime = pBasicInfoPacked->LastWriteTime;

    *pulInfoLengthUsed = sizeof(*pBasicInfo);

error:

    return ntStatus;
}

static
NTSTATUS
RdrUnmarshalQueryFileStandardInfoReply(
    PBYTE pData,
    USHORT usDataCount,
    PFILE_STANDARD_INFORMATION pStandardInfo,
    ULONG ulInfoLength,
    PULONG pulInfoLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PTRANS2_FILE_STANDARD_INFORMATION pStandardInfoPacked = NULL;

    pStandardInfoPacked = (PTRANS2_FILE_STANDARD_INFORMATION) pData;

    if (usDataCount != sizeof(*pStandardInfoPacked))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulInfoLength < sizeof(*pStandardInfo))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pStandardInfo->AllocationSize = pStandardInfoPacked->AllocationSize;
    pStandardInfo->EndOfFile = pStandardInfoPacked->EndOfFile;
    pStandardInfo->NumberOfLinks = pStandardInfoPacked->NumberOfLinks;
    pStandardInfo->DeletePending = pStandardInfoPacked->bDeletePending;
    pStandardInfo->Directory = pStandardInfoPacked->bDirectory;

    *pulInfoLengthUsed = sizeof(*pStandardInfo);

error:

    return ntStatus;
}
