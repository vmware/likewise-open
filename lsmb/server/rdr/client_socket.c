#include "includes.h"

static
int
SMBSrvClientHashSockaddrCompare(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
SMBSrvClientHashAddrinfo(
    PCVOID vp
    );

static
DWORD
_FindSocketByName(
    uchar8_t    *pszHostname,
    PSMB_SOCKET* ppSocket
    );

static
DWORD
_AddSocketByName(
    PSMB_SOCKET pSocket,
    uchar8_t   *pszHostname
    );

static
DWORD
_FindOrCreateSocket(
    struct addrinfo *addresses,
    uchar8_t        *pszHostname,
    PSMB_SOCKET*    ppSocket
    );

DWORD
RdrSocketInit(
    VOID
    )
{
    DWORD dwError = 0;

    assert(!gpSocketHashByName);
    assert(!gpSocketHashByAddress);

    /* @todo: support case and normalization insensitive string comparisons */
    /* Once we have libidn we'll also need the ability do Unicode case and
       normalization insensitive comparisons of strings.  Otherwise, if the
       string isn't an exact match a redundant DNS lookup would occur. Any
       existing connections to the resolved IP will be caught by the IPv4
       hash, but in a multiple A record DNS setup one could end up with
       multiple connections to a multihomed DCs or set of load-balanced DCs.
       The load balancing case should really be handled by SRV records
       parsed by the client.*/
    dwError = SMBHashCreate(
                    19,
                    SMBHashCaselessStringCompare,
                    SMBHashCaselessString,
                    NULL,
                    &gpSocketHashByName);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBHashCreate(
                    19,
                    &SMBSrvClientHashSockaddrCompare,
                    &SMBSrvClientHashAddrinfo,
                    NULL,
                    &gpSocketHashByAddress);
    BAIL_ON_SMB_ERROR(dwError);

error:

    return dwError;
}

/* @todo: add support for libidn for internationalized UNC hostnames */
/* @todo: handle IPv6 addresses */
/* The socket is returned with a reference */
DWORD
SMBSrvClientSocketCreate(
    uchar8_t    *pszHostname,
    PSMB_SOCKET* ppSocket
    )
{
    DWORD dwError = 0;
    PSMB_SOCKET pSocket = NULL;
    struct addrinfo *addresses = NULL;

    /* Check for the name in the name hash; if it's not there, resolve it */
    dwError = _FindSocketByName(pszHostname, &pSocket);
    if (dwError == ENOENT)
    {
        dwError = 0;
    }
    BAIL_ON_SMB_ERROR(dwError);

    if (!pSocket)
    {
        struct addrinfo hints;
        int s;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        /* @todo: make port configurable */
        s = getaddrinfo((char *) pszHostname, "445", &hints, &addresses);
        if (s != 0) {
            dwError = s;
        }
        BAIL_ON_SMB_ERROR(dwError);

        /* Check for the resolved IPs in the IP hash; if none are found,
           connect */
        dwError = _FindOrCreateSocket(addresses, pszHostname, &pSocket);
        BAIL_ON_SMB_ERROR(dwError);

        /* Whether we found an existing entry or not, add to the name hash */
        dwError = _AddSocketByName(pSocket, pszHostname);
        BAIL_ON_SMB_ERROR(dwError);

        dwError = Negotiate(pSocket);
        BAIL_ON_SMB_ERROR(dwError);

        /* Set state and awake any waiting threads */
        SMBSocketSetState(pSocket, SMB_RESOURCE_STATE_VALID);
    }

    *ppSocket = pSocket;

cleanup:

    if (addresses)
    {
        freeaddrinfo(addresses);
    }

    return dwError;

error:

    *ppSocket = NULL;

    goto cleanup;
}


/* Must be called with the session mutex held */
DWORD
SMBSrvClientSocketIsStale_inlock(
    PSMB_SOCKET pSocket,
    PBOOLEAN    pbIsStale
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsStale = FALSE;
    SMB_HASH_ITERATOR iterator;

    if (pSocket->refCount > 2)
    {
        goto done;
    }

    dwError = SMBHashGetIterator(
                    pSocket->pSessionHashByPrincipal,
                    &iterator);
    BAIL_ON_SMB_ERROR(dwError);

    if (SMBHashNext(&iterator))
    {
        goto done;
    }

    /* @todo: find a tick function which won't jump backward */
    /* @todo: make idle time configurable */
    if (difftime(time(NULL), pSocket->lastActiveTime) < 60*15)
    {
        goto done;
    }

    bIsStale = TRUE;

done:

    *pbIsStale = bIsStale;

error:

    return dwError;
}


/* Do not mix address types */
static
int
SMBSrvClientHashSockaddrCompare(
    PCVOID vp1,
    PCVOID vp2
    )
{
    struct sockaddr *pSa1 = (struct sockaddr *) vp1;
    struct sockaddr *pSa2 = (struct sockaddr *) vp2;

    return memcmp(&pSa1->sa_data, &pSa2->sa_data, sizeof(pSa1->sa_data));
}

/* @todo: hash on address family, too */
static
size_t
SMBSrvClientHashAddrinfo(
    PCVOID vp
    )
{
    struct sockaddr *pSa = (struct sockaddr *) vp;
    size_t addressLen = sizeof(pSa->sa_data);
    char *pData = pSa->sa_data;

    size_t result = 0;
    size_t chunkSize = sizeof(size_t);
    size_t rem = addressLen % chunkSize;
    size_t i = 0, j = 0;

    for (i = 0; i < addressLen / chunkSize; i++)
    {
        result ^= *((size_t*) (&pData[i*chunkSize]));
    }

    for (j = 0; j < rem; j++)
    {
        ((uint8_t*) &result)[j] ^= pData[i*chunkSize + j];
    }

    return result;
}

/* returns a socket with a reference */
static
DWORD
_FindSocketByName(
    uchar8_t    *pszHostname,
    PSMB_SOCKET* ppSocket
    )
{
    DWORD dwError = 0;
    PSMB_SOCKET pSocket = NULL;
    BOOLEAN bInLock = FALSE;

    /* This path exists only to avoid DNS lookups in the fast path.  If the
       socket is in any state but connected, we return NULL to force the
       lookup.  Site affinity is handled by the client. */

    /* @todo: add expiration to the name hash */
    SMB_LOCK_RWMUTEX_SHARED(bInLock, &gSocketHashLock);

    dwError = SMBHashGetValue(
                    gpSocketHashByName,
                    pszHostname,
                    (PVOID *) &pSocket);
    BAIL_ON_SMB_ERROR(dwError);

    SMBSocketAddReference(pSocket);

    *ppSocket = pSocket;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &gSocketHashLock);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
_AddSocketByName(
    PSMB_SOCKET pSocket,
    uchar8_t   *pszHostname
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSocketHashLock);

    /* We replace any existing entry on the grounds that new socket has been
       created from fresher DNS information */
    dwError = SMBHashSetValue(
                    gpSocketHashByName,
                    pSocket->pszHostname,
                    pSocket);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &gSocketHashLock);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
