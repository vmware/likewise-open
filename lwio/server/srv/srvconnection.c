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
SrvConnectionAcquireSessionId_inlock(
   PSMB_SRV_CONNECTION pConnection,
   PUSHORT             pUid
   );

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
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvConnectionSessionRelease(
    PVOID pSession
    );

NTSTATUS
SrvConnectionCreate(
    PSMB_SRV_SOCKET           pSocket,
    HANDLE                    hPacketAllocator,
    HANDLE                    hGssContext,
    PSMB_SRV_SHARE_DB_CONTEXT pShareDbContext,
    PSRV_PROPERTIES           pServerProperties,
    PSRV_HOST_INFO            pHostinfo,
    PSMB_SRV_CONNECTION*      ppConnection
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = NULL;

    SMB_LOG_DEBUG("Creating server connection [fd:%d]", pSocket->fd);

    ntStatus = LW_RTL_ALLOCATE(
                    &pConnection,
                    SMB_SRV_CONNECTION,
                    sizeof(SMB_SRV_CONNECTION));
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->refCount = 1;

    SMB_LOG_DEBUG("Associating connection [object:0x%x][fd:%d]",
                    pConnection,
                    pSocket->fd);

    pthread_rwlock_init(&pConnection->mutex, NULL);
    pConnection->pMutex = &pConnection->mutex;

    ntStatus = LwRtlRBTreeCreate(
                    &SrvConnectionSessionCompare,
                    NULL,
                    &SrvConnectionSessionRelease,
                    &pConnection->pSessionCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAcquireHostInfo(
                    pHostinfo,
                    &pConnection->pHostinfo);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGssAcquireContext(
                    pConnection->pHostinfo,
                    hGssContext,
                    &pConnection->hGssContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->ulSequence = 0;
    pConnection->hPacketAllocator = hPacketAllocator;
    pConnection->pShareDbContext = pShareDbContext;
    pConnection->state = SMB_SRV_CONN_STATE_INITIAL;
    pConnection->pSocket = pSocket;

    memcpy(&pConnection->serverProperties, pServerProperties, sizeof(*pServerProperties));
    uuid_copy(pConnection->serverProperties.GUID, pServerProperties->GUID);

    *ppConnection = pConnection;

cleanup:

    return ntStatus;

error:

    *ppConnection = NULL;

    if (pConnection)
    {
        SrvConnectionRelease(pConnection);
    }

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
    PULONG              pulRequestSequence
    )
{
    NTSTATUS ntStatus = 0;
    ULONG ulRequestSequence = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

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

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;
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

    SMB_LOG_DEBUG("Setting connection [fd:%d] to invalid", pConnection->pSocket->fd);

    pConnection->state = SMB_SRV_CONN_STATE_INVALID;

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);
}

SMB_SRV_CONN_STATE
SrvConnectionGetState(
    PSMB_SRV_CONNECTION pConnection
    )
{
    SMB_SRV_CONN_STATE connState = SMB_SRV_CONN_STATE_INITIAL;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    connState = pConnection->state;

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return connState;
}

VOID
SrvConnectionSetState(
    PSMB_SRV_CONNECTION pConnection,
    SMB_SRV_CONN_STATE  connState
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    pConnection->state = connState;

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
    PSMB_SRV_CONNECTION pConnection,
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

    SMB_UNLOCK_MUTEX(bInLock, &pConnection->pSocket->mutex);

    return ntStatus;

error:

    SMB_UNLOCK_MUTEX(bInLock, &pConnection->pSocket->mutex);

    SrvConnectionSetInvalid(pConnection);

    goto cleanup;
}

NTSTATUS
SrvConnectionFindSession(
    PSMB_SRV_CONNECTION pConnection,
    USHORT uid,
    PSMB_SRV_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pConnection->pSessionCollection,
                    &uid,
                    (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pSession->refcount);

    *ppSession = pSession;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppSession = NULL;

    goto cleanup;
}

NTSTATUS
SrvConnectionRemoveSession(
    PSMB_SRV_CONNECTION pConnection,
    USHORT              uid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pConnection->pSessionCollection,
                    &uid);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    goto cleanup;
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
    USHORT  uid = 0;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = SrvConnectionAcquireSessionId_inlock(
                    pConnection,
                    &uid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionCreate(
                    uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pConnection->pSessionCollection,
                    &pSession->uid,
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
    SMB_LOG_DEBUG("Releasing connection [fd:%d]", pConnection->pSocket->fd);

    if (InterlockedDecrement(&pConnection->refCount) == 0)
    {
        SMB_LOG_DEBUG("Freeing connection [object:0x%x][fd:%d]",
                        pConnection,
                        pConnection->pSocket->fd);

        if (pConnection->readerState.pRequestPacket)
        {
            SMBPacketFree(
                pConnection->hPacketAllocator,
                pConnection->readerState.pRequestPacket);
        }

        if (pConnection->pSessionKey)
        {
            LwRtlMemoryFree(pConnection->pSessionKey);
        }

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
            LwRtlRBTreeFree(pConnection->pSessionCollection);
        }

        if (pConnection->pHostinfo)
        {
            SrvReleaseHostInfo(pConnection->pHostinfo);
        }

        if (pConnection->pMutex)
        {
            pthread_rwlock_destroy(&pConnection->mutex);
            pConnection->pMutex = NULL;
        }

        SrvConnectionFreeContentsClientProperties(&pConnection->clientProperties);

        LwRtlMemoryFree(pConnection);
    }
}

static
NTSTATUS
SrvConnectionAcquireSessionId_inlock(
   PSMB_SRV_CONNECTION pConnection,
   PUSHORT             pUid
   )
{
    NTSTATUS ntStatus = 0;
    USHORT   candidateUid = pConnection->nextAvailableUid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PSMB_SRV_SESSION pSession = NULL;

        if (!candidateUid || (candidateUid == UINT16_MAX))
        {
            candidateUid++;
        }

        ntStatus = LwRtlRBTreeFind(
                        pConnection->pSessionCollection,
                        &candidateUid,
                        (PVOID*)&pSession);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = 0;
            bFound = TRUE;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateUid != pConnection->nextAvailableUid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_SESSIONS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pConnection->nextAvailableUid = candidateUid + 1;
    *pUid = candidateUid;

cleanup:

    return ntStatus;

error:

    *pUid = 0;

    goto cleanup;
}

static
VOID
SrvConnectionFreeContentsClientProperties(
    PSRV_CLIENT_PROPERTIES pProperties
    )
{
    if (pProperties->pwszNativeLanMan)
    {
        LwRtlMemoryFree(pProperties->pwszNativeLanMan);
    }
    if (pProperties->pwszNativeOS)
    {
        LwRtlMemoryFree(pProperties->pwszNativeOS);
    }
    if (pProperties->pwszNativeDomain)
    {
        LwRtlMemoryFree(pProperties->pwszNativeDomain);
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

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    *psNumBytesRead = 0;

    goto cleanup;
}

static
int
SrvConnectionSessionCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PUSHORT pUid1 = (PUSHORT)pKey1;
    PUSHORT pUid2 = (PUSHORT)pKey2;

    assert (pUid1 != NULL);
    assert (pUid2 != NULL);

    if (*pUid1 > *pUid2)
    {
        return 1;
    }
    else if (*pUid1 < *pUid2)
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
