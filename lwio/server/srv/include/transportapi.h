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

///
/// Transport indication that there is a new connection.
///
/// This is called when a new connection arrives.  It should return
/// STATUS_ACCESS_DENIED or any other error to reject the connection.
///
/// If the connection is accepted, #PFN_SRV_TRANSPORT_CONNECTION_DONE
/// will be called unless the socket is explicitly closed with
/// SrvTransportSocketClose().
///
typedef NTSTATUS (*PFN_SRV_TRANSPORT_CONNECTION_NEW)(
    OUT PSRV_CONNECTION* ppConnection,
    IN PSRV_PROTOCOL_TRANSPORT_CONTEXT pProtocolDispatchContext,
    IN PSRV_SOCKET pSocket
    );

///
/// Transport indication that there is data available.
///
/// This is called when at least the minimum requested number of bytes are
/// available in the buffer specified via SrvTransportSocketSetBuffer().
///
/// This call is always asynchronous.
///
typedef NTSTATUS (*PFN_SRV_TRANSPORT_CONNECTION_DATA)(
    IN PSRV_CONNECTION pConnection,
    IN ULONG BytesAvailable
    );

///
/// Transport indication that the connection is done.
///
/// This is called if #PFN_SRV_TRANSPORT_CONNECTION_NEW succeeded
/// and SrvTransportSocketClose() was not called to indicate
/// that the transport is done with the socket.
///
/// Note that any #PFN_SRV_TRANSPORT_SEND_DONE will happen
/// before this is called.
///
/// This call is always asynchronous.
///
/// The PTD cannot call back into the driver while and after processing
/// this.
///
typedef VOID (*PFN_SRV_TRANSPORT_CONNECTION_DONE)(
    IN PSRV_CONNECTION pConnection,
    IN NTSTATUS Status
    );

///
/// Transport indication that the send reply is queued.
///
/// This is called from within SrvTransportSocketSendReply() and
/// SrvTransportSocketSendZctReply() so that the PTD can perform signing,
/// etc. in the same order that the replies are queued.  If this returns
/// an error, SrvTransportSocketSendReply()/SrvTransportSocketSendZctReply()
/// will also return an error.
///
typedef NTSTATUS (*PFN_SRV_TRANSPORT_SEND_PREPARE)(
    IN PSRV_SEND_CONTEXT pSendContext
    );

///
/// Transport indication that the send reply is done.
///
/// This is called with a status code indicating whether or not
/// sending the reply was successful.  This called IFF
/// SrvTransportSocketSendReply()/SrvTransportSocketSendZctReply()
/// returns STATUS_PENDING and SrvTransportSocketClose() has not been called.
///
/// This can be called synchronously from inside the send reply
/// functions or asynchronously.
///
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
    IN PSRV_SOCKET              pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT SOCKLEN_T*              pAddressLength
    );

VOID
SrvTransportSocketGetServerAddress(
    IN PSRV_SOCKET              pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT SOCKLEN_T*              pAddressLength
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

///
/// Set the socket buffer to use to receive data.
///
/// This will set the buffer into which to receive data.  If a NULL
/// or zero-size buffer is specified, no more data will be accepted.
/// Normally, this function can only be called from within
/// #PFN_SRV_TRANSPORT_CONNECTION_DATA.  However, if the buffer is
/// already NULL or zero, this can be called from outside to resume
/// receiving data.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
/// The caller must ensure that no SrvTransportSocketReceiveZct()
/// call overlaps calling into this function w/a non-zero buffer.
///
NTSTATUS
SrvTransportSocketSetBuffer(
    IN PSRV_SOCKET pSocket,
    IN PVOID pBuffer,
    IN ULONG Size,
    IN ULONG Minimum
    );

///
/// Receive data using a ZCT vector.
///
/// This will receive data into a ZCT vector which must have > 0 bytes
/// remaining. Will call #PFN_SRV_TRANSPORT_CONNECTION_DATA or
/// #PFN_SRV_TRANSPORT_CONNECTION_DONE asynchronously IFF
/// retrning STATUS_PENDING.
///
/// This function cal only be called if there is no pending call
/// to either SrvTransportSocketReceiveBuffer() or
/// SrvTransportSocketReceiveZct() for this socket.
///
/// The caller must ensure that the socket buffer is NULL before calling
/// this function (see SrvTransportSocketSetBuffer()).
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
NTSTATUS
SrvTransportSocketReceiveZct(
    IN PSRV_SOCKET pSocket,
    IN PLW_ZCT_VECTOR pZct
    );

///
/// Send a reply using a bufer.
///
/// This will send a reply, queueing it as needed.  Once the reply is queued,
/// tt will synchronously call #PFN_SRV_TRANSPORT_SEND_PREPARE.  Will call
/// #PFN_SRV_TRANSPORT_SEND_DONE asynchronously IFF returning STATUS_PENDING.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
/// @retval STATUS_SUCCESS
/// @retval STATUS_PENDING
/// @retval !NT_SUCCESS
///
NTSTATUS
SrvTransportSocketSendReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PVOID pBuffer,
    IN ULONG Size
    );

///
/// Send a reply using a ZCT.
///
/// This will send a reply, queueing it as needed.  Once the reply is queued,
/// tt will synchronously call #PFN_SRV_TRANSPORT_SEND_PREPARE.  Will call
/// #PFN_SRV_TRANSPORT_SEND_DONE asynchronously IFF returning STATUS_PENDING.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
/// @retval STATUS_SUCCESS
/// @retval STATUS_PENDING
/// @retval !NT_SUCCESS
///
NTSTATUS
SrvTransportSocketSendZctReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PLW_ZCT_VECTOR pZct
    );

///
/// Disconnect a socket.
///
/// This will disconnect a socket while keeping the memory referece valid.
/// It will asynchronously call #PFN_SRV_TRANSPORT_SEND_DONE and
/// #PFN_SRV_TRANSPORT_CONNECTION_DONE.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
VOID
SrvTransportSocketDisconnect(
    IN PSRV_SOCKET pSocket
    );

///
/// Close the socket.
///
/// This should be done when the PTD is completely done with the socket.
/// No #PFN_SRV_TRANSPORT_SEND_DONE or #PFN_SRV_TRANSPORT_CONNECTION_DONE
/// callbacks are triggered by this.  So the the PTD needs those callbacks,
/// it must call SrvTransportSocketDisconnect() and wait for the callbacks.
///
/// If called from outside of a callback, this cannot be called with
/// any locks help that can also be held by a callback.
///
VOID
SrvTransportSocketClose(
    IN OUT PSRV_SOCKET pSocket
    );

#endif /* __TRANSPORT_API_H__ */
