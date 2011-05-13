/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvProtocolExecute_SMB_V1_Filter(
    PSRV_EXEC_CONTEXT pContext
    );

static
VOID
SrvProtocolFreeExecContext(
    PSRV_PROTOCOL_EXEC_CONTEXT pContext
    );

NTSTATUS
SrvProtocolInit(
    IN PLW_THREAD_POOL ThreadPool,
    IN PLWIO_PACKET_ALLOCATOR hPacketAllocator,
    IN PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bSupportSMBV2 = FALSE;

    ntStatus = LwErrnoToNtStatus(pthread_rwlock_init(&gProtocolApiGlobals.mutex, NULL));
    BAIL_ON_NT_STATUS(ntStatus);
    gProtocolApiGlobals.pMutex = &gProtocolApiGlobals.mutex;

    ntStatus = LwErrnoToNtStatus(pthread_rwlock_init(&gProtocolApiGlobals.TransportStartStopMutex, NULL));
    BAIL_ON_NT_STATUS(ntStatus);
    gProtocolApiGlobals.pTransportStartStopMutex = &gProtocolApiGlobals.TransportStartStopMutex;

    gProtocolApiGlobals.hPacketAllocator = hPacketAllocator;
    gProtocolApiGlobals.pShareList = pShareList;

    SrvProtocolInitConfig(&gProtocolApiGlobals.config);

    ntStatus = SrvProtocolReadConfig(&gProtocolApiGlobals.config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProtocolInit_SMB_V1();
    BAIL_ON_NT_STATUS(ntStatus);

    bSupportSMBV2 = SrvProtocolConfigIsSmb2Enabled();
    if (bSupportSMBV2)
    {
        ntStatus = SrvProtocolInit_SMB_V2();
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvProtocolTransportDriverInit(
                   &gProtocolApiGlobals,
                   ThreadPool);
    BAIL_ON_NT_STATUS(ntStatus);

    if (SrvProtocolConfigIsTransportEnabled())
    {
        ntStatus = SrvProtocolStart();
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    SrvProtocolShutdown();
    goto cleanup;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
SrvProtocolExecute(
    PSRV_EXEC_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    SrvAcquireExecContext(pContext);

    LWIO_LOCK_MUTEX(bInLock, &pContext->execMutex);

    SrvMpxTrackerSetExecutingExecContext(pContext);

    ntStatus = SrvProtocolAddContext(pContext, FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pContext->pSmbRequest->netbiosOpcode !=
        SRV_NETBIOS_OPCODE_SESSION_MESSAGE)
    {
        switch(pContext->pSmbRequest->netbiosOpcode)
        {
            case SRV_NETBIOS_OPCODE_SESSION_REQUEST:
                SMBPacketAllocate(
                    pContext->pConnection->hPacketAllocator,
                    &pContext->pSmbResponse);

                ntStatus = SrvAllocateMemory(
                               sizeof(NETBIOS_HEADER),
                               (PVOID*)&pContext->pSmbResponse->pRawBuffer);
                BAIL_ON_NT_STATUS(ntStatus);

                pContext->pSmbResponse->bufferUsed = sizeof(NETBIOS_HEADER);

                pContext->pSmbResponse->pNetBIOSHeader =
                    (NETBIOS_HEADER *) pContext->pSmbResponse->pRawBuffer;

                // Store the netbios response opcode in the response packet
                // netbios header length
                pContext->pSmbResponse->pNetBIOSHeader->len =
                    htonl(SRV_NETBIOS_OPCODE_SESSION_POSITIVE_RESPONSE<<24);

                break;

            case SRV_NETBIOS_OPCODE_KEEPALIVE:
                //no-op
                break;

            default:
                SrvConnectionSetInvalid(pContext->pConnection);

                ntStatus = STATUS_CONNECTION_RESET;
                break;
        }
    } else
    {
        if ((pContext->pSmbRequest->pSMBHeader->command == COM_NEGOTIATE) &&
            (SrvConnectionGetState(pContext->pConnection) !=
                                            LWIO_SRV_CONN_STATE_INITIAL))
        {
            SrvConnectionSetInvalid(pContext->pConnection);

            ntStatus = STATUS_CONNECTION_RESET;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        switch (pContext->pSmbRequest->protocolVer)
        {
            case SMB_PROTOCOL_VERSION_1:

                ntStatus = SrvProtocolExecute_SMB_V1_Filter(pContext);

                break;

            case SMB_PROTOCOL_VERSION_2:

                ntStatus = SrvProtocolExecute_SMB_V2(pContext);

                break;

            default:

                ntStatus = STATUS_INTERNAL_ERROR;

                break;
        }
    }
    BAIL_ON_NT_STATUS(ntStatus);

    // Remove mid before sending a response since the client can immediately
    // re-use it. Note that, even if we are not sending a response for
    // some reason, it is fine to remove the mid since we are done with the
    // request.  Also note that in a ZCT read file case, we already sent
    // the response and will not send it here, but we already removed
    // the mid from the request.

    SrvMpxTrackerRemoveExecContext(pContext);

    // Cleanup any protocol state before sending a response.
    if (pContext->pProtocolContext)
    {
        pContext->pfnFreeContext(pContext->pProtocolContext);
        pContext->pProtocolContext = NULL;
    }

    if (pContext->pSmbResponse && pContext->pSmbResponse->pNetBIOSHeader->len)
    {
        ntStatus = SrvProtocolTransportSendResponse(
                        pContext->pConnection,
                        pContext->pSmbResponse,
                        pContext->pStatInfo);
        BAIL_ON_NT_STATUS(ntStatus);
    }

error:
    LWIO_UNLOCK_MUTEX(bInLock, &pContext->execMutex);

    switch (ntStatus)
    {
        case STATUS_PENDING:
            ntStatus = STATUS_SUCCESS;
            break;

        default:
            break;
    }

    SrvReleaseExecContext(pContext);

    return ntStatus;
}

////////////////////////////////////////////////////////////////////////

static
VOID
SrvProtocolExecuteCallback(
    PVOID pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pExecContext = (PSRV_EXEC_CONTEXT)pContext;

    ntStatus = SrvProtocolExecute(pExecContext);
    // TODO - Add additional logging here if (!NT_SUCCESS(ntStatus))

    SrvReleaseExecContext(pExecContext);

    return;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
SrvScheduleExecContextEx(
    IN PSRV_EXEC_CONTEXT pExecContext,
    IN LW_WORK_ITEM_FLAGS Flags
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    SrvAcquireExecContext(pExecContext);
    pExecContext->bInline = FALSE;

    ntStatus = SrvScheduleWorkItem(
                   pExecContext,
                   SrvProtocolExecuteCallback,
                   Flags);

    if (!NT_SUCCESS(ntStatus))
    {
        LWIO_LOG_ERROR(
            "Failed to schedule execution context (%s)\n",
            LwNtStatusToName(ntStatus));

        SrvReleaseExecContext(pExecContext);
    }

    return ntStatus;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
SrvScheduleExecContext(
    IN PSRV_EXEC_CONTEXT pExecContext
    )
{
    return SrvScheduleExecContextEx(pExecContext, 0);
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
SrvSchedulePriorityExecContext(
    IN PSRV_EXEC_CONTEXT pExecContext
    )
{
    return SrvScheduleExecContextEx(pExecContext, LW_WORK_ITEM_HIGH_PRIOTIRY);
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
SrvProtocolAddContext(
    PSRV_EXEC_CONTEXT pExecContext,
    BOOLEAN bInConnectionLock
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pExecContext->pProtocolContext)
    {
        ntStatus = SrvAllocateMemory(
                        sizeof(SRV_PROTOCOL_EXEC_CONTEXT),
                        (PVOID*)&pExecContext->pProtocolContext);
        BAIL_ON_NT_STATUS(ntStatus);

        pExecContext->pProtocolContext->protocolVersion =
            (bInConnectionLock ?
             SrvConnectionGetProtocolVersion_inlock(pExecContext->pConnection) :
             SrvConnectionGetProtocolVersion(pExecContext->pConnection));

        pExecContext->pfnFreeContext = &SrvProtocolFreeExecContext;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvProtocolExecute_SMB_V1_Filter(
    PSRV_EXEC_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pSmbRequest;

    switch (pSmbRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

                if (pContext->pStatInfo)
                {
                    ntStatus = SrvStatisticsPushMessage(
                                    pContext->pStatInfo,
                                    pSmbRequest->pSMBHeader->command,
                                    pSmbRequest->pNetBIOSHeader->len);
                    if (ntStatus)
                    {
                        LWIO_ASSERT(ntStatus != STATUS_PENDING);
                        ntStatus = SrvProtocolBuildErrorResponse_SMB_V1(
                                        pConnection,
                                        pSmbRequest->pSMBHeader,
                                        ntStatus,
                                        &pContext->pSmbResponse);
                        break;
                    }
                }

                ntStatus = SrvProcessNegotiate(pContext);
                if ((ntStatus != STATUS_SUCCESS) && (ntStatus != STATUS_PENDING))
                {
                    ntStatus = SrvProtocolBuildErrorResponse_SMB_V1(
                                    pConnection,
                                    pSmbRequest->pSMBHeader,
                                    ntStatus,
                                    &pContext->pSmbResponse);
                }

                if ((ntStatus == STATUS_SUCCESS) && pContext->pStatInfo)
                {
                    NTSTATUS ntStatus2 = STATUS_SUCCESS;

                    ntStatus2 = SrvStatisticsPopMessage(
                                    pContext->pStatInfo,
                                    pSmbRequest->pSMBHeader->command,
                                    ntohl(pContext->pSmbResponse->pNetBIOSHeader->len),
                                    pContext->pSmbResponse->pSMBHeader->error);
                }

                break;

        default:

                ntStatus = SrvProtocolExecute_SMB_V1(pContext);

                break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
VOID
SrvProtocolFreeExecContext(
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext
    )
{
    switch (pProtocolContext->protocolVersion)
    {
        case SMB_PROTOCOL_VERSION_1:

            if (pProtocolContext->pSmb1Context)
            {
                SrvProtocolFreeContext_SMB_V1(pProtocolContext->pSmb1Context);
            }

            break;

        case SMB_PROTOCOL_VERSION_2:

            if (pProtocolContext->pSmb2Context)
            {
                SrvProtocolFreeContext_SMB_V2(pProtocolContext->pSmb2Context);
            }

            break;

        default:

            break;
    }

    SrvFreeMemory(pProtocolContext);
}

BOOLEAN
SrvProtocolIsStarted(
    VOID
    )
{
    return SrvProtocolTransportDriverIsStarted(&gProtocolApiGlobals);
}

NTSTATUS
SrvProtocolStart(
    VOID
    )
{
    return SrvProtocolTransportDriverStart(&gProtocolApiGlobals);
}

BOOLEAN
SrvProtocolStop(
    IN BOOLEAN IsForce
    )
{
    return SrvProtocolTransportDriverStop(&gProtocolApiGlobals, IsForce);
}

VOID
SrvProtocolShutdown(
    VOID
    )
{
    SrvProtocolTransportDriverShutdown(&gProtocolApiGlobals);

    // Always shutdown all protocols regardless of enabled state
    // to allow implementing dynamic enable/disable where pre-existing
    // connections are not torn down.
    SrvProtocolShutdown_SMB_V1();
    SrvProtocolShutdown_SMB_V2();

    SrvProtocolFreeConfigContents(&gProtocolApiGlobals.config);

    gProtocolApiGlobals.hPacketAllocator = NULL;
    gProtocolApiGlobals.pShareList = NULL;

    if (gProtocolApiGlobals.pTransportStartStopMutex)
    {
        pthread_rwlock_destroy(&gProtocolApiGlobals.TransportStartStopMutex);
        gProtocolApiGlobals.pTransportStartStopMutex = NULL;
    }
    if (gProtocolApiGlobals.pMutex)
    {
        pthread_rwlock_destroy(&gProtocolApiGlobals.mutex);
        gProtocolApiGlobals.pMutex = NULL;
    }
}


