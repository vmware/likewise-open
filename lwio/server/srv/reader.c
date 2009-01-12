#include "includes.h"

NTSTATUS
SrvSocketReaderInit(
    PSMB_SRV_SOCKET_READER pReader
    )
{
    NTSTATUS ntStatus = 0;

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

    reutrn ulSockets;
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

    // TODO: Signal the thread to pick up the new
    //       connection

cleanup:

    pthread_mutex_unlock(&pReader->context.mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvSocketReaderFreeContents(
    PSMB_SRV_SOCKET_READER pReader
    )
{
    NTSTATUS ntStatus = 0;

    return ntStatus;
}
