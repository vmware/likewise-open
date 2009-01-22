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
 *        session_setup.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        SMB Packet Marshalling
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

static uchar8_t smbMagic[4] = { 0xFF, 'S', 'M', 'B' };

BOOLEAN
SMBIsAndXCommand(
    uint8_t command
    )
{
    BOOLEAN bResult = FALSE;

    switch(command)
    {
        case COM_LOCKING_ANDX:
        case COM_OPEN_ANDX:
        case COM_READ_ANDX:
        case COM_WRITE_ANDX:
        case COM_SESSION_SETUP_ANDX:
        case COM_LOGOFF_ANDX:
        case COM_TREE_CONNECT_ANDX:
        case COM_NT_CREATE_ANDX:
        {
            bResult = TRUE;
            break;
        }
    }

    return bResult;
}

NTSTATUS
SMBPacketCreateAllocator(
    ULONG   ulNumMaxPackets,
    PHANDLE phPacketAllocator
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_PACKET_ALLOCATOR pPacketAllocator = NULL;

    if (!ulNumMaxPackets || ulNumMaxPackets < 10)
    {
        ulNumMaxPackets = 10;
    }

    ntStatus = SMBAllocateMemory(
                    sizeof(LWIO_PACKET_ALLOCATOR),
                    (PVOID*)&pPacketAllocator);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pPacketAllocator->mutex, NULL);
    pPacketAllocator->pMutex = &pPacketAllocator->mutex;
    pPacketAllocator->ulNumMaxPackets = ulNumMaxPackets;

    *phPacketAllocator = (HANDLE)pPacketAllocator;

cleanup:

    return ntStatus;

error:

    *phPacketAllocator = NULL;

    goto cleanup;
}

NTSTATUS
SMBPacketAllocate(
    HANDLE       hPacketAllocator,
    PSMB_PACKET* ppPacket
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_PACKET_ALLOCATOR pPacketAllocator = NULL;
    BOOLEAN bInLock = FALSE;
    PSMB_PACKET pPacket = NULL;

    pPacketAllocator = (PLWIO_PACKET_ALLOCATOR) hPacketAllocator;

    SMB_LOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    if (pPacketAllocator->pFreePacketStack)
    {
        pPacket = (PSMB_PACKET) pPacketAllocator->pFreePacketStack;

        SMBStackPopNoFree(&pPacketAllocator->pFreePacketStack);

        pPacketAllocator->freePacketCount--;
    }
    else
    {
        ntStatus = SMBAllocateMemory(
                        sizeof(SMB_PACKET),
                        (PVOID *) &pPacket);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppPacket = pPacket;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    return ntStatus;

error:

    *ppPacket = NULL;

    goto cleanup;
}

VOID
SMBPacketFree(
    HANDLE      hPacketAllocator,
    PSMB_PACKET pPacket
    )
{
    PLWIO_PACKET_ALLOCATOR pPacketAllocator = NULL;
    BOOLEAN bInLock = FALSE;

    SMBPacketBufferFree(
                hPacketAllocator,
                pPacket->pRawBuffer,
                pPacket->bufferLen);

    pPacketAllocator = (PLWIO_PACKET_ALLOCATOR)hPacketAllocator;

    SMB_LOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    /* If the len is greater than our current allocator len, adjust */
    /* @todo: make free list configurable */
    if (pPacketAllocator->freePacketCount < pPacketAllocator->ulNumMaxPackets)
    {
        assert(sizeof(SMB_PACKET) > sizeof(SMB_STACK));
        SMBStackPushNoAlloc(&pPacketAllocator->pFreePacketStack, (PSMB_STACK) pPacket);
        pPacketAllocator->freePacketCount++;
    }
    else
    {
        SMBFreeMemory(pPacket);
    }

    SMB_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);
}

