#include "includes.h"

NTSTATUS
SrvSocketCreate(
    int fd,
    struct sockaddr_in* pClientAddr,
    PSMB_SRV_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SOCKET pSocket = NULL;

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_SRV_SOCKET),
                    (PVOID*)&pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pSocket->mutex, NULL);
    pSocket->pMutex = &pSocket->mutex;

    pSocket->fd = fd;
    memcpy(&pSocket->cliaddr, pClientAddr, sizeof(pSocket->cliaddr));

    *ppSocket = pSocket;

cleanup:

    return ntStatus;

error:

    *ppSocket = NULL;

    goto cleanup;
}

VOID
SrvSocketFree(
    PSMB_SRV_SOCKET pSocket
    )
{
    if (pSocket->fd >= 0)
    {
        close(pSocket->fd);
    }
    if (pSocket->pMutex)
    {
        pthread_mutex_destroy(pSocket->pMutex);
        pSocket->pMutex = NULL;
    }
    SMBFreeMemory(pSocket);
}
