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

// wire.c

NTSTATUS
SMB2MarshalHeader(
    PSMB_PACKET pSmbPacket,
    USHORT      usCommand,
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
SMB2MarshalFooter(
    PSMB_PACKET pPacket
    );

#endif /* __PROTOTYPES_H__ */

