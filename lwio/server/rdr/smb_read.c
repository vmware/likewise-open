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

uint32_t
WireReadFile(
    PSMB_TREE pTree,
    uint16_t  fid,
    uint64_t  llFileReadOffset,
    uint8_t*  pReadBuffer,
    uint16_t  wReadLen,
    uint16_t* pwRead,
    void*     pOverlapped
    )
{
    uint32_t dwError = 0;
    SMB_PACKET packet = {0};
    uint16_t packetByteCount = 0;
    READ_REQUEST_HEADER *pRequestHeader = NULL;
    READ_RESPONSE_HEADER *pResponseHeader = NULL;
    PSMB_RESPONSE pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
    uint16_t wMid = 0;
    uint16_t wBytesRead = 0;
    DWORD dwResponseSequence = 0;

    SMB_LOG_DEBUG("Begin: WireReadFile: fid [%d] Read Length [%d]", fid, wReadLen);

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
                COM_READ_ANDX,
                0,
                0,
                pTree->tid,
                0,
                pTree->pSession->uid,
                wMid,
                SMBSrvClientSessionSignMessages(pTree->pSession),
                &packet);
    BAIL_ON_SMB_ERROR(dwError);

    packet.pData = packet.pParams + sizeof(READ_REQUEST_HEADER);
    /* @todo: handle size restart */
    packet.bufferUsed += sizeof(READ_REQUEST_HEADER);

    /* if LM 0.12 is used, word count is 12. Otherwise 10 */
    packet.pSMBHeader->wordCount = 12;

    pRequestHeader = (READ_REQUEST_HEADER *) packet.pParams;

    pRequestHeader->fid = fid;
    pRequestHeader->offset = llFileReadOffset & 0x00000000FFFFFFFFLL;
    pRequestHeader->maxCount = wReadLen;
    pRequestHeader->minCount = wReadLen; /* blocking read */
    /* @todo: if CAP_LARGE_READX is set, what are the high 16 bits? */
    pRequestHeader->maxCountHigh = 0;
    pRequestHeader->remaining = 0; /* obsolete */
    pRequestHeader->offsetHigh = (llFileReadOffset & 0xFFFFFFFF00000000LL) >> 32;
    pRequestHeader->byteCount = 0;

    packet.bufferUsed += packetByteCount;

    dwError = SMBPacketMarshallFooter(&packet);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBResponseCreate(wMid, &pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientTreeAddResponse(pTree, pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    if (SMBSrvClientSessionSignMessages(pTree->pSession))
    {
        DWORD dwSequence = SMBSocketGetNextSequence(pTree->pSession->pSocket);

        dwError = SMBPacketSign(
                        &packet,
                        dwSequence,
                        pTree->pSession->pSocket->pSessionKey,
                        pTree->pSession->pSocket->dwSessionKeyLength);
        BAIL_ON_SMB_ERROR(dwError);

        // resultant is the response sequence from server
        dwResponseSequence = dwSequence + 1;
    }

    /* @todo: on send packet error, the response must be removed from the
       tree. */
    dwError = SMBPacketSend(pTree->pSession->pSocket, &packet);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBTreeReceiveResponse(
                    pTree,
                    pResponse,
                    &pResponsePacket);
    BAIL_ON_SMB_ERROR(dwError);

    if (SMBSrvClientSessionSignMessages(pTree->pSession))
    {
        dwError = SMBPacketVerifySignature(
                        pResponsePacket,
                        dwResponseSequence,
                        pTree->pSession->pSocket->pSessionKey,
                        pTree->pSession->pSocket->dwSessionKeyLength);
        BAIL_ON_SMB_ERROR(dwError);
    }

    if (pResponsePacket->pSMBHeader->error)
    {
        SMB_LOG_DEBUG("WireReadFile [error code: 0x%X]", pResponsePacket->pSMBHeader->error);

        if (pResponsePacket->pSMBHeader->error != STATUS_PIPE_EMPTY)
        {
            dwError = pResponsePacket->pSMBHeader->error;
            BAIL_ON_SMB_ERROR(dwError);
        }
    }
    else
    {
        pResponseHeader = (READ_RESPONSE_HEADER *) pResponsePacket->pParams;

        if (pResponseHeader->dataLength)
        {
            wBytesRead = pResponseHeader->dataLength;

            memcpy(pReadBuffer,
                   (uint8_t*)pResponsePacket->pSMBHeader + pResponseHeader->dataOffset,
                   wBytesRead);
        }
    }

    *pwRead = wBytesRead;

cleanup:

    if (pResponsePacket)
    {
        SMBSocketPacketFree(
            pTree->pSession->pSocket,
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

    SMB_LOG_DEBUG("End: WireReadFile: fid [%d] Bytes read [%d]", fid, *pwRead);

    return dwError;

error:

    *pwRead = 0;

    goto cleanup;
}

