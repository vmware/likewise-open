#include "includes.h"

/* @todo: support internationalized principals */
DWORD
SMBSrvClientSessionCreate(
    PSMB_SOCKET   pSocket,
    uchar8_t      *pszPrincipal,
    PSMB_SESSION* ppSession
    )
{
    DWORD dwError = 0;
    PSMB_SESSION pSession = NULL;
    BOOLEAN bAddedByPrincipal = FALSE;

    dwError = SMBSocketFindSessionByPrincipal(
                    pSocket,
                    pszPrincipal,
                    &pSession);
    if (!dwError)
    {
        goto done;
    }

    dwError = SMBSessionCreate(&pSession);
    BAIL_ON_SMB_ERROR(dwError);

    pSession->pSocket = pSocket;

    SMB_SAFE_FREE_MEMORY(pSession->pszPrincipal);

    /* Principal is trusted */
    dwError = SMBStrndup(
                    (char *) pszPrincipal,
                    strlen((char *) pszPrincipal) + sizeof(NUL),
                    (char **) &pSession->pszPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientSocketAddSessionByPrincipal(pSocket, pSession);
    BAIL_ON_SMB_ERROR(dwError);

    bAddedByPrincipal = TRUE;

    pSession->bSignedMessagesSupported = pSocket->bSignedMessagesSupported;
    pSession->bSignedMessagesRequired = pSocket->bSignedMessagesRequired;

    dwError = SessionSetup(
                    pSocket,
                    SMBSrvClientSessionSignMessages(pSession),
                    pSocket->pSessionKey,
                    pSocket->dwSessionKeyLength,
                    &pSession->uid,
                    &pSession->pSessionKey,
                    &pSession->dwSessionKeyLength,
                    &pSession->hSMBGSSContext);
    BAIL_ON_SMB_ERROR(dwError);

    if (!pSocket->pSessionKey && pSession->pSessionKey)
    {
        dwError = SMBAllocateMemory(
                        pSession->dwSessionKeyLength,
                        (PVOID*)&pSocket->pSessionKey);
        BAIL_ON_SMB_ERROR(dwError);

        memcpy(pSocket->pSessionKey, pSession->pSessionKey, pSession->dwSessionKeyLength);

        pSocket->dwSessionKeyLength = pSession->dwSessionKeyLength;
    }

    /* Set state and awake any waiting threads */
    SMBSessionSetState(pSession, SMB_RESOURCE_STATE_VALID);

    dwError = SMBSrvClientSocketAddSessionByUID(pSocket, pSession);
    BAIL_ON_SMB_ERROR(dwError);

done:

    *ppSession = pSession;

cleanup:

    return dwError;

error:

    *ppSession = NULL;

    if (pSession)
    {
        if (bAddedByPrincipal)
        {
            SMBSrvClientSocketRemoveSessionByPrincipal(pSocket, pSession);
        }

        SMBSessionInvalidate(pSession, ERROR_SMB, dwError);

        SMBSessionRelease(pSession);
    }

    goto cleanup;
}


/* Must be called with the session mutex held */
DWORD
SMBSrvClientSessionIsStale_inlock(
    PSMB_SESSION pSession,
    PBOOLEAN     pbIsStale
    )
{
    DWORD dwError = 0;
    SMB_HASH_ITERATOR iterator;
    BOOLEAN bIsStale = FALSE;

    if (pSession->refCount > 2)
    {
        goto done;
    }

    dwError = SMBHashGetIterator(
                    pSession->pTreeHashByPath,
                    &iterator);
    BAIL_ON_SMB_ERROR(dwError);

    if (SMBHashNext(&iterator))
    {
        goto done;
    }

    /* @todo: find a tick function which won't jump backward */
    /* @todo: make idle time configurable */
    if (difftime(time(NULL), pSession->lastActiveTime) < 60*15)
    {
        goto done;
    }

    bIsStale = TRUE;

done:

    *pbIsStale = bIsStale;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBSrvClientSessionAddTreeById(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->hashLock);

    /* No need to check for a race here; the path hash is always checked
       first */
    dwError = SMBHashSetValue(
                    pSession->pTreeHashByTID,
                    &pTree->tid,
                    pTree);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSession->hashLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBSrvClientSessionRemoveTreeById(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->hashLock);

    dwError = SMBHashRemoveKey(
                    pSession->pTreeHashByTID,
                    &pTree->tid);
    BAIL_ON_SMB_ERROR(dwError);

    SMBSessionUpdateLastActiveTime(pSession);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSession->hashLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBSrvClientSessionAddTreeByPath(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->hashLock);

    /* @todo: check for race */
    dwError = SMBHashSetValue(
                    pSession->pTreeHashByPath,
                    pTree->pszPath,
                    pTree);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSession->hashLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBSrvClientSessionRemoveTreeByPath(
    PSMB_SESSION pSession,
    PSMB_TREE    pTree
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->hashLock);

    dwError = SMBHashRemoveKey(pSession->pTreeHashByPath, pTree->pszPath);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSession->hashLock);

    return dwError;

error:

    goto cleanup;
}

BOOLEAN
SMBSrvClientSessionSignMessages(
    PSMB_SESSION pSession
    )
{
    // TODO: Grab the config lock when available
    return (pSession->bSignedMessagesRequired || (pSession->bSignedMessagesSupported && gSignMessagesIfSupported));
}

DWORD
SMBSrvClientSessionRelease(
    PSMB_SESSION pSession
    )
{
    DWORD dwError = 0;

    /** @todo: keep unused sockets around for a little while when daemonized.
     * To avoid writing frequently to shared cache lines, perhaps set a bit
     * when the hash transitions to non-empty, then periodically sweep for
     * empty hashes.  If a hash is empty after x number of timed sweeps, tear
     * down the parent.
     */

    /* @todo: verify that the tree hash is empty */
    dwError = Logoff(pSession);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMBSessionRelease(pSession);

    return dwError;

error:

    goto cleanup;
}
