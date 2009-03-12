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

typedef struct
{
    USHORT               usFid;
    USHORT               usReserved;
    SECURITY_INFORMATION ulSecurityInfo;
} __attribute__((__packed__)) SMB_SECURITY_INFORMATION_HEADER, *PSMB_SECURITY_INFORMATION_HEADER;

static
NTSTATUS
SrvQuerySecurityDescriptor(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PBYTE               pParameters,
    ULONG               ulParameterCount,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvSetSecurityDescriptor(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PBYTE               pParameters,
    ULONG               ulParameterCount,
    PBYTE               pData,
    ULONG               ulDataLen,
    PSMB_PACKET*        ppSmbResponse
    );

NTSTATUS
SrvProcessNtTransact(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
    PNT_TRANSACTION_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PUSHORT pusBytecount = NULL; // Do not free
    PUSHORT pSetup = NULL; // Do not free
    PBYTE   pParameters = NULL; // Do not free
    PBYTE   pData = NULL; // Do not free
    PSMB_SRV_SESSION pSession = NULL;
    PSMB_SRV_TREE    pTree = NULL;
    PSMB_SRV_FILE    pFile = NULL;
    ULONG       ulOffset = 0;
    PSMB_PACKET pSmbResponse = NULL;

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

    ntStatus = WireUnmarshallNtTransactionRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pSetup,
                    &pusBytecount,
                    &pParameters,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pRequestHeader->usFunction)
    {
        case SMB_SUB_COMMAND_NT_TRANSACT_QUERY_SECURITY_DESC:

            ntStatus = SrvQuerySecurityDescriptor(
                            pConnection,
                            pSmbRequest,
                            pTree,
                            pParameters,
                            pRequestHeader->ulTotalParameterCount,
                            &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_SET_SECURITY_DESC :

            ntStatus = SrvSetSecurityDescriptor(
                            pConnection,
                            pSmbRequest,
                            pTree,
                            pParameters,
                            pRequestHeader->ulTotalParameterCount,
                            pData,
                            pRequestHeader->ulTotalDataCount,
                            &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_CREATE :
        case SMB_SUB_COMMAND_NT_TRANSACT_IOCTL :
        case SMB_SUB_COMMAND_NT_TRANSACT_NOTIFY_CHANGE :
        case SMB_SUB_COMMAND_NT_TRANSACT_RENAME :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;
            break;
    }
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
SrvQuerySecurityDescriptor(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PBYTE               pParameters,
    ULONG               ulParameterCount,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_FILE pFile = NULL;
    PUSHORT       pSetup = NULL;
    UCHAR         ucSetupCount = 0;
    PBYTE         pSecurityDescriptor = NULL;
    ULONG         ulSecurityDescInitialLen = 256;
    ULONG         ulSecurityDescAllocLen = 0;
    ULONG         ulSecurityDescActualLen = 0;
    ULONG         ulDataOffset = 0;
    ULONG         ulParameterOffset = 0;
    ULONG         ulNumPackageBytesUsed = 0;
    PSMB_SECURITY_INFORMATION_HEADER pQueryRequest = NULL;
    PSMB_PACKET pSmbResponse = NULL;

    if (ulParameterCount != sizeof(SMB_SECURITY_INFORMATION_HEADER))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pQueryRequest = (PSMB_SECURITY_INFORMATION_HEADER)pParameters;

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pQueryRequest->usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LW_RTL_ALLOCATE(
                    &pSecurityDescriptor,
                    BYTE,
                    ulSecurityDescInitialLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ulSecurityDescAllocLen = ulSecurityDescInitialLen;

    do
    {
        IO_STATUS_BLOCK ioStatusBlock = {0};
        ULONG ulNewLen = 0;

        ntStatus = IoQuerySecurityFile(
                        pFile->hFile,
                        NULL,
                        &ioStatusBlock,
                        pQueryRequest->ulSecurityInfo,
                        (PSECURITY_DESCRIPTOR_RELATIVE)pSecurityDescriptor,
                        ulSecurityDescAllocLen);
        if ((ntStatus == STATUS_BUFFER_TOO_SMALL) &&
            (ulSecurityDescLen != SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE))
        {
            PBYTE pNewMemory = NULL;

            ulNewLen = LW_MIN(SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE, ulSecurityDescAllocLen + 4096);

            ntStatus = LW_RTL_ALLOCATE(
                            &pNewMemory,
                            BYTE,
                            ulNewLen);
            BAIL_ON_NT_STATUS(ntStatus);

            if (pSecurityDescriptor)
            {
                LwRtlMemoryFree(pSecurityDescriptor);
            }

            pSecurityDescriptor = pNewMemory;
            ulSecurityDescAllocLen = ulNewLen;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ulSecurityDescLen = ioStatusBlock.BytesTransferred;

    } while (ntStatus != STATUS_SUCCESS);

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
                COM_NT_TRANSACT,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 18 + ucSetupCount;

    ntStatus = WireMarshallNtTransactionResponse(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    ucSetupCount,
                    (PBYTE)&ulSecurityDescLen,
                    sizeof(ulSecurityDescLen),
                    pSecurityDescriptor,
                    ulSecurityDescLen,
                    &ulDataOffset,
                    &ulParameterOffset,
                    &ulNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += ulNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    if (pSecurityDescriptor)
    {
        LwRtlMemoryFree(pSecurityDescriptor);
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
SrvSetSecurityDescriptor(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PBYTE               pParameters,
    ULONG               ulParameterCount,
    PBYTE               pData,
    ULONG               ulDataLen,
    PSMB_PACKET*        ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_FILE pFile = NULL;
    PUSHORT pSetup = NULL;
    UCHAR   ucSetupCount = 0;
    ULONG   ulDataOffset = 0;
    ULONG   ulParameterOffset = 0;
    ULONG   ulNumPackageBytesUsed = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PSMB_SECURITY_INFORMATION_HEADER pSecuritySetRequest = NULL;
    PSMB_PACKET pSmbResponse = NULL;

    if (ulParameterCount != sizeof(SMB_SECURITY_INFORMATION_HEADER))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSecuritySetRequest = (PSMB_SECURITY_INFORMATION_HEADER)pParameters;

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pSecuritySetRequest->usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoSetSecurityFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pSecuritySetRequest->ulSecurityInfo,
                    (PSECURITY_DESCRIPTOR_RELATIVE)pData,
                    ulDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

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
                COM_NT_TRANSACT,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 18 + ucSetupCount;

    ntStatus = WireMarshallNtTransactionResponse(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    pSetup,
                    ucSetupCount,
                    NULL,
                    0,
                    NULL,
                    0,
                    &ulDataOffset,
                    &ulParameterOffset,
                    &ulNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += ulNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
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
