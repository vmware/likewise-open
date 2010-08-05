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
 *        srvconnection.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Connection Object
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
inline
BOOLEAN
SrvConnectionIsInvalid_inlock(
    PLWIO_SRV_CONNECTION pConnection
    );

static
NTSTATUS
SrvConnectionCountSessions(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvConnection2CountSessions(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvConnectionAddSession_inlock(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION pSession
    );

static
NTSTATUS
SrvConnection2AddSession_inlock(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION_2 pSession
    );

static
NTSTATUS
SrvConnection2AcquireAsyncId_inlock(
   PLWIO_SRV_CONNECTION pConnection,
   PULONG64             pullAsyncId
   );

static
NTSTATUS
SrvConnectionSelectCandidateSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvConnection2SelectCandidateSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvConnectionDeleteSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvConnection2DeleteSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
VOID
SrvConnectionFree(
    PLWIO_SRV_CONNECTION pConnection
    );

// Rules:
//
// Only one reader thread can read from this socket
// Multiple writers can write to the socket, in a synchronized manner per message
// Both readers and writers can set the connection state to be invalid
// The file descriptor is set to -1 only by the reader thread (not the writers)
// The file descriptor is closed only when the connection object is freed.

static
NTSTATUS
SrvConnectionAcquireSessionId_inlock(
   PLWIO_SRV_CONNECTION pConnection,
   PUSHORT             pUid
   );

static
VOID
SrvConnectionFreeContentsClientProperties(
    PSRV_CLIENT_PROPERTIES pProperties
    );

static
int
SrvConnectionSessionCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvConnectionSessionRelease(
    PVOID pSession
    );

static
NTSTATUS
SrvConnection2AcquireSessionId_inlock(
   PLWIO_SRV_CONNECTION pConnection,
   PULONG64             pUid
   );

static
NTSTATUS
SrvConnectionCreateSessionCollection(
   PLWRTL_RB_TREE* ppSessionCollection
   );

static
NTSTATUS
SrvConnection2CreateSessionCollection(
   PLWRTL_RB_TREE* ppSessionCollection
   );

static
int
SrvConnection2SessionCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvConnection2SessionRelease(
    PVOID pSession
    );

static
int
SrvConnection2AsyncStateCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvConnection2AsyncStateRelease(
    PVOID pAsyncState
    );

static
VOID
SrvConnectionRundown(
    PLWIO_SRV_CONNECTION pConnection
    );

static
BOOLEAN
SrvConnectionGatherRundownSessionListCallback(
    PLWIO_SRV_SESSION pSession,
    PVOID pContext
    );

static
BOOLEAN
SrvConnection2GatherRundownSessionListCallback(
    PLWIO_SRV_SESSION_2 pSession,
    PVOID pContext
    );


static
VOID
SrvConnectionRundownSessionList(
    PLWIO_SRV_SESSION pRundownList
    );

static
VOID
SrvConnection2RundownSessionList(
    PLWIO_SRV_SESSION_2 pRundownList
    );

static
BOOLEAN
SrvConnectionRundownAsyncStateCallback(
    PLWIO_ASYNC_STATE pAsyncState,
    PVOID pContext
    );

NTSTATUS
SrvConnectionCreate(
    const struct sockaddr*          pClientAddress,
    SOCKLEN_T                       clientAddrLen,
    const struct sockaddr*          pServerAddress,
    SOCKLEN_T                       serverAddrLen,
    PLWIO_SRV_SOCKET                pSocket,
    HANDLE                          hPacketAllocator,
    HANDLE                          hGssContext,
    PLWIO_SRV_SHARE_ENTRY_LIST      pShareList,
    PSRV_PROPERTIES                 pServerProperties,
    PSRV_HOST_INFO                  pHostinfo,
    PSRV_CONNECTION_SOCKET_DISPATCH pSocketDispatch,
    PLWIO_SRV_CONNECTION*           ppConnection
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION pConnection = NULL;

    if (!pClientAddress)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(sizeof(*pConnection), (PVOID*)&pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->refCount = 1;

    pthread_rwlock_init(&pConnection->mutex, NULL);
    pConnection->pMutex = &pConnection->mutex;

    pthread_mutex_init(&pConnection->mutexSessionSetup, NULL);
    pConnection->pMutexSessionSetup = &pConnection->mutexSessionSetup;

    ntStatus = SrvCreditorCreate(&pConnection->pCreditor);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnectionCreateSessionCollection(
                    &pConnection->pSessionCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeCreate(
                    &SrvConnection2AsyncStateCompare,
                    NULL,
                    &SrvConnection2AsyncStateRelease,
                    &pConnection->pAsyncStateCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAcquireHostInfo(
                    pHostinfo,
                    &pConnection->pHostinfo);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGssAcquireContext(
                    pConnection->pHostinfo,
                    hGssContext,
                    &pConnection->hGssContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection->clientAddress = *pClientAddress;
    pConnection->clientAddrLen = clientAddrLen;
    pConnection->serverAddress = *pServerAddress;
    pConnection->serverAddrLen = serverAddrLen;

    pConnection->ulSequence = 0;
    pConnection->hPacketAllocator = hPacketAllocator;
    pConnection->pShareList = pShareList;
    pConnection->state = LWIO_SRV_CONN_STATE_INITIAL;
    pConnection->pSocket = pSocket;
    pConnection->pSocketDispatch = pSocketDispatch;

    memcpy(&pConnection->serverProperties, pServerProperties, sizeof(*pServerProperties));
    uuid_copy(pConnection->serverProperties.GUID, pServerProperties->GUID);

    pConnection->resource.resourceType = SRV_RESOURCE_TYPE_CONNECTION;

    SRV_ELEMENTS_INCREMENT_CONNECTIONS;

    *ppConnection = pConnection;

cleanup:

    return ntStatus;

error:

    *ppConnection = NULL;

    if (pConnection)
    {
        SrvConnectionRelease(pConnection);
    }

    goto cleanup;
}

SMB_PROTOCOL_VERSION
SrvConnectionGetProtocolVersion(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    BOOLEAN bInLock = FALSE;
    SMB_PROTOCOL_VERSION protocolVersion = SMB_PROTOCOL_VERSION_UNKNOWN;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    protocolVersion = pConnection->protocolVer;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return protocolVersion;
}

SMB_PROTOCOL_VERSION
SrvConnectionGetProtocolVersion_inlock(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    return pConnection->protocolVer;
}

NTSTATUS
SrvConnectionSetProtocolVersion(
    PLWIO_SRV_CONNECTION pConnection,
    SMB_PROTOCOL_VERSION protoVer
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = SrvConnectionSetProtocolVersion_inlock(pConnection, protoVer);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;
}

NTSTATUS
SrvConnectionSetProtocolVersion_inlock(
    PLWIO_SRV_CONNECTION pConnection,
    SMB_PROTOCOL_VERSION protoVer
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWRTL_RB_TREE pNewSessionCollection = NULL;

    if (protoVer != pConnection->protocolVer)
    {
        if (SrvConnectionIsInvalid_inlock(pConnection))
        {
            ntStatus = STATUS_CONNECTION_DISCONNECTED;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (pConnection->ullSessionCount)
        {
            LWIO_LOG_ERROR("Attempting to change protocol version after establishing a session");
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pConnection->protocolVer = protoVer;

        // Replace existing session collection with new one of appropriate
        // type.  This is needed because the collection needs compare/free
        // functions that match the session type.

        switch (protoVer)
        {
            case SMB_PROTOCOL_VERSION_1:

                LWIO_ASSERT(!pConnection->ulSequence);
                LWIO_ASSERT(!pConnection->usNextAvailableUid);

                ntStatus = SrvConnectionCreateSessionCollection(
                                &pNewSessionCollection);
                BAIL_ON_NT_STATUS(ntStatus);

                break;

            case SMB_PROTOCOL_VERSION_2:

                LWIO_ASSERT(!pConnection->ullNextAvailableUid);

                ntStatus = SrvConnection2CreateSessionCollection(
                                &pNewSessionCollection);
                BAIL_ON_NT_STATUS(ntStatus);

                break;

            default:

                ntStatus = STATUS_INVALID_PARAMETER_2;
                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

        LwRtlRBTreeFree(pConnection->pSessionCollection);
        pConnection->pSessionCollection = pNewSessionCollection;
        pNewSessionCollection = NULL;
    }

cleanup:

    if (pNewSessionCollection)
    {
        LwRtlRBTreeFree(pNewSessionCollection);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
inline
BOOLEAN
SrvConnectionIsInvalid_inlock(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    return pConnection->state == LWIO_SRV_CONN_STATE_INVALID;
}

BOOLEAN
SrvConnectionIsInvalid(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    BOOLEAN bInvalid = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    bInvalid = SrvConnectionIsInvalid_inlock(pConnection);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return bInvalid;
}

VOID
SrvConnectionSetInvalid(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bDisconnect = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    if (pConnection->state != LWIO_SRV_CONN_STATE_INVALID)
    {
        bDisconnect = TRUE;
        pConnection->state = LWIO_SRV_CONN_STATE_INVALID;
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    if (bDisconnect)
    {
        SrvConnectionRundown(pConnection);

        // Call disconnect w/o lock held.
        if (pConnection->pSocket)
        {
            pConnection->pSocketDispatch->pfnDisconnect(pConnection->pSocket);
        }
    }
}

LWIO_SRV_CONN_STATE
SrvConnectionGetState(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    LWIO_SRV_CONN_STATE connState = LWIO_SRV_CONN_STATE_INITIAL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    connState = pConnection->state;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return connState;
}

VOID
SrvConnectionSetState(
    PLWIO_SRV_CONNECTION pConnection,
    LWIO_SRV_CONN_STATE  connState
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    pConnection->state = connState;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);
}

BOOLEAN
SrvConnectionIsSigningActive(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    bResult = SrvConnectionIsSigningActive_inlock(pConnection);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return bResult;
}

BOOLEAN
SrvConnectionIsSigningActive_inlock(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    return (pConnection->serverProperties.bRequireSecuritySignatures &&
            pConnection->pSessionKey);
}

NTSTATUS
SrvConnectionFindSession_inlock(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT uid,
    PLWIO_SRV_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;

    pSession = pConnection->lruSession[ uid % SRV_LRU_CAPACITY ];
    if (!pSession || (pSession->uid != uid))
    {
        ntStatus = LwRtlRBTreeFind(
                        pConnection->pSessionCollection,
                        &uid,
                        (PVOID*)&pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        pConnection->lruSession[ uid % SRV_LRU_CAPACITY ] = pSession;
    }

    InterlockedIncrement(&pSession->refcount);

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_INVALID_HANDLE;
    }

    *ppSession = NULL;

    goto cleanup;
}

NTSTATUS
SrvConnectionFindSession(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT uid,
    PLWIO_SRV_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    ntStatus = SrvConnectionFindSession_inlock(pConnection, uid, &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppSession = NULL;

    goto cleanup;
}

NTSTATUS
SrvConnection2FindSession_inlock(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullUid,
    PLWIO_SRV_SESSION_2* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION_2 pSession = NULL;

    pSession = pConnection->lruSession2[ ullUid % SRV_LRU_CAPACITY ];
    if (!pSession || (pSession->ullUid != ullUid))
    {
        ntStatus = LwRtlRBTreeFind(
                        pConnection->pSessionCollection,
                        &ullUid,
                        (PVOID*)&pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        pConnection->lruSession2[ ullUid % SRV_LRU_CAPACITY ] = pSession;
    }

    InterlockedIncrement(&pSession->refcount);

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_USER_SESSION_DELETED;
    }

    *ppSession = NULL;

    goto cleanup;
}

NTSTATUS
SrvConnection2FindSession(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullUid,
    PLWIO_SRV_SESSION_2* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION_2 pSession = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    ntStatus = SrvConnection2FindSession_inlock(pConnection, ullUid, &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppSession = NULL;

    goto cleanup;
}

NTSTATUS
SrvConnectionGetSessionCount(
    PLWIO_SRV_CONNECTION pConnection,
    PWSTR                pwszUsername,
    PULONG64             pullSessionCount
    )
{
    NTSTATUS ntStatus        = STATUS_SUCCESS;
    BOOLEAN  bInLock         = FALSE;
    SRV_SESSION_ENUM_QUERY sessionEnumQuery =
    {
          .pwszUsername    = pwszUsername,
          .ullSessionCount = 0
    };

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    if (!pwszUsername)
    {
        sessionEnumQuery.ullSessionCount = pConnection->ullSessionCount;
    }
    else
    {
        ntStatus = LwRtlRBTreeTraverse(
                        pConnection->pSessionCollection,
                        LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                        &SrvConnectionCountSessions,
                        &sessionEnumQuery);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pullSessionCount = sessionEnumQuery.ullSessionCount;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *pullSessionCount = 0;

    goto cleanup;
}

static
NTSTATUS
SrvConnectionCountSessions(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_SRV_SESSION       pSession          = (PLWIO_SRV_SESSION)pData;
    PSRV_SESSION_ENUM_QUERY pSessionEnumQuery =
                                (PSRV_SESSION_ENUM_QUERY)pUserData;
    BOOLEAN                 bIsMatch          = FALSE;

    bIsMatch = SrvSessionIsMatchPrincipal(
                    pSession,
                    pSessionEnumQuery->pwszUsername);
    if (bIsMatch)
    {
        pSessionEnumQuery->ullSessionCount++;
    }

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS
SrvConnection2GetSessionCount(
    PLWIO_SRV_CONNECTION pConnection,
    PWSTR                pwszUsername,
    PULONG64             pullSessionCount
    )
{
    NTSTATUS ntStatus        = STATUS_SUCCESS;
    BOOLEAN  bInLock         = FALSE;
    SRV_SESSION_ENUM_QUERY sessionEnumQuery =
    {
          .pwszUsername    = pwszUsername,
          .ullSessionCount = 0
    };

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    if (!pwszUsername)
    {
        sessionEnumQuery.ullSessionCount = pConnection->ullSessionCount;
    }
    else
    {
        ntStatus = LwRtlRBTreeTraverse(
                        pConnection->pSessionCollection,
                        LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                        &SrvConnection2CountSessions,
                        &sessionEnumQuery);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pullSessionCount = sessionEnumQuery.ullSessionCount;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *pullSessionCount = 0;

    goto cleanup;
}

static
NTSTATUS
SrvConnection2CountSessions(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_SRV_SESSION_2     pSession          = (PLWIO_SRV_SESSION_2)pData;
    PSRV_SESSION_ENUM_QUERY pSessionEnumQuery =
                                (PSRV_SESSION_ENUM_QUERY)pUserData;
    BOOLEAN                 bIsMatch          = FALSE;

    bIsMatch = SrvSession2IsMatchPrincipal(
                    pSession,
                    pSessionEnumQuery->pwszUsername);
    if (bIsMatch)
    {
        pSessionEnumQuery->ullSessionCount++;
    }

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

NTSTATUS
SrvConnectionRemoveSession(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_SESSION pCachedSession = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    if (SrvSessionIsInParent(pSession))
    {
        pCachedSession = pConnection->lruSession[ pSession->uid % SRV_LRU_CAPACITY ];
        if (pCachedSession && (pCachedSession->uid == pSession->uid))
        {
            pConnection->lruSession[ pSession->uid % SRV_LRU_CAPACITY ] = NULL;
        }

        // removal automatically releases reference
        ntStatus = LwRtlRBTreeRemove(
                        pConnection->pSessionCollection,
                        &pSession->uid);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvSessionClearInParent(pSession);

        pConnection->ullSessionCount--;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    LWIO_LOG_ERROR("Cannot fail!, status = 0x%08X (%s)",
                   ntStatus, LwNtStatusToName(ntStatus));
    LWIO_ASSERT_MSG(FALSE, "Cannot fail");

    goto cleanup;
}

NTSTATUS
SrvConnection2RemoveSession(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION_2 pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_SESSION_2 pCachedSession = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    if (SrvSession2IsInParent(pSession))
    {
        pCachedSession = pConnection->lruSession2[ pSession->ullUid % SRV_LRU_CAPACITY ];
        if (pCachedSession && (pCachedSession->ullUid == pSession->ullUid))
        {
            pConnection->lruSession2[ pSession->ullUid % SRV_LRU_CAPACITY ] = NULL;
        }

        // removal automatically releases reference
        ntStatus = LwRtlRBTreeRemove(
                        pConnection->pSessionCollection,
                        &pSession->ullUid);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvSession2ClearInParent(pSession);

        pConnection->ullSessionCount--;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    LWIO_LOG_ERROR("Cannot fail!, status = 0x%08X (%s)",
                   ntStatus, LwNtStatusToName(ntStatus));
    LWIO_ASSERT_MSG(FALSE, "Cannot fail");

    goto cleanup;
}

NTSTATUS
SrvConnectionCreateSession(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;
    USHORT  uid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    if (SrvConnectionIsInvalid_inlock(pConnection))
    {
        ntStatus = STATUS_CONNECTION_DISCONNECTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvConnectionAcquireSessionId_inlock(
                    pConnection,
                    &uid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionCreate(
                    pConnection,
                    uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnectionAddSession_inlock(pConnection, pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    *ppSession = NULL;

    if (pSession)
    {
        SrvSessionRundown(pSession);
        SrvSessionRelease(pSession);
    }

    goto cleanup;
}

NTSTATUS
SrvConnection2CreateSession(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION_2* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION_2 pSession = NULL;
    BOOLEAN bInLock = FALSE;
    ULONG64 ullUid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    if (SrvConnectionIsInvalid_inlock(pConnection))
    {
        ntStatus = STATUS_CONNECTION_DISCONNECTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvConnection2AcquireSessionId_inlock(
                    pConnection,
                    &ullUid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2Create(
                    pConnection,
                    ullUid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnection2AddSession_inlock(pConnection, pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    *ppSession = NULL;

    if (pSession)
    {
        SrvSession2Rundown(pSession);
        SrvSession2Release(pSession);
    }

    goto cleanup;
}

static
NTSTATUS
SrvConnectionAddSession_inlock(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = LwRtlRBTreeAdd(
                    pConnection->pSessionCollection,
                    &pSession->uid,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    // Reference from parent
    SrvSessionAcquire(pSession);
    SrvSessionSetInParent(pSession);

    pConnection->lruSession[pSession->uid % SRV_LRU_CAPACITY] = pSession;
    pConnection->ullSessionCount++;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvConnection2AddSession_inlock(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION_2 pSession
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = LwRtlRBTreeAdd(
                    pConnection->pSessionCollection,
                    &pSession->ullUid,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvSession2Acquire(pSession);
    SrvSession2SetInParent(pSession);

    pConnection->lruSession2[pSession->ullUid % SRV_LRU_CAPACITY] = pSession;
    pConnection->ullSessionCount++;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}


NTSTATUS
SrvConnection2CreateAsyncState(
    PLWIO_SRV_CONNECTION            pConnection,
    USHORT                          usCommand,
    PFN_LWIO_SRV_CANCEL_ASYNC_STATE pfnCancelAsyncState,
    PFN_LWIO_SRV_FREE_ASYNC_STATE   pfnFreeAsyncState,
    PLWIO_ASYNC_STATE*              ppAsyncState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PLWIO_ASYNC_STATE pAsyncState = NULL;
    ULONG64           ullAsyncId  = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    if (SrvConnectionIsInvalid_inlock(pConnection))
    {
        ntStatus = STATUS_CONNECTION_DISCONNECTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvConnection2AcquireAsyncId_inlock(
                    pConnection,
                    &ullAsyncId);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAsyncStateCreate(
                    ullAsyncId,
                    usCommand,
                    NULL,
                    pfnCancelAsyncState,
                    pfnFreeAsyncState,
                    &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pConnection->pAsyncStateCollection,
                    &pAsyncState->ullAsyncId,
                    pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppAsyncState = SrvAsyncStateAcquire(pAsyncState);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppAsyncState = NULL;

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvConnection2AcquireAsyncId_inlock(
   PLWIO_SRV_CONNECTION pConnection,
   PULONG64             pullAsyncId
   )
{
    NTSTATUS ntStatus = 0;
    ULONG64  ullCandidateAsyncId = pConnection->ullNextAvailableAsyncId;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_ASYNC_STATE pAsyncState = NULL;

        /* 0 is never a valid async id */

        if ((ullCandidateAsyncId == 0) || (ullCandidateAsyncId == UINT64_MAX))
        {
            ullCandidateAsyncId = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pConnection->pAsyncStateCollection,
                        &ullCandidateAsyncId,
                        (PVOID*)&pAsyncState);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            ullCandidateAsyncId++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((ullCandidateAsyncId != pConnection->ullNextAvailableAsyncId) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_CONTEXT_IDS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pullAsyncId = ullCandidateAsyncId;

    /* Increment by 1 by make sure to deal with wrap around */

    ullCandidateAsyncId++;
    pConnection->ullNextAvailableAsyncId = ullCandidateAsyncId ? ullCandidateAsyncId : 1;

cleanup:

    return ntStatus;

error:

    *pullAsyncId = 0;

    goto cleanup;
}

NTSTATUS
SrvConnection2FindAsyncState(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullAsyncId,
    PLWIO_ASYNC_STATE*   ppAsyncState
    )
{
    NTSTATUS          ntStatus = STATUS_SUCCESS;
    PLWIO_ASYNC_STATE pAsyncState = NULL;
    BOOLEAN           bInLock     = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pConnection->pAsyncStateCollection,
                    &ullAsyncId,
                    (PVOID*)&pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppAsyncState = SrvAsyncStateAcquire(pAsyncState);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *ppAsyncState = NULL;

    goto cleanup;
}

VOID
SrvConnection2RemoveAsyncState(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullAsyncId
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pConnection->pAsyncStateCollection,
                    &ullAsyncId);
    if ((ntStatus != STATUS_SUCCESS) && (ntStatus != STATUS_NOT_FOUND))
    {
        LWIO_LOG_ERROR("Unexpected error removing async state, "
                       "status = 0x%08X (%s)",
                       ntStatus, LwNtStatusToName(ntStatus));
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);
}

NTSTATUS
SrvConnectionGetNamedPipeClientAddress(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PVOID pAddr = NULL;
    ULONG ulAddrLength = 0;

    ntStatus = pConnection->pSocketDispatch->pfnGetAddressBytes(
                    pConnection->pSocket,
                    &pAddr,
                    &ulAddrLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoRtlEcpListInsert(pEcpList,
                                  IO_ECP_TYPE_PEER_ADDRESS,
                                  pAddr,
                                  ulAddrLength,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvConnectionGetNamedPipeSessionKey(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pSessionKey = pConnection->pSessionKey;
    ULONG ulSessionKeyLength = pConnection->ulSessionKeyLength;

    if (pSessionKey != NULL)
    {
        ntStatus = IoRtlEcpListInsert(pEcpList,
                                      IO_ECP_TYPE_SESSION_KEY,
                                      pSessionKey,
                                      ulSessionKeyLength,
                                      NULL);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvConnectionDeleteSessions(
    PLWIO_SRV_CONNECTION pConnection,
    PWSTR                pwszUsername
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_SESSION_DEL_QUERY sessionDelQuery =
    {
            .pSessionCollection = NULL,
            .pwszUsername       = pwszUsername
    };
    BOOLEAN  bInLock  = FALSE;

    switch (pConnection->protocolVer)
    {
        case SMB_PROTOCOL_VERSION_1:

            ntStatus = SrvConnectionCreateSessionCollection(
                            &sessionDelQuery.pSessionCollection);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

            ntStatus = LwRtlRBTreeTraverse(
                            pConnection->pSessionCollection,
                            LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                            &SrvConnectionSelectCandidateSession,
                            &sessionDelQuery);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

            ntStatus = LwRtlRBTreeTraverse(
                           sessionDelQuery.pSessionCollection,
                           LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                           &SrvConnectionDeleteSession,
                           pConnection);

            break;

        case SMB_PROTOCOL_VERSION_2:

            ntStatus = SrvConnection2CreateSessionCollection(
                            &sessionDelQuery.pSessionCollection);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

            ntStatus = LwRtlRBTreeTraverse(
                            pConnection->pSessionCollection,
                            LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                            &SrvConnection2SelectCandidateSession,
                            &sessionDelQuery);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

            ntStatus = LwRtlRBTreeTraverse(
                           sessionDelQuery.pSessionCollection,
                           LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                           &SrvConnection2DeleteSession,
                           pConnection);

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    if (sessionDelQuery.pSessionCollection)
    {
        LwRtlRBTreeFree(sessionDelQuery.pSessionCollection);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvConnectionSelectCandidateSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS               ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION      pSession = (PLWIO_SRV_SESSION)pData;
    BOOLEAN                bIsMatch = FALSE;
    PSRV_SESSION_DEL_QUERY pSessionDelQuery = (PSRV_SESSION_DEL_QUERY)pUserData;

    bIsMatch = SrvSessionIsMatchPrincipal(
                    pSession,
                    pSessionDelQuery->pwszUsername);
    if (bIsMatch)
    {
        ntStatus = LwRtlRBTreeAdd(
                        pSessionDelQuery->pSessionCollection,
                        &pSession->uid,
                        pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvSessionAcquire(pSession);
    }

    *pbContinue = TRUE;

cleanup:

    return ntStatus;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvConnection2SelectCandidateSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION_2 pSession = (PLWIO_SRV_SESSION_2)pData;
    PSRV_SESSION_DEL_QUERY pSessionDelQuery = (PSRV_SESSION_DEL_QUERY)pUserData;
    BOOLEAN bIsMatch = FALSE;

    bIsMatch = SrvSession2IsMatchPrincipal(
                    pSession,
                    pSessionDelQuery->pwszUsername);
    if (bIsMatch)
    {
        ntStatus = LwRtlRBTreeAdd(
                        pSessionDelQuery->pSessionCollection,
                        &pSession->ullUid,
                        pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvSession2Acquire(pSession);
    }

    *pbContinue = TRUE;

cleanup:

    return ntStatus;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvConnectionDeleteSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_SRV_SESSION pSession  = (PLWIO_SRV_SESSION)pData;

    SrvSessionRundown(pSession);

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

static
NTSTATUS
SrvConnection2DeleteSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    PLWIO_SRV_SESSION_2 pSession  = (PLWIO_SRV_SESSION_2)pData;

    SrvSession2Rundown(pSession);

    *pbContinue = TRUE;

    return STATUS_SUCCESS;
}

PLWIO_SRV_CONNECTION
SrvConnectionAcquire(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    InterlockedIncrement(&pConnection->refCount);

    return pConnection;
}

VOID
SrvConnectionRelease(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    if (InterlockedDecrement(&pConnection->refCount) == 0)
    {
        SRV_ELEMENTS_DECREMENT_CONNECTIONS;

        SrvConnectionFree(pConnection);
    }
}

static
VOID
SrvConnectionFree(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    if (pConnection->readerState.pRequestPacket)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pConnection->readerState.pRequestPacket);
    }

    if (pConnection->pSessionKey)
    {
        SrvFreeMemory(pConnection->pSessionKey);
    }

    if (pConnection->hGssNegotiate)
    {
        SrvGssEndNegotiate(
            pConnection->hGssContext,
            pConnection->hGssNegotiate);
    }

    if (pConnection->hGssContext)
    {
        SrvGssReleaseContext(pConnection->hGssContext);
    }

    if (pConnection->pSocket)
    {
        pConnection->pSocketDispatch->pfnFree(pConnection->pSocket);
    }

    if (pConnection->pCreditor)
    {
        SrvCreditorFree(pConnection->pCreditor);
    }

    if (pConnection->pSessionCollection)
    {
        LwRtlRBTreeFree(pConnection->pSessionCollection);
    }

    if (pConnection->pOEMConnection)
    {
        SrvOEMCloseClientConnection(
                pConnection->pOEMConnection,
                pConnection->ulOEMConnectionLength);
    }

    if (pConnection->pAsyncStateCollection)
    {
        LwRtlRBTreeFree(pConnection->pAsyncStateCollection);
    }

    if (pConnection->pHostinfo)
    {
        SrvReleaseHostInfo(pConnection->pHostinfo);
    }

    if (pConnection->pMutex)
    {
        pthread_rwlock_destroy(&pConnection->mutex);
        pConnection->pMutex = NULL;
    }

    if (pConnection->pMutexSessionSetup)
    {
        pthread_mutex_destroy(&pConnection->mutexSessionSetup);;
        pConnection->pMutexSessionSetup = NULL;
    }

    SrvConnectionFreeContentsClientProperties(&pConnection->clientProperties);

    SrvFreeMemory(pConnection);
}

static
NTSTATUS
SrvConnectionAcquireSessionId_inlock(
   PLWIO_SRV_CONNECTION pConnection,
   PUSHORT             pUid
   )
{
    NTSTATUS ntStatus = 0;
    USHORT   candidateUid = pConnection->usNextAvailableUid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_SRV_SESSION pSession = NULL;

        /* 0 is never a valid session vuid */

        if ((candidateUid == 0) || (candidateUid == UINT16_MAX))
        {
            candidateUid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pConnection->pSessionCollection,
                        &candidateUid,
                        (PVOID*)&pSession);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            candidateUid++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateUid != pConnection->usNextAvailableUid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_SESSIONS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pUid = candidateUid;

    /* Increment by 1 by make sure tyo deal with wraparound */

    candidateUid++;
    pConnection->usNextAvailableUid = candidateUid ? candidateUid : 1;

cleanup:

    return ntStatus;

error:

    *pUid = 0;

    goto cleanup;
}

static
VOID
SrvConnectionFreeContentsClientProperties(
    PSRV_CLIENT_PROPERTIES pProperties
    )
{
    if (pProperties->pwszNativeLanMan)
    {
        SrvFreeMemory(pProperties->pwszNativeLanMan);
    }
    if (pProperties->pwszNativeOS)
    {
        SrvFreeMemory(pProperties->pwszNativeOS);
    }
    if (pProperties->pwszNativeDomain)
    {
        SrvFreeMemory(pProperties->pwszNativeDomain);
    }
}

static
int
SrvConnectionSessionCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PUSHORT pUid1 = (PUSHORT)pKey1;
    PUSHORT pUid2 = (PUSHORT)pKey2;

    assert (pUid1 != NULL);
    assert (pUid2 != NULL);

    if (*pUid1 > *pUid2)
    {
        return 1;
    }
    else if (*pUid1 < *pUid2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
SrvConnectionSessionRelease(
    PVOID pSession
    )
{
    SrvSessionRelease((PLWIO_SRV_SESSION)pSession);
}

static
NTSTATUS
SrvConnection2AcquireSessionId_inlock(
   PLWIO_SRV_CONNECTION pConnection,
   PULONG64             pUid
   )
{
    NTSTATUS ntStatus = 0;
    ULONG64  candidateUid = pConnection->ullNextAvailableUid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_SRV_SESSION_2 pSession = NULL;

        /* 0 is never a valid session vuid */

        if ((candidateUid == 0) || (candidateUid == UINT64_MAX))
        {
            candidateUid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pConnection->pSessionCollection,
                        &candidateUid,
                        (PVOID*)&pSession);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            candidateUid++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateUid != pConnection->ullNextAvailableUid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_SESSIONS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pUid = candidateUid;

    /* Increment by 1 by make sure to deal with wrap-around */

    candidateUid++;
    pConnection->ullNextAvailableUid = candidateUid ? candidateUid : 1;

cleanup:

    return ntStatus;

error:

    *pUid = 0;

    goto cleanup;
}

static
NTSTATUS
SrvConnectionCreateSessionCollection(
   PLWRTL_RB_TREE* ppSessionCollection
   )
{
    return LwRtlRBTreeCreate(
                &SrvConnectionSessionCompare,
                NULL,
                &SrvConnectionSessionRelease,
                ppSessionCollection);
}

static
NTSTATUS
SrvConnection2CreateSessionCollection(
   PLWRTL_RB_TREE* ppSessionCollection
   )
{
    return LwRtlRBTreeCreate(
                &SrvConnection2SessionCompare,
                NULL,
                &SrvConnection2SessionRelease,
                ppSessionCollection);
}

static
int
SrvConnection2SessionCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG64 pUid1 = (PULONG64)pKey1;
    PULONG64 pUid2 = (PULONG64)pKey2;

    assert (pUid1 != NULL);
    assert (pUid2 != NULL);

    if (*pUid1 > *pUid2)
    {
        return 1;
    }
    else if (*pUid1 < *pUid2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
SrvConnection2SessionRelease(
    PVOID pSession
    )
{
    SrvSession2Release((PLWIO_SRV_SESSION_2)pSession);
}

static
int
SrvConnection2AsyncStateCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG64 pAsyncId1 = (PULONG64)pKey1;
    PULONG64 pAsyncId2 = (PULONG64)pKey2;

    if (*pAsyncId1 > *pAsyncId2)
    {
        return 1;
    }
    else if (*pAsyncId1 < *pAsyncId2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
SrvConnection2AsyncStateRelease(
    PVOID pAsyncState
    )
{
    SrvAsyncStateRelease((PLWIO_ASYNC_STATE)pAsyncState);
}

static
VOID
SrvConnectionRundown(
    PLWIO_SRV_CONNECTION pConnection
    )
{
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_SESSION pRundownSessionList = NULL;
    PLWIO_SRV_SESSION_2 pRundownSession2List = NULL;

    // TODO: Merge with SrvConnectionSetInvalid()

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pConnection->mutex);

    LWIO_ASSERT(SrvConnectionIsInvalid_inlock(pConnection));

    switch (pConnection->protocolVer)
    {
        case SMB_PROTOCOL_VERSION_1:
            SrvEnumSessionCollection(
                    pConnection->pSessionCollection,
                    SrvConnectionGatherRundownSessionListCallback,
                    &pRundownSessionList);
            break;
        case SMB_PROTOCOL_VERSION_2:
            SrvEnumSession2Collection(
                    pConnection->pSessionCollection,
                    SrvConnection2GatherRundownSessionListCallback,
                    &pRundownSession2List);
            break;
        default:
            break;
    }

    SrvEnumAsyncStateCollection(
            pConnection->pAsyncStateCollection,
            SrvConnectionRundownAsyncStateCallback,
            NULL);

    LwRtlRBTreeRemoveAll(pConnection->pAsyncStateCollection);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    // Cannot rundown with lock held as they self-remove
    SrvConnectionRundownSessionList(pRundownSessionList);
    SrvConnection2RundownSessionList(pRundownSession2List);
}

static
BOOLEAN
SrvConnectionGatherRundownSessionListCallback(
    PLWIO_SRV_SESSION pSession,
    PVOID pContext
    )
{
    PLWIO_SRV_SESSION* ppRundownList = (PLWIO_SRV_SESSION*) pContext;

    LWIO_ASSERT(!pSession->pRundownNext);
    pSession->pRundownNext = *ppRundownList;

    SrvSessionAcquire(pSession);
    *ppRundownList = pSession;

    return TRUE;
}

static
BOOLEAN
SrvConnection2GatherRundownSessionListCallback(
    PLWIO_SRV_SESSION_2 pSession,
    PVOID pContext
    )
{
    PLWIO_SRV_SESSION_2* ppRundownList = (PLWIO_SRV_SESSION_2*) pContext;

    LWIO_ASSERT(!pSession->pRundownNext);
    pSession->pRundownNext = *ppRundownList;

    SrvSession2Acquire(pSession);
    *ppRundownList = pSession;

    return TRUE;
}

static
VOID
SrvConnectionRundownSessionList(
    PLWIO_SRV_SESSION pRundownList
    )
{
    while (pRundownList)
    {
        PLWIO_SRV_SESSION pSession = pRundownList;
        pRundownList = pSession->pRundownNext;

        SrvSessionRundown(pSession);
        SrvSessionRelease(pSession);
    }
}

static
VOID
SrvConnection2RundownSessionList(
    PLWIO_SRV_SESSION_2 pRundownList
    )
{
    while (pRundownList)
    {
        PLWIO_SRV_SESSION_2 pSession = pRundownList;
        pRundownList = pSession->pRundownNext;

        SrvSession2Rundown(pSession);
        SrvSession2Release(pSession);
    }
}

static
BOOLEAN
SrvConnectionRundownAsyncStateCallback(
    PLWIO_ASYNC_STATE pAsyncState,
    PVOID pContext
    )
{
    SrvAsyncStateCancel(pAsyncState);

    return TRUE;
}

