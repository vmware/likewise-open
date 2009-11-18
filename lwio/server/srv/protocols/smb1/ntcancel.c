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
 *        ntcancel.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV1
 *
 *        Cancel
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

NTSTATUS
SrvProcessNTCancel(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;
    BOOLEAN                    bInLock      = FALSE;
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    ULONG64                    ullNotifyId  = 0LL;
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState      = NULL;

    ntStatus = SrvConnectionFindSession_SMB_V1(
                            pCtxSmb1,
                            pConnection,
                            pSmbRequest->pHeader->uid,
                            &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree_SMB_V1(
                    pCtxSmb1,
                    pSession,
                    pSmbRequest->pHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ullNotifyId = SrvNotifyGetId(
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pSmbRequest->pHeader->mid);

    ntStatus = SrvTreeFindAsyncState(pTree, ullNotifyId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pNotifyState = (PSRV_CHANGE_NOTIFY_STATE_SMB_V1)pAsyncState->hAsyncState;

    LWIO_LOCK_MUTEX(bInLock, &pNotifyState->mutex);

    if (pNotifyState->pAcb && pNotifyState->pAcb->AsyncCancelContext)
    {
        IoCancelAsyncCancelContext(pNotifyState->pAcb->AsyncCancelContext);
    }

cleanup:

    if (pNotifyState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pNotifyState->mutex);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    return ntStatus;

error:

    LWIO_LOG_ERROR("NT CANCEL failed on mid %d [status code: 0x%x]",
                    pSmbRequest->pHeader->mid,
                    ntStatus);

    ntStatus = STATUS_SUCCESS;

    goto cleanup;
}
