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
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Transport API
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvTransportInit(
    OUT PSRV_TRANSPORT_HANDLE phTransport,
    IN PSRV_TRANSPORT_PROTOCOL_DISPATCH pProtocolDispatch,
    IN OPTIONAL PSRV_PROTOCOL_TRANSPORT_CONTEXT pProtocolDispatchContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    NTSTATUS v4Status = STATUS_SUCCESS;
    NTSTATUS v6Status = STATUS_SUCCESS;
    PSRV_TRANSPORT_HANDLE_DATA pTransport = NULL;
    PLW_THREAD_POOL_ATTRIBUTES pAttrs = NULL;

    ntStatus = SrvAllocateMemory(sizeof(*pTransport), OUT_PPVOID(&pTransport));
    BAIL_ON_NT_STATUS(ntStatus);

    pTransport->Dispatch = *pProtocolDispatch;
    pTransport->pContext = pProtocolDispatchContext;

    ntStatus = LwRtlCreateThreadPoolAttributes(&pAttrs);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Our tasks sometimes make blocking IPC calls, so use a private set of task threads */
    LwRtlSetThreadPoolAttribute(pAttrs, LW_THREAD_POOL_OPTION_DELEGATE_TASKS, FALSE);
    /* Create one task thread per CPU */
    LwRtlSetThreadPoolAttribute(pAttrs, LW_THREAD_POOL_OPTION_TASK_THREADS, -1);
    /* We don't presently use work threads, so turn them off */
    LwRtlSetThreadPoolAttribute(pAttrs, LW_THREAD_POOL_OPTION_WORK_THREADS, 0);

    ntStatus = LwRtlCreateThreadPool(&pTransport->pPool, pAttrs);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Try to listen on both IPv6 and IPv4 interfaces */
    LWIO_LOG_VERBOSE("Attempting to create IPv6 listener");
    v6Status = SrvListenerInit(&pTransport->Listener6, pTransport, TRUE);
    LWIO_LOG_VERBOSE("Attempting to create IPv4 listener");
    v4Status = SrvListenerInit(&pTransport->Listener, pTransport, FALSE);

    /* Don't fail if only one of the listeners could not be started.  This
       could be for a variety of reasons:

       - We were not compiled with IPv6 support.
       - The system has no IPv6 support or configured IPv6 interfaces.
       - Listening on an IPv6 socket implicitly listens for IPv4 connections
         as well, so the IPv4 listener will fail with an "address in use" error.
         This is the case on default linux configurations.
    */
    if (v6Status != STATUS_SUCCESS && v4Status != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Could not establish any listeners");

        ntStatus = v4Status ? v4Status : v6Status;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    LwRtlFreeThreadPoolAttributes(&pAttrs);

    *phTransport = pTransport;

    return ntStatus;

error:

    SrvTransportShutdown(pTransport);
    pTransport = NULL;

    goto cleanup;
}

VOID
SrvTransportShutdown(
    IN OUT SRV_TRANSPORT_HANDLE hTransport
    )
{
    if (hTransport)
    {
        SrvListenerShutdown(&hTransport->Listener);
        SrvListenerShutdown(&hTransport->Listener6);
        LwRtlFreeThreadPool(&hTransport->pPool);
        SrvFreeMemory(hTransport);
    }
}

VOID
SrvTransportSocketGetAddress(
    IN PSRV_SOCKET              pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT SOCKLEN_T*              pAddressLength
    )
{
    SrvSocketGetAddress(pSocket, ppAddress, pAddressLength);
}

VOID
SrvTransportSocketGetServerAddress(
    IN PSRV_SOCKET              pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT SOCKLEN_T*              pAddressLength
    )
{
    SrvSocketGetServerAddress(pSocket, ppAddress, pAddressLength);
}

PCSTR
SrvTransportSocketGetAddressString(
    IN PSRV_SOCKET pSocket
    )
{
    return SrvSocketGetAddressString(pSocket);
}

int
SrvTransportSocketGetFileDescriptor(
    IN PSRV_SOCKET pSocket
    )
{
    return SrvSocketGetFileDescriptor(pSocket);
}

NTSTATUS
SrvTransportSocketSetBuffer(
    IN PSRV_SOCKET pSocket,
    IN PVOID pBuffer,
    IN ULONG Size,
    IN ULONG Minimum
    )
{
    return SrvSocketSetBuffer(pSocket, pBuffer, Size, Minimum);
}

NTSTATUS
SrvTransportSocketReceiveZct(
    IN PSRV_SOCKET pSocket,
    IN PLW_ZCT_VECTOR pZct
    )
{
    return SrvSocketReceiveZct(pSocket, pZct);
}

NTSTATUS
SrvTransportSocketSendReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PVOID pBuffer,
    IN ULONG Size
    )
{
    return SrvSocketSendReply(pSocket, pSendContext, pBuffer, Size);
}

NTSTATUS
SrvTransportSocketSendZctReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PLW_ZCT_VECTOR pZct
    )
{
    return SrvSocketSendZctReply(pSocket, pSendContext, pZct);
}

VOID
SrvTransportSocketDisconnect(
    IN PSRV_SOCKET pSocket
    )
{
    return SrvSocketDisconnect(pSocket);
}

VOID
SrvTransportSocketClose(
    IN OUT PSRV_SOCKET pSocket
    )
{
    SrvSocketClose(pSocket);
}