_FindOrCreateSocket(
    struct addrinfo *addresses,
    uchar8_t        *pszHostname,
    PSMB_SOCKET*    ppSocket
    )
{
    DWORD dwError = 0;

    /* In order to prevent returning an error to the client when we don't have
       to, we recheck the hash for a fresh, working connection in the address
       list every time we wake up from a failed sleep.  If the negative cache
       time is too small, then the reaper combined with other threads may
       livelock a thread attempting to finish running through the list of
       candidate addresses.  For this reason don't restart if the negative
       cache time is zero.
     */

    PSMB_SOCKET pSocket = NULL;
    struct addrinfo *ai = NULL;
    BOOLEAN bFirstPass = true;
    BOOLEAN bFoundValid = false;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bSocketInLock = FALSE;

restart:

    if (bFirstPass)
    {
        SMB_LOCK_RWMUTEX_SHARED(bInLock, &gSocketHashLock);
    }
    else
    {
        SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSocketHashLock);
    }

    for (ai = addresses; ai != NULL && !bFoundValid; ai = ai->ai_next)
    {
        dwError = SMBHashGetValue(
                        gpSocketHashByAddress,
                        (PCVOID *) ai,
                        (PVOID *) &pSocket);
        if (!dwError)
        {
            SMB_LOCK_MUTEX(bSocketInLock, &pSocket->mutex);

            switch (pSocket->state)
            {
                case SMB_RESOURCE_STATE_VALID:

                    pSocket->refCount++;
                    bFoundValid = true;
                    goto found;

                    break;

                case SMB_RESOURCE_STATE_INITIALIZING:

                    if (bFirstPass)
                        break;

                    pSocket->refCount++;

                    SMB_UNLOCK_RWMUTEX(bInLock, &gSocketHashLock);

                    pthread_cond_wait(
                            &pSocket->event,
                            &pSocket->mutex);

                    pSocket->refCount--; /* @todo: free on zero */

                    SMB_UNLOCK_MUTEX(bSocketInLock, &pSocket->mutex);

                    bFirstPass = true;

                    goto restart;

                case SMB_RESOURCE_STATE_INVALID:

                    /* No need to check error codes; only permanent errors
                       (negative cache entries) will remain in the hash. */
                    SMB_UNLOCK_MUTEX(bSocketInLock, &pSocket->mutex);

                    continue;

                default:

                    /* Invalid state */
                    assert(false);
            }

            SMB_UNLOCK_MUTEX(bSocketInLock, &pSocket->mutex);

        }
        else if (dwError != ENOENT)
        {
            BAIL_ON_SMB_ERROR(dwError);
        }
        dwError = 0;

        /* Not found; create in second pass */
        if (!bFirstPass)
        {
            dwError = SMBSocketCreate(ai, pszHostname, &pSocket);
            BAIL_ON_SMB_ERROR(dwError);

            /* add to hash */
            dwError = SMBHashSetValue(
                            gpSocketHashByAddress,
                            &pSocket->address,
                            pSocket);
            BAIL_ON_SMB_ERROR(dwError);

            /* Don't hold locks across a network operation */
            SMB_UNLOCK_RWMUTEX(bInLock, &gSocketHashLock);

            /* Attempt to connect */
            /* @todo: most likely a socket error is a resource issue;
               therefore, the address in question should not be added to
               the negative connection cache */
            /* Retry any transient errors? */
            dwError = SMBSocketConnect(pSocket, ai);
            if (dwError == ETIMEDOUT)
            {
                /* On timeout, rescan to find new sockets.  The error state
                   set by the failed connect ensures forward progress. */
                /* @todo: wake waiters */
                bFirstPass = true;
                goto restart;
            }
            else if (dwError)
            {
                continue;
            }

            bFoundValid = true;
            goto found;
        }
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &gSocketHashLock);

    if (!bFoundValid && bFirstPass)
    {
        bFirstPass = false;
        goto restart;
    }

