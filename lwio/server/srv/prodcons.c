#include "includes.h"

NTSTATUS
SrvProdConsInit(
    PSMB_PROD_CONS_QUEUE pQueue
    )
{
    NTSTATUS ntStatus = 0;

    pQueue->mutex = PTHREAD_MUTEX_INITIALIZER;

    pthread_cond_init(&pQueue->event, NULL);
    pQueue->pEvent = &pQueue->event;

    memset(&pQueue->queue, 0, sizeof(pQueue->queue));

    return ntStatus;
}

NTSTATUS
SrvProdConsEnqueue(
    PSMB_PROD_CONS_QUEUE pQueue,
    PVOID                pItem
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pQueue->mutex);

    ntStatus = SMBEnqueue(pQueue, pItem);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    pthread_cond_broadcast(&pQueue->event);

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return ntStatus;
}

NTSTATUS
SrvProdConsDequeue(
    PSMB_PROD_CONS_QUEUE pQueue,
    PVOID*               ppItem
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    PVOID    pItem = NULL;

    do
    {
        SMB_LOCK_MUTEX(bInLock, &pQueue->mutex);

        if (!SMBQueueIsEmpty(pQueue))
        {
            ntStatus = SMBDequeue(pQueue, &pItem);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            pthread_cond_wait(&pQueue->event, &pQueue->mutex);
        }

    } while (!pItem);

    *ppItem = pItem;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return ntStatus;

error:

    *ppItem = NULL;

    goto cleanup;
}

VOID
SrvProdConsFreeContents(
    PSMB_PROD_CONS_QUEUE pQueue
    )
{
    NTSTATUS ntStatus = 0;

    pthread_mutex_lock(&pQueue->mutex);

    if (pQueue->pEvent)
    {
        pthread_cond_destroy(&pQueue->event);
        pQueue->pEvent = NULL;
    }

    pthread_mutex_unlock(&pQueue->mutex);

    // TODO: Free Queue

    return ntStatus;
}
