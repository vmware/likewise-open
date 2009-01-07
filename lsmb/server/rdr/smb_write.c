#include "includes.h"

uint32_t
WireWriteFile(
    PSMB_TREE pTree,
    uint16_t  fid,
    uint64_t  llFileWriteOffset,
    uint8_t*  pWriteBuffer,
    uint16_t  wWriteLen,
    uint16_t* pwWritten,
    void*     pOverlapped
    )
{
    uint32_t dwError = 0;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    WRITE_REQUEST_HEADER *pRequestHeader = NULL;
    WRITE_RESPONSE_HEADER *pResponseHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    SMB_PACKET *pResponsePacket = NULL;
    uint16_t wMid = 0;
    uint16_t wNumBytesWriteable = 0;
    DWORD dwResponseSequence = 0;

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
                COM_WRITE_ANDX,
                0,
                0,
                pTree->tid,
                0,
                pTree->pSession->uid,
                wMid,
                SMBSrvClientSessionSignMessages(pTree->pSession),
                &packet);
    BAIL_ON_SMB_ERROR(dwError);

    packet.pData = packet.pParams + sizeof(WRITE_REQUEST_HEADER);
    /* @todo: handle size restart */
    packet.bufferUsed += sizeof(WRITE_REQUEST_HEADER);

    packet.pSMBHeader->wordCount = 14;

    pRequestHeader = (WRITE_REQUEST_HEADER *) packet.pParams;

    pRequestHeader->fid = fid;
    pRequestHeader->offset = llFileWriteOffset & 0x00000000FFFFFFFFLL;
    pRequestHeader->reserved = 0;
    pRequestHeader->writeMode = 0;
    pRequestHeader->remaining = 0;

    /* ignored if CAP_LARGE_WRITEX is set */
    wNumBytesWriteable = UINT16_MAX - (packet.pParams - (uint8_t*)packet.pSMBHeader) - sizeof(WRITE_REQUEST_HEADER);
    // And, then the alignment
    wNumBytesWriteable -= (packet.pData - (uint8_t *) pRequestHeader) % 2;
    if (wWriteLen > wNumBytesWriteable)
    {
        wWriteLen = wNumBytesWriteable;
    }

    pRequestHeader->dataLength = wWriteLen;
    /* @todo: what is this value if CAP_LARGE_WRITEX is set? */
    pRequestHeader->dataLengthHigh = 0;
    pRequestHeader->dataOffset = 0;
    /* only present if wordCount = 14 and not 12 */
    pRequestHeader->offsetHigh = (llFileWriteOffset & 0xFFFFFFFF00000000LL) >> 32;
    pRequestHeader->byteCount = wWriteLen;

    dwError = MarshallWriteRequestData(
                packet.pData,
                packet.bufferLen - packet.bufferUsed,
                (packet.pData - (uint8_t *) pRequestHeader) % 2,
                &packetByteCount,
                &pRequestHeader->dataOffset,
                pWriteBuffer,
                wWriteLen);
    BAIL_ON_SMB_ERROR(dwError);

    packet.bufferUsed += packetByteCount;
    pRequestHeader->dataOffset += packet.pData - (uint8_t *) packet.pSMBHeader;

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

    dwError = pResponsePacket->pSMBHeader->error;
    BAIL_ON_SMB_ERROR(dwError);

    pResponseHeader = (WRITE_RESPONSE_HEADER*) pResponsePacket->pParams;

    *pwWritten = pResponseHeader->count;

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

    *pwWritten = 0;

    goto cleanup;
}
