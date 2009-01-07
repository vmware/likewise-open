/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        logoff.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        SMB Client Logoff Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

uint32_t
Logoff(
    PSMB_SESSION pSession
    )
{
    uint32_t dwError = 0;
    SMB_PACKET packet = {0};
    PSMB_PACKET pResponsePacket = NULL;
    DWORD dwSequence = 0;
    DWORD dwResponseSequence = 0;

    /* @todo: make initial length configurable */
    dwError = SMBSocketBufferAllocate(
                    pSession->pSocket,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBPacketMarshallHeader(
                packet.pRawBuffer,
                packet.bufferLen,
                COM_LOGOFF_ANDX,
                0,
                0,
                0,
                0,
                pSession->uid,
                0,
                SMBSrvClientSessionSignMessages(pSession),
                &packet);
    BAIL_ON_SMB_ERROR(dwError);

    packet.pSMBHeader->wordCount = 2;

    packet.pData = packet.pParams;
    packet.bufferUsed += sizeof(uint16_t); /* ByteCount */
    *((uint16_t *) packet.pData) = 0;

    dwError = SMBPacketMarshallFooter(&packet);
    BAIL_ON_SMB_ERROR(dwError);

    if (SMBSrvClientSessionSignMessages(pSession))
    {
        dwSequence = SMBSocketGetNextSequence(pSession->pSocket);

        dwError = SMBPacketSign(
                        &packet,
                        dwSequence,
                        pSession->pSocket->pSessionKey,
                        pSession->pSocket->dwSessionKeyLength);
        BAIL_ON_SMB_ERROR(dwError);

        // resultant is the response sequence from server
        dwResponseSequence = dwSequence + 1;
    }

    dwError = SMBPacketSend(pSession->pSocket, &packet);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSocketReceiveLogoffResponse(
                    pSession->pSocket,
                    &pResponsePacket);
    BAIL_ON_SMB_ERROR(dwError);

    if (SMBSrvClientSessionSignMessages(pSession))
    {
        dwError = SMBPacketVerifySignature(
                        pResponsePacket,
                        dwResponseSequence,
                        pSession->pSocket->pSessionKey,
                        pSession->pSocket->dwSessionKeyLength);
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = pResponsePacket->pSMBHeader->error;
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pResponsePacket)
    {
        SMBSocketPacketFree(pSession->pSocket, pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBSocketBufferFree(
                pSession->pSocket,
                packet.pRawBuffer,
                packet.bufferLen);
    }

    return dwError;

error:

    goto cleanup;
}

