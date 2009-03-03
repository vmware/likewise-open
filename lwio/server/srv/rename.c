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
SrvExecuteRename(
    PSMB_SRV_TREE pTree,
    USHORT        usSearchAttributes,
    PWSTR         pwszOldName,
    PWSTR         pwszNewName
    );

static
NTSTATUS
SrvBuildRenameResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessRename(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE    pTree = NULL;
    PSMB_RENAME_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PWSTR       pwszOldName = NULL; // Do not free
    PWSTR       pwszNewName = NULL; // Do not free
    PSMB_PACKET pSmbResponse = NULL;
    ULONG       ulOffset = 0;

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

    ntStatus = WireUnmarshallRenameRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pwszOldName,
                    &pwszNewName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvExecuteRename(
                    pTree,
                    pRequestHeader->usSearchAttributes,
                    pwszOldName,
                    pwszNewName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildRenameResponse(
                    pConnection,
                    pSmbRequest,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = pSmbResponse;

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
SrvExecuteRename(
    PSMB_SRV_TREE pTree,
    USHORT        usSearchAttributes,
    PWSTR         pwszOldName,
    PWSTR         pwszNewName
    )
{
    NTSTATUS ntStatus = 0;
    PWSTR    pwszOldPath = NULL;
    BOOLEAN  bInLock = FALSE;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    IO_FILE_HANDLE  hFile = NULL;
    IO_FILE_HANDLE  hDir  = NULL;
    PIO_ASYNC_CONTROL_BLOCK     pAsyncControlBlock = NULL;
    PIO_CREATE_SECURITY_CONTEXT pSecurityContext = NULL;
    PVOID        pSecurityDescriptor = NULL;
    PVOID        pSecurityQOS = NULL;
    IO_FILE_NAME oldName = {0};
    IO_FILE_NAME newName = {0};
    IO_FILE_NAME dirPath = {0};
    PFILE_RENAME_INFORMATION pFileRenameInfo = NULL;
    PBYTE pData = NULL;
    ULONG ulDataLen = 0;

    if (!pwszOldName || !*pwszOldName || !pwszNewName || !*pwszNewName)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    ntStatus = SrvBuildFilePath(
                    pTree->pShareInfo->pwszPath,
                    pwszOldName,
                    &pwszOldPath);
    BAIL_ON_NT_STATUS(ntStatus);

    dirPath.FileName = pTree->pShareInfo->pwszPath;

    ntStatus = IoCreateFile(
                    &hDir,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSecurityContext,
                    &dirPath,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    GENERIC_READ,
                    0,
                    FILE_ATTRIBUTE_NORMAL,
                    0,
                    FILE_OPEN,
                    FILE_DIRECTORY_FILE,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    NULL  /* ECP List  */
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    oldName.FileName = pwszOldPath;
    newName.FileName = pwszNewName;

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSecurityContext,
                    &oldName,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    GENERIC_READ,
                    0,
                    FILE_ATTRIBUTE_NORMAL,
                    0,
                    FILE_OPEN,
                    0,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    NULL  /* ECP List  */
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ulDataLen = sizeof(FILE_RENAME_INFORMATION) + wc16slen(newName.FileName) * sizeof(wchar16_t);

    ntStatus = LW_RTL_ALLOCATE(
                    &pData,
                    BYTE,
                    ulDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    pFileRenameInfo = (PFILE_RENAME_INFORMATION)pData;
    pFileRenameInfo->ReplaceIfExists = TRUE;
    pFileRenameInfo->RootDirectory = hDir;
    pFileRenameInfo->FileNameLength = wc16slen(newName.FileName) * sizeof(wchar16_t);
    memcpy((PBYTE)pFileRenameInfo->FileName, (PBYTE)newName.FileName, pFileRenameInfo->FileNameLength);

    ntStatus = IoSetInformationFile(
                    hFile,
                    NULL,
                    &ioStatusBlock,
                    pFileRenameInfo,
                    ulDataLen,
                    FileRenameInformation);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    if (hFile)
    {
        IoCloseFile(hFile);
    }

    if (hDir)
    {
        IoCloseFile(hDir);
    }

    if (pwszOldPath)
    {
        LwRtlMemoryFree(pwszOldPath);
    }

    if (pData)
    {
        LwRtlMemoryFree(pData);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildRenameResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RENAME_RESPONSE_HEADER pResponseHeader = NULL;
    USHORT usNumPackageBytesUsed = 0;
    PSMB_PACKET pSmbResponse = NULL;

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
                COM_RENAME,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 2;

    ntStatus = WireMarshallRenameResponse(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    &pResponseHeader,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->usByteCount = 0;

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

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


