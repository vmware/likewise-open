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

#include "includes.h"

static
NTSTATUS
SrvUnmarshallSetFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    );

NTSTATUS
SrvProcessTrans2SetFileInformation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pTrans2State->stage)
    {
        case SRV_TRANS2_STAGE_SMB_V1_INITIAL:

            ntStatus = SrvUnmarshallSetFileInfoParams(
                            pTrans2State->pParameters,
                            pTrans2State->pRequestHeader->parameterCount,
                            &pTrans2State->usFid,
                            &pTrans2State->pSmbInfoLevel);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvConnectionFindSession_SMB_V1(
                            pCtxSmb1,
                            pConnection,
                            pSmbRequest->pHeader->uid,
                            &pTrans2State->pSession);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSessionFindTree_SMB_V1(
                            pCtxSmb1,
                            pTrans2State->pSession,
                            pSmbRequest->pHeader->tid,
                            &pTrans2State->pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pTrans2State->pTree,
                            pTrans2State->usFid,
                            &pTrans2State->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_ATTEMPT_IO:

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_IO_COMPLETE;

            ntStatus = SrvSetFileInfo(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_IO_COMPLETE:

            ntStatus = pTrans2State->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildSetInfoResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_DONE:

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvUnmarshallSetFileInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT          pusFid,
    PSMB_INFO_LEVEL* ppSmbInfoLevel
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL;
    USHORT   usFid = 0;
    PBYTE    pDataCursor = pParams;

    // FID
    if (ulBytesAvailable < sizeof(USHORT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    usFid = *((PUSHORT)pDataCursor);
    pDataCursor += sizeof(USHORT);
    ulBytesAvailable -= sizeof(USHORT);

    // Info level
    if (ulBytesAvailable < sizeof(SMB_INFO_LEVEL))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbInfoLevel = (PSMB_INFO_LEVEL)pDataCursor;
    pDataCursor += sizeof(SMB_INFO_LEVEL);
    ulBytesAvailable -= sizeof(SMB_INFO_LEVEL);

    *pusFid = usFid;
    *ppSmbInfoLevel = pSmbInfoLevel;

cleanup:

    return ntStatus;

error:

    *pusFid = 0;
    *ppSmbInfoLevel = NULL;

    goto cleanup;
}

