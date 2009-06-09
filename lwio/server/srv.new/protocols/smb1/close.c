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
SrvSetLastWriteTime(
    PLWIO_SRV_FILE pFile,
    ULONG         ulLastWriteTime
    );

static
NTSTATUS
SrvBuildCloseResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_TREE       pTree,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessCloseAndX(
    IN  PLWIO_SRV_CONNECTION pConneciton,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    PCLOSE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    ULONG ulOffset = 0;

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

    ntStatus = WireUnmarshallCloseRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pRequestHeader->fid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pRequestHeader->lastWriteTime != 0)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvSetLastWriteTime(
                                pFile,
                                pRequestHeader->lastWriteTime);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to set the last write time for file [fid:%u][code:%d]",
                            pFile->fid,
                            ntStatus2);
        }
    }

    ntStatus = SrvTreeRemoveFile(
                    pTree,
                    pFile->fid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildCloseResponse(
                    pConnection,
                    pTree,
                    pSmbRequest,
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
             SrvTransportGetAllocator(pConnection),
             pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvSetLastWriteTime(
    PLWIO_SRV_FILE pFile,
    ULONG         ulLastWriteTime
    )
{
    NTSTATUS ntStatus = 0;

    // TODO

    return ntStatus;
}

static
NTSTATUS
SrvBuildCloseResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_TREE       pTree,
    PSMB_PACKET         pSmbRequest,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PCLOSE_RESPONSE_HEADER pResponseHeader = NULL;

    ntStatus = SMBPacketAllocate(
                    SrvTransportGetAllocator(pConnection),
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    SrvTransportGetAllocator(pConnection),
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                COM_CLOSE,
                0,
                TRUE,
                pTree->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 0;

    pResponseHeader = (PCLOSE_RESPONSE_HEADER)pSmbResponse->pParams;
    pSmbResponse->pData = pSmbResponse->pParams + sizeof(CLOSE_RESPONSE_HEADER);
    pSmbResponse->bufferUsed += sizeof(CLOSE_RESPONSE_HEADER);

    pResponseHeader->byteCount = 0;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    if (pSmbResponse)
    {
        SMBPacketFree(
            SrvTransportGetAllocator(pConnection),
            pSmbResponse);
    }

    goto cleanup;
}


