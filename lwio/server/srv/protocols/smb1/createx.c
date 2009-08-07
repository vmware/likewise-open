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
SrvBuildCreateRequest(
    PSRV_EXEC_CONTEXT        pExecContext,
    PCREATE_REQUEST_HEADER   pRequestHeader,
    PWSTR                    pwszFilename,
    PSRV_SMB_CREATE_REQUEST* ppCreateRequest
    );

static
VOID
SrvReleaseCreateState(
    HANDLE hState
    );

static
VOID
SrvReleaseCreateRequest(
    PSRV_SMB_CREATE_REQUEST pCreateRequest
    );

static
VOID
SrvFreeCreateRequest(
    PSRV_SMB_CREATE_REQUEST pCreateRequest
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
    PSRV_SMB_CREATE_REQUEST    pCreateRequest = NULL;
    BOOLEAN                    bInLock = FALSE;

    pCreateRequest = (PSRV_SMB_CREATE_REQUEST)pCtxSmb1->hState;

    if (pCreateRequest)
    {
        InterlockedIncrement(&pCreateRequest->refCount);

        LWIO_LOCK_MUTEX(bInLock, &pCreateRequest->mutex)
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

        ntStatus = SrvBuildCreateRequest(
                            pExecContext,
                            pRequestHeader,
                            pwszFilename,
                            &pCreateRequest);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pCreateRequest;
        InterlockedIncrement(&pCreateRequest->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseCreateState;

        LWIO_LOCK_MUTEX(bInLock, &pCreateRequest->mutex);

        ntStatus = IoCreateFile(
                        &pCreateRequest->hFile,
                        pCreateRequest->pAcb,
                        &pCreateRequest->ioStatusBlock,
                        pSession->pIoSecurityContext,
                        pCreateRequest->pFilename,
                        pCreateRequest->pSecurityDescriptor,
                        pCreateRequest->pSecurityQOS,
                        pRequestHeader->desiredAccess,
                        pRequestHeader->allocationSize,
                        pRequestHeader->extFileAttributes,
                        pRequestHeader->shareAccess,
                        pRequestHeader->createDisposition,
                        pRequestHeader->createOptions,
                        NULL, /* EA Buffer */
                        0,    /* EA Length */
                        pCreateRequest->pEcpList);

        if (ntStatus == STATUS_PENDING)
        {
            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed
        }
        else if (pCreateRequest->pAcb && pCreateRequest->pAcb->CallbackContext)
        {
            SrvReleaseExecContext(pExecContext);
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = pCreateRequest->ioStatusBlock.Status;
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

    LWIO_UNLOCK_MUTEX(bInLock, &pCreateRequest->mutex);

    if (pCreateRequest)
    {
        SrvReleaseCreateRequest(pCreateRequest);
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
    PSRV_SMB_CREATE_REQUEST     pCreateRequest = NULL;
    FILE_BASIC_INFORMATION      fileBasicInfo = {0};
    FILE_STANDARD_INFORMATION   fileStdInfo = {0};
    FILE_PIPE_INFORMATION       filePipeInfo = {0};
    FILE_PIPE_LOCAL_INFORMATION filePipeLocalInfo = {0};
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    BOOLEAN                     bRemoveFileFromTree = FALSE;

    pCreateRequest = (PSRV_SMB_CREATE_REQUEST)pCtxSmb1->hState;

    ntStatus = SrvTreeCreateFile(
                    pCtxSmb1->pTree,
                    pCreateRequest->pwszFilename,
                    &pCreateRequest->hFile,
                    &pCreateRequest->pFilename,
                    pCreateRequest->ulDesiredAccess,
                    pCreateRequest->llAllocationSize,
                    pCreateRequest->ulExtFileAttributes,
                    pCreateRequest->ulShareAccess,
                    pCreateRequest->ulCreateDisposition,
                    pCreateRequest->ulCreateOptions,
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

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_NT_CREATE_ANDX,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
                    pSmbRequest->pHeader->pid,
                    pCtxSmb1->pSession->uid,
                    pSmbRequest->pHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 26;

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

    pResponseHeader->fid = pCtxSmb1->pFile->fid;
    pResponseHeader->createAction = pCreateRequest->ioStatusBlock.CreateResult;
    pResponseHeader->creationTime = fileBasicInfo.CreationTime;
    pResponseHeader->lastAccessTime = fileBasicInfo.LastAccessTime;
    pResponseHeader->lastWriteTime = fileBasicInfo.LastWriteTime;
    pResponseHeader->changeTime = fileBasicInfo.ChangeTime;
    pResponseHeader->extFileAttributes = fileBasicInfo.FileAttributes;
    pResponseHeader->allocationSize = fileStdInfo.AllocationSize;
    pResponseHeader->endOfFile = fileStdInfo.EndOfFile;

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
SrvBuildCreateRequest(
    PSRV_EXEC_CONTEXT        pExecContext,
    PCREATE_REQUEST_HEADER   pRequestHeader,
    PWSTR                    pwszFilename,
    PSRV_SMB_CREATE_REQUEST* ppCreateRequest
    )
{
    NTSTATUS                   ntStatus       = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection    = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1       = pCtxProtocol->pSmb1Context;
    PSRV_SMB_CREATE_REQUEST    pCreateRequest = NULL;
    BOOLEAN                    bTreeInLock    = FALSE;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_SMB_CREATE_REQUEST),
                    (PVOID*)&pCreateRequest);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreateRequest->refCount = 1;

    // TODO: Handle root fids
    ntStatus = SrvAllocateMemory(
                    sizeof(IO_FILE_NAME),
                    (PVOID*)&pCreateRequest->pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pCtxSmb1->pTree->mutex);

    ntStatus = SrvBuildFilePath(
                    pCtxSmb1->pTree->pShareInfo->pwszPath,
                    pwszFilename,
                    &pCreateRequest->pFilename->FileName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb1->pTree->mutex);

    pCreateRequest->pwszFilename = pwszFilename;

    /* For named pipes, we need to pipe some extra data into the npfs driver:
     *  - Session key
     *  - Client principal name
     *  - Client address
     */
    if (SrvTreeIsNamedPipe(pCtxSmb1->pTree))
    {
        ntStatus = IoRtlEcpListAllocate(&pCreateRequest->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeSessionKey(
                       pConnection,
                       pCreateRequest->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSessionGetNamedPipeClientPrincipal(
                       pCtxSmb1->pSession,
                       pCreateRequest->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeClientAddress(
                       pConnection,
                       pCreateRequest->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pthread_mutex_init(&pCreateRequest->mutex, NULL);
    pCreateRequest->pMutex = &pCreateRequest->mutex;

    pCreateRequest->ulDesiredAccess     = pRequestHeader->desiredAccess;
    pCreateRequest->llAllocationSize    = pRequestHeader->allocationSize;
    pCreateRequest->ulExtFileAttributes = pRequestHeader->extFileAttributes;
    pCreateRequest->ulShareAccess       = pRequestHeader->shareAccess;
    pCreateRequest->ulCreateDisposition = pRequestHeader->createDisposition;
    pCreateRequest->ulCreateOptions     = pRequestHeader->createOptions;

    pCreateRequest->acb.Callback = &SrvExecuteCreateAsyncCB;
    pCreateRequest->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);
    pCreateRequest->pAcb = &pCreateRequest->acb;

    *ppCreateRequest = pCreateRequest;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pCtxSmb1->pTree->mutex);

    return ntStatus;

error:

    *ppCreateRequest = NULL;

    if (pCreateRequest)
    {
        SrvFreeCreateRequest(pCreateRequest);
    }

    goto cleanup;
}

static
VOID
SrvReleaseCreateState(
    HANDLE hState
    )
{
    SrvReleaseCreateRequest((PSRV_SMB_CREATE_REQUEST)hState);
}

static
VOID
SrvReleaseCreateRequest(
    PSRV_SMB_CREATE_REQUEST pCreateRequest
    )
{
    if (InterlockedDecrement(&pCreateRequest->refCount) == 0)
    {
        SrvFreeCreateRequest(pCreateRequest);
    }
}

static
VOID
SrvFreeCreateRequest(
    PSRV_SMB_CREATE_REQUEST pCreateRequest
    )
{
    if (pCreateRequest->pEcpList)
    {
        IoRtlEcpListFree(&pCreateRequest->pEcpList);
    }

    if (pCreateRequest->pAcb)
    {
        if (pCreateRequest->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pCreateRequest->pAcb->AsyncCancelContext);
        }
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pCreateRequest->pFilename)
    {
        if (pCreateRequest->pFilename->FileName)
        {
            SrvFreeMemory(pCreateRequest->pFilename->FileName);
        }

        SrvFreeMemory(pCreateRequest->pFilename);
    }

    if (pCreateRequest->hFile)
    {
        IoCloseFile(pCreateRequest->hFile);
    }

    if (pCreateRequest->pMutex)
    {
        pthread_mutex_destroy(&pCreateRequest->mutex);
    }

    SrvFreeMemory(pCreateRequest);
}
