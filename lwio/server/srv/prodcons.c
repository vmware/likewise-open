#include "includes.h"

NTSTATUS
SrvProdConsInit(
    ULONG                         ulNumMaxItems,
    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem,
    PSMB_PROD_CONS_QUEUE*         ppQueue
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PROD_CONS_QUEUE pQueue = NULL;

    if (!ulNumMaxItems)
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
                    sizeof(SMB_PROD_CONS_QUEUE),
                    (PVOID*)&pQueue);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProdConsInitContents(
                    pQueue,
                    ulNumMaxItems,
                    pfnFreeItem);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppQueue = pQueue;

cleanup:

    return ntStatus;

error:

    *ppQueue = NULL;

    if (pQueue)
    {
        SrvProdConsFree(pQueue);
    }

    goto cleanup;
}

NTSTATUS
SrvProdConsInitContents(
    PSMB_PROD_CONS_QUEUE          pQueue,
    ULONG                         ulNumMaxItems,
    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem
    )
{
    NTSTATUS ntStatus = 0;

    memset(pQueue, 0, sizeof(*pQueue));

    pthread_mutex_init(&pQueue->mutex, NULL);
    pQueue->pMutex = &pQueue->mutex;

    pQueue->ulNumMaxItems = ulNumMaxItems;
    pQueue->pfnFreeItem = pfnFreeItem;

    pthread_cond_init(&pQueue->event, NULL);
    pQueue->pEvent = &pQueue->event;

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
    BOOLEAN  bSignalEvent = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        pthread_cond_wait(pQueue->pEvent, &pQueue->mutex);
    }

    ntStatus = SMBEnqueue(&pQueue->queue, pItem);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pQueue->ulNumItems)
    {
        bSignalEvent = TRUE;
    }

    pQueue->ulNumItems++;

    SMB_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    if (bSignalEvent)
    {
        pthread_cond_broadcast(pQueue->pEvent);
    }

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return ntStatus;

error:

    goto cleanup;
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
    BOOLEAN  bSignalEvent = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (!pQueue->ulNumItems)
    {
        pthread_cond_wait(pQueue->pEvent, &pQueue->mutex);
    }

    pItem = SMBDequeue(&pQueue->queue);

    if (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        bSignalEvent = TRUE;
    }

    pQueue->ulNumItems--;

    SMB_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    if (bSignalEvent)
    {
        pthread_cond_broadcast(pQueue->pEvent);
    }

    *ppItem = pItem;

    return ntStatus;
}

NTSTATUS
SrvProdConsTimedDequeue(
    PSMB_PROD_CONS_QUEUE pQueue,
    struct timespec*     pTimespec,
    PVOID*               ppItem
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    PVOID    pItem = NULL;

    SMB_LOCK_MUTEX(bInLock, &pQueue->mutex);

    if (!pQueue->ulNumItems)
    {
        BOOLEAN bRetryWait = FALSE;

        do
        {
            bRetryWait = FALSE;

            int unixErrorCode = pthread_cond_timedwait(
                                    pQueue->pEvent,
                                    &pQueue->mutex,
                                    pTimespec);
            if (unixErrorCode == ETIMEDOUT)
            {
                if (time(NULL) < pTimespec->tv_sec)
                {
                    bRetryWait = TRUE;
                    continue;
                }
            }

            ntStatus = LwUnixErrnoToNtStatus(unixErrorCode);
            BAIL_ON_NT_STATUS(ntStatus);

        } while (bRetryWait);
    }

    if (pQueue->ulNumItems)
    {
        BOOLEAN  bSignalEvent = FALSE;

        pItem = SMBDequeue(&pQueue->queue);

        if (pQueue->ulNumItems == pQueue->ulNumMaxItems)
        {
            bSignalEvent = TRUE;
        }

        pQueue->ulNumItems--;

        SMB_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

        if (bSignalEvent)
        {
            pthread_cond_broadcast(pQueue->pEvent);
        }
    }

    *ppItem = pItem;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return ntStatus;

error:

    *ppItem = NULL;

    goto cleanup;
}

VOID
SrvProdConsFree(
    PSMB_PROD_CONS_QUEUE pQueue
    )
{
    SrvProdConsFreeContents(pQueue);

    SMBFreeMemory(pQueue);
}

VOID
SrvProdConsFreeContents(
    PSMB_PROD_CONS_QUEUE pQueue
    )
{
    if (pQueue->pMutex)
    {
        pthread_mutex_lock(pQueue->pMutex);
    }

    if (pQueue->pEvent)
    {
        pthread_cond_destroy(pQueue->pEvent);
        pQueue->pEvent = NULL;
    }

    if (pQueue->pfnFreeItem)
    {
        PVOID pItem = NULL;

        while ((pItem = SMBDequeue(&pQueue->queue)) != NULL)
        {
            pQueue->pfnFreeItem(pItem);
        }
    }

    if (pQueue->pMutex)
    {
        pthread_mutex_unlock(&pQueue->mutex);
        pthread_mutex_destroy(pQueue->pMutex);
        pQueue->pMutex = NULL;
    }
}
