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
 *        close.c
 *
 * Abstract:
 *
 *        Likewise SMB Redirector File System Driver (RDR)
 *
 *        Close Dispatch Function
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "rdr.h"

static
BOOLEAN
RdrFinishClose(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    );

static
VOID
RdrCancelClose(
    PIRP pIrp,
    PVOID pContext
    )
{
    return;
}

NTSTATUS
RdrClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PRDR_CCB pFile = IoFileGetContext(pIrp->FileHandle);
    PCLOSE_REQUEST_HEADER pHeader = NULL;
    PRDR_OP_CONTEXT pContext = NULL;

    ntStatus = RdrCreateContext(pIrp, &pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pFile->fid)
    {
        IoIrpMarkPending(pIrp, RdrCancelClose, NULL);

        ntStatus = RdrAllocateContextPacket(
            pContext,
            1024*64);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBPacketMarshallHeader(
            pContext->Packet.pRawBuffer,
            pContext->Packet.bufferLen,
            COM_CLOSE,
            0,
            0,
            pFile->pTree->tid,
            gRdrRuntime.SysPid,
            pFile->pTree->pSession->uid,
            0,
            TRUE,
            &pContext->Packet);
        BAIL_ON_NT_STATUS(ntStatus);

        pContext->Packet.pData = pContext->Packet.pParams + sizeof(CLOSE_REQUEST_HEADER);
        pContext->Packet.bufferUsed += sizeof(CLOSE_REQUEST_HEADER);

        pContext->Packet.pSMBHeader->wordCount = 3;

        pHeader = (PCLOSE_REQUEST_HEADER) pContext->Packet.pParams;

        pHeader->fid = SMB_HTOL16(pFile->fid);
        pHeader->ulLastWriteTime = SMB_HTOL32(0);
        pHeader->byteCount = SMB_HTOL16(0);

        ntStatus = SMBPacketMarshallFooter(&pContext->Packet);
        BAIL_ON_NT_STATUS(ntStatus);

        pContext->Continue = RdrFinishClose;

        ntStatus = RdrSocketTransceive(
            pFile->pTree->pSession->pSocket,
            pContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        RdrReleaseFile(pFile);
    }


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
BOOLEAN
RdrFinishClose(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PIRP pIrp = pContext->pIrp;
    PRDR_CCB pFile = IoFileGetContext(pIrp->FileHandle);

    if (status == STATUS_SUCCESS)
    {
        status = pPacket->pSMBHeader->error;
    }

    if (pPacket)
    {
        SMBPacketRelease(
            pFile->pTree->pSession->pSocket->hPacketAllocator,
            pPacket);
    }

    pIrp->IoStatusBlock.Status = status;
    IoIrpComplete(pIrp);
    RdrReleaseFile(pFile);
    RdrFreeContext(pContext);

    return FALSE;
}

void
RdrReleaseFile(
    PRDR_CCB pFile
    )
{
    if (pFile->pTree)
    {
        RdrTreeRelease(pFile->pTree);
    }

    if (pFile->pMutex)
    {
        pthread_mutex_destroy(pFile->pMutex);
    }

    RTL_FREE(&pFile->pwszPath);
    RTL_FREE(&pFile->find.pBuffer);

    SMBFreeMemory(pFile);
}
