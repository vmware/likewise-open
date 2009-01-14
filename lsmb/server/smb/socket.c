/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
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
 *        socket.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LSMB)
 *
 *        Common Socket Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "includes.h"

static
int
SMBSocketHashSessionCompareByUID(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
SMBSocketHashSessionByUID(
    PCVOID vp
    );

static
void *
SMBSocketReaderMain(
    void *pData
    );

static
DWORD
SMBSocketFindAndSignalResponse(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    );

static
DWORD
SMBSocketFindSessionByUID(
    PSMB_SOCKET   pSocket,
    uint16_t      uid,
    PSMB_SESSION* ppSession
    );

static
DWORD
SMBSocketReceiveNegotiateOrSetupResponse(
    PSMB_SOCKET   pSocket,
    PSMB_PACKET* ppPacket
    );

static
DWORD
SMBSocketPacketAllocate(
    PSMB_SOCKET  pSocket,
    PSMB_PACKET* ppPacket
    );

static
VOID
SMBSocketFree(
    PSMB_SOCKET pSocket
    );

DWORD
SMBSrvSocketCreate(
    int fd,
    struct sockaddr_in clientAddr,
    PSMB_SOCKET* ppSocket
    )
{
    DWORD dwError = 0;
    SMB_SOCKET *pSocket = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroyHashLock = FALSE;
    BOOLEAN bDestroyMutex = FALSE;
    BOOLEAN bDestroyWriteMutex = FALSE;
    BOOLEAN bDestroySessionMutex = FALSE;

    dwError = SMBAllocateMemory(
                sizeof(SMB_SOCKET),
                (PVOID*)&pSocket);
    BAIL_ON_SMB_ERROR(dwError);

    pthread_mutex_init(&pSocket->mutex, NULL);
    bDestroyMutex = TRUE;

    pSocket->state = SMB_RESOURCE_STATE_INITIALIZING;
    pSocket->error.type = ERROR_SMB;
    pSocket->error.smb = SMB_ERROR_SUCCESS;

    dwError = pthread_cond_init(&pSocket->event, NULL);
    BAIL_ON_SMB_ERROR(dwError);

    bDestroyCondition = TRUE;

    pSocket->refCount = 1;  /* One for reaper */

    /* @todo: find a portable time call which is immune to host date and time
       changes, such as made by ntpd */
    pSocket->lastActiveTime = time(NULL);

    pthread_mutex_init(&pSocket->writeMutex, NULL);
    bDestroyMutex = TRUE;

    pSocket->fd = fd;

    pSocket->address = *((struct sockaddr*)&clientAddr);

    pSocket->maxBufferSize = 0;
    pSocket->maxRawSize = 0;
    pSocket->sessionKey = 0;
    pSocket->capabilities = 0;
    pSocket->pSecurityBlob = NULL;
    pSocket->securityBlobLen = 0;

    pSocket->pFreeBufferStack = NULL;
    pSocket->freeBufferCount = 0;
    pSocket->freeBufferLen = 0;
    pSocket->pFreePacketStack = NULL;
    pSocket->freePacketCount = 0;

    dwError = pthread_rwlock_init(&pSocket->hashLock, NULL);
    BAIL_ON_SMB_ERROR(dwError);

    bDestroyHashLock = TRUE;

    dwError = SMBHashCreate(
                    19,
                    SMBHashCaselessStringCompare,
                    SMBHashCaselessString,
                    NULL,
                    &pSocket->pSessionHashByPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBHashCreate(
                    19,
                    &SMBSocketHashSessionCompareByUID,
                    &SMBSocketHashSessionByUID,
                    NULL,
                    &pSocket->pSessionHashByUID);
    BAIL_ON_SMB_ERROR(dwError);

    pthread_mutex_init(&pSocket->sessionMutex, NULL);
    bDestroySessionMutex = TRUE;

    /* The reader thread will immediately block waiting for initialization */
    dwError = pthread_create(
                    &pSocket->readerThread,
                    NULL,
                    &SMBSocketReaderMain,
                    (void *) pSocket);
    BAIL_ON_SMB_ERROR(dwError);

    pSocket->pSessionPacket = NULL;

cleanup:

    return dwError;

error:

    if (pSocket)
    {
        SMBHashSafeFree(&pSocket->pSessionHashByUID);
        SMBHashSafeFree(&pSocket->pSessionHashByPrincipal);

        SMB_SAFE_FREE_MEMORY(pSocket->pszHostname);

        if (bDestroyCondition)
        {
            pthread_cond_destroy(&pSocket->event);
        }

        if (bDestroyHashLock)
        {
            pthread_rwlock_destroy(&pSocket->hashLock);
        }

        if (bDestroyWriteMutex)
        {
            pthread_mutex_destroy(&pSocket->writeMutex);
        }

        if (bDestroySessionMutex)
        {
            pthread_mutex_destroy(&pSocket->sessionMutex);
        }

        if (bDestroyMutex)
        {
            pthread_mutex_destroy(&pSocket->mutex);
        }

        SMBFreeMemory(pSocket);
    }

    *ppSocket = NULL;

    goto cleanup;
}

DWORD
SMBSocketCreate(
    struct addrinfo *address,
    uchar8_t        *pszHostname,
    PSMB_SOCKET*     ppSocket
    )
{
    DWORD dwError = 0;
    SMB_SOCKET *pSocket = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroyHashLock = FALSE;
    BOOLEAN bDestroyMutex = FALSE;
    BOOLEAN bDestroyWriteMutex = FALSE;
    BOOLEAN bDestroySessionMutex = FALSE;

    dwError = SMBAllocateMemory(
                sizeof(SMB_SOCKET),
                (PVOID*)&pSocket);
    BAIL_ON_SMB_ERROR(dwError);

    pthread_mutex_init(&pSocket->mutex, NULL);
    bDestroyMutex = TRUE;

    pSocket->state = SMB_RESOURCE_STATE_INITIALIZING;
    pSocket->error.type = ERROR_SMB;
    pSocket->error.smb = SMB_ERROR_SUCCESS;

    dwError = pthread_cond_init(&pSocket->event, NULL);
    BAIL_ON_SMB_ERROR(dwError);

    bDestroyCondition = TRUE;

    pSocket->refCount = 2;  /* One for reaper */

    /* @todo: find a portable time call which is immune to host date and time
       changes, such as made by ntpd */
    pSocket->lastActiveTime = time(NULL);

    pthread_mutex_init(&pSocket->writeMutex, NULL);
    bDestroyWriteMutex = TRUE;

    pSocket->fd = 0;

    /* Hostname is trusted */
    dwError = SMBStrndup(
                    (char *) pszHostname,
                    strlen((char *) pszHostname) + sizeof(NUL),
                    (char **) &pSocket->pszHostname);
    BAIL_ON_SMB_ERROR(dwError);

    pSocket->address = *address->ai_addr;

    pSocket->maxBufferSize = 0;
    pSocket->maxRawSize = 0;
    pSocket->sessionKey = 0;
    pSocket->capabilities = 0;
    pSocket->pSecurityBlob = NULL;
    pSocket->securityBlobLen = 0;

    pSocket->pFreeBufferStack = NULL;
    pSocket->freeBufferCount = 0;
    pSocket->freeBufferLen = 0;
    pSocket->pFreePacketStack = NULL;
    pSocket->freePacketCount = 0;

    dwError = pthread_rwlock_init(&pSocket->hashLock, NULL);
    BAIL_ON_SMB_ERROR(dwError);

    bDestroyHashLock = TRUE;

    dwError = SMBHashCreate(
                    19,
                    SMBHashCaselessStringCompare,
                    SMBHashCaselessString,
                    NULL,
                    &pSocket->pSessionHashByPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBHashCreate(
                    19,
                    &SMBSocketHashSessionCompareByUID,
                    &SMBSocketHashSessionByUID,
                    NULL,
                    &pSocket->pSessionHashByUID);
    BAIL_ON_SMB_ERROR(dwError);

    pthread_mutex_init(&pSocket->sessionMutex, NULL);
    bDestroySessionMutex = TRUE;

    /* The reader thread will immediately block waiting for initialization */
    dwError = pthread_create(
                    &pSocket->readerThread,
                    NULL,
                    &SMBSocketReaderMain,
                    (void *) pSocket);
    BAIL_ON_SMB_ERROR(dwError);

    pSocket->pSessionPacket = NULL;

    *ppSocket = pSocket;

cleanup:

    return dwError;

error:

    if (pSocket)
    {
        SMBHashSafeFree(&pSocket->pSessionHashByUID);
        SMBHashSafeFree(&pSocket->pSessionHashByPrincipal);

        SMB_SAFE_FREE_MEMORY(pSocket->pszHostname);

        if (bDestroyCondition)
        {
            pthread_cond_destroy(&pSocket->event);
        }

        if (bDestroyHashLock)
        {
            pthread_rwlock_destroy(&pSocket->hashLock);
        }

        if (bDestroyWriteMutex)
        {
            pthread_mutex_destroy(&pSocket->writeMutex);
        }

        if (bDestroySessionMutex)
        {
            pthread_mutex_destroy(&pSocket->sessionMutex);
        }

        if (bDestroyMutex)
        {
            pthread_mutex_destroy(&pSocket->mutex);
        }

        SMBFreeMemory(pSocket);

        SMBFreeMemory(pSocket);
    }

    *ppSocket = NULL;

    goto cleanup;
}

static
int
SMBSocketHashSessionCompareByUID(
    PCVOID vp1,
    PCVOID vp2
    )
{
    uint16_t uid1 = *((uint16_t *) vp1);
    uint16_t uid2 = *((uint16_t *) vp2);

    if (uid1 == uid2)
    {
        return 0;
    }
    else if (uid1 > uid2)
    {
        return 1;
    }

    return -1;
}

static
size_t
SMBSocketHashSessionByUID(
    PCVOID vp
    )
{
   return *((uint16_t *) vp);
}

BOOLEAN
SMBSocketTimedOut(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bTimedOut = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    bTimedOut = SMBSocketTimedOut_InLock(pSocket);

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return bTimedOut;
}

BOOLEAN
SMBSocketTimedOut_InLock(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bTimedOut = FALSE;
    DWORD   dwDiffSeconds = 0;

    /* Because we don't compare with the time of last write, only last read, a
       socket can become stale immediately after a request has been sent if
       the socket was previously idle.  We rely on the large timeout in
       SMBTreeReceiveResponse to smooth this over; we don't want to block
       forever on requests just because other threads are writing to the
       (possibly dead) socket. */
    dwDiffSeconds = difftime(time(NULL), pSocket->lastActiveTime);
    if (dwDiffSeconds > 30)
    {
        SMB_LOG_DEBUG("Socket timed out and was stale for [%d] seconds", dwDiffSeconds);
        bTimedOut = TRUE;
    }

    return bTimedOut;
}

DWORD
SMBSocketGetNextSequence(
    PSMB_SOCKET pSocket
    )
{
    DWORD dwSequence = 0;

    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    dwSequence = pSocket->dwSequence;
    // Next for response
    // Next for next message
    pSocket->dwSequence += 2;

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return dwSequence;
}

VOID
SMBSocketUpdateLastActiveTime(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->lastActiveTime = time(NULL);

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

static
PVOID
SMBSocketReaderMain(
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSMB_SOCKET pSocket = (PSMB_SOCKET) pData;
    BOOLEAN bInLock = FALSE;
    PSMB_PACKET pPacket = NULL;

    pthread_detach(pthread_self());

    SMB_LOG_INFO("Spawning socket reader thread for [%s]",
                 SMB_SAFE_LOG_STRING((char *) pSocket->pszHostname));

    /* Wait for thread to become ready */
    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    while (pSocket->state == SMB_RESOURCE_STATE_INITIALIZING)
    {
        pthread_cond_wait(&pSocket->event, &pSocket->mutex);
    }

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    /* When the ref. count drops to zero, pthread_cancel() breaks out of this
       loop */
    while (pSocket->state == SMB_RESOURCE_STATE_VALID)
    {
        int ret = 0;
        fd_set fdset;

        FD_SET(pSocket->fd, &fdset);

        ret = select(pSocket->fd + 1, &fdset, NULL, &fdset, NULL);
        if (ret == -1)
        {
            dwError = errno;
        }
        else if (ret != 1)
        {
            dwError = EFAULT;
        }
        BAIL_ON_SMB_ERROR(dwError);

        SMBSocketUpdateLastActiveTime(pSocket);

        dwError = SMBSocketPacketAllocate(pSocket, &pPacket);
        BAIL_ON_SMB_ERROR(dwError);

        dwError = SMBSocketBufferAllocate(
                    pSocket, 1024*64,
                    &pPacket->pRawBuffer,
                    &pPacket->bufferLen);
        BAIL_ON_SMB_ERROR(dwError);

        /* Read whole messages */
        dwError = SMBPacketReceiveAndUnmarshall(pSocket, pPacket);
        BAIL_ON_SMB_ERROR(dwError);

        if (pSocket->maxMpxCount && (sem_post(&pSocket->semMpx) < 0))
        {
            SMB_LOG_ERROR("Error when posting socket semaphore");
            dwError = errno;
            BAIL_ON_SMB_ERROR(dwError);
        }

        /* @todo: the client thread is responsible for calling FreePacket(),
           which will lock the socket and add the memory back to the free
           list, if appropriate. */
        /* This function should free packet and socket memory on error */
        dwError = SMBSocketFindAndSignalResponse(pSocket, pPacket);
        BAIL_ON_SMB_ERROR(dwError);

        pPacket = NULL;
    }

cleanup:

    if (pPacket)
    {
        if (pPacket->pRawBuffer)
        {
            SMBSocketBufferFree(pSocket, pPacket->pRawBuffer, pPacket->bufferLen);
        }

        SMBSocketPacketFree(pSocket, pPacket);
    }

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return NULL;

error:

    SMB_LOG_ERROR("Error when handling SMB socket[code:%d]", dwError);

    goto cleanup;
}

/* Ref. counting intermediate structures is not strictly necessary because
   there are currently no blocking operations in the response path; one could
   simply lock hashes all the way up the path */
static
DWORD
SMBSocketFindAndSignalResponse(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    DWORD dwError = 0;
    PSMB_SESSION  pSession  = NULL;
    PSMB_TREE     pTree     = NULL;
    PSMB_RESPONSE pResponse = NULL;
    BOOLEAN bSocketInLock = FALSE;

    /* If any intermediate object has an error status, then the
       waiting thread has (or will soon) be awoken with an error
       condition by the thread which set the original error. */

    if (pPacket->pSMBHeader->command == COM_NEGOTIATE ||
        pPacket->pSMBHeader->command == COM_SESSION_SETUP_ANDX ||
        pPacket->pSMBHeader->command == COM_LOGOFF_ANDX)
    {
        SMB_LOCK_MUTEX(bSocketInLock, &pSocket->mutex);

        assert(!pSocket->pSessionPacket);

        pSocket->pSessionPacket = pPacket;

        pthread_cond_broadcast(&pSocket->sessionEvent);

        SMB_UNLOCK_MUTEX(bSocketInLock, &pSocket->mutex);

        goto cleanup;
    }

    dwError = SMBSocketFindSessionByUID(
                    pSocket,
                    pPacket->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_SMB_ERROR(dwError);

    /* COM_TREE_DISCONNECT has a MID and is handled by the normal MID path. */
    if (pPacket->pSMBHeader->command == COM_TREE_CONNECT_ANDX)
    {
        BOOLEAN bSessionInLock = FALSE;

        SMB_LOCK_MUTEX(bSessionInLock, &pSession->mutex);

        assert(!pSession->pTreePacket);
        pSession->pTreePacket = pPacket;

        pthread_cond_broadcast(&pSession->treeEvent);

        SMB_UNLOCK_MUTEX(bSessionInLock, &pSession->mutex);

        goto cleanup;
    }

    dwError = SMBSessionFindTreeById(
                    pSession,
                    pPacket->pSMBHeader->tid,
                    &pTree);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBTreeFindLockedResponseByMID(
                    pTree,
                    pPacket->pSMBHeader->mid,
                    &pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    SMB_LOG_DEBUG("Found response [mid: %d] in Tree [0x%x] Socket [0x%x]", pPacket->pSMBHeader->mid, pTree, pSocket);

    pResponse->pPacket = pPacket;
    pResponse->state = SMB_RESOURCE_STATE_VALID;

    pthread_cond_broadcast(&pResponse->event);

cleanup:

    if (pResponse)
    {
        SMB_LOG_DEBUG("Unlocking response [mid: %d] in Tree [0x%x]", pResponse->mid, pTree);

        SMBResponseUnlock(pResponse);
    }

    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    if (pSession)
    {
        SMBSessionRelease(pSession);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SMBSocketFindSessionByUID(
    PSMB_SOCKET   pSocket,
    uint16_t      uid,
    PSMB_SESSION* ppSession
    )
{
    /* It is not necessary to ref. the socket here because we're guaranteed
       that the reader thread dies before the socket is destroyed */
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SESSION pSession = NULL;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pSocket->hashLock);

    dwError = SMBHashGetValue(
                    pSocket->pSessionHashByUID,
                    &uid,
                    (PVOID *) &pSession);
    BAIL_ON_SMB_ERROR(dwError);

    SMBSessionAddReference(pSession);

    *ppSession = pSession;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return dwError;

error:

    *ppSession = NULL;

    goto cleanup;
}

/* This must be called with the parent hash lock held to avoid a race with the
reaper. */
VOID
SMBSocketAddReference(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->refCount++;

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

/* @todo: catch signals? */
/* @todo: set socket option NODELAY for better performance */
DWORD
SMBSocketConnect(
    PSMB_SOCKET pSocket,
    struct addrinfo *ai
    )
{
    DWORD dwError = 0;
    int fd;
    fd_set fdset;
    struct timeval tv = { .tv_sec = 10, .tv_usec = 0 };
    int ret = 0;
    BOOLEAN bInLock = FALSE;

    if ((fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0)
    {
        dwError = errno;
    }
    BAIL_ON_SMB_ERROR(dwError);

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        dwError = errno;
    }
    BAIL_ON_SMB_ERROR(dwError);

    FD_ZERO(&fdset);
    FD_SET(fd, &fdset);

    if (connect(fd, ai->ai_addr, ai->ai_addrlen) && errno != EINPROGRESS)
    {
        dwError = errno;
    }
    BAIL_ON_SMB_ERROR(dwError);

    ret = select(fd + 1, NULL, &fdset, &fdset, &tv);
    if (ret == 0)
    {
        dwError = ETIMEDOUT;
    }
    else if (ret == -1)
    {
        dwError = errno;
    }
    else if (ret != 1)
    {
        dwError = EFAULT;
    }
    BAIL_ON_SMB_ERROR(dwError);

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->state = SMB_RESOURCE_STATE_INITIALIZING;
    pSocket->fd = fd;

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    pthread_cond_broadcast(&pSocket->event);

cleanup:

    return dwError;

error:

    if (fd < 0)
    {
        close(fd);
    }

    SMBSocketInvalidate(pSocket, ERROR_SMB, dwError);

    goto cleanup;
}

DWORD
SMBSocketRead(
    PSMB_SOCKET pSocket,
    uint8_t *buffer,
    uint32_t len,
    uint32_t *actualLen
    )
{
    DWORD dwError = 0;
    ssize_t totalRead = 0;
    ssize_t nRead = 0;
    fd_set fdset;

    FD_ZERO(&fdset);

    while (totalRead < len)
    {
        int ret = 0;
        /* Always ten seconds from last data read */
        struct timeval tv = { .tv_sec = 10, .tv_usec = 0 };

        FD_SET(pSocket->fd, &fdset);

        ret = select(pSocket->fd + 1, &fdset, NULL, &fdset, &tv);
        if (ret == 0)
        {
            dwError = ETIMEDOUT;
        }
        else if (ret == -1)
        {
            dwError = errno;
        }
        else if (ret != 1)
        {
            dwError = EFAULT;
        }
        BAIL_ON_SMB_ERROR(dwError);

        nRead = read(pSocket->fd, buffer + totalRead, len - totalRead);
        if(nRead < 0)
        {
            dwError = errno;
        }
        else if (nRead == 0)
        {
            /* EOF */
            goto cleanup;
        }

        totalRead += nRead;
    }

cleanup:

    *actualLen = totalRead;

    return dwError;

error:

    SMBSocketInvalidate(pSocket, ERROR_SMB, dwError);

    goto cleanup;
}

VOID
SMBSocketInvalidate(
    PSMB_SOCKET pSocket,
    SMB_ERROR_TYPE errorType,
    uint32_t networkError
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    SMBSocketInvalidate_InLock(
                    pSocket,
                    errorType,
                    networkError);

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

VOID
SMBSocketInvalidate_InLock(
    PSMB_SOCKET pSocket,
    SMB_ERROR_TYPE errorType,
    uint32_t networkError
    )
{
    pSocket->state = SMB_RESOURCE_STATE_INVALID;
    /* mark permanent error, continue */
    /* Permanent error?  Or temporary? */
    /* If temporary, retry? */
    pSocket->error.type = errorType;
    pSocket->error.smb = networkError;

    pthread_cond_broadcast(&pSocket->event);

    pthread_cond_broadcast(&pSocket->sessionEvent);
}

VOID
SMBSocketSetState(
    PSMB_SOCKET        pSocket,
    SMB_RESOURCE_STATE state
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->state = state;

    pthread_cond_broadcast(&pSocket->event);

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

DWORD
SMBSocketReceiveNegotiateResponse(
    PSMB_SOCKET   pSocket,
    PSMB_PACKET* ppPacket
    )
{
    return SMBSocketReceiveNegotiateOrSetupResponse(pSocket, ppPacket);
}

DWORD
SMBSocketReceiveSessionSetupResponse(
    PSMB_SOCKET   pSocket,
    PSMB_PACKET* ppPacket
    )
{
    return SMBSocketReceiveNegotiateOrSetupResponse(pSocket, ppPacket);
}

DWORD
SMBSocketReceiveLogoffResponse(
    PSMB_SOCKET   pSocket,
    PSMB_PACKET* ppPacket
    )
{
    return SMBSocketReceiveNegotiateOrSetupResponse(pSocket, ppPacket);
}

static
DWORD
SMBSocketReceiveNegotiateOrSetupResponse(
    PSMB_SOCKET   pSocket,
    PSMB_PACKET* ppPacket
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    struct timespec ts = { 0, 0 };

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* @todo: always check socket state for error */

    while (!pSocket->pSessionPacket)
    {

        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

retry_wait:

        /* @todo: always verify non-error state after acquiring mutex */
        dwError = pthread_cond_timedwait(
                        &pSocket->sessionEvent,
                        &pSocket->mutex,
                        &ts);
        if (dwError == ETIMEDOUT)
        {
            if (time(NULL) < ts.tv_sec)
            {
                dwError = 0;
                goto retry_wait;
            }

            /* As long as the socket is active, continue to wait.
             * otherwise, mark the socket as bad and return
             */
            if (SMBSocketTimedOut_InLock(pSocket))
            {
                SMBSocketInvalidate_InLock(pSocket, ERROR_SMB, ETIMEDOUT);
            }
            else
            {
                dwError = SMB_ERROR_SUCCESS;
            }
        }
        BAIL_ON_SMB_ERROR(dwError);
    }

    *ppPacket = pSocket->pSessionPacket;
    pSocket->pSessionPacket = NULL;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return dwError;

error:

    *ppPacket = NULL;

    goto cleanup;
}

DWORD
SMBSocketFindSessionByPrincipal(
    PSMB_SOCKET   pSocket,
    uint8_t      *pszPrincipal,
    PSMB_SESSION* ppSession
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SESSION pSession = NULL;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pSocket->hashLock);

    dwError = SMBHashGetValue(
                    pSocket->pSessionHashByPrincipal,
                    pszPrincipal,
                    (PVOID *) &pSession);
    BAIL_ON_SMB_ERROR(dwError);

    SMBSessionAddReference(pSession);

    *ppSession = pSession;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return dwError;

error:

    goto cleanup;
}

/* Hanging memory off the socket seems a reasonable tradeoff between
   concurrency and memory efficiency. */
DWORD
SMBSocketBufferAllocate(
    PSMB_SOCKET pSocket,
    size_t      len,
    uint8_t   **ppBuffer,
    size_t     *pAllocatedLen
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* If the len is greater than our current allocator len, adjust */
    if (len > pSocket->freeBufferLen)
    {
        SMBStackFree(pSocket->pFreeBufferStack);
        pSocket->pFreeBufferStack = NULL;

        pSocket->freeBufferLen = len;
    }

    if (pSocket->pFreeBufferStack)
    {
        *ppBuffer = (uint8_t *) pSocket->pFreeBufferStack;
        *pAllocatedLen = pSocket->freeBufferLen;
        SMBStackPopNoFree(&pSocket->pFreeBufferStack);
        memset(*ppBuffer, 0, *pAllocatedLen);
        pSocket->freeBufferCount--;
    }
    else
    {
        dwError = SMBAllocateMemory(
                        pSocket->freeBufferLen,
                        (PVOID *) ppBuffer);
        BAIL_ON_SMB_ERROR(dwError);

        *pAllocatedLen = pSocket->freeBufferLen;
    }

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return dwError;

error:

    goto cleanup;
}

VOID
SMBSocketBufferFree(
    PSMB_SOCKET pSocket,
    uint8_t    *pBuffer,
    size_t      bufferLen
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* If the len is greater than our current allocator len, adjust */
    /* @todo: make free list configurable */
    if (bufferLen == pSocket->freeBufferLen && pSocket->freeBufferCount < 10)
    {
        assert(bufferLen > sizeof(SMB_STACK));

        SMBStackPushNoAlloc(&pSocket->pFreeBufferStack, (PSMB_STACK) pBuffer);

        pSocket->freeBufferCount++;
    }
    else
    {
        SMBFreeMemory(pBuffer);
    }

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}


/* Hanging memory off the socket seems a reasonable tradeoff between
   concurrency and memory efficiency. */
static
DWORD
SMBSocketPacketAllocate(
    PSMB_SOCKET  pSocket,
    PSMB_PACKET* ppPacket
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_PACKET pPacket = NULL;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    if (pSocket->pFreePacketStack)
    {
        pPacket = (PSMB_PACKET) pSocket->pFreePacketStack;

        SMBStackPopNoFree(&pSocket->pFreePacketStack);

        pSocket->freePacketCount--;
    }
    else
    {
        dwError = SMBAllocateMemory(
                        sizeof(SMB_PACKET),
                        (PVOID *) &pPacket);
        BAIL_ON_SMB_ERROR(dwError);
    }

    *ppPacket = pPacket;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return dwError;

error:

    *ppPacket = NULL;

    goto cleanup;
}

VOID
SMBSocketPacketFree(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    BOOLEAN bInLock = FALSE;

    SMBSocketBufferFree(
                pSocket,
                pPacket->pRawBuffer,
                pPacket->bufferLen);

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* If the len is greater than our current allocator len, adjust */
    /* @todo: make free list configurable */
    if (pSocket->freePacketCount < 10)
    {
        assert(sizeof(SMB_PACKET) > sizeof(SMB_STACK));
        SMBStackPushNoAlloc(&pSocket->pFreePacketStack, (PSMB_STACK) pPacket);
        pSocket->freePacketCount++;
    }
    else
    {
        SMBFreeMemory(pPacket);
    }

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

/** @todo: keep unused sockets around for a little while when daemonized.
  * To avoid writing frequently to shared cache lines, perhaps set a bit
  * when the hash transitions to non-empty, then periodically sweep for
  * empty hashes.  If a hash is empty after x number of timed sweeps, tear
  * down the parent.
  */
/* This function does not decrement the reference count of the parent
   socket on destruction.  The reaper thread manages that with proper forward
   locking. */
VOID
SMBSocketRelease(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->refCount--;
    assert(pSocket->refCount >= 0);

    if (!pSocket->refCount)
    {
        SMBSocketFree(pSocket);

        bInLock = FALSE;
    }

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

/* This function does not remove a socket from it's parent hash; it merely
   frees the memory if the refcount is zero. */
/* @todo: pthread_join on socket reader thread */
static
VOID
SMBSocketFree(
    PSMB_SOCKET pSocket
    )
{
    assert(!pSocket->refCount);

    pthread_cancel(pSocket->readerThread);

    pthread_join(pSocket->readerThread, NULL);

    if ((pSocket->fd >= 0) && (close(pSocket->fd) < 0))
    {
        SMB_LOG_ERROR("Failed to close socket [fd:%d]", pSocket->fd);
    }

    if (pSocket && pSocket->maxMpxCount)
    {
        if (sem_destroy(&pSocket->semMpx) < 0)
        {
            SMB_LOG_ERROR("Failed to destroy semaphore [code: %d]", errno);
        }
    }

    pthread_cond_destroy(&pSocket->event);

    pthread_rwlock_destroy(&pSocket->hashLock);

    SMB_SAFE_FREE_MEMORY(pSocket->pszHostname);
    SMB_SAFE_FREE_MEMORY(pSocket->pSecurityBlob);

    if (pSocket->pFreeBufferStack)
    {
        SMBStackFree(pSocket->pFreeBufferStack);
    }

    if (pSocket->pFreePacketStack)
    {
        SMBStackFree(pSocket->pFreePacketStack);
    }

    /* @todo: assert that the session hashes are empty */
    SMBHashSafeFree(&pSocket->pSessionHashByPrincipal);
    SMBHashSafeFree(&pSocket->pSessionHashByUID);

    assert(!pSocket->pSessionPacket);

    pthread_cond_destroy(&pSocket->sessionEvent);

    pthread_mutex_destroy(&pSocket->writeMutex);
    pthread_mutex_destroy(&pSocket->sessionMutex);
    pthread_mutex_destroy(&pSocket->mutex);

    SMB_SAFE_FREE_MEMORY(pSocket->pSessionKey);

    /* @todo: use allocator */
    SMBFreeMemory(pSocket);
}
