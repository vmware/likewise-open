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
SrvExecuteReadFile(
    PSMB_SRV_FILE pFile,
    ULONG         ulBytesToRead,
    PLONG64       plByteOffset,
    PVOID*        ppBuffer,
    PULONG        pulBytesRead,
    PULONG        pKey
    );

static
NTSTATUS
SrvBuildReadResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PVOID               pBuffer,
    ULONG               ulBytesRead,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessReadAndX(
    PLWIO_SRV_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PSMB_PACKET pSmbResponse = NULL;
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE pTree = NULL;
    PSMB_SRV_FILE pFile = NULL;
    PREAD_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PVOID  pBuffer = NULL;
    ULONG64 ullBytesToRead = 0;
    ULONG  ulBytesRead = 0;
    LONG64 llByteOffset = 0;

    if (pConnection->serverProperties.bRequireSecuritySignatures &&
        pConnection->pSessionKey)
    {
        ntStatus = SMBPacketVerifySignature(
                        pSmbRequest,
                        pContext->ulRequestSequence,
                        pConnection->pSessionKey,
                        pConnection->ulSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    ntStatus = WireUnmarshallReadRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->bufferLen - pSmbRequest->bufferUsed,
                    (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader,
                    &pRequestHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pRequestHeader->fid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    llByteOffset = (((LONG64)pRequestHeader->offsetHigh) << 32) | ((LONG64)pRequestHeader->offset);
    ullBytesToRead = (((ULONG64)pRequestHeader->maxCountHigh) << 32) | ((ULONG64)pRequestHeader->maxCount);

    ntStatus = SrvExecuteReadFile(
                    pFile,
                    ullBytesToRead,
                    &llByteOffset,
                    &pBuffer,
                    &ulBytesRead,
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildReadResponse(
                    pConnection,
                    pSmbRequest,
                    pTree,
                    pBuffer,
                    ulBytesRead,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pConnection->serverProperties.bRequireSecuritySignatures &&
        pConnection->pSessionKey)
    {
        ntStatus = SMBPacketSign(
                        pSmbResponse,
                        pContext->ulResponseSequence,
                        pConnection->pSessionKey,
                        pConnection->ulSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvConnectionWriteMessage(
                    pConnection,
                    pSmbResponse);
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

    if (pSmbResponse)
    {
        SMBPacketFree(
             pConnection->hPacketAllocator,
             pSmbResponse);
    }

    SMB_SAFE_FREE_MEMORY(pBuffer);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvExecuteReadFile(
    PSMB_SRV_FILE pFile,
    ULONG         ulBytesToRead,
    PLONG64       pllByteOffset,
    PVOID*        ppBuffer,
    PULONG        pulBytesRead,
    PULONG        pKey
    )
{
    NTSTATUS ntStatus = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PVOID pBuffer = NULL;

    ntStatus = SMBAllocateMemory(
                    ulBytesToRead,
                    (PVOID*)&pBuffer);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoReadFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pBuffer,
                    ulBytesToRead,
                    pllByteOffset,
                    pKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = ioStatusBlock.Status;
    BAIL_ON_NT_STATUS(ntStatus);

    *pulBytesRead = ioStatusBlock.BytesTransferred;
    *ppBuffer = pBuffer;

cleanup:

    return ntStatus;

error:

    *ppBuffer = NULL;
    *pulBytesRead = 0;

    SMB_SAFE_FREE_MEMORY(pBuffer);

    goto cleanup;
}

static
NTSTATUS
SrvBuildReadResponse(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PVOID               pBuffer,
    ULONG               ulBytesRead,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PREAD_RESPONSE_HEADER pResponseHeader = NULL;
    ULONG ulPackageByteCount = 0;

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
                COM_READ_ANDX,
                0,
                TRUE,
                pTree->tid,
                getpid(),
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 12;

    pResponseHeader = (PREAD_RESPONSE_HEADER)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(READ_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(READ_RESPONSE_HEADER);

    pResponseHeader->remaining = -1;
    pResponseHeader->reserved = 0;
    memset(&pResponseHeader->reserved2, 0, sizeof(pResponseHeader->reserved2));
    pResponseHeader->dataCompactionMode = 0;
    pResponseHeader->dataLength = ulBytesRead;
    // TODO: For large cap files
    pResponseHeader->dataLengthHigh = 0;
    pResponseHeader->dataOffset = 0;

    ntStatus = WireMarshallReadResponseData(
                    pSmbResponse->pData,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (pSmbResponse->pData - pSmbResponse->pParams) % 2,
                    pBuffer,
                    ulBytesRead,
                    &ulPackageByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pByteCount = &pResponseHeader->byteCount;
    *pSmbResponse->pByteCount = (USHORT)ulPackageByteCount;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    if (pSmbResponse)
    {
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

