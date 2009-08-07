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
    PSRV_EXEC_CONTEXT pExecContext,
    PBYTE             pParameters,
    ULONG             ulParameterCount,
    ULONG             ulMaxDataCount
    );

static
NTSTATUS
SrvSetSecurityDescriptor(
    PSRV_EXEC_CONTEXT pExecContext,
    PBYTE             pParameters,
    ULONG             ulParameterCount,
    PBYTE             pData,
    ULONG             ulDataLen
    );

static
NTSTATUS
SrvProcessIOCTL(
    PSRV_EXEC_CONTEXT pExecContext,
    PUSHORT           pSetup,
    UCHAR             ucSetupCount,
    PBYTE             pParameters,
    ULONG             ulParameterCount,
    PBYTE             pData,
    ULONG             ulDataLen
    );

static
NTSTATUS
SrvExecuteFsctl(
    PLWIO_SRV_FILE pFile,
    PBYTE          pData,
    ULONG          ulDataLen,
    ULONG          ulFunctionCode,
    PBYTE*         ppResponseBuffer,
    PUSHORT        pusResponseBufferLen
    );

static
NTSTATUS
SrvExecuteIoctl(
    PLWIO_SRV_FILE pFile,
    PBYTE          pData,
    ULONG          ulDataLen,
    ULONG          ulControlCode,
    PBYTE*         ppResponseBuffer,
    PUSHORT        pusResponseBufferLen
    );

