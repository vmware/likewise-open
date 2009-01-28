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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        open.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        SMB Client Transact Named Pipe Handler
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

uint32_t
NPTransact(
    SMB_TREE *pTree,
    uint16_t  fid,
    uint8_t  *pWrite,
    uint16_t writeLen,
    uint8_t  *pRead,
    uint16_t readLen
    )
{
    uint32_t dwError = 0;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    TRANSACTION_REQUEST_HEADER *pHeader = NULL;
    TRANSACTION_SECONDARY_REQUEST_HEADER *pSecondaryHeader = NULL;
    uint8_t  bSetupCount = 2;
    uint16_t setup[] = {0x26, fid};
    wchar16_t wszName[1024];
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    TRANSACTION_RESPONSE_HEADER* pResponseHeader = NULL;
    uint16_t wMid = 0;
    WORD wMaxAllowedSize = 0;
    WORD wIterWriteLen = writeLen;
    uint16_t wWriteOffset = 0;
    uint16_t wWriteRemaining = writeLen;
    uint16_t wReadRemaining = readLen;

    /* @todo: make initial length configurable */
    dwError = SMBSocketBufferAllocate(
                    pTree->pSession->pSocket,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBTreeAcquireMid(
                    pTree,
                    &wMid);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBPacketMarshallHeader(
                packet.pRawBuffer,
                packet.bufferLen,
                COM_TRANSACTION,
                0,
                0,
                pTree->tid,
                0,
                pTree->pSession->uid,
                wMid,
                TRUE,
                &packet);
    BAIL_ON_SMB_ERROR(dwError);

    packet.pData = packet.pParams + sizeof(TRANSACTION_REQUEST_HEADER);
    /* @todo: handle size restart */
    packet.bufferUsed += sizeof(TRANSACTION_REQUEST_HEADER);

    /* If most commands have word counts which are easy to compute, this
       should be folded into a parameter to SMBPacketMarshallHeader() */
    packet.pSMBHeader->wordCount = 14 + bSetupCount;

    pHeader = (TRANSACTION_REQUEST_HEADER *) packet.pParams;

    pHeader->totalParameterCount = 0;
    pHeader->totalDataCount = writeLen;
    pHeader->maxParameterCount = 0;
    pHeader->maxDataCount = readLen;
    pHeader->maxSetupCount = 0;
    pHeader->reserved = 0;
    pHeader->flags = 0;
    pHeader->timeout = 0; /* wait indefinitely */
    pHeader->reserved2 = 0;
    pHeader->parameterCount = 0;
    pHeader->dataCount = writeLen;
    pHeader->setupCount = bSetupCount;
    pHeader->reserved3 = 0;

    wcstowc16s(wszName, L"\\PIPE\\", sizeof(wszName));

    wMaxAllowedSize = pTree->pSession->pSocket->maxBufferSize;
    wMaxAllowedSize -= packet.pData - (uint8_t *) packet.pSMBHeader;
    wMaxAllowedSize -= sizeof(TRANSACTION_REQUEST_HEADER);
    wMaxAllowedSize -= 4; /* max alignment size */
    wMaxAllowedSize -= (wc16slen(wszName) + 1) * sizeof(wchar16_t);

    if (wIterWriteLen > wMaxAllowedSize)
    {
        wIterWriteLen = wMaxAllowedSize;
    }

    pHeader->dataCount = wIterWriteLen;

    dwError = MarshallTransactionRequestData(
                packet.pData,
                packet.bufferLen - packet.bufferUsed,
                &packetByteCount,
                &setup[0],
                bSetupCount*2,
                wszName,
                NULL,
                0,
                &pHeader->parameterOffset,
                pWrite,
                wIterWriteLen,
                &pHeader->dataOffset);
    BAIL_ON_SMB_ERROR(dwError);

    pHeader->parameterOffset += packet.pData - (uint8_t *) packet.pSMBHeader;
    pHeader->dataOffset += packet.pData - (uint8_t *) packet.pSMBHeader;

    packet.bufferUsed += packetByteCount;

    dwError = SMBPacketMarshallFooter(&packet);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBResponseCreate(wMid, &pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientTreeAddResponse(pTree, pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBPacketSend(pTree->pSession->pSocket, &packet);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBTreeReceiveResponse(
                    pTree,
                    packet.haveSignature,
                    packet.sequence + 1,
                    pResponse,
                    &pResponsePacket);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = pResponsePacket->pSMBHeader->error;
    BAIL_ON_SMB_ERROR(dwError);

    wWriteRemaining -= wIterWriteLen;
    pWrite += wIterWriteLen;
    wWriteOffset += wIterWriteLen;

    // verify interim response
    if (!pResponsePacket->pSMBHeader->wordCount)
    {
        SMB_LOG_VERBOSE("Received interim response");
    }
    else if (wReadRemaining)
    {
        pResponseHeader = (TRANSACTION_RESPONSE_HEADER *) packet.pParams;

        if (pResponseHeader->byteCount)
        {
            memcpy(pRead, pResponsePacket->pData, pResponseHeader->byteCount);

            wReadRemaining -= pResponseHeader->byteCount;
            pRead += pResponseHeader->byteCount;
        }
    }

    // Send secondary requests
    while (wWriteRemaining)
    {
        dwError = SMBPacketMarshallHeader(
                    packet.pRawBuffer,
                    packet.bufferLen,
                    COM_TRANSACTION_SECONDARY,
                    0,
                    0,
                    pTree->tid,
                    wMid,
                    pTree->pSession->uid,
                    0,
                    TRUE,
                    &packet);
        BAIL_ON_SMB_ERROR(dwError);

        packet.pData = packet.pParams + sizeof(TRANSACTION_SECONDARY_REQUEST_HEADER);
        /* @todo: handle size restart */
        packet.bufferUsed += sizeof(TRANSACTION_SECONDARY_REQUEST_HEADER);

        packet.pSMBHeader->wordCount = 8;

        pSecondaryHeader = (TRANSACTION_SECONDARY_REQUEST_HEADER *) packet.pParams;

        pSecondaryHeader->totalParameterCount = 0;
        pSecondaryHeader->totalDataCount = writeLen;
        pSecondaryHeader->parameterCount = 0;
        pSecondaryHeader->dataCount = writeLen;
        pSecondaryHeader->dataDisplacement = wWriteOffset;
        pSecondaryHeader->fid = fid;

        wMaxAllowedSize = pTree->pSession->pSocket->maxBufferSize;
        wMaxAllowedSize -= packet.pData - (uint8_t *) packet.pSMBHeader;
        wMaxAllowedSize -= sizeof(TRANSACTION_SECONDARY_REQUEST_HEADER);
        wMaxAllowedSize -= 4; /* max alignment size */

        wIterWriteLen = wWriteRemaining;
        if (wIterWriteLen > wMaxAllowedSize)
        {
            wIterWriteLen = wMaxAllowedSize;
        }

        pHeader->dataCount = wIterWriteLen;

        packetByteCount = 0;
        dwError = MarshallTransactionSecondaryRequestData(
                        packet.pData,
                        packet.bufferLen - packet.bufferUsed,
                        &packetByteCount,
                        NULL,
                        0,
                        &pHeader->parameterOffset,
                        pWrite,
                        wIterWriteLen,
                        &pHeader->dataOffset);
        BAIL_ON_SMB_ERROR(dwError);

        pHeader->parameterOffset += packet.pData - (uint8_t *) packet.pSMBHeader;
        pHeader->dataOffset += packet.pData - (uint8_t *) packet.pSMBHeader;

        packet.bufferUsed += packetByteCount;

        dwError = SMBPacketMarshallFooter(&packet);
        BAIL_ON_SMB_ERROR(dwError);

        dwError = SMBPacketSend(pTree->pSession->pSocket, &packet);
        BAIL_ON_SMB_ERROR(dwError);

        wWriteRemaining -= wIterWriteLen;
        wWriteOffset += wIterWriteLen;
        pWrite += wIterWriteLen;
    }

    while (wReadRemaining)
    {
        dwError = SMBTreeReceiveResponse(
                    pTree,
                    FALSE,
                    0,
                    pResponse,
                    &pResponsePacket);
        BAIL_ON_SMB_ERROR(dwError);

        pResponseHeader = (TRANSACTION_RESPONSE_HEADER *) packet.pParams;

        if (pResponseHeader->byteCount)
        {
            memcpy(pRead, pResponsePacket->pData, pResponseHeader->byteCount);

            wReadRemaining -= pResponseHeader->byteCount;
            pRead += pResponseHeader->byteCount;
        }
    }

cleanup:

    if (pResponsePacket)
    {
        SMBSocketPacketFree(pTree->pSession->pSocket,
                pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBSocketBufferFree(
                pTree->pSession->pSocket,
                packet.pRawBuffer,
                packet.bufferLen);
    }

    if (pResponse)
    {
        SMBResponseFree(pResponse);
    }

    return dwError;

error:

    goto cleanup;
}