NTSTATUS
SMBPacketBufferAllocate(
    HANDLE      hPacketAllocator,
    size_t      len,
    uint8_t**   ppBuffer,
    size_t*     pAllocatedLen
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_PACKET_ALLOCATOR pPacketAllocator = NULL;
    BOOLEAN bInLock = FALSE;

    pPacketAllocator = (PLWIO_PACKET_ALLOCATOR)hPacketAllocator;

    SMB_LOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    /* If the len is greater than our current allocator len, adjust */
    if (len > pPacketAllocator->freeBufferLen)
    {
        SMBStackFree(pPacketAllocator->pFreeBufferStack);
        pPacketAllocator->pFreeBufferStack = NULL;

        pPacketAllocator->freeBufferLen = len;
    }

    if (pPacketAllocator->pFreeBufferStack)
    {
        *ppBuffer = (uint8_t *) pPacketAllocator->pFreeBufferStack;
        *pAllocatedLen = pPacketAllocator->freeBufferLen;
        SMBStackPopNoFree(&pPacketAllocator->pFreeBufferStack);
        memset(*ppBuffer, 0, *pAllocatedLen);
        pPacketAllocator->freeBufferCount--;
    }
    else
    {
        ntStatus = SMBAllocateMemory(
                        pPacketAllocator->freeBufferLen,
                        (PVOID *) ppBuffer);
        BAIL_ON_NT_STATUS(ntStatus);

        *pAllocatedLen = pPacketAllocator->freeBufferLen;
    }

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    return ntStatus;

error:

    goto cleanup;
}

VOID
SMBPacketBufferFree(
    HANDLE      hPacketAllocator,
    uint8_t*    pBuffer,
    size_t      bufferLen
    )
{
    BOOLEAN bInLock = FALSE;
    PLWIO_PACKET_ALLOCATOR pPacketAllocator = NULL;

    pPacketAllocator = (PLWIO_PACKET_ALLOCATOR)hPacketAllocator;

    SMB_LOCK_MUTEX(bInLock, &pPacketAllocator->mutex);

    /* If the len is greater than our current allocator len, adjust */
    /* @todo: make free list configurable */
    if (bufferLen == pPacketAllocator->freeBufferLen &&
        pPacketAllocator->freeBufferCount < pPacketAllocator->ulNumMaxPackets)
    {
        assert(bufferLen > sizeof(SMB_STACK));

        SMBStackPushNoAlloc(&pPacketAllocator->pFreeBufferStack, (PSMB_STACK) pBuffer);

        pPacketAllocator->freeBufferCount++;
    }
    else
    {
        SMBFreeMemory(pBuffer);
    }

    SMB_UNLOCK_MUTEX(bInLock, &pPacketAllocator->mutex);
}

VOID
SMBPacketResetBuffer(
    PSMB_PACKET pPacket
    )
{
    if (pPacket->pRawBuffer && pPacket->bufferLen)
    {
        memset(pPacket->pRawBuffer, 0, pPacket->bufferLen);
    }
}

VOID
SMBPacketFreeAllocator(
    HANDLE hPacketAllocator
    )
{
    PLWIO_PACKET_ALLOCATOR pAllocator = (PLWIO_PACKET_ALLOCATOR)hPacketAllocator;

    if (pAllocator->pMutex)
    {
        pthread_mutex_destroy(&pAllocator->mutex);
        pAllocator->pMutex = NULL;
    }

    if (pAllocator->pFreeBufferStack)
    {
        SMBStackFree(pAllocator->pFreeBufferStack);
    }

    if (pAllocator->pFreePacketStack)
    {
        SMBStackFree(pAllocator->pFreePacketStack);
    }

    SMBFreeMemory(pAllocator);
}

