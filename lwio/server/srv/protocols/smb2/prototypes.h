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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __PROTOTYPES_H__
#define __PROTOTYPES_H__


// close.c

NTSTATUS
SrvProcessClose_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );

// create.c

NTSTATUS
SrvProcessCreate_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );

// echo.c

NTSTATUS
SrvProcessEcho_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );

// flush.c

NTSTATUS
SrvProcessFlush_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );

// getinfo.c
NTSTATUS
SrvProcessGetInfo_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );

// logoff.c

NTSTATUS
SrvProcessLogoff_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );

// session_setup.c

NTSTATUS
SrvProcessSessionSetup_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );


// error.c

NTSTATUS
SrvBuildErrorResponse_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB2_HEADER         pSmbRequestHeader,
    NTSTATUS             errorStatus,
    PSMB_PACKET*         ppSmbResponse
    );

// tree_connect.c

NTSTATUS
SrvProcessTreeConnect_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );

// tree_disconnect.c

NTSTATUS
SrvProcessTreeDisconnect_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );

// wire.c

NTSTATUS
SMB2MarshalHeader(
    PSMB_PACKET pSmbPacket,
    USHORT      usCommand,
    USHORT      usEpoch,
    USHORT      usCredits,
    ULONG       ulPid,
    ULONG64     ullMid,
    ULONG       ulTid,
    ULONG64     ullSessionId,
    NTSTATUS    status,
    BOOLEAN     bCommandAllowsSignature,
    BOOLEAN     bIsResponse
    );

NTSTATUS
SMB2UnmarshallSessionSetup(
    PSMB_PACKET                         pPacket,
    PSMB2_SESSION_SETUP_REQUEST_HEADER* ppHeader,
    PBYTE*                              ppSecurityBlob,
    PULONG                              pulSecurityBlobLen
    );

NTSTATUS
SMB2MarshalSessionSetup(
    PSMB_PACKET        pPacket,
    SMB2_SESSION_FLAGS usFlags,
    PBYTE              pSecurityBlob,
    ULONG              ulSecurityBlobLen
    );

NTSTATUS
SMB2UnmarshalLogoffRequest(
    PSMB_PACKET                  pPacket,
    PSMB2_LOGOFF_REQUEST_HEADER* ppHeader
    );

NTSTATUS
SMB2MarshalLogoffResponse(
    PSMB_PACKET pSmbResponse
    );

NTSTATUS
SMB2UnmarshalTreeConnect(
    PSMB_PACKET                        pSmbRequest,
    PSMB2_TREE_CONNECT_REQUEST_HEADER* ppTreeConnectRequestHeader,
    PUNICODE_STRING                    pwszPath
    );

NTSTATUS
SMB2MarshalTreeConnectResponse(
    PSMB_PACKET          pPacket,
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_TREE_2     pTree
    );

NTSTATUS
SMB2UnmarshalTreeDisconnectRequest(
    PSMB_PACKET                           pSmbRequest,
    PSMB2_TREE_DISCONNECT_REQUEST_HEADER* ppTreeDisconnectHeader
    );

NTSTATUS
SMB2MarshalTreeDisconnectResponse(
    PSMB_PACKET      pSmbResponse
    );

NTSTATUS
SMB2UnmarshalCreateRequest(
    PSMB_PACKET                  pPacket,
    PSMB2_CREATE_REQUEST_HEADER* ppCreateRequestHeader,
    PUNICODE_STRING              pwszFileName
    );

NTSTATUS
SMB2UnmarshalCloseRequest(
   PSMB_PACKET pPacket,
   PSMB2_FID*  ppFid
   );

NTSTATUS
SMB2UnmarshalFlushRequest(
   PSMB_PACKET pPacket,
   PSMB2_FID*  ppFid
   );

NTSTATUS
SMB2MarshalFlushResponse(
    PSMB_PACKET pPacket
    );

NTSTATUS
SMB2UnmarshalEchoRequest(
   PSMB_PACKET                 pPacket,
   PSMB2_ECHO_REQUEST_HEADER*  ppHeader
   );

NTSTATUS
SMB2MarshalEchoResponse(
    PSMB_PACKET pPacket
    );

NTSTATUS
SMB2UnmarshalGetInfoRequest(
    PSMB_PACKET                    pPacket,
    PSMB2_GET_INFO_REQUEST_HEADER* ppHeader
    );

NTSTATUS
SMB2MarshalError(
    PSMB_PACKET pPacket,
    NTSTATUS    status
    );

NTSTATUS
SMB2MarshalFooter(
    PSMB_PACKET pPacket
    );

#endif /* __PROTOTYPES_H__ */

