#include "includes.h"

// Rules:
//
// Only one reader thread can read from this socket
// Multiple writers can write to the socket, in a synchronized manner per message
// Both readers and writers can set the connection state to be invalid
// The file descriptor is set to -1 only by the reader thread (not the writers)
// The file descriptor is closed only when the connection object is freed.

static
NTSTATUS
SrvConnectionReadMessage(
    PLWIO_SRV_SOCKET pSocket,
    size_t          sBytesToRead,
    size_t          sOffset,
    PSMB_PACKET     pPacket,
    size_t*         psNumBytesRead
    );

int
SrvConnectionGetFd(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    int fd = -1;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    if (pConnection->pSocket)
    {
        pthread_mutex_lock(&pConnection->pSocket->mutex);

        fd = pConnection->pSocket->fd;

        pthread_mutex_unlock(&pConnection->pSocket->mutex);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return fd;
}

NTSTATUS
SrvConnectionGetNextSequence(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PULONG              pulRequestSequence
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulRequestSequence = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    switch (pSmbRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:
        case COM_ECHO:

            break;

        case COM_NT_CANCEL:

            ulRequestSequence = pConnection->ulSequence++;

            break;

        default:

            ulRequestSequence = pConnection->ulSequence++;

            pConnection->ulSequence++; // Response

            break;
    }

    *pulRequestSequence = ulRequestSequence;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;
}

NTSTATUS
SrvConnectionReadPacket(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET*        ppPacket
    )
{
    NTSTATUS ntStatus = 0;

    if (!pConnection->readerState.pRequestPacket)
    {
        ntStatus = SMBPacketAllocate(
                        pConnection->hPacketAllocator,
                        &pConnection->readerState.pRequestPacket);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBPacketBufferAllocate(
                        pConnection->hPacketAllocator,
                        64 * 1024,
                        &pConnection->readerState.pRequestPacket->pRawBuffer,
                        &pConnection->readerState.pRequestPacket->bufferLen);
        BAIL_ON_NT_STATUS(ntStatus);

        pConnection->readerState.bReadHeader = TRUE;
        pConnection->readerState.sNumBytesToRead = sizeof(NETBIOS_HEADER);
        pConnection->readerState.sOffset = 0;
    }

    if (pConnection->readerState.bReadHeader)
    {
        size_t sNumBytesRead = 0;

        ntStatus = SrvConnectionReadMessage(
                        pConnection->pSocket,
                        pConnection->readerState.sNumBytesToRead,
                        pConnection->readerState.sOffset,
                        pConnection->readerState.pRequestPacket,
                        &sNumBytesRead);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!sNumBytesRead)
        {
            // peer reset connection
            SrvConnectionSetInvalid(pConnection);
            ntStatus = STATUS_CONNECTION_RESET;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pConnection->readerState.sNumBytesToRead -= sNumBytesRead;
        pConnection->readerState.sOffset += sNumBytesRead;

        if (!pConnection->readerState.sNumBytesToRead)
        {
            PSMB_PACKET pPacket = pConnection->readerState.pRequestPacket;

            pConnection->readerState.bReadHeader = FALSE;

            pPacket->pNetBIOSHeader = (NETBIOS_HEADER *) pPacket->pRawBuffer;
            pPacket->pNetBIOSHeader->len = ntohl(pPacket->pNetBIOSHeader->len);

            pConnection->readerState.sNumBytesToRead = pPacket->pNetBIOSHeader->len;

            // check if the message fits in our currently allocated buffer
            if (pConnection->readerState.sNumBytesToRead > (pPacket->bufferLen - sizeof(NETBIOS_HEADER)))
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }
    }

    if (!pConnection->readerState.bReadHeader &&
         pConnection->readerState.sNumBytesToRead)
    {
        size_t sNumBytesRead = 0;

        ntStatus = SrvConnectionReadMessage(
                        pConnection->pSocket,
                        pConnection->readerState.sNumBytesToRead,
                        pConnection->readerState.sOffset,
                        pConnection->readerState.pRequestPacket,
                        &sNumBytesRead);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!sNumBytesRead)
        {
            // peer reset connection
            SrvConnectionSetInvalid(pConnection);
            ntStatus = STATUS_CONNECTION_RESET;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pConnection->readerState.sNumBytesToRead -= sNumBytesRead;
        pConnection->readerState.sOffset += sNumBytesRead;
    }

    // Packet is complete
    if (!pConnection->readerState.bReadHeader &&
        !pConnection->readerState.sNumBytesToRead)
    {
        PSMB_PACKET pPacket = pConnection->readerState.pRequestPacket;
        size_t bufferUsed = sizeof(NETBIOS_HEADER);

        pPacket->pSMBHeader = (SMB_HEADER *)(pPacket->pRawBuffer + bufferUsed);
        bufferUsed += sizeof(SMB_HEADER);

        if (SMBIsAndXCommand(pPacket->pSMBHeader->command))
        {
            pPacket->pAndXHeader = (ANDX_HEADER *)(pPacket->pSMBHeader + bufferUsed);
            bufferUsed += sizeof(ANDX_HEADER);
        }

        pPacket->pParams = pPacket->pRawBuffer + bufferUsed;
        pPacket->pData = NULL;
        pPacket->bufferUsed = bufferUsed;

        *ppPacket = pConnection->readerState.pRequestPacket;

        pConnection->readerState.pRequestPacket = NULL;
    }

cleanup:

    return ntStatus;

error:

    *ppPacket = NULL;

    SrvConnectionSetInvalid(pConnection);

    goto cleanup;
}

NTSTATUS
SrvConnectionWriteMessage(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG               ulSequence,
    PSMB_PACKET         pPacket
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    size_t   sNumBytesToWrite = pPacket->bufferUsed;
    size_t   sTotalNumBytesWritten = 0;
    int fd = -1;

    if (pConnection->serverProperties.bRequireSecuritySignatures &&
        pConnection->pSessionKey)
    {
        pPacket->pSMBHeader->flags2 |= FLAG2_SECURITY_SIG;

        ntStatus = SMBPacketSign(
                        pPacket,
                        ulSequence,
                        pConnection->pSessionKey,
                        pConnection->ulSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    fd = SrvConnectionGetFd(pConnection);

    LWIO_LOCK_MUTEX(bInLock, &pConnection->pSocket->mutex);

    // TODO: Use select to find out if the fd is ready to be written to

    while (sNumBytesToWrite)
    {
        ssize_t sNumBytesWritten = 0;

        sNumBytesWritten = write(
                                fd,
                                pPacket->pRawBuffer + sTotalNumBytesWritten,
                                sNumBytesToWrite);
        if (sNumBytesWritten < 0)
        {
            int unixErrno = errno;

            if ((unixErrno == EINTR) || (unixErrno == EAGAIN))
            {
                continue;
            }

            ntStatus = LwUnixErrnoToNtStatus(unixErrno);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        sNumBytesToWrite -= sNumBytesWritten;
        sTotalNumBytesWritten += sNumBytesWritten;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pConnection->pSocket->mutex);

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &pConnection->pSocket->mutex);

    SrvConnectionSetInvalid(pConnection);

    goto cleanup;
}


static
NTSTATUS
SrvConnectionReadMessage(
    PLWIO_SRV_SOCKET pSocket,
    size_t          sBytesToRead,
    size_t          sOffset,
    PSMB_PACKET     pPacket,
    size_t*         psNumBytesRead
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t  sNumBytesRead = 0;
    BOOLEAN  bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    do
    {
        sNumBytesRead = read(pSocket->fd, pPacket->pRawBuffer + sOffset, sBytesToRead);
        if (sNumBytesRead < 0)
        {
            if ((errno != EAGAIN) && (errno != EINTR))
            {
                ntStatus = LwUnixErrnoToNtStatus(errno);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }

    } while (sNumBytesRead < 0);

    *psNumBytesRead = sNumBytesRead;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    *psNumBytesRead = 0;

    goto cleanup;
}

