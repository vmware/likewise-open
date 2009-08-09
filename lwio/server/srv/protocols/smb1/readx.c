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
SrvBuildReadAndXResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    LONG64               llByteOffset,
    ULONG64              ullBytesToRead
    );

static
NTSTATUS
SrvExecuteReadFileAndX(
    PLWIO_SRV_FILE pFile,
    ULONG          ulBytesToRead,
    PLONG64        plByteOffset,
    PBYTE*         ppBuffer,
    PULONG         pulBytesRead,
    PULONG         pulKey
    );

NTSTATUS
SrvProcessReadAndX(
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
    PREAD_ANDX_REQUEST_HEADER  pRequestHeader = NULL; // Do not free
    ULONG64           ullBytesToRead = 0;
    LONG64            llByteOffset = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    PLWIO_SRV_FILE    pFile = NULL;

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

    ntStatus = WireUnmarshallReadAndXRequest(
                    pBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pRequestHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
                    pTree,
                    pRequestHeader->fid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    llByteOffset = (((LONG64)pRequestHeader->offsetHigh) << 32) |
                    ((LONG64)pRequestHeader->offset);

    ullBytesToRead = (((ULONG64)pRequestHeader->maxCountHigh) << 32) |
                      ((ULONG64)pRequestHeader->maxCount);

    ntStatus = SrvBuildReadAndXResponse(
                        pExecContext,
                        llByteOffset,
                        ullBytesToRead);
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
SrvBuildReadAndXResponse(
    PSRV_EXEC_CONTEXT pExecContext,
    LONG64            llByteOffset,
    ULONG64           ullBytesToRead
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG  ulPackageByteCount  = 0;
    ULONG ulTotalBytesUsed     = 0;
    PREAD_ANDX_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    ULONG ulDataOffset = 0;
    ULONG ulBytesRead = 0;
    ULONG ulBytesToRead = 0;
    PBYTE pData = NULL;
    ULONG ulKey = 0;

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_READ_ANDX,
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

    pSmbResponse->pHeader->wordCount = 12;

    if (ulBytesAvailable < sizeof(READ_ANDX_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PREAD_ANDX_RESPONSE_HEADER)pOutBuffer;

    pOutBuffer       += sizeof(READ_ANDX_RESPONSE_HEADER);
    ulOffset         += sizeof(READ_ANDX_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(READ_ANDX_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(READ_ANDX_RESPONSE_HEADER);

    pResponseHeader->remaining = -1;
    pResponseHeader->reserved = 0;
    memset(&pResponseHeader->reserved2, 0, sizeof(pResponseHeader->reserved2));
    pResponseHeader->dataCompactionMode = 0;
    pResponseHeader->dataLength = ulBytesRead;
    // TODO: For large cap files
    pResponseHeader->dataLengthHigh = 0;

    // Estimate how much data can fit in message
    ntStatus = WireMarshallReadResponseDataEx(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pData,
                    ulBytesRead,
                    &ulDataOffset,
                    &ulPackageByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    // Allow for alignment bytes
    ulDataOffset += ulDataOffset % 2;

    ulBytesToRead =
        SMB_MIN(ullBytesToRead,
                pConnection->serverProperties.MaxBufferSize - ulDataOffset);
    ulKey = pSmbRequest->pHeader->pid;

    ntStatus = SrvExecuteReadFileAndX(
                    pCtxSmb1->pFile,
                    ulBytesToRead,
                    &llByteOffset,
                    &pData,
                    &ulBytesRead,
                    &ulKey);
    if (ntStatus != STATUS_END_OF_FILE)
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = WireMarshallReadResponseDataEx(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pData,
                    ulBytesRead,
                    &ulDataOffset,
                    &ulPackageByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->dataLength = ulBytesRead;
    // TODO: For large cap files
    pResponseHeader->dataLengthHigh = 0;

    // The data offset will fit in 16 bits
    assert(ulDataOffset <= UINT16_MAX);
    pResponseHeader->dataOffset = (USHORT)ulDataOffset;

    assert(ulPackageByteCount <= UINT16_MAX);
    pResponseHeader->byteCount = (USHORT)ulPackageByteCount;

    // pOutBuffer       += ulPackageByteCount;
    // ulOffset         += ulPackageByteCount;
    // ulBytesAvailable -= ulPackageByteCount;
    ulTotalBytesUsed += ulPackageByteCount;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pData)
    {
        SrvFreeMemory(pData);
    }

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

static
NTSTATUS
SrvExecuteReadFileAndX(
    PLWIO_SRV_FILE pFile,
    ULONG          ulBytesToRead,
    PLONG64        pllByteOffset,
    PBYTE*         ppBuffer,
    PULONG         pulBytesRead,
    PULONG         pulKey
    )
{
    NTSTATUS ntStatus = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PBYTE pBuffer = NULL;

    ntStatus = SrvAllocateMemory(ulBytesToRead, (PVOID*)&pBuffer);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoReadFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pBuffer,
                    ulBytesToRead,
                    pllByteOffset,
                    pulKey);
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesRead = ioStatusBlock.BytesTransferred;
    *ppBuffer = pBuffer;

cleanup:

    return ntStatus;

error:

    *ppBuffer = NULL;
    *pulBytesRead = 0;

    if (pBuffer)
    {
        SrvFreeMemory(pBuffer);
    }

    goto cleanup;
}

