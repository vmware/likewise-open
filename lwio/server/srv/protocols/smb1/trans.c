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

typedef struct _SMB_GET_NAMED_PIPE_INFO_DATA
{
    USHORT usOutputBufferSize;
    USHORT usInputBufferSize;
    UCHAR  ucMaximumInstances;
    UCHAR  ucCurrentInstances;
    UCHAR  ucPipeNameLength;
//  WSTR   wszPipeName[1];

} __attribute__((__packed__)) SMB_GET_NAMED_PIPE_INFO_DATA, *PSMB_GET_NAMED_PIPE_INFO_DATA;

static
NTSTATUS
SrvProcessSetNamedPipeHandleState(
    PLWIO_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    );

static
NTSTATUS
SrvProcessGetNamedPipeHandleState(
    PLWIO_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    );

static
NTSTATUS
SrvProcessGetNamedPipeInfo(
    PLWIO_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    );

static
NTSTATUS
SrvProcessTransactNamedPipe(
    PLWIO_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    );

static
NTSTATUS
SrvMarshallGetNamedPipeInfoData(
    ULONG   ulInputBufferSize,
    ULONG   ulOutputBufferSize,
    ULONG   ulCurrentInstances,
    ULONG   ulMaximumInstances,
    PWSTR   pwszFilePath,
    USHORT  usMaxDataCount,
    USHORT  usDataOffset,
    PBYTE*  ppResponseData,
    PUSHORT pusResponseDataLen
    );

