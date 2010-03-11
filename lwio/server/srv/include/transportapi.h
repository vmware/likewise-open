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
 *        transportapi.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Transport
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#ifndef __TRANSPORT_API_H__
#define __TRANSPORT_API_H__

#include <sys/socket.h>
#include "lwthreads.h"
#include "lwzct.h"

// Provided by Transport layer -- Opaque to Protocol layer:
typedef struct _SRV_TRANSPORT_HANDLE_DATA *SRV_TRANSPORT_HANDLE, **PSRV_TRANSPORT_HANDLE;
typedef struct _SRV_SOCKET *PSRV_SOCKET;

// Provided by Protocol layer's Protocol Transport Driver -- opaque to Transport layer:
// TODO: Perhaps rename PSRV_PROTOCOL_TRANSPORT_CONTEXT to PSRV_PTD_CONTEXT for Protocol Transport Driver.
//       If so, perhaps use _PTD_ for these three.  Plus perhaps change any "_TRANSPORT_PROTOCOL_" stuff to "_PTD_".
typedef struct _SRV_PROTOCOL_TRANSPORT_CONTEXT *PSRV_PROTOCOL_TRANSPORT_CONTEXT;
typedef struct _SRV_CONNECTION *PSRV_CONNECTION;
typedef struct _SRV_SEND_CONTEXT *PSRV_SEND_CONTEXT;

//
// Callbacks for protocol driver layer
//

// Called when a new connection arrives.  Should return
// STATUS_ACCESS_DENIED to reject the connection.
typedef NTSTATUS (*PFN_SRV_TRANSPORT_CONNECTION_NEW)(
    OUT PSRV_CONNECTION* ppConnection,
    IN PSRV_PROTOCOL_TRANSPORT_CONTEXT pProtocolDispatchContext,
    IN PSRV_SOCKET pSocket
    );

typedef NTSTATUS (*PFN_SRV_TRANSPORT_CONNECTION_DATA)(
    IN PSRV_CONNECTION pConnection,
    IN ULONG BytesAvailable
    );

// This will always be called if NEW succeeded.
typedef VOID (*PFN_SRV_TRANSPORT_CONNECTION_DONE)(
    IN PSRV_CONNECTION pConnection,
    IN NTSTATUS Status
    );

// If SEND_PREPARE fails, SEND_DONE is *NOT* called.
typedef NTSTATUS (*PFN_SRV_TRANSPORT_SEND_PREPARE)(
    IN PSRV_SEND_CONTEXT pSendContext
    );

// Only called if SEND_PRAPARE succeeds.
typedef VOID (*PFN_SRV_TRANSPORT_SEND_DONE)(
    IN PSRV_SEND_CONTEXT pSendContext,
    IN NTSTATUS Status
    );

typedef struct _SRV_TRANSPORT_PROTOCOL_DISPATCH {
    PFN_SRV_TRANSPORT_CONNECTION_NEW pfnConnectionNew;
    PFN_SRV_TRANSPORT_CONNECTION_DATA pfnConnectionData;
    PFN_SRV_TRANSPORT_CONNECTION_DONE pfnConnectionDone;
    PFN_SRV_TRANSPORT_SEND_PREPARE pfnSendPrepare;
    PFN_SRV_TRANSPORT_SEND_DONE pfnSendDone;
} SRV_TRANSPORT_PROTOCOL_DISPATCH, *PSRV_TRANSPORT_PROTOCOL_DISPATCH;

NTSTATUS
SrvTransportInit(
    OUT PSRV_TRANSPORT_HANDLE phTransport,
    IN PSRV_TRANSPORT_PROTOCOL_DISPATCH pProtocolDispatch,
    IN OPTIONAL PSRV_PROTOCOL_TRANSPORT_CONTEXT pProtocolDispatchContext
    );

VOID
SrvTransportShutdown(
    IN OUT SRV_TRANSPORT_HANDLE hTransport
    );

VOID
SrvTransportSocketGetAddress(
    IN PSRV_SOCKET pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT size_t* pAddressLength
    );

PCSTR
SrvTransportSocketGetAddressString(
    IN PSRV_SOCKET pSocket
    );

// For logging only.
int
SrvTransportSocketGetFileDescriptor(
    IN PSRV_SOCKET pSocket
    );

NTSTATUS
SrvTransportSocketSetBuffer(
    IN PSRV_SOCKET pSocket,
    IN PVOID pBuffer,
    IN ULONG Size,
    IN ULONG Minimum
    );

NTSTATUS
SrvTransportSocketSendReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PVOID pBuffer,
    IN ULONG Size
    );

NTSTATUS
SrvTransportSocketSendZctReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PLW_ZCT_VECTOR pZct
    );

///
/// Close Transport's socket.
///
/// This can only be called if CONNECTION_DONE has not yet
/// been called.
///
VOID
SrvTransportSocketClose(
    IN OUT PSRV_SOCKET pSocket
    );

#endif /* __TRANSPORT_API_H__ */
