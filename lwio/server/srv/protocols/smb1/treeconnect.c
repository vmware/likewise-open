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
SrvBuildTreeConnectState(
    PTREE_CONNECT_REQUEST_HEADER    pRequestHeader,
    PBYTE                           pszPassword,
    PBYTE                           pszService,
    PWSTR                           pwszPath,
    PSRV_TREE_CONNECT_STATE_SMB_V1* ppTConState
    );

static
NTSTATUS
SrvQueryTreeConnectInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareTreeConnectStateAsync(
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState,
    PSRV_EXEC_CONTEXT              pExecContext
    );

static
VOID
SrvExecuteTreeConnectAsyncCB(
    PVOID pContext
    );

static
VOID
SrvReleaseTreeConnectStateAsync(
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState
    );

static
NTSTATUS
SrvBuildTreeConnectResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvGetServiceName(
    PSRV_SHARE_INFO pShareInfo,
    PSTR*           ppszService
    );

static
NTSTATUS
SrvGetNativeFilesystem(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvReleaseTreeConnectStateHandle(
    HANDLE hTConState
    );

static
VOID
SrvReleaseTreeConnectState(
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState
    );

static
VOID
SrvFreeTreeConnectState(
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState
    );

NTSTATUS
SrvProcessTreeConnectAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState    = NULL;
    PLWIO_SRV_SESSION              pSession      = NULL;
    PWSTR                          pwszSharename = NULL;
    BOOLEAN                        bTConStateInLock = FALSE;
    BOOLEAN                        bShareInfoInLock = FALSE;

    pTConState = (PSRV_TREE_CONNECT_STATE_SMB_V1)pCtxSmb1->hState;
    if (pTConState)
    {
        InterlockedIncrement(&pTConState->refCount);
    }
    else
    {
        ULONG               iMsg         = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
        PBYTE pBuffer  = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PTREE_CONNECT_REQUEST_HEADER  pRequestHeader = NULL; // Do not free
        PBYTE                         pszPassword    = NULL; // Do not free
        PBYTE                         pszService     = NULL; // Do not free
        PWSTR                         pwszPath       = NULL; // Do not free

        if (pCtxSmb1->pTree)
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

        ntStatus = UnmarshallTreeConnectRequest(
                        pBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        &pRequestHeader,
                        &pszPassword,
                        &pwszPath,
                        &pszService);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pRequestHeader->flags & 0x1)
        {
            NTSTATUS ntStatus2 = 0;

            ntStatus2 = SrvSessionRemoveTree(
                            pSession,
                            pSmbRequest->pHeader->tid);
            if (ntStatus2)
            {
                LWIO_LOG_ERROR("Failed to remove tid [%u] from session [uid=%u]. [code:%d]",
                                pSmbRequest->pHeader->tid,
                                pSession->uid,
                                ntStatus2);
            }
        }

        ntStatus = SrvBuildTreeConnectState(
                        pRequestHeader,
                        pszPassword,
                        pszService,
                        pwszPath,
                        &pTConState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pTConState;
        InterlockedIncrement(&pTConState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseTreeConnectStateHandle;

        pTConState->pSession = pSession;
        InterlockedIncrement(&pSession->refcount);
    }

    LWIO_LOCK_MUTEX(bTConStateInLock, &pTConState->mutex);

    switch (pTConState->stage)
    {
        case SRV_TREE_CONNECT_STAGE_SMB_V1_INITIAL:

            LWIO_LOCK_RWMUTEX_SHARED(
                            bShareInfoInLock,
                            &pConnection->pHostinfo->mutex);

            ntStatus = SrvGetShareName(
                            pConnection->pHostinfo->pszHostname,
                            pConnection->pHostinfo->pszDomain,
                            pTConState->pwszPath,
                            &pwszSharename);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(
                            bShareInfoInLock,
                            &pConnection->pHostinfo->mutex);

            ntStatus = SrvShareFindByName(
                            pConnection->pShareList,
                            pwszSharename,
                            &pTConState->pShareInfo);
            if (ntStatus == STATUS_NOT_FOUND)
            {
                ntStatus = STATUS_BAD_NETWORK_NAME;
            }
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSessionCreateTree(
                            pSession,
                            pTConState->pShareInfo,
                            &pTConState->pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            pTConState->bRemoveTreeFromSession = TRUE;

            pTConState->stage = SRV_TREE_CONNECT_STAGE_ATTEMPT_QUERY_INFO;

            // Intentional fall through

        case SRV_TREE_CONNECT_STAGE_ATTEMPT_QUERY_INFO:

            ntStatus = SrvQueryTreeConnectInfo(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTConState->stage = SRV_TREE_CONNECT_STAGE_QUERY_INFO_COMPLETED;

            // Intentional fall through

        case SRV_TREE_CONNECT_STAGE_QUERY_INFO_COMPLETED:

            ntStatus = pTConState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvBuildTreeConnectResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTConState->stage = SRV_TREE_CONNECT_STAGE_DONE;

            // Intentional fall through

        case SRV_TREE_CONNECT_STAGE_DONE:

            pTConState->bRemoveTreeFromSession = FALSE;

            pCtxSmb1->pTree = pTConState->pTree;
            InterlockedIncrement(&pTConState->pTree->refcount);

            break;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bShareInfoInLock, &pConnection->pHostinfo->mutex);

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pwszSharename)
    {
        SrvFreeMemory(pwszSharename);
    }

    if (pTConState)
    {
        LWIO_UNLOCK_MUTEX(bTConStateInLock, &pTConState->mutex);

        SrvReleaseTreeConnectState(pTConState);
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

            if (pTConState)
            {
                SrvReleaseTreeConnectStateAsync(pTConState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildTreeConnectState(
    PTREE_CONNECT_REQUEST_HEADER    pRequestHeader,
    PBYTE                           pszPassword,
    PBYTE                           pszService,
    PWSTR                           pwszPath,
    PSRV_TREE_CONNECT_STATE_SMB_V1* ppTConState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_TREE_CONNECT_STATE_SMB_V1),
                    (PVOID*)&pTConState);
    BAIL_ON_NT_STATUS(ntStatus);

    pTConState->refCount = 1;

    pthread_mutex_init(&pTConState->mutex, NULL);
    pTConState->pMutex = &pTConState->mutex;

    pTConState->stage = SRV_TREE_CONNECT_STAGE_SMB_V1_INITIAL;

    pTConState->pRequestHeader = pRequestHeader;
    pTConState->pszPassword    = pszPassword;
    pTConState->pszService     = pszService;
    pTConState->pwszPath       = pwszPath;

    *ppTConState = pTConState;

cleanup:

    return ntStatus;

error:

    *ppTConState = NULL;

    if (pTConState)
    {
        SrvFreeTreeConnectState(pTConState);
    }

    goto cleanup;
}

static
NTSTATUS
SrvQueryTreeConnectInfo(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState = NULL;

    pTConState = (PSRV_TREE_CONNECT_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = SrvGetServiceName(
                    pTConState->pTree->pShareInfo,
                    &pTConState->pszService2);
    BAIL_ON_NT_STATUS(ntStatus);

    if ((pTConState->pTree->pShareInfo->service == SHARE_SERVICE_DISK_SHARE) &&
        (!pTConState->pwszNativeFileSystem))
    {
        ntStatus = SrvGetNativeFilesystem(pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvGetMaximalShareAccessMask(
                    pTConState->pTree->pShareInfo,
                    &pTConState->ulMaximalShareAccessMask);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGetGuestShareAccessMask(
                    pTConState->pTree->pShareInfo,
                    &pTConState->ulGuestMaximalShareAccessMask);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvPrepareTreeConnectStateAsync(
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState,
    PSRV_EXEC_CONTEXT              pExecContext
    )
{
    pTConState->acb.Callback        = &SrvExecuteTreeConnectAsyncCB;

    pTConState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pTConState->acb.AsyncCancelContext = NULL;

    pTConState->pAcb = &pTConState->acb;
}

static
VOID
SrvExecuteTreeConnectAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState   = NULL;
    BOOLEAN                        bInLock      = FALSE;

    pTConState =
        (PSRV_TREE_CONNECT_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pTConState->mutex);

    if (pTConState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pTConState->pAcb->AsyncCancelContext);
    }

    pTConState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pTConState->mutex);

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
SrvReleaseTreeConnectStateAsync(
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState
    )
{
    if (pTConState->pAcb)
    {
        pTConState->acb.Callback = NULL;

        if (pTConState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pTConState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pTConState->pAcb->CallbackContext = NULL;
        }

        if (pTConState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pTConState->pAcb->AsyncCancelContext);
        }

        pTConState->pAcb = NULL;
    }
}

static
NTSTATUS
SrvBuildTreeConnectResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState   = NULL;
    PSRV_MESSAGE_SMB_V1            pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PTREE_CONNECT_RESPONSE_HEADER  pResponseHeader = NULL; // Do not free
    PBYTE  pOutBuffer       = pSmbResponse->pBuffer;
    ULONG  ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG  ulOffset         = 0;
    USHORT usBytesUsed      = 0;
    ULONG  ulTotalBytesUsed = 0;

    pTConState = (PSRV_TREE_CONNECT_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TREE_CONNECT_ANDX,
                        STATUS_SUCCESS,
                        TRUE,
                        pTConState->pTree->tid,
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
                        COM_TREE_CONNECT_ANDX,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 7;

    if (ulBytesAvailable < sizeof(TREE_CONNECT_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PTREE_CONNECT_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(TREE_CONNECT_RESPONSE_HEADER);
    ulOffset         += sizeof(TREE_CONNECT_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(TREE_CONNECT_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(TREE_CONNECT_RESPONSE_HEADER);

    pResponseHeader->maximalShareAccessMask =
                                pTConState->ulMaximalShareAccessMask;

    pResponseHeader->guestMaximalShareAccessMask =
                                pTConState->ulGuestMaximalShareAccessMask;

    ntStatus = MarshallTreeConnectResponseData(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &usBytesUsed,
                    (const PBYTE)pTConState->pszService2,
                    pTConState->pwszNativeFileSystem);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pResponseHeader->byteCount = usBytesUsed;

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
SrvGetServiceName(
    PSRV_SHARE_INFO pShareInfo,
    PSTR* ppszService
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszService = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    ntStatus = SrvShareMapIdToServiceStringA(
                    pShareInfo->service,
                    &pszService);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppszService = pszService;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    return ntStatus;

error:

    *ppszService = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvGetNativeFilesystem(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState  = NULL;
    PFILE_FS_ATTRIBUTE_INFORMATION pFsAttrInfo = NULL;
    BOOLEAN  bInLock = FALSE;
    BOOLEAN  bContinue = TRUE;

    pTConState = (PSRV_TREE_CONNECT_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pTConState->fileName.FileName)
    {
        LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTConState->pShareInfo->mutex);

        ntStatus = SrvAllocateStringW(
                        pTConState->pShareInfo->pwszPath,
                        &pTConState->fileName.FileName);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_RWMUTEX(bInLock, &pTConState->pShareInfo->mutex);
    }

    if (!pTConState->pIoSecContext)
    {
        ntStatus = IoSecurityCreateSecurityContextFromUidGid(
                        &pTConState->pIoSecContext,
                        0,
                        0,
                        NULL);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (!pTConState->hFile)
    {
        SrvPrepareTreeConnectStateAsync(pTConState, pExecContext);

        ntStatus = IoCreateFile(
                        &pTConState->hFile,
                        pTConState->pAcb,
                        &pTConState->ioStatusBlock,
                        pTConState->pIoSecContext,
                        &pTConState->fileName,
                        pTConState->pSecurityDescriptor,
                        pTConState->pSecurityQOS,
                        GENERIC_READ,
                        0,
                        FILE_ATTRIBUTE_NORMAL,
                        FILE_SHARE_READ,
                        FILE_OPEN,
                        0,
                        NULL, /* EA Buffer */
                        0,    /* EA Length */
                        NULL  /* ECP List  */
                        );
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseTreeConnectStateAsync(pTConState); // completed synchronously
    }

    if (!pTConState->pVolumeInfo)
    {
        pTConState->ioStatusBlock.Status = STATUS_BUFFER_TOO_SMALL;
    }

    do
    {
        ntStatus = pTConState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_SUCCESS:

                bContinue = FALSE;

                break;

            case STATUS_BUFFER_TOO_SMALL:
                {
                    USHORT usNewSize =  pTConState->usBytesAllocated +
                                        256 * sizeof(wchar16_t);

                    ntStatus = SMBReallocMemory(
                                    pTConState->pVolumeInfo,
                                    (PVOID*)&pTConState->pVolumeInfo,
                                    usNewSize);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pTConState->usBytesAllocated = usNewSize;
                }

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (!bContinue)
        {
            break;
        }

        SrvPrepareTreeConnectStateAsync(pTConState, pExecContext);

        ntStatus = IoQueryVolumeInformationFile(
                        pTConState->hFile,
                        pTConState->pAcb,
                        &pTConState->ioStatusBlock,
                        pTConState->pVolumeInfo,
                        pTConState->usBytesAllocated,
                        FileFsAttributeInformation);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            SrvReleaseTreeConnectStateAsync(pTConState);

            continue;
        }
        else if (ntStatus == STATUS_SUCCESS)
        {
            SrvReleaseTreeConnectStateAsync(pTConState);

            bContinue = FALSE;

            break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (bContinue);

    pFsAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)pTConState->pVolumeInfo;

    if (!pFsAttrInfo->FileSystemNameLength)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                    pFsAttrInfo->FileSystemNameLength + sizeof(wchar16_t),
                    (PVOID*)&pTConState->pwszNativeFileSystem);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy((PBYTE)pTConState->pwszNativeFileSystem,
           (PBYTE)pFsAttrInfo->FileSystemName,
           pFsAttrInfo->FileSystemNameLength);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTConState->pShareInfo->mutex);

    return ntStatus;

error:

    goto cleanup;
}

static
VOID
SrvReleaseTreeConnectStateHandle(
    HANDLE hTConState
    )
{
    SrvReleaseTreeConnectState((PSRV_TREE_CONNECT_STATE_SMB_V1)hTConState);
}

static
VOID
SrvReleaseTreeConnectState(
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState
    )
{
    if (InterlockedDecrement(&pTConState->refCount) == 0)
    {
        SrvFreeTreeConnectState(pTConState);
    }
}

static
VOID
SrvFreeTreeConnectState(
    PSRV_TREE_CONNECT_STATE_SMB_V1 pTConState
    )
{
    if (pTConState->pEcpList)
    {
        IoRtlEcpListFree(&pTConState->pEcpList);
    }

    if (pTConState->pAcb)
    {
        if (pTConState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pTConState->pAcb->AsyncCancelContext);
        }
    }

    // TODO: Free the following if set
    // pSecurityDescriptor;
    // pSecurityQOS;

    if (pTConState->fileName.FileName)
    {
        SrvFreeMemory(pTConState->fileName.FileName);
    }

    if (pTConState->pShareInfo)
    {
        SrvShareReleaseInfo(pTConState->pShareInfo);
    }

    if (pTConState->hFile)
    {
        IoCloseFile(pTConState->hFile);
    }

    if (pTConState->pszService2)
    {
        SrvFreeMemory(pTConState->pszService2);
    }

    if (pTConState->pwszNativeFileSystem)
    {
        SrvFreeMemory(pTConState->pwszNativeFileSystem);
    }

    if (pTConState->pVolumeInfo)
    {
        SrvFreeMemory(pTConState->pVolumeInfo);
    }

    if (pTConState->pIoSecContext)
    {
        IoSecurityDereferenceSecurityContext(&pTConState->pIoSecContext);
    }

    if (pTConState->bRemoveTreeFromSession)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvSessionRemoveTree(
                        pTConState->pSession,
                        pTConState->pTree->tid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove tid [%u] from session [uid=%u][code:%d]",
                            pTConState->pTree->tid,
                            pTConState->pSession->uid,
                            ntStatus2);
        }
    }

    if (pTConState->pSession)
    {
        SrvSessionRelease(pTConState->pSession);
    }

    if (pTConState->pTree)
    {
        SrvTreeRelease(pTConState->pTree);
    }

    if (pTConState->pMutex)
    {
        pthread_mutex_destroy(&pTConState->mutex);
    }

    SrvFreeMemory(pTConState);
}


