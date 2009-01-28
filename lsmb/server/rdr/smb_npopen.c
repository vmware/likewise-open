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
 *        SMB Client Open Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

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
    uint32_t smbCreateDisposition = 0;

    switch (dwCreationDisposition)
    {
    case CREATE_NEW:
        smbCreateDisposition = 2;
        break;
    case CREATE_ALWAYS:
        smbCreateDisposition = 5;
        break;
    case OPEN_EXISTING:
        smbCreateDisposition = 1;
        break;
    case OPEN_ALWAYS:
        smbCreateDisposition = 3;
        break;
    case TRUNCATE_EXISTING:
        smbCreateDisposition = 4;
        break;
    default:
        dwError = SMB_ERROR_INVALID_PARAMETER;
        BAIL_ON_SMB_ERROR(dwError);
    }

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
                TRUE,
                &packet);
    BAIL_ON_SMB_ERROR(dwError);

    packet.pData = packet.pParams + sizeof(CREATE_REQUEST_HEADER);

    /* @todo: handle size restart */
    packet.bufferUsed += sizeof(CREATE_REQUEST_HEADER);

    /* If most commands have word counts which are easy to compute, this
       should be folded into a parameter to SMBPacketMarshallHeader() */
    packet.pSMBHeader->wordCount = 24;

    pHeader = (CREATE_REQUEST_HEADER *) packet.pParams;

    pHeader->reserved = 0;
    /* @todo: does the length include alignment padding? */
    pHeader->nameLength = (wc16slen(pwszPath) + 1) * sizeof(wchar16_t);
    pHeader->flags = 0;
    pHeader->rootDirectoryFid = 0;
    pHeader->desiredAccess = dwDesiredAccess;
    pHeader->allocationSize = 0;
    pHeader->extFileAttributes = dwFlagsAndAttributes;
    pHeader->shareAccess = dwSharedMode;
    pHeader->createDisposition = smbCreateDisposition;
    pHeader->createOptions = 0x80; /* FIXME */
    pHeader->impersonationLevel = 0x2; /* FIXME */

    /* @todo: handle buffer size restart with ERESTART */
    dwError = MarshallCreateRequestData(
                packet.pData,
                packet.bufferLen - packet.bufferUsed,
                (packet.pData - (uint8_t *) packet.pSMBHeader) % 2,
                &packetByteCount,
                pwszPath);
    BAIL_ON_SMB_ERROR(dwError);

    assert(packetByteCount <= UINT16_MAX);
    pHeader->byteCount = (uint16_t) packetByteCount;
    packet.bufferUsed += packetByteCount;

    // byte order conversions
    SMB_HTOL8_INPLACE(pHeader->reserved);
    SMB_HTOL16_INPLACE(pHeader->nameLength);
    SMB_HTOL32_INPLACE(pHeader->flags);
    SMB_HTOL32_INPLACE(pHeader->rootDirectoryFid);
    SMB_HTOL32_INPLACE(pHeader->desiredAccess);
    SMB_HTOL64_INPLACE(pHeader->allocationSize);
    SMB_HTOL32_INPLACE(pHeader->extFileAttributes);
    SMB_HTOL32_INPLACE(pHeader->shareAccess);
    SMB_HTOL32_INPLACE(pHeader->createDisposition);
    SMB_HTOL32_INPLACE(pHeader->createOptions);
    SMB_HTOL32_INPLACE(pHeader->impersonationLevel);
    SMB_HTOL8_INPLACE(pHeader->securityFlags);
    SMB_HTOL16_INPLACE(pHeader->byteCount);

    dwError = SMBPacketMarshallFooter(&packet);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBResponseCreate(wMid, &pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientTreeAddResponse(pTree, pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    /* @todo: on send packet error, the response must be removed from the
       tree.*/
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
