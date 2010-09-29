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
SrvQueryFileInformation_inlock(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvRequestCreateXOplocks(
    PSRV_EXEC_CONTEXT pExecContext
    );

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
VOID
SrvLogCreateState_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
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
SrvPrepareCreateStateAsync(
    PSRV_CREATE_STATE_SMB_V1 pCreateState,
    PSRV_EXEC_CONTEXT        pExecContext
    );

static
VOID
SrvReleaseCreateStateAsync(
    PSRV_CREATE_STATE_SMB_V1 pCreateState
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
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;
    PSRV_CREATE_STATE_SMB_V1   pCreateState = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pCreateState = (PSRV_CREATE_STATE_SMB_V1)pCtxSmb1->hState;

    if (pCreateState)
    {
        InterlockedIncrement(&pCreateState->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PCREATE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PWSTR                  pwszFilename = NULL;   // Do not free

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

        ntStatus = SrvSetStatSessionInfo(pExecContext, pSession);
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
    }

    LWIO_LOCK_MUTEX(bInLock, &pCreateState->mutex);

    switch (pCreateState->stage)
    {
        case SRV_CREATE_STAGE_SMB_V1_INITIAL:

            SRV_LOG_CALL_DEBUG(
                    pExecContext->pLogContext,
                    SMB_PROTOCOL_VERSION_1,
                    pCtxSmb1->pRequests[pCtxSmb1->iMsg].pHeader->command,
                    &SrvLogCreateState_SMB_V1,
                    pCreateState);

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V1_CREATE_FILE_COMPLETED;

            SrvPrepareCreateStateAsync(pCreateState, pExecContext);

            ntStatus = SrvIoCreateFile(
                            pCreateState->pTree->pShareInfo,
                            &pCreateState->hFile,
                            pCreateState->pAcb,
                            &pCreateState->ioStatusBlock,
                            pSession->pIoSecurityContext,
                            pCreateState->pFilename,
                            pCreateState->pSecurityDescriptor,
                            pCreateState->pSecurityQOS,
                            pCreateState->pRequestHeader->desiredAccess,
                            pCreateState->pRequestHeader->allocationSize,
                            pCreateState->pRequestHeader->extFileAttributes,
                            pCreateState->pRequestHeader->shareAccess,
                            pCreateState->pRequestHeader->createDisposition,
                            pCreateState->pRequestHeader->createOptions,
                            NULL, /* EA Buffer */
                            0,    /* EA Length */
                            pCreateState->pEcpList);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseCreateStateAsync(pCreateState); // completed synchronously

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V1_CREATE_FILE_COMPLETED:

            ntStatus = pCreateState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pCreateState->ulCreateAction =
                            pCreateState->ioStatusBlock.CreateResult;

            ntStatus = SrvTreeCreateFile(
                            pCreateState->pTree,
                            pCreateState->pwszFilename,
                            &pCreateState->hFile,
                            &pCreateState->pFilename,
                            pCreateState->pRequestHeader->desiredAccess,
                            pCreateState->pRequestHeader->allocationSize,
                            pCreateState->pRequestHeader->extFileAttributes,
                            pCreateState->pRequestHeader->shareAccess,
                            pCreateState->pRequestHeader->createDisposition,
                            pCreateState->pRequestHeader->createOptions,
                            &pCreateState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pCreateState->pFile->pfnCancelAsyncOperationsFile = SrvCancelFileAsyncOperations;

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V1_ATTEMPT_QUERY_INFO;

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V1_ATTEMPT_QUERY_INFO:

            ntStatus = SrvQueryFileInformation_inlock(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V1_REQUEST_OPLOCK;

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V1_REQUEST_OPLOCK:

            ntStatus = SrvRequestCreateXOplocks(pExecContext);
            // We don't fail to if the oplock cannot be granted

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V1_QUERY_INFO_COMPLETED;

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V1_QUERY_INFO_COMPLETED:

            ntStatus = pCreateState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            if (!pCreateState->pFile->hCancellableBRLStateList)
            {
                PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList = NULL;

                ntStatus = SrvCreatePendingLockStateList(&pBRLStateList);
                BAIL_ON_NT_STATUS(ntStatus);

                pCreateState->pFile->hCancellableBRLStateList =
                                (HANDLE)pBRLStateList;

                pCreateState->pFile->pfnFreeBRLStateList =
                                &SrvFreePendingLockStateListHandle;
            }

            ntStatus = SrvBuildNTCreateResponse_inlock(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvElementsRegisterResource(
                            &pCreateState->pFile->resource,
                            NULL);
            BAIL_ON_NT_STATUS(ntStatus);

            pCreateState->stage = SRV_CREATE_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_CREATE_STAGE_SMB_V1_DONE:

            if (pCreateState->pRequestHeader->desiredAccess & FILE_READ_DATA)
            {
                pCreateState->pFile->ulPermissions |= SRV_PERM_FILE_READ;
            }
            if (pCreateState->pRequestHeader->desiredAccess & FILE_WRITE_DATA)
            {
                pCreateState->pFile->ulPermissions |= SRV_PERM_FILE_WRITE;
            }
            if (pCreateState->ulCreateAction == FILE_CREATED)
            {
                pCreateState->pFile->ulPermissions |= SRV_PERM_FILE_CREATE;
            }

            // transfer file so we do not run it down
            pCtxSmb1->pFile = pCreateState->pFile;
            pCreateState->pFile = NULL;

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

    if (pCreateState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pCreateState->mutex);

        SrvReleaseCreateState(pCreateState);
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

            if (pCreateState)
            {
                SrvReleaseCreateStateAsync(pCreateState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvQueryFileInformation_inlock(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_CREATE_STATE_SMB_V1    pCreateState = NULL;

    pCreateState = (PSRV_CREATE_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pCreateState->pFileBasicInfo)
    {
        pCreateState->pFileBasicInfo = &pCreateState->fileBasicInfo;

        SrvPrepareCreateStateAsync(pCreateState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCreateState->pFile->hFile,
                        pCreateState->pAcb,
                        &pCreateState->ioStatusBlock,
                        pCreateState->pFileBasicInfo,
                        sizeof(pCreateState->fileBasicInfo),
                        FileBasicInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseCreateStateAsync(pCreateState); // completed synchronously
    }

    if (!(pCreateState->pFileBasicInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        SrvFileBlockIdleTimeout(pCreateState->pFile);
    }
    else
    {
        SrvFileUnblockIdleTimeout(pCreateState->pFile);
    }

    if (!pCreateState->pFileStdInfo)
    {
        pCreateState->pFileStdInfo = &pCreateState->fileStdInfo;

        SrvPrepareCreateStateAsync(pCreateState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pCreateState->pFile->hFile,
                        pCreateState->pAcb,
                        &pCreateState->ioStatusBlock,
                        pCreateState->pFileStdInfo,
                        sizeof(pCreateState->fileStdInfo),
                        FileStandardInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseCreateStateAsync(pCreateState); // completed synchronously
    }

    if (SrvTreeIsNamedPipe(pCreateState->pTree))
    {
        if (!pCreateState->pFilePipeInfo)
        {
            pCreateState->pFilePipeInfo = &pCreateState->filePipeInfo;

            SrvPrepareCreateStateAsync(pCreateState, pExecContext);

            ntStatus = IoQueryInformationFile(
                            pCreateState->pFile->hFile,
                            pCreateState->pAcb,
                            &pCreateState->ioStatusBlock,
                            pCreateState->pFilePipeInfo,
                            sizeof(pCreateState->filePipeInfo),
                            FilePipeInformation);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseCreateStateAsync(pCreateState); // completed synchronously
        }

        if (!pCreateState->pFilePipeLocalInfo)
        {
            pCreateState->pFilePipeLocalInfo = &pCreateState->filePipeLocalInfo;

            SrvPrepareCreateStateAsync(pCreateState, pExecContext);

            ntStatus = IoQueryInformationFile(
                            pCreateState->pFile->hFile,
                            pCreateState->pAcb,
                            &pCreateState->ioStatusBlock,
                            pCreateState->pFilePipeLocalInfo,
                            sizeof(pCreateState->filePipeLocalInfo),
                            FilePipeLocalInformation);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseCreateStateAsync(pCreateState); // completed synchronously
        }
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvRequestCreateXOplocks(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS        ntStatus           = STATUS_SUCCESS;
    SRV_OPLOCK_INFO batchOplockChain[] =
            {
               { IO_OPLOCK_REQUEST_OPLOCK_BATCH,   SMB_OPLOCK_LEVEL_BATCH },
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2, SMB_OPLOCK_LEVEL_II    },
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    SRV_OPLOCK_INFO exclOplockChain[] =
            {
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1, SMB_OPLOCK_LEVEL_I     },
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2, SMB_OPLOCK_LEVEL_II    },
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    SRV_OPLOCK_INFO noOplockChain[] =
            {
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    PSRV_OPLOCK_INFO           pOplockCursor = NULL;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1      = pCtxProtocol->pSmb1Context;
    PSRV_CREATE_STATE_SMB_V1   pCreateState  = NULL;
    PSRV_OPLOCK_STATE_SMB_V1   pOplockState  = NULL;
    BOOLEAN                    bContinue     = TRUE;

    pCreateState = (PSRV_CREATE_STATE_SMB_V1)pCtxSmb1->hState;

    if (SrvTreeIsNamedPipe(pCreateState->pTree) ||
        pCreateState->fileStdInfo.Directory)
    {
        pOplockCursor = &noOplockChain[0];

        goto done;
    }

    ntStatus = SrvBuildOplockState(
                    pExecContext->pConnection,
                    pCtxSmb1->pSession,
                    pCtxSmb1->pTree,
                    pCreateState->pFile,
                    &pOplockState);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pCreateState->pRequestHeader->flags & SMB_OPLOCK_REQUEST_BATCH)
    {
        pOplockCursor = &batchOplockChain[0];
    }
    else if (pCreateState->pRequestHeader->flags & SMB_OPLOCK_REQUEST_EXCLUSIVE)
    {
        pOplockCursor = &exclOplockChain[0];
    }
    else
    {
        pOplockCursor = &noOplockChain[0];
    }

    while (bContinue && (pOplockCursor->oplockRequest != SMB_OPLOCK_LEVEL_NONE))
    {
        pOplockState->oplockBuffer_in.OplockRequestType =
                        pOplockCursor->oplockRequest;

        SrvPrepareOplockStateAsync(pOplockState);

        ntStatus = IoFsControlFile(
                        pCreateState->pFile->hFile,
                        pOplockState->pAcb,
                        &pOplockState->ioStatusBlock,
                        IO_FSCTL_OPLOCK_REQUEST,
                        &pOplockState->oplockBuffer_in,
                        sizeof(pOplockState->oplockBuffer_in),
                        &pOplockState->oplockBuffer_out,
                        sizeof(pOplockState->oplockBuffer_out));
        switch (ntStatus)
        {
            case STATUS_OPLOCK_NOT_GRANTED:

                SrvReleaseOplockStateAsync(pOplockState); // completed sync

                pOplockCursor++;

                break;

            case STATUS_PENDING:

                InterlockedIncrement(&pOplockState->refCount);

                ntStatus = SrvFileSetOplockState(
                               pCreateState->pFile,
                               pOplockState,
                               &SrvCancelOplockStateHandle,
                               &SrvReleaseOplockStateHandle);
                if (ntStatus != STATUS_SUCCESS)
                {
                    InterlockedDecrement(&pOplockState->refCount);
                }
                BAIL_ON_NT_STATUS(ntStatus);

                SrvFileSetOplockLevel(
                        pCreateState->pFile,
                        pOplockCursor->oplockLevel);

                ntStatus = STATUS_SUCCESS;

                bContinue = FALSE;

                break;

            default:

                SrvReleaseOplockStateAsync(pOplockState); // completed sync

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }
    }

done:

    pCreateState->ucOplockLevel = pOplockCursor->oplockLevel;

cleanup:

    if (pOplockState)
    {
        SrvReleaseOplockState(pOplockState);
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
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
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

    pCreateState = (PSRV_CREATE_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_NT_CREATE_ANDX,
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

    *pSmbResponse->pWordCount = 34;

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

    pResponseHeader->oplockLevel     = pCreateState->ucOplockLevel;
    pResponseHeader->fid             = pCreateState->pFile->fid;
    pResponseHeader->createAction    = pCreateState->ulCreateAction;
    pResponseHeader->creationTime    = pCreateState->fileBasicInfo.CreationTime;
    pResponseHeader->lastAccessTime  = pCreateState->fileBasicInfo.LastAccessTime;
    pResponseHeader->lastWriteTime   = pCreateState->fileBasicInfo.LastWriteTime;
    pResponseHeader->changeTime      = pCreateState->fileBasicInfo.ChangeTime;
    pResponseHeader->extFileAttributes = pCreateState->fileBasicInfo.FileAttributes;
    pResponseHeader->allocationSize    = pCreateState->fileStdInfo.AllocationSize;
    pResponseHeader->endOfFile         = pCreateState->fileStdInfo.EndOfFile;

    if (SrvTreeIsNamedPipe(pCtxSmb1->pTree))
    {
        ntStatus = SrvMarshallPipeInfo(
                        &pCreateState->filePipeInfo,
                        &pCreateState->filePipeLocalInfo,
                        &pResponseHeader->deviceState);
        BAIL_ON_NT_STATUS(ntStatus);

        pResponseHeader->fileType = (USHORT)pCreateState->filePipeInfo.ReadMode;
    }
    else
    {
        pResponseHeader->fileType = 0;
        // TODO: Get these values from the driver
        pResponseHeader->deviceState = SMB_DEVICE_STATE_NO_EAS |
                                       SMB_DEVICE_STATE_NO_SUBSTREAMS |
                                       SMB_DEVICE_STATE_NO_REPARSE_TAG;
    }

    pResponseHeader->isDirectory = pCreateState->fileStdInfo.Directory;
    pResponseHeader->byteCount = 0;

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
SrvExecuteCreateAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_CREATE_STATE_SMB_V1   pCreateState     = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pCreateState = (PSRV_CREATE_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pCreateState->mutex);

    if (pCreateState->pAcb && pCreateState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pCreateState->pAcb->AsyncCancelContext);
    }

    pCreateState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pCreateState->mutex);

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
SrvLogCreateState_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_CREATE_STATE_SMB_V1 pCreateState = NULL;
    PSTR pszPath = NULL;
    va_list msgList;

    va_start(msgList, ulLine);

    pCreateState = va_arg(msgList, PSRV_CREATE_STATE_SMB_V1);

    if (pCreateState)
    {
        if (pCreateState->pFilename->FileName)
        {
            ntStatus = SrvWc16sToMbs(pCreateState->pFilename->FileName, &pszPath);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (logLevel >= LWIO_LOG_LEVEL_DEBUG)
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "[%s() %s:%u] CreateAndX state: "
                    "alloc-size(%llu),create-disp(0x%x),desired-access(0x%x),"
                    "create-options(0x%x),share-access(0x%x),flags(0x%x),"
                    "security-flags(0x%x),root-dir-fid(%u),path(%s)",
                    LWIO_SAFE_LOG_STRING(pszFunction),
                    LWIO_SAFE_LOG_STRING(pszFile),
                    ulLine,
                    pCreateState->pRequestHeader->allocationSize,
                    pCreateState->pRequestHeader->createDisposition,
                    pCreateState->pRequestHeader->desiredAccess,
                    pCreateState->pRequestHeader->createOptions,
                    pCreateState->pRequestHeader->shareAccess,
                    pCreateState->pRequestHeader->flags,
                    pCreateState->pRequestHeader->securityFlags,
                    pCreateState->pRequestHeader->rootDirectoryFid,
                    LWIO_SAFE_LOG_STRING(pszPath));
        }
        else
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "CreateAndX state: "
                    "alloc-size(%llu),create-disp(0x%x),desired-access(0x%x),"
                    "create-options(0x%x),share-access(0x%x),flags(0x%x),"
                    "security-flags(0x%x),root-dir-fid(%u),path(%s)",
                    pCreateState->pRequestHeader->allocationSize,
                    pCreateState->pRequestHeader->createDisposition,
                    pCreateState->pRequestHeader->desiredAccess,
                    pCreateState->pRequestHeader->createOptions,
                    pCreateState->pRequestHeader->shareAccess,
                    pCreateState->pRequestHeader->flags,
                    pCreateState->pRequestHeader->securityFlags,
                    pCreateState->pRequestHeader->rootDirectoryFid,
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

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_CREATE_STATE_SMB_V1),
                    (PVOID*)&pCreateState);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreateState->refCount = 1;

    pthread_mutex_init(&pCreateState->mutex, NULL);
    pCreateState->pMutex = &pCreateState->mutex;

    pCreateState->stage = SRV_CREATE_STAGE_SMB_V1_INITIAL;

    ntStatus = SrvAllocateMemory(
                    sizeof(IO_FILE_NAME),
                    (PVOID*)&pCreateState->pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pCreateState->pTree = SrvTreeAcquire(pCtxSmb1->pTree);

    if (pRequestHeader->rootDirectoryFid != 0)
    {
        ntStatus = SrvTreeFindFile(
                        pCreateState->pTree,
                        pRequestHeader->rootDirectoryFid,
                        &pCreateState->pRootDirectory);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pwszFilename && *pwszFilename)
        {
            wchar16_t wszFwdSlash[]  = {'/',  0};
            wchar16_t wszBackSlash[] = {'\\', 0};

            if ((*pwszFilename == wszFwdSlash[0]) ||
                (*pwszFilename == wszBackSlash[0]))
            {
                ntStatus = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = SrvBuildFilePath(
                            NULL, /* relative path */
                            pwszFilename,
                            &pCreateState->pFilename->FileName);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pCreateState->pFilename->RootFileHandle =
                        pCreateState->pRootDirectory->hFile;
    }
    else
    {
        ntStatus = SrvBuildTreeRelativePath(
                        pCtxSmb1->pTree,
                        pwszFilename,
                        pCreateState->pFilename);
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

        ntStatus = SrvConnectionGetNamedPipeClientAddress(
                       pConnection,
                       pCreateState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pCreateState->pRequestHeader = pRequestHeader;

    *ppCreateState = pCreateState;

cleanup:

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
SrvPrepareCreateStateAsync(
    PSRV_CREATE_STATE_SMB_V1 pCreateState,
    PSRV_EXEC_CONTEXT        pExecContext
    )
{
    pCreateState->acb.Callback        = &SrvExecuteCreateAsyncCB;

    pCreateState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pCreateState->acb.AsyncCancelContext = NULL;

    pCreateState->pAcb = &pCreateState->acb;
}

static
VOID
SrvReleaseCreateStateAsync(
    PSRV_CREATE_STATE_SMB_V1 pCreateState
    )
{
    if (pCreateState->pAcb)
    {
        pCreateState->acb.Callback        = NULL;

        if (pCreateState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pCreateState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pCreateState->pAcb->CallbackContext = NULL;
        }

        if (pCreateState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pCreateState->pAcb->AsyncCancelContext);
        }

        pCreateState->pAcb = NULL;
    }
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
    if (pCreateState->pAcb && pCreateState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pCreateState->pAcb->AsyncCancelContext);
    }

    if (pCreateState->pEcpList)
    {
        IoRtlEcpListFree(&pCreateState->pEcpList);
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

    if (pCreateState->pFile)
    {
        SrvFileResetOplockState(pCreateState->pFile);
        SrvFileRundown(pCreateState->pFile);
        SrvFileRelease(pCreateState->pFile);
    }

    if (pCreateState->pRootDirectory)
    {
        SrvFileRelease(pCreateState->pRootDirectory);
    }

    if (pCreateState->pTree)
    {
        SrvTreeRelease(pCreateState->pTree);
    }

    if (pCreateState->pMutex)
    {
        pthread_mutex_destroy(&pCreateState->mutex);
    }

    SrvFreeMemory(pCreateState);
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
