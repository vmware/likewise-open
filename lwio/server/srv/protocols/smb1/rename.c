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
SrvBuildRenameState(
    PSMB_RENAME_REQUEST_HEADER pRequestHeader,
    PWSTR                      pwszOldName,
    PWSTR                      pwszNewName,
    PSRV_RENAME_STATE_SMB_V1*  ppRenameState
    );

static
NTSTATUS
SrvExecuteRename(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildRenameResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareRenameStateAsync(
    PSRV_RENAME_STATE_SMB_V1 pRenameState,
    PSRV_EXEC_CONTEXT        pExecContext
    );

static
VOID
SrvExecuteRenameAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseRenameStateAsync(
    PSRV_RENAME_STATE_SMB_V1 pRenameState
    );

static
VOID
SrvReleaseRenameStateHandle(
    HANDLE hState
    );

static
VOID
SrvReleaseRenameState(
    PSRV_RENAME_STATE_SMB_V1 pRenameState
    );

static
VOID
SrvFreeRenameState(
    PSRV_RENAME_STATE_SMB_V1 pRenameState
    );

NTSTATUS
SrvProcessRename(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;
    PSRV_RENAME_STATE_SMB_V1   pRenameState = NULL;
    BOOLEAN                    bTreeInLock  = FALSE;
    BOOLEAN                    bInLock      = FALSE;

    pRenameState = (PSRV_RENAME_STATE_SMB_V1)pCtxSmb1->hState;
    if (pRenameState)
    {
        InterlockedIncrement(&pRenameState->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PSMB_RENAME_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PWSTR                      pwszOldName    = NULL; // Do not free
        PWSTR                      pwszNewName    = NULL; // Do not free

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

        ntStatus = WireUnmarshallRenameRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pwszOldName,
                        &pwszNewName);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildRenameState(
                        pRequestHeader,
                        pwszOldName,
                        pwszNewName,
                        &pRenameState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pRenameState;
        InterlockedIncrement(&pRenameState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseRenameStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pRenameState->mutex);

    switch (pRenameState->stage)
    {
        case SRV_RENAME_STAGE_SMB_V1_INITIAL:

            LWIO_LOCK_RWMUTEX_SHARED(   bTreeInLock,
                                        &pCtxSmb1->pTree->pShareInfo->mutex);

            ntStatus = SrvBuildFilePath(
                            pCtxSmb1->pTree->pShareInfo->pwszPath,
                            pRenameState->pwszOldName,
                            &pRenameState->oldName.FileName);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvAllocateStringW(
                            pTree->pShareInfo->pwszPath,
                            &pRenameState->dirPath.FileName);
            BAIL_ON_NT_STATUS(ntStatus);

            pRenameState->newName.FileName = pRenameState->pwszNewName;

            LWIO_UNLOCK_RWMUTEX(bTreeInLock,
                                &pCtxSmb1->pTree->pShareInfo->mutex);

            pRenameState->stage = SRV_RENAME_STAGE_SMB_V1_ATTEMPT_RENAME;

            // intentional fall through

        case SRV_RENAME_STAGE_SMB_V1_ATTEMPT_RENAME:

            ntStatus = SrvExecuteRename(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pRenameState->stage = SRV_RENAME_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_RENAME_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildRenameResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pRenameState->stage = SRV_RENAME_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_RENAME_STAGE_SMB_V1_DONE:

            break;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb1->pTree->pShareInfo->mutex);

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pRenameState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pRenameState->mutex);

        SrvReleaseRenameState(pRenameState);
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

            if (pRenameState)
            {
                SrvReleaseRenameStateAsync(pRenameState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildRenameState(
    PSMB_RENAME_REQUEST_HEADER pRequestHeader,
    PWSTR                      pwszOldName,
    PWSTR                      pwszNewName,
    PSRV_RENAME_STATE_SMB_V1*  ppRenameState
    )
{
    NTSTATUS                 ntStatus     = STATUS_SUCCESS;
    PSRV_RENAME_STATE_SMB_V1 pRenameState = NULL;

    if (!pwszOldName || !*pwszOldName || !pwszNewName || !*pwszNewName)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_RENAME_STATE_SMB_V1),
                    (PVOID*)&pRenameState);
    BAIL_ON_NT_STATUS(ntStatus);

    pRenameState->refCount = 1;

    pthread_mutex_init(&pRenameState->mutex, NULL);
    pRenameState->pMutex = &pRenameState->mutex;

    pRenameState->stage = SRV_RENAME_STAGE_SMB_V1_INITIAL;

    pRenameState->pRequestHeader = pRequestHeader;
    pRenameState->pwszOldName    = pwszOldName;
    pRenameState->pwszNewName    = pwszNewName;

    *ppRenameState = pRenameState;

cleanup:

    return ntStatus;

error:

    *ppRenameState = NULL;

    if (pRenameState)
    {
        SrvFreeRenameState(pRenameState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteRename(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus        = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol    = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1        = pCtxProtocol->pSmb1Context;
    PSRV_RENAME_STATE_SMB_V1   pRenameState    = NULL;

    pRenameState = (PSRV_RENAME_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = pRenameState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pRenameState->hDir)
    {
        SrvPrepareRenameStateAsync(pRenameState, pExecContext);

        ntStatus = IoCreateFile(
                        &pRenameState->hDir,
                        pRenameState->pAcb,
                        &pRenameState->ioStatusBlock,
                        pCtxSmb1->pSession->pIoSecurityContext,
                        &pRenameState->dirPath,
                        pRenameState->pSecurityDescriptor,
                        pRenameState->pSecurityQOS,
                        GENERIC_READ,
                        0,
                        FILE_ATTRIBUTE_NORMAL,
                        0,
                        FILE_OPEN,
                        FILE_DIRECTORY_FILE,
                        NULL, /* EA Buffer */
                        0,    /* EA Length */
                        NULL  /* ECP List  */
                        );
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseRenameStateAsync(pRenameState); // completed synchronously
    }

    if (!pRenameState->hFile)
    {
        SrvPrepareRenameStateAsync(pRenameState, pExecContext);

        ntStatus = IoCreateFile(
                        &pRenameState->hFile,
                        pRenameState->pAcb,
                        &pRenameState->ioStatusBlock,
                        pCtxSmb1->pSession->pIoSecurityContext,
                        &pRenameState->oldName,
                        pRenameState->pSecurityDescriptor,
                        pRenameState->pSecurityQOS,
                        DELETE,
                        0,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,
                        FILE_OPEN,
                        0,
                        NULL, /* EA Buffer */
                        0,    /* EA Length */
                        NULL  /* ECP List  */
                        );
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseRenameStateAsync(pRenameState); // completed synchronously
    }

    if (!pRenameState->pFileRenameInfo)
    {
        pRenameState->ulDataLen =
                sizeof(FILE_RENAME_INFORMATION) +
                wc16slen(pRenameState->newName.FileName) * sizeof(wchar16_t);

        ntStatus = SrvAllocateMemory(
                        pRenameState->ulDataLen,
                        (PVOID*)&pRenameState->pData);
        BAIL_ON_NT_STATUS(ntStatus);

        pRenameState->pFileRenameInfo =
                    (PFILE_RENAME_INFORMATION)pRenameState->pData;

        pRenameState->pFileRenameInfo->ReplaceIfExists = TRUE;
        pRenameState->pFileRenameInfo->RootDirectory   = pRenameState->hDir;
        pRenameState->pFileRenameInfo->FileNameLength  =
                wc16slen(pRenameState->newName.FileName) * sizeof(wchar16_t);
        memcpy( (PBYTE)pRenameState->pFileRenameInfo->FileName,
                (PBYTE)pRenameState->newName.FileName,
                pRenameState->pFileRenameInfo->FileNameLength);

        SrvPrepareRenameStateAsync(pRenameState, pExecContext);

        ntStatus = IoSetInformationFile(
                        pRenameState->hFile,
                        pRenameState->pAcb,
                        &pRenameState->ioStatusBlock,
                        pRenameState->pFileRenameInfo,
                        pRenameState->ulDataLen,
                        FileRenameInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseRenameStateAsync(pRenameState); // completed synchronously
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildRenameResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus     = 0;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                       iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1         pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1         pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PSMB_RENAME_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
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
                        COM_RENAME,
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
                        COM_RENAME,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 2;

    ntStatus = WireMarshallRenameResponse(
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

    pResponseHeader->usByteCount = 0;

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
SrvPrepareRenameStateAsync(
    PSRV_RENAME_STATE_SMB_V1 pRenameState,
    PSRV_EXEC_CONTEXT        pExecContext
    )
{
    pRenameState->acb.Callback        = &SrvExecuteRenameAsyncCB;

    pRenameState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pRenameState->acb.AsyncCancelContext = NULL;

    pRenameState->pAcb = &pRenameState->acb;
}

static
VOID
SrvExecuteRenameAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_RENAME_STATE_SMB_V1   pRenameState     = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pRenameState =
            (PSRV_RENAME_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pRenameState->mutex);

    if (pRenameState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pRenameState->pAcb->AsyncCancelContext);
    }

    pRenameState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pRenameState->mutex);

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
SrvReleaseRenameStateAsync(
    PSRV_RENAME_STATE_SMB_V1 pRenameState
    )
{
    if (pRenameState->pAcb)
    {
        pRenameState->acb.Callback       = NULL;

        if (pRenameState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext =
                    (PSRV_EXEC_CONTEXT)pRenameState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pRenameState->pAcb->CallbackContext = NULL;
        }

        if (pRenameState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pRenameState->pAcb->AsyncCancelContext);
        }

        pRenameState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseRenameStateHandle(
    HANDLE hState
    )
{
    SrvReleaseRenameState((PSRV_RENAME_STATE_SMB_V1)hState);
}

static
VOID
SrvReleaseRenameState(
    PSRV_RENAME_STATE_SMB_V1 pRenameState
    )
{
    if (InterlockedDecrement(&pRenameState->refCount) == 0)
    {
        SrvFreeRenameState(pRenameState);
    }
}

static
VOID
SrvFreeRenameState(
    PSRV_RENAME_STATE_SMB_V1 pRenameState
    )
{
    if (pRenameState->pAcb && pRenameState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pRenameState->pAcb->AsyncCancelContext);
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pRenameState->oldName.FileName)
    {
        SrvFreeMemory(pRenameState->oldName.FileName);
    }

    if (pRenameState->dirPath.FileName)
    {
        SrvFreeMemory(pRenameState->dirPath.FileName);
    }

    if (pRenameState->hDir)
    {
        IoCloseFile(pRenameState->hDir);
    }

    if (pRenameState->hFile)
    {
        IoCloseFile(pRenameState->hFile);
    }

    if (pRenameState->pData)
    {
        SrvFreeMemory(pRenameState->pData);
    }

    if (pRenameState->pMutex)
    {
        pthread_mutex_destroy(&pRenameState->mutex);
    }

    SrvFreeMemory(pRenameState);
}


