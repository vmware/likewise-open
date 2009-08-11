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
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData
    );

static
NTSTATUS
SrvProcessGetNamedPipeHandleState(
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData
    );

static
NTSTATUS
SrvProcessGetNamedPipeInfo(
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData
    );

static
NTSTATUS
SrvProcessTransactNamedPipe(
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData
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
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
    ULONG ulOffset         = pSmbRequest->usHeaderSize;
    ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
    PTRANSACTION_REQUEST_HEADER pRequestHeader = NULL; // Do not free
    PUSHORT                     pBytecount     = NULL; // Do not free
    PWSTR                       pwszName       = NULL; // Do not free
    PUSHORT                     pSetup         = NULL; // Do not free
    PBYTE                       pParameters    = NULL; // Do not free
    PBYTE                       pData          = NULL; // Do not free

    ntStatus = WireUnmarshallTransactionRequest(
                    pBuffer,
                    ulBytesAvailable,
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
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    switch (*pSetup)
    {
        case SMB_SUB_COMMAND_TRANS_SET_NAMED_PIPE_HANDLE_STATE:

            ntStatus = SrvProcessSetNamedPipeHandleState(
                            pExecContext,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pwszName,
                            pParameters,
                            pData);

            break;

        case SMB_SUB_COMMAND_TRANS_QUERY_NAMED_PIPE_HANDLE_STATE:

            ntStatus = SrvProcessGetNamedPipeHandleState(
                            pExecContext,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pwszName,
                            pParameters,
                            pData);

            break;

        case SMB_SUB_COMMAND_TRANS_QUERY_NAMED_PIPE_INFO:

            ntStatus = SrvProcessGetNamedPipeInfo(
                            pExecContext,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pwszName,
                            pParameters,
                            pData);

            break;

        case SMB_SUB_COMMAND_TRANS_TRANSACT_NAMED_PIPE:

            ntStatus = SrvProcessTransactNamedPipe(
                            pExecContext,
                            pRequestHeader,
                            pSetup,
                            pBytecount,
                            pwszName,
                            pParameters,
                            pData);

            break;

        case SMB_SUB_COMMAND_TRANS_RAW_READ_NAMED_PIPE:
        case SMB_SUB_COMMAND_TRANS_PEEK_NAMED_PIPE:
        case SMB_SUB_COMMAND_TRANS_RAW_WRITE_NAMED_PIPE:
        case SMB_SUB_COMMAND_TRANS_WAIT_NAMED_PIPE:
        case SMB_SUB_COMMAND_TRANS_CALL_NAMED_PIPE:

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvProcessSetNamedPipeHandleState(
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData
    )
{
    NTSTATUS                   ntStatus     = 0;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    PLWIO_SRV_FILE    pFile = NULL;
    FILE_PIPE_INFORMATION pipeInfo = {0};
    IO_STATUS_BLOCK ioStatusBlock = {0};
    USHORT usDataOffset = 0;
    USHORT usParameterOffset = 0;

    if ((pRequestHeader->setupCount != 2) ||
        (pRequestHeader->parameterCount != sizeof(USHORT)))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
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

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        STATUS_SUCCESS,
                        TRUE,
                        pCtxSmb1->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pCtxSmb1->pSession->uid,
                        pSmbRequest->pHeader->mid,
                        pConnection->serverProperties.bRequireSecuritySignatures,
                        &pSmbResponse->pHeader,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    else
    {
        ntStatus = SrvMarshalHeaderAndX_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 10;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    NULL,
                    0,
                    NULL,
                    0,
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

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
SrvProcessGetNamedPipeHandleState(
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData
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
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    FILE_PIPE_INFORMATION pipeInfo = {0};
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInfo = {0};
    IO_STATUS_BLOCK ioStatusBlock = {0};
    USHORT usDataOffset = 0;
    USHORT usParameterOffset = 0;
    USHORT usDeviceState = 0;

    if ((pRequestHeader->setupCount != 2) ||
        (pRequestHeader->maxParameterCount != sizeof(USHORT)) ||
        (pRequestHeader->maxDataCount != 0))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
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

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        STATUS_SUCCESS,
                        TRUE,
                        pCtxSmb1->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pCtxSmb1->pSession->uid,
                        pSmbRequest->pHeader->mid,
                        pConnection->serverProperties.bRequireSecuritySignatures,
                        &pSmbResponse->pHeader,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    else
    {
        ntStatus = SrvMarshalHeaderAndX_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 10;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    NULL,
                    0,
                    (PBYTE)&usDeviceState,
                    sizeof(usDeviceState),
                    NULL,
                    0,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

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
SrvProcessGetNamedPipeInfo(
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData
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
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE pTree = NULL;
    PLWIO_SRV_FILE pFile = NULL;
    FILE_PIPE_LOCAL_INFORMATION pipeLocalInfo = {0};
    IO_STATUS_BLOCK ioStatusBlock = {0};
    PWSTR  pwszFilePath = NULL;
    wchar16_t wszPipenamePrefix[] = {'\\', '\\', 'P', 'I', 'P', 'E', 0};
    PBYTE  pResponseData = NULL;
    USHORT usDataOffset = 0;
    USHORT usParameterOffset = 0;
    USHORT usResponseDataLen = 0;

    if ((pRequestHeader->setupCount != 2) ||
        (pRequestHeader->totalParameterCount != sizeof(USHORT)) ||
        (pRequestHeader->maxDataCount == 0))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (*((PUSHORT)pParameters) != 1)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
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

    ntStatus = SrvBuildFilePath(
                    &wszPipenamePrefix[0],
                    pFile->pwszFilename,
                    &pwszFilePath);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        STATUS_SUCCESS,
                        TRUE,
                        pCtxSmb1->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pCtxSmb1->pSession->uid,
                        pSmbRequest->pHeader->mid,
                        pConnection->serverProperties.bRequireSecuritySignatures,
                        &pSmbResponse->pHeader,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    else
    {
        ntStatus = SrvMarshalHeaderAndX_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 10;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    NULL,
                    0,
                    NULL,
                    0,
                    pResponseData,
                    usResponseDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
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
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    NULL,
                    0,
                    NULL,
                    0,
                    pResponseData,
                    usResponseDataLen,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

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

    if (pwszFilePath)
    {
        SrvFreeMemory(pwszFilePath);
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
SrvProcessTransactNamedPipe(
    PSRV_EXEC_CONTEXT           pExecContext,
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pSetup,
    PUSHORT                     pByteCount,
    PWSTR                       pwszName,
    PBYTE                       pParameters,
    PBYTE                       pData
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
    USHORT usBytesUsed          = 0;
    ULONG ulTotalBytesUsed     = 0;
    PLWIO_SRV_SESSION pSession = NULL;
    PLWIO_SRV_TREE    pTree = NULL;
    PLWIO_SRV_FILE    pFile = NULL;
    IO_STATUS_BLOCK   ioStatusBlock = {0};
    PBYTE  pResponseData = NULL;
    USHORT usDataOffset = 0;
    USHORT usParameterOffset = 0;
    USHORT usResponseDataLen = 0;
    LONG64 llOffset = 0;

    if ((pRequestHeader->setupCount != 2) ||
        (pRequestHeader->maxDataCount == 0))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    ntStatus = SrvTreeFindFile_SMB_V1(
                    pCtxSmb1,
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
                    &llOffset,
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

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        ioStatusBlock.BytesTransferred < usResponseDataLen ?
                             STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW,
                        TRUE,
                        pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pSession->uid,
                        pSmbRequest->pHeader->mid,
                        pConnection->serverProperties.bRequireSecuritySignatures,
                        &pSmbResponse->pHeader,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    else
    {
        ntStatus = SrvMarshalHeaderAndX_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 10;

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    NULL,
                    0,
                    NULL,
                    0,
                    pResponseData,
                    ioStatusBlock.BytesTransferred,
                    &usDataOffset,
                    &usParameterOffset,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += usBytesUsed;
    // ulOffset         += usBytesUsed;
    // ulBytesAvailable -= usBytesUsed;
    ulTotalBytesUsed += usBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

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
