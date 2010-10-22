/*
 * Copyright Likewise Software    2004-2009
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        prodcons.c
 *
 * Abstract:
 *
 *        Likewise Input Output (LWIO) - SRV
 *
 *        Utilities
 *
 *        Producer Consumer Queue
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

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

    ntStatus = SrvAllocateMemory(
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

    LWIO_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        pthread_cond_wait(&pQueue->event, &pQueue->mutex);
    }

    ntStatus = SMBEnqueue(&pQueue->queue, pItem);
    BAIL_ON_NT_STATUS(ntStatus);

    pQueue->ulNumItems++;

    pthread_cond_signal(&pQueue->event);

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvProdConsEnqueueFront(
    PSMB_PROD_CONS_QUEUE pQueue,
    PVOID                pItem
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        pthread_cond_wait(&pQueue->event, &pQueue->mutex);
    }

    ntStatus = SMBEnqueueFront(&pQueue->queue, pItem);
    BAIL_ON_NT_STATUS(ntStatus);

    pQueue->ulNumItems++;

    pthread_cond_signal(&pQueue->event);

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

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

    LWIO_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (!pQueue->ulNumItems)
    {
        pthread_cond_wait(&pQueue->event, &pQueue->mutex);
    }

    pItem = SMBDequeue(&pQueue->queue);

    if (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        // Unblock any threads that may be waiting to queue.
        pthread_cond_broadcast(&pQueue->event);
    }

    pQueue->ulNumItems--;

    LWIO_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

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

    LWIO_LOCK_MUTEX(bInLock, &pQueue->mutex);

    while (!pQueue->ulNumItems)
    {
        BOOLEAN bRetryWait = FALSE;

        do
        {
            bRetryWait = FALSE;

            int unixErrorCode = pthread_cond_timedwait(
                                    &pQueue->event,
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

            ntStatus = LwErrnoToNtStatus(unixErrorCode);

            /* Don't use BAIL_ON_XXX() here to reduce unnecessary noise */
            if (ntStatus != STATUS_SUCCESS)
            {
                goto error;
            }
        } while (bRetryWait);
    }

    pItem = SMBDequeue(&pQueue->queue);

    if (pQueue->ulNumItems == pQueue->ulNumMaxItems)
    {
        // Unblock any threads that may be waiting to queue.
        pthread_cond_broadcast(&pQueue->event);
    }

    pQueue->ulNumItems--;

    LWIO_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

    *ppItem = pItem;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pQueue->mutex);

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

    SrvFreeMemory(pQueue);
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
        pthread_cond_destroy(&pQueue->event);
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
