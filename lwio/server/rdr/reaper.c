#include "rdr.h"

#define INVALID_TIME ((time_t) -1)

static
void*
RdrReaperThread(
    void* pData
    );

static
NTSTATUS
RdrReaperReapGlobal(
    PRDR_GLOBAL_RUNTIME pRuntime,
    time_t currentTime,
    time_t* pNextWakeupTime
    );

static
NTSTATUS
RdrReaperReapSocket(
    PRDR_GLOBAL_RUNTIME pRuntime,
    PSMB_SOCKET pSocket,
    time_t currentTime,
    time_t* pNextWakeupTime
    );

static
NTSTATUS
RdrReaperReapSession(
    PRDR_GLOBAL_RUNTIME pRuntime,
    PSMB_SESSION pSession,
    time_t currentTime,
    time_t* pNextWakeupTime
    );

NTSTATUS
RdrReaperInit(
    PRDR_GLOBAL_RUNTIME pRuntime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pthread_mutex_init(&pRuntime->reaperMutex, NULL);
    pthread_cond_init(&pRuntime->reaperEvent, NULL);

    pRuntime->expirationTime = 10;

    pthread_create(
        &pRuntime->reaperThread,
        NULL,
        RdrReaperThread,
        pRuntime);

    return ntStatus;
}

NTSTATUS
RdrReaperShutdown(
    PRDR_GLOBAL_RUNTIME pRuntime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pthread_mutex_lock(&pRuntime->reaperMutex);
    pRuntime->bShutdown = TRUE;
    pthread_cond_signal(&pRuntime->reaperEvent);
    pthread_mutex_unlock(&pRuntime->reaperMutex);
    pthread_join(pRuntime->reaperThread, NULL);

    return ntStatus;
}

