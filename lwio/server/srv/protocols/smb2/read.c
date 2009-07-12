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
 *        read.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Read
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvBuildReadResponse_SMB_V2(
    PSMB2_MESSAGE             pSmbRequest,
    PLWIO_SRV_CONNECTION      pConnection,
    PSMB2_READ_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE_2          pFile,
    PSMB_PACKET               pSmbResponse
    );

NTSTATUS
SrvProcessRead_SMB_V2(
    IN     PLWIO_SRV_CONNECTION pConnection,
    IN     PSMB2_MESSAGE        pSmbRequest,
    IN OUT PSMB_PACKET          pSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree = NULL;
    PLWIO_SRV_FILE_2    pFile = NULL;
    PSMB2_READ_REQUEST_HEADER pRequestHeader = NULL; // Do not free

    ntStatus = SrvConnection2FindSession(
                    pConnection,
                    pSmbRequest->pHeader->ullSessionId,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSession2FindTree(
                    pSession,
                    pSmbRequest->pHeader->ulTid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2UnmarshalReadRequest(
                    pSmbRequest,
                    &pRequestHeader);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2FindFile(
                    pTree,
                    pRequestHeader->fid.ullVolatileId,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildReadResponse_SMB_V2(
                    pSmbRequest,
                    pConnection,
                    pRequestHeader,
                    pFile,
                    pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

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

    goto cleanup;
}

static
NTSTATUS
SrvBuildReadResponse_SMB_V2(
    PSMB2_MESSAGE             pSmbRequest,
    PLWIO_SRV_CONNECTION      pConnection,
    PSMB2_READ_REQUEST_HEADER pRequestHeader,
    PLWIO_SRV_FILE_2          pFile,
    PSMB_PACKET               pSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG           ulDataOffset = 0L;
    ULONG           ulBytesToRead = 0L;
    ULONG           ulRemaining = 0L;
    ULONG           ulKey = 0L;
    LONG64          llFileOffset = 0LL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PBYTE           pData = NULL;
    PBYTE pOutBufferRef = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    PBYTE pOutBuffer = pOutBufferRef;
    ULONG ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;
    ULONG ulOffset    = pSmbResponse->bufferUsed - sizeof(NETBIOS_HEADER);
    ULONG ulBytesUsed = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_READ,
                    0,
                    1,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pSmbRequest->pHeader->ulTid,
                    pSmbRequest->pHeader->ullSessionId,
                    STATUS_SUCCESS,
                    TRUE,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    pOutBuffer += ulBytesUsed;
    ulOffset += ulBytesUsed;
    ulBytesAvailable -= ulBytesUsed;

    ntStatus = SMB2MarshalReadResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    NULL,
                    pRequestHeader->ulDataLength,
                    0,
                    &ulDataOffset,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulBytesToRead = SMB_MIN(pRequestHeader->ulDataLength,
                    pConnection->serverProperties.MaxBufferSize - ulDataOffset);

    ntStatus = SrvAllocateMemory(ulBytesToRead, (PVOID*)&pData);
    BAIL_ON_NT_STATUS(ntStatus);

    llFileOffset = pRequestHeader->ullFileOffset;
    ulKey = pSmbRequest->pHeader->ulPid;

    ntStatus = IoReadFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pData,
                    ulBytesToRead,
                    &llFileOffset,
                    &ulKey);
    if (ntStatus == STATUS_END_OF_FILE)
    {
        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (ioStatusBlock.BytesTransferred < pRequestHeader->ulMinimumCount)
    {
        ulRemaining = pRequestHeader->ulMinimumCount -
                      ioStatusBlock.BytesTransferred;
    }

    ntStatus = SMB2MarshalReadResponse(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pData,
                    ioStatusBlock.BytesTransferred,
                    ulRemaining,
                    &ulDataOffset,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    ulTotalBytesUsed += ulBytesUsed;
    // pOutBuffer += ulBytesUsed;
    // ulOffset += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;

    pSmbResponse->bufferUsed += ulTotalBytesUsed;

cleanup:

    SRV_SAFE_FREE_MEMORY(pData);

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        memset(pOutBufferRef, 0, ulTotalBytesUsed);
    }

    goto cleanup;
}
