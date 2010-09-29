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
SrvBuildDeletedirState(
    PDELETE_DIRECTORY_REQUEST_HEADER pRequestHeader,
    PWSTR                            pwszPathFragment,
    PSRV_DELETEDIR_STATE_SMB_V1*     ppDeletedirState
    );

static
NTSTATUS
SrvSetFileInformation_inlock(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvLogDelDirState_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    );

static
NTSTATUS
SrvBuildDeleteDirectoryResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareDeletedirStateAsync(
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState,
    PSRV_EXEC_CONTEXT           pExecContext
    );

static
VOID
SrvExecuteDeletedirAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseDeletedirStateAsync(
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState
    );

static
VOID
SrvReleaseDeletedirStateHandle(
    HANDLE hState
    );

static
VOID
SrvReleaseDeletedirState(
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState
    );

static
VOID
SrvFreeDeletedirState(
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState
    );

NTSTATUS
SrvProcessDeleteDirectory(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus     = 0;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION           pSession     = NULL;
    PLWIO_SRV_TREE              pTree        = NULL;
    BOOLEAN                     bInLock      = FALSE;
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState = NULL;

    pDeletedirState = (PSRV_DELETEDIR_STATE_SMB_V1)pCtxSmb1->hState;
    if (pDeletedirState)
    {
        InterlockedIncrement(&pDeletedirState->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PDELETE_DIRECTORY_REQUEST_HEADER pRequestHeader   = NULL; // Do not free
        PWSTR                            pwszPathFragment = NULL; // Do not free

        ntStatus = SrvConnectionFindSession_SMB_V1(
                        pCtxSmb1,
                        pConnection,
                        pSmbRequest->pHeader->uid,
                        &pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSetStatSessionInfo(pExecContext, pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSessionFindTree_SMB_V1(
                        pCtxSmb1,
                        pSession,
                        pSmbRequest->pHeader->tid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = WireUnmarshallDirectoryDeleteRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pwszPathFragment);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pwszPathFragment || !*pwszPathFragment)
        {
            ntStatus = STATUS_CANNOT_DELETE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SrvBuildDeletedirState(
                        pRequestHeader,
                        pwszPathFragment,
                        &pDeletedirState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pDeletedirState;
        InterlockedIncrement(&pDeletedirState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseDeletedirStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pDeletedirState->mutex);

    switch (pDeletedirState->stage)
    {
        case SRV_DELETEDIR_STAGE_SMB_V1_INITIAL:

            SRV_LOG_CALL_DEBUG(
                    pExecContext->pLogContext,
                    SMB_PROTOCOL_VERSION_1,
                    pCtxSmb1->pRequests[pCtxSmb1->iMsg].pHeader->command,
                    &SrvLogDelDirState_SMB_V1,
                    pDeletedirState);

            ntStatus = SrvBuildTreeRelativePath(
                            pCtxSmb1->pTree,
                            pDeletedirState->pwszPathFragment,
                            &pDeletedirState->fileName);
            BAIL_ON_NT_STATUS(ntStatus);

            pDeletedirState->stage = SRV_DELETEDIR_STAGE_SMB_V1_ATTEMPT_SET_INFO;

            SrvPrepareDeletedirStateAsync(pDeletedirState, pExecContext);

            ntStatus = SrvIoCreateFile(
                            pCtxSmb1->pTree->pShareInfo,
                            &pDeletedirState->hFile,
                            pDeletedirState->pAcb,
                            &pDeletedirState->ioStatusBlock,
                            pCtxSmb1->pSession->pIoSecurityContext,
                            &pDeletedirState->fileName,
                            pDeletedirState->pSecurityDescriptor,
                            pDeletedirState->pSecurityQOS,
                            DELETE,
                            0,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_READ | FILE_SHARE_WRITE |
                            FILE_SHARE_DELETE,
                            FILE_OPEN,
                            FILE_DIRECTORY_FILE,
                            NULL,
                            0,
                            pDeletedirState->pEcpList);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseDeletedirStateAsync(pDeletedirState);

            // intentional fall through

        case SRV_DELETEDIR_STAGE_SMB_V1_ATTEMPT_SET_INFO:

            pDeletedirState->stage = SRV_DELETEDIR_STAGE_SMB_V1_COMPLETED;

            ntStatus = SrvSetFileInformation_inlock(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            // intentional fall through

        case SRV_DELETEDIR_STAGE_SMB_V1_COMPLETED:

            ntStatus = pDeletedirState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            // intentional fall through

        case SRV_DELETEDIR_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildDeleteDirectoryResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            // intentional fall through

        case SRV_DELETEDIR_STAGE_SMB_V1_DONE:

            break;
    }

cleanup:

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pDeletedirState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pDeletedirState->mutex);

        SrvReleaseDeletedirState(pDeletedirState);
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

            if (pDeletedirState)
            {
                SrvReleaseDeletedirStateAsync(pDeletedirState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildDeletedirState(
    PDELETE_DIRECTORY_REQUEST_HEADER pRequestHeader,
    PWSTR                                pwszPathFragment,
    PSRV_DELETEDIR_STATE_SMB_V1*         ppDeletedirState
    )
{
    NTSTATUS                    ntStatus        = STATUS_SUCCESS;
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_DELETEDIR_STATE_SMB_V1),
                    (PVOID*)&pDeletedirState);
    BAIL_ON_NT_STATUS(ntStatus);

    pDeletedirState->refCount = 1;

    pthread_mutex_init(&pDeletedirState->mutex, NULL);
    pDeletedirState->pMutex = &pDeletedirState->mutex;

    pDeletedirState->stage = SRV_DELETEDIR_STAGE_SMB_V1_INITIAL;

    pDeletedirState->pwszPathFragment = pwszPathFragment;
    pDeletedirState->pRequestHeader   = pRequestHeader;

    pDeletedirState->pFileDispositionInfo =
	&pDeletedirState->fileDispositionInfo;

    pDeletedirState->pFileDispositionInfo->DeleteFile = TRUE;

    *ppDeletedirState = pDeletedirState;

cleanup:

    return ntStatus;

error:

    *ppDeletedirState = NULL;

    if (pDeletedirState)
    {
        SrvFreeDeletedirState(pDeletedirState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvSetFileInformation_inlock(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState = NULL;

    pDeletedirState = (PSRV_DELETEDIR_STATE_SMB_V1)pCtxSmb1->hState;

    SrvPrepareDeletedirStateAsync(pDeletedirState, pExecContext);

    ntStatus = IoSetInformationFile(
                    pDeletedirState->hFile,
                    pDeletedirState->pAcb,
                    &pDeletedirState->ioStatusBlock,
                    pDeletedirState->pFileDispositionInfo,
                    sizeof(pDeletedirState->fileDispositionInfo),
                    FileDispositionInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseDeletedirStateAsync(pDeletedirState); // completed synchronously

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvLogDelDirState_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_DELETEDIR_STATE_SMB_V1 pDeldirState = NULL;
    PSTR pszPath = NULL;
    va_list msgList;

    va_start(msgList, ulLine);

    pDeldirState = va_arg(msgList, PSRV_DELETEDIR_STATE_SMB_V1);

    if (pDeldirState)
    {
        if (pDeldirState->pwszPathFragment)
        {
            ntStatus = SrvWc16sToMbs(pDeldirState->pwszPathFragment, &pszPath);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (logLevel >= LWIO_LOG_LEVEL_DEBUG)
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "[%s() %s:%u] Delete directory state: Path(%s)",
                    LWIO_SAFE_LOG_STRING(pszFunction),
                    LWIO_SAFE_LOG_STRING(pszFile),
                    ulLine,
                    LWIO_SAFE_LOG_STRING(pszPath));
        }
        else
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "Delete directory state: Path(%s)",
                    LWIO_SAFE_LOG_STRING(pszPath));
        }
    }

error:

    va_end(msgList);

    SRV_SAFE_FREE_MEMORY(pszPath);

    return;
}

static
NTSTATUS
SrvBuildDeleteDirectoryResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                       iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1         pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1         pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PSMB_DELETE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE  pOutBuffer           = pSmbResponse->pBuffer;
    ULONG  ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG  ulOffset             = 0;
    USHORT usBytesUsed          = 0;
    ULONG  ulTotalBytesUsed     = 0;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_DELETE_DIRECTORY,
                        STATUS_SUCCESS,
                        TRUE,
                        pConnection->serverProperties.Capabilities,
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
                        COM_DELETE_DIRECTORY,
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

    ntStatus = WireMarshallDeleteResponse(
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
SrvPrepareDeletedirStateAsync(
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState,
    PSRV_EXEC_CONTEXT          pExecContext
    )
{
    pDeletedirState->acb.Callback        = &SrvExecuteDeletedirAsyncCB;

    pDeletedirState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pDeletedirState->acb.AsyncCancelContext = NULL;

    pDeletedirState->pAcb = &pDeletedirState->acb;
}

static
VOID
SrvExecuteDeletedirAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState   = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pDeletedirState =
            (PSRV_DELETEDIR_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pDeletedirState->mutex);

    if (pDeletedirState->pAcb && pDeletedirState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pDeletedirState->pAcb->AsyncCancelContext);
    }

    pDeletedirState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pDeletedirState->mutex);

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
SrvReleaseDeletedirStateAsync(
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState
    )
{
    if (pDeletedirState->pAcb)
    {
        pDeletedirState->acb.Callback       = NULL;

        if (pDeletedirState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext =
                    (PSRV_EXEC_CONTEXT)pDeletedirState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pDeletedirState->pAcb->CallbackContext = NULL;
        }

        if (pDeletedirState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pDeletedirState->pAcb->AsyncCancelContext);
        }

        pDeletedirState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseDeletedirStateHandle(
    HANDLE hState
    )
{
    SrvReleaseDeletedirState((PSRV_DELETEDIR_STATE_SMB_V1)hState);
}

static
VOID
SrvReleaseDeletedirState(
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState
    )
{
    if (InterlockedDecrement(&pDeletedirState->refCount) == 0)
    {
        SrvFreeDeletedirState(pDeletedirState);
    }
}

static
VOID
SrvFreeDeletedirState(
    PSRV_DELETEDIR_STATE_SMB_V1 pDeletedirState
    )
{
    if (pDeletedirState->pAcb && pDeletedirState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pDeletedirState->pAcb->AsyncCancelContext);
    }

    if (pDeletedirState->pEcpList)
    {
        IoRtlEcpListFree(&pDeletedirState->pEcpList);
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pDeletedirState->fileName.FileName)
    {
        SrvFreeMemory(pDeletedirState->fileName.FileName);
    }

    if (pDeletedirState->hFile)
    {
        IoCloseFile(pDeletedirState->hFile);
    }

    if (pDeletedirState->pMutex)
    {
        pthread_mutex_destroy(&pDeletedirState->mutex);
    }

    SrvFreeMemory(pDeletedirState);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
