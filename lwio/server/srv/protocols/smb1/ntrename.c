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
SrvBuildNtRenameState(
    PSMB_NT_RENAME_REQUEST_HEADER pRequestHeader,
    PWSTR                         pwszOldName,
    PWSTR                         pwszNewName,
    PSRV_NT_RENAME_STATE_SMB_V1*  ppRenameState
    );

static
VOID
SrvLogNtRename_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    );

static
NTSTATUS
SrvExecuteNtRename(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildNtRenameResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareNtRenameStateAsync(
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState,
    PSRV_EXEC_CONTEXT           pExecContext
    );

static
VOID
SrvExecuteNtRenameAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseNtRenameStateAsync(
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState
    );

static
VOID
SrvReleaseNtRenameStateHandle(
    HANDLE hState
    );

static
VOID
SrvReleaseNtRenameState(
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState
    );

static
VOID
SrvFreeNtRenameState(
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState
    );

static
NTSTATUS
SrvValidateStreamNewName(
    PWSTR pwszOldName,
    PWSTR pwszNewName
    );

NTSTATUS
SrvProcessNtRename(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus     = 0;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PLWIO_SRV_SESSION           pSession     = NULL;
    PLWIO_SRV_TREE              pTree        = NULL;
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState = NULL;
    BOOLEAN                     bInLock      = FALSE;

    pRenameState = (PSRV_NT_RENAME_STATE_SMB_V1)pCtxSmb1->hState;
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
        PSMB_NT_RENAME_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PWSTR                      pwszOldName    = NULL; // Do not free
        PWSTR                      pwszNewName    = NULL; // Do not free

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

        ntStatus = WireUnmarshallNtRenameRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pwszOldName,
                        &pwszNewName);
        BAIL_ON_NT_STATUS(ntStatus);

        SRV_LOG_CALL_DEBUG(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_1,
                pSmbRequest->pHeader->command,
                &SrvLogNtRename_SMB_V1,
                pRequestHeader,
                pwszOldName,
                pwszNewName);

        ntStatus = SrvBuildNtRenameState(
                        pRequestHeader,
                        pwszOldName,
                        pwszNewName,
                        &pRenameState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pRenameState;
        InterlockedIncrement(&pRenameState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseNtRenameStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pRenameState->mutex);

    switch (pRenameState->stage)
    {
        case SRV_NT_RENAME_STAGE_SMB_V1_INITIAL:

            ntStatus = SrvBuildTreeRelativePath(
                            pCtxSmb1->pTree,
                            pRenameState->pwszOldName,
                            &pRenameState->oldName);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvBuildTreeRelativePath(
                            pCtxSmb1->pTree,
                            NULL,
                            &pRenameState->dirPath);
            BAIL_ON_NT_STATUS(ntStatus);

            pRenameState->newName.FileName = pRenameState->pwszNewName;

            pRenameState->newName.RootFileHandle = pTree->hFile;

            pRenameState->stage = SRV_NT_RENAME_STAGE_SMB_V1_ATTEMPT_RENAME;

            // intentional fall through

        case SRV_NT_RENAME_STAGE_SMB_V1_ATTEMPT_RENAME:

            ntStatus = SrvExecuteNtRename(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pRenameState->stage = SRV_NT_RENAME_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_NT_RENAME_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildNtRenameResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pRenameState->stage = SRV_NT_RENAME_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_NT_RENAME_STAGE_SMB_V1_DONE:

            break;
    }

cleanup:

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

        SrvReleaseNtRenameState(pRenameState);
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
                SrvReleaseNtRenameStateAsync(pRenameState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvValidateStreamNewName(
    PWSTR pwszOldName,
    PWSTR pwszNewName
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    wchar16_t wszColon[] = {':', 0};

    while(!IsNullOrEmptyString(pwszOldName))
    {
        if (*pwszOldName++ == wszColon[0])
        {
            if (*pwszNewName != wszColon[0] ||
                IsNullOrEmptyString((pwszNewName+1)))
            {
                ntStatus = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            break;
        }
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildNtRenameState(
    PSMB_NT_RENAME_REQUEST_HEADER pRequestHeader,
    PWSTR                         pwszOldName,
    PWSTR                         pwszNewName,
    PSRV_NT_RENAME_STATE_SMB_V1*  ppRenameState
    )
{
    NTSTATUS                    ntStatus     = STATUS_SUCCESS;
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState = NULL;

    if (!pwszOldName || !*pwszOldName || !pwszNewName || !*pwszNewName)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvValidateStreamNewName(pwszOldName, pwszNewName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_NT_RENAME_STATE_SMB_V1),
                    (PVOID*)&pRenameState);
    BAIL_ON_NT_STATUS(ntStatus);

    pRenameState->refCount = 1;

    pthread_mutex_init(&pRenameState->mutex, NULL);
    pRenameState->pMutex = &pRenameState->mutex;

    pRenameState->stage = SRV_NT_RENAME_STAGE_SMB_V1_INITIAL;

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
        SrvFreeNtRenameState(pRenameState);
    }

    goto cleanup;
}

static
VOID
SrvLogNtRename_SMB_V1(
    PSRV_LOG_CONTEXT pLogContext,
    LWIO_LOG_LEVEL   logLevel,
    PCSTR            pszFunction,
    PCSTR            pszFile,
    ULONG            ulLine,
    ...
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_NT_RENAME_REQUEST_HEADER pRequestHeader = NULL;
    PWSTR pwszOldName = NULL;
    PWSTR pwszNewName = NULL;
    PSTR  pszOldName  = NULL;
    PSTR  pszNewName  = NULL;
    va_list msgList;

    va_start(msgList, ulLine);
    pRequestHeader = va_arg(msgList, PSMB_NT_RENAME_REQUEST_HEADER);
    pwszOldName    = va_arg(msgList, PWSTR);
    pwszNewName    = va_arg(msgList, PWSTR);

    if (pwszOldName)
    {
        ntStatus = SrvWc16sToMbs(pwszOldName, &pszOldName);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pwszNewName)
    {
        ntStatus = SrvWc16sToMbs(pwszNewName, &pszNewName);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (logLevel >= LWIO_LOG_LEVEL_DEBUG)
    {
        LWIO_LOG_ALWAYS_CUSTOM(
                logLevel,
                "[%s() %s:%u] NtRename Parameters: cluster-count(%u),info-level(%u),search-attrs(0x%x),old-name(%s),new-name(%s)",
                LWIO_SAFE_LOG_STRING(pszFunction),
                LWIO_SAFE_LOG_STRING(pszFile),
                ulLine,
                pRequestHeader->ulClusterCount,
                pRequestHeader->usInfoLevel,
                pRequestHeader->usSearchAttributes,
                LWIO_SAFE_LOG_STRING(pszOldName),
                LWIO_SAFE_LOG_STRING(pszNewName));
    }
    else
    {
        LWIO_LOG_ALWAYS_CUSTOM(
                logLevel,
                "NtRename Parameters: cluster-count(%u),info-level(%u),search-attrs(0x%x),old-name(%s),new-name(%s)",
                pRequestHeader->ulClusterCount,
                pRequestHeader->usInfoLevel,
                pRequestHeader->usSearchAttributes,
                LWIO_SAFE_LOG_STRING(pszOldName),
                LWIO_SAFE_LOG_STRING(pszNewName));
    }

error:

    va_end(msgList);

    SRV_SAFE_FREE_MEMORY(pszOldName);
    SRV_SAFE_FREE_MEMORY(pszNewName);

    return;
}

static
NTSTATUS
SrvExecuteNtRename(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus        = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol    = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1        = pCtxProtocol->pSmb1Context;
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState   = NULL;

    pRenameState = (PSRV_NT_RENAME_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = pRenameState->ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pRenameState->hDir)
    {
        SrvPrepareNtRenameStateAsync(pRenameState, pExecContext);

        ntStatus = SrvIoCreateFile(
                        pCtxSmb1->pTree->pShareInfo,
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
                        pRenameState->pDirEcpList
                        );
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseNtRenameStateAsync(pRenameState); // completed synchronously
    }

    if (!pRenameState->hFile)
    {
        SrvPrepareNtRenameStateAsync(pRenameState, pExecContext);

        ntStatus = SrvIoCreateFile(
                        pCtxSmb1->pTree->pShareInfo,
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
                        FILE_SHARE_READ,
                        FILE_OPEN,
                        0,
                        NULL, /* EA Buffer */
                        0,    /* EA Length */
                        pRenameState->pFileEcpList
                        );
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseNtRenameStateAsync(pRenameState); // completed synchronously
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

        pRenameState->pFileRenameInfo->ReplaceIfExists = FALSE;
        pRenameState->pFileRenameInfo->RootDirectory   = pRenameState->hDir;
        pRenameState->pFileRenameInfo->FileNameLength  =
                wc16slen(pRenameState->newName.FileName) * sizeof(wchar16_t);
        memcpy( (PBYTE)pRenameState->pFileRenameInfo->FileName,
                (PBYTE)pRenameState->newName.FileName,
                pRenameState->pFileRenameInfo->FileNameLength);

        SrvPrepareNtRenameStateAsync(pRenameState, pExecContext);

        ntStatus = IoSetInformationFile(
                        pRenameState->hFile,
                        pRenameState->pAcb,
                        &pRenameState->ioStatusBlock,
                        pRenameState->pFileRenameInfo,
                        pRenameState->ulDataLen,
                        FileRenameInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseNtRenameStateAsync(pRenameState); // completed synchronously
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildNtRenameResponse(
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
    PSMB_NT_RENAME_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
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
                        COM_NT_RENAME,
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
                        COM_NT_RENAME,
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

    ntStatus = WireMarshallNtRenameResponse(
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
SrvPrepareNtRenameStateAsync(
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState,
    PSRV_EXEC_CONTEXT           pExecContext
    )
{
    pRenameState->acb.Callback        = &SrvExecuteNtRenameAsyncCB;

    pRenameState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pRenameState->acb.AsyncCancelContext = NULL;

    pRenameState->pAcb = &pRenameState->acb;
}

static
VOID
SrvExecuteNtRenameAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState     = NULL;
    BOOLEAN                     bInLock          = FALSE;

    pRenameState =
            (PSRV_NT_RENAME_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

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
SrvReleaseNtRenameStateAsync(
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState
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
SrvReleaseNtRenameStateHandle(
    HANDLE hState
    )
{
    SrvReleaseNtRenameState((PSRV_NT_RENAME_STATE_SMB_V1)hState);
}

static
VOID
SrvReleaseNtRenameState(
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState
    )
{
    if (InterlockedDecrement(&pRenameState->refCount) == 0)
    {
        SrvFreeNtRenameState(pRenameState);
    }
}

static
VOID
SrvFreeNtRenameState(
    PSRV_NT_RENAME_STATE_SMB_V1 pRenameState
    )
{
    if (pRenameState->pAcb && pRenameState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                    &pRenameState->pAcb->AsyncCancelContext);
    }

    if (pRenameState->pDirEcpList)
    {
        IoRtlEcpListFree(&pRenameState->pDirEcpList);
    }

    if (pRenameState->pFileEcpList)
    {
        IoRtlEcpListFree(&pRenameState->pFileEcpList);
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