NTSTATUS
SrvProcessTransaction(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PTRANSACTION_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PUSHORT pBytecount = NULL; // Do not free
    PWSTR   pwszName = NULL; // Do not free
    PUSHORT pSetup = NULL; // Do not free
    PBYTE   pParameters = NULL; // Do not free
    PBYTE   pData = NULL; // Do not free
    PSMB_PACKET pSmbResponse = NULL;
    ULONG   ulOffset = 0;

    ulOffset = (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader;

    ntStatus = WireUnmarshallTransactionRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    ulOffset,
                    &pRequestHeader,
                    &pSetup,
                    &pBytecount,
                    &pwszName,
                    &pParameters,
                    &pData);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pSetup == NULL)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    switch (*pSetup)
    {
        case SMB_SUB_COMMAND_TRANS_SET_NAMED_PIPE_HANDLE_STATE:

            ntStatus = SrvProcessSetNamedPipeHandleState(
                            pConnection,
                            pSmbRequest,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pwszName,
                            pParameters,
                            pData,
                            &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS_RAW_READ_NAMED_PIPE:

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS_QUERY_NAMED_PIPE_HANDLE_STATE:

            ntStatus = SrvProcessGetNamedPipeHandleState(
                            pConnection,
                            pSmbRequest,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pwszName,
                            pParameters,
                            pData,
                            &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS_QUERY_NAMED_PIPE_INFO:

            ntStatus = SrvProcessGetNamedPipeInfo(
                            pConnection,
                            pSmbRequest,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pwszName,
                            pParameters,
                            pData,
                            &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS_PEEK_NAMED_PIPE:

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS_TRANSACT_NAMED_PIPE:

            ntStatus = SrvProcessTransactNamedPipe(
                            pConnection,
                            pSmbRequest,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pwszName,
                            pParameters,
                            pData,
                            &pSmbResponse);

            break;

        case SMB_SUB_COMMAND_TRANS_RAW_WRITE_NAMED_PIPE:

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS_WAIT_NAMED_PIPE:

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        case SMB_SUB_COMMAND_TRANS_CALL_NAMED_PIPE:

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;

        default:

            ntStatus = STATUS_DATA_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvProcessSetNamedPipeHandleState(
    PLWIO_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    FILE_PIPE_INFORMATION pipeInfo = {0};
    IO_STATUS_BLOCK ioStatusBlock = {0};
    USHORT usDataOffset = 0;
    USHORT usParameterOffset = 0;
    USHORT usNumPackageBytesUsed = 0;

    if ((pRequestHeader->setupCount != 2) ||
        (pRequestHeader->parameterCount != sizeof(USHORT)))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pSetup[1],
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    pipeInfo.CompletionMode = (*pParameters & 0x80) ? PIPE_NOWAIT : PIPE_WAIT;
    pipeInfo.ReadMode = (*pParameters & 0x1) ? PIPE_READMODE_MESSAGE : PIPE_READMODE_BYTE;

    ntStatus = IoSetInformationFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &pipeInfo,
                    sizeof(pipeInfo),
                    FilePipeInformation);
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
                COM_TRANSACTION,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    NULL,
                    0,
                    NULL,
                    0,
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
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
        SMBPacketRelease(
             pConnection->hPacketAllocator,
             pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvProcessGetNamedPipeHandleState(
    PLWIO_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    FILE_PIPE_INFORMATION pipeInfo = {0};
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInfo = {0};
    IO_STATUS_BLOCK ioStatusBlock = {0};
    USHORT usNumPackageBytesUsed = 0;
    USHORT usDataOffset = 0;
    USHORT usParameterOffset = 0;
    USHORT usDeviceState = 0;

    if ((pRequestHeader->setupCount != 2) ||
        (pRequestHeader->maxParameterCount != sizeof(USHORT)) ||
        (pRequestHeader->maxDataCount != 0))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pSetup[1],
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoQueryInformationFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &pipeInfo,
                    sizeof(pipeInfo),
                    FilePipeInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoQueryInformationFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &pipeLocalInfo,
                    sizeof(pipeLocalInfo),
                    FilePipeLocalInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshallPipeInfo(
                    &pipeInfo,
                    &pipeLocalInfo,
                    &usDeviceState);
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
                COM_TRANSACTION,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    NULL,
                    0,
                    (PBYTE)&usDeviceState,
                    sizeof(usDeviceState),
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
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
        SMBPacketRelease(
             pConnection->hPacketAllocator,
             pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvProcessGetNamedPipeInfo(
    PLWIO_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInfo = {0};
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PWSTR  pwszFilePath = NULL;
    PWSTR  pwszPipenamePrefix = NULL;
    USHORT usNumPackageBytesUsed = 0;
    PBYTE  pResponseData = NULL;
    USHORT usDataOffset = 0;
    USHORT usParameterOffset = 0;
    USHORT usResponseDataLen = 0;

    if ((pRequestHeader->setupCount != 2) ||
        (pRequestHeader->totalParameterCount != sizeof(USHORT)) ||
        (pRequestHeader->maxDataCount == 0))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (*((PUSHORT)pParameters) != 1)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pSetup[1],
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoQueryInformationFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    &pipeLocalInfo,
                    sizeof(pipeLocalInfo),
                    FilePipeLocalInformation);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlWC16StringAllocateFromCString(
                    &pwszPipenamePrefix,
                    "\\PIPE");
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildFilePath(
                    pwszPipenamePrefix,
                    pFile->pwszFilename,
                    &pwszFilePath);
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
                COM_TRANSACTION,
                0,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    NULL,
                    0,
                    NULL,
                    0,
                    pResponseData,
                    usResponseDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // We need the data offset to justify the alignment within the data
    ntStatus = SrvMarshallGetNamedPipeInfoData(
                    pipeLocalInfo.InboundQuota,
                    pipeLocalInfo.OutboundQuota,
                    pipeLocalInfo.CurrentInstances,
                    pipeLocalInfo.MaximumInstances,
                    pwszFilePath,
                    pRequestHeader->maxDataCount,
                    usDataOffset,
                    &pResponseData,
                    &usResponseDataLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    NULL,
                    0,
                    NULL,
                    0,
                    pResponseData,
                    usResponseDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
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

    if (pwszPipenamePrefix)
    {
        SrvFreeMemory(pwszPipenamePrefix);
    }
    if (pwszFilePath)
    {
        SrvFreeMemory(pwszFilePath);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
             pConnection->hPacketAllocator,
             pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvProcessTransactNamedPipe(
    PLWIO_SRV_CONNECTION         pConnection,
    PSMB_PACKET                 pSmbRequest,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSMB_PACKET*                ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    IO_STATUS_BLOCK ioStatusBlock = {0};
    USHORT usNumPackageBytesUsed = 0;
    PBYTE  pResponseData = NULL;
    USHORT usDataOffset = 0;
    USHORT usParameterOffset = 0;
    USHORT usResponseDataLen = 0;
    LONG64 ullOffset = 0;

    if ((pRequestHeader->setupCount != 2) ||
        (pRequestHeader->maxDataCount == 0))
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    ntStatus = SrvTreeFindFile(
                    pTree,
                    pSetup[1],
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO:
    //
    // The call must fail in the following events.
    // a. the pipe has any data remaining
    // b. the pipe is not in message mode
    ntStatus = IoWriteFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pData,
                    pRequestHeader->dataCount,
                    &ullOffset,
                    NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: Make sure we have enough space in the reply buffer for this
    usResponseDataLen = pRequestHeader->maxDataCount;

    ntStatus = SrvAllocateMemory(usResponseDataLen, (PVOID*)&pResponseData);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoReadFile(
                    pFile->hFile,
                    NULL,
                    &ioStatusBlock,
                    pResponseData,
                    usResponseDataLen,
                    NULL,
                    NULL);
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
                COM_TRANSACTION,
                ioStatusBlock.BytesTransferred < usResponseDataLen ?
                         STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 10;

    ntStatus = WireMarshallTransaction2Response(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    NULL,
                    0,
                    NULL,
                    0,
                    pResponseData,
                    ioStatusBlock.BytesTransferred,
                    &usDataOffset,
                    &usParameterOffset,
                    &usNumPackageBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += usNumPackageBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
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

    if (pResponseData)
    {
        SrvFreeMemory(pResponseData);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
             pConnection->hPacketAllocator,
             pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvMarshallGetNamedPipeInfoData(
    ULONG   ulInputBufferSize,
    ULONG   ulOutputBufferSize,
    ULONG   ulCurrentInstances,
    ULONG   ulMaximumInstances,
    PWSTR   pwszFilePath,
    USHORT  usMaxDataCount,
    USHORT  usDataOffset,
    PBYTE*  ppResponseData,
    PUSHORT pusResponseDataLen
    )
{
    NTSTATUS ntStatus = 0;
    USHORT   usResponseDataLen = 0;
    PBYTE    pResponseDataBuffer = NULL;
    PBYTE    pResponseDataCursor = NULL;
    PSMB_GET_NAMED_PIPE_INFO_DATA pResponseData = NULL;
    size_t   sFilePathLen = 0;

    sFilePathLen = wc16slen(pwszFilePath);

    usResponseDataLen = sizeof(SMB_GET_NAMED_PIPE_INFO_DATA) +
                        (usDataOffset + sizeof(SMB_GET_NAMED_PIPE_INFO_DATA)) % 2 +
                        (sFilePathLen + 1) * sizeof(wchar16_t);

    if (usResponseDataLen > usMaxDataCount)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(
                    usResponseDataLen,
                    (PVOID*)&pResponseDataBuffer);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseDataCursor = pResponseDataBuffer;
    pResponseData = (PSMB_GET_NAMED_PIPE_INFO_DATA)pResponseDataCursor;
    pResponseDataCursor += sizeof(SMB_GET_NAMED_PIPE_INFO_DATA);

    pResponseData->usOutputBufferSize = SMB_MIN(ulOutputBufferSize, UINT16_MAX);
    pResponseData->usInputBufferSize  = SMB_MIN(ulInputBufferSize,  UINT16_MAX);
    pResponseData->ucCurrentInstances = (BYTE)SMB_MIN(ulCurrentInstances, 0xFF);
    pResponseData->ucMaximumInstances = (BYTE)SMB_MIN(ulMaximumInstances, 0xFF);

    if ((usDataOffset + sizeof(SMB_GET_NAMED_PIPE_INFO_DATA)) % 2)
    {
        *pResponseDataCursor++ = 0x2;
    }

    memcpy(pResponseDataCursor, (PBYTE)pwszFilePath, sFilePathLen * sizeof(wchar16_t));

    *ppResponseData = pResponseDataBuffer;
    *pusResponseDataLen = usResponseDataLen;

cleanup:

    return ntStatus;

error:

    *ppResponseData = NULL;
    *pusResponseDataLen = 0;

    if (pResponseData)
    {
        SrvFreeMemory(pResponseData);
    }

    goto cleanup;
}
