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

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_SRV_SOCKET),
                    (PVOID*)&pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket->mutex = PTHREAD_MUTEX_INITIALIZER;
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
    SMBFreeMemory(pSocket);
}
