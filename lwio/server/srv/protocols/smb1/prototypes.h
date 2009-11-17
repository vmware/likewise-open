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

// checkdir.c

NTSTATUS
SrvProcessCheckDirectory(
    PSRV_EXEC_CONTEXT pExecContext
    );

// close.c

NTSTATUS
SrvProcessCloseAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

// config.c

ULONG
SrvConfigGetOplockTimeout_SMB_V1(
    VOID
    );

// connection.c

NTSTATUS
SrvConnectionFindSession_SMB_V1(
    PSRV_EXEC_CONTEXT_SMB_V1 pSmb1Context,
    PLWIO_SRV_CONNECTION     pConnection,
    USHORT                   usUid,
    PLWIO_SRV_SESSION*       ppSession
    );

// createdir.c

NTSTATUS
SrvProcessCreateDirectory(
    PSRV_EXEC_CONTEXT pExecContext
    );

// createx.c

NTSTATUS
SrvProcessNTCreateAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

// deldir.c

NTSTATUS
SrvProcessDeleteDirectory(
    PSRV_EXEC_CONTEXT pExecContext
    );

// delete.c

NTSTATUS
SrvProcessDelete(
    PSRV_EXEC_CONTEXT pExecContext
    );

// echo.c

NTSTATUS
SrvProcessEchoAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

// finder.c

NTSTATUS
SrvFinderCreateRepository(
    PHANDLE phFinderRepository
    );

NTSTATUS
SrvFinderCreateSearchSpace(
    PIO_CREATE_SECURITY_CONTEXT pIoSecurityContext,
    HANDLE         hFinderRepository,
    PWSTR          pwszFilesystemPath,
    PWSTR          pwszSearchPattern,
    USHORT         usSearchAttrs,
    ULONG          ulSearchStorageType,
    SMB_INFO_LEVEL infoLevel,
    BOOLEAN        bUseLongFilenames,
    PHANDLE        phFinder,
    PUSHORT        pusSearchId
    );

NTSTATUS
SrvFinderGetSearchSpace(
    HANDLE  hFinderRepository,
    USHORT  usSearchId,
    PHANDLE phFinder
    );

NTSTATUS
SrvFinderGetSearchResults(
    HANDLE   hSearchSpace,
    BOOLEAN  bReturnSingleEntry,
    BOOLEAN  bRestartScan,
    USHORT   usDesiredSearchCount,
    USHORT   usMaxDataCount,
    USHORT   usDataOffset,
    PBYTE*   ppData,
    PUSHORT  pusDataLen,
    PUSHORT  pusSearchResultCount,
    PBOOLEAN pbEndOfSearch
    );

VOID
SrvFinderReleaseSearchSpace(
    HANDLE hFinder
    );

NTSTATUS
SrvFinderCloseSearchSpace(
    HANDLE hFinderRepository,
    USHORT usSearchId
    );

VOID
SrvFinderCloseRepository(
    HANDLE hFinderRepository
    );

// findfirst2.c

NTSTATUS
SrvProcessTrans2FindFirst2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// findnext2.c

NTSTATUS
SrvProcessTrans2FindNext2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// findclose2.c

NTSTATUS
SrvProcessFindClose2(
    PSRV_EXEC_CONTEXT pExecContext
    );

// flush.c

NTSTATUS
SrvProcessFlush(
    PSRV_EXEC_CONTEXT pExecContext
    );

// libmain.c

NTSTATUS
SrvBuildExecContext_SMB_V1(
    PLWIO_SRV_CONNECTION      pConnection,
    PSMB_PACKET               pSmbRequest,
    PSRV_EXEC_CONTEXT_SMB_V1* ppSmb1Context
    );;

NTSTATUS
SrvBuildErrorResponse_SMB_V1(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_HEADER          pRequestHeader,
    NTSTATUS             errorStatus,
    PSRV_MESSAGE_SMB_V1  pSmbResponse
    );

// lockx.c

NTSTATUS
SrvProcessLockAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

// logoff.c

NTSTATUS
SrvProcessLogoffAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

// notify.c

NTSTATUS
SrvNotifyRepositoryInit(
    VOID
    );

