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
 *        negotiate.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB Client Negotiate Handler
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

/* @todo: clean up */
uint32_t
Negotiate(
    SMB_SOCKET *pSocket
    )
{
    uint32_t ntStatus = 0;
    SMB_PACKET packet = {0};

    NEGOTIATE_RESPONSE_HEADER *pHeader;
    uint8_t *pSecurityBlob;
    uint8_t *pGUID;
    uint32_t securityBlobLen = 0;

    uint32_t packetByteCount = 0;

    wchar16_t nativeOS[1024];
    wchar16_t nativeLanMan[1024];
    wchar16_t nativeDomain[1024];
    BOOLEAN   bSocketLocked = FALSE;

    const uchar8_t *pszDialects[1] = { (uchar8_t *) "NT LM 0.12" };

    PSMB_PACKET pResponsePacket = NULL;

    /* @todo: make initial length configurable */
    ntStatus = SMBPacketBufferAllocate(
                    pSocket->hPacketAllocator,
                    1024*64,
                    &packet.pRawBuffer,
                    &packet.bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                packet.pRawBuffer,
                packet.bufferLen,
                COM_NEGOTIATE,
                0, /* error */
                0, /* is response */
                0xFFFF, /* tid */
                0, /* pid */
                0, /* uid */
                0, /* mid */
                FALSE, /* sign messages */
                &packet);
   BAIL_ON_NT_STATUS(ntStatus);

    /* @todo: messages with a header consisting only of ByteCount don't get
       their own HEADER structs (yet), which is confusing */
    packet.pByteCount = (uint16_t*) packet.pParams;
    packet.pData = packet.pParams + 2;
    packet.bufferUsed += 2;
    packet.pSMBHeader->wordCount = 0;   /* No parameter words */

    wcstowc16s(nativeOS, L"Unix", sizeof(nativeOS));
    wcstowc16s(nativeLanMan, L"Likewise SMB", sizeof(nativeLanMan));
    wcstowc16s(nativeDomain, L"WORKGROUP", sizeof(nativeDomain));

    /* @todo: handle buffer size restart with ERESTART */
    ntStatus = MarshallNegotiateRequest(
                    packet.pData,
                    packet.bufferLen - packet.bufferUsed,
                    &packetByteCount,
                    pszDialects,
                    1);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(packetByteCount <= UINT16_MAX);
    *packet.pByteCount = (uint16_t) packetByteCount;
    packet.bufferUsed += *packet.pByteCount;

    ntStatus = SMBPacketMarshallFooter(&packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSocketSend(pSocket, &packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSocketReceiveNegotiateResponse(
                    pSocket,
                    &pResponsePacket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = UnmarshallNegotiateResponse(
                    pResponsePacket->pParams,
                    pResponsePacket->bufferLen - pResponsePacket->bufferUsed,
                    &pHeader,
                    &pGUID,
                    &pSecurityBlob,
                    &securityBlobLen);
    BAIL_ON_NT_STATUS(ntStatus);

    /* This backlock is safe because the session hash is unlocked. */
    SMB_LOCK_MUTEX(bSocketLocked, &pSocket->mutex);

    pSocket->maxBufferSize = pHeader->maxBufferSize;
    pSocket->maxRawSize = pHeader->maxRawSize;
    pSocket->sessionKey = pHeader->sessionKey;
    pSocket->capabilities = pHeader->capabilities;
    pSocket->securityMode = (pHeader->securityMode & 0x1) ? SMB_SECURITY_MODE_USER : SMB_SECURITY_MODE_SHARE;
    pSocket->bPasswordsMustBeEncrypted = (pHeader->securityMode & 0x2) ? TRUE : FALSE;
    pSocket->bSignedMessagesSupported = (pHeader->securityMode & 0x4) ? TRUE : FALSE;
    pSocket->bSignedMessagesRequired = (pHeader->securityMode & 0x8) ? TRUE : FALSE;

    if (pHeader->maxMpxCount)
    {
        if (sem_init(&pSocket->semMpx, FALSE, pHeader->maxMpxCount) < 0)
        {
            ntStatus = errno;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        pSocket->maxMpxCount = pHeader->maxMpxCount;
    }

    pSocket->securityBlobLen = securityBlobLen;

    ntStatus = SMBAllocateMemory(
                    pSocket->securityBlobLen,
                    (PVOID *) &pSocket->pSecurityBlob);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(pSocket->pSecurityBlob,
           pSecurityBlob,
           pSocket->securityBlobLen);

cleanup:

    SMB_UNLOCK_MUTEX(bSocketLocked, &pSocket->mutex);

    if (pResponsePacket)
    {
        SMBPacketFree(pSocket, pResponsePacket);
    }

    if (packet.bufferLen)
    {
        SMBPacketBufferFree(
                pSocket->hPacketAllocator,
                packet.pRawBuffer,
                packet.bufferLen);
    }

    return ntStatus;

error:

    if (pSocket && pSocket->maxMpxCount)
    {
        if (sem_destroy(&pSocket->semMpx) < 0)
        {
            SMB_LOG_ERROR("Failed to destroy semaphore [code: %d]", errno);
        }
    }

    goto cleanup;
}
