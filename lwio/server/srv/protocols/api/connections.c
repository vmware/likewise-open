/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        connections.c
 *
 * Abstract:
 *
 *        Likewise File System Driver (Srv)
 *
 *        Protocol API
 *
 *        Connection Management
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *          Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
NTSTATUS
SrvProtocolCountCandidateConnections(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolEnumCandidateConnections(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolProcessCandidateConnection(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolProcessCandidateTree(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolProcessCandidateConnection2(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolProcessCandidateTree2(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolProcessConnection_level_0(
    ULONG   ulConnId,
    PBYTE   pBuffer,
    ULONG   ulBufferSize,
    PULONG  pulBytesUsed
    );

static
NTSTATUS
SrvProtocolProcessSession_level_1(
    ULONG   ulConnId,
    ULONG   ulShareType,
    ULONG   ulTotalOpenFileCount,
    ULONG   ulTotalNumUsers,
    ULONG   ulConnectedTime,
    PWSTR   pwszUserName,
    PWSTR   pwszNetName,
    PBYTE   pBuffer,
    ULONG   ulBufferSize,
    PULONG  pulBytesUsed
    );

static
VOID
SrvProtocolFreeConnectionEnumQueryContents(
    PSRV_PROTOCOL_CONNECTION_ENUM_QUERY pConnectionEnumQuery
    );


NTSTATUS
SrvProtocolEnumerateConnections(
    PWSTR  pwszComputerName,
    PWSTR  pwszShareName,
    ULONG  ulInfoLevel,
    ULONG  ulPreferredMaxLength,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed,
    PULONG pulEntriesRead,
    PULONG pulTotalEntries,
    PULONG pulResumeHandle
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    BOOLEAN  bMoreData = FALSE;
    SRV_PROTOCOL_CONNECTION_ENUM_QUERY ConnectionEnumQuery =
    {
            .pwszComputerName     = pwszComputerName,
            .pwszShareName        = pwszShareName,
            .pClientAddress       = NULL,
            .clientAddrLen        = 0,
            .pwszClientHost       = NULL,
            .ulInfoLevel          = ulInfoLevel,
            .ulPreferredMaxLength = ulPreferredMaxLength,
            .iEntryIndex          = 0,
            .iResumeIndex         = pulResumeHandle ? *pulResumeHandle : 0,
            .ulEntriesRead        = 0,
            .ulTotalEntries       = 0,
            .pBuffer              = pBuffer,
            .ulBufferSize         = ulBufferSize,
            .ulBytesUsed          = 0,
            .pQueryAddress        = NULL
    };

    if (pwszComputerName)
    {
        wchar16_t wszPrefix[] = {'\\','\\', 0};
        size_t    sPrefixLen =
                (sizeof(wszPrefix)-sizeof(wszPrefix[0]))/sizeof(wszPrefix[0]);

        if (!SMBWc16snCmp(pwszComputerName, &wszPrefix[0], sPrefixLen))
        {
            pwszComputerName += sPrefixLen;
        }

        ConnectionEnumQuery.pwszComputerName = pwszComputerName;

        ntStatus = SrvSocketGetAddrInfoW(
                        pwszComputerName,
                        &ConnectionEnumQuery.pQueryAddress);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gProtocolApiGlobals.mutex);

    ntStatus = LwRtlRBTreeTraverse(
                    gProtocolApiGlobals.pConnections,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvProtocolCountCandidateConnections,
                    &ConnectionEnumQuery);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeTraverse(
                    gProtocolApiGlobals.pConnections,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvProtocolEnumCandidateConnections,
                    &ConnectionEnumQuery);
    /* If we still have more data to read, then return MORE_ENTRIES */
    if (ntStatus == STATUS_END_OF_FILE &&
        ConnectionEnumQuery.ulEntriesRead < ConnectionEnumQuery.ulTotalEntries)
    {
        bMoreData = TRUE;
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed    = ConnectionEnumQuery.ulBytesUsed;
    *pulEntriesRead  = ConnectionEnumQuery.ulEntriesRead;
    *pulTotalEntries = ConnectionEnumQuery.ulTotalEntries;
    if (pulResumeHandle)
    {
        *pulResumeHandle =
                ConnectionEnumQuery.iResumeIndex + ConnectionEnumQuery.ulEntriesRead;
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bInLock, &gProtocolApiGlobals.mutex);

    SrvProtocolFreeConnectionEnumQueryContents(&ConnectionEnumQuery);

    return (NT_SUCCESS(ntStatus) && bMoreData ? STATUS_MORE_ENTRIES : ntStatus);

error:
    *pulBytesUsed    = 0;
    *pulEntriesRead  = 0;
    *pulTotalEntries = 0;

    if (pBuffer && ulBufferSize)
    {
        memset(pBuffer, 0, ulBufferSize);
    }

    goto cleanup;
}


static
NTSTATUS
SrvProtocolCountCandidateConnections(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = (PLWIO_SRV_CONNECTION)pData;
    PSRV_PROTOCOL_CONNECTION_ENUM_QUERY pConnectionEnumQuery =
                                    (PSRV_PROTOCOL_CONNECTION_ENUM_QUERY)pUserData;
    ULONG ulTotalConnectionCount = pConnectionEnumQuery->ulTotalEntries;
    ULONG ulConnectionCount = 0;


    if (pConnectionEnumQuery->pQueryAddress)
    {
        /*
         * Look for connections by computer address first in case
         * that was the qualifier string
         */
        struct addrinfo* pCursor = pConnectionEnumQuery->pQueryAddress;
        BOOLEAN bMatch = FALSE;

        for (; !bMatch && (pCursor != NULL); pCursor = pCursor->ai_next)
        {
            ntStatus = SrvSocketCompareAddress(
                            pConnection->pClientAddress,
                            pConnection->clientAddrLen,
                            pCursor->ai_addr,
                            pCursor->ai_addrlen,
                            &bMatch);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (!bMatch)
        {
            pConnection = NULL;
        }
    }

    if (pConnection)
    {
        switch (SrvConnectionGetProtocolVersion(pConnection))
        {
            case SMB_PROTOCOL_VERSION_1:
                ntStatus = SrvConnectionGetConnectionCount(
                                pConnection,
                                pConnectionEnumQuery->pwszShareName,
                                &ulConnectionCount);

                break;

            case SMB_PROTOCOL_VERSION_2:
                ntStatus = SrvConnectionGetConnectionCount2(
                                pConnection,
                                pConnectionEnumQuery->pwszShareName,
                                &ulConnectionCount);

                break;

            case SMB_PROTOCOL_VERSION_UNKNOWN:
                /* Ignore connections that are still being established */
                break;

            default:
                ntStatus = STATUS_INTERNAL_ERROR;
                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ulTotalConnectionCount += ulConnectionCount;

        pConnectionEnumQuery->ulTotalEntries = ulTotalConnectionCount;
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
SrvProtocolEnumCandidateConnections(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = (PLWIO_SRV_CONNECTION)pData;
    PSRV_PROTOCOL_CONNECTION_ENUM_QUERY pConnectionEnumQuery =
                                    (PSRV_PROTOCOL_CONNECTION_ENUM_QUERY)pUserData;
    BOOLEAN bInLock = FALSE;
    PWSTR pwszClientHost = NULL;

    if (pConnectionEnumQuery->pQueryAddress)
    {
        /*
         * Look for connections by computer address first in case
         * that was the qualifier string
         */
        struct addrinfo* pCursor = pConnectionEnumQuery->pQueryAddress;
        BOOLEAN bMatch = FALSE;

        for (; !bMatch && (pCursor != NULL); pCursor = pCursor->ai_next)
        {
            ntStatus = SrvSocketCompareAddress(
                            pConnection->pClientAddress,
                            pConnection->clientAddrLen,
                            pCursor->ai_addr,
                            pCursor->ai_addrlen,
                            &bMatch);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (!bMatch)
        {
            pConnection = NULL;
        }
    }

    if (pConnection)
    {
        LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->mutex);

        ntStatus = SrvSocketAddressToStringW(pConnection->pClientAddress,
                                             &pwszClientHost);
        BAIL_ON_NT_STATUS(ntStatus);

        pConnectionEnumQuery->pwszClientHost    = pwszClientHost;
        pConnectionEnumQuery->pClientAddress    = pConnection->pClientAddress;
        pConnectionEnumQuery->clientAddrLen     = pConnection->clientAddrLen;
        pConnectionEnumQuery->ulConnectionResId
            = pConnection->resource.ulResourceId;

        ntStatus = WireGetCurrentNTTime(&pConnectionEnumQuery->llCurTime);
        BAIL_ON_NT_STATUS(ntStatus);

        switch (SrvConnectionGetProtocolVersion(pConnection))
        {
            case SMB_PROTOCOL_VERSION_1:
                ntStatus = LwRtlRBTreeTraverse(
                                pConnection->pSessionCollection,
                                LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                                &SrvProtocolProcessCandidateConnection,
                                pConnectionEnumQuery);
                break;

            case SMB_PROTOCOL_VERSION_2:
                ntStatus = LwRtlRBTreeTraverse(
                                pConnection->pSessionCollection,
                                LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                                &SrvProtocolProcessCandidateConnection2,
                                pConnectionEnumQuery);
                break;

            case SMB_PROTOCOL_VERSION_UNKNOWN:
                /* Ignore connections that are still being established */
                break;

            default:
                ntStatus = STATUS_INTERNAL_ERROR;
                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pbContinue = TRUE;

cleanup:
    pConnectionEnumQuery->pClientAddress  = NULL;
    pConnectionEnumQuery->clientAddrLen   = 0;
    pConnectionEnumQuery->pwszClientHost  = NULL;

    SRV_SAFE_FREE_MEMORY(pwszClientHost);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:
    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessCandidateConnection(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION pSession = (PLWIO_SRV_SESSION)pData;
    PSRV_PROTOCOL_CONNECTION_ENUM_QUERY pConnectionEnumQuery =
                                (PSRV_PROTOCOL_CONNECTION_ENUM_QUERY)pUserData;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    ntStatus = LwRtlRBTreeTraverse(
                    pSession->pTreeCollection,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvProtocolProcessCandidateTree,
                    pConnectionEnumQuery);
    BAIL_ON_NT_STATUS(ntStatus);

    *pbContinue = TRUE;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:
    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessCandidateTree(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_TREE pTree = (PLWIO_SRV_TREE)pData;
    PSRV_PROTOCOL_CONNECTION_ENUM_QUERY pConnectionEnumQuery =
                                    (PSRV_PROTOCOL_CONNECTION_ENUM_QUERY)pUserData;
    BOOLEAN bInTreeLock = FALSE;

    if (pConnectionEnumQuery->pwszShareName)
    {
        if (!LwRtlWC16StringIsEqual(pConnectionEnumQuery->pwszShareName,
                                    pTree->pShareInfo->pwszName,
                                    FALSE))
        {
            pTree = NULL;
        }
    }

    if (pTree)
    {
        if (pConnectionEnumQuery->iEntryIndex < pConnectionEnumQuery->iResumeIndex)
        {
            pConnectionEnumQuery->iEntryIndex++;

            pTree = NULL; // Skip
        }
    }

    if (pTree)
    {
        ULONG ulBytesUsed = 0;
        ULONG ulConnId = 0;
        ULONG ulShareType = 0;
        ULONG ulTotalOpenFileCount = 0;
        ULONG ulTotalNumUsers = 1;
        ULONG ulConnectedTime = 0;
        PWSTR pwszUserName = NULL;
        PWSTR pwszNetName = NULL;
        PLWIO_SRV_SESSION pSession = pTree->pSession;

        LWIO_LOCK_RWMUTEX_SHARED(bInTreeLock, &pTree->mutex);

        ulConnId    = pTree->resource.ulResourceId;
        ulShareType = (ULONG)pTree->pShareInfo->service;

        if (pConnectionEnumQuery->ulInfoLevel == 1)
        {
            ulTotalOpenFileCount = pTree->ulNumOpenFiles;
            /*
             * There's only one user per connection in user-level security
             */
            ulTotalNumUsers      = 1;
            ulConnectedTime      = (pConnectionEnumQuery->llCurTime -
                                    pSession->llBirthTime)
                                   /WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS;
            pwszUserName         = pSession->pwszClientPrincipalName;

            if (!pConnectionEnumQuery->pwszShareName)
            {
                pwszNetName = pTree->pShareInfo->pwszName;
            }
            else
            {
                pwszNetName = pConnectionEnumQuery->pwszClientHost;
            }
        }

        switch (pConnectionEnumQuery->ulInfoLevel)
        {
            case 0:
                ntStatus = SrvProtocolProcessConnection_level_0(
                                ulConnId,
                                pConnectionEnumQuery->pBuffer,
                                pConnectionEnumQuery->ulBufferSize,
                                &ulBytesUsed);
                break;

            case 1:
                ntStatus = SrvProtocolProcessSession_level_1(
                                ulConnId,
                                ulShareType,
                                ulTotalOpenFileCount,
                                ulTotalNumUsers,
                                ulConnectedTime,
                                pwszUserName,
                                pwszNetName,
                                pConnectionEnumQuery->pBuffer,
                                pConnectionEnumQuery->ulBufferSize,
                                &ulBytesUsed);
                break;

            default:
                ntStatus = STATUS_INVALID_INFO_CLASS;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (pConnectionEnumQuery->ulEntriesRead > 0 &&
            (pConnectionEnumQuery->ulPreferredMaxLength <
             pConnectionEnumQuery->ulBytesUsed + ulBytesUsed))
        {
            ntStatus = STATUS_END_OF_FILE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pConnectionEnumQuery->pBuffer      += ulBytesUsed;
        pConnectionEnumQuery->ulBufferSize -= ulBytesUsed;
        pConnectionEnumQuery->ulBytesUsed  += ulBytesUsed;

        pConnectionEnumQuery->iEntryIndex++;
        pConnectionEnumQuery->ulEntriesRead++;
    }

    *pbContinue = TRUE;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInTreeLock, &pTree->mutex);

    return ntStatus;

error:
    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessCandidateConnection2(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION_2 pSession = (PLWIO_SRV_SESSION_2)pData;
    PSRV_PROTOCOL_CONNECTION_ENUM_QUERY pConnectionEnumQuery =
                                (PSRV_PROTOCOL_CONNECTION_ENUM_QUERY)pUserData;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    ntStatus = LwRtlRBTreeTraverse(
                    pSession->pTreeCollection,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvProtocolProcessCandidateTree2,
                    pConnectionEnumQuery);
    BAIL_ON_NT_STATUS(ntStatus);

    *pbContinue = TRUE;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:
    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessCandidateTree2(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_TREE_2 pTree = (PLWIO_SRV_TREE_2)pData;
    PSRV_PROTOCOL_CONNECTION_ENUM_QUERY pConnectionEnumQuery =
                                    (PSRV_PROTOCOL_CONNECTION_ENUM_QUERY)pUserData;
    BOOLEAN bInTreeLock = FALSE;

    if (pConnectionEnumQuery->pwszShareName)
    {
        if (!LwRtlWC16StringIsEqual(pConnectionEnumQuery->pwszShareName,
                                    pTree->pShareInfo->pwszName,
                                    FALSE))
        {
            pTree = NULL;
        }
    }

    if (pTree)
    {
        if (pConnectionEnumQuery->iEntryIndex < pConnectionEnumQuery->iResumeIndex)
        {
            pConnectionEnumQuery->iEntryIndex++;

            pTree = NULL; // Skip
        }
    }

    if (pTree)
    {
        ULONG ulBytesUsed = 0;
        ULONG ulConnId = 0;
        ULONG ulShareType = 0;
        ULONG ulTotalOpenFileCount = 0;
        ULONG ulTotalNumUsers = 0;
        ULONG ulConnectedTime = 0;
        PWSTR pwszUserName = NULL;
        PWSTR pwszNetName = NULL;
        PLWIO_SRV_SESSION_2 pSession = pTree->pSession;

        LWIO_LOCK_RWMUTEX_SHARED(bInTreeLock, &pTree->mutex);

        ulConnId    = pTree->resource.ulResourceId;
        ulShareType = (ULONG)pTree->pShareInfo->service;

        if (pConnectionEnumQuery->ulInfoLevel == 1)
        {
            ulTotalOpenFileCount = pTree->ulNumOpenFiles;
            /*
             * There's only one user per connection in user-level security
             */
            ulTotalNumUsers      = 1;
            ulConnectedTime      = (pConnectionEnumQuery->llCurTime -
                                    pSession->llBirthTime)
                                   /WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS;
            pwszUserName         = pSession->pwszClientPrincipalName;

            if (!pConnectionEnumQuery->pwszShareName)
            {
                pwszNetName = pTree->pShareInfo->pwszName;
            }
            else
            {
                pwszNetName = pConnectionEnumQuery->pwszClientHost;
            }
        }

        switch (pConnectionEnumQuery->ulInfoLevel)
        {
            case 0:
                ntStatus = SrvProtocolProcessConnection_level_0(
                                ulConnId,
                                pConnectionEnumQuery->pBuffer,
                                pConnectionEnumQuery->ulBufferSize,
                                &ulBytesUsed);
                break;

            case 1:
                ntStatus = SrvProtocolProcessSession_level_1(
                                ulConnId,
                                ulShareType,
                                ulTotalOpenFileCount,
                                ulTotalNumUsers,
                                ulConnectedTime,
                                pwszUserName,
                                pwszNetName,
                                pConnectionEnumQuery->pBuffer,
                                pConnectionEnumQuery->ulBufferSize,
                                &ulBytesUsed);
                break;
            default:
                ntStatus = STATUS_INVALID_INFO_CLASS;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (pConnectionEnumQuery->ulEntriesRead > 0 &&
            (pConnectionEnumQuery->ulPreferredMaxLength <
             pConnectionEnumQuery->ulBytesUsed + ulBytesUsed))
        {
            ntStatus = STATUS_END_OF_FILE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pConnectionEnumQuery->pBuffer      += ulBytesUsed;
        pConnectionEnumQuery->ulBufferSize -= ulBytesUsed;
        pConnectionEnumQuery->ulBytesUsed  += ulBytesUsed;

        pConnectionEnumQuery->iEntryIndex++;
        pConnectionEnumQuery->ulEntriesRead++;
    }

    *pbContinue = TRUE;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bInTreeLock, &pTree->mutex);

    return ntStatus;

error:
    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessConnection_level_0(
    ULONG   ulConnId,
    PBYTE   pBuffer,
    ULONG   ulBufferSize,
    PULONG  pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulBytesUsed = 0;
    CONNECTION_INFO_0 ConnectionInfo = {0};

    ConnectionInfo.coni0_id = ulConnId;

    ntStatus = LwConnectionInfoMarshalEnumOutputInfo_level_0(
                    &ConnectionInfo,
                    pBuffer,
                    ulBufferSize,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = ulBytesUsed;

cleanup:
    return ntStatus;

error:
    *pulBytesUsed = 0;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessSession_level_1(
    ULONG   ulConnId,
    ULONG   ulShareType,
    ULONG   ulTotalOpenFileCount,
    ULONG   ulTotalNumUsers,
    ULONG   ulConnectedTime,
    PWSTR   pwszUserName,
    PWSTR   pwszNetName,
    PBYTE   pBuffer,
    ULONG   ulBufferSize,
    PULONG  pulBytesUsed
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulBytesUsed = 0;
    CONNECTION_INFO_1 ConnectionInfo = {0};

    ConnectionInfo.coni1_id         = ulConnId;
    ConnectionInfo.coni1_type       = ulShareType;
    ConnectionInfo.coni1_num_opens  = ulTotalOpenFileCount;
    ConnectionInfo.coni1_num_users  = ulTotalNumUsers,
    ConnectionInfo.coni1_time       = ulConnectedTime;
    ConnectionInfo.coni1_username   = pwszUserName;
    ConnectionInfo.coni1_netname    = pwszNetName;

    ntStatus = LwConnectionInfoMarshalEnumOutputInfo_level_1(
                    &ConnectionInfo,
                    pBuffer,
                    ulBufferSize,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed = ulBytesUsed;

cleanup:
    return ntStatus;

error:
    *pulBytesUsed = 0;

    goto cleanup;
}

static
VOID
SrvProtocolFreeConnectionEnumQueryContents(
    PSRV_PROTOCOL_CONNECTION_ENUM_QUERY pConnectionEnumQuery
    )
{
    if (pConnectionEnumQuery->pQueryAddress)
    {
        freeaddrinfo(pConnectionEnumQuery->pQueryAddress);
    }
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
