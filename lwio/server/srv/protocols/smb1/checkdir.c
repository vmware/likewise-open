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
 *        checkdir.c
 *
 * Abstract:
 *
 *        Likewise SMB Server
 *
 *        SMBCheckDirectory
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *          Sriram Nambakam <snambakam@likewise.com>
 */


#include "includes.h"

static
NTSTATUS
SrvBuildCheckdirState(
    PSMB_CHECK_DIRECTORY_REQUEST_HEADER pRequestHeader,
    PWSTR                               pwszPathFragment,
    PSRV_CHECKDIR_STATE_SMB_V1*         ppCheckdirState
    );

static
NTSTATUS
SrvBuildCheckDirectoryResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareCheckdirStateAsync(
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState,
    PSRV_EXEC_CONTEXT          pExecContext
    );

static
VOID
SrvExecuteCheckdirAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseCheckdirStateAsync(
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState
    );

static
VOID
SrvReleaseCheckdirStateHandle(
    HANDLE hState
    );

static
VOID
SrvReleaseCheckdirState(
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState
    );

static
VOID
SrvFreeCheckdirState(
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState
    );

NTSTATUS
SrvProcessCheckDirectory(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus       = 0;
    PLWIO_SRV_CONNECTION       pConnection    = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1       = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION          pSession       = NULL;
    PLWIO_SRV_TREE             pTree          = NULL;
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState = NULL;
    BOOLEAN                    bTreeInLock    = FALSE;
    BOOLEAN                    bInLock        = FALSE;

    pCheckdirState = (PSRV_CHECKDIR_STATE_SMB_V1)pCtxSmb1->hState;
    if (pCheckdirState)
    {
        InterlockedIncrement(&pCheckdirState->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PWSTR pwszPathFragment = NULL; // Do not free
        PSMB_CHECK_DIRECTORY_REQUEST_HEADER pRequestHeader = NULL;// Do not free

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

        ntStatus = WireUnmarshallCheckDirectoryRequest(
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

        ntStatus = SrvBuildCheckdirState(
                        pRequestHeader,
                        pwszPathFragment,
                        &pCheckdirState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pCheckdirState;
        InterlockedIncrement(&pCheckdirState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseCheckdirStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pCheckdirState->mutex);

    switch (pCheckdirState->stage)
    {
        case SRV_CHECKDIR_STAGE_SMB_V1_INITIAL:

            LWIO_LOCK_RWMUTEX_SHARED(   bTreeInLock,
                                        &pCtxSmb1->pTree->pShareInfo->mutex);

            ntStatus = SrvBuildFilePath(
                            pCtxSmb1->pTree->pShareInfo->pwszPath,
                            pCheckdirState->pwszPathFragment,
                            &pCheckdirState->fileName.FileName);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(bTreeInLock,
                                &pCtxSmb1->pTree->pShareInfo->mutex);

            pCheckdirState->stage = SRV_CHECKDIR_STAGE_SMB_V1_ATTEMPT_CHECK;

            // Intentional fall through

        case SRV_CHECKDIR_STAGE_SMB_V1_ATTEMPT_CHECK:

            if (!pCheckdirState->hFile)
            {
                SrvPrepareCheckdirStateAsync(pCheckdirState, pExecContext);

                ntStatus = IoCreateFile(
                                &pCheckdirState->hFile,
                                pCheckdirState->pAcb,
                                &pCheckdirState->ioStatusBlock,
                                pCtxSmb1->pSession->pIoSecurityContext,
                                &pCheckdirState->fileName,
                                pCheckdirState->pSecurityDescriptor,
                                pCheckdirState->pSecurityQOS,
                                GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,
                                0, /* allocation size */
                                FILE_ATTRIBUTE_NORMAL,
                                FILE_SHARE_READ|FILE_SHARE_WRITE,
                                FILE_OPEN,
                                FILE_DIRECTORY_FILE,
                                NULL, /* EA Buffer */
                                0,    /* EA Length */
                                NULL);
                BAIL_ON_NT_STATUS(ntStatus);

                SrvReleaseCheckdirStateAsync(pCheckdirState); // completed sync
            }

            pCheckdirState->stage = SRV_CHECKDIR_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_CHECKDIR_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = pCheckdirState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvBuildCheckDirectoryResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCheckdirState->stage = SRV_CHECKDIR_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_CHECKDIR_STAGE_SMB_V1_DONE:

            break;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb1->pTree->pShareInfo->mutex);

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pCheckdirState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pCheckdirState->mutex);

        SrvReleaseCheckdirState(pCheckdirState);
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

            if (pCheckdirState)
            {
                SrvReleaseCheckdirStateAsync(pCheckdirState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildCheckdirState(
    PSMB_CHECK_DIRECTORY_REQUEST_HEADER pRequestHeader,
    PWSTR                               pwszPathFragment,
    PSRV_CHECKDIR_STATE_SMB_V1*         ppCheckdirState
    )
{
    NTSTATUS                   ntStatus       = STATUS_SUCCESS;
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_CHECKDIR_STATE_SMB_V1),
                    (PVOID*)&pCheckdirState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCheckdirState->refCount = 1;

    pthread_mutex_init(&pCheckdirState->mutex, NULL);
    pCheckdirState->pMutex = &pCheckdirState->mutex;

    pCheckdirState->stage = SRV_CHECKDIR_STAGE_SMB_V1_INITIAL;

    pCheckdirState->pwszPathFragment = pwszPathFragment;
    pCheckdirState->pRequestHeader   = pRequestHeader;

    *ppCheckdirState = pCheckdirState;

cleanup:

    return ntStatus;

error:

    *ppCheckdirState = NULL;

    if (pCheckdirState)
    {
        SrvFreeCheckdirState(pCheckdirState);
    }

    goto cleanup;
}


static
NTSTATUS
SrvBuildCheckDirectoryResponse(
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
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    USHORT usBytesUsed     = 0;
    ULONG ulTotalBytesUsed = 0;
    PSMB_CHECK_DIRECTORY_RESPONSE_HEADER pResponseHeader = NULL; // Do not free

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_CHECK_DIRECTORY,
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
                        COM_CHECK_DIRECTORY,
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

    ntStatus = WireMarshallCheckDirectoryResponse(
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
SrvPrepareCheckdirStateAsync(
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState,
    PSRV_EXEC_CONTEXT          pExecContext
    )
{
    pCheckdirState->acb.Callback        = &SrvExecuteCheckdirAsyncCB;

    pCheckdirState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pCheckdirState->acb.AsyncCancelContext = NULL;

    pCheckdirState->pAcb = &pCheckdirState->acb;
}

static
VOID
SrvExecuteCheckdirAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState   = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pCheckdirState =
            (PSRV_CHECKDIR_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pCheckdirState->mutex);

    if (pCheckdirState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pCheckdirState->pAcb->AsyncCancelContext);
    }

    pCheckdirState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pCheckdirState->mutex);

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
SrvReleaseCheckdirStateAsync(
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState
    )
{
    if (pCheckdirState->pAcb)
    {
        pCheckdirState->acb.Callback       = NULL;

        if (pCheckdirState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext =
                    (PSRV_EXEC_CONTEXT)pCheckdirState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pCheckdirState->pAcb->CallbackContext = NULL;
        }

        if (pCheckdirState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pCheckdirState->pAcb->AsyncCancelContext);
        }

        pCheckdirState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseCheckdirStateHandle(
    HANDLE hState
    )
{
    SrvReleaseCheckdirState((PSRV_CHECKDIR_STATE_SMB_V1)hState);
}

static
VOID
SrvReleaseCheckdirState(
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState
    )
{
    if (InterlockedDecrement(&pCheckdirState->refCount) == 0)
    {
        SrvFreeCheckdirState(pCheckdirState);
    }
}

static
VOID
SrvFreeCheckdirState(
    PSRV_CHECKDIR_STATE_SMB_V1 pCheckdirState
    )
{
    if (pCheckdirState->pAcb && pCheckdirState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pCheckdirState->pAcb->AsyncCancelContext);
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pCheckdirState->fileName.FileName)
    {
        SrvFreeMemory(pCheckdirState->fileName.FileName);
    }

    if (pCheckdirState->hFile)
    {
        IoCloseFile(pCheckdirState->hFile);
    }

    if (pCheckdirState->pMutex)
    {
        pthread_mutex_destroy(&pCheckdirState->mutex);
    }

    SrvFreeMemory(pCheckdirState);
}
