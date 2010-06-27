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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        trans2dfs.c
 *
 * Abstract:
 *
 *        Likewise SMB/CIFS Server driver  (SRV)
 *
 *        TRANS2_GET_DFS_REFERRAL handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "includes.h"


static
NTSTATUS
SrvUnmarshallGetDfsReferralParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT*         ppusMaxDfsReferralLevel,
    PWSTR*           ppwszFilename
    );

NTSTATUS
SrvProcessTrans2GetDfsReferral(
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

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pTrans2State->stage)
    {
        case SRV_TRANS2_STAGE_SMB_V1_INITIAL:

            ntStatus = SrvUnmarshallGetDfsReferralParams(
                            pTrans2State->pParameters,
                            pTrans2State->pRequestHeader->parameterCount,
                            &pTrans2State->pMaxDfsReferralLevel,
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

            ntStatus = STATUS_NO_SUCH_DEVICE;
            BAIL_ON_NT_STATUS(ntStatus);

            break;

        default:

            break;
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvUnmarshallGetDfsReferralParams(
    PBYTE            pParams,
    USHORT           ulBytesAvailable,
    PUSHORT*         ppusMaxDfsReferralLevel,
    PWSTR*           ppwszFilename
    )
{
    NTSTATUS ntStatus = 0;
    PUSHORT  pusMaxDfsReferralLevel = NULL;
    PWSTR    pwszFilename = NULL;
    PBYTE    pDataCursor = pParams;

    // Info level
    if (ulBytesAvailable < sizeof(USHORT))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pusMaxDfsReferralLevel = (PUSHORT)pDataCursor;
    pDataCursor += sizeof(USHORT);
    ulBytesAvailable -= sizeof(USHORT);

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

    *ppusMaxDfsReferralLevel = pusMaxDfsReferralLevel;
    *ppwszFilename = pwszFilename;

cleanup:

    return ntStatus;

error:

    *ppusMaxDfsReferralLevel = NULL;
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
