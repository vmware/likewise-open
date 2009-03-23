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

NTSTATUS
SrvProcessDeleteDirectory(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PSMB_PACKET pSmbResponse = NULL;
    PDELETE_DIRECTORY_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PDELETE_DIRECTORY_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PWSTR       pwszPathFragment = NULL; // Do not free
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE    pTree = NULL;
    PWSTR       pwszDirectoryPath = NULL;
    BOOLEAN     bInLock = FALSE;
    IO_FILE_HANDLE  hFile = NULL;
    IO_FILE_NAME    fileName = {0};
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PVOID  pSecurityDescriptor = NULL;
    PVOID  pSecurityQOS = NULL;
    USHORT usPacketByteCount = 0;
    ULONG  ulOffset = 0;

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

    ntStatus = WireUnmarshallDirectoryDeleteRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pwszPathFragment);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pwszPathFragment || !*pwszPathFragment)
    {
        ntStatus = STATUS_CANNOT_DELETE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SrvBuildFilePath(
                    pTree->pShareInfo->pwszPath,
                    pwszPathFragment,
                    &pwszDirectoryPath);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    fileName.FileName = pwszDirectoryPath;

    ntStatus = IoCreateFile(
                    &hFile,
                    NULL,
                    &ioStatusBlock,
                    pSecurityContext,
                    &fileName,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    GENERIC_WRITE,
                    0,
                    FILE_ATTRIBUTE_NORMAL,
                    0,
                    FILE_OPEN,
                    FILE_DELETE_ON_CLOSE,
                    NULL,
                    0,
                    NULL);
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
                COM_DELETE_DIRECTORY,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallDeleteDirectoryResponse(
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

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    if (pTree)
    {
        SMB_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

        SrvTreeRelease(pTree);
    }

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    if (pwszDirectoryPath)
    {
        LwRtlMemoryFree(pwszDirectoryPath);
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


