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
SrvBuildNTCreateResponse_inlock(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvExecuteCreateAsyncCB(
    PVOID pContext
    );

static
VOID
SrvExecuteCreateAsyncCB(
    PVOID pContext
    );

static
NTSTATUS
SrvBuildCreateState(
    PSRV_EXEC_CONTEXT         pExecContext,
    PCREATE_REQUEST_HEADER    pRequestHeader,
    PWSTR                     pwszFilename,
    PSRV_CREATE_STATE_SMB_V1* ppCreateState
    );

static
VOID
SrvReleaseCreateStateHandle(
    HANDLE hState
    );

static
VOID
SrvReleaseCreateState(
    PSRV_CREATE_STATE_SMB_V1 pCreateState
    );

static
VOID
SrvFreeCreateState(
    PSRV_CREATE_STATE_SMB_V1 pCreateState
    );

NTSTATUS
SrvProcessNTCreateAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PLWIO_SRV_SESSION          pSession = NULL;
    PLWIO_SRV_TREE             pTree = NULL;
    PCREATE_REQUEST_HEADER     pRequestHeader = NULL; // Do not free
    PWSTR                      pwszFilename = NULL;   // Do not free
    PSRV_CREATE_STATE_SMB_V1   pCreateState = NULL;
    BOOLEAN                    bInLock = FALSE;

    pCreateState = (PSRV_CREATE_STATE_SMB_V1)pCtxSmb1->hState;

    if (pCreateState)
    {
        InterlockedIncrement(&pCreateState->refCount);

        LWIO_LOCK_MUTEX(bInLock, &pCreateState->mutex)
    }
    else
    {
        if (pCtxSmb1->pFile)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

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

        ntStatus = WireUnmarshallCreateFileRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pwszFilename);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildCreateState(
                            pExecContext,
                            pRequestHeader,
                            pwszFilename,
                            &pCreateState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pCreateState;
        InterlockedIncrement(&pCreateState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseCreateStateHandle;

        LWIO_LOCK_MUTEX(bInLock, &pCreateState->mutex);

        ntStatus = IoCreateFile(
                        &pCreateState->hFile,
                        pCreateState->pAcb,
                        &pCreateState->ioStatusBlock,
                        pSession->pIoSecurityContext,
                        pCreateState->pFilename,
                        pCreateState->pSecurityDescriptor,
                        pCreateState->pSecurityQOS,
                        pRequestHeader->desiredAccess,
                        pRequestHeader->allocationSize,
                        pRequestHeader->extFileAttributes,
                        pRequestHeader->shareAccess,
                        pRequestHeader->createDisposition,
                        pRequestHeader->createOptions,
                        NULL, /* EA Buffer */
                        0,    /* EA Length */
                        pCreateState->pEcpList);

        if (ntStatus == STATUS_PENDING)
        {
            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed
        }
        else if (pCreateState->pAcb && pCreateState->pAcb->CallbackContext)
        {
            SrvReleaseExecContext(pExecContext);
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = pCreateState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildNTCreateResponse_inlock(pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pCreateState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pCreateState->mutex);

        SrvReleaseCreateState(pCreateState);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildNTCreateResponse_inlock(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG ulTotalBytesUsed     = 0;
    PCREATE_RESPONSE_HEADER     pResponseHeader = NULL; // Do not free
    PSRV_CREATE_STATE_SMB_V1    pCreateState = NULL;
    FILE_BASIC_INFORMATION      fileBasicInfo = {0};
    FILE_STANDARD_INFORMATION   fileStdInfo = {0};
    FILE_PIPE_INFORMATION       filePipeInfo = {0};
    FILE_PIPE_LOCAL_INFORMATION filePipeLocalInfo = {0};
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    BOOLEAN                     bRemoveFileFromTree = FALSE;

    pCreateState = (PSRV_CREATE_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = SrvTreeCreateFile(
                    pCtxSmb1->pTree,
                    pCreateState->pwszFilename,
                    &pCreateState->hFile,
                    &pCreateState->pFilename,
                    pCreateState->pRequestHeader->desiredAccess,
                    pCreateState->pRequestHeader->allocationSize,
                    pCreateState->pRequestHeader->extFileAttributes,
                    pCreateState->pRequestHeader->shareAccess,
                    pCreateState->pRequestHeader->createDisposition,
                    pCreateState->pRequestHeader->createOptions,
                    &pCtxSmb1->pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    bRemoveFileFromTree = TRUE;

    ntStatus = IoQueryInformationFile(
                    pCtxSmb1->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileBasicInfo,
                    sizeof(fileBasicInfo),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoQueryInformationFile(
                    pCtxSmb1->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileStdInfo,
                    sizeof(fileStdInfo),
                    FileStandardInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_NT_CREATE_ANDX,
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
                        COM_NT_CREATE_ANDX,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 26;

    if (ulBytesAvailable < sizeof(CREATE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PCREATE_RESPONSE_HEADER)pOutBuffer;

    // pOutBuffer       += sizeof(CREATE_RESPONSE_HEADER);
    // ulOffset         += sizeof(CREATE_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(CREATE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(CREATE_RESPONSE_HEADER);

    pResponseHeader->fid             = pCtxSmb1->pFile->fid;
    pResponseHeader->createAction    = pCreateState->ioStatusBlock.CreateResult;
    pResponseHeader->creationTime    = fileBasicInfo.CreationTime;
    pResponseHeader->lastAccessTime  = fileBasicInfo.LastAccessTime;
    pResponseHeader->lastWriteTime   = fileBasicInfo.LastWriteTime;
    pResponseHeader->changeTime      = fileBasicInfo.ChangeTime;
    pResponseHeader->extFileAttributes = fileBasicInfo.FileAttributes;
    pResponseHeader->allocationSize    = fileStdInfo.AllocationSize;
    pResponseHeader->endOfFile         = fileStdInfo.EndOfFile;

    if (SrvTreeIsNamedPipe(pCtxSmb1->pTree))
    {
        ntStatus = IoQueryInformationFile(
                        pCtxSmb1->pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        &filePipeInfo,
                        sizeof(filePipeInfo),
                        FilePipeInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb1->pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        &filePipeLocalInfo,
                        sizeof(filePipeLocalInfo),
                        FilePipeLocalInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvMarshallPipeInfo(
                        &filePipeInfo,
                        &filePipeLocalInfo,
                        &pResponseHeader->deviceState);
        BAIL_ON_NT_STATUS(ntStatus);

        pResponseHeader->fileType = (USHORT)filePipeInfo.ReadMode;
    }
    else
    {
        pResponseHeader->fileType = 0;
        // TODO: Get these values from the driver
        pResponseHeader->deviceState = SMB_DEVICE_STATE_NO_EAS |
                                       SMB_DEVICE_STATE_NO_SUBSTREAMS |
                                       SMB_DEVICE_STATE_NO_REPARSE_TAG;
    }
    pResponseHeader->isDirectory = fileStdInfo.Directory;
    pResponseHeader->byteCount = 0;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (bRemoveFileFromTree)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvTreeRemoveFile(
                        pCtxSmb1->pTree,
                        pCtxSmb1->pFile->fid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove file from tree [Tid:%d][Fid:%d][code:%d]",
                            pCtxSmb1->pTree->tid,
                            pCtxSmb1->pFile->fid,
                            ntStatus2);
        }

        SrvFileRelease(pCtxSmb1->pFile);
        pCtxSmb1->pFile = NULL;
    }

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
SrvExecuteCreateAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V1.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);
    }
}

static
NTSTATUS
SrvBuildCreateState(
    PSRV_EXEC_CONTEXT         pExecContext,
    PCREATE_REQUEST_HEADER    pRequestHeader,
    PWSTR                     pwszFilename,
    PSRV_CREATE_STATE_SMB_V1* ppCreateState
    )
{
    NTSTATUS                   ntStatus       = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection    = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1       = pCtxProtocol->pSmb1Context;
    PSRV_CREATE_STATE_SMB_V1   pCreateState   = NULL;
    BOOLEAN                    bTreeInLock    = FALSE;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_CREATE_STATE_SMB_V1),
                    (PVOID*)&pCreateState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreateState->refCount = 1;

    // TODO: Handle root fids
    ntStatus = SrvAllocateMemory(
                    sizeof(IO_FILE_NAME),
                    (PVOID*)&pCreateState->pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pCtxSmb1->pTree->mutex);

    ntStatus = SrvBuildFilePath(
                    pCtxSmb1->pTree->pShareInfo->pwszPath,
                    pwszFilename,
                    &pCreateState->pFilename->FileName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb1->pTree->mutex);

    pCreateState->pwszFilename = pwszFilename;

    /* For named pipes, we need to pipe some extra data into the npfs driver:
     *  - Session key
     *  - Client principal name
     *  - Client address
     */
    if (SrvTreeIsNamedPipe(pCtxSmb1->pTree))
    {
        ntStatus = IoRtlEcpListAllocate(&pCreateState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeSessionKey(
                       pConnection,
                       pCreateState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSessionGetNamedPipeClientPrincipal(
                       pCtxSmb1->pSession,
                       pCreateState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeClientAddress(
                       pConnection,
                       pCreateState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pthread_mutex_init(&pCreateState->mutex, NULL);
    pCreateState->pMutex = &pCreateState->mutex;

    pCreateState->pRequestHeader = pRequestHeader;

    pCreateState->acb.Callback = &SrvExecuteCreateAsyncCB;
    pCreateState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);
    pCreateState->pAcb = &pCreateState->acb;

    *ppCreateState = pCreateState;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb1->pTree->mutex);

    return ntStatus;

error:

    *ppCreateState = NULL;

    if (pCreateState)
    {
        SrvFreeCreateState(pCreateState);
    }

    goto cleanup;
}

static
VOID
SrvReleaseCreateStateHandle(
    HANDLE hState
    )
{
    SrvReleaseCreateState((PSRV_CREATE_STATE_SMB_V1)hState);
}

static
VOID
SrvReleaseCreateState(
    PSRV_CREATE_STATE_SMB_V1 pCreateState
    )
{
    if (InterlockedDecrement(&pCreateState->refCount) == 0)
    {
        SrvFreeCreateState(pCreateState);
    }
}

static
VOID
SrvFreeCreateState(
    PSRV_CREATE_STATE_SMB_V1 pCreateState
    )
{
    if (pCreateState->pEcpList)
    {
        IoRtlEcpListFree(&pCreateState->pEcpList);
    }

    if (pCreateState->pAcb)
    {
        if (pCreateState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pCreateState->pAcb->AsyncCancelContext);
        }
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pCreateState->pFilename)
    {
        if (pCreateState->pFilename->FileName)
        {
            SrvFreeMemory(pCreateState->pFilename->FileName);
        }

        SrvFreeMemory(pCreateState->pFilename);
    }

    if (pCreateState->hFile)
    {
        IoCloseFile(pCreateState->hFile);
    }

    if (pCreateState->pMutex)
    {
        pthread_mutex_destroy(&pCreateState->mutex);
    }

    SrvFreeMemory(pCreateState);
}