NTSTATUS
SrvNotifyCreateRepository(
    USHORT                             usTid,
    PSRV_TREE_NOTIFY_STATE_REPOSITORY* ppRepository
    );

NTSTATUS
SrvNotifyRepositoryCreateState(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pRepository,
    PLWIO_SRV_CONNECTION              pConnection,
    PLWIO_SRV_SESSION                 pSession,
    PLWIO_SRV_TREE                    pTree,
    PLWIO_SRV_FILE                    pFile,
    USHORT                            usMid,
    ULONG                             ulPid,
    ULONG                             ulRequestSequence,
    ULONG                             ulCompletionFilter,
    BOOLEAN                           bWatchTree,
    ULONG                             ulMaxBufferSize,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1*  ppNotifyState
    );

NTSTATUS
SrvNotifyRepositoryFindState(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pRepository,
    ULONG                             ulPid,
    USHORT                            usMid,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1*  ppNotifyState
    );

NTSTATUS
SrvNotifyRepositoryRemoveState(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pRepository,
    ULONG                             ulPid,
    USHORT                            usMid
    );

VOID
SrvNotifyRepositoryRelease(
    PSRV_TREE_NOTIFY_STATE_REPOSITORY pNotifyRepository
    );

NTSTATUS
SrvNotifyRemoveRepository(
    USHORT usTid
    );

VOID
SrvNotifyRepositoryShutdown(
    VOID
    );

NTSTATUS
SrvInitNotifyStateCollection(
    VOID
    );

NTSTATUS
SrvShutdownNotifyStateCollection(
    VOID
    );

VOID
SrvPrepareNotifyStateAsync(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    );

VOID
SrvReleaseNotifyStateAsync(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    );

VOID
SrvNotifyStateRelease(
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    );

// ntcancel.c

NTSTATUS
SrvProcessNTCancel(
    PSRV_EXEC_CONTEXT pExecContext
    );

// ntrename.c

NTSTATUS
SrvProcessNtRename(
    PSRV_EXEC_CONTEXT pExecContext
    );

// nttransact.c

