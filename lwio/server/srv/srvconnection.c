#include "includes.h"

// Rules:
//
// Only one reader thread can read from this socket
// Multiple writers can write to the socket, in a synchronized manner per message
// Both readers and writers can set the connection state to be invalid
// The file descriptor is set to -1 only by the reader thread (not the writers)
// The file descriptor is closed only when the connection object is freed.

static
VOID
SrvConnectionFreeContentsClientProperties(
    PSRV_CLIENT_PROPERTIES pProperties
    );

static
NTSTATUS
SrvConnectionReadMessage(
    PSMB_SRV_SOCKET pSocket,
    size_t          sBytesToRead,
    size_t          sOffset,
    PSMB_PACKET     pPacket,
    size_t*         psNumBytesRead
    );

static
int
SrvConnectionSessionCompare(
    PVOID pSession1,
    PVOID pSession2
    );

static
VOID
SrvConnectionSessionRelease(
    PVOID pSession
    );

NTSTATUS
SrvConnectionCreate(
    PSMB_SRV_SOCKET pSocket,
    HANDLE          hPacketAllocator,
    HANDLE          hGssContext,
    PSRV_PROPERTIES pServerProperties,
    PSMB_SRV_CONNECTION* ppConnection
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = NULL;

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_SRV_CONNECTION),
                    (PVOID*)&pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->refCount = 1;

    pthread_rwlock_init(&pConnection->mutex, NULL);
    pConnection->pMutex = &pConnection->mutex;

    ntStatus = SMBRBTreeCreate(
                    &SrvConnectionSessionCompare,
                    &SrvConnectionSessionRelease,
                    &pConnection->pSessionCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvIdAllocatorCreate(
                    UINT16_MAX,
                    &pConnection->pSessionIdAllocator);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGssAcquireContext(
                    hGssContext,
                    &pConnection->hGssContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->ulSequence = 0;
    pConnection->hPacketAllocator = hPacketAllocator;
    pConnection->state = SMB_SRV_CONN_STATE_INITIAL;
    pConnection->pSocket = pSocket;

    memcpy(&pConnection->serverProperties, pServerProperties, sizeof(*pServerProperties));
    uuid_copy(pConnection->serverProperties.GUID, pServerProperties->GUID);

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
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    if (pConnection->pSocket)
    {
        pthread_mutex_lock(&pConnection->pSocket->mutex);

        fd = pConnection->pSocket->fd;

        pthread_mutex_unlock(&pConnection->pSocket->mutex);
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return fd;
}

NTSTATUS
SrvConnectionGetNextSequence(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PULONG pulRequestSequence,
    PULONG pulResponseSequence
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulRequestSequence = 0;
    ULONG ulResponseSequence = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    switch (pSmbRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:
        case COM_ECHO:

            break;

        case COM_SESSION_SETUP_ANDX:

            ulRequestSequence = pConnection->ulSequence++;
            ulResponseSequence = pConnection->ulSequence++;

            break;

        case COM_NT_CANCEL:

            ulRequestSequence = pConnection->ulSequence++;

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    *pulRequestSequence = ulRequestSequence;
    *pulResponseSequence = ulResponseSequence;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *pulRequestSequence = 0;
    *pulResponseSequence = 0;

    goto cleanup;
}

BOOLEAN
SrvConnectionIsInvalid(
    PSMB_SRV_CONNECTION pConnection
    )
{
    BOOLEAN bInvalid = FALSE;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    bInvalid = pConnection->state == SMB_SRV_CONN_STATE_INVALID;

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return bInvalid;
}

VOID
SrvConnectionSetInvalid(
    PSMB_SRV_CONNECTION pConnection
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    pConnection->state = SMB_SRV_CONN_STATE_INVALID;

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);
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
            goto cleanup;
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

PSMB_SRV_SESSION
SrvConnectionFindSession(
    PSMB_SRV_CONNECTION pConnection,
    USHORT uid
    )
{
    PSMB_SRV_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;
    SMB_SRV_SESSION finder;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    memset(&finder, 0, sizeof(finder));
    finder.uid = uid;

    pSession = SMBRBTreeFind(
                    pConnection->pSessionCollection,
                    &finder);

    if (pSession)
    {
        InterlockedIncrement(&pSession->refcount);
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return pSession;
}

NTSTATUS
SrvConnectionCreateSession(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_SRV_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;

    ntStatus = SrvSessionCreate(
                    pConnection->pSessionIdAllocator,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = SMBRBTreeAdd(
                    pConnection->pSessionCollection,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pSession->refcount);

    *ppSession = pSession;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppSession = NULL;

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

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

        SMB_SAFE_FREE_MEMORY(pConnection->pSessionKey);

        if (pConnection->hGssNegotiate)
        {
            SrvGssEndNegotiate(
                pConnection->hGssContext,
                pConnection->hGssNegotiate);
        }

        if (pConnection->hGssContext)
        {
            SrvGssReleaseContext(pConnection->hGssContext);
        }

        if (pConnection->pSessionCollection)
        {
            SMBRBTreeFree(pConnection->pSessionCollection);
        }

        if (pConnection->pSessionIdAllocator)
        {
            SrvIdAllocatorRelease(pConnection->pSessionIdAllocator);
        }

        if (pConnection->pMutex)
        {
            pthread_rwlock_destroy(&pConnection->mutex);
            pConnection->pMutex = NULL;
        }

        SrvConnectionFreeContentsClientProperties(&pConnection->clientProperties);

        SMBFreeMemory(pConnection);
    }
}

static
VOID
SrvConnectionFreeContentsClientProperties(
    PSRV_CLIENT_PROPERTIES pProperties
    )
{
    SMB_SAFE_FREE_MEMORY(pProperties->pwszNativeLanMan);
    SMB_SAFE_FREE_MEMORY(pProperties->pwszNativeOS);
    SMB_SAFE_FREE_MEMORY(pProperties->pwszNativeDomain);
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
        sNumBytesRead = read(pSocket->fd, pPacket->pRawBuffer + sOffset, sBytesToRead);
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

static
int
SrvConnectionSessionCompare(
    PVOID pSession1,
    PVOID pSession2
    )
{
    PSMB_SRV_SESSION pSession1_casted = (PSMB_SRV_SESSION)pSession1;
    PSMB_SRV_SESSION pSession2_casted = (PSMB_SRV_SESSION)pSession2;

    // safe to access uid here without locking the session object
    if (pSession1_casted->uid > pSession2_casted->uid)
    {
        return 1;
    }
    else if (pSession1_casted->uid < pSession2_casted->uid)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
SrvConnectionSessionRelease(
    PVOID pSession
    )
{
    SrvSessionRelease((PSMB_SRV_SESSION)pSession);
}
