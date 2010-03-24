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
    PSRV_TRANSPORT_HANDLE_DATA pTransport = NULL;

    ntStatus = SrvAllocateMemory(sizeof(*pTransport), OUT_PPVOID(&pTransport));
    BAIL_ON_NT_STATUS(ntStatus);

    pTransport->Dispatch = *pProtocolDispatch;
    pTransport->pContext = pProtocolDispatchContext;

    ntStatus = SrvListenerInit(&pTransport->Listener, pTransport);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

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
        SrvFreeMemory(hTransport);
    }
}

VOID
SrvTransportSocketGetAddress(
    IN PSRV_SOCKET pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT size_t* pAddressLength
    )
{
    SrvSocketGetAddress(pSocket, ppAddress, pAddressLength);
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
