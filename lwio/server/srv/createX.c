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

static
NTSTATUS
SrvSetNamedPipeClientPrincipal(
    PSMB_SRV_SESSION pSession,
    PIO_ECP_LIST* ppEcpList
    );

static
NTSTATUS
SrvSetNamedPipeClientAddress(
    PSMB_SRV_CONNECTION pConnection,
    PIO_ECP_LIST* ppEcpList
    );

static
BOOLEAN
SrvIsNamedPipe(
    PSMB_SRV_TREE pTree
    );

static
NTSTATUS
SrvSetNamedPipeSessionKey(
    PSMB_SRV_CONNECTION pConnection,
    PIO_ECP_LIST* ppEcpList
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

    ntStatus = WireUnmarshallCreateFileRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->bufferLen - pSmbRequest->bufferUsed,
                    (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader,
                    &pRequestHeader,
                    &pwszFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: Handle root fids
    ntStatus = SMBAllocateMemory(
                    sizeof(IO_FILE_NAME),
                    (PVOID*)&pFilename);
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
    if (SrvIsNamedPipe(pTree))
    {
        ntStatus = SrvSetNamedPipeSessionKey(
            pConnection,
            &pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSetNamedPipeClientPrincipal(
            pSession,
            &pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSetNamedPipeClientAddress(
            pConnection,
            &pEcpList);
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
        SMB_SAFE_FREE_MEMORY(pFilename->FileName);
        SMBFreeMemory(pFilename);
    }

    if (hFile)
    {
        IoCloseFile(hFile);
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

    if (SrvIsNamedPipe(pTree))
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

static
BOOLEAN
SrvIsNamedPipe(
    PSMB_SRV_TREE pTree
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    bResult = (pTree->pShareInfo->service == SHARE_SERVICE_NAMED_PIPE);

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    return bResult;
}

static
NTSTATUS
SrvSetNamedPipeSessionKey(
    PSMB_SRV_CONNECTION pConnection,
    PIO_ECP_LIST* ppEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PBYTE pSessionKey = pConnection->pSessionKey;
    ULONG ulSessionKeyLength = pConnection->ulSessionKeyLength;

    if (pSessionKey != NULL)
    {
        if (!*ppEcpList)
        {
            ntStatus = IoRtlEcpListAllocate(ppEcpList);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = IoRtlEcpListInsert(*ppEcpList,
                                      IO_ECP_TYPE_SESSION_KEY,
                                      pSessionKey,
                                      ulSessionKeyLength,
                                      NULL);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvSetNamedPipeClientPrincipal(
    PSMB_SRV_SESSION pSession,
    PIO_ECP_LIST* ppEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszClientPrincipalName = pSession->pszClientPrincipalName;
    ULONG ulEcpLength = strlen(pszClientPrincipalName) + 1;

    if (!*ppEcpList)
    {
        ntStatus = IoRtlEcpListAllocate(ppEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoRtlEcpListInsert(*ppEcpList,
                                  IO_ECP_TYPE_PEER_PRINCIPAL,
                                  pszClientPrincipalName,
                                  ulEcpLength,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvSetNamedPipeClientAddress(
    PSMB_SRV_CONNECTION pConnection,
    PIO_ECP_LIST* ppEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PULONG pAddr = &pConnection->pSocket->cliaddr.sin_addr.s_addr;
    ULONG ulAddrLength = sizeof(pConnection->pSocket->cliaddr.sin_addr.s_addr);

    if (!*ppEcpList)
    {
        ntStatus = IoRtlEcpListAllocate(ppEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoRtlEcpListInsert(*ppEcpList,
                                  IO_ECP_TYPE_PEER_ADDRESS,
                                  pAddr,
                                  ulAddrLength,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}
