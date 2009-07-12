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
    PLWIO_SRV_CONNECTION pConnection,
    PSMB2_MESSAGE        pSmbRequest,
    PIO_STATUS_BLOCK     pIoStatusBlock,
    PLWIO_SRV_TREE_2     pTree,
    PLWIO_SRV_FILE_2     pFile,
    PSMB_PACKET          pSmbResponse
    );

NTSTATUS
SrvProcessCreate_SMB_V2(
    IN     PLWIO_SRV_CONNECTION pConnection,
    IN     PSMB2_MESSAGE        pSmbRequest,
    IN OUT PSMB_PACKET          pSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree = NULL;
    PLWIO_SRV_FILE_2    pFile = NULL;
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

    ntStatus = SMB2UnmarshalCreateRequest(
                    pSmbRequest,
                    &pCreateRequestHeader,
                    &wszFileName,
                    &pCreateContexts,
                    &ulNumContexts);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnection2FindSession(
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindTree(
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

    ntStatus = SrvAllocateMemory(
                    sizeof(IO_FILE_NAME),
                    (PVOID*)&pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildFilePath(
                    pTree->pShareInfo->pwszPath,
                    pwszFilename,
                    &pFilename->FileName);
    BAIL_ON_NT_STATUS(ntStatus);

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
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    bRemoveFileFromTree = TRUE;

    // TODO: Execute create contexts and add results

    ntStatus = SrvBuildCreateResponse_SMB_V2(
                    pConnection,
                    pSmbRequest,
                    &ioStatusBlock,
                    pTree,
                    pFile,
                    pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pFile)
    {
        SrvFile2Release(pFile);
    }

    if (pTree)
    {
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
                        pFile->ullFid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove file from tree [Tid:%u][Fid:%ul][code:%d]",
                           pTree->ulTid,
                           pFile->ullFid,
                           ntStatus2);
        }
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildCreateResponse_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB2_MESSAGE        pSmbRequest,
    PIO_STATUS_BLOCK     pIoStatusBlock,
    PLWIO_SRV_TREE_2     pTree,
    PLWIO_SRV_FILE_2     pFile,
    PSMB_PACKET          pSmbResponse
    )
{
    NTSTATUS                     ntStatus = 0;
    FILE_BASIC_INFORMATION       fileBasicInfo = {0};
    FILE_STANDARD_INFORMATION    fileStdInfo = {0};
    IO_STATUS_BLOCK              ioStatusBlock = {0};
    PSMB2_CREATE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBufferRef = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    PBYTE pOutBuffer = pOutBufferRef;
    ULONG ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    ULONG ulOffset    = pSmbResponse->bufferUsed - sizeof(NETBIOS_HEADER);
    ULONG ulBytesUsed = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = IoQueryInformationFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileBasicInfo,
                    sizeof(fileBasicInfo),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoQueryInformationFile(
                    pFile->hFile,
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
                    pTree->ulTid,
                    pSmbRequest->pHeader->ullSessionId,
                    STATUS_SUCCESS,
                    TRUE,
                    NULL,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    pOutBuffer += ulBytesUsed;
    ulOffset += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;

    if (ulBytesAvailable < sizeof(SMB2_CREATE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_CREATE_RESPONSE_HEADER)pOutBuffer;
    ulTotalBytesUsed += sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulBytesUsed += sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_CREATE_RESPONSE_HEADER);
    ulOffset += sizeof(SMB2_CREATE_RESPONSE_HEADER);

    pResponseHeader->fid.ullVolatileId = pFile->ullFid;
    pResponseHeader->ulCreateAction = pIoStatusBlock->CreateResult;
    pResponseHeader->ullCreationTime = fileBasicInfo.CreationTime;
    pResponseHeader->ullLastAccessTime = fileBasicInfo.LastAccessTime;
    pResponseHeader->ullLastWriteTime = fileBasicInfo.LastWriteTime;
    pResponseHeader->ullLastChangeTime = fileBasicInfo.ChangeTime;
    pResponseHeader->ulFileAttributes = fileBasicInfo.FileAttributes;
    pResponseHeader->ullAllocationSize = fileStdInfo.AllocationSize;
    pResponseHeader->ullEndOfFile = fileStdInfo.EndOfFile;
    pResponseHeader->usLength = ulBytesUsed + 1;

    pSmbResponse->bufferUsed += ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        memset(pOutBufferRef, 0, ulTotalBytesUsed);
    }

    goto cleanup;
}
