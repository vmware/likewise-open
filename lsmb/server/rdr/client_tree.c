#include "includes.h"

static
DWORD
SMBSrvClientTreeCreate(
    PSMB_SESSION pSession,
    uchar8_t    *pszPath,
    PSMB_TREE*  ppTree
    );

DWORD
SMBSrvClientTreeOpen(
    PCSTR pszHostname,
    PCSTR pszPrincipal,
    PCSTR pszSharename,
    PSMB_TREE* ppTree
    )
{
    DWORD dwError = 0;
    PSMB_SOCKET pSocket = NULL;
    PSMB_SESSION pSession = NULL;
    PSMB_TREE pTree = NULL;

    dwError = SMBSrvClientSocketCreate(
                    (uchar8_t *) pszHostname,
                    &pSocket);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientSessionCreate(
                    pSocket,
                    (uchar8_t *) pszPrincipal,
                    &pSession);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientTreeCreate(
                    pSession,
                    (uchar8_t *) pszSharename,
                    &pTree);
    BAIL_ON_SMB_ERROR(dwError);

    SMBTreeAddReference(pTree);

    *ppTree = pTree;

cleanup:

    return dwError;

error:

    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    if (pSession)
    {
        SMBSrvClientSessionRelease(pSession);
    }

    if (pSocket)
    {
        SMBSocketRelease(pSocket);
    }

    goto cleanup;
}

static
DWORD
SMBSrvClientTreeCreate(
    PSMB_SESSION pSession,
    uchar8_t    *pszPath,
    PSMB_TREE*  ppTree
    )
{
    DWORD     dwError = 0;
    PSMB_TREE pTree = NULL;
    PWSTR     pwszPath = NULL;
    BOOLEAN   bAddedTreeByPath = FALSE;

    dwError = SMBSessionFindTreeByPath(
                    pSession,
                    pszPath,
                    &pTree);
    if (!dwError)
    {
        goto done;
    }

    dwError = SMBTreeCreate(&pTree);
    BAIL_ON_SMB_ERROR(dwError);

    pTree->pSession = pSession;

    SMB_SAFE_FREE_MEMORY(pTree->pszPath);

    /* Path is trusted */
    dwError = SMBStrndup(
                    (char *) pszPath,
                    strlen((char *) pszPath) + sizeof(NUL),
                    (char **) &pTree->pszPath);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvClientSessionAddTreeByPath(pSession, pTree);
    BAIL_ON_SMB_ERROR(dwError);

    bAddedTreeByPath = TRUE;

    /* @todo: once we can hash Unicode case-insensitively, we can remove this
       hack and go fully Unicode-native */
    dwError = SMBMbsToWc16s((char *) pszPath, &pwszPath);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = TreeConnect(pSession, pwszPath, &pTree->tid);
    BAIL_ON_SMB_ERROR(dwError);

    /* Set state and awake any waiting threads */
    /* @todo: move into TreeConenct */
    SMBTreeSetState(pTree, SMB_RESOURCE_STATE_VALID);

    dwError = SMBSrvClientSessionAddTreeById(pSession, pTree);
    BAIL_ON_SMB_ERROR(dwError);

done:

    *ppTree = pTree;

cleanup:

    SMB_SAFE_FREE_MEMORY(pwszPath);

    return dwError;

error:

    *ppTree = NULL;

    if (pTree)
    {
        if (bAddedTreeByPath)
        {
            SMBSrvClientSessionRemoveTreeByPath(pSession, pTree);
        }

        SMBTreeInvalidate(pTree, ERROR_SMB, dwError);

        SMBTreeRelease(pTree);
    }

    goto cleanup;
}

DWORD
SMBSrvClientTreeAddResponse(
    PSMB_TREE     pTree,
    PSMB_RESPONSE pResponse
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &pTree->mutex);

    /* @todo: if we allocate the MID outside of this function, we need to
       check for a conflict here */
    dwError = SMBHashSetValue(
                    pTree->pResponseHash,
                    &pResponse->mid,
                    pResponse);
    BAIL_ON_SMB_ERROR(dwError);

    if (pResponse->pTree)
    {
        SMBTreeRelease(pResponse->pTree);
    }
    SMBTreeAddReference(pTree);

    pResponse->pTree = pTree;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pTree->mutex);

    return dwError;

error:

    goto cleanup;
}

/* Must be called with the tree mutex held */
DWORD
SMBSrvClientTreeIsStale_inlock(
    PSMB_TREE pTree,
    PBOOLEAN  pbIsStale
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsStale = FALSE;
    SMB_HASH_ITERATOR iterator;

    if (pTree->refCount > 2)
    {
        goto done;
    }

    dwError = SMBHashGetIterator(
                    pTree->pResponseHash,
                    &iterator);
    BAIL_ON_SMB_ERROR(dwError);

    if (SMBHashNext(&iterator))
    {
        goto done;
    }

    /* @todo: find a tick function which won't jump backward */
    /* @todo: make idle time configurable */
    if (difftime(time(NULL), pTree->lastActiveTime) < 60*15)
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
SMBSrvClientTreeClose(
    PSMB_TREE pTree
    )
{
    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    return 0;
}
