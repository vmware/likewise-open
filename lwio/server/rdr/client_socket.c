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
NTSTATUS
_FindSocketByName(
    uchar8_t    *pszHostname,
    PSMB_SOCKET* ppSocket
    );

static
NTSTATUS
_AddSocketByName(
    PSMB_SOCKET pSocket,
    uchar8_t   *pszHostname
    );

static
NTSTATUS
_FindOrCreateSocket(
    struct addrinfo *addresses,
    uchar8_t        *pszHostname,
    PSMB_SOCKET*    ppSocket
    );

NTSTATUS
RdrSocketInit(
    VOID
    )
{
    NTSTATUS ntStatus = 0;

    assert(!gRdrRuntime.pSocketHashByName);
    assert(!gRdrRuntime.pSocketHashByAddress);

    /* @todo: support case and normalization insensitive string comparisons */
    /* Once we have libidn we'll also need the ability do Unicode case and
       normalization insensitive comparisons of strings.  Otherwise, if the
       string isn't an exact match a redundant DNS lookup would occur. Any
       existing connections to the resolved IP will be caught by the IPv4
       hash, but in a multiple A record DNS setup one could end up with
       multiple connections to a multihomed DCs or set of load-balanced DCs.
       The load balancing case should really be handled by SRV records
       parsed by the client.*/
    ntStatus = SMBHashCreate(
                    19,
                    SMBHashCaselessStringCompare,
                    SMBHashCaselessString,
                    NULL,
                    &gRdrRuntime.pSocketHashByName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashCreate(
                    19,
                    &SMBSrvClientHashSockaddrCompare,
                    &SMBSrvClientHashAddrinfo,
                    NULL,
                    &gRdrRuntime.pSocketHashByAddress);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

/* @todo: add support for libidn for internationalized UNC hostnames */
/* @todo: handle IPv6 addresses */
/* The socket is returned with a reference */
NTSTATUS
SMBSrvClientSocketCreate(
    uchar8_t    *pszHostname,
    PSMB_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SOCKET pSocket = NULL;
    struct addrinfo *addresses = NULL;

    /* Check for the name in the name hash; if it's not there, resolve it */
    ntStatus = _FindSocketByName(pszHostname, &pSocket);
    if (ntStatus == ENOENT)
    {
        ntStatus = 0;
    }
    BAIL_ON_NT_STATUS(ntStatus);

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
            ntStatus = s;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        /* Check for the resolved IPs in the IP hash; if none are found,
           connect */
        ntStatus = _FindOrCreateSocket(addresses, pszHostname, &pSocket);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Whether we found an existing entry or not, add to the name hash */
        ntStatus = _AddSocketByName(pSocket, pszHostname);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = Negotiate(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Set state and awake any waiting threads */
        SMBSocketSetState(pSocket, SMB_RESOURCE_STATE_VALID);
    }

    *ppSocket = pSocket;

cleanup:

    if (addresses)
    {
        freeaddrinfo(addresses);
    }

    return ntStatus;

error:

    *ppSocket = NULL;

    goto cleanup;
}


/* Must be called with the session mutex held */
NTSTATUS
SMBSrvClientSocketIsStale_inlock(
    PSMB_SOCKET pSocket,
    PBOOLEAN    pbIsStale
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bIsStale = FALSE;
    SMB_HASH_ITERATOR iterator;

    if (pSocket->refCount > 2)
    {
        goto done;
    }

    ntStatus = SMBHashGetIterator(
                    pSocket->pSessionHashByPrincipal,
                    &iterator);
    BAIL_ON_NT_STATUS(ntStatus);

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

    return ntStatus;
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
NTSTATUS
_FindSocketByName(
    uchar8_t    *pszHostname,
    PSMB_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SOCKET pSocket = NULL;
    BOOLEAN bInLock = FALSE;

    /* This path exists only to avoid DNS lookups in the fast path.  If the
       socket is in any state but connected, we return NULL to force the
       lookup.  Site affinity is handled by the client. */

    /* @todo: add expiration to the name hash */
    SMB_LOCK_RWMUTEX_SHARED(bInLock, &gRdrRuntime.socketHashLock);

    ntStatus = SMBHashGetValue(
                    gRdrRuntime.pSocketHashByName,
                    pszHostname,
                    (PVOID *) &pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBSocketAddReference(pSocket);

    *ppSocket = pSocket;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &gRdrRuntime.socketHashLock);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
_AddSocketByName(
    PSMB_SOCKET pSocket,
    uchar8_t   *pszHostname
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gRdrRuntime.socketHashLock);

    /* We replace any existing entry on the grounds that new socket has been
       created from fresher DNS information */
    ntStatus = SMBHashSetValue(
                    gRdrRuntime.pSocketHashByName,
                    pSocket->pszHostname,
                    pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &gRdrRuntime.socketHashLock);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
_FindOrCreateSocket(
    struct addrinfo *addresses,
    uchar8_t        *pszHostname,
    PSMB_SOCKET*    ppSocket
    )
{
    NTSTATUS ntStatus = 0;

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
    BOOLEAN bFirstPass = TRUE;
    BOOLEAN bFoundValid = FALSE;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bSocketInLock = FALSE;

restart:

    if (bFirstPass)
    {
        SMB_LOCK_RWMUTEX_SHARED(bInLock, &gRdrRuntime.socketHashLock);
    }
    else
    {
        SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gRdrRuntime.socketHashLock);
    }

    for (ai = addresses; ai != NULL && !bFoundValid; ai = ai->ai_next)
    {
        ntStatus = SMBHashGetValue(
                        gRdrRuntime.pSocketHashByAddress,
                        (PCVOID *) ai,
                        (PVOID *) &pSocket);
        if (!ntStatus)
        {
            SMB_LOCK_MUTEX(bSocketInLock, &pSocket->mutex);

            switch (pSocket->state)
            {
                case SMB_RESOURCE_STATE_VALID:

                    pSocket->refCount++;
                    bFoundValid = TRUE;
                    goto found;

                    break;

                case SMB_RESOURCE_STATE_INITIALIZING:

                    if (bFirstPass)
                        break;

                    pSocket->refCount++;

                    SMB_UNLOCK_RWMUTEX(bInLock, &gRdrRuntime.socketHashLock);

                    pthread_cond_wait(
                            &pSocket->event,
                            &pSocket->mutex);

                    pSocket->refCount--; /* @todo: free on zero */

                    SMB_UNLOCK_MUTEX(bSocketInLock, &pSocket->mutex);

                    bFirstPass = TRUE;

                    goto restart;

                case SMB_RESOURCE_STATE_INVALID:

                    /* No need to check error codes; only permanent errors
                       (negative cache entries) will remain in the hash. */
                    SMB_UNLOCK_MUTEX(bSocketInLock, &pSocket->mutex);

                    continue;

                default:

                    /* Invalid state */
                    assert(FALSE);
            }

            SMB_UNLOCK_MUTEX(bSocketInLock, &pSocket->mutex);

        }
        else if (ntStatus != ENOENT)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }
        ntStatus = 0;

        /* Not found; create in second pass */
        if (!bFirstPass)
        {
            ntStatus = SMBSocketCreate(ai, pszHostname, &pSocket);
            BAIL_ON_NT_STATUS(ntStatus);

            /* add to hash */
            ntStatus = SMBHashSetValue(
                            gRdrRuntime.pSocketHashByAddress,
                            &pSocket->address,
                            pSocket);
            BAIL_ON_NT_STATUS(ntStatus);

            /* Don't hold locks across a network operation */
            SMB_UNLOCK_RWMUTEX(bInLock, &gRdrRuntime.socketHashLock);

            /* Attempt to connect */
            /* @todo: most likely a socket error is a resource issue;
               therefore, the address in question should not be added to
               the negative connection cache */
            /* Retry any transient errors? */
            ntStatus = SMBSocketConnect(pSocket, ai);
            if (ntStatus == ETIMEDOUT)
            {
                /* On timeout, rescan to find new sockets.  The error state
                   set by the failed connect ensures forward progress. */
                /* @todo: wake waiters */
                bFirstPass = TRUE;
                goto restart;
            }
            else if (ntStatus)
            {
                continue;
            }

            bFoundValid = TRUE;
            goto found;
        }
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &gRdrRuntime.socketHashLock);

    if (!bFoundValid && bFirstPass)
    {
        bFirstPass = FALSE;
        goto restart;
    }

found:

    *ppSocket = (bFoundValid ? pSocket : NULL);
    ntStatus = (bFoundValid ? 0 : ENOENT);

cleanup:

    return ntStatus;

error:

    *ppSocket = NULL;

    goto cleanup;
}

NTSTATUS
SMBSrvClientSocketAddSessionByPrincipal(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSocket->hashLock);

    /* @todo: check for race */
    ntStatus = SMBHashSetValue(
                    pSocket->pSessionHashByPrincipal,
                    pSession->pszPrincipal,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSocketRemoveSessionByPrincipal(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSocket->hashLock);

    ntStatus = SMBHashRemoveKey(
                    pSocket->pSessionHashByPrincipal,
                    pSession->pszPrincipal);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSocketAddSessionByUID(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSocket->hashLock);

    /* No need to check for a race here; the principal hash is always checked
       first */
    ntStatus = SMBHashSetValue(
                    pSocket->pSessionHashByUID,
                    &pSession->uid,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSrvClientSocketRemoveSessionByUID(
    PSMB_SOCKET  pSocket,
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSocket->hashLock);

    ntStatus = SMBHashRemoveKey(
                    pSocket->pSessionHashByUID,
                    &pSession->uid);
    BAIL_ON_NT_STATUS(ntStatus);

    /* @todo: this need be set only when the hash is empty */
    SMBSocketUpdateLastActiveTime(pSocket);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSocket->hashLock);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
RdrSocketShutdown(
    VOID
    )
{
    /* @todo: assert empty */
    assert(gRdrRuntime.pSocketHashByName);
    /* @todo: assert empty */
    assert(gRdrRuntime.pSocketHashByAddress);

    SMBHashSafeFree(&gRdrRuntime.pSocketHashByName);

    SMBHashSafeFree(&gRdrRuntime.pSocketHashByAddress);

    return 0;
}
