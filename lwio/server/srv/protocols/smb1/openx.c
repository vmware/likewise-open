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
SrvBuildOpenResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    PIO_STATUS_BLOCK  pIoStatusBlock
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
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PLWIO_SRV_SESSION          pSession = NULL;
    PLWIO_SRV_TREE             pTree    = NULL;
    BOOLEAN                    bRemoveFileFromTree = FALSE;
    IO_FILE_HANDLE             hFile = NULL;
    IO_STATUS_BLOCK            ioStatusBlock = {0};
    PVOID                      pSecurityDescriptor = NULL;
    PVOID                      pSecurityQOS = NULL;
    PIO_FILE_NAME              pFilename = NULL;
    PIO_ECP_LIST               pEcpList = NULL;
    USHORT                     usCreateDisposition = 0;
    USHORT                     usCreateOptions = FILE_NON_DIRECTORY_FILE;
    USHORT                     usShareAccess = 0;
    PIO_ASYNC_CONTROL_BLOCK    pAsyncControlBlock = NULL;
    ACCESS_MASK                DesiredAccessMask = 0;
    POPEN_REQUEST_HEADER       pRequestHeader = NULL; // Do not free
    PWSTR                      pwszFilename = NULL; // Do not free
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;

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

    // TODO: Handle root fids
    ntStatus = SrvAllocateMemory(sizeof(IO_FILE_NAME), (PVOID*)&pFilename);
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

    usShareAccess = (FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE);

    switch (pRequestHeader->usDesiredAccess & 0x70)
    {
        case 0x00: /* compatibility mode */

            break;

        case 0x10: /* deny read/write/execute (exclusive) */

            usShareAccess &= (USHORT)~FILE_SHARE_READ;
            usShareAccess &= (USHORT)~FILE_SHARE_WRITE;

            break;

        case 0x20: /* deny write */

            usShareAccess &= (USHORT)~FILE_SHARE_WRITE;

            break;

        case 0x30: /* deny read/execute */

            usShareAccess &= (USHORT)~FILE_SHARE_READ;

            break;

        case 0x40: /* deny none */

            break;
    }

    /* action to take if the file exists */
    switch (pRequestHeader->usOpenFunction)
    {
        case 0x0001: /* Open file */
            usCreateDisposition = FILE_OPEN;
            break;
        case 0x0002: /* truncate file */
            usCreateDisposition = FILE_OVERWRITE;
            break;
        case 0x0010: /* create new file */
            usCreateDisposition = FILE_CREATE;
            break;
        case 0x0011: /* open or create file */
            usCreateDisposition = FILE_OPEN_IF;
            break;
        case 0x0012: /* create or truncate */
            usCreateDisposition = FILE_OVERWRITE_IF;
            break;
        default:
            ntStatus = STATUS_INVALID_DISPOSITION;
            BAIL_ON_NT_STATUS(ntStatus);
            break;
    }

    /* action to take if the file exists */
    switch (pRequestHeader->usOpenFunction & 0x7)
    {
        case 0x00:
            DesiredAccessMask = GENERIC_READ;
        case 0x01:
            DesiredAccessMask = GENERIC_WRITE;
        case 0x02:
            DesiredAccessMask = GENERIC_READ | GENERIC_WRITE;
            break;
        case 0x03:
            DesiredAccessMask = GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE;
            break;
        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
            break;
    }

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSession->pIoSecurityContext,
                    pFilename,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    DesiredAccessMask,
                    pRequestHeader->ulAllocationSize,
                    pRequestHeader->usFileAttributes,
                    usShareAccess,
                    usCreateDisposition,
                    usCreateOptions,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    pEcpList);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeCreateFile(
                    pTree,
                    pwszFilename,
                    &hFile,
                    &pFilename,
                    pRequestHeader->usDesiredAccess,
                    pRequestHeader->ulAllocationSize,
                    pRequestHeader->usFileAttributes,
                    usShareAccess,
                    usCreateDisposition,
                    usCreateOptions,
                    &pCtxSmb1->pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    bRemoveFileFromTree = TRUE;

    ntStatus = SrvBuildOpenResponse(pExecContext, &ioStatusBlock);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

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

    return ntStatus;

error:

    if (pFilename)
    {
        if (pFilename->FileName)
        {
            SrvFreeMemory(pFilename->FileName);
        }

        SrvFreeMemory(pFilename);
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
                        pCtxSmb1->pFile->fid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove file from tree [Tid:%d][Fid:%d][code:%d]",
                          pTree->tid,
                          pCtxSmb1->pFile->fid,
                          ntStatus2);
        }

        SrvFileRelease(pCtxSmb1->pFile);
        pCtxSmb1->pFile = NULL;
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildOpenResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    PIO_STATUS_BLOCK  pIoStatusBlock
    )
{
    NTSTATUS                    ntStatus          = 0;
    FILE_BASIC_INFORMATION      fileBasicInfo     = {0};
    FILE_STANDARD_INFORMATION   fileStdInfo       = {0};
    FILE_PIPE_INFORMATION       filePipeInfo      = {0};
    FILE_PIPE_LOCAL_INFORMATION filePipeLocalInfo = {0};
    IO_STATUS_BLOCK             ioStatusBlock     = {0};
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

    ntStatus = IoQueryInformationFile(
                    pCtxSmb1->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileBasicInfo,
                    sizeof(fileBasicInfo),
                    FileBasicInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoQueryInformationFile(
                    pCtxSmb1->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &fileStdInfo,
                    sizeof(fileStdInfo),
                    FileStandardInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_OPEN_ANDX,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
                    SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
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

    pSmbResponse->pHeader->wordCount = 15;

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


    pResponseHeader->usFid = pCtxSmb1->pFile->fid;
    pResponseHeader->ulServerFid = pCtxSmb1->pFile->fid;
    pResponseHeader->usOpenAction = pIoStatusBlock->CreateResult;
    // TODO:
    // pResponseHeader->usGrantedAccess = 0;

    ntStatus = WireNTTimeToSMBDateTime(
                    fileBasicInfo.LastWriteTime,
                    &pResponseHeader->lastWriteDate,
                    &pResponseHeader->lastWriteTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->ulDataSize = SMB_MIN(UINT32_MAX, fileStdInfo.EndOfFile);

    if (SrvTreeIsNamedPipe(pCtxSmb1->pTree))
    {
        ntStatus = IoQueryInformationFile(
                        pCtxSmb1->pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        &filePipeInfo,
                        sizeof(filePipeInfo),
                        FilePipeInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoQueryInformationFile(
                        pCtxSmb1->pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        &filePipeLocalInfo,
                        sizeof(filePipeLocalInfo),
                        FilePipeLocalInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvMarshallPipeInfo(
                        &filePipeInfo,
                        &filePipeLocalInfo,
                        &pResponseHeader->usDeviceState);
        BAIL_ON_NT_STATUS(ntStatus);

        pResponseHeader->usFileType = (USHORT)filePipeInfo.ReadMode;
    }
    else
    {
        pResponseHeader->usFileType = 0;

        // TODO: Get these values from the driver
        pResponseHeader->usDeviceState = (SMB_DEVICE_STATE_NO_EAS |
                                          SMB_DEVICE_STATE_NO_SUBSTREAMS |
                                          SMB_DEVICE_STATE_NO_REPARSE_TAG);
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



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
