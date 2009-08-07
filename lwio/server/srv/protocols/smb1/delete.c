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
SrvDeleteFiles(
    PLWIO_SRV_SESSION pSession,
    USHORT            usSearchAttributes,
    PWSTR             pwszFilesystemPath,
    PWSTR             pwszFilePattern,
    BOOLEAN           bUseLongFilenames
    );

static
NTSTATUS
SrvBuildDeleteResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessDelete(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PSMB_DELETE_REQUEST_HEADER  pRequestHeader    = NULL; // Do not free
    PWSTR                       pwszSearchPattern = NULL; // Do not free
    PLWIO_SRV_SESSION           pSession = NULL;
    PLWIO_SRV_TREE              pTree    = NULL;
    PWSTR       pwszFilesystemPath = NULL;
    BOOLEAN     bInLock = FALSE;
    BOOLEAN     bUseLongFilenames = FALSE;

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

    ntStatus = WireUnmarshallDeleteRequest(
                    pBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pRequestHeader,
                    &pwszSearchPattern);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pwszSearchPattern || !*pwszSearchPattern)
    {
        ntStatus = STATUS_CANNOT_DELETE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SrvAllocateStringW(
                    pTree->pShareInfo->pwszPath,
                    &pwszFilesystemPath);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    if (pSmbRequest->pHeader->flags2 & FLAG2_KNOWS_LONG_NAMES)
    {
        bUseLongFilenames = TRUE;
    }

    ntStatus = SrvDeleteFiles(
                    pSession,
                    pRequestHeader->usSearchAttributes,
                    pwszFilesystemPath,
                    pwszSearchPattern,
                    bUseLongFilenames);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildDeleteResponse(pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pTree)
    {
        LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pwszFilesystemPath)
    {
        SrvFreeMemory(pwszFilesystemPath);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvDeleteFiles(
    PLWIO_SRV_SESSION pSession,
    USHORT            usSearchAttributes,
    PWSTR             pwszFilesystemPath,
    PWSTR             pwszSearchPattern,
    BOOLEAN           bUseLongFilenames
    )
{
    NTSTATUS ntStatus = 0;
    ULONG    ulSearchStorageType = 0;
    HANDLE   hSearchSpace = NULL;
    USHORT   usSearchId = 0;
    SMB_INFO_LEVEL infoLevel = SMB_FIND_FILE_BOTH_DIRECTORY_INFO;
    BOOLEAN  bEndOfSearch = FALSE;
    USHORT   usDesiredSearchCount = 10;
    USHORT   usMaxDataCount = UINT16_MAX;
    USHORT   usDataOffset = 0;
    PBYTE    pData = NULL;
    USHORT   usDataLen = 0;
    USHORT   usSearchResultCount = 0;
    PWSTR    pwszFilePath = NULL;
    PWSTR    pwszFilename = NULL;
    IO_FILE_HANDLE hFile = NULL;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    PWSTR     pwszFilesystemPath2 = NULL;
    PWSTR     pwszSearchPattern2 = NULL;

    ntStatus = SrvFinderBuildSearchPath(
                    pwszFilesystemPath,
                    pwszSearchPattern,
                    &pwszFilesystemPath2,
                    &pwszSearchPattern2);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFinderCreateSearchSpace(
                    pSession->pIoSecurityContext,
                    pSession->hFinderRepository,
                    pwszFilesystemPath2,
                    pwszSearchPattern2,
                    usSearchAttributes,
                    ulSearchStorageType,
                    infoLevel,
                    bUseLongFilenames,
                    &hSearchSpace,
                    &usSearchId);
    BAIL_ON_NT_STATUS(ntStatus);

    do
    {
        USHORT   iResult = 0;
        BOOLEAN  bReturnSingleEntry = FALSE;
        BOOLEAN  bRestartScan = FALSE;
        IO_STATUS_BLOCK ioStatusBlock = {0};
        PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER pResult = NULL;
        IO_FILE_NAME fileName = {0};
        PVOID  pSecurityDescriptor = NULL;
        PVOID  pSecurityQOS = NULL;

        if (pData)
        {
            SrvFreeMemory(pData);
            pData = NULL;
        }

        ntStatus = SrvFinderGetSearchResults(
                        hSearchSpace,
                        bReturnSingleEntry,
                        bRestartScan,
                        usDesiredSearchCount,
                        usMaxDataCount,
                        usDataOffset,
                        &pData,
                        &usDataLen,
                        &usSearchResultCount,
                        &bEndOfSearch);
        BAIL_ON_NT_STATUS(ntStatus);

        pResult = (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER)pData;

        for (; iResult < usSearchResultCount; iResult++)
        {
            if (pwszFilePath)
            {
                SrvFreeMemory(pwszFilePath);
                pwszFilePath = NULL;
            }
            if (pwszFilename)
            {
                SrvFreeMemory(pwszFilename);
                pwszFilename = NULL;
            }

            if (bUseLongFilenames)
            {
                ntStatus = SrvAllocateMemory(
                                pResult->FileNameLength + sizeof(wchar16_t),
                                (PVOID*)&pwszFilename);
                BAIL_ON_NT_STATUS(ntStatus);

                memcpy((PBYTE)pwszFilename,
                       (PBYTE)pResult->FileName,
                       pResult->FileNameLength);
            }
            else
            {
                ntStatus = SrvAllocateMemory(
                                pResult->ShortNameLength + sizeof(wchar16_t),
                                (PVOID*)&pwszFilename);
                BAIL_ON_NT_STATUS(ntStatus);

                memcpy((PBYTE)pwszFilename,
                       (PBYTE)pResult->ShortName,
                       pResult->ShortNameLength);
            }

            ntStatus = SrvBuildFilePath(
                            pwszFilesystemPath2,
                            pwszFilename,
                            &pwszFilePath);
            BAIL_ON_NT_STATUS(ntStatus);

            fileName.FileName = pwszFilePath;

            if (pResult->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                ntStatus = STATUS_FILE_IS_A_DIRECTORY;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            CreateOptions |=  FILE_DELETE_ON_CLOSE|FILE_NON_DIRECTORY_FILE;

            ntStatus = IoCreateFile(
                            &hFile,
                            NULL,
                            &ioStatusBlock,
                            pSession->pIoSecurityContext,
                            &fileName,
                            pSecurityDescriptor,
                            pSecurityQOS,
                            DELETE,
                            0,
                            FILE_ATTRIBUTE_NORMAL,
                            FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,
                            FILE_OPEN,
                            CreateOptions,
                            NULL,
                            0,
                            NULL);
            BAIL_ON_NT_STATUS(ntStatus);

            IoCloseFile(hFile);
            hFile = NULL;

            if (pResult->NextEntryOffset)
            {
                PBYTE pTmp = (PBYTE)pResult + pResult->NextEntryOffset;

                pResult = (PSMB_FIND_FILE_BOTH_DIRECTORY_INFO_HEADER)pTmp;
            }
        }

    } while (!bEndOfSearch);

cleanup:

    if (hSearchSpace)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvFinderCloseSearchSpace(
                        pSession->hFinderRepository,
                        usSearchId);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to close search space [Id:%d][code:%d]",
                          usSearchId,
                          ntStatus2);
        }

        SrvFinderReleaseSearchSpace(hSearchSpace);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    RTL_FREE(&pData);
    RTL_FREE(&pwszFilename);
    RTL_FREE(&pwszFilePath);
    RTL_FREE(&pwszFilesystemPath2);
    RTL_FREE(&pwszSearchPattern2);

    return ntStatus;

error:

    /* Have to do some error mapping here to match WinXP */

    switch (ntStatus) {
    case STATUS_FILE_IS_A_DIRECTORY:
        break;

    case STATUS_OBJECT_NAME_NOT_FOUND:
    case STATUS_NO_SUCH_FILE:
        ntStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        break;

    default:
        ntStatus = STATUS_CANNOT_DELETE;
        break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildDeleteResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PSMB_DELETE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_DELETE,
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

    ntStatus = WireMarshallDeleteResponse(
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

