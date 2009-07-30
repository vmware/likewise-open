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
 *        SMB V1 Protocol Handler API
 *
 *        prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __PROTOTYPES_H__
#define __PROTOTYPES_H__

VOID
SrvExecuteAsyncRequest_SMB_V1(
    PVOID pData
    );

VOID
SrvReleaseAsyncRequest_SMB_V1(
    PVOID pData
    );

NTSTATUS
SrvProcessCheckDirectory(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessCloseAndX(
    IN  PLWIO_SRV_CONNECTION pConneciton,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessCreateDirectory(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessNTCreateAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

VOID
SrvExecuteCreateRequest(
    PSRV_SMB_CREATE_REQUEST pCreateRequest
    );

VOID
SrvReleaseCreateRequest(
    PSRV_SMB_CREATE_REQUEST pCreateRequest
    );

NTSTATUS
SrvProcessDeleteDirectory(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessDelete(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessEchoAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessTrans2FindFirst2(
    IN  PLWIO_SRV_CONNECTION        pConnection,
    IN  PSMB_PACKET                 pSmbRequest,
    IN  PTRANSACTION_REQUEST_HEADER pRequestHeader,
    IN  PUSHORT                     pSetup,
    IN  PUSHORT                     pByteCount,
    IN  PBYTE                       pParameters,
    IN  PBYTE                       pData,
    OUT PSMB_PACKET*                ppSmbResponse
    );

NTSTATUS
SrvProcessTrans2FindNext2(
    IN  PLWIO_SRV_CONNECTION         pConnection,
    IN  PSMB_PACKET                 pSmbRequest,
    IN  PTRANSACTION_REQUEST_HEADER pRequestHeader,
    IN  PUSHORT                     pSetup,
    IN  PUSHORT                     pByteCount,
    IN  PBYTE                       pParameters,
    IN  PBYTE                       pData,
    OUT PSMB_PACKET*                ppSmbResponse
    );

NTSTATUS
SrvProcessFindClose2(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessFlush(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessLockAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

VOID
SrvReleaseLockRequest(
    PSRV_SMB_LOCK_REQUEST pLockRequest
    );

NTSTATUS
SrvProcessLogoffAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessNtTransact(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessOpenAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessRead(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessReadAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessRename(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessSessionSetup(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessTransaction(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessTransaction2(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessTreeConnectAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessTreeDisconnectAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessWrite(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessWriteAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    );

#endif /* __PROTOTYPES_H__ */
