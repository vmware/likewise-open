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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise I/O Subsystem
 *
 *        SRV Threadpool Transport
 *
 *        Prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

// listener.c

NTSTATUS
SrvListenerInit(
    OUT PSRV_TRANSPORT_LISTENER pListener,
    IN SRV_TRANSPORT_HANDLE pTransport
    );

VOID
SrvListenerShutdown(
    IN OUT PSRV_TRANSPORT_LISTENER pListener
    );

// srvsocket.c

PCSTR
SrvSocketAddressToString(
    IN struct sockaddr* pSocketAddress,
    OUT PSTR pszAddress,
    IN ULONG AddressLength
    );

NTSTATUS
SrvSocketCreate(
    IN PSRV_TRANSPORT_LISTENER pListener,
    IN int fd,
    IN struct sockaddr* pClientAddress,
    IN SOCKLEN_T ClientAddressLength,
    OUT PSRV_SOCKET* ppSocket
    );

VOID
SrvSocketGetAddress(
    IN PSRV_SOCKET pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT size_t* pAddressLength
    );

PCSTR
SrvSocketGetAddressString(
    IN PSRV_SOCKET pSocket
    );

int
SrvSocketGetFileDescriptor(
    IN PSRV_SOCKET pSocket
    );

NTSTATUS
SrvSocketSetBuffer(
    IN PSRV_SOCKET pSocket,
    IN PVOID pBuffer,
    IN ULONG Size,
    IN ULONG Minimum
    );

NTSTATUS
SrvSocketSendReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PVOID pBuffer,
    IN ULONG Size
    );

NTSTATUS
SrvSocketSendZctReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PLW_ZCT_VECTOR pZct
    );

VOID
SrvSocketDisconnect(
    IN PSRV_SOCKET pSocket
    );

VOID
SrvSocketClose(
    IN OUT PSRV_SOCKET pSocket
    );
