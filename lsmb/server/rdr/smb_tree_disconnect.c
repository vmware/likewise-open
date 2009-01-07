/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        tree_disconnect.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        SMB Tree Disconnect Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

uint32_t
TreeDisconnect(
    PSMB_TREE pTree
    )
{
    uint32_t dwError = 0;
    SMB_PACKET packet = {0};
    SMB_RESPONSE *pResponse = NULL;
    SMB_PACKET *pResponsePacket = NULL;
    uint16_t wMid = 0;
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
                COM_TREE_DISCONNECT,
                0,
                0,
                pTree->tid,
                0,
                pTree->pSession->uid,
                wMid,
                SMBSrvClientSessionSignMessages(pTree->pSession),
                &packet);
    BAIL_ON_SMB_ERROR(dwError);

    packet.pSMBHeader->wordCount = 0;

    /* @todo: to restart properly, we need a marshalling function to check the
       size of bytecount against the size of the remaining packet bytes. */
    packet.pData = packet.pParams;          /* ByteCount */
    packet.bufferUsed += sizeof(uint16_t);
    *((uint16_t *) packet.pData) = 0;

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

    return dwError;

error:

    goto cleanup;
}
