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
SrvExecuteWriteAndX(
    PLWIO_SRV_FILE pFile,
    PBYTE         pData,
    PLONG64       pllDataOffset,
    ULONG64       ullDataLength,
    PULONG64      pullBytesWritten,
    PULONG        pulKey
    );

static
NTSTATUS
SrvBuildWriteAndXResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    ULONG64           ullBytesWritten
    );

NTSTATUS
SrvProcessWriteAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PWRITE_ANDX_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE                      pData = NULL; // Do not free
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    PLWIO_SRV_FILE    pFile = NULL;
    LONG64            llDataOffset = 0;
    LONG64            llDataLength = 0;
    ULONG64           ullBytesWritten = 0;
    ULONG             ulKey = 0;

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

    ntStatus = WireUnmarshallWriteAndXRequest(
                    pBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pRequestHeader,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
                    pTree,
                    pRequestHeader->fid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    llDataOffset = (((LONG64)pRequestHeader->offsetHigh) << 32) |
                    ((LONG64)pRequestHeader->offset);
    llDataLength = (((LONG64)pRequestHeader->dataLengthHigh) << 32) |
                    ((LONG64)pRequestHeader->dataLength);

    ulKey = pSmbRequest->pHeader->pid;

    ntStatus = SrvExecuteWriteAndX(
                    pFile,
                    pData,
                    &llDataOffset,
                    llDataLength,
                    &ullBytesWritten,
                    &ulKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildWriteAndXResponse(pExecContext, ullBytesWritten);
    BAIL_ON_NT_STATUS(ntStatus);

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

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvExecuteWriteAndX(
    PLWIO_SRV_FILE pFile,
    PBYTE         pData,
    PLONG64       pllDataOffset,
    ULONG64       ullDataLength,
    PULONG64      pullBytesWritten,
    PULONG        pulKey
    )
{
    NTSTATUS ntStatus = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    ULONG ulDataLength = 0;
    PBYTE   pDataCursor = pData;
    ULONG64 ullBytesWritten = 0;
    LONG64  llDataOffset = *pllDataOffset;

    while (ullDataLength)
    {
        if (ullDataLength > UINT32_MAX)
        {
            ulDataLength = UINT32_MAX;
        }
        else
        {
            ulDataLength = (ULONG)ullDataLength;
        }

        ntStatus = IoWriteFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        pData,
                        ulDataLength,
                        &llDataOffset,
                        pulKey);
        BAIL_ON_NT_STATUS(ntStatus);

        ullDataLength -= ioStatusBlock.BytesTransferred;
        ullBytesWritten += ioStatusBlock.BytesTransferred;
        pDataCursor += ioStatusBlock.BytesTransferred;
    }

cleanup:

    *pullBytesWritten = ullBytesWritten;

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildWriteAndXResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    ULONG64           ullBytesWritten
    )
{
    NTSTATUS                    ntStatus     = 0;
    PLWIO_SRV_CONNECTION        pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT  pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1    pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                       iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1         pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1         pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PWRITE_ANDX_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG ulTotalBytesUsed     = 0;

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_WRITE_ANDX,
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

    pSmbResponse->pHeader->wordCount = 6;

    if (ulBytesAvailable < sizeof(WRITE_ANDX_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PWRITE_ANDX_RESPONSE_HEADER)pOutBuffer;

    // pOutBuffer       += sizeof(WRITE_ANDX_RESPONSE_HEADER);
    // ulOffset         += sizeof(WRITE_ANDX_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(WRITE_ANDX_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(WRITE_ANDX_RESPONSE_HEADER);

    pResponseHeader->remaining = 0;
    pResponseHeader->reserved = 0;
    pResponseHeader->count = (ullBytesWritten & 0x00000000FFFFFFFFLL);
    pResponseHeader->countHigh = (ullBytesWritten & 0xFFFFFFFF00000000LL) >> 32;

    pResponseHeader->byteCount = 0;

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


