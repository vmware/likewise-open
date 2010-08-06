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

// cancel.c

NTSTATUS
SrvProcessCancel_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

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

// config.c

NTSTATUS
SrvConfigSetupInitial_SMB_V2(
    VOID
    );

NTSTATUS
SrvConfigRefresh_SMB_V2(
    VOID
    );

ULONG
SrvConfigGetOplockTimeout_SMB_V2(
    VOID
    );

VOID
SrvConfigShutdown_SMB_V2(
    VOID
    );

// create.c

NTSTATUS
SrvProcessCreate_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

VOID
SrvCancelCreate_SMB_V2(
    PLWIO_ASYNC_STATE pAsyncState
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

// getfileinfo.c

NTSTATUS
SrvGetFileInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildFileInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// getfsinfo.c

NTSTATUS
SrvGetFileSystemInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildFileSystemInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// getinfo.c

NTSTATUS
SrvProcessGetInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

VOID
SrvPrepareGetInfoStateAsync_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState,
    PSRV_EXEC_CONTEXT          pExecContext
    );

VOID
SrvReleaseGetInfoStateAsync_SMB_V2(
    PSRV_GET_INFO_STATE_SMB_V2 pGetInfoState
    );

// getsecinfo.c

NTSTATUS
SrvGetSecurityInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildSecurityInfoResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// ioctl.c

NTSTATUS
SrvProcessIOCTL_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

// libmain.c

NTSTATUS
SrvBuildInterimResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext,
    ULONG64           ullAsyncId
    );

NTSTATUS
SrvBuildExecContext_SMB_V2(
    PLWIO_SRV_CONNECTION      pConnection,
    PSMB_PACKET               pSmbRequest,
    PSRV_EXEC_CONTEXT_SMB_V2* ppSmb2Context
    );

PCSTR
SrvGetCommandDescription_SMB_V2(
    ULONG ulCommand
    );

// logging.c

VOID
SrvLogRequest_SMB_V2(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    );

// lock.c

NTSTATUS
SrvProcessLock_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

VOID
SrvCancelLockRequest_SMB_V2(
    PLWIO_ASYNC_STATE pAsyncState
    );

