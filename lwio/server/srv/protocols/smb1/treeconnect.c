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
    PSRV_SHARE_INFO pShareInfo,
    PWSTR*          ppwszNativeFilesystem
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
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    BOOLEAN                    bRemoveTreeFromSession  = FALSE;
    PSRV_SHARE_INFO            pShareInfo = NULL;
    PWSTR                      pwszSharename = NULL;
    BOOLEAN                    bInLock = FALSE;
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PTREE_CONNECT_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE                        pszPassword   = NULL; // Do not free
    PBYTE                        pszService    = NULL; // Do not free
    PWSTR                        pwszPath      = NULL; // Do not free
    PLWIO_SRV_SESSION            pSession = NULL;

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

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pConnection->pHostinfo->mutex);

    ntStatus = SrvGetShareName(
                    pConnection->pHostinfo->pszHostname,
                    pConnection->pHostinfo->pszDomain,
                    pwszPath,
                    &pwszSharename);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->pHostinfo->mutex);

    ntStatus = SrvShareFindByName(
                    pConnection->pShareList,
                    pwszSharename,
                    &pShareInfo);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_BAD_NETWORK_NAME;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionCreateTree(
                    pSession,
                    pShareInfo,
                    &pCtxSmb1->pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    bRemoveTreeFromSession = TRUE;

    ntStatus = SrvBuildTreeConnectResponse(pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pConnection->pHostinfo->mutex);

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pShareInfo)
    {
        SrvShareReleaseInfo(pShareInfo);
    }

    if (pwszSharename)
    {
        SrvFreeMemory(pwszSharename);
    }

    return ntStatus;

error:

    if (bRemoveTreeFromSession)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvSessionRemoveTree(
                        pSession,
                        pCtxSmb1->pTree->tid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove tid [%u] from session [uid=%u][code:%d]",
                            pSmbRequest->pHeader->tid,
                            pSmbRequest->pHeader->uid,
                            ntStatus2);
        }

        SrvTreeRelease(pCtxSmb1->pTree);
        pCtxSmb1->pTree = NULL;
    }

    goto cleanup;
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
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PTREE_CONNECT_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;
    PSTR  pszService           = NULL;
    PWSTR pwszNativeFileSystem = NULL;

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_TREE_CONNECT_ANDX,
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

    pSmbResponse->pHeader->wordCount = 7;

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

    ntStatus = SrvGetMaximalShareAccessMask(
                    pCtxSmb1->pTree->pShareInfo,
                    &pResponseHeader->maximalShareAccessMask);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGetGuestShareAccessMask(
                    pCtxSmb1->pTree->pShareInfo,
                    &pResponseHeader->guestMaximalShareAccessMask);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGetServiceName(
                    pCtxSmb1->pTree->pShareInfo,
                    &pszService);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pCtxSmb1->pTree->pShareInfo->service == SHARE_SERVICE_DISK_SHARE)
    {
        ntStatus = SrvGetNativeFilesystem(
                        pCtxSmb1->pTree->pShareInfo,
                        &pwszNativeFileSystem);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = MarshallTreeConnectResponseData(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &usBytesUsed,
                    (const PBYTE)pszService,
                    pwszNativeFileSystem);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pResponseHeader->byteCount = usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pszService)
    {
        SrvFreeMemory(pszService);
    }

    if (pwszNativeFileSystem)
    {
        SrvFreeMemory(pwszNativeFileSystem);
    }

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
    PSRV_SHARE_INFO pShareInfo,
    PWSTR* ppwszNativeFilesystem
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;
    IO_FILE_HANDLE hFile = NULL;
    IO_FILE_NAME   fileName = {0};
    PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock = NULL;
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    IO_STATUS_BLOCK     ioStatusBlock = {0};
    PWSTR    pwszNativeFilesystem = NULL;
    PBYTE    pVolumeInfo = NULL;
    USHORT   usBytesAllocated = 0;
    PFILE_FS_ATTRIBUTE_INFORMATION pFsAttrInfo = NULL;
    PIO_CREATE_SECURITY_CONTEXT pIoSecContext = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareInfo->mutex);

    fileName.FileName = pShareInfo->pwszPath;

    ntStatus = IoSecurityCreateSecurityContextFromUidGid(&pIoSecContext,
                             0,
                             0,
                             NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pIoSecContext,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQOS,
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

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    usBytesAllocated = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + 256 * sizeof(wchar16_t);

    ntStatus = SrvAllocateMemory(usBytesAllocated, (PVOID*)&pVolumeInfo);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        ntStatus = IoQueryVolumeInformationFile(
                        hFile,
                        NULL,
                        &ioStatusBlock,
                        pVolumeInfo,
                        usBytesAllocated,
                        FileFsAttributeInformation);
        if (ntStatus == STATUS_SUCCESS)
        {
            break;
        }
        else if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewSize = usBytesAllocated + 256 * sizeof(wchar16_t);

            ntStatus = SMBReallocMemory(
                            pVolumeInfo,
                            (PVOID*)&pVolumeInfo,
                            usNewSize);
            BAIL_ON_NT_STATUS(ntStatus);

            usBytesAllocated = usNewSize;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (TRUE);

    pFsAttrInfo = (PFILE_FS_ATTRIBUTE_INFORMATION)pVolumeInfo;

    if (!pFsAttrInfo->FileSystemNameLength)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateStringW(
                    pFsAttrInfo->FileSystemName,
                    &pwszNativeFilesystem);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszNativeFilesystem = pwszNativeFilesystem;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareInfo->mutex);

    if (pVolumeInfo)
    {
        SrvFreeMemory(pVolumeInfo);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    if (pIoSecContext)
    {
        IoSecurityDereferenceSecurityContext(&pIoSecContext);
    }


    return ntStatus;

error:

    *ppwszNativeFilesystem = NULL;

    goto cleanup;
}


