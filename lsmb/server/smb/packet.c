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

static
DWORD
ConsumeBuffer(
    IN PVOID pBuffer,
    IN uint32_t BufferLength,
    IN OUT uint32_t* BufferLengthUsed,
    IN uint32_t BytesNeeded
    )
{
    DWORD dwError = 0;

    if (*BufferLengthUsed + BytesNeeded > BufferLength)
    {
        dwError = EMSGSIZE;
    }
    else
    {
        *BufferLengthUsed += BytesNeeded;
    }

    return dwError;
}

static
VOID
SMBPacketHTOLSmbHeader(
    IN OUT SMB_HEADER* pHeader
    )
{
    SMB_HTOL32_INPLACE(pHeader->error);
    SMB_HTOL16_INPLACE(pHeader->flags2);
    SMB_HTOL16_INPLACE(pHeader->extra.pidHigh);
    SMB_HTOL16_INPLACE(pHeader->tid);
    SMB_HTOL16_INPLACE(pHeader->pid);
    SMB_HTOL16_INPLACE(pHeader->uid);
    SMB_HTOL16_INPLACE(pHeader->mid);
}

static
VOID
SMBPacketLTOHSmbHeader(
    IN OUT SMB_HEADER* pHeader
    )
{
    SMB_LTOH32_INPLACE(pHeader->error);
    SMB_LTOH16_INPLACE(pHeader->flags2);
    SMB_LTOH16_INPLACE(pHeader->extra.pidHigh);
    SMB_LTOH16_INPLACE(pHeader->tid);
    SMB_LTOH16_INPLACE(pHeader->pid);
    SMB_LTOH16_INPLACE(pHeader->uid);
    SMB_LTOH16_INPLACE(pHeader->mid);
}

