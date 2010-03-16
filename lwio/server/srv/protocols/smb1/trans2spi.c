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
SrvUnmarshallSetPathInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PSMB_INFO_LEVEL* ppSmbInfoLevel,
    PWSTR*           ppwszFilename
    );

NTSTATUS
SrvProcessTrans2SetPathInformation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    BOOLEAN                    bTreeInLock  = FALSE;
    ACCESS_MASK                accessMask   = 0;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pTrans2State->stage)
    {
        case SRV_TRANS2_STAGE_SMB_V1_INITIAL:

            ntStatus = SrvUnmarshallSetPathInfoParams(
                            pTrans2State->pParameters,
                            pTrans2State->pRequestHeader->parameterCount,
                            &pTrans2State->pSmbInfoLevel,
                            &pTrans2State->pwszFilename);
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

            LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pTrans2State->pTree->mutex);

            ntStatus = SrvBuildFilePath(
                            pTrans2State->pTree->pShareInfo->pwszPath,
                            pTrans2State->pwszFilename,
                            &pTrans2State->fileName.FileName);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTrans2State->pTree->mutex);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_CREATE_FILE_COMPLETED;

            SrvPrepareTrans2StateAsync(pTrans2State, pExecContext);

            accessMask = SrvGetPathAccessMask(pExecContext);

            ntStatus = SrvIoCreateFile(
                            pTrans2State->pTree->pShareInfo,
                            &pTrans2State->hFile,
                            pTrans2State->pAcb,
                            &pTrans2State->ioStatusBlock,
                            pTrans2State->pSession->pIoSecurityContext,
                            &pTrans2State->fileName,
                            pTrans2State->pSecurityDescriptor,
                            pTrans2State->pSecurityQOS,
                            accessMask,
                            0,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                            FILE_OPEN,
                            0,
                            NULL, /* EA Buffer */
                            0,    /* EA Length */
                            &pTrans2State->pEcpList
                            );
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseTrans2StateAsync(pTrans2State); // completed synchronously

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_CREATE_FILE_COMPLETED:

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_TRANS2_STAGE_SMB_V1_ATTEMPT_IO:

            ntStatus = pTrans2State->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSetPathInfo(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_IO_COMPLETE;

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

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTrans2State->pTree->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallSetPathInfoParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PSMB_INFO_LEVEL* ppSmbInfoLevel,
    PWSTR*           ppwszFilename
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_INFO_LEVEL pSmbInfoLevel = NULL;
    PWSTR    pwszFilename = NULL;
    PBYTE    pDataCursor = pParams;

    // Info level
    if (ulBytesAvailable < sizeof(SMB_INFO_LEVEL))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSmbInfoLevel = (PSMB_INFO_LEVEL)pDataCursor;
    pDataCursor += sizeof(SMB_INFO_LEVEL);
    ulBytesAvailable -= sizeof(SMB_INFO_LEVEL);

    // Reserved field
    if (ulBytesAvailable < sizeof(ULONG))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pDataCursor += sizeof(ULONG);
    ulBytesAvailable -= sizeof(ULONG);

    // Filename
    if (ulBytesAvailable < sizeof(wchar16_t))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pwszFilename = (PWSTR)pDataCursor;

    *ppSmbInfoLevel = pSmbInfoLevel;
    *ppwszFilename = pwszFilename;

cleanup:

    return ntStatus;

error:

    *ppSmbInfoLevel = NULL;
    *ppwszFilename = NULL;

    goto cleanup;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
