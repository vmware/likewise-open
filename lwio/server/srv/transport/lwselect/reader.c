#include "includes.h"

typedef struct _SMB_SOCKET_READER_WORK_SET
{
    fd_set          fdset;
    int             maxFd;
    PSMBDLINKEDLIST pConnectionList;
} SMB_SOCKET_READER_WORK_SET, *PSMB_SOCKET_READER_WORK_SET;

static
VOID
SrvSocketReaderSetActiveState(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    BOOLEAN bState
    );

static
PVOID
SrvSocketReaderMain(
    PVOID pData
    );

static
NTSTATUS
SrvSocketReaderFillFdSet(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    PSMB_SOCKET_READER_WORK_SET pReaderWorkset
    );

static
NTSTATUS
SrvSocketReaderFillFdSetInOrder(
    PVOID pKey,
    PVOID pConnection,
    PVOID pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvSocketReaderProcessConnections(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    PSMB_SOCKET_READER_WORK_SET pReaderWorkset
    );

static
NTSTATUS
SrvSocketReaderReadMessage(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    PLWIO_SRV_CONNECTION pConnection
    );

static
NTSTATUS
SrvVerifyContext(
    PSRV_EXEC_CONTEXT pContext
    );

static
VOID
SrvSocketReaderPurgeConnections(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    PSMB_SOCKET_READER_WORK_SET pReaderWorkset
    );

static
NTSTATUS
SrvSocketReaderInterrupt(
    PLWIO_SRV_SOCKET_READER pReader
    );

static
BOOLEAN
SrvSocketReaderMustStop(
    PLWIO_SRV_SOCKET_READER_CONTEXT pContext
    );

static
int
SrvSocketReaderCompareConnections(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvSocketReaderReleaseConnection(
    PVOID pConnection
    );

NTSTATUS
SrvSocketReaderInit(
    PSMB_PROD_CONS_QUEUE   pWorkQueue,
    PLWIO_SRV_SOCKET_READER pReader
    )
{
    NTSTATUS ntStatus = 0;

    memset(&pReader->context, 0, sizeof(pReader->context));

    pthread_mutex_init(&pReader->context.mutex, NULL);
    pReader->context.pMutex = &pReader->context.mutex;

    pReader->context.bStop = FALSE;
    pReader->context.bActive = FALSE;
    pReader->context.pConnections  = NULL;
    pReader->context.ulNumSockets = 0;
    pReader->context.fd[0] = pReader->context.fd[1] = -1;
    pReader->context.readerId = pReader->readerId;

    ntStatus = LwRtlRBTreeCreate(
                    &SrvSocketReaderCompareConnections,
                    NULL,
                    &SrvSocketReaderReleaseConnection,
                    &pReader->context.pConnections);
    BAIL_ON_NT_STATUS(ntStatus);

    pReader->context.pWorkQueue = pWorkQueue;

    if (pipe(pReader->context.fd) < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
    }

    ntStatus = pthread_create(
                    &pReader->reader,
                    NULL,
                    &SrvSocketReaderMain,
                    &pReader->context);
    BAIL_ON_NT_STATUS(ntStatus);

    pReader->pReader = &pReader->reader;

error:

    return ntStatus;
}

ULONG
SrvSocketReaderGetCount(
    PLWIO_SRV_SOCKET_READER pReader
    )
{
    ULONG ulSockets = 0;

    pthread_mutex_lock(&pReader->context.mutex);

    ulSockets = pReader->context.ulNumSockets;

    pthread_mutex_unlock(&pReader->context.mutex);

    return ulSockets;
}

BOOLEAN
SrvSocketReaderIsActive(
    PLWIO_SRV_SOCKET_READER pReader
    )
{
    BOOLEAN bActive = FALSE;

    pthread_mutex_lock(&pReader->context.mutex);

    bActive = pReader->context.bActive;

    pthread_mutex_unlock(&pReader->context.mutex);

    return bActive;
}

static
VOID
SrvSocketReaderSetActiveState(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    BOOLEAN bState
    )
{
    pthread_mutex_lock(&pReaderContext->mutex);

    pReaderContext->bActive = bState;

    pthread_mutex_unlock(&pReaderContext->mutex);
}

NTSTATUS
SrvSocketReaderEnqueueConnection(
    PLWIO_SRV_SOCKET_READER pReader,
    PLWIO_SRV_CONNECTION    pConnection
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SOCKET pSocket = (PLWIO_SRV_SOCKET)pConnection->hSocket;

    pthread_mutex_lock(&pReader->context.mutex);

    LWIO_LOG_DEBUG("Enqueueing connection [fd:%d] at reader [id:%u]",
                    pSocket->fd,
                    pReader->readerId);

    ntStatus = LwRtlRBTreeAdd(
                    pReader->context.pConnections,
                    &pSocket->fd,
                    pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    pReader->context.ulNumSockets++;

    ntStatus = SrvSocketReaderInterrupt(
                    pReader);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    pthread_mutex_unlock(&pReader->context.mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
PVOID
SrvSocketReaderMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SOCKET_READER_CONTEXT pContext = (PLWIO_SRV_SOCKET_READER_CONTEXT)pData;
    SMB_SOCKET_READER_WORK_SET workSet;

    LWIO_LOG_DEBUG("Srv reader [id:%u] running", pContext->readerId);

    SrvSocketReaderSetActiveState(pContext, TRUE);

    memset(&workSet, 0, sizeof(workSet));

    while (!SrvSocketReaderMustStop(pContext))
    {
        int ret = 0;
        struct timeval timeout = { .tv_sec = 60, .tv_usec = 0 };

        if (workSet.pConnectionList)
        {
            SrvSocketReaderPurgeConnections(
                    pContext,
                    &workSet);
        }

        ntStatus = SrvSocketReaderFillFdSet(
                        pContext,
                        &workSet);
        BAIL_ON_NT_STATUS(ntStatus);

        ret = select(
                workSet.maxFd + 1,
                &workSet.fdset,
                NULL,
                NULL,
                &timeout);
        if (ret == -1)
        {
            if (errno == EINTR)
            {
                continue;
            }

            ntStatus = LwUnixErrnoToNtStatus(errno);
        }
        else if (ret == 0)
        {
            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (SrvSocketReaderMustStop(pContext))
        {
            break;
        }

        ntStatus = SrvSocketReaderProcessConnections(
                        pContext,
                        &workSet);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (workSet.pConnectionList)
    {
        SrvSocketReaderPurgeConnections(
                pContext,
                &workSet);
    }

    SrvSocketReaderSetActiveState(pContext, FALSE);

    LWIO_LOG_DEBUG("Srv reader [id:%u] stopping", pContext->readerId);

    return NULL;

error:

    goto cleanup;
}

NTSTATUS
SrvSocketReaderFreeContents(
    PLWIO_SRV_SOCKET_READER pReader
    )
{
    NTSTATUS ntStatus = 0;
    int iFd = 0;

    if (pReader->pReader)
    {
        if (pReader->context.pMutex)
        {
            pthread_mutex_lock(pReader->context.pMutex);

            pReader->context.bStop = TRUE;

            pthread_mutex_unlock(pReader->context.pMutex);

            ntStatus = SrvSocketReaderInterrupt(pReader);
            BAIL_ON_NT_STATUS(ntStatus);

            pthread_join(pReader->reader, NULL);

            pthread_mutex_destroy(pReader->context.pMutex);
            pReader->context.pMutex = NULL;

            pReader->pReader = NULL;
        }
    }

    if (pReader->context.pConnections)
    {
        LwRtlRBTreeFree(pReader->context.pConnections);
    }

    if (pReader->context.pWorkQueue)
    {
        SrvProdConsFreeContents(pReader->context.pWorkQueue);
    }

    for (; iFd < 2; iFd++)
    {
        if (pReader->context.fd[iFd] >= 0)
        {
            close(pReader->context.fd[iFd]);
        }
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvSocketReaderFillFdSet(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    PSMB_SOCKET_READER_WORK_SET pReaderWorkset
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    FD_ZERO(&pReaderWorkset->fdset);
    pReaderWorkset->maxFd = -1;

    FD_SET(pReaderContext->fd[0], &pReaderWorkset->fdset);

    LWIO_LOCK_MUTEX(bInLock, &pReaderContext->mutex);

    ntStatus = LwRtlRBTreeTraverse(
                    pReaderContext->pConnections,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvSocketReaderFillFdSetInOrder,
                    pReaderWorkset);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pReaderContext->fd[0] > pReaderWorkset->maxFd)
    {
        pReaderWorkset->maxFd = pReaderContext->fd[0];
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pReaderContext->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvSocketReaderFillFdSetInOrder(
    PVOID pKey,
    PVOID pConnection,
    PVOID pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION pSrvConnection = (PLWIO_SRV_CONNECTION)pConnection;
    PSMB_SOCKET_READER_WORK_SET pReaderWorkset = (PSMB_SOCKET_READER_WORK_SET)pUserData;

    if (!SrvConnectionIsInvalid(pSrvConnection))
    {
        int fd = -1;

        fd = SrvConnectionGetFd(pSrvConnection);

        FD_SET(fd, &pReaderWorkset->fdset);
        pReaderWorkset->maxFd = fd;
    }

    ntStatus = SMBDLinkedListAppend(&pReaderWorkset->pConnectionList, pSrvConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pSrvConnection->refCount);

    *pbContinue = TRUE;

cleanup:

    return ntStatus;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvSocketReaderProcessConnections(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    PSMB_SOCKET_READER_WORK_SET pReaderWorkset
    )
{
    NTSTATUS ntStatus = 0;
    PSMBDLINKEDLIST pIter = NULL;

    if (FD_ISSET(pReaderContext->fd[0], &pReaderWorkset->fdset))
    {
        CHAR c;

        if (read(pReaderContext->fd[0], &c, 1) != 1)
        {
            ntStatus = STATUS_LOCAL_DISCONNECT;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    for (pIter = pReaderWorkset->pConnectionList;
         pIter;
         pIter = pIter->pNext)
    {
        PLWIO_SRV_CONNECTION pConnection = (PLWIO_SRV_CONNECTION)pIter->pItem;
        PLWIO_SRV_SOCKET pSocket = (PLWIO_SRV_SOCKET)pConnection->hSocket;

        if (!SrvConnectionIsInvalid(pConnection))
        {
            int fd = SrvConnectionGetFd(pConnection);

            if (FD_ISSET(fd, &pReaderWorkset->fdset))
            {
                NTSTATUS ntStatus2 = 0;

                ntStatus2 = SrvSocketReaderReadMessage(
                                pReaderContext,
                                pConnection);

                switch (ntStatus2)
                {
                    case STATUS_SUCCESS:

                        break;

                    case STATUS_CONNECTION_RESET:
                    {
                        CHAR szIpAddr[256];

                        LWIO_LOG_DEBUG("Connection reset by peer [fd:%d][%s]",
                                        fd,
                                        SMB_SAFE_LOG_STRING(inet_ntop(
                                                                AF_INET,
                                                                &pSocket->cliaddr.sin_addr,
                                                                szIpAddr,
                                                                sizeof(szIpAddr))));
                    }

                    default:

                        SrvConnectionSetInvalid(pConnection);

                        break;
                }
            }
        }
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvSocketReaderReadMessage(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    PLWIO_SRV_CONNECTION            pConnection
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_EXEC_CONTEXT pContext = NULL;
    PSMB_PACKET pPacket = NULL;

    ntStatus = SrvConnectionReadPacket(pConnection, &pPacket);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pPacket)
    {
        ntStatus = SrvBuildExecContext(pConnection, pPacket, FALSE, &pContext);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvVerifyContext(pContext);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvProdConsEnqueue(pReaderContext->pWorkQueue, pContext);
        BAIL_ON_NT_STATUS(ntStatus);

        pContext = NULL;
    }

cleanup:

    if (pPacket)
    {
        SMBPacketRelease(pConnection->hPacketAllocator, pPacket);
    }

    return ntStatus;

error:

    if (pContext)
    {
        SrvReleaseExecContext(pContext);
    }

    goto cleanup;
}

static
NTSTATUS
SrvVerifyContext(
    PSRV_EXEC_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION pConnection = pContext->pConnection;

    switch (pConnection->protocolVer)
    {
        case SMB_PROTOCOL_VERSION_1:

            // Update the sequence whether we end up signing or not
            ntStatus = SrvConnectionGetNextSequence(
                            pConnection,
                            pContext->pSmbRequest,
                            &pContext->pSmbRequest->sequence);

            break;

        default:

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (pConnection->serverProperties.bRequireSecuritySignatures &&
        pConnection->pSessionKey)
    {
        switch (pConnection->protocolVer)
        {
            case SMB_PROTOCOL_VERSION_1:

                ntStatus = SMBPacketVerifySignature(
                                pContext->pSmbRequest,
                                pContext->pSmbRequest->sequence,
                                pConnection->pSessionKey,
                                pConnection->ulSessionKeyLength);

                break;

            case SMB_PROTOCOL_VERSION_2:

                ntStatus = SMB2PacketVerifySignature(
                                pContext->pSmbRequest,
                                pConnection->pSessionKey,
                                pConnection->ulSessionKeyLength);

                break;

            default:

                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}


static
VOID
SrvSocketReaderPurgeConnections(
    PLWIO_SRV_SOCKET_READER_CONTEXT pReaderContext,
    PSMB_SOCKET_READER_WORK_SET pReaderWorkset
    )
{
    PSMBDLINKEDLIST pIter = pReaderWorkset->pConnectionList;

    for (; pIter; pIter = pIter->pNext)
    {
        PLWIO_SRV_CONNECTION pSrvConnection = (PLWIO_SRV_CONNECTION)pIter->pItem;

        if (SrvConnectionIsInvalid(pSrvConnection))
        {
            PLWIO_SRV_SOCKET pSocket = (PLWIO_SRV_SOCKET)pSrvConnection->hSocket;

            pthread_mutex_lock(pReaderContext->pMutex);

            LwRtlRBTreeRemove(
                    pReaderContext->pConnections,
                    &pSocket->fd);

	    pReaderContext->ulNumSockets--;

	    assert(pReaderContext->ulNumSockets >= 0);

            pthread_mutex_unlock(pReaderContext->pMutex);
        }

        SrvConnectionRelease(pSrvConnection);
    }

    if (pReaderWorkset->pConnectionList)
    {
        SMBDLinkedListFree(pReaderWorkset->pConnectionList);
        pReaderWorkset->pConnectionList = NULL;
    }
}

static
NTSTATUS
SrvSocketReaderInterrupt(
    PLWIO_SRV_SOCKET_READER pReader
    )
{
    NTSTATUS ntStatus = 0;

    if (write(pReader->context.fd[1], "I", 1) != 1)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
    }

    return ntStatus;
}

static
BOOLEAN
SrvSocketReaderMustStop(
    PLWIO_SRV_SOCKET_READER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pContext->mutex);

    bStop = pContext->bStop;

    pthread_mutex_unlock(&pContext->mutex);

    return bStop;
}

static
int
SrvSocketReaderCompareConnections(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PINT pFd1 = (PINT)pKey1;
    PINT pFd2 = (PINT)pKey2;

    assert(pFd1 != NULL);
    assert(pFd2 != NULL);

    if (*pFd1 > *pFd2)
    {
        return 1;
    }
    else if (*pFd1 < *pFd2)
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
SrvSocketReaderReleaseConnection(
    PVOID pConnection
    )
{
    SrvConnectionRelease((PLWIO_SRV_CONNECTION)pConnection);
}
