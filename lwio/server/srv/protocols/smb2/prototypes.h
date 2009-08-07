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
    PSRV_EXEC_CONTEXT pExecContext
    );

// connection.c

NTSTATUS
SrvConnection2FindSession_SMB_V2(
    PSRV_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PLWIO_SRV_CONNECTION     pConnection,
    ULONG64                  ullUid,
    PLWIO_SRV_SESSION_2*     ppSession
    );

// create.c

NTSTATUS
SrvProcessCreate_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// echo.c

NTSTATUS
SrvProcessEcho_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// find.c

NTSTATUS
SrvProcessFind_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// flush.c

NTSTATUS
SrvProcessFlush_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// getinfo.c
NTSTATUS
SrvProcessGetInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// ioctl.c

NTSTATUS
SrvProcessIOCTL_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

// lock.c

NTSTATUS
SrvProcessLock_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

// logoff.c

NTSTATUS
SrvProcessLogoff_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

// negotiate.c

NTSTATUS
SrvProcessNegotiate_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// session.c

NTSTATUS
SrvSession2FindTree_SMB_V2(
    PSRV_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PLWIO_SRV_SESSION_2      pSession,
    ULONG                    ulTid,
    PLWIO_SRV_TREE_2*        ppTree
    );

// session_setup.c

NTSTATUS
SrvProcessSessionSetup_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

// setinfo.c

NTSTATUS
SrvProcessSetInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

// error.c

NTSTATUS
SrvBuildErrorResponse_SMB_V2(
    PSRV_EXEC_CONTEXT    pExecContext,
    NTSTATUS             errorStatus,
    PBYTE                pMessage,
    ULONG                ulMessageLength
    );

// read.c

NTSTATUS
SrvProcessRead_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

// tree.c

NTSTATUS
SrvTree2FindFile_SMB_V2(
    PSRV_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PLWIO_SRV_TREE_2         pTree,
    PSMB2_FID                pFid,
    PLWIO_SRV_FILE_2*        ppFile
    );

// tree_connect.c

NTSTATUS
SrvProcessTreeConnect_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

// tree_disconnect.c

NTSTATUS
SrvProcessTreeDisconnect_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// wire.c

NTSTATUS
SMB2InitPacket(
    PSMB_PACKET pSmbPacket,
    BOOLEAN     bAllowSignature
    );

NTSTATUS
SrvUnmarshalHeader_SMB_V2(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    PSMB2_HEADER* ppHeader,
    PULONG        pulBytesUsed
    );

