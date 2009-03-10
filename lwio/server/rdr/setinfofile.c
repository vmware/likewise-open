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
RdrMarshalFileInfo(
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength,
    PBYTE pData,
    ULONG ulDataLength,
    PULONG pulDataLengthUsed
    );

static
NTSTATUS
RdrMarshalFileEndOfFileInfo(
    PVOID pInfo,
    ULONG ulInfoLength,
    PBYTE pData,
    ULONG ulDataLength,
    PULONG pulDataLengthUsed
    );

static
NTSTATUS
RdrMarshalFileUnixHlink(
    PVOID pInfo,
    ULONG ulInfoLength,
    PBYTE pData,
    ULONG ulDataLength,
    PULONG pulDataLengthUsed
    );

static
NTSTATUS
RdrMarshalFileDispositionInfo(
    PVOID pInfo,
    ULONG ulInfoLength,
    PBYTE pData,
    ULONG ulDataLength,
    PULONG pulDataLengthUsed
    );

NTSTATUS
RdrTransactSetInfoFile(
    PSMB_TREE pTree,
    USHORT usFid,
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_SET_FILE_INFORMATION;
    SMB_SET_FILE_INFO_HEADER setHeader = {0};
    USHORT usSetHeaderOffset = 0;
    USHORT usSetDataOffset = 0;
    BYTE fileInfo[1024];
    ULONG fileInfoLength = 0;

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

    setHeader.usFid = usFid;
    setHeader.infoLevel = infoLevel;

    ntStatus = RdrMarshalFileInfo(
        infoLevel,
        pInfo,
        ulInfoLength,
        fileInfo,
        sizeof(fileInfo),
        &fileInfoLength);

    ntStatus = WireMarshallTransactionRequestData(
        packet.pData,
        packet.bufferLen - packet.bufferUsed,
        &packetByteCount,
        &usSetup,
        sizeof(usSetup)/sizeof(USHORT),
        NULL,
        (PBYTE) &setHeader,
        sizeof(setHeader),
        &usSetHeaderOffset,
        fileInfo,
        fileInfoLength,
        &usSetDataOffset);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(packetByteCount <= UINT16_MAX);
    packet.bufferUsed += packetByteCount;

    pHeader->totalParameterCount = sizeof(setHeader);
    pHeader->totalDataCount = fileInfoLength;
    pHeader->maxParameterCount = sizeof(setHeader);
    pHeader->maxDataCount = ulInfoLength;
    pHeader->maxSetupCount = sizeof(usSetup)/sizeof(USHORT);
    pHeader->flags = 0;
    pHeader->parameterCount = sizeof(setHeader);
    pHeader->parameterOffset = usSetHeaderOffset + (packet.pData - (PBYTE) packet.pSMBHeader);
    pHeader->dataCount = fileInfoLength;
    pHeader->dataOffset = usSetDataOffset + (packet.pData - (PBYTE) packet.pSMBHeader);
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

NTSTATUS
RdrTransactSetInfoPath(
    PSMB_TREE pTree,
    PCWSTR pwszPath,
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    USHORT usMid = 0;
    USHORT usSetup = SMB_SUB_COMMAND_TRANS2_SET_PATH_INFORMATION;
    SMB_SET_PATH_INFO_HEADER setHeader = {0};
    USHORT usSetHeaderOffset = 0;
    USHORT usSetDataOffset = 0;
    BYTE fileInfo[1024];
    ULONG fileInfoLength = 0;
    ULONG ulPathLength = LwRtlWC16StringNumChars(pwszPath);
    PBYTE pCursor = NULL;

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

    setHeader.reserved = 0;
    setHeader.infoLevel = infoLevel;

    ntStatus = RdrMarshalFileInfo(
        infoLevel,
        pInfo,
        ulInfoLength,
        fileInfo,
        sizeof(fileInfo),
        &fileInfoLength);

    ntStatus = WireMarshallTransactionRequestData(
        packet.pData,
        packet.bufferLen - packet.bufferUsed,
        &packetByteCount,
        &usSetup,
        sizeof(usSetup)/sizeof(USHORT),
        NULL,
        (PBYTE) &setHeader,
        sizeof(setHeader) + (ulPathLength + 1) * sizeof(WCHAR),
        &usSetHeaderOffset,
        fileInfo,
        fileInfoLength,
        &usSetDataOffset);
    BAIL_ON_NT_STATUS(ntStatus);

    pCursor = packet.pData + usSetHeaderOffset + offsetof(SMB_SET_PATH_INFO_HEADER, pwszPath);

    if ((pCursor - (PBYTE) packet.pSMBHeader) % 2)
    {
        pCursor++;
    }

    SMB_HTOLWSTR(pCursor, pwszPath, ulPathLength);

    assert(packetByteCount <= UINT16_MAX);
    packet.bufferUsed += packetByteCount;

    pHeader->totalParameterCount = sizeof(setHeader) + sizeof(setHeader) + (ulPathLength + 1) * sizeof(WCHAR);
    pHeader->totalDataCount = fileInfoLength;
    pHeader->maxParameterCount = sizeof(setHeader);
    pHeader->maxDataCount = ulInfoLength;
    pHeader->maxSetupCount = sizeof(usSetup)/sizeof(USHORT);
    pHeader->flags = 0;
    pHeader->parameterCount = sizeof(setHeader) + sizeof(setHeader) + (ulPathLength + 1) * sizeof(WCHAR);
    pHeader->parameterOffset = usSetHeaderOffset + (packet.pData - (PBYTE) packet.pSMBHeader);
    pHeader->dataCount = fileInfoLength;
    pHeader->dataOffset = usSetDataOffset + (packet.pData - (PBYTE) packet.pSMBHeader);
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
RdrMarshalFileInfo(
    SMB_INFO_LEVEL infoLevel,
    PVOID pInfo,
    ULONG ulInfoLength,
    PBYTE pData,
    ULONG ulDataLength,
    PULONG pulDataLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (infoLevel)
    {
    case SMB_SET_FILE_END_OF_FILE_INFO:
        ntStatus = RdrMarshalFileEndOfFileInfo(
            pInfo,
            ulInfoLength,
            pData,
            ulDataLength,
            pulDataLengthUsed
            );
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case SMB_SET_FILE_UNIX_HLINK:
        ntStatus = RdrMarshalFileUnixHlink(
            pInfo,
            ulInfoLength,
            pData,
            ulDataLength,
            pulDataLengthUsed
            );
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case SMB_SET_FILE_DISPOSITION_INFO:
        ntStatus = RdrMarshalFileDispositionInfo(
            pInfo,
            ulInfoLength,
            pData,
            ulDataLength,
            pulDataLengthUsed
            );
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    default:
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
RdrMarshalFileEndOfFileInfo(
    PVOID pInfo,
    ULONG ulInfoLength,
    PBYTE pData,
    ULONG ulDataLength,
    PULONG pulDataLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFILE_END_OF_FILE_INFORMATION pEndInfo = pInfo;
    PTRANS2_FILE_END_OF_FILE_INFORMATION pEndInfoPacked = (PTRANS2_FILE_END_OF_FILE_INFORMATION) pData;

    if (ulInfoLength < sizeof(*pEndInfo))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulDataLength < sizeof(*pEndInfoPacked))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pEndInfoPacked->EndOfFile = SMB_HTOL64(pEndInfo->EndOfFile);

    *pulDataLengthUsed = sizeof(*pEndInfoPacked);

error:

    return ntStatus;
}

static
NTSTATUS
RdrMarshalFileUnixHlink(
    PVOID pInfo,
    ULONG ulInfoLength,
    PBYTE pData,
    ULONG ulDataLength,
    PULONG pulDataLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFILE_LINK_INFORMATION pHlinkInfo = pInfo;

    if (ulInfoLength < sizeof(*pHlinkInfo))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulDataLength < (pHlinkInfo->FileNameLength + 1) * sizeof(WCHAR))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SMB_HTOLWSTR(pData+1, pHlinkInfo->FileName, pHlinkInfo->FileNameLength);

    *pulDataLengthUsed = (pHlinkInfo->FileNameLength + 1) * sizeof(WCHAR);

error:

    return ntStatus;
}

static
NTSTATUS
RdrMarshalFileDispositionInfo(
    PVOID pInfo,
    ULONG ulInfoLength,
    PBYTE pData,
    ULONG ulDataLength,
    PULONG pulDataLengthUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PFILE_DISPOSITION_INFORMATION pDispInfo = pInfo;
    PTRANS2_FILE_DISPOSITION_INFORMATION pDispInfoPacked = (PTRANS2_FILE_DISPOSITION_INFORMATION) pData;

    if (ulInfoLength < sizeof(*pDispInfo))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (ulDataLength < sizeof(*pDispInfoPacked))
    {
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pDispInfoPacked->bFileIsDeleted = pDispInfo->DeleteFile;

    *pulDataLengthUsed = sizeof(*pDispInfoPacked);

error:

    return ntStatus;
}
