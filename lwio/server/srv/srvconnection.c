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
    PSMB_SRV_SOCKET pSocket,
    PSMB_PACKET     pPacket
    );

NTSTATUS
SrvConnectionCreate(
    PSMB_SRV_SOCKET pSocket,
    HANDLE          hPacketAllocator,
    PSMB_SRV_CONNECTION* ppConnection
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = NULL;

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_SRV_CONNECTION),
                    (PVOID*)&pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->mutex = PTHREAD_MUTEX_INITIALIZER;
    pConnection->refCount = 1;
    pConnection->hPacketAllocator = hPacketAllocator;
    pConnection->state = SMB_SRV_CONN_STATE_INITIAL;
    pConnection->pSocket = pSocket;

    *ppConnection = pConnection;

cleanup:

    return ntStatus;

error:

    *ppConnection = NULL;

    goto cleanup;
}

int
SrvConnectionGetFd(
    PSMB_SRV_CONNECTION pConnection
    )
{
    int fd = -1;

    pthread_mutex_lock(&pConnection->mutex);

    if (pConnection->pSocket)
    {
        pthread_mutex_lock(&pConnection->pSocket->mutex);

        fd = pConnection->pSocket->fd;

        pthread_mutex_unlock(&pConnection->pSocket->mutex);
    }

    pthread_mutex_unlock(&pConnection->mutex);

    return fd;
}

BOOLEAN
SrvConnectionIsInvalid(
    PSMB_SRV_CONNECTION pConnection
    )
{
    BOOLEAN bInvalid = FALSE;

    pthread_mutex_lock(&pConnection->mutex);

    bInvalid = pConnection->state == SMB_SRV_CONN_STATE_INVALID;

    pthread_mutex_unlock(&pConnection->mutex);

    return bInvalid;
}

VOID
SrvConnectionSetInvalid(
    PSMB_SRV_CONNECTION pConnection
    )
{
    pthread_mutex_lock(&pConnection->mutex);

    pConnection->state = SMB_SRV_CONN_STATE_INVALID;

    pthread_mutex_unlock(&pConnection->mutex);
}

NTSTATUS
SrvConnectionReadPacket(
    PSMB_SRV_CONNECTION pConnection,
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
        pConnection->readerState.ulNumBytesToRead = sizeof(NETBIOS_HEADER);
        pConnection->readerState.ulOffset = 0;
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
            goto cleanup;
        }

        pConnection->readerState.sBytesToRead -= sNumBytesRead;
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
                        pConnection->readerState.pPacket,
                        &sNumBytesRead);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!sNumBytesRead)
        {
            // peer reset connection
            SrvConnectionSetInvalid(pConnection);
            goto cleanup;
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
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pPacket
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    size_t   sNumBytesToWrite = pPacket->bufferUsed;
    size_t   sTotalNumBytesWritten = 0;
    int fd = -1;

    fd = SrvConnectionGetFd(pConnection);

    SMB_LOCK_MUTEX(bInLock, &pConnection->pSocket->mutex);

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
            if ((errno != EINTR) || (errno != EAGAIN))
            {
                ntStatus = LwUnixErrnoToNtStatus(errno);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }

        sNumBytesToWrite -= sNumBytesWritten;
        sTotalNumBytesWritten += sNumBytesWritten;
    }

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pConnection->pSocket->mutex);

    return ntStatus;

error:

    SMB_UNLOCK_MUTEX(bInLock, &pConnection->pSocket->mutex);

    SrvConnectionSetInvalid(pConnection);

    goto cleanup;
}

VOID
SrvConnectionRelease(
    PSMB_SRV_CONNECTION pConnection
    )
{
    if (InterlockedDecrement(&pConnection->refCount) == 0)
    {
        if (pConnection->readerState.pRequestPacket)
        {
            SMBPacketFree(
                pConnection->hPacketAllocator,
                pConnection->readerState.pRequestPacket);
        }
        SMBFreeMemory(pConnection);
    }
}

static
NTSTATUS
SrvConnectionReadMessage(
    PSMB_SRV_SOCKET pSocket,
    size_t          sBytesToRead,
    size_t          sOffset,
    PSMB_PACKET     pPacket,
    size_t*         psNumBytesRead
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t  sNumBytesRead = 0;
    BOOLEAN  bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    do
    {
        sNumBytesRead = read(pSocket->fd, pPacket->pRawBuffer + ulOffset, sBytesToRead);
        if (sNumBytesRead < 0)
        {
            if ((errno != EAGAIN) && (errno != EINTR))
            {
                ntStatus = errno;
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }

    } while (sNumBytesRead < 0);

    *psNumBytesRead = sNumBytesRead;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    *psNumBytesRead = 0;

    goto cleanup;
}

