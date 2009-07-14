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
SrvMarshallEchoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbResponse,
    PSMB_PACKET         pSmbRequest,
    USHORT              usUid,
    USHORT              usMid,
    USHORT              usSequenceNumber,
    PBYTE               pEchoBlob,
    USHORT              ulEchoBlobLength
    );

NTSTATUS
SrvProcessEchoAndX(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   iEchoCount = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PECHO_REQUEST_HEADER pEchoHeader = NULL; // Do not Free
    PBYTE       pEchoBlob = NULL; // Do Not Free
    USHORT  usNumEchoesToSend = 0;
    ULONG   ulOffset = 0;

    ulOffset = (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader;

    ntStatus = WireUnmarshallEchoRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    &pEchoHeader,
                    &pEchoBlob);
    BAIL_ON_NT_STATUS(ntStatus);

    // If echo count is zero, no response is sent
    if (!pEchoHeader->echoCount)
    {
        goto cleanup;
    }

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

    usNumEchoesToSend = pEchoHeader->echoCount - 1;

    for (; iEchoCount < pEchoHeader->echoCount; iEchoCount++)
    {
        SMBPacketResetBuffer(pSmbResponse);

        ntStatus = SrvMarshallEchoResponse(
                        pConnection,
                        pSmbRequest,
                        pSmbResponse,
                        pSmbRequest->pSMBHeader->uid,
                        pSmbRequest->pSMBHeader->mid,
                        iEchoCount,
                        pEchoBlob,
                        pEchoHeader->byteCount);
        BAIL_ON_NT_STATUS(ntStatus);

        // This API always expects a response to be returned
        // So, don't send out the last echo message
        // Give that last response back to the caller
        if (usNumEchoesToSend--)
        {
            pSmbResponse->sequence = pSmbRequest->sequence + 1;

            ntStatus = SrvTransportSendResponse(pConnection, pSmbResponse);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    *ppSmbResponse = pSmbResponse;

cleanup:

    return (ntStatus);

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
SrvMarshallEchoResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET         pSmbResponse,
    USHORT              usUid,
    USHORT              usMid,
    USHORT              usSequenceNumber,
    PBYTE               pEchoBlob,
    USHORT              usEchoBlobLength
    )
{
    NTSTATUS ntStatus = 0;
    PECHO_RESPONSE_HEADER pResponseHeader = NULL;
    PCSTR    pMinEchoBlob = "lwio";
    USHORT   usPackageByteCount = 0;

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_ECHO,
                0,
                TRUE,
                0,
                pSmbRequest->pSMBHeader->pid,
                usUid,
                usMid,
                FALSE,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 1;

    pResponseHeader = (ECHO_RESPONSE_HEADER*)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(ECHO_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(ECHO_RESPONSE_HEADER);

    pResponseHeader->sequenceNumber = usSequenceNumber;

    ntStatus = WireMarshallEchoResponseData(
                    pSmbResponse->pData,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (usEchoBlobLength > 4 ? pEchoBlob : (PBYTE)pMinEchoBlob),
                    (usEchoBlobLength > 4 ? usEchoBlobLength : strlen(pMinEchoBlob)),
                    &usPackageByteCount);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->byteCount = usPackageByteCount;

    pSmbResponse->bufferUsed += usPackageByteCount;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

