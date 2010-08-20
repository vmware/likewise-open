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
SrvBuildTrans2State(
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pBytecount,
    PUSHORT                     pSetup,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSRV_TRANS2_STATE_SMB_V1*   ppTrans2State
    );

static
VOID
SrvExecuteTrans2AsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseTrans2StateHandle(
    HANDLE hTransState
    );

static
VOID
SrvReleaseTrans2State(
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State
    );

static
VOID
SrvFreeTrans2State(
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State
    );

NTSTATUS
SrvProcessTransaction2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pTrans2State = (PSRV_TRANS2_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTrans2State)
    {
        InterlockedIncrement(&pTrans2State->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PTRANSACTION_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PUSHORT                     pBytecount     = NULL; // Do not free
        PUSHORT                     pSetup         = NULL; // Do not free
        PBYTE                       pParameters    = NULL; // Do not free
        PBYTE                       pData          = NULL; // Do not free

        ntStatus = WireUnmarshallTransactionRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pSetup,
                        &pBytecount,
                        NULL, /* No transaction name */
                        &pParameters,
                        &pData);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pSetup == NULL)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SrvBuildTrans2State(
                        pRequestHeader,
                        pBytecount,
                        pSetup,
                        pParameters,
                        pData,
                        &pTrans2State);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pTrans2State;
        InterlockedIncrement(&pTrans2State->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseTrans2StateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pTrans2State->mutex);

    if (pExecContext->pStatInfo)
    {
        ntStatus = SrvStatisticsSetSubOpcode(
                        pExecContext->pStatInfo,
                        *pTrans2State->pSetup);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    switch (*pTrans2State->pSetup)
    {
        case SMB_SUB_COMMAND_TRANS2_FIND_FIRST2 :

            ntStatus = SrvProcessTrans2FindFirst2(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS2_FIND_NEXT2 :

            ntStatus = SrvProcessTrans2FindNext2(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS2_QUERY_FS_INFORMATION :

            ntStatus = SrvProcessTrans2QueryFilesystemInformation(
                          pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS2_QUERY_PATH_INFORMATION :

            ntStatus = SrvProcessTrans2QueryPathInformation(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS2_SET_PATH_INFORMATION :

            ntStatus = SrvProcessTrans2SetPathInformation(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS2_QUERY_FILE_INFORMATION :

            ntStatus = SrvProcessTrans2QueryFileInformation(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS2_SET_FILE_INFORMATION :

            ntStatus = SrvProcessTrans2SetFileInformation(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS2_GET_DFS_REFERRAL :

            ntStatus = SrvProcessTrans2GetDfsReferral(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS2_OPEN2 :
        case SMB_SUB_COMMAND_TRANS2_FSCTL :
        case SMB_SUB_COMMAND_TRANS2_IOCTL2 :
        case SMB_SUB_COMMAND_TRANS2_FIND_NOTIFY_FIRST :
        case SMB_SUB_COMMAND_TRANS2_FIND_NOTIFY_NEXT :
        case SMB_SUB_COMMAND_TRANS2_CREATE_DIRECTORY :
        case SMB_SUB_COMMAND_TRANS2_SESSION_SETUP :
        case SMB_SUB_COMMAND_TRANS2_REPORT_DFS_INCONSISTENCY :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pTrans2State)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pTrans2State->mutex);

        SrvReleaseTrans2State(pTrans2State);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed

            break;

        default:

            if (pTrans2State)
            {
                SrvReleaseTrans2StateAsync(pTrans2State);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildTrans2State(
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pBytecount,
    PUSHORT                     pSetup,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSRV_TRANS2_STATE_SMB_V1*   ppTrans2State
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_TRANS2_STATE_SMB_V1),
                    (PVOID*)&pTrans2State);
    BAIL_ON_NT_STATUS(ntStatus);

    pTrans2State->refCount = 1;

    pthread_mutex_init(&pTrans2State->mutex, NULL);
    pTrans2State->pMutex = &pTrans2State->mutex;

    pTrans2State->stage = SRV_TRANS2_STAGE_SMB_V1_INITIAL;

    pTrans2State->pRequestHeader = pRequestHeader;
    pTrans2State->pBytecount     = pBytecount;
    pTrans2State->pSetup         = pSetup;
    pTrans2State->pParameters    = pParameters;
    pTrans2State->pData          = pData;

    *ppTrans2State = pTrans2State;

cleanup:

    return ntStatus;

error:

    *ppTrans2State = NULL;

    if (pTrans2State)
    {
        SrvFreeTrans2State(pTrans2State);
    }

    goto cleanup;
}

VOID
SrvPrepareTrans2StateAsync(
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State,
    PSRV_EXEC_CONTEXT        pExecContext
    )
{
    pTrans2State->acb.Callback        = &SrvExecuteTrans2AsyncCB;

    pTrans2State->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pTrans2State->acb.AsyncCancelContext = NULL;

    pTrans2State->pAcb = &pTrans2State->acb;
}

static
VOID
SrvExecuteTrans2AsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_TRANS2_STATE_SMB_V1   pTrans2State     = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pTrans2State =
        (PSRV_TRANS2_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pTrans2State->mutex);

    if (pTrans2State->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pTrans2State->pAcb->AsyncCancelContext);
    }

    pTrans2State->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pTrans2State->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V1.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

VOID
SrvReleaseTrans2StateAsync(
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State
    )
{
    if (pTrans2State->pAcb)
    {
        pTrans2State->acb.Callback = NULL;

        if (pTrans2State->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pTrans2State->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pTrans2State->pAcb->CallbackContext = NULL;
        }

        if (pTrans2State->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pTrans2State->pAcb->AsyncCancelContext);
        }

        pTrans2State->pAcb = NULL;
    }
}

static
VOID
SrvReleaseTrans2StateHandle(
    HANDLE hTransState
    )
{
    SrvReleaseTrans2State((PSRV_TRANS2_STATE_SMB_V1)hTransState);
}

static
VOID
SrvReleaseTrans2State(
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State
    )
{
    if (InterlockedDecrement(&pTrans2State->refCount) == 0)
    {
        SrvFreeTrans2State(pTrans2State);
    }
}

static
VOID
SrvFreeTrans2State(
    PSRV_TRANS2_STATE_SMB_V1 pTrans2State
    )
{
    if (pTrans2State->pAcb && pTrans2State->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pTrans2State->pAcb->AsyncCancelContext);
    }

    if (pTrans2State->pEcpList)
    {
        IoRtlEcpListFree(&pTrans2State->pEcpList);
    }

    if (pTrans2State->pFile)
    {
        SrvFileRelease(pTrans2State->pFile);
    }

    if (pTrans2State->pRootDir)
    {
        SrvFileRelease(pTrans2State->pRootDir);
    }

    if (pTrans2State->pTree)
    {
        SrvTreeRelease(pTrans2State->pTree);
    }

    if (pTrans2State->pSession)
    {
        SrvSessionRelease(pTrans2State->pSession);
    }

    if (pTrans2State->hFile)
    {
        IoCloseFile(pTrans2State->hFile);
    }

    if (pTrans2State->fileName.FileName)
    {
        SrvFreeMemory(pTrans2State->fileName.FileName);
    }

    if (pTrans2State->hDir)
    {
        IoCloseFile(pTrans2State->hDir);
    }

    if (pTrans2State->dirPath.FileName)
    {
        SrvFreeMemory(pTrans2State->dirPath.FileName);
    }

    if (pTrans2State->pData2)
    {
        SrvFreeMemory(pTrans2State->pData2);
    }

    if (pTrans2State->pMutex)
    {
        pthread_mutex_destroy(&pTrans2State->mutex);
    }

    SrvFreeMemory(pTrans2State);
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