static
void*
RdrReaperThread(
    void* pData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PRDR_GLOBAL_RUNTIME pRuntime = pData;
    time_t currentTime = 0;
    time_t nextWakeupTime = INVALID_TIME;
    BOOLEAN bInReaperLock = FALSE;
    struct timespec wakeTime = {0};

    SMB_LOCK_MUTEX(bInReaperLock, &pRuntime->reaperMutex);

    while (!pRuntime->bShutdown)
    {
        if (nextWakeupTime != INVALID_TIME)
        {
            do
            {
                wakeTime.tv_sec = nextWakeupTime;
                wakeTime.tv_nsec = 0;

                pthread_cond_timedwait(
                    &pRuntime->reaperEvent,
                    &pRuntime->reaperMutex,
                    &wakeTime
                    );

                currentTime = time(NULL);
            } while (!pRuntime->bShutdown && currentTime < nextWakeupTime);
        }
        else
        {
            pthread_cond_wait(&pRuntime->reaperEvent, &pRuntime->reaperMutex);
            currentTime = time(NULL);
        }

        if (!pRuntime->bShutdown)
        {
            nextWakeupTime = INVALID_TIME;

            ntStatus = RdrReaperReapGlobal(
                pRuntime,
                currentTime,
                &nextWakeupTime);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

error:

    return NULL;
}

static
NTSTATUS
RdrReaperSnapshotHashValues(
    PSMB_HASH_TABLE pHash,
    void*** pppSnapshot
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    void** ppSnapshot = NULL;
    void** ppNewSnapshot = NULL;
    SMB_HASH_ITERATOR iter = {0};
    SMB_HASH_ENTRY* pEntry = NULL;
    ULONG ulIndex = 0;
    ULONG ulCapacity = 0;

    ntStatus = SMBHashGetIterator(pHash, &iter);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        pEntry = SMBHashNext(&iter);

        if (ulIndex >= ulCapacity)
        {
            if (ulCapacity == 0)
            {
                ulCapacity = 8;
            }
            else
            {
                ulCapacity *= 2;
            }

            ppNewSnapshot = LwRtlMemoryRealloc(ppSnapshot, ulCapacity);
            if (!ppNewSnapshot)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ppSnapshot = ppNewSnapshot;
        }

        if (pEntry)
        {
            ppSnapshot[ulIndex++] = pEntry->pValue;
        }
        else
        {
            ppSnapshot[ulIndex] = NULL;
        }
    } while (pEntry != NULL);

    *pppSnapshot = ppSnapshot;

cleanup:

    return ntStatus;

error:

    *pppSnapshot = NULL;

    RTL_FREE(&ppSnapshot);

    goto cleanup;
}

static
BOOLEAN
RdrReaperIsExpired(
    time_t lastActivity,
    time_t currentTime,
    time_t expirationTime
    )
{
    return lastActivity + expirationTime >= currentTime;
}

VOID
RdrReaperUpdateNextWakeTime(
    time_t lastActivity,
    time_t expirationTime,
    time_t* pNextWakeupTime
    )
{
    if (*pNextWakeupTime == INVALID_TIME ||
        *pNextWakeupTime > lastActivity + expirationTime)
    {
        *pNextWakeupTime = lastActivity + expirationTime;
    }
}

static
NTSTATUS
RdrReaperReapGlobal(
    PRDR_GLOBAL_RUNTIME pRuntime,
    time_t currentTime,
    time_t* pNextWakeupTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_SOCKET* pSocketList = NULL;
    PSMB_SOCKET* pSocketIter = NULL;
    PSMB_SOCKET pSocket = NULL;
    BOOLEAN bInHashLock = FALSE;
    time_t expirationTime = pRuntime->expirationTime;

    /* Within lock, obtain a snapshot of all sockets and take a reference to each */
    SMB_LOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);

    ntStatus = RdrReaperSnapshotHashValues(
        pRuntime->pSocketHashByName,
        (void***) (void*) &pSocketList);
    BAIL_ON_NT_STATUS(ntStatus);

    for (pSocketIter = pSocketList; *pSocketIter; pSocketIter++)
    {
         pSocket = *pSocketIter;
         pSocket->refCount++;
    }

    SMB_UNLOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);

    /* Reap sessions within each socket */
    for (pSocketIter = pSocketList; *pSocketIter; pSocketIter++)
    {
        pSocket = *pSocketIter;

        ntStatus = RdrReaperReapSocket(
            pRuntime,
            pSocket,
            currentTime,
            pNextWakeupTime);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pSocketList)
    {
        /* Within lock, remove reference to each socket.  If the
           socket is expired, destroy it.  Otherwise, update the
           time we should next wake at */
        SMB_LOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);

        for (pSocketIter = pSocketList; *pSocketIter; pSocketIter++)
        {
            pSocket = *pSocketIter;

            if (--pSocket->refCount == 0 &&
                RdrReaperIsExpired(pSocket->lastActiveTime,
                                   currentTime,
                                   expirationTime))
            {
                SMBHashRemoveKey(
                    pRuntime->pSocketHashByName,
                    pSocket->pszHostname
                    );
            }
            else
            {
                /* Socket is not to be reaped, so take it
                   out of the list and calculate the next
                   wakeup time */
                *pSocketIter = NULL;

                RdrReaperUpdateNextWakeTime(
                    pSocket->lastActiveTime,
                    expirationTime,
                    pNextWakeupTime);
            }
        }

        SMB_UNLOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);
    }

    for (pSocketIter = pSocketList; *pSocketIter; pSocketIter++)
    {
        pSocket = *pSocketIter;
        if (pSocket)
        {
            SMBSocketFree(pSocket);
        }
    }

    RTL_FREE(&pSocketList);

    return ntStatus;

error:

    SMB_UNLOCK_MUTEX(bInHashLock, &pRuntime->socketHashLock);

    goto cleanup;
}

