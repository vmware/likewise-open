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
    PSMB_SRV_SESSION pSession,
    USHORT           usSearchAttributes,
    PWSTR            pwszFilesystemPath,
    PWSTR            pwszFilePattern,
    BOOLEAN          bUseLongFilenames
    );

NTSTATUS
SrvProcessDelete(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE    pTree = NULL;
    PSMB_DELETE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PWSTR       pwszSearchPattern = NULL; // Do not free
    ULONG       ulOffset = 0;
    PSMB_DELETE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    USHORT      usPacketByteCount = 0;
    PWSTR       pwszFilesystemPath = NULL;
    PSMB_PACKET pSmbResponse = NULL;
    BOOLEAN     bInLock = FALSE;
    BOOLEAN     bUseLongFilenames = FALSE;

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

    ntStatus = WireUnmarshallDeleteRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
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

    ntStatus = SMBAllocateStringW(
                    pTree->pShareInfo->pwszPath,
                    &pwszFilesystemPath);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    if (pSmbRequest->pSMBHeader->flags2 & FLAG2_KNOWS_LONG_NAMES)
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

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_DELETE,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallDeleteResponse(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    &pResponseHeader,
                    &usPacketByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usPacketByteCount;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

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
        LwRtlMemoryFree(pwszFilesystemPath);
    }

    return ntStatus;

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
SrvDeleteFiles(
    PSMB_SRV_SESSION pSession,
    USHORT           usSearchAttributes,
    PWSTR            pwszFilesystemPath,
    PWSTR            pwszSearchPattern,
    BOOLEAN          bUseLongFilenames
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

    ntStatus = SrvBuildSearchPath(
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
            LwRtlMemoryFree(pData);
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
                LwRtlMemoryFree(pwszFilePath);
                pwszFilePath = NULL;
            }
            if (pwszFilename)
            {
                LwRtlMemoryFree(pwszFilename);
                pwszFilename = NULL;
            }

            if (bUseLongFilenames)
            {
                ntStatus = LW_RTL_ALLOCATE(
                                &pwszFilename,
                                WCHAR,
                                pResult->FileNameLength + sizeof(wchar16_t));
                BAIL_ON_NT_STATUS(ntStatus);

                memcpy((PBYTE)pwszFilename,
                       (PBYTE)pResult->FileName,
                       pResult->FileNameLength);
            }
            else
            {
                ntStatus = LW_RTL_ALLOCATE(
                                &pwszFilename,
                                WCHAR,
                                pResult->ShortNameLength + sizeof(wchar16_t));
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

	    CreateOptions = FILE_DELETE_ON_CLOSE;
	    if (pResult->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		    CreateOptions |= FILE_DIRECTORY_FILE;
	    } else {
		    CreateOptions |= FILE_NON_DIRECTORY_FILE;
	    }

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
                            0,
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

    ntStatus = STATUS_CANNOT_DELETE;

    goto cleanup;
}

