/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Write
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildWriteResponse_SMB_V2(
    PSMB_PACKET          pSmbRequest,
    PLWIO_SRV_CONNECTION pConnection,
    ULONG                ulBytesWritten,
    ULONG                ulBytesRemaining,
    PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProcessWrite_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree = NULL;
    PLWIO_SRV_FILE_2    pFile = NULL;
    PSMB2_WRITE_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE                      pData = NULL; // Do not free
    LONG64                     llDataOffset = 0LL;
    ULONG                      ulKey = 0L;
    IO_STATUS_BLOCK            ioStatusBlock = {0};
    PSMB_PACKET                pSmbResponse = NULL;

    ntStatus = SrvConnection2FindSession(
                    pConnection,
                    pSmbRequest->pSMB2Header->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindTree(
                    pSession,
                    pSmbRequest->pSMB2Header->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2UnmarshalWriteRequest(
                    pSmbRequest,
                    &pRequestHeader,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2FindFile(
                    pTree,
                    pRequestHeader->fid.ullVolatileId,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ulKey = pSmbRequest->pSMB2Header->ulPid;
    llDataOffset = pRequestHeader->ullFileOffset;

    ntStatus = IoWriteFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pData,
                    pRequestHeader->ulDataLength,
                    &llDataOffset,
                    &ulKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildWriteResponse_SMB_V2(
                    pSmbRequest,
                    pConnection,
                    ioStatusBlock.BytesTransferred,
                    pRequestHeader->ulDataLength-ioStatusBlock.BytesTransferred,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pFile)
    {
        SrvFile2Release(pFile);
    }

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    if (pSession)
    {
        SrvSession2Release(pSession);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(pConnection->hPacketAllocator, pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildWriteResponse_SMB_V2(
    PSMB_PACKET          pSmbRequest,
    PLWIO_SRV_CONNECTION pConnection,
    ULONG                ulBytesWritten,
    ULONG                ulBytesRemaining,
    PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_PACKET pSmbResponse = NULL;

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

    ntStatus = SMB2MarshalHeader(
                    pSmbResponse,
                    COM2_WRITE,
                    0,
                    1,
                    pSmbRequest->pSMB2Header->ulPid,
                    pSmbRequest->pSMB2Header->ullCommandSequence,
                    pSmbRequest->pSMB2Header->ulTid,
                    pSmbRequest->pSMB2Header->ullSessionId,
                    STATUS_SUCCESS,
                    TRUE,
                    TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalWriteResponse(
                    pSmbResponse,
                    ulBytesWritten,
                    ulBytesRemaining);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

*ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(pConnection->hPacketAllocator, pSmbResponse);
    }

    goto cleanup;
}