static
NTSTATUS
RdrReaperReapSocket(
    PRDR_GLOBAL_RUNTIME pRuntime,
    PSMB_SOCKET pSocket,
    time_t currentTime,
    time_t* pNextWakeupTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_SESSION* pSessionList = NULL;
    PSMB_SESSION* pSessionIter = NULL;
    PSMB_SESSION pSession = NULL;
    BOOLEAN bInSocketLock = FALSE;
    time_t expirationTime = pRuntime->expirationTime;

    /* Within lock, obtain a snapshot of all sessions and take a reference to each */
    SMB_LOCK_MUTEX(bInSocketLock, &pSocket->mutex);

    ntStatus = RdrReaperSnapshotHashValues(
        pSocket->pSessionHashByUID,
        (void***) (void*) &pSessionList);
    BAIL_ON_NT_STATUS(ntStatus);

    for (pSessionIter = pSessionList; *pSessionIter; pSessionIter++)
    {
         pSession = *pSessionIter;
         pSession->refCount++;
    }

    SMB_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);

    /* Reap trees within each session */
    for (pSessionIter = pSessionList; *pSessionIter; pSessionIter++)
    {
        pSession = *pSessionIter;

        ntStatus = RdrReaperReapSession(
            pRuntime,
            pSession,
            currentTime,
            pNextWakeupTime);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pSessionList)
    {
        /* Within lock, remove reference to each session.  If the
           session is expired, destroy it.  Otherwise, update the
           time we should next wake at */
        SMB_LOCK_MUTEX(bInSocketLock, &pSocket->mutex);

        for (pSessionIter = pSessionList; *pSessionIter; pSessionIter++)
        {
            pSession = *pSessionIter;

            if (--pSession->refCount == 0 &&
                RdrReaperIsExpired(pSession->lastActiveTime,
                                   currentTime,
                                   expirationTime))
            {
                SMBHashRemoveKey(
                    pSocket->pSessionHashByUID,
                    &pSession->uid
                    );
                SMBHashRemoveKey(
                    pSocket->pSessionHashByPrincipal,
                    pSession->pszPrincipal
                    );
            }
            else
            {
                /* Session is not to be reaped, so take it
                   out of the list and calculate the next
                   wakeup time */
                *pSessionIter = NULL;

                RdrReaperUpdateNextWakeTime(
                    pSession->lastActiveTime,
                    expirationTime,
                    pNextWakeupTime);
            }
        }

        SMB_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);
    }

    for (pSessionIter = pSessionList; *pSessionIter; pSessionIter++)
    {
        pSession = *pSessionIter;
        if (pSession)
        {
            /* Attempt to log off session, ignoring errors */
            Logoff(pSession);
            SMBSessionFree(pSession);
        }
    }

    RTL_FREE(&pSessionList);

    return ntStatus;

error:

    SMB_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);

    goto cleanup;
}

static
NTSTATUS
RdrReaperReapSession(
    PRDR_GLOBAL_RUNTIME pRuntime,
    PSMB_SESSION pSession,
    time_t currentTime,
    time_t* pNextWakeupTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_TREE* pTreeList = NULL;
    PSMB_TREE* pTreeIter = NULL;
    PSMB_TREE pTree = NULL;
    BOOLEAN bInSessionLock = FALSE;
    time_t expirationTime = pRuntime->expirationTime;

    SMB_LOCK_MUTEX(bInSessionLock, &pSession->mutex);

    ntStatus = RdrReaperSnapshotHashValues(
        pSession->pTreeHashByTID,
        (void***) (void*) &pTreeList);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pTreeList)
    {
        for (pTreeIter = pTreeList; *pTreeIter; pTreeIter++)
        {
            pTree = *pTreeIter;

            if (pTree->refCount == 0 &&
                RdrReaperIsExpired(pTree->lastActiveTime,
                                   currentTime,
                                   expirationTime))
            {
                /* We can't remove the tree from
                   the TID hash because the receiver
                   thread will need it to deliver the
                   tree disconnect response.  It will
                   be removed in a separate pass.  We
                   can still remove it from the path
                   hash to prevent anyone else trying
                   to use it for new requests */
                SMBHashRemoveKey(
                    pSession->pTreeHashByPath,
                    pTree->pszPath
                    );
            }
            else
            {
                *pTreeIter = NULL;

                RdrReaperUpdateNextWakeTime(
                    pTree->lastActiveTime,
                    expirationTime,
                    pNextWakeupTime);
            }
        }
    }

    SMB_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);

    /* Pass one -- disconnect each tree outside the lock.
       We can't free or remove the tree from the session
       TID hash until we have received the disconnect reply */
    for (pTreeIter = pTreeList; *pTreeIter; pTreeIter++)
    {
        pTree = *pTreeIter;
        if (pTree)
        {
            TreeDisconnect(pTree);
        }
    }

    SMB_LOCK_MUTEX(bInSessionLock, &pSession->mutex);

    /* Pass two -- remove and free each tree now that they
       are all disconnected */
    for (pTreeIter = pTreeList; *pTreeIter; pTreeIter++)
    {
        pTree = *pTreeIter;
        if (pTree)
        {
            SMBHashRemoveKey(
                pSession->pTreeHashByTID,
                &pTree->tid
                );

            SMBTreeFree(pTree);
        }
    }

    SMB_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);

    RTL_FREE(&pTreeList);

    return ntStatus;

error:

    goto cleanup;
}
