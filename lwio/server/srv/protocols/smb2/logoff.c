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
 *        logoff.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Logoff
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildLogoffResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessLogoff_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PLWIO_SRV_SESSION_2        pSession      = NULL;

    SRV_LOG_DEBUG(
            pExecContext->pLogContext,
            SMB_PROTOCOL_VERSION_2,
            pSmbRequest->pHeader->command,
            "Logoff request params: "
            "command(%u),uid(%llu),cmd-seq(%llu),pid(%u),tid(%u),"
            "credits(%u),flags(0x%x),chain-offset(%u)",
            pSmbRequest->pHeader->command,
            (long long)pSmbRequest->pHeader->ullSessionId,
            (long long)pSmbRequest->pHeader->ullCommandSequence,
            pSmbRequest->pHeader->ulPid,
            pSmbRequest->pHeader->ulTid,
            pSmbRequest->pHeader->usCredits,
            pSmbRequest->pHeader->ulFlags,
            pSmbRequest->pHeader->ulChainOffset);

    ntStatus = SrvConnection2FindSession_SMB_V2(
                    pCtxSmb2,
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSetStatSession2Info(pExecContext, pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvSession2Rundown(pSession);

    ntStatus = SrvBuildLogoffResponse_SMB_V2(pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pCtxSmb2->pSession)
    {
        SrvSession2Release(pCtxSmb2->pSession);
        pCtxSmb2->pSession = NULL;
    }

cleanup:

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildLogoffResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset    = 0;
    ULONG ulBytesUsed = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SrvCreditorAdjustCredits(
                    pExecContext->pConnection->pCreditor,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pExecContext->ullAsyncId,
                    pSmbRequest->pHeader->usCredits,
                    &pExecContext->usCreditsGranted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_LOGOFF,
                    pSmbRequest->pHeader->usEpoch,
                    pExecContext->usCreditsGranted,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pSmbRequest->pHeader->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    0LL, /* Async Id */
                    STATUS_SUCCESS,
                    TRUE,
                    LwIsSetFlag(
                        pSmbRequest->pHeader->ulFlags,
                        SMB2_FLAGS_RELATED_OPERATION),
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    ntStatus = SMB2MarshalLogoffResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer += ulBytesUsed;
    // ulOffset += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader      = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}
