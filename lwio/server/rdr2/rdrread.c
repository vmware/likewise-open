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
 *        read.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (RDR)
 *
 *       Read Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

/* Offset from the beginning off the SMB header to
   the data in the Read&X response */

#define READ_DATA_OFFSET     60

static
PRDR_OP_CONTEXT
RdrFinishReadFile(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS ntStatus,
    PVOID pParam
    );

static
VOID
RdrCancelReadFile(
    PIRP pIrp,
    PVOID _pContext
    )
{
    PRDR_OP_CONTEXT pContext = _pContext;
    PSMB_CLIENT_FILE_HANDLE pFile = IoFileGetContext(pIrp->FileHandle);

    RdrSocketCancel(pFile->pTree->pSession->pSocket, pContext);
}

NTSTATUS
RdrRead(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_CLIENT_FILE_HANDLE pFile = IoFileGetContext(pIrp->FileHandle);
    PRDR_OP_CONTEXT pContext = NULL;

    ntStatus = RdrCreateContext(
        pIrp,
        &pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pContext->Continue = RdrFinishReadFile;

    if (pIrp->Args.ReadWrite.ByteOffset)
    {
        pContext->State.Read.llByteOffset = *pIrp->Args.ReadWrite.ByteOffset;
    }
    else
    {
        pContext->State.Read.llByteOffset = pFile->llOffset;
    }

    IoIrpMarkPending(pIrp, RdrCancelReadFile, pContext);

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
RdrTransceiveReadFile(
    PRDR_OP_CONTEXT pContext,
    PSMB_CLIENT_FILE_HANDLE pFile,
    ULONG64 ullFileReadOffset,
    USHORT usReadLen
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    uint16_t packetByteCount = 0;
    READ_ANDX_REQUEST_HEADER_WC_12 *pRequestHeader = NULL;

    ntStatus = RdrAllocateContextPacket(
        pContext,
        1024*64);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pContext->Packet.pRawBuffer,
                pContext->Packet.bufferLen,
                COM_READ_ANDX,
                0,
                0,
                pFile->pTree->tid,
                gRdrRuntime.SysPid,
                pFile->pTree->pSession->uid,
                0,
                TRUE,
                &pContext->Packet);
    BAIL_ON_NT_STATUS(ntStatus);

    pContext->Packet.pData = pContext->Packet.pParams + sizeof(READ_ANDX_REQUEST_HEADER_WC_12);
    pContext->Packet.bufferUsed += sizeof(READ_ANDX_REQUEST_HEADER_WC_12);
    pContext->Packet.pSMBHeader->wordCount = 12;

    pRequestHeader = (PREAD_ANDX_REQUEST_HEADER_WC_12) pContext->Packet.pParams;

    pRequestHeader->fid = pFile->fid;
    pRequestHeader->offset = ullFileReadOffset & 0x00000000FFFFFFFFLL;
    pRequestHeader->maxCount = usReadLen;
    pRequestHeader->minCount = usReadLen; /* blocking read */
    pRequestHeader->maxCountHigh = 0;
    pRequestHeader->remaining = 0; /* obsolete */
    pRequestHeader->offsetHigh = (ullFileReadOffset & 0xFFFFFFFF00000000LL) >> 32;
    pRequestHeader->byteCount = 0;

    pContext->Packet.bufferUsed += packetByteCount;

    // byte order conversions
    SMB_HTOL16_INPLACE(pRequestHeader->fid);
    SMB_HTOL32_INPLACE(pRequestHeader->offset);
    SMB_HTOL16_INPLACE(pRequestHeader->maxCount);
    SMB_HTOL16_INPLACE(pRequestHeader->minCount);
    SMB_HTOL32_INPLACE(pRequestHeader->maxCountHigh);
    SMB_HTOL16_INPLACE(pRequestHeader->remaining);
    SMB_HTOL32_INPLACE(pRequestHeader->offsetHigh);
    SMB_HTOL16_INPLACE(pRequestHeader->byteCount);

    ntStatus = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RdrSocketTransceive(
        pFile->pTree->pSession->pSocket,
        pContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
PRDR_OP_CONTEXT
RdrFinishReadFile(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS ntStatus,
    PVOID pParam
    )
{
    READ_ANDX_RESPONSE_HEADER *pResponseHeader = NULL;
    PSMB_PACKET pResponsePacket = pParam;
    USHORT usBytesRead = 0;
    PBYTE pBuffer = pContext->pIrp->Args.ReadWrite.Buffer;
    ULONG ulLength = pContext->pIrp->Args.ReadWrite.Length;
    PSMB_CLIENT_FILE_HANDLE pFile = IoFileGetContext(pContext->pIrp->FileHandle);
    ULONG ulReadMax = 0;
    ULONG ulReadLength = 0;

    BAIL_ON_NT_STATUS(ntStatus);

    if (pResponsePacket)
    {
        ntStatus = pResponsePacket->pSMBHeader->error;
        BAIL_ON_NT_STATUS(ntStatus);

        if (pResponsePacket->pSMBHeader->command != COM_READ_ANDX ||
            pResponsePacket->pNetBIOSHeader->len - (pResponsePacket->pParams - pResponsePacket->pRawBuffer) <
            sizeof(READ_ANDX_RESPONSE_HEADER))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pResponseHeader = (PREAD_ANDX_RESPONSE_HEADER) pResponsePacket->pParams;

        // byte order conversions
        SMB_LTOH16_INPLACE(pResponseHeader->dataLength);
        SMB_LTOH16_INPLACE(pResponseHeader->dataOffset);
        SMB_LTOH16_INPLACE(pResponseHeader->dataLengthHigh);

        if (pResponseHeader->dataLength)
        {
            usBytesRead = pResponseHeader->dataLength;

            if (usBytesRead > pContext->State.Read.usReadLen ||
                pResponseHeader->dataOffset + usBytesRead > pResponsePacket->pNetBIOSHeader->len)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            memcpy(pBuffer + pContext->State.Read.llTotalBytesRead,
                   (uint8_t*)pResponsePacket->pSMBHeader + pResponseHeader->dataOffset,
                   usBytesRead);

            pContext->State.Read.llTotalBytesRead += usBytesRead;
            pContext->State.Read.llByteOffset += usBytesRead;
        }
    }

    if (pContext->State.Read.llTotalBytesRead < ulLength)
    {
        if (pContext->State.Read.usReadLen && usBytesRead < pContext->State.Read.usReadLen)
        {
            if (pContext->State.Read.llTotalBytesRead == 0)
            {
                ntStatus = STATUS_END_OF_FILE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                ntStatus = STATUS_SUCCESS;
                goto cleanup;
            }
        }

        ulReadMax = pFile->pTree->pSession->pSocket->maxBufferSize - READ_DATA_OFFSET;
        ulReadLength = ulReadMax;

        if (ulReadLength > UINT16_MAX)
        {
            ulReadLength = UINT16_MAX;
        }

        if (ulReadLength > ulLength)
        {
            ulReadLength = ulLength;
        }

        pContext->State.Read.usReadLen = (USHORT) ulReadLength;

        ntStatus = RdrTransceiveReadFile(
            pContext,
            pFile,
            (ULONG64) pContext->State.Read.llByteOffset,
            (USHORT) ulReadLength);
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
            pContext->pIrp->IoStatusBlock.BytesTransferred = pContext->State.Read.llTotalBytesRead;
        }

        IoIrpComplete(pContext->pIrp);
        RdrFreeContext(pContext);
    }

    return NULL;

error:

    goto cleanup;
}
