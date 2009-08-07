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
 *        setinfo.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Set Information
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvSetFileInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

NTSTATUS
SrvSetFileBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

NTSTATUS
SrvSetFileRenameInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

NTSTATUS
SrvSetFileDispositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

NTSTATUS
SrvSetFilePositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

NTSTATUS
SrvSetFileModeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

NTSTATUS
SrvSetFileAllocationInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

NTSTATUS
SrvSetFileEOFInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

NTSTATUS
SrvSetPipeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

static
NTSTATUS
SrvSetFileSystemInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

static
NTSTATUS
SrvSetSecurityInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    );

NTSTATUS
SrvProcessSetInfo_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection   = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE                         pData = NULL; // Do not free
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree = NULL;
    PLWIO_SRV_FILE_2    pFile = NULL;

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

    ntStatus = SMB2UnmarshalSetInfoRequest(
                    pSmbRequest,
                    &pRequestHeader,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2FindFile_SMB_V2(
                        pCtxSmb2,
                        pTree,
                        &pRequestHeader->fid,
                        &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pRequestHeader->ucInfoType)
    {
        case SMB2_INFO_TYPE_FILE:

            ntStatus = SrvSetFileInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        case SMB2_INFO_TYPE_FILE_SYSTEM:

            ntStatus = SrvSetFileSystemInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        case SMB2_INFO_TYPE_SECURITY:

            ntStatus = SrvSetSecurityInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }
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

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvSetFileInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    switch (pRequestHeader->ucInfoClass)
    {
        case SMB2_FILE_INFO_CLASS_BASIC :

            ntStatus = SrvSetFileBasicInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        case SMB2_FILE_INFO_CLASS_RENAME :

            ntStatus = SrvSetFileRenameInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        case SMB2_FILE_INFO_CLASS_DISPOSITION :

            ntStatus = SrvSetFileDispositionInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        case SMB2_FILE_INFO_CLASS_POSITION :

            ntStatus = SrvSetFilePositionInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        case SMB2_FILE_INFO_CLASS_MODE :

            ntStatus = SrvSetFileModeInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        case SMB2_FILE_INFO_CLASS_ALLOCATION :

            ntStatus = SrvSetFileAllocationInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        case SMB2_FILE_INFO_CLASS_EOF :

            ntStatus = SrvSetFileEOFInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        case SMB2_FILE_INFO_CLASS_PIPE:

            ntStatus = SrvSetPipeInfo_SMB_V2(
                            pExecContext,
                            pRequestHeader,
                            pData);

            break;

        default:

            ntStatus = STATUS_INVALID_INFO_CLASS;

            break;
    }

    return ntStatus;
}

NTSTATUS
SrvSetFileBasicInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvSetFileRenameInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvSetFileDispositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvSetFilePositionInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvSetFileModeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvSetFileAllocationInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;
    PFILE_ALLOCATION_INFORMATION pFileAllocationInfo = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PSMB2_SET_INFO_RESPONSE_HEADER pResponseHeader = NULL; // Do not free

    if (pRequestHeader->ulInputBufferLen < sizeof(FILE_ALLOCATION_INFORMATION))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pFileAllocationInfo = (PFILE_ALLOCATION_INFORMATION)pData;

    ntStatus = IoSetInformationFile(
                    pCtxSmb2->pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pFileAllocationInfo,
                    sizeof(FILE_ALLOCATION_INFORMATION),
                    FileAllocationInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_SETINFO,
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

    if (ulBytesAvailable < sizeof(SMB2_SET_INFO_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PSMB2_SET_INFO_RESPONSE_HEADER)pOutBuffer;
    pResponseHeader->usLength = sizeof(SMB2_SET_INFO_RESPONSE_HEADER);

    // pOutBuffer += sizeof(SMB2_SET_INFO_RESPONSE_HEADER);
    // ulOffset += sizeof(SMB2_SET_INFO_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(SMB2_SET_INFO_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_SET_INFO_RESPONSE_HEADER);

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

NTSTATUS
SrvSetFileEOFInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
SrvSetPipeInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvSetFileSystemInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    return STATUS_NOT_SUPPORTED;
}

static
NTSTATUS
SrvSetSecurityInfo_SMB_V2(
    PSRV_EXEC_CONTEXT             pExecContext,
    PSMB2_SET_INFO_REQUEST_HEADER pRequestHeader,
    PBYTE                         pData
    )
{
    return STATUS_NOT_SUPPORTED;
}