/* @todo: support AndX */
/* @todo: support signing */
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
    BOOLEAN     bCommandAllowsSignature,
    PSMB_PACKET pPacket
    )
{
    DWORD dwError = 0;
    uint32_t bufferUsed = 0;
    SMB_HEADER* pHeader = NULL;

    pPacket->allowSignature = bCommandAllowsSignature;

    pPacket->pNetBIOSHeader = (NETBIOS_HEADER *) (pBuffer + bufferUsed);

    dwError = ConsumeBuffer(pBuffer, bufferLen, &bufferUsed, sizeof(NETBIOS_HEADER));
    BAIL_ON_SMB_ERROR(dwError);

    pPacket->pSMBHeader = (SMB_HEADER *) (pBuffer + bufferUsed);

    dwError = ConsumeBuffer(pBuffer, bufferLen, &bufferUsed, sizeof(SMB_HEADER));
    BAIL_ON_SMB_ERROR(dwError);

    pHeader = pPacket->pSMBHeader;
    memcpy(&pHeader->smb, smbMagic, sizeof(smbMagic));
    pHeader->command = command;
    pHeader->error = error;
    pHeader->flags = isResponse ? FLAG_RESPONSE : 0;
    pHeader->flags |= FLAG_CASELESS_PATHS | FLAG_OBSOLETE_2;
    pHeader->flags2 = ((isResponse ? 0 : FLAG2_KNOWS_LONG_NAMES) |
                       (isResponse ? 0 : FLAG2_IS_LONG_NAME) |
                       FLAG2_KNOWS_EAS | FLAG2_EXT_SEC |
                       FLAG2_ERR_STATUS | FLAG2_UNICODE);
    memset(pHeader->pad, 0, sizeof(pHeader->pad));
    pHeader->extra.pidHigh = pid >> 16;
    pHeader->tid = tid;
    pHeader->pid = pid;
    pHeader->uid = uid;
    pHeader->mid = mid;

    if (IsAndXCommand(command))
    {
        pPacket->pAndXHeader = (ANDX_HEADER *) (pBuffer + bufferUsed);

        dwError = ConsumeBuffer(pBuffer, bufferLen, &bufferUsed, sizeof(ANDX_HEADER));
        BAIL_ON_SMB_ERROR(dwError);

        pPacket->pAndXHeader->andXCommand = 0xFF;
        pPacket->pAndXHeader->andXOffset = 0;
        pPacket->pAndXHeader->andXReserved = 0;
    }
    else
    {
        pPacket->pAndXHeader = NULL;
    }

    pPacket->pParams = pBuffer + bufferUsed;
    pPacket->pData = NULL;
    pPacket->pByteCount = NULL;
    pPacket->bufferLen = bufferLen;
    pPacket->bufferUsed = bufferUsed;

    assert(bufferUsed <= bufferLen);

cleanup:
    return dwError;

error:
    pPacket->pNetBIOSHeader = NULL;
    pPacket->pSMBHeader = NULL;
    pPacket->pAndXHeader = NULL;
    pPacket->pParams = NULL;
    pPacket->pData = NULL;
    pPacket->pByteCount = NULL;
    pPacket->bufferLen = bufferLen;
    pPacket->bufferUsed = 0;

    goto cleanup;
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
    uint32_t littleEndianSequence = SMB_HTOL32(dwExpectedSequence);

    assert (sizeof(origSignature) == sizeof(pPacket->pSMBHeader->extra.securitySignature));

    memcpy(origSignature, pPacket->pSMBHeader->extra.securitySignature, sizeof(pPacket->pSMBHeader->extra.securitySignature));
    memset(&pPacket->pSMBHeader->extra.securitySignature[0], 0, sizeof(pPacket->pSMBHeader->extra.securitySignature));
    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &littleEndianSequence, sizeof(littleEndianSequence));

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
SMBPacketDecodeHeader(
    IN OUT PSMB_PACKET pPacket,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    IN OPTIONAL PBYTE pSessionKey,
    IN DWORD dwSessionKeyLength
    )
{
    DWORD dwError = 0;

    if (bVerifySignature)
    {
        dwError = SMBPacketVerifySignature(
                        pPacket,
                        dwExpectedSequence,
                        pSessionKey,
                        dwSessionKeyLength);
        BAIL_ON_SMB_ERROR(dwError);
    }

    SMBPacketLTOHSmbHeader(pPacket->pSMBHeader);

error:
    return dwError;
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
    uint32_t littleEndianSequence = SMB_HTOL32(dwSequence);

    memset(&pPacket->pSMBHeader->extra.securitySignature[0], 0, sizeof(pPacket->pSMBHeader->extra.securitySignature));
    memcpy(&pPacket->pSMBHeader->extra.securitySignature[0], &littleEndianSequence, sizeof(littleEndianSequence));

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
    IN PSMB_SOCKET pSocket,
    IN OUT PSMB_PACKET pPacket
    )
{
    /* @todo: signal handling */
    DWORD dwError = 0;
    ssize_t  writtenLen = 0;
    BOOLEAN  bInLock = FALSE;
    BOOLEAN  bSemaphoreAcquired = FALSE;
    BOOLEAN bIsSignatureRequired = FALSE;

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

    if (pPacket->pSMBHeader->command != COM_NEGOTIATE)
    {
        pPacket->sequence = SMBSocketGetNextSequence(pSocket);
    }

    if (pPacket->allowSignature)
    {
        bIsSignatureRequired = SMBSocketIsSignatureRequired(pSocket);
    }

    if (bIsSignatureRequired)
    {
        pPacket->pSMBHeader->flags2 |= FLAG2_SECURITY_SIG;
    }

    SMBPacketHTOLSmbHeader(pPacket->pSMBHeader);

    if (bIsSignatureRequired)
    {
        dwError = SMBPacketSign(
                        pPacket,
                        pPacket->sequence,
                        pSocket->pSessionKey,
                        pSocket->dwSessionKeyLength);
        BAIL_ON_SMB_ERROR(dwError);
    }

    pPacket->haveSignature = bIsSignatureRequired;

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
    IN PSMB_SOCKET pSocket,
    OUT PSMB_PACKET pPacket
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

    if (IsAndXCommand(SMB_LTOH8(pPacket->pSMBHeader->command)))
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

DWORD
SMBPacketAppendUnicodeString(
    OUT uint8_t* pBuffer,
    IN ULONG BufferLength,
    IN OUT PULONG BufferUsed,
    IN const wchar16_t* pwszString
    )
{
    DWORD dwError = 0;
    ULONG bytesNeeded = 0;
    ULONG bufferUsed = *BufferUsed;
    wchar16_t* pOutputBuffer = NULL;
    size_t writeLength = 0;

    bytesNeeded = sizeof(pwszString[0]) * (wc16slen(pwszString) + 1);
    if (bufferUsed + bytesNeeded > BufferLength)
    {
        dwError = EMSGSIZE;
        BAIL_ON_SMB_ERROR(dwError);
    }

    pOutputBuffer = (wchar16_t *) (pBuffer + bufferUsed);
    writeLength = wc16stowc16les(pOutputBuffer, pwszString, bytesNeeded / sizeof(pwszString[0]));
    // Verify that expected write length was returned.  Note that the
    // returned length does not include the NULL though the NULL gets
    // written out.
    if (writeLength == (size_t) -1)
    {
        dwError = EMSGSIZE;
        BAIL_ON_SMB_ERROR(dwError);
    }
    else if (((writeLength + 1) * sizeof(wchar16_t)) != bytesNeeded)
    {
        dwError = EMSGSIZE;
        BAIL_ON_SMB_ERROR(dwError);
    }

    bufferUsed += bytesNeeded;

error:
    *BufferUsed = bufferUsed;
    return dwError;
}

DWORD
SMBPacketAppendString(
    OUT uint8_t* pBuffer,
    IN ULONG BufferLength,
    IN OUT PULONG BufferUsed,
    IN const char* pszString
    )
{
    DWORD dwError = 0;
    ULONG bytesNeeded = 0;
    ULONG bufferUsed = *BufferUsed;
    char* pOutputBuffer = NULL;
    char* pszCursor = NULL;

    bytesNeeded = sizeof(pszString[0]) * (strlen(pszString) + 1);
    if (bufferUsed + bytesNeeded > BufferLength)
    {
        dwError = EMSGSIZE;
        BAIL_ON_SMB_ERROR(dwError);
    }

    pOutputBuffer = (char *) (pBuffer + bufferUsed);

    pszCursor = stpncpy(pOutputBuffer, pszString, bytesNeeded / sizeof(pszString[0]));
    *pszCursor = 0;
    if ((pszCursor - pOutputBuffer) != (bytesNeeded - 1))
    {
        dwError = EMSGSIZE;
        BAIL_ON_SMB_ERROR(dwError);
    }

    bufferUsed += bytesNeeded;

error:
    *BufferUsed = bufferUsed;
    return dwError;
}