found:

    *ppSocket = (bFoundValid ? pSocket : NULL);
    dwError = (bFoundValid ? 0 : ENOENT);

cleanup:

    return dwError;

error:

    *ppSocket = NULL;

    goto cleanup;
}

DWORD
SMBSrvClientSocketAddSessionByPrincipal(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSocket->hashLock);

    /* @todo: check for race */
    dwError = SMBHashSetValue(
                    pSocket->pSessionHashByPrincipal,
                    pSession->pszPrincipal,
                    pSession);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBSrvClientSocketRemoveSessionByPrincipal(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSocket->hashLock);

    dwError = SMBHashRemoveKey(
                    pSocket->pSessionHashByPrincipal,
                    pSession->pszPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBSrvClientSocketAddSessionByUID(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSocket->hashLock);

    /* No need to check for a race here; the principal hash is always checked
       first */
    dwError = SMBHashSetValue(
                    pSocket->pSessionHashByUID,
                    &pSession->uid,
                    pSession);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBSrvClientSocketRemoveSessionByUID(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSocket->hashLock);

    dwError = SMBHashRemoveKey(
                    pSocket->pSessionHashByUID,
                    &pSession->uid);
    BAIL_ON_SMB_ERROR(dwError);

    /* @todo: this need be set only when the hash is empty */
    SMBSocketUpdateLastActiveTime(pSocket);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
RdrSocketShutdown(
    VOID
    )
{
    /* @todo: assert empty */
    assert(gpSocketHashByName);
    /* @todo: assert empty */
    assert(gpSocketHashByAddress);

    SMBHashSafeFree(&gpSocketHashByName);

    SMBHashSafeFree(&gpSocketHashByAddress);

    return 0;
}
