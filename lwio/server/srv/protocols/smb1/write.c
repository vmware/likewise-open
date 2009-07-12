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
SrvExecuteWrite(
    PLWIO_SRV_FILE pFile,
    PBYTE         pData,
    PULONG        pulDataOffset,
    USHORT        usDataLength,
    PUSHORT       pusBytesWritten,
    PULONG        pulKey
    );

static
NTSTATUS
SrvBuildWriteResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_TREE       pTree,
    PLWIO_SRV_FILE       pFile,
    USHORT              usBytesWritten,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessWrite(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    ULONG ulOffset = 0;
    ULONG ulDataOffset = 0;
    PWRITE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE pData = NULL; // Do not free
    USHORT usBytesWritten = 0;
    PSMB_PACKET pSmbResponse = NULL;
    ULONG ulKey = 0;

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

    ntStatus = WireUnmarshallWriteRequest(
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

    ulDataOffset = pRequestHeader->offset;

    ulKey = pSmbRequest->pSMBHeader->pid;

    ntStatus = SrvExecuteWrite(
                    pFile,
                    pData,
                    &ulDataOffset,
                    pRequestHeader->dataLength,
                    &usBytesWritten,
		    &ulKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildWriteResponse(
                    pConnection,
                    pSmbRequest,
                    pTree,
                    pFile,
                    usBytesWritten,
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
SrvExecuteWrite(
    PLWIO_SRV_FILE pFile,
    PBYTE         pData,
    PULONG        pulDataOffset,
    USHORT        usDataLength,
    PUSHORT       pusBytesWritten,
    PULONG        pulKey
    )
{
    NTSTATUS ntStatus = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    ULONG   ulDataLength = usDataLength;
    LONG64  llDataOffset = *pulDataOffset;
    ULONG   ulBytesWritten = 0;

    if (pData)
    {
        ntStatus = IoWriteFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        pData,
                        ulDataLength,
                        &llDataOffset,
                        pulKey);
        BAIL_ON_NT_STATUS(ntStatus);

        ulBytesWritten = ioStatusBlock.BytesTransferred;
    }
    else
    {
        FILE_END_OF_FILE_INFORMATION fileEofInfo = {0};

        fileEofInfo.EndOfFile = llDataOffset;

        ntStatus = IoSetInformationFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        &fileEofInfo,
                        sizeof(fileEofInfo),
                        FileEndOfFileInformation
                        );
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pusBytesWritten = ulBytesWritten;
    *pulDataOffset = SMB_MIN(UINT32_MAX, llDataOffset);

cleanup:

    return ntStatus;

error:

    *pusBytesWritten = 0;

    goto cleanup;
}

static
NTSTATUS
SrvBuildWriteResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PLWIO_SRV_TREE       pTree,
    PLWIO_SRV_FILE       pFile,
    USHORT              usBytesWritten,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PWRITE_RESPONSE_HEADER pResponseHeader = NULL;

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
                COM_WRITE,
                0,
                TRUE,
                pTree->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 1;

    pResponseHeader = (PWRITE_RESPONSE_HEADER)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(WRITE_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(WRITE_RESPONSE_HEADER);

    pResponseHeader->count = usBytesWritten;

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
