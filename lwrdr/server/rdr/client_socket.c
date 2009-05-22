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

#include "includes.h"

static
DWORD
_FindOrCreateSocket(
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

    dwError = _FindOrCreateSocket(pszHostname, &pSocket);
    BAIL_ON_SMB_ERROR(dwError);

    *ppSocket = pSocket;

cleanup:

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

static
DWORD
_FindOrCreateSocket(
    uchar8_t        *pszHostname,
    PSMB_SOCKET*    ppSocket
    )
{
    DWORD dwError = 0;
    struct addrinfo *addresses = NULL;
    PSMB_SOCKET pSocket = NULL;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bInSocketLock = FALSE;

    SMB_LOCK_MUTEX(bInLock, &gSocketLock);

    dwError = SMBHashGetValue(
        gpSocketHashByName,
        pszHostname,
        (PVOID *) &pSocket);

    if (!dwError)
    {
        pSocket->refCount++;

        SMB_UNLOCK_MUTEX(bInLock, &gSocketLock);
        SMB_LOCK_MUTEX(bInSocketLock, &pSocket->mutex);

        while (pSocket->state != SMB_RESOURCE_STATE_VALID)
        {
            if (pSocket->state == SMB_RESOURCE_STATE_INVALID)
            {
                dwError = pSocket->error.smb;
                BAIL_ON_SMB_ERROR(dwError);
            }

            pthread_cond_wait(&pSocket->event, &pSocket->mutex);
        }

        SMB_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);
    }
    else
    {
        struct addrinfo hints;

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = PF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        dwError = getaddrinfo((char *) pszHostname, "445", &hints, &addresses);
        BAIL_ON_SMB_ERROR(dwError);

        dwError = SMBSocketCreate(
            addresses,
            pszHostname,
            &pSocket,
            gSignMessagesIfSupported);
        BAIL_ON_SMB_ERROR(dwError);

        dwError = SMBHashSetValue(
            gpSocketHashByName,
            pSocket->pszHostname,
            pSocket);
        BAIL_ON_SMB_ERROR(dwError);

        pSocket->reverseRef = TRUE;

        SMB_UNLOCK_MUTEX(bInLock, &gSocketLock);

        dwError = SMBSocketConnect(pSocket, addresses);
        BAIL_ON_SMB_ERROR(dwError);

        dwError = Negotiate(pSocket);
        BAIL_ON_SMB_ERROR(dwError);

        /* Awake any waiting threads */
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

    SMB_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);
    SMB_UNLOCK_MUTEX(bInLock, &gSocketLock);

    if (pSocket)
    {
        SMBSocketRelease(pSocket);
    }

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

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    dwError = SMBHashSetValue(
                    pSocket->pSessionHashByPrincipal,
                    pSession->pszPrincipal,
                    pSession);
    BAIL_ON_SMB_ERROR(dwError);

    pSession->reverseRef = TRUE;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

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

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    dwError = SMBHashRemoveKey(
                    pSocket->pSessionHashByPrincipal,
                    pSession->pszPrincipal);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

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

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    dwError = SMBHashSetValue(
                    pSocket->pSessionHashByUID,
                    &pSession->uid,
                    pSession);
    BAIL_ON_SMB_ERROR(dwError);

    pSession->reverseRef = TRUE;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

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

    SMB_LOCK_MUTEX(bInLock, &pSocket->mutex);

    dwError = SMBHashRemoveKey(
                    pSocket->pSessionHashByUID,
                    &pSession->uid);
    BAIL_ON_SMB_ERROR(dwError);

    /* @todo: this need be set only when the hash is empty */
    SMBSocketUpdateLastActiveTime(pSocket);

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return dwError;

error:

    goto cleanup;
}

DWORD
RdrSocketShutdown(
    VOID
    )
{
    assert(gpSocketHashByName);

    SMBHashSafeFree(&gpSocketHashByName);

    return 0;
}
