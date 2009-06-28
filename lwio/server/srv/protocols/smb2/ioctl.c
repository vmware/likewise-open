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
 *        ioctl.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        IOCTL
 *
 * Authors: Krishna Ganugapati (kganugapati@likewise.com)
 *          Sriram Nambakam (snambakam@likewise.com)
 *
 *
 */

#include "includes.h"

static
NTSTATUS
SrvExecuteFsctl_SMB_V2(
    PLWIO_SRV_FILE_2 pFile,
    PBYTE            pData,
    ULONG            ulDataLen,
    ULONG            ulControlCode,
    PBYTE*           ppResponseBuffer,
    PULONG           pulResponseBufferLen
    );

static
NTSTATUS
SrvExecuteIoctl_SMB_V2(
    PLWIO_SRV_FILE_2 pFile,
    PBYTE            pData,
    ULONG            ulDataLen,
    ULONG            ulControlCode,
    PBYTE*           ppResponseBuffer,
    PULONG           pulResponseBufferLen
    );

static
NTSTATUS
SrvBuildIOCTLResponse_SMB_V2(
    PSMB_PACKET                pSmbRequest,
    PLWIO_SRV_CONNECTION       pConnection,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pResponseBuffer,
    ULONG                      ulResponseBufferLen,
    PSMB_PACKET*               ppSmbResponse
    );

NTSTATUS
SrvProcessIOCTL_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_SESSION_2 pSession = NULL;
    PLWIO_SRV_TREE_2    pTree = NULL;
    PLWIO_SRV_FILE_2    pFile = NULL;
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PBYTE   pData = NULL; // Do not free
    PBYTE   pResponseBuffer = NULL;
    ULONG   ulResponseBufferLen = 0;
    PSMB_PACKET pSmbResponse = NULL;

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

    ntStatus = SMB2UnmarshalIOCTLRequest(
                    pSmbRequest,
                    &pRequestHeader,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTree2FindFile(
                    pTree,
                    pRequestHeader->fid.ullVolatileId,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pRequestHeader->ulFlags & 0x1)
    {
        ntStatus = SrvExecuteFsctl_SMB_V2(
                        pFile,
                        pData,
                        pRequestHeader->ulInLength,
                        pRequestHeader->ulFunctionCode,
                        &pResponseBuffer,
                        &ulResponseBufferLen);
    }
    else
    {
        ntStatus = SrvExecuteIoctl_SMB_V2(
                        pFile,
                        pData,
                        pRequestHeader->ulInLength,
                        pRequestHeader->ulFunctionCode,
                        &pResponseBuffer,
                        &ulResponseBufferLen);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildIOCTLResponse_SMB_V2(
                    pSmbRequest,
                    pConnection,
                    pRequestHeader,
                    pResponseBuffer,
                    ulResponseBufferLen,
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
SrvExecuteFsctl_SMB_V2(
    PLWIO_SRV_FILE_2 pFile,
    PBYTE            pData,
    ULONG            ulDataLen,
    ULONG            ulFunctionCode,
    PBYTE*           ppResponseBuffer,
    PULONG           pulResponseBufferLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pResponseBuffer = NULL;
    ULONG    ulResponseBufferLen = 0;
    ULONG    ulActualResponseLen = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    ntStatus = SrvAllocateMemory(512, (PVOID*) &pResponseBuffer);
    BAIL_ON_NT_STATUS(ntStatus);

    ulResponseBufferLen = 512;

    do
    {
        ntStatus = IoFsControlFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        ulFunctionCode,
                        pData,
                        ulDataLen,
                        pResponseBuffer,
                        ulResponseBufferLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            ULONG ulNewLength = 0;

            if ((ulResponseBufferLen + 256) > UINT32_MAX)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulNewLength = ulResponseBufferLen + 256;

            if (pResponseBuffer)
            {
                SrvFreeMemory(pResponseBuffer);
                pResponseBuffer = NULL;
                ulResponseBufferLen = 0;
            }

            ntStatus = SrvAllocateMemory(ulNewLength, (PVOID*)&pResponseBuffer);
            BAIL_ON_NT_STATUS(ntStatus);

            ulResponseBufferLen = ulNewLength;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ulActualResponseLen = ioStatusBlock.BytesTransferred;

    } while (ntStatus != STATUS_SUCCESS);

    *ppResponseBuffer = pResponseBuffer;
    *pulResponseBufferLen = ulResponseBufferLen;

cleanup:

    return ntStatus;

error:

    *ppResponseBuffer = NULL;
    *pulResponseBufferLen = 0;

    if (pResponseBuffer)
    {
        SrvFreeMemory(pResponseBuffer);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteIoctl_SMB_V2(
    PLWIO_SRV_FILE_2 pFile,
    PBYTE            pData,
    ULONG            ulDataLen,
    ULONG            ulControlCode,
    PBYTE*           ppResponseBuffer,
    PULONG           pulResponseBufferLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pResponseBuffer = NULL;
    USHORT   ulResponseBufferLen = 0;
    USHORT   ulActualResponseLen = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    ntStatus = SrvAllocateMemory(512, (PVOID*) &pResponseBuffer);
    BAIL_ON_NT_STATUS(ntStatus);

    ulResponseBufferLen = 512;

    do
    {
        ntStatus = IoDeviceIoControlFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        ulControlCode,
                        pData,
                        ulDataLen,
                        pResponseBuffer,
                        ulResponseBufferLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT ulNewLength = 0;

            if ((ulResponseBufferLen + 256) > UINT32_MAX)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulNewLength = ulResponseBufferLen + 256;

            if (pResponseBuffer)
            {
                SrvFreeMemory(pResponseBuffer);
                pResponseBuffer = NULL;
                ulResponseBufferLen = 0;
            }

            ntStatus = SrvAllocateMemory(ulNewLength, (PVOID*)&pResponseBuffer);
            BAIL_ON_NT_STATUS(ntStatus);

            ulResponseBufferLen = ulNewLength;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ulActualResponseLen = ioStatusBlock.BytesTransferred;

    } while (ntStatus != STATUS_SUCCESS);

    *ppResponseBuffer = pResponseBuffer;
    *pulResponseBufferLen = ulResponseBufferLen;

cleanup:

    return ntStatus;

error:

    *ppResponseBuffer = NULL;
    *pulResponseBufferLen = 0;

    if (pResponseBuffer)
    {
        SrvFreeMemory(pResponseBuffer);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildIOCTLResponse_SMB_V2(
    PSMB_PACKET                pSmbRequest,
    PLWIO_SRV_CONNECTION       pConnection,
    PSMB2_IOCTL_REQUEST_HEADER pRequestHeader,
    PBYTE                      pResponseBuffer,
    ULONG                      ulResponseBufferLen,
    PSMB_PACKET*               ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
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
                    COM2_IOCTL,
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

    ntStatus = SMB2MarshalIOCTLResponse(
                    pSmbResponse,
                    pRequestHeader,
                    pResponseBuffer,
                    SMB_MIN(ulResponseBufferLen, pRequestHeader->ulOutLength));
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
