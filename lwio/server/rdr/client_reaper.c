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

#include "rdr.h"

static
PVOID
SMBSrvClientReaperMain(
    PVOID pData
    );

static
NTSTATUS
SMBSrvClientReaperManageTree_inlock(
    PSMB_TREE pTree,
    PBOOLEAN  pbReaped
    );

static
NTSTATUS
SMBSrvClientReaperManageSession_inlock(
    PSMB_SESSION pSession,
    PBOOLEAN     pbReaped
    );

static
NTSTATUS
SMBSrvClientReaperManageSocket_inlock(
    PSMB_SOCKET pSocket,
    PBOOLEAN    pbReaped
    );

NTSTATUS
RdrReaperStart(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
    int iReaper = 0;

    /* The reaper threads will immediately block waiting for a non-empty
       socket hash. */
    for (; iReaper < 1; iReaper++)
    {
        pthread_t *pReaperThread = NULL;

        ntStatus = SMBAllocateMemory(
                        sizeof(pthread_t),
                        (PVOID *) &pReaperThread);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = pthread_create(
                        pReaperThread,
                        NULL,
                        &SMBSrvClientReaperMain,
                        NULL);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBStackPush(pReaperThread, &gRdrRuntime.pReaperStack);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

static
PVOID
SMBSrvClientReaperMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = 0;

    /* Execute an in-order traversal of the state tree.
     * Through careful refcounting, it is possible to backtrack and not deadlock.
     * Even if we run only one reaper, one must keep a seperate, ref'd
     * work queue because iterators may become invalidated between backtracks
     * when state is removed from a hash by a worker thread on error.
     */

    SMB_LOG_VERBOSE("Starting SMB Client Reaper");

    while (1)
    {
        SMB_HASH_ITERATOR iterator;
        SMB_HASH_ENTRY *pEntry = NULL;

        PSMB_STACK  pSocketStack = NULL;
        PSMB_SOCKET pSocket = NULL;

        PSMB_STACK   pSessionStack = NULL;
        PSMB_SESSION pSession = NULL;

        PSMB_STACK pTreeStack = NULL;
        PSMB_TREE  pTree = NULL;

        sleep(60);

        ntStatus = pthread_rwlock_rdlock(&gRdrRuntime.socketHashLock);
        if (ntStatus) goto error_socket_hash_lock;

        ntStatus = SMBHashGetIterator(gRdrRuntime.pSocketHashByAddress, &iterator);
        if (ntStatus) goto error_socket_hash_iterator;

        while ((pEntry = SMBHashNext(&iterator)))
        {
            pSocket = pEntry->pValue;

            SMBSocketAddReference(pSocket);

            ntStatus = SMBStackPush(pSocket, &pSocketStack);
            if (ntStatus) goto error_socket_hash_iterator;
        }

        ntStatus = pthread_rwlock_unlock(&gRdrRuntime.socketHashLock);
        BAIL_ON_NT_STATUS(ntStatus); /* @todo: ref. leaks! */

        while ((pSocket = (SMB_SOCKET *) SMBStackPop(&pSocketStack)))
        {
            BOOLEAN bSocketIsStale = false;
            BOOLEAN bReapedSocket = false;

            ntStatus = pthread_rwlock_rdlock(&pSocket->hashLock);
            if (ntStatus) goto error_session_hash_lock;

            ntStatus = SMBHashGetIterator(pSocket->pSessionHashByPrincipal,
                &iterator);
            if (ntStatus) goto error_session_hash_iterator;

            while ((pEntry = SMBHashNext(&iterator)))
            {
                pSession = pEntry->pValue;

                SMBSessionAddReference(pSession);

                ntStatus = SMBStackPush(pSession, &pSessionStack);
                if (ntStatus) goto error_session_hash_iterator;
            }

            ntStatus = pthread_rwlock_unlock(&pSocket->hashLock);
            BAIL_ON_NT_STATUS(ntStatus); /* @todo: ref. leaks! */

            while ((pSession = SMBStackPop(&pSessionStack)))
            {
                BOOLEAN bSessionIsStale = false;
                BOOLEAN bReapedSession = false;

                ntStatus = pthread_rwlock_rdlock(&pSession->hashLock);
                if (ntStatus) goto error_tree_hash_lock;

                ntStatus = SMBHashGetIterator(pSession->pTreeHashByPath,
                    &iterator);
                if (ntStatus) goto error_tree_hash_iterator;

                while ((pEntry = SMBHashNext(&iterator)))
                {
                    pTree = pEntry->pValue;

                    SMBTreeAddReference(pTree);

                    ntStatus = SMBStackPush(pTree, &pTreeStack);
                    if (ntStatus) goto error_tree_hash_iterator;
                }

                ntStatus = pthread_rwlock_unlock(&pSession->hashLock);
                BAIL_ON_NT_STATUS(ntStatus); /* @todo: ref. leaks! */

                while ((pTree = SMBStackPop(&pTreeStack)))
                {
                    BOOLEAN bTreeIsStale = false;
                    BOOLEAN bReapedTree = false;

                    ntStatus = SMBSrvClientTreeIsStale_inlock(pTree, &bTreeIsStale);
                    BAIL_ON_NT_STATUS(ntStatus);

                    if (!bTreeIsStale)
                    {
                        SMBTreeRelease(pTree);
                        pTree = NULL;
                        continue;
                    }

                    ntStatus = SMBSrvClientReaperManageTree_inlock(pTree, &bReapedTree);
                    BAIL_ON_NT_STATUS(ntStatus); /* @todo: ref. leaks! */

                    if (!bReapedTree)
                    {
                        SMBTreeRelease(pTree);
                        pTree = NULL;
                    }
                }

                ntStatus = SMBSrvClientSessionIsStale_inlock(pSession, &bSessionIsStale);
                BAIL_ON_NT_STATUS(ntStatus);

                if (!bSessionIsStale)
                {
                    SMBSessionRelease(pSession);
                    pSession = NULL;

                    continue;
                }

                ntStatus = SMBSrvClientReaperManageSession_inlock(pSession, &bReapedSession);
                BAIL_ON_NT_STATUS(ntStatus); /* @todo: ref. leaks! */

                if (!bReapedSession)
                {
                    SMBSessionRelease(pSession);
                    pSession = NULL;
                }
            }

            ntStatus = SMBSrvClientSocketIsStale_inlock(pSocket, &bSocketIsStale);
            BAIL_ON_NT_STATUS(ntStatus);

            if (!bSocketIsStale)
            {
                SMBSocketRelease(pSocket);
                pSocket = NULL;

                continue;
            }

            ntStatus = SMBSrvClientReaperManageSocket_inlock(pSocket, &bReapedSocket);
            BAIL_ON_NT_STATUS(ntStatus); /* @todo: ref. leaks! */

            if (!bReapedSocket)
            {
                SMBSocketRelease(pSocket);
                pSocket = NULL;
            }
        }

        continue;

error_tree_hash_iterator:
        pthread_rwlock_unlock(&pSession->hashLock);
        BAIL_ON_NT_STATUS(ntStatus);

error_tree_hash_lock:
        BAIL_ON_NT_STATUS(ntStatus);

error_session_hash_iterator:
        pthread_rwlock_unlock(&pSocket->hashLock);
        BAIL_ON_NT_STATUS(ntStatus);

error_session_hash_lock:
        BAIL_ON_NT_STATUS(ntStatus);

error_socket_hash_iterator:
        pthread_rwlock_unlock(&gRdrRuntime.socketHashLock);
        BAIL_ON_NT_STATUS(ntStatus);

error_socket_hash_lock:
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    SMB_LOG_VERBOSE("Concluding SMB Client Reaper");

    return 0;
}

/* Must be called holding the tree mutex */
static
NTSTATUS
SMBSrvClientReaperManageTree_inlock(
    PSMB_TREE pTree,
    PBOOLEAN  pbReaped
    )
{
    NTSTATUS ntStatus = 0;
    SMB_TREE *pFoundTree = NULL;
    SMB_SESSION *pSession = pTree->pSession;
    BOOLEAN bTreeIsStale = false;
    BOOLEAN bHashLocked = false;
    BOOLEAN bTreeInLock = FALSE;
    BOOLEAN bTreeSetupInLock = FALSE;
    BOOLEAN bReaped = false;

    /* Lock the parent session for write */
    ntStatus = pthread_rwlock_wrlock(&pSession->hashLock);
    BAIL_ON_NT_STATUS(ntStatus);
    bHashLocked = true;

    ntStatus = SMBHashGetValue(
                    pSession->pTreeHashByTID,
                    &pTree->tid,
                    (PVOID *) &pFoundTree);
    BAIL_ON_NT_STATUS(ntStatus);

    assert(pTree == pFoundTree);

    SMB_LOCK_MUTEX(bTreeInLock, &pTree->mutex);

    /* Check for race */
    ntStatus = SMBSrvClientTreeIsStale_inlock(pTree, &bTreeIsStale);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!bTreeIsStale)
    {
        goto out;   /* Lost race */
    }

    pTree->state = SMB_RESOURCE_STATE_TEARDOWN;

    SMB_UNLOCK_MUTEX(bTreeInLock, &pTree->mutex);

    /* Lock the tree connect mutex to prevent any new TIDs from being issued
       until this disconnect completes.  This reduces confusion should the
       server issue the same TID to a new request and the responses get handled
       out of order due to thread scheduling. */
    SMB_LOCK_MUTEX(bTreeSetupInLock, &pSession->treeMutex);

    /* Remove from the parent's forward (path) hash */
    ntStatus = SMBHashRemoveKey(
                    pSession->pTreeHashByPath,
                    pTree->pszPath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashRemoveKey(
                    pSession->pTreeHashByTID,
                    &pTree->tid);
    BAIL_ON_NT_STATUS(ntStatus);

    /* From this point on, any instance if this path in the tree hash is
       from a new thread wishing to connect.  They won't be able to add to
       the TID hash until the tree connect mutex is released. */
    ntStatus = pthread_rwlock_unlock(&pSession->hashLock);
    BAIL_ON_NT_STATUS(ntStatus);
    bHashLocked = false;

    /* Don't die if the tree disconnect fails; if the socket dies, the error
       will be appropriately propagated. */
    TreeDisconnect(pTree);

    /* Unlock disconnect mutex */
    SMB_UNLOCK_MUTEX(bTreeSetupInLock, &pSession->treeMutex);

    /* Now that we've reveived some kind of response (or error) remove from
       the parent back (TID) hash */
    ntStatus = SMBSrvClientSessionRemoveTreeById(pSession, pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBTreeRelease(pTree);
    /* The second release will call DestroyTree() */
    SMBTreeRelease(pTree);

    bReaped = true;

out:
    *pbReaped = bReaped;

error:
    if (bHashLocked)
    {
        pthread_rwlock_unlock(&pSession->hashLock);
    }

    SMB_UNLOCK_MUTEX(bTreeInLock, &pTree->mutex);

    SMB_UNLOCK_MUTEX(bTreeSetupInLock, &pSession->treeMutex);

    return ntStatus;
}

/* Must be called holding the session mutex */
static
NTSTATUS
SMBSrvClientReaperManageSession_inlock(
    PSMB_SESSION pSession,
    PBOOLEAN     pbReaped
    )
{
    NTSTATUS ntStatus = 0;
    SMB_SESSION *pFoundSession = NULL;
    SMB_SOCKET *pSocket = pSession->pSocket;
    BOOLEAN bSessionIsStale = false;
    BOOLEAN bHashLocked = false;
    BOOLEAN bSessionLocked = false;
    BOOLEAN bSessionSetupLocked = false;
    BOOLEAN bReaped = false;

    /* Lock the parent session for write */
    ntStatus = pthread_rwlock_wrlock(&pSocket->hashLock);
    BAIL_ON_NT_STATUS(ntStatus);
    bHashLocked = true;

    ntStatus = SMBHashGetValue(pSocket->pSessionHashByPrincipal,
        pSession->pszPrincipal, (PVOID *) &pFoundSession);
    BAIL_ON_NT_STATUS(ntStatus);
    assert(pSession == pFoundSession);

    ntStatus = pthread_mutex_lock(&pSession->mutex);
    BAIL_ON_NT_STATUS(ntStatus);
    bSessionLocked = true;

    /* Check for race */
    ntStatus = SMBSrvClientSessionIsStale_inlock(pSession, &bSessionIsStale);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!bSessionIsStale)
    {
        goto out;   /* Lost race */
    }

    pSession->state = SMB_RESOURCE_STATE_TEARDOWN;

    ntStatus = pthread_mutex_unlock(&pSession->mutex);
    BAIL_ON_NT_STATUS(ntStatus);
    bSessionLocked = false;

    /* Lock the session setup mutex to prevent any new UIDs from being issued
       until this disconnect completes.  This reduces confusion should the
       server issue the same UID to a new request and the responses get handled
       out of order due to thread scheduling. */
    ntStatus = pthread_mutex_lock(&pSocket->sessionMutex);
    BAIL_ON_NT_STATUS(ntStatus);
    bSessionSetupLocked = true;

    /* Remove from the parent's forward (principal) hash and UID hash.  Since
       the reponses won't have a tree relative MID, they'll be special cased
       like setup and don't need to be in the MID hash for correct routing of
       responses. */
    ntStatus = SMBHashRemoveKey(pSocket->pSessionHashByPrincipal,
        pSession->pszPrincipal);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashRemoveKey(pSocket->pSessionHashByUID, &pSession->uid);
    BAIL_ON_NT_STATUS(ntStatus);

    /* From this point on, any instance if this principal in the session
       hash is from a new thread wishing to connect.  They won't be able
       to add to the hashes until the session setup mutex is released. */
    ntStatus = pthread_rwlock_unlock(&pSocket->hashLock);
    BAIL_ON_NT_STATUS(ntStatus);
    bHashLocked = false;

    /* Don't die if the logoff; if the socket dies, the error will be
       appropriately propagated. */
    /* @todo: send logoff */

    /* Unlock disconnect mutex */
    ntStatus = pthread_mutex_unlock(&pSocket->sessionMutex);
    BAIL_ON_NT_STATUS(ntStatus);
    bSessionSetupLocked = false;

    SMBSessionRelease(pSession);
    /* The second release will call DestroySession() */
    SMBSessionRelease(pSession);

    bReaped = true;

out:
    *pbReaped = bReaped;

error:
    if (bHashLocked)
    {
        pthread_rwlock_unlock(&pSocket->hashLock);
    }

    if (bSessionLocked)
    {
        pthread_mutex_unlock(&pSession->mutex);
    }

    if (bSessionSetupLocked)
    {
        pthread_mutex_unlock(&pSocket->sessionMutex);
    }

    return ntStatus;
}

/* Must be called holding the tree mutex */
static
NTSTATUS
SMBSrvClientReaperManageSocket_inlock(
    PSMB_SOCKET pSocket,
    PBOOLEAN    pbReaped
    )
{
    SMB_SOCKET *pFoundSocket = NULL;
    BOOLEAN bSocketIsStale = false;
    BOOLEAN bHashLocked = false;
    BOOLEAN bSocketLocked = false;
    NTSTATUS ntStatus = 0;
    BOOLEAN bReaped = false;

    /* Lock the parent session for write */
    ntStatus = pthread_rwlock_wrlock(&gRdrRuntime.socketHashLock);
    BAIL_ON_NT_STATUS(ntStatus);
    bHashLocked = true;

    ntStatus = SMBHashGetValue(gRdrRuntime.pSocketHashByAddress, &pSocket->address,
        (PVOID *) &pFoundSocket);
    BAIL_ON_NT_STATUS(ntStatus);
    assert(pSocket == pFoundSocket);

    ntStatus = pthread_mutex_lock(&pSocket->mutex);
    BAIL_ON_NT_STATUS(ntStatus);
    bSocketLocked = true;

    /* Check for race */
    ntStatus = SMBSrvClientSocketIsStale_inlock(pSocket, &bSocketIsStale);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!bSocketIsStale)
    {
        goto out;   /* Lost race */
    }

    pSocket->state = SMB_RESOURCE_STATE_TEARDOWN;

    ntStatus = pthread_mutex_unlock(&pSocket->mutex);
    BAIL_ON_NT_STATUS(ntStatus);
    bSocketLocked = false;

    /* Remove from the global address and address hashes. */
    ntStatus = SMBHashRemoveKey(gRdrRuntime.pSocketHashByAddress, &pSocket->address);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashRemoveKey(gRdrRuntime.pSocketHashByName, pSocket->pszHostname);
    BAIL_ON_NT_STATUS(ntStatus);

    /* From this point on, any instance if this socket in the socket
       hash is from a new thread wishing to connect. */
    ntStatus = pthread_rwlock_unlock(&gRdrRuntime.socketHashLock);
    BAIL_ON_NT_STATUS(ntStatus);
    bHashLocked = false;

    /* No need to call close() here; it's handled in _DestroySocket() */

    SMBSocketRelease(pSocket);
    /* The second release will call _DestroySocket() */
    SMBSocketRelease(pSocket);

    bReaped = true;

out:
    *pbReaped = bReaped;

error:
    if (bHashLocked)
    {
        pthread_rwlock_unlock(&gRdrRuntime.socketHashLock);
    }

    if (bSocketLocked)
    {
        pthread_mutex_unlock(&pSocket->mutex);
    }

    return ntStatus;
}

NTSTATUS
RdrReaperStop(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
    pthread_t *pReaperThread = NULL;

    while ((pReaperThread = SMBStackPop(&gRdrRuntime.pReaperStack)))
    {
        pthread_cancel(*pReaperThread);

        pthread_join(*pReaperThread, NULL);

        SMBFreeMemory(pReaperThread);
    }

    return ntStatus;
}
