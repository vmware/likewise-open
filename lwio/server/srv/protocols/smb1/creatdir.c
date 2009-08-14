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
SrvBuildCreatedirState(
    PSMB_CREATE_DIRECTORY_REQUEST_HEADER pRequestHeader,
    PWSTR                                pwszPathFragment,
    PSRV_CREATEDIR_STATE_SMB_V1*         ppCreatedirState
    );

static
NTSTATUS
SrvBuildCreateDirectoryResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareCreatedirStateAsync(
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState,
    PSRV_EXEC_CONTEXT          pExecContext
    );

static
VOID
SrvExecuteCreatedirAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseCreatedirStateAsync(
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState
    );

static
VOID
SrvReleaseCreatedirStateHandle(
    HANDLE hState
    );

static
VOID
SrvReleaseCreatedirState(
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState
    );

static
VOID
SrvFreeCreatedirState(
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState
    );

NTSTATUS
SrvProcessCreateDirectory(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus     = 0;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION           pSession     = NULL;
    PLWIO_SRV_TREE              pTree        = NULL;
    BOOLEAN                     bTreeInLock  = FALSE;
    BOOLEAN                     bInLock      = FALSE;
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState = NULL;

    pCreatedirState = (PSRV_CREATEDIR_STATE_SMB_V1)pCtxSmb1->hState;
    if (pCreatedirState)
    {
        InterlockedIncrement(&pCreatedirState->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PSMB_CREATE_DIRECTORY_REQUEST_HEADER pRequestHeader = NULL;//Do not free
        PWSTR pwszPathFragment = NULL; // Do not free

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

        ntStatus = WireUnmarshallCreateDirectoryRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pwszPathFragment);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pwszPathFragment || !*pwszPathFragment)
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SrvBuildCreatedirState(
                        pRequestHeader,
                        pwszPathFragment,
                        &pCreatedirState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pCreatedirState;
        InterlockedIncrement(&pCreatedirState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseCreatedirStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pCreatedirState->mutex);

    switch (pCreatedirState->stage)
    {
        case SRV_CREATEDIR_STAGE_SMB_V1_INITIAL:

            LWIO_LOCK_RWMUTEX_SHARED(   bTreeInLock,
                                        &pCtxSmb1->pTree->pShareInfo->mutex);

            ntStatus = SrvBuildFilePath(
                            pCtxSmb1->pTree->pShareInfo->pwszPath,
                            pCreatedirState->pwszPathFragment,
                            &pCreatedirState->fileName.FileName);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTree->pShareInfo->mutex);

            pCreatedirState->stage = SRV_CREATEDIR_STAGE_SMB_V1_COMPLETED;

            SrvPrepareCreatedirStateAsync(pCreatedirState, pExecContext);

            ntStatus = IoCreateFile(
                            &pCreatedirState->hFile,
                            pCreatedirState->pAcb,
                            &pCreatedirState->ioStatusBlock,
                            pCtxSmb1->pSession->pIoSecurityContext,
                            &pCreatedirState->fileName,
                            pCreatedirState->pSecurityDescriptor,
                            pCreatedirState->pSecurityQOS,
                            GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,
                            0, /* allocation size */
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_CREATE,
                            FILE_DIRECTORY_FILE,
                            NULL, /* EA Buffer */
                            0,    /* EA Length */
                            NULL);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseCreatedirStateAsync(pCreatedirState); // completed sync

            // intentional fall through

        case SRV_CREATEDIR_STAGE_SMB_V1_COMPLETED:

            ntStatus = pCreatedirState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pCreatedirState->stage = SRV_CREATEDIR_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_CREATEDIR_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildCreateDirectoryResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCreatedirState->stage = SRV_CREATEDIR_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_CREATEDIR_STAGE_SMB_V1_DONE:

            break;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTree->pShareInfo->mutex);

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pCreatedirState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pCreatedirState->mutex);

        SrvReleaseCreatedirState(pCreatedirState);
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

            if (pCreatedirState)
            {
                SrvReleaseCreatedirStateAsync(pCreatedirState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildCreatedirState(
    PSMB_CREATE_DIRECTORY_REQUEST_HEADER pRequestHeader,
    PWSTR                                pwszPathFragment,
    PSRV_CREATEDIR_STATE_SMB_V1*         ppCreatedirState
    )
{
    NTSTATUS                    ntStatus        = STATUS_SUCCESS;
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_CREATEDIR_STATE_SMB_V1),
                    (PVOID*)&pCreatedirState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreatedirState->refCount = 1;

    pthread_mutex_init(&pCreatedirState->mutex, NULL);
    pCreatedirState->pMutex = &pCreatedirState->mutex;

    pCreatedirState->stage = SRV_CREATEDIR_STAGE_SMB_V1_INITIAL;

    pCreatedirState->pwszPathFragment = pwszPathFragment;
    pCreatedirState->pRequestHeader   = pRequestHeader;

    *ppCreatedirState = pCreatedirState;

cleanup:

    return ntStatus;

error:

    *ppCreatedirState = NULL;

    if (pCreatedirState)
    {
        SrvFreeCreatedirState(pCreatedirState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildCreateDirectoryResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PSMB_CREATE_DIRECTORY_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_CREATE_DIRECTORY,
                        STATUS_SUCCESS,
                        TRUE,
                        pCtxSmb1->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pCtxSmb1->pSession->uid,
                        pSmbRequest->pHeader->mid,
                        pConnection->serverProperties.bRequireSecuritySignatures,
                        &pSmbResponse->pHeader,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    else
    {
        ntStatus = SrvMarshalHeaderAndX_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_CREATE_DIRECTORY,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 0;

    ntStatus = WireMarshallCreateDirectoryResponse(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pResponseHeader,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->pAndXHeader = NULL;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
VOID
SrvPrepareCreatedirStateAsync(
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState,
    PSRV_EXEC_CONTEXT          pExecContext
    )
{
    pCreatedirState->acb.Callback        = &SrvExecuteCreatedirAsyncCB;

    pCreatedirState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pCreatedirState->acb.AsyncCancelContext = NULL;

    pCreatedirState->pAcb = &pCreatedirState->acb;
}

static
VOID
SrvExecuteCreatedirAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState   = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pCreatedirState =
            (PSRV_CREATEDIR_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pCreatedirState->mutex);

    if (pCreatedirState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pCreatedirState->pAcb->AsyncCancelContext);
    }

    pCreatedirState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pCreatedirState->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V1.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

static
VOID
SrvReleaseCreatedirStateAsync(
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState
    )
{
    if (pCreatedirState->pAcb)
    {
        pCreatedirState->acb.Callback       = NULL;

        if (pCreatedirState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext =
                    (PSRV_EXEC_CONTEXT)pCreatedirState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pCreatedirState->pAcb->CallbackContext = NULL;
        }

        if (pCreatedirState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pCreatedirState->pAcb->AsyncCancelContext);
        }

        pCreatedirState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseCreatedirStateHandle(
    HANDLE hState
    )
{
    SrvReleaseCreatedirState((PSRV_CREATEDIR_STATE_SMB_V1)hState);
}

static
VOID
SrvReleaseCreatedirState(
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState
    )
{
    if (InterlockedDecrement(&pCreatedirState->refCount) == 0)
    {
        SrvFreeCreatedirState(pCreatedirState);
    }
}

static
VOID
SrvFreeCreatedirState(
    PSRV_CREATEDIR_STATE_SMB_V1 pCreatedirState
    )
{
    if (pCreatedirState->pAcb && pCreatedirState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pCreatedirState->pAcb->AsyncCancelContext);
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pCreatedirState->fileName.FileName)
    {
        SrvFreeMemory(pCreatedirState->fileName.FileName);
    }

    if (pCreatedirState->hFile)
    {
        IoCloseFile(pCreatedirState->hFile);
    }

    if (pCreatedirState->pMutex)
    {
        pthread_mutex_destroy(&pCreatedirState->mutex);
    }

    SrvFreeMemory(pCreatedirState);
}
