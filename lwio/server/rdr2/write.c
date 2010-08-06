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



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        write.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (RDR)
 *
 *       Write Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

#define WRITE_DATA_OFFSET     133

static
BOOLEAN
RdrFinishWriteFile(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS ntStatus,
    PVOID pParam
    );

static
VOID
RdrCancelWriteFile(
    PIRP pIrp,
    PVOID _pContext
    )
{
    PRDR_OP_CONTEXT pContext = _pContext;
    PRDR_CCB pFile = IoFileGetContext(pIrp->FileHandle);

    RdrSocketCancel(pFile->pTree->pSession->pSocket, pContext);
}

NTSTATUS
RdrWrite(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PRDR_CCB pFile = IoFileGetContext(pIrp->FileHandle);
    PRDR_OP_CONTEXT pContext = NULL;

    ntStatus = RdrCreateContext(
        pIrp,
        &pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pContext->Continue = RdrFinishWriteFile;

    if (pIrp->Args.ReadWrite.ByteOffset)
    {
        pContext->State.Write.llByteOffset = *pIrp->Args.ReadWrite.ByteOffset;
    }
    else
    {
        pContext->State.Write.llByteOffset = pFile->llOffset;
    }

    IoIrpMarkPending(pIrp, RdrCancelWriteFile, pContext);

    RdrContinueContext(pContext, STATUS_SUCCESS, NULL);

    ntStatus = STATUS_PENDING;

cleanup:

    if (ntStatus != STATUS_PENDING)
    {
        RdrFreeContext(pContext);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveWriteFile(
    PRDR_OP_CONTEXT pContext,
    PRDR_CCB pFile,
    ULONG64 ullFileWriteOffset,
    PBYTE pBuffer,
    USHORT usWriteLen,
    USHORT usWriteMode
    )
{
    NTSTATUS status = 0;
    uint32_t packetByteCount = 0;
    WRITE_ANDX_REQUEST_HEADER_WC_14 *pRequestHeader = NULL;
    uint16_t wNumBytesWriteable = 0;
    uint16_t dataOffset = 0;

    status = RdrAllocateContextPacket(pContext, 1024*64);
    BAIL_ON_NT_STATUS(status);

    status = SMBPacketMarshallHeader(
        pContext->Packet.pRawBuffer,
        pContext->Packet.bufferLen,
        COM_WRITE_ANDX,
        0,
        0,
        pFile->pTree->tid,
        gRdrRuntime.SysPid,
        pFile->pTree->pSession->uid,
        0,
        TRUE,
        &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pContext->Packet.pData = pContext->Packet.pParams + sizeof(WRITE_ANDX_REQUEST_HEADER_WC_14);
    /* @todo: handle size restart */
    pContext->Packet.bufferUsed += sizeof(WRITE_ANDX_REQUEST_HEADER_WC_14);

    pContext->Packet.pSMBHeader->wordCount = 14;

    pRequestHeader = (WRITE_ANDX_REQUEST_HEADER_WC_14 *) pContext->Packet.pParams;

    pRequestHeader->fid = pFile->fid;
    pRequestHeader->offset = ullFileWriteOffset & 0x00000000FFFFFFFFLL;
    pRequestHeader->reserved = 0;
    pRequestHeader->writeMode = usWriteMode;
    pRequestHeader->remaining = 0;

    /* ignored if CAP_LARGE_WRITEX is set */
    wNumBytesWriteable = UINT16_MAX - (pContext->Packet.pParams - (uint8_t*)pContext->Packet.pSMBHeader) - sizeof(WRITE_ANDX_REQUEST_HEADER_WC_14);
    // And, then the alignment
    wNumBytesWriteable -= (pContext->Packet.pData - (uint8_t *) pRequestHeader) % 2;
    if (usWriteLen > wNumBytesWriteable)
    {
        usWriteLen = wNumBytesWriteable;
    }

    pRequestHeader->dataLength = usWriteLen;
    /* @todo: what is this value if CAP_LARGE_WRITEX is set? */
    pRequestHeader->dataLengthHigh = 0;
    pRequestHeader->dataOffset = 0;
    /* only present if wordCount = 14 and not 12 */
    pRequestHeader->offsetHigh = (ullFileWriteOffset & 0xFFFFFFFF00000000LL) >> 32;
    pRequestHeader->byteCount = usWriteLen;

    status = MarshallWriteRequestData(
                pContext->Packet.pData,
                pContext->Packet.bufferLen - pContext->Packet.bufferUsed,
                (pContext->Packet.pData - (uint8_t *) pRequestHeader) % 2,
                &packetByteCount,
                &dataOffset,
                pBuffer,
                usWriteLen);
    BAIL_ON_NT_STATUS(status);

    pContext->Packet.bufferUsed += packetByteCount;

    pRequestHeader->dataOffset = dataOffset;
    pRequestHeader->dataOffset += pContext->Packet.pData - (uint8_t *) pContext->Packet.pSMBHeader;

    // byte order conversions
    SMB_HTOL16_INPLACE(pRequestHeader->fid);
    SMB_HTOL32_INPLACE(pRequestHeader->offset);
    SMB_HTOL16_INPLACE(pRequestHeader->writeMode);
    SMB_HTOL16_INPLACE(pRequestHeader->remaining);
    SMB_HTOL16_INPLACE(pRequestHeader->dataLengthHigh);
    SMB_HTOL16_INPLACE(pRequestHeader->dataLength);
    SMB_HTOL16_INPLACE(pRequestHeader->dataOffset);
    SMB_HTOL32_INPLACE(pRequestHeader->offsetHigh);
    SMB_HTOL16_INPLACE(pRequestHeader->byteCount);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pFile->pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

static
BOOLEAN
RdrFinishWriteFile(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS ntStatus,
    PVOID pParam
    )
{
    WRITE_ANDX_RESPONSE_HEADER *pResponseHeader = NULL;
    PSMB_PACKET pResponsePacket = pParam;
    USHORT usBytesWritten = 0;
    PBYTE pBuffer = pContext->pIrp->Args.ReadWrite.Buffer;
    ULONG ulLength = pContext->pIrp->Args.ReadWrite.Length;
    PRDR_CCB pFile = IoFileGetContext(pContext->pIrp->FileHandle);
    ULONG ulWriteMax = 0;
    ULONG ulWriteLength = 0;
    USHORT usWriteMode = 0;

    BAIL_ON_NT_STATUS(ntStatus);

    if (pResponsePacket)
    {
        ntStatus = pResponsePacket->pSMBHeader->error;
        BAIL_ON_NT_STATUS(ntStatus);

        if (pResponsePacket->pSMBHeader->command != COM_WRITE_ANDX ||
            pResponsePacket->bufferUsed - (pResponsePacket->pParams - pResponsePacket->pRawBuffer) <
            sizeof(WRITE_ANDX_RESPONSE_HEADER))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pResponseHeader = (PWRITE_ANDX_RESPONSE_HEADER) pResponsePacket->pParams;

        // byte order conversions
        SMB_LTOH16_INPLACE(pResponseHeader->count);
        SMB_LTOH16_INPLACE(pResponseHeader->countHigh);

        if (pResponseHeader->count)
        {
            usBytesWritten = pResponseHeader->count;
            pContext->State.Write.llTotalBytesWritten += usBytesWritten;
            pContext->State.Write.llByteOffset += usBytesWritten;
        }
    }

    if (pContext->State.Write.llTotalBytesWritten < ulLength)
    {
        ulWriteMax = pFile->pTree->pSession->pSocket->maxBufferSize - WRITE_DATA_OFFSET;
        ulWriteLength = ulWriteMax;

        if (ulWriteLength > UINT16_MAX)
        {
            ulWriteLength = UINT16_MAX;
        }

        if (ulWriteLength > ulLength)
        {
            ulWriteLength = ulLength;
        }

        /* If file is a named pipe in message mode and this is the
           first write command, set the "start of message" bit */
        if (pFile->usFileType == 0x2 &&
            pContext->State.Write.llTotalBytesWritten == 0)
        {
            usWriteMode |= 0x8;
        }

        ntStatus = RdrTransceiveWriteFile(
            pContext,
            pFile,
            (ULONG64) pContext->State.Write.llByteOffset,
            pBuffer + pContext->State.Write.llTotalBytesWritten,
            (USHORT) ulWriteLength,
            usWriteMode);
        BAIL_ON_NT_STATUS(ntStatus);
    }


cleanup:

    if (pResponsePacket)
    {
        SMBPacketRelease(
            pFile->pTree->pSession->pSocket->hPacketAllocator,
            pResponsePacket);
    }

    if (ntStatus != STATUS_PENDING)
    {
        pContext->pIrp->IoStatusBlock.Status = ntStatus;

        if (ntStatus == STATUS_SUCCESS)
        {
            pContext->pIrp->IoStatusBlock.BytesTransferred = pContext->State.Write.llTotalBytesWritten;
        }

        IoIrpComplete(pContext->pIrp);
        RdrFreeContext(pContext);
    }

    return FALSE;

error:

    goto cleanup;
}
