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
    PSRV_SMB_CREATE_REQUEST pCreateRequest,
    PSMB_PACKET*            ppSmbResponse
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
VOID
SrvCreateRequestExpiredCB(
    PSRV_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    );

static
NTSTATUS
SrvBuildCreateRequest(
    PLWIO_SRV_CONNECTION     pConnection,
    PLWIO_SRV_SESSION        pSession,
    PLWIO_SRV_TREE           pTree,
    PSMB_PACKET              pSmbRequest,
    PCREATE_REQUEST_HEADER   pRequestHeader,
    PWSTR                    pwszFilename,
    PSRV_SMB_CREATE_REQUEST* ppCreateRequest
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
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS                ntStatus = 0;
    PSMB_PACKET             pSmbResponse = NULL;
    PLWIO_SRV_SESSION       pSession = NULL;
    PLWIO_SRV_TREE          pTree = NULL;
    PCREATE_REQUEST_HEADER  pRequestHeader = NULL; // Do not free
    PWSTR                   pwszFilename = NULL; // Do not free
    ULONG                   ulOffset = 0;
    PSRV_SMB_CREATE_REQUEST pCreateRequest = NULL;
    BOOLEAN                 bInLock = FALSE;

    ntStatus = SrvConnectionFindSession(
                    pConnection,
                    pSmbRequest->pSMBHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree(
                    pSession,
                    pSmbRequest->pSMBHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ulOffset = (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader;

    ntStatus = WireUnmarshallCreateFileRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildCreateRequest(
                        pConnection,
                        pSession,
                        pTree,
                        pSmbRequest,
                        pRequestHeader,
                        pwszFilename,
                        &pCreateRequest);
    BAIL_ON_NT_STATUS(ntStatus);

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

    switch (ntStatus)
    {
        case STATUS_PENDING:

            {
                LONG64 llExpiry = 0LL;

                llExpiry = (time(NULL) +
                            (pCreateRequest->ulTimeout/1000) + 11644473600LL) * 10000000LL;

                ntStatus = SrvTimerPostRequest(
                                llExpiry,
                                pCreateRequest,
                                &SrvCreateRequestExpiredCB,
                                &pCreateRequest->pTimerRequest);
                BAIL_ON_NT_STATUS(ntStatus);

                InterlockedIncrement(&pCreateRequest->refCount);
            }

            break;

        case STATUS_SUCCESS:

            ntStatus = SrvBuildNTCreateResponse_inlock(
                            pCreateRequest,
                            &pSmbResponse);

            break;

        default:

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

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

    return (ntStatus);

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildNTCreateResponse_inlock(
    PSRV_SMB_CREATE_REQUEST pCreateRequest,
    PSMB_PACKET*            ppSmbResponse
    )
{
    NTSTATUS                    ntStatus = 0;
    PSMB_PACKET                 pSmbResponse = NULL;
    PCREATE_RESPONSE_HEADER     pResponseHeader = NULL;
    FILE_BASIC_INFORMATION      fileBasicInfo = {0};
    FILE_STANDARD_INFORMATION   fileStdInfo = {0};
    FILE_PIPE_INFORMATION       filePipeInfo = {0};
    FILE_PIPE_LOCAL_INFORMATION filePipeLocalInfo = {0};
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    BOOLEAN                     bRemoveFileFromTree = FALSE;

    ntStatus = SrvTreeCreateFile(
                    pCreateRequest->pTree,
                    pCreateRequest->pwszFilename,
                    &pCreateRequest->hFile,
                    &pCreateRequest->pFilename,
                    pCreateRequest->ulDesiredAccess,
                    pCreateRequest->llAllocationSize,
                    pCreateRequest->ulExtFileAttributes,
                    pCreateRequest->ulShareAccess,
                    pCreateRequest->ulCreateDisposition,
                    pCreateRequest->ulCreateOptions,
                    &pCreateRequest->pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    bRemoveFileFromTree = TRUE;

    ntStatus = IoQueryInformationFile(
                    pCreateRequest->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileBasicInfo,
                    sizeof(fileBasicInfo),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoQueryInformationFile(
                    pCreateRequest->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileStdInfo,
                    sizeof(fileStdInfo),
                    FileStandardInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    pCreateRequest->pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pCreateRequest->pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_NT_CREATE_ANDX,
                0,
                TRUE,
                pCreateRequest->pTree->tid,
                pCreateRequest->usPid,
                pCreateRequest->pSession->uid,
                pCreateRequest->usMid,
                pCreateRequest->pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 26;

    pResponseHeader = (PCREATE_RESPONSE_HEADER)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(CREATE_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(CREATE_RESPONSE_HEADER);

    pResponseHeader->fid = pCreateRequest->pFile->fid;
    pResponseHeader->createAction = pCreateRequest->ioStatusBlock.CreateResult;
    pResponseHeader->creationTime = fileBasicInfo.CreationTime;
    pResponseHeader->lastAccessTime = fileBasicInfo.LastAccessTime;
    pResponseHeader->lastWriteTime = fileBasicInfo.LastWriteTime;
    pResponseHeader->changeTime = fileBasicInfo.ChangeTime;
    pResponseHeader->extFileAttributes = fileBasicInfo.FileAttributes;
    pResponseHeader->allocationSize = fileStdInfo.AllocationSize;
    pResponseHeader->endOfFile = fileStdInfo.EndOfFile;

    if (SrvTreeIsNamedPipe(pCreateRequest->pTree))
    {
        ntStatus = IoQueryInformationFile(
                        pCreateRequest->pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        &filePipeInfo,
                        sizeof(filePipeInfo),
                        FilePipeInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoQueryInformationFile(
                        pCreateRequest->pFile->hFile,
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

    ntStatus = SMBPacketUpdateAndXOffset(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (bRemoveFileFromTree)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvTreeRemoveFile(
                        pCreateRequest->pTree,
                        pCreateRequest->pFile->fid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove file from tree [Tid:%d][Fid:%d][code:%d]",
                           pCreateRequest->pTree->tid,
                           pCreateRequest->pFile->fid,
                           ntStatus2);
        }
    }

    if (pSmbResponse)
    {
        SMBPacketFree(
                pCreateRequest->pConnection->hPacketAllocator,
                pSmbResponse);
    }

    goto cleanup;
}

static
VOID
SrvExecuteCreateAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SMB_CREATE_REQUEST pCreateRequest = (PSRV_SMB_CREATE_REQUEST)pContext;
    BOOLEAN bInLock = FALSE;
    PSMB_PACKET pSmbResponse = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pCreateRequest->mutex);

    if (!pCreateRequest->bExpired && !pCreateRequest->bResponseSent)
    {
        if (pCreateRequest->ioStatusBlock.Status == STATUS_SUCCESS)
        {
            ntStatus = SrvBuildNTCreateResponse_inlock(
                                pCreateRequest,
                                &pSmbResponse);
        }
        else
        {
            ntStatus = SrvProtocolBuildErrorResponse(
                            pCreateRequest->pConnection,
                            COM_NT_CREATE_ANDX,
                            pCreateRequest->pTree->tid,
                            pCreateRequest->usPid,
                            pCreateRequest->pSession->uid,
                            pCreateRequest->usMid,
                            pCreateRequest->ioStatusBlock.Status,
                            &pSmbResponse);
        }
        BAIL_ON_NT_STATUS(ntStatus);

        pSmbResponse->sequence = pCreateRequest->ulResponseSequence;

        ntStatus = SrvTransportSendResponse(
                            pCreateRequest->pConnection,
                            pSmbResponse);
        BAIL_ON_NT_STATUS(ntStatus);

        pCreateRequest->bResponseSent = TRUE;
    }

cleanup:

    if (pCreateRequest->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pCreateRequest->pAcb->AsyncCancelContext);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pCreateRequest->mutex);

    if (pSmbResponse)
    {
        SMBPacketFree(
                pCreateRequest->pConnection->hPacketAllocator,
                pSmbResponse);
    }

    SrvReleaseCreateRequest(pCreateRequest);

    return;

error:

    goto cleanup;
}

static
VOID
SrvCreateRequestExpiredCB(
    PSRV_TIMER_REQUEST pTimerRequest,
    PVOID              pUserData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SMB_CREATE_REQUEST pCreateRequest = (PSRV_SMB_CREATE_REQUEST)pUserData;
    BOOLEAN bInLock = FALSE;
    PSMB_PACKET pSmbResponse = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pCreateRequest->mutex);

    pCreateRequest->bExpired = TRUE;

    if (!pCreateRequest->bResponseSent)
    {
        // The timer expired first

        ntStatus = SrvProtocolBuildErrorResponse(
                        pCreateRequest->pConnection,
                        COM_NT_CREATE_ANDX,
                        pCreateRequest->pTree->tid,
                        pCreateRequest->usPid,
                        pCreateRequest->pSession->uid,
                        pCreateRequest->usMid,
                        STATUS_IO_TIMEOUT,
                        &pSmbResponse);
        BAIL_ON_NT_STATUS(ntStatus);

        pSmbResponse->sequence = pCreateRequest->ulResponseSequence;

        ntStatus = SrvTransportSendResponse(
                        pCreateRequest->pConnection,
                        pSmbResponse);
        BAIL_ON_NT_STATUS(ntStatus);

        pCreateRequest->bResponseSent = TRUE;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pCreateRequest->mutex);

    if (pSmbResponse)
    {
        SMBPacketFree(
                pCreateRequest->pConnection->hPacketAllocator,
                pSmbResponse);
    }

    SrvReleaseCreateRequest(pCreateRequest);

    return;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildCreateRequest(
    PLWIO_SRV_CONNECTION     pConnection,
    PLWIO_SRV_SESSION        pSession,
    PLWIO_SRV_TREE           pTree,
    PSMB_PACKET              pSmbRequest,
    PCREATE_REQUEST_HEADER   pRequestHeader,
    PWSTR                    pwszFilename,
    PSRV_SMB_CREATE_REQUEST* ppCreateRequest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SMB_CREATE_REQUEST pCreateRequest = NULL;
    BOOLEAN bTreeInLock = FALSE;

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

    LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pTree->mutex);

    ntStatus = SrvBuildFilePath(
                    pTree->pShareInfo->pwszPath,
                    pwszFilename,
                    &pCreateRequest->pFilename->FileName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTree->mutex);

    ntStatus = SrvAllocateStringW(pwszFilename, &pCreateRequest->pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    /* For named pipes, we need to pipe some extra data into the npfs driver:
     *  - Session key
     *  - Client principal name
     *  - Client address
     */
    if (SrvTreeIsNamedPipe(pTree))
    {
        ntStatus = IoRtlEcpListAllocate(&pCreateRequest->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeSessionKey(
                       pConnection,
                       pCreateRequest->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSessionGetNamedPipeClientPrincipal(
                       pSession,
                       pCreateRequest->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeClientAddress(
                       pConnection,
                       pCreateRequest->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pthread_mutex_init(&pCreateRequest->mutex, NULL);
    pCreateRequest->pMutex = &pCreateRequest->mutex;

    pCreateRequest->pConnection = pConnection;
    InterlockedIncrement(&pConnection->refCount);

    pCreateRequest->pSession = pSession;
    InterlockedIncrement(&pSession->refcount);

    pCreateRequest->pTree = pTree;
    InterlockedIncrement(&pTree->refcount);

    pCreateRequest->ulDesiredAccess     = pRequestHeader->desiredAccess;
    pCreateRequest->llAllocationSize    = pRequestHeader->allocationSize;
    pCreateRequest->ulExtFileAttributes = pRequestHeader->extFileAttributes;
    pCreateRequest->ulShareAccess       = pRequestHeader->shareAccess;
    pCreateRequest->ulCreateDisposition = pRequestHeader->createDisposition;
    pCreateRequest->ulCreateOptions     = pRequestHeader->createOptions;

    pCreateRequest->usPid = pSmbRequest->pSMBHeader->pid;
    pCreateRequest->usMid = pSmbRequest->pSMBHeader->mid;
    pCreateRequest->ulResponseSequence = pSmbRequest->sequence + 1;

    pCreateRequest->bExpired = FALSE;
    pCreateRequest->bResponseSent = FALSE;

    pCreateRequest->ulTimeout = LWIO_SRV_DEFAULT_TIMEOUT_MSECS;

    pCreateRequest->acb.Callback = &SrvExecuteCreateAsyncCB;
    pCreateRequest->acb.CallbackContext = pCreateRequest;

    if (pCreateRequest->ulTimeout)
    {
        pCreateRequest->pAcb = &pCreateRequest->acb;
    }

    *ppCreateRequest = pCreateRequest;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTree->mutex);

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
    if (pCreateRequest->pConnection)
    {
        SrvConnectionRelease(pCreateRequest->pConnection);
    }

    if (pCreateRequest->pSession)
    {
        SrvSessionRelease(pCreateRequest->pSession);
    }

    if (pCreateRequest->pTree)
    {
        SrvTreeRelease(pCreateRequest->pTree);
    }

    if (pCreateRequest->pFile)
    {
        SrvFileRelease(pCreateRequest->pFile);
    }

    if (pCreateRequest->pEcpList)
    {
        IoRtlEcpListFree(&pCreateRequest->pEcpList);
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

    if (pCreateRequest->pwszFilename)
    {
        SrvFreeMemory(pCreateRequest->pwszFilename);
    }

    if (pCreateRequest->pMutex)
    {
        pthread_mutex_destroy(&pCreateRequest->mutex);
    }

    if (pCreateRequest->pTimerRequest)
    {
        SrvTimerCancelRequest(pCreateRequest->pTimerRequest);
        SrvTimerRelease(pCreateRequest->pTimerRequest);
    }

    SrvFreeMemory(pCreateRequest);
}
