/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        session_setup.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
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

static
uint32_t
IsAndXCommand(
    uint8_t command
    )
{
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
            return true;
    }

    return false;
}

/* @todo: support AndX */
/* @todo: support signing */
/* @todo: support endian swapping */
DWORD
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
    DWORD dwError = 0;
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

    if(IsAndXCommand(command))
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
        dwError = EMSGSIZE;
    }

    return dwError;
}

DWORD
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

DWORD
SMBPacketVerifySignature(
    PSMB_PACKET pPacket,
    DWORD       dwExpectedSequence,
    PBYTE       pSessionKey,
    DWORD       dwSessionKeyLength
    )
{
    DWORD dwError = 0;
    uint8_t digest[16];
    uint8_t origSignature[8];
    MD5_CTX md5Value;

    assert (sizeof(origSignature) == sizeof(pPacket->pSMBHeader->extra.securitySignature));

    memcpy(origSignature, pPacket->pSMBHeader->extra.securitySignature, sizeof(pPacket->pSMBHeader->extra.securitySignature));
    memset(&pPacket->pSMBHeader->extra.securitySignature[0], 0, sizeof(pPacket->pSMBHeader->extra.securitySignature));
    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &dwExpectedSequence, sizeof(dwExpectedSequence));

    MD5_Init(&md5Value);

    if (pSessionKey)
    {
        MD5_Update(&md5Value, pSessionKey, dwSessionKeyLength);
    }

    MD5_Update(&md5Value, (PBYTE)pPacket->pSMBHeader, pPacket->pNetBIOSHeader->len);
    MD5_Final(digest, &md5Value);

    if (memcmp(&origSignature[0], &digest[0], sizeof(origSignature)))
    {
        dwError = EACCES;
    }

    // restore signature
    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &origSignature[8], sizeof(origSignature));

    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    return dwError;

error:

    SMB_LOG_WARNING("SMB Packet verification failed [code:%d]", dwError);

    goto cleanup;
}

DWORD
SMBPacketSign(
    PSMB_PACKET pPacket,
    DWORD       dwSequence,
    PBYTE       pSessionKey,
    DWORD       dwSessionKeyLength
    )
{
    DWORD dwError = 0;
    uint8_t digest[16];
    MD5_CTX md5Value;

    memset(&pPacket->pSMBHeader->extra.securitySignature[0], 0, sizeof(pPacket->pSMBHeader->extra.securitySignature));
    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &dwSequence, sizeof(dwSequence));

    MD5_Init(&md5Value);

    if (pSessionKey)
    {
        MD5_Update(&md5Value, pSessionKey, dwSessionKeyLength);
    }

    MD5_Update(&md5Value, (PBYTE)pPacket->pSMBHeader, ntohl(pPacket->pNetBIOSHeader->len));
    MD5_Final(digest, &md5Value);

    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &digest[0], sizeof(pPacket->pSMBHeader->extra.securitySignature));

    return dwError;
}

DWORD
SMBPacketSend(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    /* @todo: signal handling */
    DWORD dwError = 0;
    ssize_t  writtenLen = 0;
    BOOLEAN  bInLock = FALSE;
    BOOLEAN  bSemaphoreAcquired = FALSE;

    if (pSocket->maxMpxCount)
    {
        if (sem_wait(&pSocket->semMpx) < 0)
        {
            dwError = errno;
            BAIL_ON_SMB_ERROR(dwError);
        }

        bSemaphoreAcquired = TRUE;
    }

    SMB_LOCK_MUTEX(bInLock, &pSocket->writeMutex);

    writtenLen = write(
                    pSocket->fd,
                    pPacket->pRawBuffer,
                    pPacket->bufferUsed);

    if (writtenLen < 0)
    {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->writeMutex);

    return dwError;

error:

    if (bSemaphoreAcquired)
    {
        if (sem_post(&pSocket->semMpx) < 0)
        {
            SMB_LOG_ERROR("Failed to post semaphore [code: %d]", errno);
        }
    }

    goto cleanup;
}

DWORD
SMBPacketReceiveAndUnmarshall(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    DWORD dwError = 0;
    /* @todo: handle timeouts, signals, buffer overflow */
    /* This logic would need to be modified for zero copy */
    uint32_t len = sizeof(NETBIOS_HEADER);
    uint32_t readLen = 0;
    uint32_t bufferUsed = 0;

    /* @todo: support read threads in the daemonized case */
    dwError = SMBSocketRead(pSocket, pPacket->pRawBuffer, len, &readLen);
    BAIL_ON_SMB_ERROR(dwError);

    if (len != readLen)
    {
        dwError = EPIPE;
        BAIL_ON_SMB_ERROR(dwError);
    }

    pPacket->pNetBIOSHeader = (NETBIOS_HEADER *) pPacket->pRawBuffer;
    bufferUsed += len;

    pPacket->pNetBIOSHeader->len = ntohl(pPacket->pNetBIOSHeader->len);

    dwError = SMBSocketRead(
                    pSocket,
                    pPacket->pRawBuffer + bufferUsed,
                    pPacket->pNetBIOSHeader->len,
                    &readLen);
    BAIL_ON_SMB_ERROR(dwError);

    if(pPacket->pNetBIOSHeader->len != readLen)
    {
        dwError = EPIPE;
        BAIL_ON_SMB_ERROR(dwError);
    }

    pPacket->pSMBHeader = (SMB_HEADER *) (pPacket->pRawBuffer + bufferUsed);
    bufferUsed += sizeof(SMB_HEADER);

    if (IsAndXCommand(pPacket->pSMBHeader->command))
    {
        pPacket->pAndXHeader = (ANDX_HEADER *)
            (pPacket->pSMBHeader + bufferUsed);
        bufferUsed += sizeof(ANDX_HEADER);
    }

    pPacket->pParams = pPacket->pRawBuffer + bufferUsed;
    pPacket->pData = NULL;
    pPacket->bufferUsed = bufferUsed;

error:

    return dwError;
}


