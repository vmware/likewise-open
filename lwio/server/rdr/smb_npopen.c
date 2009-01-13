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
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB Client Open Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

uint32_t
NPOpen(
    SMB_TREE  *pTree,
    wchar16_t *pwszPath,
    DWORD dwDesiredAccess,
    DWORD dwSharedMode,
    DWORD dwCreationDisposition,
    DWORD dwFlagsAndAttributes,
    uint16_t  *pFid
    )
{
    uint32_t dwError = 0;
    SMB_PACKET packet = {0};
    uint32_t packetByteCount = 0;
    CREATE_REQUEST_HEADER *pHeader = NULL;
    CREATE_RESPONSE_HEADER *pResponseHeader = NULL;
    SMB_RESPONSE *pResponse = NULL;
    PSMB_PACKET pResponsePacket = NULL;
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
                COM_NT_CREATE_ANDX,
                0,
                0,
                pTree->tid,
                0,
                pTree->pSession->uid,
                wMid,
                SMBSrvClientSessionSignMessages(pTree->pSession),
                &packet);
    BAIL_ON_SMB_ERROR(dwError);

    packet.pData = packet.pParams + sizeof(CREATE_REQUEST_HEADER);

    /* @todo: handle size restart */
    packet.bufferUsed += sizeof(CREATE_REQUEST_HEADER);

    /* If most commands have word counts which are easy to compute, this
       should be folded into a parameter to SMBPacketMarshallHeader() */
    packet.pSMBHeader->wordCount = 24;

    pHeader = (CREATE_REQUEST_HEADER *) packet.pParams;
    packet.pByteCount = &pHeader->byteCount;

    pHeader->reserved = 0;
    /* @todo: does the length include alignment padding? */
    pHeader->nameLength = wc16slen(pwszPath) * sizeof(wchar16_t) +
        sizeof(WNUL);
    pHeader->flags = 0;
    pHeader->rootDirectoryFid = 0;
    pHeader->desiredAccess = dwDesiredAccess;
    pHeader->allocationSize = 0;
    pHeader->extFileAttributes = dwFlagsAndAttributes;
    pHeader->shareAccess = dwSharedMode;
    pHeader->createOptions = 0x80; /* FIXME */
    pHeader->impersonationLevel = 0x2; /* FIXME */

    switch (dwCreationDisposition)
    {
    case CREATE_NEW:
        pHeader->createDisposition = 0x2;
        break;
    case CREATE_ALWAYS:
        pHeader->createDisposition = 0x5;
        break;
    case OPEN_EXISTING:
        pHeader->createDisposition = 0x1;
        break;
    case OPEN_ALWAYS:
        pHeader->createDisposition = 0x3;
        break;
    case TRUNCATE_EXISTING:
        pHeader->createDisposition = 0x4;
        break;
    default:
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

    /* @todo: handle buffer size restart with ERESTART */
    dwError = MarshallCreateRequestData(
                packet.pData,
                packet.bufferLen - packet.bufferUsed,
                (packet.pData - (uint8_t *) packet.pSMBHeader) % 2,
                &packetByteCount,
                pwszPath);
    BAIL_ON_SMB_ERROR(dwError);

    assert(packetByteCount <= UINT16_MAX);

    *packet.pByteCount = (uint16_t) packetByteCount;

    packet.bufferUsed += *packet.pByteCount;

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
       tree.*/
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

    dwError = UnmarshallSMBResponseCreate(
                pResponsePacket->pParams,
                pResponsePacket->bufferLen - pResponsePacket->bufferUsed,
                &pResponseHeader);
    BAIL_ON_SMB_ERROR(dwError);

    *pFid = pResponseHeader->fid;

cleanup:

    if (pResponsePacket)
    {
        SMBSocketPacketFree(
            pTree->pSession->pSocket,
            pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBSocketBufferFree(pTree->pSession->pSocket,
                            packet.pRawBuffer,
                            packet.bufferLen);
    }

    if (pResponse)
    {
        SMBResponseFree(pResponse);
    }

    return dwError;

error:

    *pFid = 0;

    goto cleanup;
}
