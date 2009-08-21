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
SrvDeleteFiles(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildDeleteResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildDeleteState(
    PSMB_DELETE_REQUEST_HEADER  pRequestHeader,
    PWSTR                       pwszSearchPattern,
    BOOLEAN                     bUseLongFilenames,
    PSRV_DELETE_STATE_SMB_V1*   ppDeleteState
    );

VOID
SrvPrepareDeleteStateAsync(
    PSRV_DELETE_STATE_SMB_V1 pDeleteState,
    PSRV_EXEC_CONTEXT        pExecContext
    );

static
VOID
SrvExecuteDeleteAsyncCB(
    PVOID pContext
    );

VOID
SrvReleaseDeleteStateAsync(
    PSRV_DELETE_STATE_SMB_V1 pDeleteState
    );

static
VOID
SrvReleaseDeleteStateHandle(
    HANDLE hDeleteState
    );

static
VOID
SrvReleaseDeleteState(
    PSRV_DELETE_STATE_SMB_V1 pDeleteState
    );

static
VOID
SrvFreeDeleteState(
    PSRV_DELETE_STATE_SMB_V1 pDeleteState
    );

NTSTATUS
SrvProcessDelete(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_DELETE_STATE_SMB_V1   pDeleteState = NULL;
    BOOLEAN                    bInLock      = FALSE;
    BOOLEAN                    bTreeInLock  = FALSE;
    PWSTR                      pwszFilesystemPath = NULL;

    pDeleteState = (PSRV_DELETE_STATE_SMB_V1)pCtxSmb1->hState;

    if (pDeleteState)
    {
        InterlockedIncrement(&pDeleteState->refCount);
    }
    else
    {
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PSMB_DELETE_REQUEST_HEADER  pRequestHeader    = NULL; // Do not free
        PWSTR                       pwszSearchPattern = NULL; // Do not free
        BOOLEAN                     bUseLongFilenames = FALSE;

        ntStatus = WireUnmarshallDeleteRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pwszSearchPattern);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pwszSearchPattern || !*pwszSearchPattern)
        {
            ntStatus = STATUS_CANNOT_DELETE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (pSmbRequest->pHeader->flags2 & FLAG2_KNOWS_LONG_NAMES)
        {
            bUseLongFilenames = TRUE;
        }

        ntStatus = SrvBuildDeleteState(
                        pRequestHeader,
                        pwszSearchPattern,
                        bUseLongFilenames,
                        &pDeleteState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pDeleteState;
        InterlockedIncrement(&pDeleteState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseDeleteStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pDeleteState->mutex);

    switch (pDeleteState->stage)
    {
        case SRV_DELETE_STAGE_SMB_V1_INITIAL:

            ntStatus = SrvConnectionFindSession_SMB_V1(
                            pCtxSmb1,
                            pConnection,
                            pSmbRequest->pHeader->uid,
                            &pDeleteState->pSession);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSessionFindTree_SMB_V1(
                            pCtxSmb1,
                            pDeleteState->pSession,
                            pSmbRequest->pHeader->tid,
                            &pDeleteState->pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_LOCK_RWMUTEX_SHARED(   bTreeInLock,
                                        &pDeleteState->pTree->pShareInfo->mutex);

            ntStatus = SrvAllocateStringW(
                            pDeleteState->pTree->pShareInfo->pwszPath,
                            &pwszFilesystemPath);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(bTreeInLock,
                                &pDeleteState->pTree->pShareInfo->mutex);

            ntStatus = SrvFinderBuildSearchPath(
                            pwszFilesystemPath,
                            pDeleteState->pwszSearchPattern,
                            &pDeleteState->pwszFilesystemPath,
                            &pDeleteState->pwszSearchPattern2);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvFinderCreateSearchSpace(
                            pDeleteState->pSession->pIoSecurityContext,
                            pDeleteState->pSession->hFinderRepository,
                            pDeleteState->pwszFilesystemPath,
                            pDeleteState->pwszSearchPattern2,
                            pDeleteState->pRequestHeader->usSearchAttributes,
                            pDeleteState->ulSearchStorageType,
                            SMB_FIND_FILE_BOTH_DIRECTORY_INFO,
                            pDeleteState->bUseLongFilenames,
                            &pDeleteState->hSearchSpace,
                            &pDeleteState->usSearchId);
            BAIL_ON_NT_STATUS(ntStatus);

            pDeleteState->stage = SRV_DELETE_STAGE_SMB_V1_DELETE_FILES;

            // intentional fall through

        case SRV_DELETE_STAGE_SMB_V1_DELETE_FILES:

            ntStatus = SrvDeleteFiles(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pDeleteState->stage = SRV_DELETE_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_DELETE_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildDeleteResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pDeleteState->stage = SRV_DELETE_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_DELETE_STAGE_SMB_V1_DONE:

            break;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pDeleteState->pTree->pShareInfo->mutex);

    if (pDeleteState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pDeleteState->mutex);

        SrvReleaseDeleteState(pDeleteState);
    }

    if (pwszFilesystemPath)
    {
        SrvFreeMemory(pwszFilesystemPath);
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

            if (pDeleteState)
            {
                SrvReleaseDeleteStateAsync(pDeleteState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvDeleteFiles(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_DELETE_STATE_SMB_V1   pDeleteState = NULL;
    PWSTR                      pwszFilename = NULL;
    BOOLEAN                    bDone        = FALSE;

    pDeleteState = (PSRV_DELETE_STATE_SMB_V1)pCtxSmb1->hState;

    if (pDeleteState->bPendingCreate)
    {
        pDeleteState->bPendingCreate = FALSE;

        ntStatus = pDeleteState->ioStatusBlock.Status;
        BAIL_ON_NT_STATUS(ntStatus);

        if (pDeleteState->hFile)
        {
            IoCloseFile(pDeleteState->hFile);
            pDeleteState->hFile = NULL;
        }

        pDeleteState->iResult++;
    }

    do
    {
        for (   ;
                pDeleteState->iResult < pDeleteState->usSearchResultCount;
                pDeleteState->iResult++)
        {
            if (!pDeleteState->pResult)
            {
                pDeleteState->pResult =
                    (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER)pDeleteState->pData;
            }
            else if (pDeleteState->pResult->NextEntryOffset)
            {
                PBYTE pTmp = (PBYTE)pDeleteState->pResult +
                                pDeleteState->pResult->NextEntryOffset;

                pDeleteState->pResult =
                            (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER)pTmp;
            }
            else
            {
                ntStatus = STATUS_INTERNAL_ERROR;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            if (pDeleteState->hFile)
            {
                IoCloseFile(pDeleteState->hFile);
                pDeleteState->hFile = NULL;
            }

            if (pwszFilename)
            {
                SrvFreeMemory(pwszFilename);
                pwszFilename = NULL;
            }

            if (pDeleteState->bUseLongFilenames)
            {
                ntStatus = SrvAllocateMemory(
                                pDeleteState->pResult->FileNameLength + sizeof(wchar16_t),
                                (PVOID*)&pwszFilename);
                BAIL_ON_NT_STATUS(ntStatus);

                memcpy((PBYTE)pwszFilename,
                       (PBYTE)pDeleteState->pResult->FileName,
                       pDeleteState->pResult->FileNameLength);
            }
            else
            {
                ntStatus = SrvAllocateMemory(
                                pDeleteState->pResult->ShortNameLength + sizeof(wchar16_t),
                                (PVOID*)&pwszFilename);
                BAIL_ON_NT_STATUS(ntStatus);

                memcpy((PBYTE)pwszFilename,
                       (PBYTE)pDeleteState->pResult->ShortName,
                       pDeleteState->pResult->ShortNameLength);
            }

            if (pDeleteState->fileName.FileName)
            {
                SrvFreeMemory(pDeleteState->fileName.FileName);
                pDeleteState->fileName.FileName = NULL;
            }

            ntStatus = SrvBuildFilePath(
                            pDeleteState->pwszFilesystemPath,
                            pwszFilename,
                            &pDeleteState->fileName.FileName);
            BAIL_ON_NT_STATUS(ntStatus);

            if (pDeleteState->pResult->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                ntStatus = STATUS_FILE_IS_A_DIRECTORY;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            SrvPrepareDeleteStateAsync(pDeleteState, pExecContext);

            pDeleteState->bPendingCreate = TRUE;

            ntStatus = IoCreateFile(
                            &pDeleteState->hFile,
                            pDeleteState->pAcb,
                            &pDeleteState->ioStatusBlock,
                            pDeleteState->pSession->pIoSecurityContext,
                            &pDeleteState->fileName,
                            pDeleteState->pSecurityDescriptor,
                            pDeleteState->pSecurityQOS,
                            DELETE,
                            0,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN,
                            pDeleteState->ulCreateOptions,
                            NULL,
                            0,
                            NULL);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseDeleteStateAsync(pDeleteState); // completed sync

            pDeleteState->bPendingCreate = FALSE;
        }

        if (!pDeleteState->bEndOfSearch)
        {
            if (pDeleteState->pData)
            {
                SrvFreeMemory(pDeleteState->pData);
                pDeleteState->pData = NULL;
            }

            pDeleteState->iResult = 0;
            pDeleteState->pResult = NULL;

            ntStatus = SrvFinderGetSearchResults(
                            pDeleteState->hSearchSpace,
                            FALSE,                 /* bReturnSingleEntry    */
                            FALSE,                 /* bRestartScan          */
                            10,                    /* Desired serarch count */
                            UINT16_MAX,            /* Max data count        */
                            pDeleteState->usDataOffset,
                            &pDeleteState->pData,
                            &pDeleteState->usDataLen,
                            &pDeleteState->usSearchResultCount,
                            &pDeleteState->bEndOfSearch);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            bDone = TRUE;
        }

    } while (!bDone);

cleanup:

    if (pwszFilename)
    {
        SrvFreeMemory(pwszFilename);
    }

    return ntStatus;

error:

    /* Have to do some error mapping here to match WinXP */

    switch (ntStatus)
    {
        case STATUS_PENDING:
        case STATUS_FILE_IS_A_DIRECTORY:

            break;

        case STATUS_OBJECT_NAME_NOT_FOUND:
        case STATUS_NO_SUCH_FILE:

            ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;

            break;

        default:

            ntStatus = STATUS_CANNOT_DELETE;

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildDeleteResponse(
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
    PSRV_DELETE_STATE_SMB_V1   pDeleteState = NULL;
    PSMB_DELETE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE  pOutBuffer           = pSmbResponse->pBuffer;
    ULONG  ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG  ulOffset             = 0;
    USHORT usBytesUsed          = 0;
    ULONG  ulTotalBytesUsed     = 0;

    pDeleteState = (PSRV_DELETE_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_DELETE,
                        STATUS_SUCCESS,
                        TRUE,
                        pDeleteState->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pDeleteState->pSession->uid,
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
                        COM_DELETE,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 1;

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
NTSTATUS
SrvBuildDeleteState(
    PSMB_DELETE_REQUEST_HEADER  pRequestHeader,
    PWSTR                       pwszSearchPattern,
    BOOLEAN                     bUseLongFilenames,
    PSRV_DELETE_STATE_SMB_V1*   ppDeleteState
    )
{
    NTSTATUS                 ntStatus    = STATUS_SUCCESS;
    PSRV_DELETE_STATE_SMB_V1 pDeleteState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_DELETE_STATE_SMB_V1),
                    (PVOID*)&pDeleteState);
    BAIL_ON_NT_STATUS(ntStatus);

    pDeleteState->refCount = 1;

    pthread_mutex_init(&pDeleteState->mutex, NULL);
    pDeleteState->pMutex = &pDeleteState->mutex;

    pDeleteState->stage = SRV_DELETE_STAGE_SMB_V1_INITIAL;

    pDeleteState->pRequestHeader    = pRequestHeader;
    pDeleteState->pwszSearchPattern = pwszSearchPattern;
    pDeleteState->bUseLongFilenames = bUseLongFilenames;

    pDeleteState->ulCreateOptions =
                            FILE_DELETE_ON_CLOSE|FILE_NON_DIRECTORY_FILE;

    *ppDeleteState = pDeleteState;

cleanup:

    return ntStatus;

error:

    *ppDeleteState = NULL;

    if (pDeleteState)
    {
        SrvFreeDeleteState(pDeleteState);
    }

    goto cleanup;
}

VOID
SrvPrepareDeleteStateAsync(
    PSRV_DELETE_STATE_SMB_V1 pDeleteState,
    PSRV_EXEC_CONTEXT        pExecContext
    )
{
    pDeleteState->acb.Callback        = &SrvExecuteDeleteAsyncCB;

    pDeleteState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pDeleteState->acb.AsyncCancelContext = NULL;

    pDeleteState->pAcb = &pDeleteState->acb;
}

static
VOID
SrvExecuteDeleteAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_DELETE_STATE_SMB_V1   pDeleteState     = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pDeleteState =
        (PSRV_DELETE_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pDeleteState->mutex);

    if (pDeleteState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pDeleteState->pAcb->AsyncCancelContext);
    }

    pDeleteState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pDeleteState->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V1.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

VOID
SrvReleaseDeleteStateAsync(
    PSRV_DELETE_STATE_SMB_V1 pDeleteState
    )
{
    if (pDeleteState->pAcb)
    {
        pDeleteState->acb.Callback = NULL;

        if (pDeleteState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pDeleteState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pDeleteState->pAcb->CallbackContext = NULL;
        }

        if (pDeleteState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pDeleteState->pAcb->AsyncCancelContext);
        }

        pDeleteState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseDeleteStateHandle(
    HANDLE hDeleteState
    )
{
    SrvReleaseDeleteState((PSRV_DELETE_STATE_SMB_V1)hDeleteState);
}

static
VOID
SrvReleaseDeleteState(
    PSRV_DELETE_STATE_SMB_V1 pDeleteState
    )
{
    if (InterlockedDecrement(&pDeleteState->refCount) == 0)
    {
        SrvFreeDeleteState(pDeleteState);
    }
}

static
VOID
SrvFreeDeleteState(
    PSRV_DELETE_STATE_SMB_V1 pDeleteState
    )
{
    if (pDeleteState->pAcb && pDeleteState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pDeleteState->pAcb->AsyncCancelContext);
    }

    if (pDeleteState->hSearchSpace)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvFinderCloseSearchSpace(
                        pDeleteState->pSession->hFinderRepository,
                        pDeleteState->usSearchId);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to close search space [Id:%d][code:%d]",
                          pDeleteState->usSearchId,
                          ntStatus2);
        }

        SrvFinderReleaseSearchSpace(pDeleteState->hSearchSpace);
    }

    if (pDeleteState->pTree)
    {
        SrvTreeRelease(pDeleteState->pTree);
    }

    if (pDeleteState->pSession)
    {
        SrvSessionRelease(pDeleteState->pSession);
    }

    if (pDeleteState->pwszFilesystemPath)
    {
        SrvFreeMemory(pDeleteState->pwszFilesystemPath);
    }

    if (pDeleteState->pwszSearchPattern2)
    {
        SrvFreeMemory(pDeleteState->pwszSearchPattern2);
    }

    if (pDeleteState->hFile)
    {
        IoCloseFile(pDeleteState->hFile);
    }

    if (pDeleteState->pData)
    {
        SrvFreeMemory(pDeleteState->pData);
    }

    if (pDeleteState->fileName.FileName)
    {
        SrvFreeMemory(pDeleteState->fileName.FileName);
    }

    if (pDeleteState->pMutex)
    {
        pthread_mutex_destroy(&pDeleteState->mutex);
    }

    SrvFreeMemory(pDeleteState);
}
