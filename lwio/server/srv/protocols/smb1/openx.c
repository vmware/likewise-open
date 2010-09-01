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
SrvBuildOpenState(
    PSRV_EXEC_CONTEXT       pExecContext,
    POPEN_REQUEST_HEADER    pRequestHeader,
    PWSTR                   pwszFilename,
    PSRV_OPEN_STATE_SMB_V1* ppOpenState
    );

static
VOID
SrvLogOpenState_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    );

static
NTSTATUS
SrvQueryFileOpenInformation(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvRequestOpenXOplocks(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildOpenResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareOpenStateAsync(
    PSRV_OPEN_STATE_SMB_V1 pOpenState,
    PSRV_EXEC_CONTEXT      pExecContext
    );

static
VOID
SrvExecuteOpenAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseOpenStateAsync(
    PSRV_OPEN_STATE_SMB_V1 pOpenState
    );

static
VOID
SrvReleaseOpenStateHandle(
    HANDLE hState
    );

static
VOID
SrvReleaseOpenState(
    PSRV_OPEN_STATE_SMB_V1 pOpenState
    );

static
VOID
SrvFreeOpenState(
    PSRV_OPEN_STATE_SMB_V1 pOpenState
    );

NTSTATUS
SrvProcessOpenAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;
    PSRV_OPEN_STATE_SMB_V1     pOpenState   = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pOpenState = (PSRV_OPEN_STATE_SMB_V1)pCtxSmb1->hState;
    if (pOpenState)
    {
        InterlockedIncrement(&pOpenState->refCount);
    }
    else
    {
        ULONG               iMsg        = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        POPEN_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PWSTR                pwszFilename = NULL;   // Do not free

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

        ntStatus = WireUnmarshallOpenRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pwszFilename);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvBuildOpenState(
                        pExecContext,
                        pRequestHeader,
                        pwszFilename,
                        &pOpenState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pOpenState;
        InterlockedIncrement(&pOpenState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseOpenStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pOpenState->mutex);

    switch (pOpenState->stage)
    {
        case SRV_OPEN_STAGE_SMB_V1_INITIAL:

            SRV_LOG_CALL_DEBUG(
                    pExecContext->pLogContext,
                    SMB_PROTOCOL_VERSION_1,
                    pCtxSmb1->pRequests[pCtxSmb1->iMsg].pHeader->command,
                    &SrvLogOpenState_SMB_V1,
                    pOpenState);

            pOpenState->stage = SRV_OPEN_STAGE_SMB_V1_OPEN_FILE_COMPLETED;

            SrvPrepareOpenStateAsync(pOpenState, pExecContext);

            ntStatus = SrvIoCreateFile(
                            pOpenState->pTree->pShareInfo,
                            &pOpenState->hFile,
                            pOpenState->pAcb,
                            &pOpenState->ioStatusBlock,
                            pSession->pIoSecurityContext,
                            pOpenState->pFilename,
                            pOpenState->pSecurityDescriptor,
                            pOpenState->pSecurityQOS,
                            pOpenState->ulDesiredAccessMask,
                            pOpenState->pRequestHeader->ulAllocationSize,
                            pOpenState->pRequestHeader->usFileAttributes,
                            pOpenState->usShareAccess,
                            pOpenState->ulCreateDisposition,
                            pOpenState->usCreateOptions,
                            NULL, /* EA Buffer */
                            0,    /* EA Length */
                            pOpenState->pEcpList);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseOpenStateAsync(pOpenState); // completed synchronously

            // intentional fall through

        case SRV_OPEN_STAGE_SMB_V1_OPEN_FILE_COMPLETED:

            ntStatus = pOpenState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pOpenState->ulCreateAction =
                            pOpenState->ioStatusBlock.CreateResult;

            ntStatus = SrvTreeCreateFile(
                            pOpenState->pTree,
                            pOpenState->pwszFilename,
                            &pOpenState->hFile,
                            &pOpenState->pFilename,
                            pOpenState->pRequestHeader->usDesiredAccess,
                            pOpenState->pRequestHeader->ulAllocationSize,
                            pOpenState->pRequestHeader->usFileAttributes,
                            pOpenState->usShareAccess,
                            pOpenState->ulCreateDisposition,
                            pOpenState->usCreateOptions,
                            &pOpenState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pOpenState->pFile->pfnCancelAsyncOperationsFile = SrvCancelFileAsyncOperations;

            pOpenState->stage = SRV_OPEN_STAGE_SMB_V1_ATTEMPT_QUERY_INFO;

            // intentional fall through

        case SRV_OPEN_STAGE_SMB_V1_ATTEMPT_QUERY_INFO:

            ntStatus = SrvQueryFileOpenInformation(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pOpenState->stage = SRV_OPEN_STAGE_SMB_V1_REQUEST_OPLOCK;

            // intentional fall through

        case SRV_OPEN_STAGE_SMB_V1_REQUEST_OPLOCK:

            ntStatus = SrvRequestOpenXOplocks(pExecContext);
            // Don't fail on the account on not being granted an oplock

            pOpenState->stage = SRV_OPEN_STAGE_SMB_V1_QUERY_INFO_COMPLETED;

            // intentional fall through

        case SRV_OPEN_STAGE_SMB_V1_QUERY_INFO_COMPLETED:

            ntStatus = pOpenState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            if (!pOpenState->pFile->hCancellableBRLStateList)
            {
                PSRV_BYTE_RANGE_LOCK_STATE_LIST pBRLStateList = NULL;

                ntStatus = SrvCreatePendingLockStateList(&pBRLStateList);
                BAIL_ON_NT_STATUS(ntStatus);

                pOpenState->pFile->hCancellableBRLStateList =
                                (HANDLE)pBRLStateList;

                pOpenState->pFile->pfnFreeBRLStateList =
                                &SrvFreePendingLockStateListHandle;
            }

            ntStatus = SrvBuildOpenResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvElementsRegisterResource(
                            &pOpenState->pFile->resource,
                            NULL);
            BAIL_ON_NT_STATUS(ntStatus);

            pOpenState->stage = SRV_OPEN_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_OPEN_STAGE_SMB_V1_DONE:

            if (pOpenState->pRequestHeader->usDesiredAccess & FILE_READ_DATA)
            {
                pOpenState->pFile->ulPermissions |= SRV_PERM_FILE_READ;
            }
            if (pOpenState->pRequestHeader->usDesiredAccess & FILE_WRITE_DATA)
            {
                pOpenState->pFile->ulPermissions |= SRV_PERM_FILE_WRITE;
            }
            if (pOpenState->ulCreateAction == FILE_CREATED)
            {
                pOpenState->pFile->ulPermissions |= SRV_PERM_FILE_CREATE;
            }

            // transfer file so we do not run it down
            pCtxSmb1->pFile = pOpenState->pFile;
            pOpenState->pFile = NULL;

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

    if (pOpenState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pOpenState->mutex);

        SrvReleaseOpenState(pOpenState);
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

            if (pOpenState)
            {
                SrvReleaseOpenStateAsync(pOpenState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildOpenState(
    PSRV_EXEC_CONTEXT       pExecContext,
    POPEN_REQUEST_HEADER    pRequestHeader,
    PWSTR                   pwszFilename,
    PSRV_OPEN_STATE_SMB_V1* ppOpenState
    )
{
    NTSTATUS                   ntStatus       = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection    = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1       = pCtxProtocol->pSmb1Context;
    PSRV_OPEN_STATE_SMB_V1     pOpenState   = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_OPEN_STATE_SMB_V1),
                    (PVOID*)&pOpenState);
    BAIL_ON_NT_STATUS(ntStatus);

    pOpenState->refCount = 1;

    pthread_mutex_init(&pOpenState->mutex, NULL);
    pOpenState->pMutex = &pOpenState->mutex;

    pOpenState->stage = SRV_OPEN_STAGE_SMB_V1_INITIAL;

    // TODO: Handle root fids
    ntStatus = SrvAllocateMemory(
                    sizeof(IO_FILE_NAME),
                    (PVOID*)&pOpenState->pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pOpenState->pTree = SrvTreeAcquire(pCtxSmb1->pTree);

    ntStatus = SrvBuildTreeRelativePath(
                    pCtxSmb1->pTree,
                    pwszFilename,
                    pOpenState->pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    pOpenState->pwszFilename = pwszFilename;

    /* For named pipes, we need to pipe some extra data into the npfs driver:
     *  - Session key
     *  - Client principal name
     *  - Client address
     */
    if (SrvTreeIsNamedPipe(pCtxSmb1->pTree))
    {
        ntStatus = IoRtlEcpListAllocate(&pOpenState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeSessionKey(
                       pConnection,
                       pOpenState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeClientAddress(
                       pConnection,
                       pOpenState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pOpenState->usCreateOptions = FILE_NON_DIRECTORY_FILE;

    pOpenState->usShareAccess = (FILE_SHARE_READ  |
                                 FILE_SHARE_WRITE |
                                 FILE_SHARE_DELETE);

    switch (pRequestHeader->usDesiredAccess & 0x70)
    {
        case 0x00: /* compatibility mode */

            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);

            break;

        case 0x10: /* deny read/write/execute (exclusive) */

            pOpenState->usShareAccess &= (USHORT)~FILE_SHARE_READ;
            pOpenState->usShareAccess &= (USHORT)~FILE_SHARE_WRITE;

            break;

        case 0x20: /* deny write */

            pOpenState->usShareAccess &= (USHORT)~FILE_SHARE_WRITE;

            break;

        case 0x30: /* deny read/execute */

            pOpenState->usShareAccess &= (USHORT)~FILE_SHARE_READ;

            break;

        case 0x40: /* deny none */

            break;
    }

    /* desired access mask */
    switch (pRequestHeader->usDesiredAccess & 0x7)
    {
        case 0x00:

            pOpenState->ulDesiredAccessMask = GENERIC_READ;

            break;

        case 0x01:

            pOpenState->ulDesiredAccessMask = GENERIC_WRITE;

            break;

        case 0x02:

            pOpenState->ulDesiredAccessMask = GENERIC_READ | GENERIC_WRITE;

            break;

        case 0x03:

            pOpenState->ulDesiredAccessMask = (GENERIC_READ  |
                                               GENERIC_EXECUTE);

            break;

        default:

            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    /* action to take if the file exists */
    switch (pRequestHeader->usOpenFunction)
    {
        case 0x0000: /* Weird EXECUTE -> OPEN_IF semantics */

            if ((pOpenState->ulDesiredAccessMask & GENERIC_EXECUTE) == 0) {
                ntStatus = STATUS_INVALID_DISPOSITION;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pOpenState->ulCreateDisposition = FILE_OPEN_IF;

            break;

        case 0x0001: /* Open file */

            pOpenState->ulCreateDisposition = FILE_OPEN;

            break;

        case 0x0002: /* truncate file */

            pOpenState->ulCreateDisposition = FILE_OVERWRITE;

            break;

        case 0x0010: /* create new file */

            pOpenState->ulCreateDisposition = FILE_CREATE;

            break;

        case 0x0011: /* open or create file */

            pOpenState->ulCreateDisposition = FILE_OPEN_IF;

            break;

        case 0x0012: /* create or truncate */

            pOpenState->ulCreateDisposition = FILE_OVERWRITE_IF;

            break;

        default:

            ntStatus = STATUS_INVALID_DISPOSITION;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

    pOpenState->pRequestHeader = pRequestHeader;

    *ppOpenState = pOpenState;

cleanup:

    return ntStatus;

error:

    *ppOpenState = NULL;

    if (pOpenState)
    {
        SrvFreeOpenState(pOpenState);
    }

    goto cleanup;
}

static
VOID
SrvLogOpenState_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_OPEN_STATE_SMB_V1 pOpenState = NULL;
    PSTR pszPath = NULL;
    va_list msgList;

    va_start(msgList, ulLine);

    pOpenState = va_arg(msgList, PSRV_OPEN_STATE_SMB_V1);

    if (pOpenState)
    {
        if (pOpenState->pFilename->FileName)
        {
            ntStatus = SrvWc16sToMbs(pOpenState->pFilename->FileName, &pszPath);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (logLevel >= LWIO_LOG_LEVEL_DEBUG)
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "[%s() %s:%u] Open state: "
                    "alloc-size(%u),creation-time(0x%x),desired-access(0x%x),"
                    "file-attrs(0x%x),flags(0x%x),open-function(%u),"
                    "search-attrs(0x%x),path(%s)",
                    LWIO_SAFE_LOG_STRING(pszFunction),
                    LWIO_SAFE_LOG_STRING(pszFile),
                    ulLine,
                    pOpenState->pRequestHeader->ulAllocationSize,
                    pOpenState->pRequestHeader->ulCreationTime,
                    pOpenState->pRequestHeader->usDesiredAccess,
                    pOpenState->pRequestHeader->usFileAttributes,
                    pOpenState->pRequestHeader->usFlags,
                    pOpenState->pRequestHeader->usOpenFunction,
                    pOpenState->pRequestHeader->usSearchAttributes,
                    LWIO_SAFE_LOG_STRING(pszPath));
        }
        else
        {
            LWIO_LOG_ALWAYS_CUSTOM(
                    logLevel,
                    "Open state: "
                    "alloc-size(%u),creation-time(0x%x),desired-access(0x%x),"
                    "file-attrs(0x%x),flags(0x%x),open-function(%u),"
                    "search-attrs(0x%x),path(%s)",
                    pOpenState->pRequestHeader->ulAllocationSize,
                    pOpenState->pRequestHeader->ulCreationTime,
                    pOpenState->pRequestHeader->usDesiredAccess,
                    pOpenState->pRequestHeader->usFileAttributes,
                    pOpenState->pRequestHeader->usFlags,
                    pOpenState->pRequestHeader->usOpenFunction,
                    pOpenState->pRequestHeader->usSearchAttributes,
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
SrvQueryFileOpenInformation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_OPEN_STATE_SMB_V1     pOpenState = NULL;

    pOpenState = (PSRV_OPEN_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pOpenState->pFileBasicInfo)
    {
        pOpenState->pFileBasicInfo = &pOpenState->fileBasicInfo;

        SrvPrepareOpenStateAsync(pOpenState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pOpenState->pFile->hFile,
                        pOpenState->pAcb,
                        &pOpenState->ioStatusBlock,
                        pOpenState->pFileBasicInfo,
                        sizeof(pOpenState->fileBasicInfo),
                        FileBasicInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseOpenStateAsync(pOpenState); // completed synchronously
    }

    if (!(pOpenState->pFileBasicInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        SrvFileBlockIdleTimeout(pOpenState->pFile);
    }
    else
    {
        SrvFileUnblockIdleTimeout(pOpenState->pFile);
    }

    if (!pOpenState->pFileStdInfo)
    {
        pOpenState->pFileStdInfo = &pOpenState->fileStdInfo;

        SrvPrepareOpenStateAsync(pOpenState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pOpenState->pFile->hFile,
                        pOpenState->pAcb,
                        &pOpenState->ioStatusBlock,
                        pOpenState->pFileStdInfo,
                        sizeof(pOpenState->fileStdInfo),
                        FileStandardInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseOpenStateAsync(pOpenState); // completed synchronously
    }

    if (SrvTreeIsNamedPipe(pOpenState->pTree))
    {
        if (!pOpenState->pFilePipeInfo)
        {
            pOpenState->pFilePipeInfo = &pOpenState->filePipeInfo;

            SrvPrepareOpenStateAsync(pOpenState, pExecContext);

            ntStatus = IoQueryInformationFile(
                            pOpenState->pFile->hFile,
                            pOpenState->pAcb,
                            &pOpenState->ioStatusBlock,
                            pOpenState->pFilePipeInfo,
                            sizeof(pOpenState->filePipeInfo),
                            FilePipeInformation);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseOpenStateAsync(pOpenState); // completed synchronously
        }

        if (!pOpenState->pFilePipeLocalInfo)
        {
            pOpenState->pFilePipeLocalInfo = &pOpenState->filePipeLocalInfo;

            SrvPrepareOpenStateAsync(pOpenState, pExecContext);

            ntStatus = IoQueryInformationFile(
                            pOpenState->pFile->hFile,
                            pOpenState->pAcb,
                            &pOpenState->ioStatusBlock,
                            pOpenState->pFilePipeLocalInfo,
                            sizeof(pOpenState->filePipeLocalInfo),
                            FilePipeLocalInformation);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseOpenStateAsync(pOpenState); // completed synchronously
        }
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvRequestOpenXOplocks(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS        ntStatus           = STATUS_SUCCESS;
    SRV_OPLOCK_INFO batchOplockChain[] =
            {
               { IO_OPLOCK_REQUEST_OPLOCK_BATCH,   SMB_OPLOCK_LEVEL_BATCH },
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    SRV_OPLOCK_INFO exclOplockChain[] =
            {
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1, SMB_OPLOCK_LEVEL_I     },
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    SRV_OPLOCK_INFO noOplockChain[] =
            {
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    PSRV_OPLOCK_INFO           pOplockCursor = NULL;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1      = pCtxProtocol->pSmb1Context;
    PSRV_OPEN_STATE_SMB_V1     pOpenState  = NULL;
    PSRV_OPLOCK_STATE_SMB_V1   pOplockState  = NULL;
    BOOLEAN                    bContinue     = TRUE;

    pOpenState = (PSRV_OPEN_STATE_SMB_V1)pCtxSmb1->hState;

    if (SrvTreeIsNamedPipe(pOpenState->pTree) ||
        pOpenState->fileStdInfo.Directory)
    {
        pOplockCursor = &noOplockChain[0];

        goto done;
    }

    ntStatus = SrvBuildOplockState(
                    pExecContext->pConnection,
                    pCtxSmb1->pSession,
                    pCtxSmb1->pTree,
                    pOpenState->pFile,
                    &pOplockState);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pOpenState->pRequestHeader->usFlags & SMB_OPLOCK_REQUEST_BATCH)
    {
        pOplockCursor = &batchOplockChain[0];
    }
    else if (pOpenState->pRequestHeader->usFlags & SMB_OPLOCK_REQUEST_EXCLUSIVE)
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
                        pOpenState->pFile->hFile,
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
                               pOpenState->pFile,
                               pOplockState,
                               &SrvCancelOplockStateHandle,
                               &SrvReleaseOplockStateHandle);
                if (ntStatus != STATUS_SUCCESS)
                {
                    InterlockedDecrement(&pOplockState->refCount);
                }
                BAIL_ON_NT_STATUS(ntStatus);

                SrvFileSetOplockLevel(
                        pOpenState->pFile,
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

    pOpenState->ucOplockLevel = pOplockCursor->oplockLevel;

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
SrvBuildOpenResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus          = 0;
    PLWIO_SRV_CONNECTION        pConnection    = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol   = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1        = pCtxProtocol->pSmb1Context;
    ULONG                       iMsg            = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1         pSmbRequest     = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1         pSmbResponse    = &pCtxSmb1->pResponses[iMsg];
    POPEN_RESPONSE_HEADER       pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PSRV_OPEN_STATE_SMB_V1 pOpenState = NULL;

    pOpenState = (PSRV_OPEN_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_OPEN_ANDX,
                        STATUS_SUCCESS,
                        TRUE,
                        pConnection->serverProperties.Capabilities,
                        pOpenState->pTree->tid,
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
                        COM_OPEN_ANDX,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 15;

    if (ulBytesAvailable < sizeof(OPEN_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (POPEN_RESPONSE_HEADER)pOutBuffer;

    // pOutBuffer       += sizeof(OPEN_RESPONSE_HEADER);
    // ulOffset         += sizeof(OPEN_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(OPEN_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(OPEN_RESPONSE_HEADER);

    pResponseHeader->usFid        = pOpenState->pFile->fid;
    pResponseHeader->ulServerFid  = pOpenState->pFile->fid;

    pResponseHeader->usOpenAction = pOpenState->ulCreateAction;
    switch (pOpenState->ucOplockLevel)
    {
        case SMB_OPLOCK_LEVEL_I:

            // file is opened only by this user at the current time
            pResponseHeader->usOpenAction |= 0x8000;

            break;

        default:

            pResponseHeader->usOpenAction &= ~0x8000;

            break;
    }

    // TODO: Mirroring this field is close, but probably not exactly correct
    pResponseHeader->usGrantedAccess = pOpenState->pRequestHeader->usDesiredAccess;

    pResponseHeader->usFileAttributes = pOpenState->fileBasicInfo.FileAttributes;

    ntStatus = WireNTTimeToSMBUTime(
                    pOpenState->fileBasicInfo.LastWriteTime,
                    &pResponseHeader->ulLastWriteTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->ulDataSize = SMB_MIN(  UINT32_MAX,
                                            pOpenState->fileStdInfo.EndOfFile);

    if (SrvTreeIsNamedPipe(pOpenState->pTree))
    {
        ntStatus = SrvMarshallPipeInfo(
                        pOpenState->pFilePipeInfo,
                        pOpenState->pFilePipeLocalInfo,
                        &pResponseHeader->usDeviceState);
        BAIL_ON_NT_STATUS(ntStatus);

        pResponseHeader->usFileType = (USHORT)pOpenState->filePipeInfo.ReadMode;
    }
    else
    {
        pResponseHeader->usFileType = 0;
        pResponseHeader->usDeviceState = 0;
    }
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
SrvPrepareOpenStateAsync(
    PSRV_OPEN_STATE_SMB_V1 pOpenState,
    PSRV_EXEC_CONTEXT      pExecContext
    )
{
    pOpenState->acb.Callback        = &SrvExecuteOpenAsyncCB;

    pOpenState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pOpenState->acb.AsyncCancelContext = NULL;

    pOpenState->pAcb = &pOpenState->acb;
}

static
VOID
SrvExecuteOpenAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_OPEN_STATE_SMB_V1     pOpenState     = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pOpenState = (PSRV_OPEN_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pOpenState->mutex);

    if (pOpenState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pOpenState->pAcb->AsyncCancelContext);
    }

    pOpenState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pOpenState->mutex);

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
SrvReleaseOpenStateAsync(
    PSRV_OPEN_STATE_SMB_V1 pOpenState
    )
{
    if (pOpenState->pAcb)
    {
        pOpenState->acb.Callback        = NULL;

        if (pOpenState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pOpenState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pOpenState->pAcb->CallbackContext = NULL;
        }

        if (pOpenState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pOpenState->pAcb->AsyncCancelContext);
        }

        pOpenState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseOpenStateHandle(
    HANDLE hState
    )
{
    SrvReleaseOpenState((PSRV_OPEN_STATE_SMB_V1)hState);
}

static
VOID
SrvReleaseOpenState(
    PSRV_OPEN_STATE_SMB_V1 pOpenState
    )
{
    if (InterlockedDecrement(&pOpenState->refCount) == 0)
    {
        SrvFreeOpenState(pOpenState);
    }
}

static
VOID
SrvFreeOpenState(
    PSRV_OPEN_STATE_SMB_V1 pOpenState
    )
{
    if (pOpenState->pAcb && pOpenState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pOpenState->pAcb->AsyncCancelContext);
    }

    if (pOpenState->pEcpList)
    {
        IoRtlEcpListFree(&pOpenState->pEcpList);
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pOpenState->pFilename)
    {
        if (pOpenState->pFilename->FileName)
        {
            SrvFreeMemory(pOpenState->pFilename->FileName);
        }

        SrvFreeMemory(pOpenState->pFilename);
    }

    if (pOpenState->hFile)
    {
        IoCloseFile(pOpenState->hFile);
    }

    if (pOpenState->pFile)
    {
        SrvFileResetOplockState(pOpenState->pFile);
        SrvFileRundown(pOpenState->pFile);
        SrvFileRelease(pOpenState->pFile);
    }

    if (pOpenState->pTree)
    {
        SrvTreeRelease(pOpenState->pTree);
    }

    if (pOpenState->pMutex)
    {
        pthread_mutex_destroy(&pOpenState->mutex);
    }

    SrvFreeMemory(pOpenState);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