VOID
SrvCancelAsyncLockState_SMB_V2(
    HANDLE hLockState
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

// notify_request.c

NTSTATUS
SrvProcessNotify_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessNotifyCompletion_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

VOID
SrvCancelChangeNotify_SMB_V2(
    PLWIO_ASYNC_STATE pAsyncState
    );

// notify_state.c

NTSTATUS
SrvNotifyCreateState_SMB_V2(
    ULONG64                   ullAsyncId,
    PLWIO_SRV_CONNECTION      pConnection,
    PLWIO_SRV_SESSION_2       pSession,
    PLWIO_SRV_TREE_2          pTree,
    PLWIO_SRV_FILE_2          pFile,
    USHORT                    usEpoch,
    ULONG64                   ullCommandSequence,
    ULONG                     ulPid,
    ULONG                     ulCompletionFilter,
    BOOLEAN                   bWatchTree,
    ULONG                     ulMaxBufferSize,
    PSRV_NOTIFY_STATE_SMB_V2* ppNotifyState
    );

VOID
SrvPrepareNotifyStateAsync_SMB_V2(
    PSRV_NOTIFY_STATE_SMB_V2 pNotifyState
    );

VOID
SrvReleaseNotifyStateAsync_SMB_V2(
    PSRV_NOTIFY_STATE_SMB_V2 pNotifyState
    );

PSRV_NOTIFY_STATE_SMB_V2
SrvNotifyStateAcquire_SMB_V2(
    PSRV_NOTIFY_STATE_SMB_V2 pNotifyState
    );

VOID
SrvNotifyStateReleaseHandle_SMB_V2(
    HANDLE hNotifyState
    );

VOID
SrvNotifyStateRelease_SMB_V2(
    PSRV_NOTIFY_STATE_SMB_V2 pNotifyState
    );

// oplock.c

NTSTATUS
SrvBuildOplockState_SMB_V2(
    PLWIO_SRV_CONNECTION      pConnection,
    PLWIO_SRV_SESSION_2       pSession,
    PLWIO_SRV_TREE_2          pTree,
    PLWIO_SRV_FILE_2          pFile,
    PSRV_OPLOCK_STATE_SMB_V2* ppOplockState
    );

NTSTATUS
SrvProcessOplock_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessOplockBreak_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvAcknowledgeOplockBreak_SMB_V2(
    PSRV_OPLOCK_STATE_SMB_V2 pOplockState,
    PUCHAR  pucNewOplockLevel,
    BOOLEAN bFileIsClosed
    );

VOID
SrvReleaseOplockStateHandle_SMB_V2(
    HANDLE hOplockState
    );

VOID
SrvReleaseOplockState_SMB_V2(
    PSRV_OPLOCK_STATE_SMB_V2 pOplockState
    );

VOID
SrvPrepareOplockStateAsync_SMB_V2(
    PSRV_OPLOCK_STATE_SMB_V2 pOplockState
    );

VOID
SrvReleaseOplockStateAsync_SMB_V2(
    PSRV_OPLOCK_STATE_SMB_V2 pOplockState
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
SrvSetErrorMessage_SMB_V2(
    PSRV_EXEC_CONTEXT_SMB_V2 pSmb2Context,
    PBYTE                    pErrorMessage,
    ULONG                    ulErrorMessageLength
    );

VOID
SrvFreeErrorMessage_SMB_V2(
    PSRV_EXEC_CONTEXT_SMB_V2 pSmb2Context
    );

NTSTATUS
SrvBuildErrorResponse_SMB_V2(
    PSRV_EXEC_CONTEXT    pExecContext,
    ULONG64              ullAsyncId,
    NTSTATUS             errorStatus
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

// utils.c

NTSTATUS
SrvBuildTreeRelativePath_SMB_V2(
    PLWIO_SRV_TREE_2 pTree,
    PWSTR            pwszFilename,
    PIO_FILE_NAME    pFilename
    );

NTSTATUS
SrvSetStatSession2Info(
    PSRV_EXEC_CONTEXT   pExecContext,
    PLWIO_SRV_SESSION_2 pSession
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
    ULONG64       ullAsyncId,
    NTSTATUS      status,
    BOOLEAN       bIsResponse,
    BOOLEAN       bIsPartOfCompoundMessage,
    PSMB2_HEADER* ppSMB2Header,
    PULONG        pulBytesUsed
    );

NTSTATUS
SMB2GetAsyncId(
    PSMB2_HEADER pHeader,
    PULONG64     pUllAsyncId
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
    PBYTE                               pBuffer,
    ULONG                               ulOffset,
    ULONG                               ulBytesAvailable,
    PLWIO_SRV_CONNECTION                pConnection,
    PLWIO_SRV_TREE_2                    pTree,
    PSMB2_TREE_CONNECT_RESPONSE_HEADER* ppResponseHeader,
    PULONG                              pulBytesUsed
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
SMB2MarshalCreateContext(
    IN OUT PBYTE                 pBuffer,
    IN     ULONG                 ulOffset,
    IN     PBYTE                 pName,
    IN     USHORT                usNameSize,
    IN     PBYTE                 pData,
    IN     ULONG                 ulDataSize,
    IN     ULONG                 ulBytesAvailable,
    IN OUT PULONG                pulBytesUsed,
    IN OUT PSMB2_CREATE_CONTEXT* ppCreateContext
    );

NTSTATUS
SMB2UnmarshalCloseRequest(
   IN     PSRV_MESSAGE_SMB_V2         pSmbRequest,
   IN OUT PSMB2_CLOSE_REQUEST_HEADER* ppHeader
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
SMB2UnmarshalOplockBreakRequest(
    IN     PSRV_MESSAGE_SMB_V2        pSmbRequest,
    IN OUT PSMB2_OPLOCK_BREAK_HEADER* ppRequestHeader
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
SMB2UnmarshalNotifyRequest(
    IN     PSRV_MESSAGE_SMB_V2         pSmbRequest,
    IN OUT PSMB2_NOTIFY_CHANGE_HEADER* ppNotifyRequestHeader
    );

NTSTATUS
SMB2MarshalNotifyResponse(
    IN OUT PBYTE                         pBuffer,
    IN     ULONG                         ulOffset,
    IN     ULONG                         ulBytesAvailable,
    IN OUT PBYTE                         pData,
    IN     ULONG                         ulDataLength,
    IN OUT PULONG                        pulDataOffset,
    IN OUT PSMB2_NOTIFY_RESPONSE_HEADER* ppHeader,
    IN OUT PULONG                        pulBytesUsed
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

VOID
SMB2UnmarshallBoolean(
    PBOOLEAN pbValue
    );

// write.c

NTSTATUS
SrvProcessWrite_SMB_V2(
    PSRV_EXEC_CONTEXT pContext
    );

#endif /* __PROTOTYPES_H__ */

