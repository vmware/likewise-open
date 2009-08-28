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

    if (pConnection->hSocket)
    {
        PLWIO_SRV_SOCKET pSocket = (PLWIO_SRV_SOCKET)pConnection->hSocket;

        pthread_mutex_lock(&pSocket->mutex);

        fd = pSocket->fd;

        pthread_mutex_unlock(&pSocket->mutex);
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
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    if (!pConnection->readerState.pRequestPacket)
    {
        ntStatus = SMBPacketAllocate(
                        pConnection->hPacketAllocator,
                        &pConnection->readerState.pRequestPacket);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBPacketBufferAllocate(
                        pConnection->hPacketAllocator,
                        (64 * 1024) + 4096,
                        &pConnection->readerState.pRequestPacket->pRawBuffer,
                        &pConnection->readerState.pRequestPacket->bufferLen);
        BAIL_ON_NT_STATUS(ntStatus);

        pConnection->readerState.bReadHeader = TRUE;
        pConnection->readerState.sNumBytesToRead = sizeof(NETBIOS_HEADER);
        pConnection->readerState.sOffset = 0;
        pConnection->readerState.pRequestPacket->bufferUsed = 0;
    }

    if (pConnection->readerState.bReadHeader)
    {
        size_t sNumBytesRead = 0;

        ntStatus = SrvConnectionReadMessage(
                        (PLWIO_SRV_SOCKET)pConnection->hSocket,
                        pConnection->readerState.sNumBytesToRead,
                        pConnection->readerState.sOffset,
                        pConnection->readerState.pRequestPacket,
                        &sNumBytesRead);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!sNumBytesRead)
        {
            // peer reset connection
            ntStatus = STATUS_CONNECTION_RESET;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pConnection->readerState.sNumBytesToRead -= sNumBytesRead;
        pConnection->readerState.sOffset += sNumBytesRead;
        pConnection->readerState.pRequestPacket->bufferUsed += sNumBytesRead;

        if (!pConnection->readerState.sNumBytesToRead)
        {
            PSMB_PACKET pPacket = pConnection->readerState.pRequestPacket;
            ULONG ulBytesAvailable =
                                    pPacket->bufferLen - sizeof(NETBIOS_HEADER);

            pConnection->readerState.bReadHeader = FALSE;

            pPacket->pNetBIOSHeader = (NETBIOS_HEADER *) pPacket->pRawBuffer;
            pPacket->pNetBIOSHeader->len = ntohl(pPacket->pNetBIOSHeader->len);

            pConnection->readerState.sNumBytesToRead = pPacket->pNetBIOSHeader->len;

            // check if the message fits in our currently allocated buffer
            if (pConnection->readerState.sNumBytesToRead > ulBytesAvailable)
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
                        (PLWIO_SRV_SOCKET)pConnection->hSocket,
                        pConnection->readerState.sNumBytesToRead,
                        pConnection->readerState.sOffset,
                        pConnection->readerState.pRequestPacket,
                        &sNumBytesRead);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!sNumBytesRead)
        {
            // peer reset connection
            ntStatus = STATUS_CONNECTION_RESET;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pConnection->readerState.sNumBytesToRead            -= sNumBytesRead;
        pConnection->readerState.sOffset                    += sNumBytesRead;
        pConnection->readerState.pRequestPacket->bufferUsed += sNumBytesRead;
    }

    // Packet is complete
    if (!pConnection->readerState.bReadHeader &&
        !pConnection->readerState.sNumBytesToRead)
    {
        PSMB_PACKET pPacket    = pConnection->readerState.pRequestPacket;
        PBYTE pBuffer          = pPacket->pRawBuffer + sizeof(NETBIOS_HEADER);
        ULONG ulBytesAvailable = pPacket->bufferUsed - sizeof(NETBIOS_HEADER);

        switch (*pBuffer)
        {
            case 0xFF:

                pPacket->protocolVer = SMB_PROTOCOL_VERSION_1;

                if (ulBytesAvailable < sizeof(SMB_HEADER))
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                pPacket->pSMBHeader = (PSMB_HEADER)(pBuffer);
                pBuffer            += sizeof(SMB_HEADER);
                ulBytesAvailable   -= sizeof(SMB_HEADER);

                if (SMBIsAndXCommand(pPacket->pSMBHeader->command))
                {
                    if (ulBytesAvailable < sizeof(ANDX_HEADER))
                    {
                        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                        BAIL_ON_NT_STATUS(ntStatus);
                    }

                    pPacket->pAndXHeader = (PANDX_HEADER)pBuffer;
                    pBuffer             += sizeof(ANDX_HEADER);
                    ulBytesAvailable    -= sizeof(ANDX_HEADER);
                }

                pPacket->pParams = (ulBytesAvailable > 0) ? pBuffer : NULL;
                pPacket->pData = NULL;

                break;

            case 0xFE:

                pPacket->protocolVer = SMB_PROTOCOL_VERSION_2;

                if (ulBytesAvailable < sizeof(SMB2_HEADER))
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                pPacket->pSMB2Header = (PSMB2_HEADER)pBuffer;
                pBuffer             += sizeof(SMB2_HEADER);
                ulBytesAvailable    -= sizeof(SMB2_HEADER);

                pPacket->pParams    = NULL;
                pPacket->pData      = NULL;

                break;

            default:

                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

        *ppPacket = pConnection->readerState.pRequestPacket;

        pConnection->readerState.pRequestPacket = NULL;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppPacket = NULL;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    SrvConnectionSetInvalid(pConnection);

    goto cleanup;
}

NTSTATUS
SrvConnectionWriteMessage(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pPacket
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    PLWIO_SRV_SOCKET pSocket = (PLWIO_SRV_SOCKET)pConnection->hSocket;
    size_t   sNumBytesToWrite = pPacket->bufferUsed;
    size_t   sTotalNumBytesWritten = 0;
    int fd = -1;

    if (pConnection->serverProperties.bRequireSecuritySignatures &&
        pConnection->pSessionKey)
    {
        switch (pPacket->protocolVer)
        {
            case SMB_PROTOCOL_VERSION_1:

                pPacket->pSMBHeader->flags2 |= FLAG2_SECURITY_SIG;

                ntStatus = SMBPacketSign(
                                pPacket,
                                pPacket->sequence,
                                pConnection->pSessionKey,
                                pConnection->ulSessionKeyLength);

                break;

            case SMB_PROTOCOL_VERSION_2:

                ntStatus = SMB2PacketSign(
                                pPacket,
                                pConnection->pSessionKey,
                                pConnection->ulSessionKeyLength);

                break;

            default:

                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    fd = SrvConnectionGetFd(pConnection);

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

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

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

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

NTSTATUS
SrvConnectionGetNamedPipeClientAddress(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SOCKET pSocket = (PLWIO_SRV_SOCKET)pConnection->hSocket;
    PBYTE pAddr = (PBYTE)&pSocket->cliaddr.sin_addr.s_addr;
    ULONG ulAddrLength = sizeof(pSocket->cliaddr.sin_addr.s_addr);

    ntStatus = IoRtlEcpListInsert(pEcpList,
                                  IO_ECP_TYPE_PEER_ADDRESS,
                                  pAddr,
                                  ulAddrLength,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

