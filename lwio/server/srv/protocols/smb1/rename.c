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
    PLWIO_SRV_SESSION pSession,
    PLWIO_SRV_TREE    pTree,
    USHORT            usSearchAttributes,
    PWSTR             pwszOldName,
    PWSTR             pwszNewName
    );

static
NTSTATUS
SrvBuildRenameResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessRename(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PLWIO_SRV_SESSION          pSession = NULL;
    PLWIO_SRV_TREE             pTree    = NULL;
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PSMB_RENAME_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PWSTR                      pwszOldName = NULL; // Do not free
    PWSTR                      pwszNewName = NULL; // Do not free

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

    ntStatus = WireUnmarshallRenameRequest(
                    pBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pRequestHeader,
                    &pwszOldName,
                    &pwszNewName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvExecuteRename(
                    pSession,
                    pTree,
                    pRequestHeader->usSearchAttributes,
                    pwszOldName,
                    pwszNewName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildRenameResponse(pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvExecuteRename(
    PLWIO_SRV_SESSION pSession,
    PLWIO_SRV_TREE    pTree,
    USHORT            usSearchAttributes,
    PWSTR             pwszOldName,
    PWSTR             pwszNewName
    )
{
    NTSTATUS ntStatus = 0;
    PWSTR    pwszOldPath = NULL;
    BOOLEAN  bInLock = FALSE;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    IO_FILE_HANDLE  hFile = NULL;
    IO_FILE_HANDLE  hDir  = NULL;
    PIO_ASYNC_CONTROL_BLOCK     pAsyncControlBlock = NULL;
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

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

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
                    pSession->pIoSecurityContext,
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

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    oldName.FileName = pwszOldPath;
    newName.FileName = pwszNewName;

    ntStatus = IoCreateFile(
                    &hFile,
                    pAsyncControlBlock,
                    &ioStatusBlock,
                    pSession->pIoSecurityContext,
                    &oldName,
                    pSecurityDescriptor,
                    pSecurityQOS,
                    DELETE,
                    0,
                    FILE_ATTRIBUTE_NORMAL,
                    FILE_SHARE_DELETE|FILE_SHARE_READ|FILE_SHARE_WRITE,
                    FILE_OPEN,
                    0,
                    NULL, /* EA Buffer */
                    0,    /* EA Length */
                    NULL  /* ECP List  */
                    );
    BAIL_ON_NT_STATUS(ntStatus);

    ulDataLen = sizeof(FILE_RENAME_INFORMATION) + wc16slen(newName.FileName) * sizeof(wchar16_t);

    ntStatus = SrvAllocateMemory(ulDataLen, (PVOID*)&pData);
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

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

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
        SrvFreeMemory(pwszOldPath);
    }

    if (pData)
    {
        SrvFreeMemory(pData);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildRenameResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                    ntStatus     = 0;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                       iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1         pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1         pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PSMB_RENAME_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;

    ntStatus = SrvMarshalHeader_SMB_V1(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM_RENAME,
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

    pSmbResponse->pHeader->wordCount = 2;

    ntStatus = WireMarshallRenameResponse(
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


