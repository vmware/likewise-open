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
SrvBuildNTCreateResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PIO_STATUS_BLOCK    pIoStatusBlock,
    PSMB_SRV_TREE       pTree,
    PSMB_SRV_FILE       pFile,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessNTCreateAndX(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET         pSmbRequest = pContext->pRequest;
    PSMB_PACKET         pSmbResponse = NULL;
    PSMB_SRV_SESSION    pSession = NULL;
    PSMB_SRV_TREE       pTree = NULL;
    PSMB_SRV_FILE       pFile = NULL;
    BOOLEAN             bRemoveFileFromTree = FALSE;
    PCREATE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PWSTR               pwszFilename = NULL; // Do not free
    IO_FILE_HANDLE      hFile = NULL;
    PIO_ASYNC_CONTROL_BLOCK pAsyncControlBlock = NULL;
    IO_STATUS_BLOCK     ioStatusBlock = {0};
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PVOID               pSecurityDescriptor = NULL;
    PVOID               pSecurityQOS = NULL;
    PIO_FILE_NAME       pFilename = NULL;
    PIO_ECP_LIST        pEcpList = NULL;
    ULONG               ulOffset = 0;

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

    // TODO: Handle root fids
    ntStatus = LW_RTL_ALLOCATE(
                    &pFilename,
                    IO_FILE_NAME,
                    sizeof(IO_FILE_NAME));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildFilePath(
                    pTree->pShareInfo->pwszPath,
                    pwszFilename,
                    &pFilename->FileName);
    BAIL_ON_NT_STATUS(ntStatus);

    /* For named pipes, we need to pipe some extra data into the npfs driver:
     *  - Session key
     *  - Client principal name
     *  - Client address
     */
    if (SrvTreeIsNamedPipe(pTree))
    {
        ntStatus = IoRtlEcpListAllocate(&pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeSessionKey(
                       pConnection,
                       pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSessionGetNamedPipeClientPrincipal(
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
                    pSecurityContext,
                    pFilename,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    pRequestHeader->desiredAccess,
                    pRequestHeader->allocationSize,
                    pRequestHeader->extFileAttributes,
                    pRequestHeader->shareAccess,
                    pRequestHeader->createDisposition,
                    pRequestHeader->createOptions,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    pEcpList);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeCreateFile(
                    pTree,
                    pwszFilename,
                    &hFile,
                    &pFilename,
                    pRequestHeader->desiredAccess,
                    pRequestHeader->allocationSize,
                    pRequestHeader->extFileAttributes,
                    pRequestHeader->shareAccess,
                    pRequestHeader->createDisposition,
                    pRequestHeader->createOptions,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    bRemoveFileFromTree = TRUE;

    ntStatus = SrvBuildNTCreateResponse(
                    pConnection,
                    pSmbRequest,
                    &ioStatusBlock,
                    pTree,
                    pFile,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pEcpList)
    {
        IoRtlEcpListFree(&pEcpList);
    }

    return (ntStatus);

error:

    *ppSmbResponse = NULL;

    if (pFilename)
    {
        if (pFilename->FileName)
        {
            LwRtlMemoryFree(pFilename->FileName);
        }

        LwRtlMemoryFree(pFilename);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    if (bRemoveFileFromTree)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvTreeRemoveFile(
                        pTree,
                        pFile->fid);
        if (ntStatus2)
        {
            SMB_LOG_ERROR("Failed to remove file from tree [Tid:%d][Fid:%d][code:%d]",
                          pTree->tid,
                          pFile->fid,
                          ntStatus2);
        }
    }

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
SrvBuildNTCreateResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PIO_STATUS_BLOCK    pIoStatusBlock,
    PSMB_SRV_TREE       pTree,
    PSMB_SRV_FILE       pFile,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS    ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PCREATE_RESPONSE_HEADER pResponseHeader = NULL;
    FILE_BASIC_INFORMATION fileBasicInfo = {0};
    FILE_STANDARD_INFORMATION fileStdInfo = {0};
    FILE_PIPE_INFORMATION filePipeInfo = {0};
    FILE_PIPE_LOCAL_INFORMATION filePipeLocalInfo = {0};
    IO_STATUS_BLOCK ioStatusBlock = {0};

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
                COM_NT_CREATE_ANDX,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 26;

    pResponseHeader = (PCREATE_RESPONSE_HEADER)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(CREATE_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(CREATE_RESPONSE_HEADER);

    pResponseHeader->fid = pFile->fid;
    pResponseHeader->createAction = pIoStatusBlock->CreateResult;
    pResponseHeader->creationTime = fileBasicInfo.CreationTime;
    pResponseHeader->lastAccessTime = fileBasicInfo.LastAccessTime;
    pResponseHeader->lastWriteTime = fileBasicInfo.LastWriteTime;
    pResponseHeader->changeTime = fileBasicInfo.ChangeTime;
    pResponseHeader->extFileAttributes = fileBasicInfo.FileAttributes;
    pResponseHeader->allocationSize = fileStdInfo.AllocationSize;
    pResponseHeader->endOfFile = fileStdInfo.EndOfFile;

    if (SrvTreeIsNamedPipe(pTree))
    {
        ntStatus = IoQueryInformationFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        &filePipeInfo,
                        sizeof(filePipeInfo),
                        FilePipeInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoQueryInformationFile(
                        pFile->hFile,
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
        pResponseHeader->deviceState = SMB_DEVICE_STATE_NO_EAS | SMB_DEVICE_STATE_NO_SUBSTREAMS | SMB_DEVICE_STATE_NO_REPARSE_TAG;
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

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

