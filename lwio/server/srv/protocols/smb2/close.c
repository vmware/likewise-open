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
 *        close.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Close
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildCloseResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

NTSTATUS
SrvProcessClose_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2 pCtxSmb2       = pProtocolContext->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PSMB2_FID           pFid     = NULL; // Do not free
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree    = NULL;
    PLWIO_SRV_FILE_2    pFile    = NULL;

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

    ntStatus = SMB2UnmarshalCloseRequest(pSmbRequest, &pFid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2FindFile_SMB_V2(
                    pCtxSmb2,
                    pTree,
                    pFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2RemoveFile(pTree, pFile->ullFid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildCloseResponse_SMB_V2(pExecContext);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pCtxSmb2->pFile)
    {
        SrvFile2Release(pCtxSmb2->pFile);
        pCtxSmb2->pFile = NULL;
    }

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

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildCloseResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2 pCtxSmb2 = pProtocolContext->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse = &pCtxSmb2->pResponses[iMsg];
    FILE_BASIC_INFORMATION      fileBasicInfo = {0};
    FILE_STANDARD_INFORMATION   fileStdInfo = {0};
    IO_STATUS_BLOCK             ioStatusBlock = {0};
    PSMB2_CLOSE_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer = pSmbResponse->pBuffer;
    ULONG ulOffset = 0;
    ULONG ulTotalBytesUsed = 0;
    ULONG ulBytesUsed = 0;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;

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
                    COM2_CLOSE,
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

    if (ulBytesAvailable < sizeof(SMB2_CLOSE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_CLOSE_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulBytesUsed       = sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_CLOSE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_CLOSE_RESPONSE_HEADER);

    pResponseHeader->ullCreationTime   = fileBasicInfo.CreationTime;
    pResponseHeader->ullLastAccessTime = fileBasicInfo.LastAccessTime;
    pResponseHeader->ullLastWriteTime  = fileBasicInfo.LastWriteTime;
    pResponseHeader->ullLastChangeTime = fileBasicInfo.ChangeTime;
    pResponseHeader->ulFileAttributes  = fileBasicInfo.FileAttributes;
    pResponseHeader->ullAllocationSize = fileStdInfo.AllocationSize;
    pResponseHeader->ullEndOfFile      = fileStdInfo.EndOfFile;
    pResponseHeader->usLength          = ulBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}
