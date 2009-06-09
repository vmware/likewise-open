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
 *        srvtransport.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Transport
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __TRANSPORT_API_H__
#define __TRANSPORT_API_H__

typedef struct LWIO_SRV_CONNECTION *PLWIO_SRV_CONNECTION;
typedef struct LWIO_SRV_SOCKET     *PLWIO_SRV_SOCKET;

NTSTATUS
SrvTransportInit(
    VOID
    );

HANDLE
SrvTransportGetAllocator(
    IN PLWIO_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionCreate(
    PLWIO_SRV_SOCKET           pSocket,
    HANDLE                    hPacketAllocator,
    HANDLE                    hGssContext,
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    PSRV_PROPERTIES           pServerProperties,
    PSRV_HOST_INFO            pHostinfo,
    PLWIO_SRV_CONNECTION*      ppConnection
    );

NTSTATUS
SrvConnectionCreateSession(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION* ppSession
    );

NTSTATUS
SrvConnectionFindSession(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT uid,
    PLWIO_SRV_SESSION* ppSession
    );

NTSTATUS
SrvConnectionGetNamedPipeClientAddress(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

NTSTATUS
SrvConnectionGetNamedPipeSessionKey(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

LWIO_SRV_CONN_STATE
SrvConnectionGetState(
    PLWIO_SRV_CONNECTION pConnection
    );

BOOLEAN
SrvConnectionIsInvalid(
    PLWIO_SRV_CONNECTION pConnection
    );

VOID
SrvConnectionRelease(
    PLWIO_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionRemoveSession(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT              uid
    );

VOID
SrvConnectionSetInvalid(
    PLWIO_SRV_CONNECTION pConnection
    );

VOID
SrvConnectionSetState(
    PLWIO_SRV_CONNECTION pConnection,
    LWIO_SRV_CONN_STATE  connState
    );

NTSTATUS
SrvTransportGetRequest(
    OUT PLWIO_SRV_CONNECTION* ppConnection,
    OUT PSMB_PACKET*          ppRequest
    );

NTSTATUS
SrvTransportSendResponse(
    IN          PLWIO_SRV_CONNECTION pConnection,
    IN OPTIONAL PSMB_PACKET          pRequest,
    IN          PSMB_PACKET          pResponse
    );

NTSTATUS
SrvTransportShutdown(
    VOID
    );

#endif /* __TRANSPORT_API_H__ */