/* @todo: support AndX */
/* @todo: support signing */
/* @todo: support endian swapping */
NTSTATUS
SMBPacketMarshallHeader(
    uint8_t    *pBuffer,
    uint32_t    bufferLen,
    uint8_t     command,
    uint32_t    error,
    uint32_t    isResponse,
    uint16_t    tid,
    uint32_t    pid,
    uint16_t    uid,
    uint16_t    mid,
    BOOLEAN     bSignMessages,
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    uint32_t bufferUsed = 0;
    uint32_t len = sizeof(NETBIOS_HEADER);

    if(bufferUsed + len <= bufferLen)
    {
       pPacket->pNetBIOSHeader = (NETBIOS_HEADER*) pBuffer;
    }
    bufferUsed += len;

    len = sizeof(SMB_HEADER);
    if(bufferUsed + len <= bufferLen)
    {
        SMB_HEADER* pHeader = pPacket->pSMBHeader =
            (SMB_HEADER *) (pBuffer + bufferUsed);
        memcpy(&pPacket->pSMBHeader->smb, smbMagic, sizeof(smbMagic));
        pHeader->command = command;
        pHeader->error = error;
        pHeader->flags = isResponse ? FLAG_RESPONSE : 0;
        pHeader->flags |= FLAG_CASELESS_PATHS | FLAG_OBSOLETE_2;
        pHeader->flags2 = (isResponse ? 0 : FLAG2_KNOWS_LONG_NAMES) |
            (isResponse ? 0 : FLAG2_IS_LONG_NAME) |
            (bSignMessages ? FLAG2_SECURITY_SIG : 0) |
            FLAG2_KNOWS_EAS |
            FLAG2_EXT_SEC | FLAG2_ERR_STATUS | FLAG2_UNICODE;
        pHeader->extra.pidHigh = pid >> 16;
        memset(pHeader->extra.securitySignature, 0,
            sizeof(pHeader->extra.securitySignature));
        pHeader->pad[5] = 0;
        pHeader->tid = tid;
        pHeader->pid = pid;
        pHeader->uid = uid;
        pHeader->mid = mid;
    }
    bufferUsed += len;

    if(SMBIsAndXCommand(command))
    {
        len = sizeof(ANDX_HEADER);
        if(bufferUsed + len <= bufferLen)
        {
            pPacket->pAndXHeader = (ANDX_HEADER *) (pBuffer + bufferUsed);
            pPacket->pAndXHeader->andXCommand = 0xFF;
            pPacket->pAndXHeader->andXOffset = 0;
            pPacket->pAndXHeader->andXReserved = 0;
        }
        bufferUsed += len;
    }
    else
    {
        pPacket->pAndXHeader = NULL;
    }

    if(bufferUsed <= bufferLen)
    {
        pPacket->pParams = pBuffer + bufferUsed;
    }

    pPacket->pData = NULL;
    pPacket->pByteCount = NULL;
    pPacket->bufferLen = bufferLen;
    pPacket->bufferUsed = bufferUsed;

    if (bufferUsed > bufferLen)
    {
        ntStatus = EMSGSIZE;
    }

    return ntStatus;
}

NTSTATUS
SMBPacketMarshallFooter(
    PSMB_PACKET pPacket
    )
{
    pPacket->pNetBIOSHeader->len = htonl(pPacket->bufferUsed - sizeof(NETBIOS_HEADER));

    return 0;
}

BOOLEAN
SMBPacketIsSigned(
    PSMB_PACKET pPacket
    )
{
    return (pPacket->pSMBHeader->flags2 & FLAG2_SECURITY_SIG);
}

NTSTATUS
SMBPacketVerifySignature(
    PSMB_PACKET pPacket,
    ULONG       ulExpectedSequence,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;
    uint8_t digest[16];
    uint8_t origSignature[8];
    MD5_CTX md5Value;

    assert (sizeof(origSignature) == sizeof(pPacket->pSMBHeader->extra.securitySignature));

    memcpy(origSignature, pPacket->pSMBHeader->extra.securitySignature, sizeof(pPacket->pSMBHeader->extra.securitySignature));
    memset(&pPacket->pSMBHeader->extra.securitySignature[0], 0, sizeof(pPacket->pSMBHeader->extra.securitySignature));
    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &ulExpectedSequence, sizeof(ulExpectedSequence));

    MD5_Init(&md5Value);

    if (pSessionKey)
    {
        MD5_Update(&md5Value, pSessionKey, ulSessionKeyLength);
    }

    MD5_Update(&md5Value, (PBYTE)pPacket->pSMBHeader, pPacket->pNetBIOSHeader->len);
    MD5_Final(digest, &md5Value);

    if (memcmp(&origSignature[0], &digest[0], sizeof(origSignature)))
    {
        ntStatus = EACCES;
    }

    // restore signature
    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &origSignature[8], sizeof(origSignature));

    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    SMB_LOG_WARNING("SMB Packet verification failed [code:%d]", ntStatus);

    goto cleanup;
}

NTSTATUS
SMBPacketSign(
    PSMB_PACKET pPacket,
    ULONG       ulSequence,
    PBYTE       pSessionKey,
    ULONG       ulSessionKeyLength
    )
{
    NTSTATUS ntStatus = 0;
    uint8_t digest[16];
    MD5_CTX md5Value;

    memset(&pPacket->pSMBHeader->extra.securitySignature[0], 0, sizeof(pPacket->pSMBHeader->extra.securitySignature));
    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &ulSequence, sizeof(ulSequence));

    MD5_Init(&md5Value);

    if (pSessionKey)
    {
        MD5_Update(&md5Value, pSessionKey, ulSessionKeyLength);
    }

    MD5_Update(&md5Value, (PBYTE)pPacket->pSMBHeader, ntohl(pPacket->pNetBIOSHeader->len));
    MD5_Final(digest, &md5Value);

    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &digest[0], sizeof(pPacket->pSMBHeader->extra.securitySignature));

    return ntStatus;
}


