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

// nttransact.c

NTSTATUS
SrvProcessNtTransact(
    PSRV_EXEC_CONTEXT pExecContext
    );

// openx.c

NTSTATUS
SrvProcessOpenAndX(
    PSRV_EXEC_CONTEXT pExecContext
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
