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
    IN OUT PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// connection.c

NTSTATUS
SrvConnection2FindSession_SMB_V2(
    PSMB2_CONTEXT        pContext,
    PLWIO_SRV_CONNECTION pConnection,
    ULONG64              ullUid,
    PLWIO_SRV_SESSION_2* ppSession
    );

// context.c

NTSTATUS
SrvInitContextContents_SMB_V2(
    IN     PLWIO_SRV_CONNECTION pConnection,
    IN OUT PSMB2_CONTEXT        pContext
    );

VOID
SrvFreeContextContents_SMB_V2(
    IN  PSMB2_CONTEXT pContext
    );

// create.c

NTSTATUS
SrvProcessCreate_SMB_V2(
    IN OUT PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// echo.c

NTSTATUS
SrvProcessEcho_SMB_V2(
    IN     PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// find.c

NTSTATUS
SrvProcessFind_SMB_V2(
    IN     PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// flush.c

NTSTATUS
SrvProcessFlush_SMB_V2(
    IN     PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// getinfo.c
NTSTATUS
SrvProcessGetInfo_SMB_V2(
    IN     PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// ioctl.c

NTSTATUS
SrvProcessIOCTL_SMB_V2(
    IN     PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// lock.c

NTSTATUS
SrvProcessLock_SMB_V2(
    IN     PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// logoff.c

NTSTATUS
SrvProcessLogoff_SMB_V2(
    IN OUT PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// negotiate.c

NTSTATUS
SrvProcessNegotiate_SMB_V2(
    IN OUT PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// session.c

NTSTATUS
SrvSession2FindTree_SMB_V2(
    PSMB2_CONTEXT       pContext,
    PLWIO_SRV_SESSION_2 pSession,
    ULONG               ulTid,
    PLWIO_SRV_TREE_2*   ppTree
    );

// session_setup.c

NTSTATUS
SrvProcessSessionSetup_SMB_V2(
    IN OUT PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// error.c

NTSTATUS
SrvBuildErrorResponse_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB2_HEADER         pSmbRequestHeader,
    NTSTATUS             errorStatus,
    PBYTE                pMessage,
    ULONG                ulMessageLength,
    PSMB_PACKET          pSmbResponse
    );

// read.c

NTSTATUS
SrvProcessRead_SMB_V2(
    IN     PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// tree.c

NTSTATUS
SrvTree2FindFile_SMB_V2(
    PSMB2_CONTEXT     pContext,
    PLWIO_SRV_TREE_2  pTree,
    PSMB2_FID         pFid,
    PLWIO_SRV_FILE_2* ppFile
    );

// tree_connect.c

NTSTATUS
SrvProcessTreeConnect_SMB_V2(
    IN OUT PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// tree_disconnect.c

NTSTATUS
SrvProcessTreeDisconnect_SMB_V2(
    IN OUT PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

// wire.c

NTSTATUS
SMB2InitPacket(
    IN OUT PSMB_PACKET pSmbPacket,
    IN     BOOLEAN     bAllowSignature
    );

NTSTATUS
SMB2MarshalHeader(
    IN OUT          PBYTE         pBuffer,
    IN              ULONG         ulOffset,
    IN              ULONG         ulBytesAvailable,
    IN              USHORT        usCommand,
    IN              USHORT        usEpoch,
    IN              USHORT        usCredits,
    IN              ULONG         ulPid,
    IN              ULONG64       ullMid,
    IN              ULONG         ulTid,
    IN              ULONG64       ullSessionId,
    IN              NTSTATUS      status,
    IN              BOOLEAN       bIsResponse,
    IN OUT OPTIONAL PSMB2_HEADER* ppSMB2Header,
    IN OUT          PULONG        pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalNegotiateRequest(
    PSMB2_MESSAGE                   pRequest,
    PSMB2_NEGOTIATE_REQUEST_HEADER* ppHeader,
    PUSHORT*                        ppusDialects
    );

NTSTATUS
SMB2UnmarshallSessionSetup(
    IN     PSMB2_MESSAGE                       pRequest,
    IN OUT PSMB2_SESSION_SETUP_REQUEST_HEADER* ppHeader,
    IN OUT PBYTE*                              ppSecurityBlob,
    IN OUT PULONG                              pulSecurityBlobLen
    );

NTSTATUS
SMB2MarshalSessionSetup(
    IN OUT PBYTE              pBuffer,
    IN     ULONG              ulOffset,
    IN     ULONG              ulBytesAvailable,
    IN     SMB2_SESSION_FLAGS usFlags,
    IN     PBYTE              pSecurityBlob,
    IN     ULONG              ulSecurityBlobLen,
    IN OUT PULONG             pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalLogoffRequest(
    IN     PSMB2_MESSAGE                pRequest,
    IN OUT PSMB2_LOGOFF_REQUEST_HEADER* ppHeader
    );

NTSTATUS
SMB2MarshalLogoffResponse(
    IN OUT PBYTE  pBuffer,
    IN     ULONG  ulOffset,
    IN     ULONG  ulBytesAvailable,
    IN OUT PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalTreeConnect(
    IN     PSMB2_MESSAGE                      pRequest,
    IN OUT PSMB2_TREE_CONNECT_REQUEST_HEADER* ppHeader,
    IN OUT PUNICODE_STRING                    pwszPath
    );

NTSTATUS
SMB2MarshalTreeConnectResponse(
    IN OUT PBYTE                pBuffer,
    IN     ULONG                ulOffset,
    IN     ULONG                ulBytesAvailable,
    IN     PLWIO_SRV_CONNECTION pConnection,
    IN     PLWIO_SRV_TREE_2     pTree,
    IN OUT PULONG               pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalTreeDisconnectRequest(
    IN  PSMB2_MESSAGE                         pSmbRequest,
    OUT PSMB2_TREE_DISCONNECT_REQUEST_HEADER* ppTreeDisconnectHeader
    );

NTSTATUS
SMB2MarshalTreeDisconnectResponse(
    IN OUT PBYTE                pBuffer,
    IN     ULONG                ulOffset,
    IN     ULONG                ulBytesAvailable,
    IN OUT PULONG               pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalCreateRequest(
    IN     PSMB2_MESSAGE                pSmbRequest,
    IN OUT PSMB2_CREATE_REQUEST_HEADER* ppCreateRequestHeader,
    IN OUT PUNICODE_STRING              pwszFileName,
    OUT    PSRV_CREATE_CONTEXT*         ppCreateContexts,
    IN OUT PULONG                       pulNumContexts
    );

NTSTATUS
SMB2UnmarshalCloseRequest(
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB2_FID*  ppFid
    );

NTSTATUS
SMB2UnmarshalFlushRequest(
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB2_FID*    ppFid
    );

NTSTATUS
SMB2MarshalFlushResponse(
    IN OUT PBYTE                pBuffer,
    IN     ULONG                ulOffset,
    IN     ULONG                ulBytesAvailable,
    IN OUT PULONG               pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalEchoRequest(
   IN     PSMB2_MESSAGE               pSmbRequest,
   IN OUT PSMB2_ECHO_REQUEST_HEADER*  ppHeader
   );

NTSTATUS
SMB2MarshalEchoResponse(
    IN OUT PBYTE  pBuffer,
    IN     ULONG  ulOffset,
    IN     ULONG  ulBytesAvailable,
    IN OUT PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalGetInfoRequest(
    IN     PSMB2_MESSAGE                  pSmbRequest,
    IN OUT PSMB2_GET_INFO_REQUEST_HEADER* ppHeader
    );

NTSTATUS
SMB2UnmarshalReadRequest(
    IN     PSMB2_MESSAGE              pSmbRequest,
    IN OUT PSMB2_READ_REQUEST_HEADER* ppRequestHeader
    );

NTSTATUS
SMB2MarshalReadResponse(
    IN OUT PBYTE  pBuffer,
    IN     ULONG  ulOffset,
    IN     ULONG  ulBytesAvailable,
    IN     PBYTE  pData,
    IN     ULONG  ulBytesRead,
    IN     ULONG  ulBytesRemaining,
    IN OUT PULONG pulDataOffset,
    IN OUT PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalWriteRequest(
    IN     PSMB2_MESSAGE               pSmbRequest,
    IN OUT PSMB2_WRITE_REQUEST_HEADER* ppRequestHeader,
    IN OUT PBYTE*                      ppData
    );

NTSTATUS
SMB2MarshalWriteResponse(
    IN OUT PBYTE  pBuffer,
    IN     ULONG  ulOffset,
    IN     ULONG  ulBytesAvailable,
    IN     ULONG  ulBytesWritten,
    IN     ULONG  ulBytesRemaining,
    IN OUT PULONG pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalLockRequest(
    IN     PSMB2_MESSAGE              pSmbRequest,
    IN OUT PSMB2_LOCK_REQUEST_HEADER* ppRequestHeader
    );

NTSTATUS
SMB2MarshalLockResponse(
    IN OUT PBYTE                  pBuffer,
    IN     ULONG                  ulOffset,
    IN     ULONG                  ulBytesAvailable,
    IN     PSRV_SMB2_LOCK_REQUEST pLockRequest,
    IN OUT PULONG                 pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalIOCTLRequest(
    IN     PSMB2_MESSAGE               pSmbRequest,
    IN OUT PSMB2_IOCTL_REQUEST_HEADER* ppRequestHeader,
    IN OUT PBYTE*                      ppData
    );

NTSTATUS
SMB2MarshalIOCTLResponse(
    IN OUT PBYTE                      pBuffer,
    IN     ULONG                      ulOffset,
    IN     ULONG                      ulBytesAvailable,
    IN     PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    IN     PBYTE                      pOutBuffer,
    IN     ULONG                      ulOutLength,
    IN OUT PULONG                     pulBytesUsed
    );

NTSTATUS
SMB2UnmarshalFindRequest(
    IN     PSMB2_MESSAGE              pSmbRequest,
    IN OUT PSMB2_FIND_REQUEST_HEADER* ppRequestHeader,
    IN OUT PUNICODE_STRING            pwszFilename
    );

NTSTATUS
SMB2MarshalError(
    IN OUT PBYTE    pBuffer,
    IN     ULONG    ulOffset,
    IN     ULONG    ulBytesAvailable,
    IN     PBYTE    pMessage,
    IN     ULONG    ulMessageLength,
    IN OUT PULONG   pulBytesUsed
    );

NTSTATUS
SMB2MarshalFooter(
    PSMB_PACKET pPacket
    );

// write.c

NTSTATUS
SrvProcessWrite_SMB_V2(
    IN     PSMB2_CONTEXT pContext,
    IN     PSMB2_MESSAGE pSmbRequest,
    IN OUT PSMB_PACKET   pSmbResponse
    );

#endif /* __PROTOTYPES_H__ */

