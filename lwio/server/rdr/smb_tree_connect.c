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
 *        tree_connect.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB Client Tree Connect Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

DWORD
TreeConnect(
    SMB_SESSION *pSession,
    wchar16_t   *pwszPath,
    uint16_t    *pTID
    )
{
    DWORD dwError = 0;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    TREE_CONNECT_REQUEST_HEADER *pHeader = NULL;
    SMB_PACKET *pResponsePacket = NULL;
    BOOLEAN bInLock = FALSE;
    DWORD dwResponseSequence = 0;

    /* @todo: make initial length configurable */
    dwError = SMBPacketBufferAllocate(
                    pSession->pSocket->hPacketAllocator,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBPacketMarshallHeader(
                    packet.pRawBuffer,
                    packet.bufferLen,
                    COM_TREE_CONNECT_ANDX,
                    0,
                    0,
                    0,
                    0,
                    pSession->uid,
                    0,
                    SMBSrvClientSessionSignMessages(pSession),
                    &packet);
    BAIL_ON_SMB_ERROR(dwError);

    packet.pData = packet.pParams + sizeof(TREE_CONNECT_REQUEST_HEADER);
    packet.bufferUsed += sizeof(TREE_CONNECT_REQUEST_HEADER);
    /* If most commands have word counts which are easy to compute, this
       should be folded into a parameter to SMBPacketMarshallHeader() */
    packet.pSMBHeader->wordCount = 4;

    pHeader = (TREE_CONNECT_REQUEST_HEADER *) packet.pParams;
    packet.pByteCount = &pHeader->byteCount;

    pHeader->flags = 0;
    pHeader->passwordLength = 0;    /* Authentication handled via uid */

    /* @todo: handle buffer size0xFF restart with ERESTART */
    dwError = MarshallTreeConnectRequestData(
                    packet.pData,
                    packet.bufferLen - packet.bufferUsed,
                    (packet.pData - (uint8_t *) packet.pSMBHeader) % 2,
                    &packetByteCount,
                    pwszPath,
                    (uchar8_t *) "?????");
    BAIL_ON_SMB_ERROR(dwError);

    assert(packetByteCount <= UINT16_MAX);
    *packet.pByteCount = (uint16_t) packetByteCount;
    packet.bufferUsed += *packet.pByteCount;

    dwError = SMBPacketMarshallFooter(&packet);
    BAIL_ON_SMB_ERROR(dwError);

    if (SMBSrvClientSessionSignMessages(pSession))
    {
        DWORD dwSequence = SMBSocketGetNextSequence(pSession->pSocket);

        dwError = SMBPacketSign(
                        &packet,
                        dwSequence,
                        pSession->pSocket->pSessionKey,
                        pSession->pSocket->dwSessionKeyLength);
        BAIL_ON_SMB_ERROR(dwError);

        // resultant is the response sequence from server
        dwResponseSequence = dwSequence + 1;
    }

    /* Because there's no MID, only one TREE_CONNECT_ANDX packet can be
       outstanding. */
    /* @todo: test multiple session setups with multiple MIDs */
    SMB_LOCK_MUTEX(bInLock, &pSession->treeMutex);

    dwError = SMBSocketSend(pSession->pSocket, &packet);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSessionReceiveResponse(
                    pSession,
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

    *pTID = pResponsePacket->pSMBHeader->tid;

cleanup:

    if (pResponsePacket)
    {
        SMBPacketFree(pSession->pSocket, pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBPacketBufferFree(
                pSession->pSocket->hPacketAllocator,
                packet.pRawBuffer,
                packet.bufferLen);
    }

    SMB_UNLOCK_MUTEX(bInLock, &pSession->treeMutex);

    return dwError;

error:

    *pTID = 0;

    goto cleanup;
}
