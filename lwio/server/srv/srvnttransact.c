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
} __attribute__((__packed__)) SMB_SECURITY_INFORMATION_HEADER,
                             *PSMB_SECURITY_INFORMATION_HEADER;

typedef struct
{
    ULONG   ulFunctionCode;
    USHORT  usFid;
    BOOLEAN bIsFsctl;
    UCHAR   ucFlags;
} __attribute__((__packed__)) SMB_IOCTL_HEADER, *PSMB_IOCTL_HEADER;

static
NTSTATUS
SrvQuerySecurityDescriptor(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PBYTE               pParameters,
    ULONG               ulParameterCount,
    ULONG               ulMaxDataCount,
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

static
NTSTATUS
SrvProcessIOCTL(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PSMB_SRV_TREE       pTree,
    PBYTE               pParameters,
    ULONG               ulParameterCount,
    PBYTE               pData,
    ULONG               ulDataLen,
    PSMB_PACKET*        ppSmbResponse
    );

static
NTSTATUS
SrvExecuteFsctl(
    PSMB_SRV_FILE pFile,
    PBYTE         pData,
    ULONG         ulDataLen,
    ULONG         ulFunctionCode,
    PBYTE*        ppResponseBuffer,
    PUSHORT       pusResponseBufferLen
    );

static
NTSTATUS
SrvExecuteIoctl(
    PSMB_SRV_FILE pFile,
    PBYTE         pData,
    ULONG         ulDataLen,
    ULONG         ulControlCode,
    PBYTE*        ppResponseBuffer,
    PUSHORT       pusResponseBufferLen
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
                            pRequestHeader->ulMaxDataCount,
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

        case SMB_SUB_COMMAND_NT_TRANSACT_IOCTL :

            ntStatus = SrvProcessIOCTL(
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
    ULONG               ulMaxDataCount,
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
            (ulSecurityDescAllocLen != SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE))
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

        ulSecurityDescActualLen = ioStatusBlock.BytesTransferred;

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
                (ulSecurityDescActualLen <= ulMaxDataCount ? STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL),
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
                    (PBYTE)&ulSecurityDescActualLen,
                    sizeof(ulSecurityDescActualLen),
                    (ulSecurityDescActualLen <= ulMaxDataCount ? pSecurityDescriptor : NULL),
                    (ulSecurityDescActualLen <= ulMaxDataCount ? ulSecurityDescActualLen : 0),
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

static
NTSTATUS
SrvProcessIOCTL(
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
    UCHAR   ucSetupCount = 1;
    ULONG   ulDataOffset = 0;
    ULONG   ulParameterOffset = 0;
    ULONG   ulNumPackageBytesUsed = 0;
    PSMB_IOCTL_HEADER pIoctlRequest = NULL;
    PSMB_PACKET pSmbResponse = NULL;
    PBYTE   pResponseBuffer = NULL;
    USHORT  usResponseBufferLen = 0;

    if (ulParameterCount != sizeof(SMB_IOCTL_HEADER))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pIoctlRequest = (PSMB_IOCTL_HEADER)pParameters;

    if (pIoctlRequest->ucFlags & 0x1)
    {
        // TODO: Apply only to DFS Share
        //       We don't support DFS yet
        ntStatus = STATUS_NOT_IMPLEMENTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pIoctlRequest->usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pIoctlRequest->bIsFsctl)
    {
        ntStatus = SrvExecuteFsctl(
                        pFile,
                        pData,
                        ulDataLen,
                        pIoctlRequest->ulFunctionCode,
                        &pResponseBuffer,
                        &usResponseBufferLen);
    }
    else
    {
        ntStatus = SrvExecuteIoctl(
                        pFile,
                        pData,
                        ulDataLen,
                        pIoctlRequest->ulFunctionCode,
                        &pResponseBuffer,
                        &usResponseBufferLen);
    }
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
                    &usResponseBufferLen,
                    ucSetupCount,
                    NULL,
                    0,
                    pResponseBuffer,
                    usResponseBufferLen,
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

static
NTSTATUS
SrvExecuteFsctl(
    PSMB_SRV_FILE pFile,
    PBYTE         pData,
    ULONG         ulDataLen,
    ULONG         ulFunctionCode,
    PBYTE*        ppResponseBuffer,
    PUSHORT       pusResponseBufferLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pResponseBuffer = NULL;
    USHORT   usResponseBufferLen = 0;
    USHORT   usActualResponseLen = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    ntStatus = LW_RTL_ALLOCATE(
                    &pResponseBuffer,
                    BYTE,
                    512);
    BAIL_ON_NT_STATUS(ntStatus);

    usResponseBufferLen = 512;

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
                        usResponseBufferLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewLength = 0;

            if ((usResponseBufferLen + 256) > UINT16_MAX)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            usNewLength = usResponseBufferLen + 256;

            if (pResponseBuffer)
            {
                LwRtlMemoryFree(pResponseBuffer);
                pResponseBuffer = NULL;
                usResponseBufferLen = 0;
            }

            ntStatus = LW_RTL_ALLOCATE(
                            &pResponseBuffer,
                            BYTE,
                            usNewLength);
            BAIL_ON_NT_STATUS(ntStatus);

            usResponseBufferLen = usNewLength;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        usActualResponseLen = ioStatusBlock.BytesTransferred;

    } while (ntStatus != STATUS_SUCCESS);

    *ppResponseBuffer = pResponseBuffer;
    *pusResponseBufferLen = usResponseBufferLen;

cleanup:

    return ntStatus;

error:

    *ppResponseBuffer = NULL;
    *pusResponseBufferLen = 0;

    if (pResponseBuffer)
    {
        LwRtlMemoryFree(pResponseBuffer);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteIoctl(
    PSMB_SRV_FILE pFile,
    PBYTE         pData,
    ULONG         ulDataLen,
    ULONG         ulControlCode,
    PBYTE*        ppResponseBuffer,
    PUSHORT       pusResponseBufferLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pResponseBuffer = NULL;
    USHORT   usResponseBufferLen = 0;
    USHORT   usActualResponseLen = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    ntStatus = LW_RTL_ALLOCATE(
                    &pResponseBuffer,
                    BYTE,
                    512);
    BAIL_ON_NT_STATUS(ntStatus);

    usResponseBufferLen = 512;

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
                        usResponseBufferLen);
        if (ntStatus == STATUS_BUFFER_TOO_SMALL)
        {
            USHORT usNewLength = 0;

            if ((usResponseBufferLen + 256) > UINT16_MAX)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            usNewLength = usResponseBufferLen + 256;

            if (pResponseBuffer)
            {
                LwRtlMemoryFree(pResponseBuffer);
                pResponseBuffer = NULL;
                usResponseBufferLen = 0;
            }

            ntStatus = LW_RTL_ALLOCATE(
                            &pResponseBuffer,
                            BYTE,
                            usNewLength);
            BAIL_ON_NT_STATUS(ntStatus);

            usResponseBufferLen = usNewLength;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        usActualResponseLen = ioStatusBlock.BytesTransferred;

    } while (ntStatus != STATUS_SUCCESS);

    *ppResponseBuffer = pResponseBuffer;
    *pusResponseBufferLen = usResponseBufferLen;

cleanup:

    return ntStatus;

error:

    *ppResponseBuffer = NULL;
    *pusResponseBufferLen = 0;

    if (pResponseBuffer)
    {
        LwRtlMemoryFree(pResponseBuffer);
    }

    goto cleanup;
}