NTSTATUS
SMB2MarshalHeader(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    USHORT        usCommand,
    USHORT        usEpoch,
    USHORT        usCredits,
    ULONG         ulPid,
    ULONG64       ullMid,
    ULONG         ulTid,
    ULONG64       ullSessionId,
    NTSTATUS      status,
    BOOLEAN       bIsResponse,
    BOOLEAN       bIsPartOfCompoundMessage,
    PSMB2_HEADER* ppSMB2Header,
    PULONG        pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalNegotiateRequest(
    PSRV_MESSAGE_SMB_V2             pRequest,
    PSMB2_NEGOTIATE_REQUEST_HEADER* ppHeader,
    PUSHORT*                        ppusDialects
    );

NTSTATUS
SMB2UnmarshallSessionSetup(
    PSRV_MESSAGE_SMB_V2                 pRequest,
    PSMB2_SESSION_SETUP_REQUEST_HEADER* ppHeader,
    PBYTE*                              ppSecurityBlob,
    PULONG                              pulSecurityBlobLen
    );

NTSTATUS
SMB2MarshalSessionSetup(
    PBYTE              pBuffer,
    ULONG              ulOffset,
    ULONG              ulBytesAvailable,
    SMB2_SESSION_FLAGS usFlags,
    PBYTE              pSecurityBlob,
    ULONG              ulSecurityBlobLen,
    PULONG             pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalLogoffRequest(
    PSRV_MESSAGE_SMB_V2          pRequest,
    PSMB2_LOGOFF_REQUEST_HEADER* ppHeader
    );

NTSTATUS
SMB2MarshalLogoffResponse(
    PBYTE  pBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalTreeConnect(
    PSRV_MESSAGE_SMB_V2                pRequest,
    PSMB2_TREE_CONNECT_REQUEST_HEADER* ppHeader,
    PUNICODE_STRING                    pwszPath
    );

NTSTATUS
SMB2MarshalTreeConnectResponse(
    PBYTE                pBuffer,
    ULONG                ulOffset,
    ULONG                ulBytesAvailable,
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_TREE_2     pTree,
    PULONG               pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalTreeDisconnectRequest(
    PSRV_MESSAGE_SMB_V2                   pSmbRequest,
    PSMB2_TREE_DISCONNECT_REQUEST_HEADER* ppTreeDisconnectHeader
    );

NTSTATUS
SMB2MarshalTreeDisconnectResponse(
    PBYTE                pBuffer,
    ULONG                ulOffset,
    ULONG                ulBytesAvailable,
    PULONG               pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalCreateRequest(
    PSRV_MESSAGE_SMB_V2          pSmbRequest,
    PSMB2_CREATE_REQUEST_HEADER* ppCreateRequestHeader,
    PUNICODE_STRING              pwszFileName,
    PSRV_CREATE_CONTEXT*         ppCreateContexts,
    PULONG                       pulNumContexts
    );

NTSTATUS
SMB2UnmarshalCloseRequest(
    PSRV_MESSAGE_SMB_V2 pSmbRequest,
    PSMB2_FID*          ppFid
    );

NTSTATUS
SMB2UnmarshalFlushRequest(
    PSRV_MESSAGE_SMB_V2 pSmbRequest,
    PSMB2_FID*          ppFid
    );

NTSTATUS
SMB2MarshalFlushResponse(
    PBYTE                pBuffer,
    ULONG                ulOffset,
    ULONG                ulBytesAvailable,
    PULONG               pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalEchoRequest(
   PSRV_MESSAGE_SMB_V2         pSmbRequest,
   PSMB2_ECHO_REQUEST_HEADER*  ppHeader
   );

NTSTATUS
SMB2MarshalEchoResponse(
    PBYTE  pBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalGetInfoRequest(
    PSRV_MESSAGE_SMB_V2            pSmbRequest,
    PSMB2_GET_INFO_REQUEST_HEADER* ppHeader
    );

NTSTATUS
SMB2UnmarshalSetInfoRequest(
    PSRV_MESSAGE_SMB_V2            pSmbRequest,
    PSMB2_SET_INFO_REQUEST_HEADER* ppHeader,
    PBYTE*                         ppData
    );

NTSTATUS
SMB2UnmarshalReadRequest(
    PSRV_MESSAGE_SMB_V2        pSmbRequest,
    PSMB2_READ_REQUEST_HEADER* ppRequestHeader
    );

NTSTATUS
SMB2MarshalReadResponse(
    PBYTE  pBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    PBYTE  pData,
    ULONG  ulBytesRead,
    ULONG  ulBytesRemaining,
    PULONG pulDataOffset,
    PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalWriteRequest(
    PSRV_MESSAGE_SMB_V2         pSmbRequest,
    PSMB2_WRITE_REQUEST_HEADER* ppRequestHeader,
    PBYTE*                      ppData
    );

NTSTATUS
SMB2MarshalWriteResponse(
    PBYTE  pBuffer,
    ULONG  ulOffset,
    ULONG  ulBytesAvailable,
    ULONG  ulBytesWritten,
    ULONG  ulBytesRemaining,
    PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalLockRequest(
    PSRV_MESSAGE_SMB_V2        pSmbRequest,
    PSMB2_LOCK_REQUEST_HEADER* ppRequestHeader
    );

NTSTATUS
SMB2MarshalLockResponse(
    PBYTE                  pBuffer,
    ULONG                  ulOffset,
    ULONG                  ulBytesAvailable,
    PSRV_SMB2_LOCK_REQUEST pLockRequest,
    PULONG                 pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalIOCTLRequest(
    PSRV_MESSAGE_SMB_V2         pSmbRequest,
    PSMB2_IOCTL_REQUEST_HEADER* ppRequestHeader,
    PBYTE*                      ppData
    );

NTSTATUS
SMB2MarshalIOCTLResponse(
    PBYTE                      pBuffer,
    ULONG                      ulOffset,
    ULONG                      ulBytesAvailable,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pOutBuffer,
    ULONG                      ulOutLength,
    PULONG                     pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalFindRequest(
    PSRV_MESSAGE_SMB_V2        pSmbRequest,
    PSMB2_FIND_REQUEST_HEADER* ppRequestHeader,
    PUNICODE_STRING            pwszFilename
    );

NTSTATUS
SMB2MarshalFindResponse(
    PBYTE                       pBuffer,
    ULONG                       ulOffset,
    ULONG                       ulBytesAvailable,
    PBYTE                       pData,
    ULONG                       ulDataLength,
    PULONG                      pulDataOffset,
    PSMB2_FIND_RESPONSE_HEADER* ppHeader,
    PULONG                      pulBytesUsed
    );

NTSTATUS
SMB2MarshalError(
    PBYTE    pBuffer,
    ULONG    ulOffset,
    ULONG    ulBytesAvailable,
    PBYTE    pMessage,
    ULONG    ulMessageLength,
    PULONG   pulBytesUsed
    );

NTSTATUS
SMB2MarshalFooter(
    PSMB_PACKET pPacket
    );

// write.c

NTSTATUS
SrvProcessWrite_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

#endif /* __PROTOTYPES_H__ */