NTSTATUS
SrvProcessNtTransact(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PNT_TRANSACTION_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PUSHORT                        pusBytecount   = NULL; // Do not free
    PUSHORT                        pSetup         = NULL; // Do not free
    PBYTE                          pParameters    = NULL; // Do not free
    PBYTE                          pData          = NULL; // Do not free
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree    = NULL;
    PLWIO_SRV_FILE    pFile    = NULL;

    ntStatus = SrvConnectionFindSession_SMB_V1(
                    pCtxSmb1,
                    pConnection,
                    pSmbRequest->pHeader->uid,
                    &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSessionFindTree_SMB_V1(
                    pCtxSmb1,
                    pSession,
                    pSmbRequest->pHeader->tid,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireUnmarshallNtTransactionRequest(
                    pBuffer,
                    ulBytesAvailable,
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
                            pExecContext,
                            pParameters,
                            pRequestHeader->ulTotalParameterCount,
                            pRequestHeader->ulMaxDataCount);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_SET_SECURITY_DESC :

            ntStatus = SrvSetSecurityDescriptor(
                            pExecContext,
                            pParameters,
                            pRequestHeader->ulTotalParameterCount,
                            pData,
                            pRequestHeader->ulTotalDataCount);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_IOCTL :

            ntStatus = SrvProcessIOCTL(
                            pExecContext,
                            pSetup,
                            pRequestHeader->ucSetupCount,
                            pParameters,
                            pRequestHeader->ulTotalParameterCount,
                            pData,
                            pRequestHeader->ulTotalDataCount);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_CREATE :
        case SMB_SUB_COMMAND_NT_TRANSACT_NOTIFY_CHANGE :
        case SMB_SUB_COMMAND_NT_TRANSACT_RENAME :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

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

    goto cleanup;
}

static
NTSTATUS
SrvQuerySecurityDescriptor(
    PSRV_EXEC_CONTEXT pExecContext,
    PBYTE             pParameters,
    ULONG             ulParameterCount,
    ULONG             ulMaxDataCount
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG  ulPackageBytesUsed  = 0;
    ULONG  ulTotalBytesUsed    = 0;
    PLWIO_SRV_FILE pFile = NULL;
    PUSHORT        pSetup = NULL;
    UCHAR          ucSetupCount = 0;
    PBYTE          pSecurityDescriptor = NULL;
    ULONG          ulSecurityDescInitialLen = 256;
    ULONG          ulSecurityDescAllocLen = 0;
    ULONG          ulSecurityDescActualLen = 0;
    ULONG          ulDataOffset = 0;
    ULONG          ulParameterOffset = 0;
    PSMB_SECURITY_INFORMATION_HEADER pQueryRequest = NULL;

    if (ulParameterCount != sizeof(SMB_SECURITY_INFORMATION_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pQueryRequest = (PSMB_SECURITY_INFORMATION_HEADER)pParameters;

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
                    pCtxSmb1->pTree,
                    pQueryRequest->usFid,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateMemory(
                    ulSecurityDescInitialLen,
                    (PVOID*)&pSecurityDescriptor);
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

            ntStatus = SrvAllocateMemory(ulNewLen, (PVOID*)&pNewMemory);
            BAIL_ON_NT_STATUS(ntStatus);

            if (pSecurityDescriptor)
            {
                SrvFreeMemory(pSecurityDescriptor);
            }

            pSecurityDescriptor = pNewMemory;
            ulSecurityDescAllocLen = ulNewLen;

            continue;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        ulSecurityDescActualLen = ioStatusBlock.BytesTransferred;

    } while (ntStatus != STATUS_SUCCESS);

    ntStatus = SrvMarshalHeader_SMB_V1(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM_NT_TRANSACT,
                (ulSecurityDescActualLen <= ulMaxDataCount ? STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL),
                TRUE,
                pCtxSmb1->pTree->tid,
                SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                pCtxSmb1->pSession->uid,
                pSmbRequest->pHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                &pSmbResponse->pHeader,
                &pSmbResponse->pAndXHeader,
                &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 18 + ucSetupCount;

    ntStatus = WireMarshallNtTransactionResponse(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    ucSetupCount,
                    (PBYTE)&ulSecurityDescActualLen,
                    sizeof(ulSecurityDescActualLen),
                    (ulSecurityDescActualLen <= ulMaxDataCount ? pSecurityDescriptor : NULL),
                    (ulSecurityDescActualLen <= ulMaxDataCount ? ulSecurityDescActualLen : 0),
                    &ulDataOffset,
                    &ulParameterOffset,
                    &ulPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += ulPackageBytesUsed;
    // ulOffset         += ulPackageBytesUsed;
    // ulBytesAvailable -= ulPackageBytesUsed;
    ulTotalBytesUsed += ulPackageBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    if (pSecurityDescriptor)
    {
        SrvFreeMemory(pSecurityDescriptor);
    }

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->pAndXHeader = NULL;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
SrvSetSecurityDescriptor(
    PSRV_EXEC_CONTEXT pExecContext,
    PBYTE             pParameters,
    ULONG             ulParameterCount,
    PBYTE             pData,
    ULONG             ulDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG  ulPackageBytesUsed  = 0;
    ULONG ulTotalBytesUsed     = 0;
    PLWIO_SRV_FILE pFile = NULL;
    PUSHORT pSetup = NULL;
    UCHAR   ucSetupCount = 0;
    ULONG   ulDataOffset = 0;
    ULONG   ulParameterOffset = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PSMB_SECURITY_INFORMATION_HEADER pSecuritySetRequest = NULL;

    if (ulParameterCount != sizeof(SMB_SECURITY_INFORMATION_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSecuritySetRequest = (PSMB_SECURITY_INFORMATION_HEADER)pParameters;

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
                    pCtxSmb1->pTree,
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

    ntStatus = SrvMarshalHeader_SMB_V1(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM_NT_TRANSACT,
                STATUS_SUCCESS,
                TRUE,
                pCtxSmb1->pTree->tid,
                SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                pCtxSmb1->pSession->uid,
                pSmbRequest->pHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                &pSmbResponse->pHeader,
                &pSmbResponse->pAndXHeader,
                &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 18 + ucSetupCount;

    ntStatus = WireMarshallNtTransactionResponse(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    ucSetupCount,
                    NULL,
                    0,
                    NULL,
                    0,
                    &ulDataOffset,
                    &ulParameterOffset,
                    &ulPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += ulPackageBytesUsed;
    // ulOffset         += ulPackageBytesUsed;
    // ulBytesAvailable -= ulPackageBytesUsed;
    ulTotalBytesUsed += ulPackageBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->pAndXHeader = NULL;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
SrvProcessIOCTL(
    PSRV_EXEC_CONTEXT pExecContext,
    PUSHORT           pSetup,
    UCHAR             ucSetupCount,
    PBYTE             pParameters,
    ULONG             ulParameterCount,
    PBYTE             pData,
    ULONG             ulDataLen
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG  ulPackageBytesUsed  = 0;
    ULONG ulTotalBytesUsed     = 0;
    PLWIO_SRV_FILE pFile = NULL;
    UCHAR   ucResponseSetupCount = 1;
    ULONG   ulDataOffset = 0;
    ULONG   ulParameterOffset = 0;
    PSMB_IOCTL_HEADER pIoctlRequest = NULL;
    PBYTE   pResponseBuffer = NULL;
    USHORT  usResponseBufferLen = 0;

    if (pSetup)
    {
        if (sizeof(SMB_IOCTL_HEADER) != ucSetupCount * sizeof(USHORT))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pIoctlRequest = (PSMB_IOCTL_HEADER)((PBYTE)pSetup);

    }
    else if (ulParameterCount == sizeof(SMB_IOCTL_HEADER))
    {
        pIoctlRequest = (PSMB_IOCTL_HEADER)pParameters;
    }
    else
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pIoctlRequest->ucFlags & 0x1)
    {
        // TODO: Apply only to DFS Share
        //       We don't support DFS yet
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
                    pCtxSmb1->pTree,
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

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_NT_TRANSACT,
                    STATUS_SUCCESS,
                    TRUE,
                    pCtxSmb1->pTree->tid,
                    SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                    pCtxSmb1->pSession->uid,
                    pSmbRequest->pHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 18 + ucResponseSetupCount;

    ntStatus = WireMarshallNtTransactionResponse(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &usResponseBufferLen,
                    ucResponseSetupCount,
                    NULL,
                    0,
                    pResponseBuffer,
                    usResponseBufferLen,
                    &ulDataOffset,
                    &ulParameterOffset,
                    &ulPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += ulPackageBytesUsed;
    // ulOffset         += ulPackageBytesUsed;
    // ulBytesAvailable -= ulPackageBytesUsed;
    ulTotalBytesUsed += ulPackageBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    if (pResponseBuffer)
    {
        SrvFreeMemory(pResponseBuffer);
    }

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader = NULL;
        pSmbResponse->pAndXHeader = NULL;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}

static
NTSTATUS
SrvExecuteFsctl(
    PLWIO_SRV_FILE pFile,
    PBYTE          pData,
    ULONG          ulDataLen,
    ULONG          ulFunctionCode,
    PBYTE*         ppResponseBuffer,
    PUSHORT        pusResponseBufferLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pResponseBuffer = NULL;
    USHORT   usResponseBufferLen = 0;
    USHORT   usActualResponseLen = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    ntStatus = SrvAllocateMemory(512, (PVOID*) &pResponseBuffer);
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
                SrvFreeMemory(pResponseBuffer);
                pResponseBuffer = NULL;
                usResponseBufferLen = 0;
            }

            ntStatus = SrvAllocateMemory(usNewLength, (PVOID*)&pResponseBuffer);
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
        SrvFreeMemory(pResponseBuffer);
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteIoctl(
    PLWIO_SRV_FILE pFile,
    PBYTE          pData,
    ULONG          ulDataLen,
    ULONG          ulControlCode,
    PBYTE*         ppResponseBuffer,
    PUSHORT        pusResponseBufferLen
    )
{
    NTSTATUS ntStatus = 0;
    PBYTE    pResponseBuffer = NULL;
    USHORT   usResponseBufferLen = 0;
    USHORT   usActualResponseLen = 0;
    IO_STATUS_BLOCK ioStatusBlock = {0};

    ntStatus = SrvAllocateMemory(512, (PVOID*) &pResponseBuffer);
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
                SrvFreeMemory(pResponseBuffer);
                pResponseBuffer = NULL;
                usResponseBufferLen = 0;
            }

            ntStatus = SrvAllocateMemory(usNewLength, (PVOID*)&pResponseBuffer);
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
        SrvFreeMemory(pResponseBuffer);
    }

    goto cleanup;
}