NTSTATUS
SrvProcessNtTransact(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildChangeNotifyResponse(
    PSRV_EXEC_CONTEXT               pExecContext,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    );

// openx.c

NTSTATUS
SrvProcessOpenAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

// oplock.c

NTSTATUS
SrvProcessOplock(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvAcknowledgeOplockBreak(
    PSRV_EXEC_CONTEXT_SMB_V1 pCtxSmb1,
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState,
    BOOLEAN bFileIsClosed
    );

NTSTATUS
SrvBuildOplockState(
    PLWIO_SRV_CONNECTION      pConnection,
    PLWIO_SRV_SESSION         pSession,
    PLWIO_SRV_TREE            pTree,
    PLWIO_SRV_FILE            pFile,
    PSRV_OPLOCK_STATE_SMB_V1* ppOplockState
    );

VOID
SrvReleaseOplockStateHandle(
    HANDLE hOplockState
    );

VOID
SrvReleaseOplockState(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    );

VOID
SrvPrepareOplockStateAsync(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    );

VOID
SrvReleaseOplockStateAsync(
    PSRV_OPLOCK_STATE_SMB_V1 pOplockState
    );

// pipeinfo.c

NTSTATUS
SrvMarshallPipeInfo(
    PFILE_PIPE_INFORMATION       pPipeInfo,
    PFILE_PIPE_LOCAL_INFORMATION pPipeLocalInfo,
    PUSHORT                      pusDeviceState
    );

// read.c

NTSTATUS
SrvProcessRead(
    PSRV_EXEC_CONTEXT pExecContext
    );

// readx.c

NTSTATUS
SrvProcessReadAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

// rename.c

NTSTATUS
SrvProcessRename(
    PSRV_EXEC_CONTEXT pExecContext
    );

// session.c

NTSTATUS
SrvSessionFindTree_SMB_V1(
    PSRV_EXEC_CONTEXT_SMB_V1 pSmb1Context,
    PLWIO_SRV_SESSION        pSession,
    USHORT                   usTid,
    PLWIO_SRV_TREE*          ppTree
    );

// sessionsetup.c

NTSTATUS
SrvProcessSessionSetup(
    PSRV_EXEC_CONTEXT pExecContext
    );

// trans.c

NTSTATUS
SrvProcessTransaction(
    PSRV_EXEC_CONTEXT pExecContext
    );

// trans2.c

NTSTATUS
SrvProcessTransaction2(
    PSRV_EXEC_CONTEXT pExecContext
    );

VOID
SrvPrepareTrans2StateAsync(
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State,
    PSRV_EXEC_CONTEXT        pExecContext
    );

VOID
SrvReleaseTrans2StateAsync(
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State
    );

// trans2qfi.h

NTSTATUS
SrvProcessTrans2QueryFileInformation(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvQueryFileBasicInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildQueryFileBasicInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvQueryFileStandardInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildQueryFileStandardInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvQueryFileEAInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildQueryFileEAInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvQueryFileStreamInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildQueryFileStreamInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvQueryFileAllInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildQueryFileAllInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvQueryFileNameInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildQueryFileNameInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvQueryFileAltNameInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvBuildQueryFileAltNameInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

// trans2qfsi.h

NTSTATUS
SrvProcessTrans2QueryFilesystemInformation(
    PSRV_EXEC_CONTEXT       pExecContext
    );

// trans2qpi.h

NTSTATUS
SrvProcessTrans2QueryPathInformation(
    PSRV_EXEC_CONTEXT pExecContext
    );

// trans2sfi.h

NTSTATUS
SrvProcessTrans2SetFileInformation(
    PSRV_EXEC_CONTEXT pExecContext
    );

// tree.c

NTSTATUS
SrvTreeFindFile_SMB_V1(
    PSRV_EXEC_CONTEXT_SMB_V1 pSmb1Context,
    PLWIO_SRV_TREE           pTree,
    USHORT                   usFid,
    PLWIO_SRV_FILE*          ppFile
    );

// treedisconnect.c

NTSTATUS
SrvProcessTreeDisconnectAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

// treeconnect.c

NTSTATUS
SrvProcessTreeConnectAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

// wire.c

NTSTATUS
SrvInitPacket_SMB_V1(
    PSMB_PACKET pSmbPacket,
    BOOLEAN     bAllowSignature     /* in     */
    );

NTSTATUS
SrvMarshalHeader_SMB_V1(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    UCHAR         ucCommand,
    ULONG         ulError,
    BOOLEAN       bIsResponse,
    USHORT        usTid,
    ULONG         ulPid,
    USHORT        usUid,
    USHORT        usMid,
    BOOLEAN       bCommandAllowsSignature,
    PSMB_HEADER*  ppHeader,
    PBYTE*        ppWordCount,
    PANDX_HEADER* ppAndXHeader,
    PUSHORT       pusBytesUsed
    );

NTSTATUS
SrvUnmarshalHeader_SMB_V1(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    PSMB_HEADER*  ppHeader,
    PANDX_HEADER* ppAndXHeader,
    PUSHORT       pusBytesUsed
    );

NTSTATUS
SrvMarshalHeaderAndX_SMB_V1(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    UCHAR         ucCommand,
    PBYTE*        ppWordCount,
    PANDX_HEADER* ppAndXHeader,
    PUSHORT       pusBytesUsed
    );

NTSTATUS
SrvUnmarshalHeaderAndX_SMB_V1(
    PBYTE         pBuffer,
    ULONG         ulOffset,
    ULONG         ulBytesAvailable,
    UCHAR         ucCommand,
    PBYTE*        ppWordCount,
    PANDX_HEADER* ppAndXHeader,
    PUSHORT       pusBytesUsed
    );

NTSTATUS
WireUnmarshallOplockRequest(
    PBYTE              pBuffer,
    ULONG              ulBytesAvailable,
    ULONG              ulOffset,
    PLW_OPLOCK_HEADER* ppRequestHeader
    );

NTSTATUS
SrvVerifyAndXCommandSequence(
    UCHAR ucLeaderCommand,
    UCHAR ucFollowerCommand
    );

// write.c

NTSTATUS
SrvProcessWrite(
    PSRV_EXEC_CONTEXT pExecContext
    );

// writex.c

NTSTATUS
SrvProcessWriteAndX(
    PSRV_EXEC_CONTEXT pExecContext
    );

#endif /* __PROTOTYPES_H__ */
