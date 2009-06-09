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
    PULONG64      pullBytesWritten
    );

static
NTSTATUS
SrvBuildWriteAndXResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_TREE       pTree,
    PLWIO_SRV_FILE       pFile,
    ULONG64             ullBytesWritten,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessWriteAndX(
	IN  PLWIO_SRV_CONNECTION pConnection,
	IN  PSMB_PACKET          pSmbRequest,
	OUT PSMB_PACKET*         ppSmbResponse
	)
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PSMB_PACKET pSmbResponse = NULL;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    PWRITE_ANDX_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE   pData = NULL; // Do not free
    LONG64  llDataOffset = 0;
    LONG64  llDataLength = 0;
    ULONG64 ullBytesWritten = 0;
    ULONG   ulOffset = 0;

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

    ntStatus = WireUnmarshallWriteAndXRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pRequestHeader->fid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    llDataOffset = (((LONG64)pRequestHeader->offsetHigh) << 32) | ((LONG64)pRequestHeader->offset);
    llDataLength = (((LONG64)pRequestHeader->dataLengthHigh) << 32) | ((LONG64)pRequestHeader->dataLength);

    ntStatus = SrvExecuteWriteAndX(
                    pFile,
                    pData,
                    &llDataOffset,
                    llDataLength,
                    &ullBytesWritten);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildWriteAndXResponse(
                    pConnection,
                    pSmbRequest,
                    pTree,
                    pFile,
                    ullBytesWritten,
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
NTSTATUS
SrvExecuteWriteAndX(
    PLWIO_SRV_FILE pFile,
    PBYTE         pData,
    PLONG64       pllDataOffset,
    ULONG64       ullDataLength,
    PULONG64      pullBytesWritten
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
                        NULL);
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
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_TREE       pTree,
    PLWIO_SRV_FILE       pFile,
    ULONG64             ullBytesWritten,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PWRITE_ANDX_RESPONSE_HEADER pResponseHeader = NULL;

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
                COM_WRITE_ANDX,
                0,
                TRUE,
                pTree->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 6;

    pResponseHeader = (PWRITE_ANDX_RESPONSE_HEADER)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(WRITE_ANDX_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(WRITE_ANDX_RESPONSE_HEADER);

    pResponseHeader->remaining = 0;
    pResponseHeader->reserved = 0;
    pResponseHeader->count = (ullBytesWritten & 0x00000000FFFFFFFFLL);
    pResponseHeader->countHigh = (ullBytesWritten & 0xFFFFFFFF00000000LL) >> 32;

    pResponseHeader->byteCount = 0;

    ntStatus = SMBPacketUpdateAndXOffset(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

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


