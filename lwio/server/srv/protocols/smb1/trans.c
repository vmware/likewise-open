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

} __attribute__((__packed__)) SMB_GET_NAMED_PIPE_INFO_DATA,
                             *PSMB_GET_NAMED_PIPE_INFO_DATA;

static
NTSTATUS
SrvProcessSetNamedPipeHandleState(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildSetNamedPipeHandleStateResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvProcessGetNamedPipeHandleState(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildGetNamedPipeHandleStateResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvProcessGetNamedPipeInfo(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildGetNamedPipeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvProcessTransactNamedPipe(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildTransactNamedPipeResponse(
    PSRV_EXEC_CONTEXT pExecContext
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

static
NTSTATUS
SrvBuildTransState(
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pBytecount,
    PWSTR                       pwszName,
    PUSHORT                     pSetup,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSRV_TRANS_STATE_SMB_V1*    ppTransState
    );

VOID
SrvPrepareTransStateAsync(
    PSRV_TRANS_STATE_SMB_V1 pTransState,
    PSRV_EXEC_CONTEXT       pExecContext
    );

static
VOID
SrvExecuteTransAsyncCB(
    PVOID pContext
    );

VOID
SrvReleaseTransStateAsync(
    PSRV_TRANS_STATE_SMB_V1 pTransState
    );

static
VOID
SrvReleaseTransStateHandle(
    HANDLE hTransState
    );

static
VOID
SrvReleaseTransState(
    PSRV_TRANS_STATE_SMB_V1 pTransState
    );

static
VOID
SrvFreeTransState(
    PSRV_TRANS_STATE_SMB_V1 pTransState
    );

NTSTATUS
SrvProcessTransaction(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS_STATE_SMB_V1    pTransState  = NULL;
    BOOLEAN                    bInLock      = FALSE;

    pTransState = (PSRV_TRANS_STATE_SMB_V1)pCtxSmb1->hState;

    if (pTransState)
    {
        InterlockedIncrement(&pTransState->refCount);
    }
    else
    {
        ULONG               iMsg        = pCtxSmb1->iMsg;
        PSRV_MESSAGE_SMB_V1 pSmbRequest = &pCtxSmb1->pRequests[iMsg];
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

        ntStatus = SrvBuildTransState(
                        pRequestHeader,
                        pBytecount,
                        pwszName,
                        pSetup,
                        pParameters,
                        pData,
                        &pTransState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pTransState;
        InterlockedIncrement(&pTransState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseTransStateHandle;
    }

    LWIO_LOCK_MUTEX(bInLock, &pTransState->mutex);

    switch (*pTransState->pSetup)
    {
        case SMB_SUB_COMMAND_TRANS_SET_NAMED_PIPE_HANDLE_STATE:

            ntStatus = SrvProcessSetNamedPipeHandleState(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS_QUERY_NAMED_PIPE_HANDLE_STATE:

            ntStatus = SrvProcessGetNamedPipeHandleState(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS_QUERY_NAMED_PIPE_INFO:

            ntStatus = SrvProcessGetNamedPipeInfo(pExecContext);

            break;

        case SMB_SUB_COMMAND_TRANS_TRANSACT_NAMED_PIPE:

            ntStatus = SrvProcessTransactNamedPipe(pExecContext);

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

    if (pTransState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pTransState->mutex);

        SrvReleaseTransState(pTransState);
    }

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            // TODO: Add an indicator to the file object to trigger a
            //       cleanup if the connection gets closed and all the
            //       files involved have to be closed

            break;

        default:

            if (pTransState)
            {
                SrvReleaseTransStateAsync(pTransState);
            }

            break;
    }

    goto cleanup;
}

static
NTSTATUS
SrvProcessSetNamedPipeHandleState(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_TRANS_STATE_SMB_V1    pTransState  = NULL;

    pTransState = (PSRV_TRANS_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pTransState->stage)
    {
        case SRV_TRANS_STAGE_SMB_V1_INITIAL:

            if ((pTransState->pRequestHeader->setupCount != 2) ||
                (pTransState->pRequestHeader->parameterCount != sizeof(USHORT)))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = SrvConnectionFindSession_SMB_V1(
                                pCtxSmb1,
                                pConnection,
                                pSmbRequest->pHeader->uid,
                                &pTransState->pSession);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSessionFindTree_SMB_V1(
                            pCtxSmb1,
                            pTransState->pSession,
                            pSmbRequest->pHeader->tid,
                            &pTransState->pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pTransState->pTree,
                            pTransState->pSetup[1],
                            &pTransState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->pipeInfo.CompletionMode =
                (*pTransState->pParameters & 0x80) ? PIPE_NOWAIT : PIPE_WAIT;

            pTransState->pipeInfo.ReadMode =
                (*pTransState->pParameters & 0x1) ? PIPE_READMODE_MESSAGE :
                                                    PIPE_READMODE_BYTE;

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_ATTEMPT_IO:

            if (!pTransState->pPipeInfo)
            {
                pTransState->pPipeInfo = &pTransState->pipeInfo;

                SrvPrepareTransStateAsync(pTransState, pExecContext);

                ntStatus = IoSetInformationFile(
                                pTransState->pFile->hFile,
                                pTransState->pAcb,
                                &pTransState->ioStatusBlock,
                                pTransState->pPipeInfo,
                                sizeof(pTransState->pipeInfo),
                                FilePipeInformation);
                BAIL_ON_NT_STATUS(ntStatus);

                SrvReleaseTransStateAsync(pTransState); // completed sync
            }
            else
            {
                ntStatus = pTransState->ioStatusBlock.Status;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildSetNamedPipeHandleStateResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_DONE:

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildSetNamedPipeHandleStateResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS_STATE_SMB_V1    pTransState  = NULL;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE  pOutBuffer        = pSmbResponse->pBuffer;
    ULONG  ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG  ulOffset          = 0;
    USHORT usBytesUsed       = 0;
    ULONG  ulTotalBytesUsed  = 0;
    USHORT usDataOffset      = 0;
    USHORT usParameterOffset = 0;

    pTransState = (PSRV_TRANS_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        STATUS_SUCCESS,
                        TRUE,
                        pTransState->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pTransState->pSession->uid,
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
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_TRANS_STATE_SMB_V1    pTransState  = NULL;

    pTransState = (PSRV_TRANS_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pTransState->stage)
    {
        case SRV_TRANS_STAGE_SMB_V1_INITIAL:

            if ((pTransState->pRequestHeader->setupCount != 2) ||
                (pTransState->pRequestHeader->maxParameterCount != sizeof(USHORT)) ||
                (pTransState->pRequestHeader->maxDataCount != 0))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = SrvConnectionFindSession_SMB_V1(
                            pCtxSmb1,
                            pConnection,
                            pSmbRequest->pHeader->uid,
                            &pTransState->pSession);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSessionFindTree_SMB_V1(
                            pCtxSmb1,
                            pTransState->pSession,
                            pSmbRequest->pHeader->tid,
                            &pTransState->pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pTransState->pTree,
                            pTransState->pSetup[1],
                            &pTransState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_ATTEMPT_IO:

            ntStatus = pTransState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            if (!pTransState->pPipeInfo)
            {
                pTransState->pPipeInfo = &pTransState->pipeInfo;

                SrvPrepareTransStateAsync(pTransState, pExecContext);

                ntStatus = IoQueryInformationFile(
                                pTransState->pFile->hFile,
                                pTransState->pAcb,
                                &pTransState->ioStatusBlock,
                                pTransState->pPipeInfo,
                                sizeof(pTransState->pipeInfo),
                                FilePipeInformation);
                BAIL_ON_NT_STATUS(ntStatus);

                SrvReleaseTransStateAsync(pTransState); // completed sync
            }

            if (!pTransState->pPipeLocalInfo)
            {
                pTransState->pPipeLocalInfo = &pTransState->pipeLocalInfo;

                SrvPrepareTransStateAsync(pTransState, pExecContext);

                ntStatus = IoQueryInformationFile(
                                pTransState->pFile->hFile,
                                pTransState->pAcb,
                                &pTransState->ioStatusBlock,
                                pTransState->pPipeLocalInfo,
                                sizeof(pTransState->pipeLocalInfo),
                                FilePipeLocalInformation);
                BAIL_ON_NT_STATUS(ntStatus);

                SrvReleaseTransStateAsync(pTransState); // completed sync
            }

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildGetNamedPipeHandleStateResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_DONE:

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildGetNamedPipeHandleStateResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS_STATE_SMB_V1    pTransState  = NULL;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE  pOutBuffer        = pSmbResponse->pBuffer;
    ULONG  ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG  ulOffset          = 0;
    USHORT usBytesUsed       = 0;
    ULONG  ulTotalBytesUsed  = 0;
    USHORT usDataOffset      = 0;
    USHORT usParameterOffset = 0;
    USHORT usDeviceState     = 0;

    pTransState = (PSRV_TRANS_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = SrvMarshallPipeInfo(
                    pTransState->pPipeInfo,
                    pTransState->pPipeLocalInfo,
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
                        pTransState->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pTransState->pSession->uid,
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
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS_STATE_SMB_V1    pTransState  = NULL;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];

    pTransState = (PSRV_TRANS_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pTransState->stage)
    {
        case SRV_TRANS_STAGE_SMB_V1_INITIAL:

            if ((pTransState->pRequestHeader->setupCount != 2) ||
                (pTransState->pRequestHeader->totalParameterCount != sizeof(USHORT)) ||
                (pTransState->pRequestHeader->maxDataCount == 0))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            if (*((PUSHORT)pTransState->pParameters) != 1)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = SrvConnectionFindSession_SMB_V1(
                            pCtxSmb1,
                            pConnection,
                            pSmbRequest->pHeader->uid,
                            &pTransState->pSession);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSessionFindTree_SMB_V1(
                            pCtxSmb1,
                            pTransState->pSession,
                            pSmbRequest->pHeader->tid,
                            &pTransState->pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pTransState->pTree,
                            pTransState->pSetup[1],
                            &pTransState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_ATTEMPT_IO:

            if (!pTransState->pPipeLocalInfo)
            {
                pTransState->pPipeLocalInfo = &pTransState->pipeLocalInfo;

                SrvPrepareTransStateAsync(pTransState, pExecContext);

                ntStatus = IoQueryInformationFile(
                                pTransState->pFile->hFile,
                                pTransState->pAcb,
                                &pTransState->ioStatusBlock,
                                pTransState->pPipeLocalInfo,
                                sizeof(pTransState->pipeLocalInfo),
                                FilePipeLocalInformation);
                BAIL_ON_NT_STATUS(ntStatus);

                SrvReleaseTransStateAsync(pTransState); // completed sync
            }
            else
            {
                ntStatus = pTransState->ioStatusBlock.Status;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildGetNamedPipeInfoResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_DONE:

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildGetNamedPipeInfoResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_TRANS_STATE_SMB_V1    pTransState  = NULL;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE     pOutBuffer          = pSmbResponse->pBuffer;
    ULONG     ulBytesAvailable    = pSmbResponse->ulBytesAvailable;
    ULONG     ulOffset            = 0;
    USHORT    usBytesUsed         = 0;
    ULONG     ulTotalBytesUsed    = 0;
    PWSTR     pwszFilePath        = NULL;
    USHORT    usDataOffset        = 0;
    USHORT    usParameterOffset   = 0;
    wchar16_t wszPipenamePrefix[] = {'\\', '\\', 'P', 'I', 'P', 'E', 0};

    pTransState = (PSRV_TRANS_STATE_SMB_V1)pCtxSmb1->hState;

    ntStatus = SrvBuildFilePath(
                    &wszPipenamePrefix[0],
                    pTransState->pFile->pwszFilename,
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
                        pTransState->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pTransState->pSession->uid,
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

    // We need the data offset to justify the alignment within the data
    ntStatus = SrvMarshallGetNamedPipeInfoData(
                    pTransState->pPipeLocalInfo->InboundQuota,
                    pTransState->pPipeLocalInfo->OutboundQuota,
                    pTransState->pPipeLocalInfo->CurrentInstances,
                    pTransState->pPipeLocalInfo->MaximumInstances,
                    pwszFilePath,
                    pTransState->pRequestHeader->maxDataCount,
                    usDataOffset,
                    &pTransState->pData2,
                    &pTransState->usBytesRead);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = WireMarshallTransaction2Response(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    NULL,
                    0,
                    NULL,
                    0,
                    pTransState->pData2,
                    pTransState->usBytesRead,
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
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_TRANS_STATE_SMB_V1    pTransState  = NULL;

    pTransState = (PSRV_TRANS_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pTransState->stage)
    {
        case SRV_TRANS_STAGE_SMB_V1_INITIAL:

            if ((pTransState->pRequestHeader->setupCount != 2) ||
                (pTransState->pRequestHeader->maxDataCount == 0))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = SrvConnectionFindSession_SMB_V1(
                            pCtxSmb1,
                            pConnection,
                            pSmbRequest->pHeader->uid,
                            &pTransState->pSession);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvSessionFindTree_SMB_V1(
                            pCtxSmb1,
                            pTransState->pSession,
                            pSmbRequest->pHeader->tid,
                            &pTransState->pTree);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pTransState->pTree,
                            pTransState->pSetup[1],
                            &pTransState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_ATTEMPT_WRITE;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_ATTEMPT_WRITE:

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_WRITE_COMPLETE;

            SrvPrepareTransStateAsync(pTransState, pExecContext);

            // TODO:
            //
            // The call must fail in the following events.
            // a. the pipe has any data remaining
            // b. the pipe is not in message mode
            ntStatus = IoWriteFile(
                            pTransState->pFile->hFile,
                            pTransState->pAcb,
                            &pTransState->ioStatusBlock,
                            pTransState->pData,
                            pTransState->pRequestHeader->dataCount,
                            &pTransState->llOffset,
                            NULL);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseTransStateAsync(pTransState); // completed synchronously

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_WRITE_COMPLETE:

            ntStatus = pTransState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_ATTEMPT_READ;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_ATTEMPT_READ:

            // TODO: Make sure we have enough space in the reply buffer for this
            ntStatus = SrvAllocateMemory(
                            pTransState->pRequestHeader->maxDataCount,
                            (PVOID*)&pTransState->pData2);
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_READ_COMPLETE;

            SrvPrepareTransStateAsync(pTransState, pExecContext);

            ntStatus = IoReadFile(
                            pTransState->pFile->hFile,
                            pTransState->pAcb,
                            &pTransState->ioStatusBlock,
                            pTransState->pData2,
                            pTransState->pRequestHeader->maxDataCount,
                            NULL,
                            NULL);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseTransStateAsync(pTransState); // completed synchronously

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_READ_COMPLETE:

            ntStatus = pTransState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->usBytesRead =
                            pTransState->ioStatusBlock.BytesTransferred;

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildTransactNamedPipeResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pTransState->stage = SRV_TRANS_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_TRANS_STAGE_SMB_V1_DONE:

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildTransactNamedPipeResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_TRANS_STATE_SMB_V1    pTransState  = NULL;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE  pOutBuffer        = pSmbResponse->pBuffer;
    ULONG  ulBytesAvailable  = pSmbResponse->ulBytesAvailable;
    ULONG  ulOffset          = 0;
    USHORT usBytesUsed       = 0;
    ULONG  ulTotalBytesUsed  = 0;
    USHORT usDataOffset      = 0;
    USHORT usParameterOffset = 0;

    pTransState = (PSRV_TRANS_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_TRANSACTION,
                        pTransState->usBytesRead < pTransState->pRequestHeader->maxDataCount ?
                             STATUS_SUCCESS : STATUS_BUFFER_OVERFLOW,
                        TRUE,
                        pTransState->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pTransState->pSession->uid,
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
                    pTransState->pData2,
                    pTransState->usBytesRead,
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

static
NTSTATUS
SrvBuildTransState(
    PTRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                     pBytecount,
    PWSTR                       pwszName,
    PUSHORT                     pSetup,
    PBYTE                       pParameters,
    PBYTE                       pData,
    PSRV_TRANS_STATE_SMB_V1*    ppTransState
    )
{
    NTSTATUS                ntStatus    = STATUS_SUCCESS;
    PSRV_TRANS_STATE_SMB_V1 pTransState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_TRANS_STATE_SMB_V1),
                    (PVOID*)&pTransState);
    BAIL_ON_NT_STATUS(ntStatus);

    pTransState->refCount = 1;

    pthread_mutex_init(&pTransState->mutex, NULL);
    pTransState->pMutex = &pTransState->mutex;

    pTransState->stage = SRV_TRANS_STAGE_SMB_V1_INITIAL;

    pTransState->pRequestHeader = pRequestHeader;
    pTransState->pBytecount     = pBytecount;
    pTransState->pwszName       = pwszName;
    pTransState->pSetup         = pSetup;
    pTransState->pParameters    = pParameters;
    pTransState->pData          = pData;

    *ppTransState = pTransState;

cleanup:

    return ntStatus;

error:

    *ppTransState = NULL;

    if (pTransState)
    {
        SrvFreeTransState(pTransState);
    }

    goto cleanup;
}

VOID
SrvPrepareTransStateAsync(
    PSRV_TRANS_STATE_SMB_V1 pTransState,
    PSRV_EXEC_CONTEXT       pExecContext
    )
{
    pTransState->acb.Callback        = &SrvExecuteTransAsyncCB;

    pTransState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pTransState->acb.AsyncCancelContext = NULL;

    pTransState->pAcb = &pTransState->acb;
}

static
VOID
SrvExecuteTransAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_TRANS_STATE_SMB_V1    pTransState      = NULL;
    BOOLEAN                    bInLock          = FALSE;

    pTransState =
        (PSRV_TRANS_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pTransState->mutex);

    if (pTransState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(&pTransState->pAcb->AsyncCancelContext);
    }

    pTransState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pTransState->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V1.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

VOID
SrvReleaseTransStateAsync(
    PSRV_TRANS_STATE_SMB_V1 pTransState
    )
{
    if (pTransState->pAcb)
    {
        pTransState->acb.Callback = NULL;

        if (pTransState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext = NULL;

            pExecContext = (PSRV_EXEC_CONTEXT)pTransState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pTransState->pAcb->CallbackContext = NULL;
        }

        if (pTransState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pTransState->pAcb->AsyncCancelContext);
        }

        pTransState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseTransStateHandle(
    HANDLE hTransState
    )
{
    SrvReleaseTransState((PSRV_TRANS_STATE_SMB_V1)hTransState);
}

static
VOID
SrvReleaseTransState(
    PSRV_TRANS_STATE_SMB_V1 pTransState
    )
{
    if (InterlockedDecrement(&pTransState->refCount) == 0)
    {
        SrvFreeTransState(pTransState);
    }
}

static
VOID
SrvFreeTransState(
    PSRV_TRANS_STATE_SMB_V1 pTransState
    )
{
    if (pTransState->pAcb && pTransState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pTransState->pAcb->AsyncCancelContext);
    }

    if (pTransState->pFile)
    {
        SrvFileRelease(pTransState->pFile);
    }

    if (pTransState->pTree)
    {
        SrvTreeRelease(pTransState->pTree);
    }

    if (pTransState->pSession)
    {
        SrvSessionRelease(pTransState->pSession);
    }

    if (pTransState->hFile)
    {
        IoCloseFile(pTransState->hFile);
    }

    if (pTransState->fileName.FileName)
    {
        SrvFreeMemory(pTransState->fileName.FileName);
    }

    if (pTransState->pData2)
    {
        SrvFreeMemory(pTransState->pData2);
    }

    if (pTransState->pMutex)
    {
        pthread_mutex_destroy(&pTransState->mutex);
    }

    SrvFreeMemory(pTransState);
}

