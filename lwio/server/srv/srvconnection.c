#include "includes.h"

NTSTATUS
SrvConnectionCreate(
    PSMB_SRV_SOCKET pSocket,
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
SrvConnectionRelease(
    PSMB_SRV_CONNECTION pConnection
    )
{

}
