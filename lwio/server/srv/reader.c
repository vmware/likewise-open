#include "includes.h"

static
PVOID
SrvSocketReaderMain(
    PVOID pData
    );

static
NTSTATUS
SrvSocketReaderInterrupt(
    PSMB_SRV_SOCKET_READER pReader
    );

static
BOOLEAN
SrvSocketReaderMustStop(
    PSMB_SRV_SOCKET_READER_CONTEXT pContext
    );

NTSTATUS
SrvSocketReaderInit(
    PSMB_SRV_SOCKET_READER pReader
    )
{
    NTSTATUS ntStatus = 0;

    memset(&pReader->context, 0, sizeof(pReader->context));

    pReader->context.mutex = PTHREAD_MUTEX_INITIALIZER;
    pReader->context.bStop = FALSE;
    pReader->context.pSocketList  = NULL;
    pReader->context.ulNumSockets = 0;
    pReader->context.fd[0] = pReader->context.fd[1] = -1;

    ntStatus = SrvProdConsInit(&pReader->context.pWorkQueue);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pipe(pReader->context.fd) < 0)
    {
        // TODO: Map error number
        ntStatus = errno;
    }

    ntStatus = pthread_create(
                    pReader->reader,
                    NULL,
                    &SrvSocketReaderMain,
                    &pReader->context);
    BAIL_ON_NT_STATUS;

    pReader->pReader = &pReader->reader;

error:

    return ntStatus;
}

ULONG
SrvSocketReaderGetCount(
    PSMB_SRV_SOCKET_READER pReader
    )
{
    ULONG ulSockets = 0;

    pthread_mutex_lock(&pReader->pContext.mutex);

    ulSockets = pReader->context.ulNumSockets;

    pthread_mutex_unlock(&pReader->context.mutex);

    return ulSockets;
}

NTSTATUS
NTSTATUS
SrvSocketReaderEnqueueConnection(
    PSMB_SRV_SOCKET_READER pReader,
    PSMB_SRV_CONNECTION    pConnection
    )
{
    NTSTATUS ntStatus = 0;

    pthread_mutex_lock(&pReader->context.mutex);

    ntStatus = SMBDLinkedListAppend(
                    &pReader->context.pSocketList,
                    pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

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
    PSMB_SRV_SOCKET_READER_CONTEXT pContext = (PSMB_SRV_SOCKET_READER_CONTEXT)pData;
    PSMBDLINKEDLIST pIter = NULL;

    while (!SrvSocketReaderMustStop(pContext))
    {
        int ret = 0;
        fd_set fdset;
        int maxFd = pContext->fd[0];

        FD_ZERO(fdset);

        FD_SET(pContext->fd[0], &fdset);

        pthread_mutex_lock(&pContext->mutex);

        for (pIter = pContext->pSocketList; pIter; pIter = pIter->pNext)
        {
            int fd = -1;
            PSMB_SRV_CONNECTION pConnection = (PSMB_SRV_CONNECTION)pIter->pItem;

            pthread_mutex_lock(&pConnection->mutex);
            pthread_mutex_lock(&pConnection->pSocket->mutex);

            fd = pConnection->pSocket->fd;

            if (fd > maxFd)
            {
                maxFd = fd;
            }

            pthread_mutex_unlock(&pConnection->pSocket->mutex);
            pthread_mutex_unlock(&pConnection->mutex);

            FD_SET(fd, &fdset);
        }

        pthread_mutex_unlock(&pContext->mutex);

        ret = select(maxFd, &fdset, NULL, &fdset, NULL);
        if (ret == -1)
        {
            // TODO - map error number
            ntStatus = errno;
        }
        else if (ret != 1)
        {
            // TODO - map error number
            ntStatus = EFAULT;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (SrvSocketReaderMustStop(pContext))
        {
            break;
        }

        // TODO: Check for new connections

        // TODO: Read messages and enqueue in work queue
    }

error:

    return NULL;
}

NTSTATUS
SrvSocketReaderFreeContents(
    PSMB_SRV_SOCKET_READER pReader
    )
{
    NTSTATUS ntStatus = 0;
    int iFd = 0;
    PSMBDLINKEDLIST pIter = NULL;

    if (pReader->pReader)
    {
        pthread_mutex_lock(&pReader->context.mutex);

        pReader->context.bStop = TRUE;

        pthread_mutex_unlock(&pReader->context.mutex);

        ntStatus = SrvSocketReaderInterrupt(pReader);
        BAIL_ON_NT_STATUS(ntStatus);

        pthread_join(pReader->reader);

        pReader->pReader = NULL;
    }

    if (pReader->context.ulNumSockets)
    {
        for (pIter = pReader->context.pSocketList;
             pIter;
             pIter = pIter->pNext)
        {
            PSMB_SRV_CONNECTION pConnection = (PSMB_SRV_CONNECTION)pIter->pItem;

            SrvConnectionRelease(pConnection);
        }

        SMBDLinkedListFree(pReader->context.pSocketList);
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

    return ntStatus;
}

static
NTSTATUS
SrvSocketReaderInterrupt(
    PSMB_SRV_SOCKET_READER pReader
    )
{
    NTSTATUS ntStatus = 0;

    if (write(pReader->context.fd[0], 'I', 1) != 1)
    {
        // TODO: Map error number
        ntStatus = errno;
    }

    return ntStatus;
}

static
BOOLEAN
SrvSocketReaderMustStop(
    PSMB_SRV_SOCKET_READER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pContext->mutex);

    bStop = pContext->bStop;

    pthread_mutex_unlock(&pContext->mutex);

    return bStop;
}
