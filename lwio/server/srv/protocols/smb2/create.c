/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        create.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Create
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildCreateResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext,
    PIO_STATUS_BLOCK  pIoStatusBlock
    );

NTSTATUS
SrvProcessCreate_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree = NULL;
    IO_FILE_HANDLE      hFile = NULL;
    IO_STATUS_BLOCK     ioStatusBlock = {0};
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    PIO_FILE_NAME       pFilename = NULL;
    PIO_ECP_LIST        pEcpList = NULL;
    BOOLEAN             bRemoveFileFromTree = FALSE;
    PWSTR               pwszFilename = NULL;
    wchar16_t           wszEmpty[] = {0};
    PIO_ASYNC_CONTROL_BLOCK     pAsyncControlBlock = NULL;
    PSMB2_CREATE_REQUEST_HEADER pCreateRequestHeader = NULL;// Do not free
    UNICODE_STRING              wszFileName = {0}; // Do not free
    PSRV_CREATE_CONTEXT         pCreateContexts = NULL;
    ULONG                       ulNumContexts = 0;
    BOOLEAN bTreeInLock = FALSE;

    if (pCtxSmb2->pFile)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMB2UnmarshalCreateRequest(
                    pSmbRequest,
                    &pCreateRequestHeader,
                    &wszFileName,
                    &pCreateContexts,
                    &ulNumContexts);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnection2FindSession_SMB_V2(
                    pCtxSmb2,
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindTree_SMB_V2(
                    pCtxSmb2,
                    pSession,
                    pSmbRequest->pHeader->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!wszFileName.Length)
    {
        wszFileName.Buffer = &wszEmpty[0];
        wszFileName.Length = 0;
        wszFileName.MaximumLength = sizeof(wszEmpty);
    }

    ntStatus = SrvAllocateMemory(
                    wszFileName.Length + sizeof(wchar16_t),
                    (PVOID*)&pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    if (wszFileName.Length)
    {
        memcpy((PBYTE)pwszFilename,
               (PBYTE)wszFileName.Buffer,
               wszFileName.Length);
    }

    ntStatus = SrvAllocateMemory(sizeof(IO_FILE_NAME), (PVOID*)&pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pTree->mutex);

    ntStatus = SrvBuildFilePath(
                    pTree->pShareInfo->pwszPath,
                    pwszFilename,
                    &pFilename->FileName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTree->mutex);

    /* For named pipes, we need to pipe some extra data into the npfs driver
     *  - Session key
     *  - Client principal name
     *  - Client address
     */
    if (SrvTree2IsNamedPipe(pTree))
    {
        ntStatus = IoRtlEcpListAllocate(&pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeSessionKey(
                       pConnection,
                       pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSession2GetNamedPipeClientPrincipal(
                       pSession,
                       pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeClientAddress(
                       pConnection,
                       pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSession->pIoSecurityContext,
                    pFilename,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    pCreateRequestHeader->ulDesiredAccess,
                    0LL,
                    pCreateRequestHeader->ulFileAttributes,
                    pCreateRequestHeader->ulShareAccess,
                    pCreateRequestHeader->ulCreateDisposition,
                    pCreateRequestHeader->ulCreateOptions,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    pEcpList);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2CreateFile(
                    pTree,
                    pwszFilename,
                    &hFile,
                    &pFilename,
                    pCreateRequestHeader->ulDesiredAccess,
                    0LL,
                    pCreateRequestHeader->ulFileAttributes,
                    pCreateRequestHeader->ulShareAccess,
                    pCreateRequestHeader->ulCreateDisposition,
                    pCreateRequestHeader->ulCreateOptions,
                    &pCtxSmb2->pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    bRemoveFileFromTree = TRUE;

    // TODO: Execute create contexts and add results

    ntStatus = SrvBuildCreateResponse_SMB_V2(pExecContext, &ioStatusBlock);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pTree)
    {
        LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pTree->mutex);

        SrvTree2Release(pTree);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    if (pEcpList)
    {
        IoRtlEcpListFree(&pEcpList);
    }

    SRV_SAFE_FREE_MEMORY(pwszFilename);
    SRV_SAFE_FREE_MEMORY(pCreateContexts);

    return ntStatus;

error:

    if (pFilename)
    {
        SRV_SAFE_FREE_MEMORY (pFilename->FileName);
        SrvFreeMemory(pFilename);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    if (bRemoveFileFromTree)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvTree2RemoveFile(
                        pTree,
                        pCtxSmb2->pFile->ullFid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove file from tree [Tid:%u][Fid:%ul][code:%d]",
                           pCtxSmb2->pTree->ulTid,
                           pCtxSmb2->pFile->ullFid,
                           ntStatus2);
        }

        SrvFile2Release(pCtxSmb2->pFile);
        pCtxSmb2->pFile = NULL;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildCreateResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext,
    PIO_STATUS_BLOCK  pIoStatusBlock
    )
{
    NTSTATUS                     ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    FILE_BASIC_INFORMATION       fileBasicInfo = {0};
    FILE_STANDARD_INFORMATION    fileStdInfo = {0};
    IO_STATUS_BLOCK              ioStatusBlock = {0};
    PSMB2_CREATE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset    = 0;
    ULONG ulBytesUsed = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = IoQueryInformationFile(
                    pCtxSmb2->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileBasicInfo,
                    sizeof(fileBasicInfo),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoQueryInformationFile(
                    pCtxSmb2->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileStdInfo,
                    sizeof(fileStdInfo),
                    FileStandardInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_CREATE,
                    0,
                    1,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pCtxSmb2->pTree->ulTid,
                    pCtxSmb2->pSession->ullUid,
                    STATUS_SUCCESS,
                    TRUE,
                    pSmbRequest->pHeader->ulFlags & SMB2_FLAGS_RELATED_OPERATION,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_CREATE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_CREATE_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulBytesUsed       = sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_CREATE_RESPONSE_HEADER);

    pResponseHeader->fid.ullVolatileId = pCtxSmb2->pFile->ullFid;
    pResponseHeader->ulCreateAction    = pIoStatusBlock->CreateResult;
    pResponseHeader->ullCreationTime   = fileBasicInfo.CreationTime;
    pResponseHeader->ullLastAccessTime = fileBasicInfo.LastAccessTime;
    pResponseHeader->ullLastWriteTime  = fileBasicInfo.LastWriteTime;
    pResponseHeader->ullLastChangeTime = fileBasicInfo.ChangeTime;
    pResponseHeader->ulFileAttributes  = fileBasicInfo.FileAttributes;
    pResponseHeader->ullAllocationSize = fileStdInfo.AllocationSize;
    pResponseHeader->ullEndOfFile      = fileStdInfo.EndOfFile;
    pResponseHeader->usLength          = ulBytesUsed + 1;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader      = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}
