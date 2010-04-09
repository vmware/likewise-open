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
NTSTATUS
SrvProtocolCountCandidateSessions(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolEnumCandidateSessions(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolProcessCandidateSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolProcessCandidateSession2(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    );

static
NTSTATUS
SrvProtocolProcessSession_level_0(
    PWSTR  pwszUncClientname,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    );

static
NTSTATUS
SrvProtocolProcessSession_level_1(
    PWSTR   pwszUncClientname,
    PWSTR   pwszUsername,
    ULONG64 ullNumOpens,
    ULONG   ulUnixTime,
    ULONG   ulIdleTime,
    ULONG   ulUserFlags,
    PBYTE   pBuffer,
    ULONG   ulBufferSize,
    PULONG  pulBytesUsed
    );

static
NTSTATUS
SrvProtocolProcessSession_level_2(
    PWSTR  pwszUncClientname,
    PWSTR  pwszUsername,
    ULONG  ulNumOpens,
    ULONG  ulActiveTime,
    ULONG  ulIdleTime,
    ULONG  ulUserFlags,
    PWSTR  pwszClientType,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    );

static
NTSTATUS
SrvProtocolProcessSession_level_10(
    PWSTR  pwszUncClientname,
    PWSTR  pwszUsername,
    ULONG  ulActiveTime,
    ULONG  ulIdleTime,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    );

static
NTSTATUS
SrvProtocolProcessSession_level_502(
    PWSTR   pwszUncClientname,
    PWSTR   pwszUsername,
    ULONG64 ullNumOpens,
    ULONG   ulActiveTime,
    ULONG   ulIdleTime,
    ULONG   ulUserFlags,
    PWSTR   pwszClientType,
    PWSTR   pwszClientTransport,
    PBYTE   pBuffer,
    ULONG   ulBufferSize,
    PULONG  pulBytesUsed
    );

static
VOID
SrvProtocolFreeSessionQueryContents(
    PSRV_PROTOCOL_SESSION_ENUM_QUERY pSessionEnumQuery
    );

NTSTATUS
SrvProtocolEnumerateSessions(
    PWSTR  pwszUncClientname,
    PWSTR  pwszUsername,
    ULONG  ulInfoLevel,
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
    SRV_PROTOCOL_SESSION_ENUM_QUERY sessionEnumQuery =
    {
            .pwszUncClientname    = pwszUncClientname,
            .pClientAddress       = NULL,
            .clientAddrLen        = 0,
            .pwszUsername         = pwszUsername,
            .ulInfoLevel          = ulInfoLevel,
            .iEntryIndex          = 0,
            .iResumeIndex         = pulResumeHandle ? *pulResumeHandle : 0,
            .ulEntriesRead        = 0,
            .ulTotalEntries       = 0,
            .pBuffer              = pBuffer,
            .ulBufferSize         = ulBufferSize,
            .ulBytesUsed          = 0,
            .pQueryAddress        = NULL
    };

    if (pwszUncClientname)
    {
        ntStatus = SrvSocketGetAddrInfoW(
                        pwszUncClientname,
                        &sessionEnumQuery.pQueryAddress);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gProtocolApiGlobals.mutex);

    ntStatus = LwRtlRBTreeTraverse(
                    gProtocolApiGlobals.pConnections,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvProtocolCountCandidateSessions,
                    &sessionEnumQuery);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeTraverse(
                    gProtocolApiGlobals.pConnections,
                    LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                    &SrvProtocolEnumCandidateSessions,
                    &sessionEnumQuery);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesUsed    = sessionEnumQuery.ulBytesUsed;
    *pulEntriesRead  = sessionEnumQuery.ulEntriesRead;
    *pulTotalEntries = sessionEnumQuery.ulTotalEntries;
    if (pulResumeHandle)
    {
        *pulResumeHandle =
                sessionEnumQuery.iResumeIndex + sessionEnumQuery.ulEntriesRead;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gProtocolApiGlobals.mutex);

    SrvProtocolFreeSessionQueryContents(&sessionEnumQuery);

    return ntStatus;

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
SrvProtocolCountCandidateSessions(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = (PLWIO_SRV_CONNECTION)pData;
    PSRV_PROTOCOL_SESSION_ENUM_QUERY pSessionEnumQuery =
                                    (PSRV_PROTOCOL_SESSION_ENUM_QUERY)pUserData;
    BOOLEAN bContinue = TRUE;

    if (pSessionEnumQuery->pQueryAddress)
    {
        struct addrinfo* pCursor = pSessionEnumQuery->pQueryAddress;
        BOOLEAN bMatch = FALSE;

        for (; !bMatch && (pCursor != NULL); pCursor = pCursor->ai_next)
        {
            ntStatus = SrvSocketCompareAddress(
                            &pConnection->clientAddress,
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
        ULONG64 ullTotalSessionCount = pSessionEnumQuery->ulTotalEntries;
        ULONG64 ullSessionCount = 0;

        switch (SrvConnectionGetProtocolVersion(pConnection))
        {
            case SMB_PROTOCOL_VERSION_1:

                ntStatus = SrvConnectionGetSessionCount(
                                pConnection,
                                pSessionEnumQuery->pwszUsername,
                                &ullSessionCount);

                break;

            case SMB_PROTOCOL_VERSION_2:

                ntStatus = SrvConnection2GetSessionCount(
                                pConnection,
                                pSessionEnumQuery->pwszUsername,
                                &ullSessionCount);

                break;

            case SMB_PROTOCOL_VERSION_UNKNOWN:

                /* Ignore connections that are still being established */

                break;

            default:

                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ullTotalSessionCount += ullSessionCount;

        pSessionEnumQuery->ulTotalEntries =
                        SMB_MIN(UINT32_MAX, ullTotalSessionCount);

        if (pSessionEnumQuery->ulTotalEntries == UINT32_MAX)
        {
            bContinue = FALSE;
        }
    }

    *pbContinue = bContinue;

cleanup:

    return ntStatus;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolEnumCandidateSessions(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = (PLWIO_SRV_CONNECTION)pData;
    PSRV_PROTOCOL_SESSION_ENUM_QUERY pSessionEnumQuery =
                                    (PSRV_PROTOCOL_SESSION_ENUM_QUERY)pUserData;
    BOOLEAN bInLock = FALSE;

    if (pSessionEnumQuery->pQueryAddress)
    {
        struct addrinfo* pCursor = pSessionEnumQuery->pQueryAddress;
        BOOLEAN bMatch = FALSE;

        for (; !bMatch && (pCursor != NULL); pCursor = pCursor->ai_next)
        {
            ntStatus = SrvSocketCompareAddress(
                            &pConnection->clientAddress,
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

        pSessionEnumQuery->pClientAddress = &pConnection->clientAddress;
        pSessionEnumQuery->clientAddrLen  = pConnection->clientAddrLen;

        ntStatus = WireGetCurrentNTTime(&pSessionEnumQuery->llCurTime);
        BAIL_ON_NT_STATUS(ntStatus);

        switch (SrvConnectionGetProtocolVersion(pConnection))
        {
            case SMB_PROTOCOL_VERSION_1:

                ntStatus = LwRtlRBTreeTraverse(
                                pConnection->pSessionCollection,
                                LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                                &SrvProtocolProcessCandidateSession,
                                pSessionEnumQuery);

                break;

            case SMB_PROTOCOL_VERSION_2:

                ntStatus = LwRtlRBTreeTraverse(
                                pConnection->pSessionCollection,
                                LWRTL_TREE_TRAVERSAL_TYPE_IN_ORDER,
                                &SrvProtocolProcessCandidateSession2,
                                pSessionEnumQuery);

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

    pSessionEnumQuery->pClientAddress = NULL;
    pSessionEnumQuery->clientAddrLen  = 0;

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->mutex);

    return ntStatus;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessCandidateSession(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION pSession = (PLWIO_SRV_SESSION)pData;
    PSRV_PROTOCOL_SESSION_ENUM_QUERY pSessionEnumQuery =
                                    (PSRV_PROTOCOL_SESSION_ENUM_QUERY)pUserData;
    PWSTR   pwszClientname = NULL;
    BOOLEAN bInLock = FALSE;

    if (pSessionEnumQuery->pwszUsername)
    {
        BOOLEAN bIsMatch = FALSE;

        ntStatus = SrvSessionCheckPrincipal(
                        pSession,
                        pSessionEnumQuery->pwszUsername,
                        &bIsMatch);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!bIsMatch)
        {
            pSession = NULL;
        }
    }

    if (pSession)
    {
        if (pSessionEnumQuery->iEntryIndex < pSessionEnumQuery->iResumeIndex)
        {
            pSessionEnumQuery->iEntryIndex++;
            pSession = NULL; // Skip
        }
    }

    if (pSession)
    {
        ULONG     ulBytesUsed          = 0;
        ULONG     ulActiveTime         = 0;
        ULONG     ulIdleTime           = 0;
        wchar16_t wszClientType[]      = SRV_CLIENT_TYPE_W;
        wchar16_t wszClientTransport[] = {0};

        LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

        if (pSessionEnumQuery->ulInfoLevel != 0)
        {
            ulActiveTime = (pSessionEnumQuery->llCurTime -
                                pSession->llBirthTime)/WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS;

            ulIdleTime = (pSessionEnumQuery->llCurTime -
                              pSession->llLastActivityTime)/WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS;
        }

        ntStatus = SrvSocketAddressToStringW(
                                pSessionEnumQuery->pClientAddress,
                                &pwszClientname);
        BAIL_ON_NT_STATUS(ntStatus);

        switch (pSessionEnumQuery->ulInfoLevel)
        {
            case 0:

                ntStatus = SrvProtocolProcessSession_level_0(
                                pwszClientname,
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            case 1:

                ntStatus = SrvProtocolProcessSession_level_1(
                                pwszClientname,
                                pSession->pwszClientPrincipalName,
                                pSession->ullTotalFileCount,
                                ulActiveTime,
                                ulIdleTime,
                                pSession->ulUserFlags,
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            case 2:

                ntStatus = SrvProtocolProcessSession_level_2(
                                pwszClientname,
                                pSession->pwszClientPrincipalName,
                                pSession->ullTotalFileCount,
                                ulActiveTime,
                                ulIdleTime,
                                pSession->ulUserFlags,
                                &wszClientType[0],
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            case 10:

                ntStatus = SrvProtocolProcessSession_level_10(
                                pwszClientname,
                                pSession->pwszClientPrincipalName,
                                ulActiveTime,
                                ulIdleTime,
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            case 502:

                ntStatus = SrvProtocolProcessSession_level_502(
                                pwszClientname,
                                pSession->pwszClientPrincipalName,
                                pSession->ullTotalFileCount,
                                ulActiveTime,
                                ulIdleTime,
                                pSession->ulUserFlags,
                                &wszClientType[0],
                                &wszClientTransport[0],
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            default:

                ntStatus = STATUS_INVALID_INFO_CLASS;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pSessionEnumQuery->pBuffer      += ulBytesUsed;
        pSessionEnumQuery->ulBufferSize -= ulBytesUsed;
        pSessionEnumQuery->ulBytesUsed  += ulBytesUsed;

        pSessionEnumQuery->iEntryIndex++;
        pSessionEnumQuery->ulEntriesRead++;
    }

    *pbContinue = TRUE;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    SRV_SAFE_FREE_MEMORY(pwszClientname);

    return ntStatus;

error:

    *pbContinue = FALSE;

    if (ntStatus == STATUS_END_OF_FILE)
    {
        ntStatus = STATUS_SUCCESS;
    }

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessCandidateSession2(
    PVOID    pKey,
    PVOID    pData,
    PVOID    pUserData,
    PBOOLEAN pbContinue
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION_2 pSession = (PLWIO_SRV_SESSION_2)pData;
    PSRV_PROTOCOL_SESSION_ENUM_QUERY pSessionEnumQuery =
                                    (PSRV_PROTOCOL_SESSION_ENUM_QUERY)pUserData;
    PWSTR   pwszClientname = NULL;
    BOOLEAN bInLock = FALSE;

    if (pSessionEnumQuery->pwszUsername)
    {
        BOOLEAN bIsMatch = FALSE;

        ntStatus = SrvSession2CheckPrincipal(
                        pSession,
                        pSessionEnumQuery->pwszUsername,
                        &bIsMatch);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!bIsMatch)
        {
            pSession = NULL;
        }
    }

    if (pSession)
    {
        if (pSessionEnumQuery->iEntryIndex < pSessionEnumQuery->iResumeIndex)
        {
            pSessionEnumQuery->iEntryIndex++;
            pSession = NULL; // Skip
        }
    }

    if (pSession)
    {
        ULONG     ulBytesUsed          = 0;
        ULONG     ulActiveTime         = 0;
        ULONG     ulIdleTime           = 0;
        wchar16_t wszClientType[]      = SRV_CLIENT_TYPE_W;
        wchar16_t wszClientTransport[] = {0};

        ntStatus = SrvSocketAddressToStringW(
                                pSessionEnumQuery->pClientAddress,
                                &pwszClientname);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

        if (pSessionEnumQuery->ulInfoLevel != 0)
        {
            ulActiveTime = (pSessionEnumQuery->llCurTime -
                                pSession->llBirthTime)/WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS;

            ulIdleTime = (pSessionEnumQuery->llCurTime -
                              pSession->llLastActivityTime)/WIRE_FACTOR_SECS_TO_HUNDREDS_OF_NANOSECS;
        }

        switch (pSessionEnumQuery->ulInfoLevel)
        {
            case 0:

                ntStatus = SrvProtocolProcessSession_level_0(
                                pwszClientname,
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            case 1:

                ntStatus = SrvProtocolProcessSession_level_1(
                                pwszClientname,
                                pSession->pwszClientPrincipalName,
                                pSession->ullTotalFileCount,
                                ulActiveTime,
                                ulIdleTime,
                                pSession->ulUserFlags,
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            case 2:

                ntStatus = SrvProtocolProcessSession_level_2(
                                pwszClientname,
                                pSession->pwszClientPrincipalName,
                                pSession->ullTotalFileCount,
                                ulActiveTime,
                                ulIdleTime,
                                pSession->ulUserFlags,
                                &wszClientType[0],
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            case 10:

                ntStatus = SrvProtocolProcessSession_level_10(
                                pwszClientname,
                                pSession->pwszClientPrincipalName,
                                ulActiveTime,
                                ulIdleTime,
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            case 502:

                ntStatus = SrvProtocolProcessSession_level_502(
                                pwszClientname,
                                pSession->pwszClientPrincipalName,
                                pSession->ullTotalFileCount,
                                ulActiveTime,
                                ulIdleTime,
                                pSession->ulUserFlags,
                                &wszClientType[0],
                                &wszClientTransport[0],
                                pSessionEnumQuery->pBuffer,
                                pSessionEnumQuery->ulBufferSize,
                                &ulBytesUsed);

                break;

            default:

                ntStatus = STATUS_INVALID_INFO_CLASS;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pSessionEnumQuery->pBuffer      += ulBytesUsed;
        pSessionEnumQuery->ulBufferSize -= ulBytesUsed;
        pSessionEnumQuery->ulBytesUsed  += ulBytesUsed;

        pSessionEnumQuery->iEntryIndex++;
        pSessionEnumQuery->ulEntriesRead++;
    }

    *pbContinue = TRUE;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    SRV_SAFE_FREE_MEMORY(pwszClientname);

    return ntStatus;

error:

    *pbContinue = FALSE;

    if (ntStatus == STATUS_END_OF_FILE)
    {
        ntStatus = STATUS_SUCCESS;
    }

    goto cleanup;
}

static
NTSTATUS
SrvProtocolProcessSession_level_0(
    PWSTR  pwszUncClientname,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    )
{
    NTSTATUS       ntStatus    = STATUS_SUCCESS;
    ULONG          ulBytesUsed = 0;
    SESSION_INFO_0 sessionInfo = {0};

    sessionInfo.sesi0_cname = pwszUncClientname;

    ntStatus = LwSessionInfoMarshalEnumOutputInfo_level_0(
                    &sessionInfo,
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
    PWSTR   pwszUncClientname,
    PWSTR   pwszUsername,
    ULONG64 ullNumOpens,
    ULONG   ulUnixTime,
    ULONG   ulIdleTime,
    ULONG   ulUserFlags,
    PBYTE   pBuffer,
    ULONG   ulBufferSize,
    PULONG  pulBytesUsed
    )
{
    NTSTATUS       ntStatus    = STATUS_SUCCESS;
    ULONG          ulBytesUsed = 0;
    SESSION_INFO_1 sessionInfo = {0};

    sessionInfo.sesi1_cname      = pwszUncClientname;
    sessionInfo.sesi1_username   = pwszUsername;
    sessionInfo.sesi1_num_opens  = SMB_MIN(UINT32_MAX, ullNumOpens);
    sessionInfo.sesi1_time       = ulUnixTime;
    sessionInfo.sesi1_idle_time  = ulIdleTime;
    sessionInfo.sesi1_user_flags = ulUserFlags;

    ntStatus = LwSessionInfoMarshalEnumOutputInfo_level_1(
                    &sessionInfo,
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
SrvProtocolProcessSession_level_2(
    PWSTR  pwszUncClientname,
    PWSTR  pwszUsername,
    ULONG  ulNumOpens,
    ULONG  ulActiveTime,
    ULONG  ulIdleTime,
    ULONG  ulUserFlags,
    PWSTR  pwszClientType,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    )
{
    NTSTATUS       ntStatus    = STATUS_SUCCESS;
    ULONG          ulBytesUsed = 0;
    SESSION_INFO_2 sessionInfo = {0};

    sessionInfo.sesi2_cname       = pwszUncClientname;
    sessionInfo.sesi2_username    = pwszUsername;
    sessionInfo.sesi2_num_opens   = ulNumOpens;
    sessionInfo.sesi2_time        = ulActiveTime;
    sessionInfo.sesi2_idle_time   = ulIdleTime;
    sessionInfo.sesi2_user_flags  = ulUserFlags;
    sessionInfo.sesi2_cltype_name = pwszClientType;

    ntStatus = LwSessionInfoMarshalEnumOutputInfo_level_2(
                    &sessionInfo,
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
SrvProtocolProcessSession_level_10(
    PWSTR  pwszUncClientname,
    PWSTR  pwszUsername,
    ULONG  ulActiveTime,
    ULONG  ulIdleTime,
    PBYTE  pBuffer,
    ULONG  ulBufferSize,
    PULONG pulBytesUsed
    )
{
    NTSTATUS        ntStatus    = STATUS_SUCCESS;
    ULONG           ulBytesUsed = 0;
    SESSION_INFO_10 sessionInfo = {0};

    sessionInfo.sesi10_cname     = pwszUncClientname;
    sessionInfo.sesi10_username  = pwszUsername;
    sessionInfo.sesi10_time      = ulActiveTime;
    sessionInfo.sesi10_idle_time = ulIdleTime;

    ntStatus = LwSessionInfoMarshalEnumOutputInfo_level_10(
                    &sessionInfo,
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
SrvProtocolProcessSession_level_502(
    PWSTR   pwszUncClientname,
    PWSTR   pwszUsername,
    ULONG64 ullNumOpens,
    ULONG   ulActiveTime,
    ULONG   ulIdleTime,
    ULONG   ulUserFlags,
    PWSTR   pwszClientType,
    PWSTR   pwszClientTransport,
    PBYTE   pBuffer,
    ULONG   ulBufferSize,
    PULONG  pulBytesUsed
    )
{
    NTSTATUS         ntStatus    = STATUS_SUCCESS;
    ULONG            ulBytesUsed = 0;
    SESSION_INFO_502 sessionInfo = {0};

    sessionInfo.sesi502_cname       = pwszUncClientname;
    sessionInfo.sesi502_username    = pwszUsername;
    sessionInfo.sesi502_num_opens   = SMB_MIN(UINT32_MAX, ullNumOpens);
    sessionInfo.sesi502_time        = ulActiveTime;
    sessionInfo.sesi502_idle_time   = ulIdleTime;
    sessionInfo.sesi502_user_flags  = ulUserFlags;
    sessionInfo.sesi502_cltype_name = pwszClientType;
    sessionInfo.sesi502_transport   = pwszClientTransport;

    ntStatus = LwSessionInfoMarshalEnumOutputInfo_level_502(
                    &sessionInfo,
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
SrvProtocolFreeSessionQueryContents(
    PSRV_PROTOCOL_SESSION_ENUM_QUERY pSessionEnumQuery
    )
{
    if (pSessionEnumQuery->pQueryAddress)
    {
        freeaddrinfo(pSessionEnumQuery->pQueryAddress);
    }
}

