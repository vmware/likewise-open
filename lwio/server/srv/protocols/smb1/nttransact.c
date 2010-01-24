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
SrvQuerySecurityDescriptor(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteQuerySecurityDescriptor(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildQuerySecurityDescriptorResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSetSecurityDescriptor(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildSetSecurityDescriptorResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvProcessIOCTL(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteFsctl(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteIoctl(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildIOCTLResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvProcessNotifyChange(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvExecuteChangeNotify(
    PSRV_EXEC_CONTEXT               pExecContext,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    );

static
NTSTATUS
SrvProcessChangeNotifyCompletion(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildChangeNotifyResponse(
    PSRV_EXEC_CONTEXT               pExecContext,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    );

static
NTSTATUS
SrvMarshalChangeNotifyResponse(
    PBYTE  pNotifyResponse,
    ULONG  ulNotifyResponseLength,
    PBYTE* ppBuffer,
    PULONG pulParameterLength
    );

static
NTSTATUS
SrvProcessNtTransactCreate(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvParseNtTransactCreateParameters(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvQueryNTTransactFileInformation(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvRequestNTTransactOplocks(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvBuildNTTransactState(
    PNT_TRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                        pusBytecount,
    PUSHORT                        pSetup,
    PBYTE                          pParameters,
    PBYTE                          pData,
    PSRV_NTTRANSACT_STATE_SMB_V1*  ppNTTransactState
    );

static
NTSTATUS
SrvBuildNTTransactCreateResponse(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
VOID
SrvPrepareNTTransactStateAsync(
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState,
    PSRV_EXEC_CONTEXT            pExecContext
    );

static
VOID
SrvExecuteNTTransactAsyncCB(
    PVOID pContext
    );

VOID
SrvReleaseNTTransactStateAsync(
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState
    );

static
VOID
SrvReleaseNTTransactStateHandle(
    HANDLE hNTTransactState
    );

static
VOID
SrvReleaseNTTransactState(
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState
    );

static
VOID
SrvFreeNTTransactState(
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState
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
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;
    BOOLEAN                      bInLock          = FALSE;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;
    if (pNTTransactState)
    {
        InterlockedIncrement(&pNTTransactState->refCount);
    }
    else
    {
        PBYTE pBuffer          = pSmbRequest->pBuffer + pSmbRequest->usHeaderSize;
        ULONG ulOffset         = pSmbRequest->usHeaderSize;
        ULONG ulBytesAvailable = pSmbRequest->ulMessageSize - pSmbRequest->usHeaderSize;
        PNT_TRANSACTION_REQUEST_HEADER pRequestHeader = NULL; // Do not free
        PUSHORT                        pusBytecount   = NULL; // Do not free
        PUSHORT                        pSetup         = NULL; // Do not free
        PBYTE                          pParameters    = NULL; // Do not free
        PBYTE                          pData          = NULL; // Do not free

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

        ntStatus = SrvBuildNTTransactState(
                        pRequestHeader,
                        pusBytecount,
                        pSetup,
                        pParameters,
                        pData,
                        &pNTTransactState);
        BAIL_ON_NT_STATUS(ntStatus);

        pCtxSmb1->hState = pNTTransactState;
        InterlockedIncrement(&pNTTransactState->refCount);
        pCtxSmb1->pfnStateRelease = &SrvReleaseNTTransactStateHandle;

        ntStatus = SrvConnectionFindSession_SMB_V1(
                        pCtxSmb1,
                        pConnection,
                        pSmbRequest->pHeader->uid,
                        &pNTTransactState->pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvSessionFindTree_SMB_V1(
                        pCtxSmb1,
                        pNTTransactState->pSession,
                        pSmbRequest->pHeader->tid,
                        &pNTTransactState->pTree);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_MUTEX(bInLock, &pNTTransactState->mutex);

    switch (pNTTransactState->pRequestHeader->usFunction)
    {
        case SMB_SUB_COMMAND_NT_TRANSACT_QUERY_SECURITY_DESC:

            ntStatus = SrvQuerySecurityDescriptor(pExecContext);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_SET_SECURITY_DESC :

            ntStatus = SrvSetSecurityDescriptor(pExecContext);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_IOCTL :

            ntStatus = SrvProcessIOCTL(pExecContext);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_NOTIFY_CHANGE :

            ntStatus = SrvProcessNotifyChange(pExecContext);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_CREATE :

            ntStatus = SrvProcessNtTransactCreate(pExecContext);

            break;

        case SMB_SUB_COMMAND_NT_TRANSACT_RENAME :

            ntStatus = STATUS_NOT_SUPPORTED;

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pNTTransactState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pNTTransactState->mutex);

        SrvReleaseNTTransactState(pNTTransactState);
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

            if (pNTTransactState)
            {
                SrvReleaseNTTransactStateAsync(pNTTransactState);
            }

            break;
    }

    goto cleanup;
}

NTSTATUS
SrvProcessNtTransactInternal(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = 0;
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
        case SMB_SUB_COMMAND_NT_TRANSACT_NOTIFY_CHANGE :

            ntStatus = SrvProcessChangeNotifyCompletion(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            break;

        default:

            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvQuerySecurityDescriptor(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pNTTransactState->stage)
    {
        case SRV_NTTRANSACT_STAGE_SMB_V1_INITIAL:

            if (pNTTransactState->pRequestHeader->ulTotalParameterCount !=
                                sizeof(SMB_SECURITY_INFORMATION_HEADER))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pNTTransactState->pSecurityRequestHeader =
                (PSMB_SECURITY_INFORMATION_HEADER)pNTTransactState->pParameters;

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pNTTransactState->pTree,
                            pNTTransactState->pSecurityRequestHeader->usFid,
                            &pNTTransactState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO:

            ntStatus = SrvExecuteQuerySecurityDescriptor(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildQuerySecurityDescriptorResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_DONE:

            break;

        case SRV_NTTRANSACT_STAGE_SMB_V1_CREATE_COMPLETED:
        case SRV_NTTRANSACT_STAGE_SMB_V1_QUERY_INFO:
        case SRV_NTTRANSACT_STAGE_SMB_V1_GET_OPLOCKS:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvExecuteQuerySecurityDescriptor(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    BOOLEAN                    bContinue    = TRUE;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    do
    {
        ntStatus = pNTTransactState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                if (!pNTTransactState->pSecurityDescriptor2)
                {
                    ULONG ulSecurityDescInitialLen = 256;

                    ntStatus = SrvAllocateMemory(
                                    ulSecurityDescInitialLen,
                                    (PVOID*)&pNTTransactState->pSecurityDescriptor2);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pNTTransactState->ulSecurityDescAllocLen =
                                                    ulSecurityDescInitialLen;
                }
                else if (pNTTransactState->ulSecurityDescAllocLen !=
                                    SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE)
                {
                    PBYTE pNewMemory = NULL;
                    ULONG ulNewLen =
                        LW_MIN( SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE,
                                pNTTransactState->ulSecurityDescAllocLen + 4096);

                    ntStatus = SrvAllocateMemory(ulNewLen, (PVOID*)&pNewMemory);
                    BAIL_ON_NT_STATUS(ntStatus);

                    if (pNTTransactState->pSecurityDescriptor2)
                    {
                        SrvFreeMemory(pNTTransactState->pSecurityDescriptor2);
                    }

                    pNTTransactState->pSecurityDescriptor2 = pNewMemory;
                    pNTTransactState->ulSecurityDescAllocLen = ulNewLen;
                }
                else
                {
                    ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                }
                BAIL_ON_NT_STATUS(ntStatus);

                SrvPrepareNTTransactStateAsync(pNTTransactState, pExecContext);

                ntStatus = IoQuerySecurityFile(
                                pNTTransactState->pFile->hFile,
                                pNTTransactState->pAcb,
                                &pNTTransactState->ioStatusBlock,
                                pNTTransactState->pSecurityRequestHeader->ulSecurityInfo,
                                (PSECURITY_DESCRIPTOR_RELATIVE)pNTTransactState->pSecurityDescriptor2,
                                pNTTransactState->ulSecurityDescAllocLen);
                switch (ntStatus)
                {
                    case STATUS_SUCCESS:
                    case STATUS_BUFFER_TOO_SMALL:

                        // completed synchronously
                        SrvReleaseNTTransactStateAsync(pNTTransactState);

                        break;

                    default:

                        BAIL_ON_NT_STATUS(ntStatus);
                }

                break;

            case STATUS_SUCCESS:

                if (!pNTTransactState->pSecurityDescriptor2)
                {
                    pNTTransactState->ioStatusBlock.Status =
                                                    STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    pNTTransactState->ulSecurityDescActualLen =
                            pNTTransactState->ioStatusBlock.BytesTransferred;

                    bContinue = FALSE;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildQuerySecurityDescriptorResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE   pOutBuffer         = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable   = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset           = 0;
    ULONG   ulPackageBytesUsed = 0;
    ULONG   ulTotalBytesUsed   = 0;
    PUSHORT pSetup             = NULL;
    UCHAR   ucSetupCount       = 0;
    ULONG   ulDataOffset       = 0;
    ULONG   ulParameterOffset  = 0;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_NT_TRANSACT,
                        (pNTTransactState->ulSecurityDescActualLen <= pNTTransactState->pRequestHeader->ulMaxDataCount ?
                                STATUS_SUCCESS : STATUS_BUFFER_TOO_SMALL),
                        TRUE,
                        pNTTransactState->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pNTTransactState->pSession->uid,
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
                        COM_NT_TRANSACT,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 18 + ucSetupCount;

    ntStatus = WireMarshallNtTransactionResponse(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    ucSetupCount,
                    (PBYTE)&pNTTransactState->ulSecurityDescActualLen,
                    sizeof(pNTTransactState->ulSecurityDescActualLen),
                    (pNTTransactState->ulSecurityDescActualLen <= pNTTransactState->pRequestHeader->ulMaxDataCount ? pNTTransactState->pSecurityDescriptor2 : NULL),
                    (pNTTransactState->ulSecurityDescActualLen <= pNTTransactState->pRequestHeader->ulMaxDataCount ? pNTTransactState->ulSecurityDescActualLen : 0),
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
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_NTTRANSACT_STATE_SMB_V1     pNTTransactState    = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pNTTransactState->stage)
    {
        case SRV_NTTRANSACT_STAGE_SMB_V1_INITIAL:

            if (pNTTransactState->pRequestHeader->ulTotalParameterCount !=
                                    sizeof(SMB_SECURITY_INFORMATION_HEADER))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pNTTransactState->pSecurityRequestHeader =
                (PSMB_SECURITY_INFORMATION_HEADER)pNTTransactState->pParameters;

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pNTTransactState->pTree,
                            pNTTransactState->pSecurityRequestHeader->usFid,
                            &pNTTransactState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO:

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE;

            SrvPrepareNTTransactStateAsync(pNTTransactState, pExecContext);

            ntStatus = IoSetSecurityFile(
                            pNTTransactState->pFile->hFile,
                            pNTTransactState->pAcb,
                            &pNTTransactState->ioStatusBlock,
                            pNTTransactState->pSecurityRequestHeader->ulSecurityInfo,
                            (PSECURITY_DESCRIPTOR_RELATIVE)pNTTransactState->pData,
                            pNTTransactState->pRequestHeader->ulTotalDataCount);
            BAIL_ON_NT_STATUS(ntStatus);

            // completed synchronously
            SrvReleaseNTTransactStateAsync(pNTTransactState);

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = pNTTransactState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvBuildSetSecurityDescriptorResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_DONE:

            break;

        case SRV_NTTRANSACT_STAGE_SMB_V1_CREATE_COMPLETED:
        case SRV_NTTRANSACT_STAGE_SMB_V1_QUERY_INFO:
        case SRV_NTTRANSACT_STAGE_SMB_V1_GET_OPLOCKS:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildSetSecurityDescriptorResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE   pOutBuffer         = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable   = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset           = 0;
    ULONG   ulPackageBytesUsed = 0;
    ULONG   ulTotalBytesUsed   = 0;
    PUSHORT pSetup             = NULL;
    UCHAR   ucSetupCount       = 0;
    ULONG   ulDataOffset       = 0;
    ULONG   ulParameterOffset  = 0;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_NT_TRANSACT,
                        STATUS_SUCCESS,
                        TRUE,
                        pNTTransactState->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pNTTransactState->pSession->uid,
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
                        COM_NT_TRANSACT,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 18 + ucSetupCount;

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
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_NTTRANSACT_STATE_SMB_V1     pNTTransactState    = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pNTTransactState->stage)
    {
        case SRV_NTTRANSACT_STAGE_SMB_V1_INITIAL:

            if (pNTTransactState->pSetup)
            {
                if (sizeof(SMB_IOCTL_HEADER) !=
                        pNTTransactState->pRequestHeader->ucSetupCount * sizeof(USHORT))
                {
                    ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                pNTTransactState->pIoctlRequest =
                        (PSMB_IOCTL_HEADER)((PBYTE)pNTTransactState->pSetup);

            }
            else if (pNTTransactState->pRequestHeader->ulTotalParameterCount ==
                                                    sizeof(SMB_IOCTL_HEADER))
            {
                pNTTransactState->pIoctlRequest =
                        (PSMB_IOCTL_HEADER)pNTTransactState->pParameters;
            }
            else
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            if (pNTTransactState->pIoctlRequest->ucFlags & 0x1)
            {
                // TODO: Apply only to DFS Share
                //       We don't support DFS yet
                ntStatus = STATUS_NOT_SUPPORTED;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pNTTransactState->pTree,
                            pNTTransactState->pIoctlRequest->usFid,
                            &pNTTransactState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO:

            if (pNTTransactState->pIoctlRequest->bIsFsctl)
            {
                ntStatus = SrvExecuteFsctl(pExecContext);
            }
            else
            {
                ntStatus = SrvExecuteIoctl(pExecContext);
            }
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildIOCTLResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_DONE:

            break;

        case SRV_NTTRANSACT_STAGE_SMB_V1_CREATE_COMPLETED:
        case SRV_NTTRANSACT_STAGE_SMB_V1_QUERY_INFO:
        case SRV_NTTRANSACT_STAGE_SMB_V1_GET_OPLOCKS:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvExecuteFsctl(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    BOOLEAN                    bContinue    = TRUE;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    do
    {
        ntStatus = pNTTransactState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                if (!pNTTransactState->pResponseBuffer)
                {
                    USHORT usInitialLength = 512;

                    ntStatus = SrvAllocateMemory(
                                    usInitialLength,
                                    (PVOID*)&pNTTransactState->pResponseBuffer);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pNTTransactState->usResponseBufferLen = usInitialLength;
                }
                else
                {
                    USHORT usNewLength = 0;

                    if ((pNTTransactState->usResponseBufferLen + 256) > UINT16_MAX)
                    {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                        BAIL_ON_NT_STATUS(ntStatus);
                    }

                    usNewLength = pNTTransactState->usResponseBufferLen + 256;

                    if (pNTTransactState->pResponseBuffer)
                    {
                        SrvFreeMemory(pNTTransactState->pResponseBuffer);
                        pNTTransactState->pResponseBuffer = NULL;
                        pNTTransactState->usResponseBufferLen = 0;
                    }

                    ntStatus = SrvAllocateMemory(
                                    usNewLength,
                                    (PVOID*)&pNTTransactState->pResponseBuffer);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pNTTransactState->usResponseBufferLen = usNewLength;
                }

                SrvPrepareNTTransactStateAsync(pNTTransactState, pExecContext);

                ntStatus = IoFsControlFile(
                                pNTTransactState->pFile->hFile,
                                pNTTransactState->pAcb,
                                &pNTTransactState->ioStatusBlock,
                                pNTTransactState->pIoctlRequest->ulFunctionCode,
                                pNTTransactState->pData,
                                pNTTransactState->pRequestHeader->ulTotalDataCount,
                                pNTTransactState->pResponseBuffer,
                                pNTTransactState->usResponseBufferLen);
                switch (ntStatus)
                {
                    case STATUS_SUCCESS:
                    case STATUS_BUFFER_TOO_SMALL:

                        // completed synchronously
                        SrvReleaseNTTransactStateAsync(pNTTransactState);

                        break;

                    default:

                        BAIL_ON_NT_STATUS(ntStatus);

                        break;
                }

                break;

            case STATUS_SUCCESS:

                if (!pNTTransactState->pResponseBuffer)
                {
                    pNTTransactState->ioStatusBlock.Status = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    pNTTransactState->usActualResponseLen =
                            pNTTransactState->ioStatusBlock.BytesTransferred;
                    bContinue = FALSE;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvExecuteIoctl(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    BOOLEAN                    bContinue    = TRUE;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    do
    {
        ntStatus = pNTTransactState->ioStatusBlock.Status;

        switch (ntStatus)
        {
            case STATUS_BUFFER_TOO_SMALL:

                if (!pNTTransactState->pResponseBuffer)
                {
                    USHORT usInitialLength = 512;

                    ntStatus = SrvAllocateMemory(
                                    usInitialLength,
                                    (PVOID*)&pNTTransactState->pResponseBuffer);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pNTTransactState->usResponseBufferLen = usInitialLength;
                }
                else
                {
                    USHORT usNewLength = 0;

                    if ((pNTTransactState->usResponseBufferLen + 256) > UINT16_MAX)
                    {
                        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                        BAIL_ON_NT_STATUS(ntStatus);
                    }

                    usNewLength = pNTTransactState->usResponseBufferLen + 256;

                    if (pNTTransactState->pResponseBuffer)
                    {
                        SrvFreeMemory(pNTTransactState->pResponseBuffer);
                        pNTTransactState->pResponseBuffer = NULL;
                        pNTTransactState->usResponseBufferLen = 0;
                    }

                    ntStatus = SrvAllocateMemory(
                                    usNewLength,
                                    (PVOID*)&pNTTransactState->pResponseBuffer);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pNTTransactState->usResponseBufferLen = usNewLength;
                }

                SrvPrepareNTTransactStateAsync(pNTTransactState, pExecContext);

                ntStatus = IoFsControlFile(
                                pNTTransactState->pFile->hFile,
                                pNTTransactState->pAcb,
                                &pNTTransactState->ioStatusBlock,
                                pNTTransactState->pIoctlRequest->ulFunctionCode,
                                pNTTransactState->pData,
                                pNTTransactState->pRequestHeader->ulTotalDataCount,
                                pNTTransactState->pResponseBuffer,
                                pNTTransactState->usResponseBufferLen);
                switch (ntStatus)
                {
                    case STATUS_SUCCESS:
                    case STATUS_BUFFER_TOO_SMALL:

                        // completed synchronously
                        SrvReleaseNTTransactStateAsync(pNTTransactState);

                        break;

                    default:

                        BAIL_ON_NT_STATUS(ntStatus);

                        break;
                }

                break;

            case STATUS_SUCCESS:

                if (!pNTTransactState->pResponseBuffer)
                {
                    pNTTransactState->ioStatusBlock.Status = STATUS_BUFFER_TOO_SMALL;
                }
                else
                {
                    pNTTransactState->usActualResponseLen =
                            pNTTransactState->ioStatusBlock.BytesTransferred;

                    bContinue = FALSE;
                }

                break;

            default:

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }

    } while (bContinue);

error:

    return ntStatus;
}

static
NTSTATUS
SrvBuildIOCTLResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION         pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1     pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState    = NULL;
    ULONG                        iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1          pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1          pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG ulPackageBytesUsed   = 0;
    ULONG ulTotalBytesUsed     = 0;
    UCHAR ucResponseSetupCount = 1;
    ULONG ulDataOffset         = 0;
    ULONG ulParameterOffset    = 0;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_NT_TRANSACT,
                        STATUS_SUCCESS,
                        TRUE,
                        pNTTransactState->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pNTTransactState->pSession->uid,
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
                        COM_NT_TRANSACT,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 18 + ucResponseSetupCount;

    ntStatus = WireMarshallNtTransactionResponse(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pNTTransactState->usActualResponseLen,
                    ucResponseSetupCount,
                    NULL,
                    0,
                    pNTTransactState->pResponseBuffer,
                    pNTTransactState->usActualResponseLen,
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
SrvProcessNotifyChange(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1     pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                        iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1          pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_NTTRANSACT_STATE_SMB_V1      pNTTransactState  = NULL;
    PLWIO_ASYNC_STATE                 pAsyncState       = NULL;
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1   pNotifyState      = NULL;
    BOOLEAN                           bUnregisterAsync  = FALSE;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pNTTransactState->stage)
    {
        case SRV_NTTRANSACT_STAGE_SMB_V1_INITIAL:

            if (!pNTTransactState->pRequestHeader->ulMaxParameterCount ||
                (sizeof(SMB_NOTIFY_CHANGE_HEADER) !=
                    pNTTransactState->pRequestHeader->ucSetupCount * sizeof(USHORT)))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pNTTransactState->pNotifyChangeHeader =
                (PSMB_NOTIFY_CHANGE_HEADER)(PBYTE)pNTTransactState->pSetup;

            ntStatus = SrvTreeFindFile_SMB_V1(
                            pCtxSmb1,
                            pNTTransactState->pTree,
                            pNTTransactState->pNotifyChangeHeader->usFid,
                            &pNTTransactState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvNotifyCreateState(
                            pExecContext->pConnection,
                            pCtxSmb1->pSession,
                            pCtxSmb1->pTree,
                            pCtxSmb1->pFile,
                            pSmbRequest->pHeader->mid,
                            SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                            pExecContext->pSmbRequest->sequence,
                            pNTTransactState->pNotifyChangeHeader->ulCompletionFilter,
                            pNTTransactState->pNotifyChangeHeader->bWatchTree,
                            pNTTransactState->pRequestHeader->ulMaxParameterCount,
                            &pNotifyState);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvAsyncStateCreate(
                            pNotifyState->ullNotifyId,
                            SMB_SUB_COMMAND_NT_TRANSACT_NOTIFY_CHANGE,
                            pNotifyState,
                            &SrvNotifyStateReleaseHandle,
                            &pAsyncState);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvNotifyStateAcquire(pNotifyState);

            ntStatus = SrvTreeAddAsyncState(pCtxSmb1->pTree, pAsyncState);
            BAIL_ON_NT_STATUS(ntStatus);

            bUnregisterAsync = TRUE;

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO:

            ntStatus = SrvExecuteChangeNotify(pExecContext, pNotifyState);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = SrvBuildChangeNotifyResponse(pExecContext, pNotifyState);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_DONE:

            break;

        case SRV_NTTRANSACT_STAGE_SMB_V1_CREATE_COMPLETED:
        case SRV_NTTRANSACT_STAGE_SMB_V1_QUERY_INFO:
        case SRV_NTTRANSACT_STAGE_SMB_V1_GET_OPLOCKS:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }

cleanup:

    if (pNotifyState)
    {
        if (bUnregisterAsync)
        {
            SrvTreeRemoveAsyncState(pCtxSmb1->pTree, pNotifyState->ullNotifyId);
        }

        SrvNotifyStateRelease(pNotifyState);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
    }

    return ntStatus;

error:

    if (ntStatus == STATUS_PENDING)
    {
        bUnregisterAsync = FALSE;
    }

    goto cleanup;
}

static
NTSTATUS
SrvExecuteChangeNotify(
    PSRV_EXEC_CONTEXT               pExecContext,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;

    SrvPrepareNotifyStateAsync(pNotifyState);

    ntStatus = IoReadDirectoryChangeFile(
                    pCtxSmb1->pFile->hFile,
                    pNotifyState->pAcb,
                    &pNotifyState->ioStatusBlock,
                    pNotifyState->pBuffer,
                    pNotifyState->ulBufferLength,
                    pNotifyState->bWatchTree,
                    pNotifyState->ulCompletionFilter,
                    &pNotifyState->ulMaxBufferSize);

    if (ntStatus == STATUS_NOT_SUPPORTED)
    {
        //
        // The file system driver does not have support
        // to keep accumulating file change notifications
        //
        ntStatus = IoReadDirectoryChangeFile(
                        pCtxSmb1->pFile->hFile,
                        pNotifyState->pAcb,
                        &pNotifyState->ioStatusBlock,
                        pNotifyState->pBuffer,
                        pNotifyState->ulBufferLength,
                        pNotifyState->bWatchTree,
                        pNotifyState->ulCompletionFilter,
                        NULL);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    SrvReleaseNotifyStateAsync(pNotifyState); // Completed synchronously

    pNotifyState->ulBytesUsed = pNotifyState->ioStatusBlock.BytesTransferred;

error:

    return ntStatus;
}

static
NTSTATUS
SrvProcessChangeNotifyCompletion(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PLWIO_SRV_SESSION          pSession     = NULL;
    PLWIO_SRV_TREE             pTree        = NULL;
    BOOLEAN                    bInLock      = FALSE;
    PLWIO_ASYNC_STATE          pAsyncState  = NULL;
    ULONG64                    ullNotifyId  = 0LL;
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState = NULL;

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

    ullNotifyId = SrvNotifyGetId(
                    SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                    pSmbRequest->pHeader->mid);

    ntStatus = SrvTreeFindAsyncState(pTree, ullNotifyId, &pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeRemoveAsyncState(pTree, ullNotifyId);
    BAIL_ON_NT_STATUS(ntStatus);

    pNotifyState = (PSRV_CHANGE_NOTIFY_STATE_SMB_V1)pAsyncState->hAsyncState;

    LWIO_LOCK_MUTEX(bInLock, &pNotifyState->mutex);

    switch (pSmbRequest->pHeader->error)
    {
        case STATUS_CANCELLED:

            ntStatus = SrvBuildErrorResponse_SMB_V1(
                            pConnection,
                            pSmbRequest->pHeader,
                            pSmbRequest->pHeader->error,
                            pSmbResponse);

            break;

        case STATUS_NOTIFY_ENUM_DIR:
        case STATUS_SUCCESS:

            pNotifyState->ulBytesUsed =
                                pNotifyState->ioStatusBlock.BytesTransferred;
            ntStatus = SrvBuildChangeNotifyResponse(pExecContext, pNotifyState);

            break;

        default:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pNotifyState)
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pNotifyState->mutex);
    }

    if (pAsyncState)
    {
        SrvAsyncStateRelease(pAsyncState);
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
SrvBuildChangeNotifyResponse(
    PSRV_EXEC_CONTEXT               pExecContext,
    PSRV_CHANGE_NOTIFY_STATE_SMB_V1 pNotifyState
    )
{
    NTSTATUS                     ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION         pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1     pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState    = NULL;
    ULONG                        iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1          pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PUSHORT                      pSetup       = NULL;
    PBYTE                        pParams      = NULL;
    PBYTE                        pData        = NULL;
    PBYTE pOutBuffer           = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset             = 0;
    ULONG ulPackageBytesUsed   = 0;
    ULONG ulTotalBytesUsed     = 0;
    UCHAR ucResponseSetupCount = 0;
    ULONG ulDataLength         = 0;
    ULONG ulDataOffset         = 0;
    ULONG ulParameterOffset    = 0;
    ULONG ulParameterLength    = 0;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_NT_TRANSACT,
                        STATUS_SUCCESS,
                        TRUE,
                        pNotifyState->usTid,
                        pNotifyState->ulPid,
                        pNotifyState->usUid,
                        pNotifyState->usMid,
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
                        COM_NT_TRANSACT,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 18 + ucResponseSetupCount;

    if ((pNotifyState->ioStatusBlock.Status == STATUS_SUCCESS) &&
        pNotifyState->ulBytesUsed > 0)
    {
        ntStatus = SrvMarshalChangeNotifyResponse(
                        pNotifyState->pBuffer,
                        pNotifyState->ulBytesUsed,
                        &pParams,
                        &ulParameterLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = WireMarshallNtTransactionResponse(
                    pOutBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    pSetup,
                    ucResponseSetupCount,
                    pParams,
                    ulParameterLength,
                    pData,
                    ulDataLength,
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

    if (pParams)
    {
        SrvFreeMemory(pParams);
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
SrvMarshalChangeNotifyResponse(
    PBYTE  pNotifyResponse,
    ULONG  ulNotifyResponseLength,
    PBYTE* ppBuffer,
    PULONG pulBufferLength
    )
{
    NTSTATUS ntStatus         = STATUS_SUCCESS;
    PBYTE    pBuffer          = NULL;
    ULONG    ulBufferLength   = 0;
    ULONG    ulOffset         = 0;
    ULONG    ulNumRecords     = 0;
    ULONG    iRecord          = 0;
    ULONG    ulBytesAvailable = ulNotifyResponseLength;
    PBYTE    pDataCursor      = NULL;
    PFILE_NOTIFY_INFORMATION pNotifyCursor = NULL;
    PFILE_NOTIFY_INFORMATION pPrevHeader   = NULL;
    PFILE_NOTIFY_INFORMATION pCurHeader    = NULL;

    pNotifyCursor = (PFILE_NOTIFY_INFORMATION)pNotifyResponse;

    while (pNotifyCursor && (ulBufferLength < ulBytesAvailable))
    {
        ulBufferLength += offsetof(FILE_NOTIFY_INFORMATION, FileName);

        if (!pNotifyCursor->FileNameLength)
        {
            ulBufferLength += sizeof(wchar16_t);
        }
        else
        {
            ulBufferLength += pNotifyCursor->FileNameLength;
        }

        ulNumRecords++;

        if (pNotifyCursor->NextEntryOffset)
        {
            if (ulBufferLength % 4)
            {
                USHORT usAlignment = (4 - (ulBufferLength % 4));

                ulBufferLength += usAlignment;
            }

            pNotifyCursor =
                (PFILE_NOTIFY_INFORMATION)(((PBYTE)pNotifyCursor) +
                                            pNotifyCursor->NextEntryOffset);
        }
        else
        {
            pNotifyCursor = NULL;
        }
    }

    if (ulBufferLength)
    {
        ntStatus = SrvAllocateMemory(ulBufferLength, (PVOID*)&pBuffer);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNotifyCursor = (PFILE_NOTIFY_INFORMATION)pNotifyResponse;
    pDataCursor   = pBuffer;

    for (; iRecord < ulNumRecords; iRecord++)
    {
        pPrevHeader = pCurHeader;
        pCurHeader = (PFILE_NOTIFY_INFORMATION)pDataCursor;

        /* Update next entry offset for previous entry. */
        if (pPrevHeader != NULL)
        {
            pPrevHeader->NextEntryOffset = ulOffset;
        }

        ulOffset = 0;

        pCurHeader->NextEntryOffset = 0;
        pCurHeader->Action = pNotifyCursor->Action;
        pCurHeader->FileNameLength = pNotifyCursor->FileNameLength;

        pDataCursor += offsetof(FILE_NOTIFY_INFORMATION, FileName);
        ulOffset    += offsetof(FILE_NOTIFY_INFORMATION, FileName);

        if (pNotifyCursor->FileNameLength)
        {
            memcpy( pDataCursor,
                    (PBYTE)pNotifyCursor->FileName,
                    pNotifyCursor->FileNameLength);

            pDataCursor += pNotifyCursor->FileNameLength;
            ulOffset    += pNotifyCursor->FileNameLength;
        }
        else
        {
            pDataCursor += sizeof(wchar16_t);
            ulOffset    += sizeof(wchar16_t);
        }

        if (pNotifyCursor->NextEntryOffset != 0)
        {
            if (ulOffset % 4)
            {
                USHORT usAlign = 4 - (ulOffset % 4);

                pDataCursor += usAlign;
                ulOffset    += usAlign;
            }
        }

        pNotifyCursor =
                    (PFILE_NOTIFY_INFORMATION)(((PBYTE)pNotifyCursor) +
                                    pNotifyCursor->NextEntryOffset);
    }

    *ppBuffer        = pBuffer;
    *pulBufferLength = ulBufferLength;

cleanup:

    return ntStatus;

error:

    *ppBuffer        = NULL;
    *pulBufferLength = 0;

    if (pBuffer)
    {
        SrvFreeMemory(pBuffer);
    }

    goto cleanup;
}

static
NTSTATUS
SrvProcessNtTransactCreate(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1     pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    switch (pNTTransactState->stage)
    {
        case SRV_NTTRANSACT_STAGE_SMB_V1_INITIAL:

            ntStatus = SrvParseNtTransactCreateParameters(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage =
                                SRV_NTTRANSACT_STAGE_SMB_V1_CREATE_COMPLETED;

            SrvPrepareNTTransactStateAsync(pNTTransactState, pExecContext);

            ntStatus = IoCreateFile(
                            &pNTTransactState->hFile,
                            pNTTransactState->pAcb,
                            &pNTTransactState->ioStatusBlock,
                            pNTTransactState->pSession->pIoSecurityContext,
                            pNTTransactState->pFilename,
                            (PSECURITY_DESCRIPTOR_RELATIVE)pNTTransactState->pSecDesc,
                            pNTTransactState->pSecurityQOS,
                            pNTTransactState->pNtTransactCreateHeader->ulDesiredAccess,
                            pNTTransactState->pNtTransactCreateHeader->ullAllocationSize,
                            pNTTransactState->pNtTransactCreateHeader->ulExtFileAttributes,
                            pNTTransactState->pNtTransactCreateHeader->ulShareAccess,
                            pNTTransactState->pNtTransactCreateHeader->ulCreateDisposition,
                            pNTTransactState->pNtTransactCreateHeader->ulCreateOptions,
                            pNTTransactState->pEA,
                            pNTTransactState->pNtTransactCreateHeader->ulEALength,
                            pNTTransactState->pEcpList);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseNTTransactStateAsync(pNTTransactState); // completed sync

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_CREATE_COMPLETED:

            ntStatus = pNTTransactState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->ulCreateAction =
                            pNTTransactState->ioStatusBlock.CreateResult;

            ntStatus = SrvTreeCreateFile(
                            pNTTransactState->pTree,
                            pNTTransactState->pwszFilename,
                            &pNTTransactState->hFile,
                            &pNTTransactState->pFilename,
                            pNTTransactState->pNtTransactCreateHeader->ulDesiredAccess,
                            pNTTransactState->pNtTransactCreateHeader->ullAllocationSize,
                            pNTTransactState->pNtTransactCreateHeader->ulExtFileAttributes,
                            pNTTransactState->pNtTransactCreateHeader->ulShareAccess,
                            pNTTransactState->pNtTransactCreateHeader->ulCreateDisposition,
                            pNTTransactState->pNtTransactCreateHeader->ulCreateOptions,
                            &pNTTransactState->pFile);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->bRemoveFileFromTree = TRUE;

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_QUERY_INFO;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_QUERY_INFO:

            ntStatus = SrvQueryNTTransactFileInformation(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_GET_OPLOCKS;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_GET_OPLOCKS:

            ntStatus = SrvRequestNTTransactOplocks(pExecContext);
            // We don't fail to if the oplock cannot be granted

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_BUILD_RESPONSE:

            ntStatus = pNTTransactState->ioStatusBlock.Status;
            BAIL_ON_NT_STATUS(ntStatus);

            if (!pNTTransactState->pFile->hByteRangeLockState)
            {
                PSRV_PENDING_LOCK_STATE_LIST pPendingLockStateList = NULL;

                ntStatus = SrvCreatePendingLockStateList(&pPendingLockStateList);
                BAIL_ON_NT_STATUS(ntStatus);

                pNTTransactState->pFile->hByteRangeLockState =
                                (HANDLE)pPendingLockStateList;

                pNTTransactState->pFile->pfnFreeByteRangeLockState =
                                &SrvFreePendingLockStateListHandle;
            }

            ntStatus = SrvBuildNTTransactCreateResponse(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_DONE;

            // intentional fall through

        case SRV_NTTRANSACT_STAGE_SMB_V1_DONE:

            pNTTransactState->bRemoveFileFromTree = FALSE;

            pCtxSmb1->pFile = SrvFileAcquire(pNTTransactState->pFile);

            break;

        case SRV_NTTRANSACT_STAGE_SMB_V1_ATTEMPT_IO:

            ntStatus = STATUS_INTERNAL_ERROR;

            break;
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvParseNtTransactCreateParameters(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION         pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1     pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;
    PBYTE                        pDataCursor      = NULL;
    ULONG                        ulOffset         = 0;
    ULONG                        ulBytesAvailable = 0;
    PWSTR                        pwszFilename     = NULL;
    BOOLEAN                      bTreeInLock      = FALSE;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    pDataCursor      = pNTTransactState->pParameters;
    ulOffset         = pNTTransactState->pRequestHeader->ulParameterOffset;
    ulBytesAvailable = pNTTransactState->pRequestHeader->ulParameterCount;

    if (ulBytesAvailable < sizeof(SMB_NT_TRANSACT_CREATE_REQUEST_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNTTransactState->pNtTransactCreateHeader =
                            (PSMB_NT_TRANSACT_CREATE_REQUEST_HEADER)pDataCursor;

    pDataCursor      += sizeof(SMB_NT_TRANSACT_CREATE_REQUEST_HEADER);
    ulOffset         += sizeof(SMB_NT_TRANSACT_CREATE_REQUEST_HEADER);
    ulBytesAvailable -= sizeof(SMB_NT_TRANSACT_CREATE_REQUEST_HEADER);

    if (!pNTTransactState->pNtTransactCreateHeader->ulNameLength)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        if (ulOffset % 2)
        {
            if (ulBytesAvailable < 1)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pDataCursor++;
            ulOffset++;
            ulBytesAvailable--;
        }

        if (ulBytesAvailable <
                        pNTTransactState->pNtTransactCreateHeader->ulNameLength)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pNTTransactState->pwszFilename = (PWSTR)pDataCursor;

        pDataCursor      += pNTTransactState->pNtTransactCreateHeader->ulNameLength;
        ulOffset         += pNTTransactState->pNtTransactCreateHeader->ulNameLength;
        ulBytesAvailable -= pNTTransactState->pNtTransactCreateHeader->ulNameLength;

        ntStatus = SrvAllocateMemory(
                        pNTTransactState->pNtTransactCreateHeader->ulNameLength + sizeof(wchar16_t),
                        (PVOID*)&pwszFilename);
        BAIL_ON_NT_STATUS(ntStatus);

        memcpy( (PBYTE)pwszFilename,
                (PBYTE)pNTTransactState->pwszFilename,
                pNTTransactState->pNtTransactCreateHeader->ulNameLength);
    }

    pDataCursor      = pNTTransactState->pData;
    ulOffset         = pNTTransactState->pRequestHeader->ulDataOffset;
    ulBytesAvailable = pNTTransactState->pRequestHeader->ulDataCount;

    if (pNTTransactState->pNtTransactCreateHeader->ulSecurityDescLength)
    {
        SECURITY_INFORMATION secInfoAll = DACL_SECURITY_INFORMATION;

        if (ulBytesAvailable <
                pNTTransactState->pNtTransactCreateHeader->ulSecurityDescLength)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pNTTransactState->pSecDesc = pDataCursor;

        if (!RtlValidRelativeSecurityDescriptor(
                        (PSECURITY_DESCRIPTOR_RELATIVE)pNTTransactState->pSecDesc,
                        pNTTransactState->pNtTransactCreateHeader->ulSecurityDescLength,
                        secInfoAll))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pDataCursor      +=
                pNTTransactState->pNtTransactCreateHeader->ulSecurityDescLength;
        ulOffset         +=
                pNTTransactState->pNtTransactCreateHeader->ulSecurityDescLength;
        ulBytesAvailable -=
                pNTTransactState->pNtTransactCreateHeader->ulSecurityDescLength;
    }

    if (pNTTransactState->pNtTransactCreateHeader->ulEALength)
    {
        if (ulBytesAvailable <
                pNTTransactState->pNtTransactCreateHeader->ulEALength)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pNTTransactState->pEA = pDataCursor;

        // pDataCursor      +=
        //         pNTTransactState->pNtTransactCreateHeader->ulEALength;
        // ulOffset         +=
        //         pNTTransactState->pNtTransactCreateHeader->ulEALength;
        // ulBytesAvailable -=
        //         pNTTransactState->pNtTransactCreateHeader->ulEALength;
    }

    ntStatus = SrvAllocateMemory(
                    sizeof(IO_FILE_NAME),
                    (PVOID*)&pNTTransactState->pFilename);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pNTTransactState->pNtTransactCreateHeader->ulRootDirectoryFid != 0)
    {
        wchar16_t wszFwdSlash[]  = {'/',  0};
        wchar16_t wszBackSlash[] = {'\\', 0};

        ntStatus = SrvTreeFindFile(
                        pNTTransactState->pTree,
                        pNTTransactState->pNtTransactCreateHeader->ulRootDirectoryFid,
                        &pNTTransactState->pRootDirectory);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pwszFilename && *pwszFilename)
        {
            if ((*pwszFilename == wszFwdSlash[0]) ||
                (*pwszFilename == wszBackSlash[0]))
            {
                ntStatus = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pNTTransactState->pTree->mutex);

            ntStatus = SrvBuildFilePath(
                            NULL, /* relative path */
                            pwszFilename,
                            &pNTTransactState->pFilename->FileName);
            BAIL_ON_NT_STATUS(ntStatus);

            LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pNTTransactState->pTree->mutex);
        }

        pNTTransactState->pFilename->RootFileHandle =
                        pNTTransactState->pRootDirectory->hFile;
    }
    else
    {
        LWIO_LOCK_RWMUTEX_SHARED(bTreeInLock, &pNTTransactState->pTree->mutex);

        ntStatus = SrvBuildFilePath(
                        pNTTransactState->pTree->pShareInfo->pwszPath,
                        pwszFilename,
                        &pNTTransactState->pFilename->FileName);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pNTTransactState->pTree->mutex);
    }

    /* For named pipes, we need to pipe some extra data into the npfs driver:
     *  - Session key
     *  - Client principal name
     *  - Client address
     */
    if (SrvTreeIsNamedPipe(pNTTransactState->pTree))
    {
        ntStatus = IoRtlEcpListAllocate(&pNTTransactState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeSessionKey(
                       pConnection,
                       pNTTransactState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvConnectionGetNamedPipeClientAddress(
                       pConnection,
                       pNTTransactState->pEcpList);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bTreeInLock, &pNTTransactState->pTree->mutex);

    SRV_SAFE_FREE_MEMORY(pwszFilename);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvQueryNTTransactFileInformation(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                     ntStatus = 0;
    PSRV_PROTOCOL_EXEC_CONTEXT   pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1     pCtxSmb1     = pCtxProtocol->pSmb1Context;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pNTTransactState->pFileBasicInfo)
    {
        pNTTransactState->pFileBasicInfo = &pNTTransactState->fileBasicInfo;

        SrvPrepareNTTransactStateAsync(pNTTransactState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pNTTransactState->pFile->hFile,
                        pNTTransactState->pAcb,
                        &pNTTransactState->ioStatusBlock,
                        pNTTransactState->pFileBasicInfo,
                        sizeof(pNTTransactState->fileBasicInfo),
                        FileBasicInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseNTTransactStateAsync(pNTTransactState); // completed sync
    }

    if (!pNTTransactState->pFileStdInfo)
    {
        pNTTransactState->pFileStdInfo = &pNTTransactState->fileStdInfo;

        SrvPrepareNTTransactStateAsync(pNTTransactState, pExecContext);

        ntStatus = IoQueryInformationFile(
                        pNTTransactState->pFile->hFile,
                        pNTTransactState->pAcb,
                        &pNTTransactState->ioStatusBlock,
                        pNTTransactState->pFileStdInfo,
                        sizeof(pNTTransactState->fileStdInfo),
                        FileStandardInformation);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvReleaseNTTransactStateAsync(pNTTransactState); // completed sync
    }

    if (SrvTreeIsNamedPipe(pNTTransactState->pTree))
    {
        if (!pNTTransactState->pFilePipeInfo)
        {
            pNTTransactState->pFilePipeInfo = &pNTTransactState->filePipeInfo;

            SrvPrepareNTTransactStateAsync(pNTTransactState, pExecContext);

            ntStatus = IoQueryInformationFile(
                            pNTTransactState->pFile->hFile,
                            pNTTransactState->pAcb,
                            &pNTTransactState->ioStatusBlock,
                            pNTTransactState->pFilePipeInfo,
                            sizeof(pNTTransactState->filePipeInfo),
                            FilePipeInformation);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseNTTransactStateAsync(pNTTransactState); // completed sync
        }

        if (!pNTTransactState->pFilePipeLocalInfo)
        {
            pNTTransactState->pFilePipeLocalInfo = &pNTTransactState->filePipeLocalInfo;

            SrvPrepareNTTransactStateAsync(pNTTransactState, pExecContext);

            ntStatus = IoQueryInformationFile(
                            pNTTransactState->pFile->hFile,
                            pNTTransactState->pAcb,
                            &pNTTransactState->ioStatusBlock,
                            pNTTransactState->pFilePipeLocalInfo,
                            sizeof(pNTTransactState->filePipeLocalInfo),
                            FilePipeLocalInformation);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvReleaseNTTransactStateAsync(pNTTransactState); // completed sync
        }
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvRequestNTTransactOplocks(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS        ntStatus           = STATUS_SUCCESS;
    SRV_OPLOCK_INFO batchOplockChain[] =
            {
               { IO_OPLOCK_REQUEST_OPLOCK_BATCH,   SMB_OPLOCK_LEVEL_BATCH },
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2, SMB_OPLOCK_LEVEL_II    },
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    SRV_OPLOCK_INFO exclOplockChain[] =
            {
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1, SMB_OPLOCK_LEVEL_I     },
               { IO_OPLOCK_REQUEST_OPLOCK_LEVEL_2, SMB_OPLOCK_LEVEL_II    },
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    SRV_OPLOCK_INFO noOplockChain[] =
            {
               { SMB_OPLOCK_LEVEL_NONE,            SMB_OPLOCK_LEVEL_NONE  }
            };
    PSRV_OPLOCK_INFO           pOplockCursor = NULL;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1      = pCtxProtocol->pSmb1Context;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;
    PSRV_OPLOCK_STATE_SMB_V1     pOplockState  = NULL;
    BOOLEAN                      bContinue     = TRUE;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    if (SrvTreeIsNamedPipe(pNTTransactState->pTree) ||
        pNTTransactState->fileStdInfo.Directory)
    {
        pOplockCursor = &noOplockChain[0];

        goto done;
    }

    ntStatus = SrvBuildOplockState(
                    pExecContext->pConnection,
                    pNTTransactState->pSession,
                    pNTTransactState->pTree,
                    pNTTransactState->pFile,
                    &pOplockState);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pNTTransactState->pNtTransactCreateHeader->ulFlags &
                    SMB_OPLOCK_REQUEST_BATCH)
    {
        pOplockCursor = &batchOplockChain[0];
    }
    else if (pNTTransactState->pNtTransactCreateHeader->ulFlags &
                    SMB_OPLOCK_REQUEST_EXCLUSIVE)
    {
        pOplockCursor = &exclOplockChain[0];
    }
    else
    {
        pOplockCursor = &noOplockChain[0];
    }

    while (bContinue && (pOplockCursor->oplockRequest != SMB_OPLOCK_LEVEL_NONE))
    {
        pOplockState->oplockBuffer_in.OplockRequestType =
                        pOplockCursor->oplockRequest;

        SrvPrepareOplockStateAsync(pOplockState);

        ntStatus = IoFsControlFile(
                        pNTTransactState->pFile->hFile,
                        pOplockState->pAcb,
                        &pOplockState->ioStatusBlock,
                        IO_FSCTL_OPLOCK_REQUEST,
                        &pOplockState->oplockBuffer_in,
                        sizeof(pOplockState->oplockBuffer_in),
                        &pOplockState->oplockBuffer_out,
                        sizeof(pOplockState->oplockBuffer_out));
        switch (ntStatus)
        {
            case STATUS_OPLOCK_NOT_GRANTED:

                SrvReleaseOplockStateAsync(pOplockState); // completed sync

                pOplockCursor++;

                break;

            case STATUS_PENDING:

                ntStatus = SrvFileSetOplockState(
                                pNTTransactState->pFile,
                                pOplockState,
                                &SrvReleaseOplockStateHandle);
                BAIL_ON_NT_STATUS(ntStatus);

                InterlockedIncrement(&pOplockState->refCount);

                SrvFileSetOplockLevel(
                                pNTTransactState->pFile,
                                pOplockCursor->oplockLevel);

                ntStatus = STATUS_SUCCESS;

                bContinue = FALSE;

                break;

            default:

                SrvReleaseOplockStateAsync(pOplockState); // completed sync

                BAIL_ON_NT_STATUS(ntStatus);

                break;
        }
    }

done:

    pNTTransactState->ucOplockLevel = pOplockCursor->oplockLevel;

cleanup:

    if (pOplockState)
    {
        SrvReleaseOplockState(pOplockState);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvBuildNTTransactCreateResponse(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION       pConnection  = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1     = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg         = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest  = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse = &pCtxSmb1->pResponses[iMsg];
    PBYTE   pOutBuffer           = pSmbResponse->pBuffer;
    ULONG   ulBytesAvailable     = pSmbResponse->ulBytesAvailable;
    ULONG   ulOffset             = 0;
    ULONG   ulTotalBytesUsed     = 0;
    ULONG   ulDataOffset         = 0;
    ULONG   ulParameterOffset    = 0;
    ULONG   ulPackageBytesUsed   = 0;
    PUSHORT pSetup               = NULL;
    UCHAR   ucResponseSetupCount = 0;
    PBYTE   pData                = NULL;
    ULONG   ulDataLength         = 0;
    SMB_NT_TRANSACT_CREATE_RESPONSE_HEADER responseHeader   = {0};
    PSRV_NTTRANSACT_STATE_SMB_V1           pNTTransactState = NULL;

    pNTTransactState = (PSRV_NTTRANSACT_STATE_SMB_V1)pCtxSmb1->hState;

    if (!pSmbResponse->ulSerialNum)
    {
        ntStatus = SrvMarshalHeader_SMB_V1(
                        pOutBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        COM_NT_TRANSACT,
                        STATUS_SUCCESS,
                        TRUE,
                        pCtxSmb1->pTree->tid,
                        SMB_V1_GET_PROCESS_ID(pSmbRequest->pHeader),
                        pNTTransactState->pSession->uid,
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
                        COM_NT_TRANSACT,
                        &pSmbResponse->pWordCount,
                        &pSmbResponse->pAndXHeader,
                        &pSmbResponse->usHeaderSize);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 18 + ucResponseSetupCount;

    responseHeader.ucOplockLevel     = pNTTransactState->ucOplockLevel;
    responseHeader.usFid             = pNTTransactState->pFile->fid;
    responseHeader.ulCreateAction    = pNTTransactState->ulCreateAction;
    responseHeader.llCreationTime    = pNTTransactState->fileBasicInfo.CreationTime;
    responseHeader.llLastAccessTime  = pNTTransactState->fileBasicInfo.LastAccessTime;
    responseHeader.llLastWriteTime   = pNTTransactState->fileBasicInfo.LastWriteTime;
    responseHeader.llChangeTime      = pNTTransactState->fileBasicInfo.ChangeTime;
    responseHeader.ulExtFileAttributes = pNTTransactState->fileBasicInfo.FileAttributes;
    responseHeader.ullAllocationSize   = pNTTransactState->fileStdInfo.AllocationSize;
    responseHeader.ullEndOfFile        = pNTTransactState->fileStdInfo.EndOfFile;

    if (SrvTreeIsNamedPipe(pNTTransactState->pTree))
    {
        ntStatus = SrvMarshallPipeInfo(
                        &pNTTransactState->filePipeInfo,
                        &pNTTransactState->filePipeLocalInfo,
                        &responseHeader.usDeviceState);
        BAIL_ON_NT_STATUS(ntStatus);

        responseHeader.usFileType =
                            (USHORT)pNTTransactState->filePipeInfo.ReadMode;
    }
    else
    {
        responseHeader.usFileType = 0;
        // TODO: Get these values from the driver
        responseHeader.usDeviceState = SMB_DEVICE_STATE_NO_EAS |
                                       SMB_DEVICE_STATE_NO_SUBSTREAMS |
                                       SMB_DEVICE_STATE_NO_REPARSE_TAG;
    }

    responseHeader.bDirectory = pNTTransactState->fileStdInfo.Directory;

    ntStatus = WireMarshallNtTransactionResponse(
                        pOutBuffer,
                        ulBytesAvailable,
                        ulOffset,
                        pSetup,
                        ucResponseSetupCount,
                        (PBYTE)&responseHeader,
                        sizeof(responseHeader),
                        pData,
                        ulDataLength,
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
SrvBuildNTTransactState(
    PNT_TRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                        pusBytecount,
    PUSHORT                        pSetup,
    PBYTE                          pParameters,
    PBYTE                          pData,
    PSRV_NTTRANSACT_STATE_SMB_V1*  ppNTTransactState
    )
{
    NTSTATUS                     ntStatus         = STATUS_SUCCESS;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_NTTRANSACT_STATE_SMB_V1),
                    (PVOID*)&pNTTransactState);
    BAIL_ON_NT_STATUS(ntStatus);

    pNTTransactState->refCount = 1;

    pthread_mutex_init(&pNTTransactState->mutex, NULL);
    pNTTransactState->pMutex = &pNTTransactState->mutex;

    pNTTransactState->stage = SRV_NTTRANSACT_STAGE_SMB_V1_INITIAL;

    pNTTransactState->pRequestHeader = pRequestHeader;
    pNTTransactState->pusBytecount   = pusBytecount;
    pNTTransactState->pSetup         = pSetup;
    pNTTransactState->pParameters    = pParameters;
    pNTTransactState->pData          = pData;

    *ppNTTransactState = pNTTransactState;

cleanup:

    return ntStatus;

error:

    *ppNTTransactState = NULL;

    if (pNTTransactState)
    {
        SrvFreeNTTransactState(pNTTransactState);
    }

    goto cleanup;
}

static
VOID
SrvPrepareNTTransactStateAsync(
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState,
    PSRV_EXEC_CONTEXT            pExecContext
    )
{
    pNTTransactState->acb.Callback        = &SrvExecuteNTTransactAsyncCB;

    pNTTransactState->acb.CallbackContext = pExecContext;
    InterlockedIncrement(&pExecContext->refCount);

    pNTTransactState->acb.AsyncCancelContext = NULL;

    pNTTransactState->pAcb = &pNTTransactState->acb;
}

static
VOID
SrvExecuteNTTransactAsyncCB(
    PVOID pContext
    )
{
    NTSTATUS                   ntStatus         = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT          pExecContext     = (PSRV_EXEC_CONTEXT)pContext;
    PSRV_PROTOCOL_EXEC_CONTEXT pProtocolContext = pExecContext->pProtocolContext;
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState = NULL;
    BOOLEAN                      bInLock          = FALSE;

    pNTTransactState =
        (PSRV_NTTRANSACT_STATE_SMB_V1)pProtocolContext->pSmb1Context->hState;

    LWIO_LOCK_MUTEX(bInLock, &pNTTransactState->mutex);

    if (pNTTransactState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pNTTransactState->pAcb->AsyncCancelContext);
    }

    pNTTransactState->pAcb = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &pNTTransactState->mutex);

    ntStatus = SrvProdConsEnqueue(gProtocolGlobals_SMB_V1.pWorkQueue, pContext);
    if (ntStatus != STATUS_SUCCESS)
    {
        LWIO_LOG_ERROR("Failed to enqueue execution context [status:0x%x]",
                       ntStatus);

        SrvReleaseExecContext(pExecContext);
    }
}

VOID
SrvReleaseNTTransactStateAsync(
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState
    )
{
    if (pNTTransactState->pAcb)
    {
        pNTTransactState->acb.Callback = NULL;

        if (pNTTransactState->pAcb->CallbackContext)
        {
            PSRV_EXEC_CONTEXT pExecContext =
                (PSRV_EXEC_CONTEXT)pNTTransactState->pAcb->CallbackContext;

            SrvReleaseExecContext(pExecContext);

            pNTTransactState->pAcb->CallbackContext = NULL;
        }

        if (pNTTransactState->pAcb->AsyncCancelContext)
        {
            IoDereferenceAsyncCancelContext(
                    &pNTTransactState->pAcb->AsyncCancelContext);
        }

        pNTTransactState->pAcb = NULL;
    }
}

static
VOID
SrvReleaseNTTransactStateHandle(
    HANDLE hNTTransactState
    )
{
    SrvReleaseNTTransactState((PSRV_NTTRANSACT_STATE_SMB_V1)hNTTransactState);
}

static
VOID
SrvReleaseNTTransactState(
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState
    )
{
    if (InterlockedDecrement(&pNTTransactState->refCount) == 0)
    {
        SrvFreeNTTransactState(pNTTransactState);
    }
}

static
VOID
SrvFreeNTTransactState(
    PSRV_NTTRANSACT_STATE_SMB_V1 pNTTransactState
    )
{
    if (pNTTransactState->pAcb && pNTTransactState->pAcb->AsyncCancelContext)
    {
        IoDereferenceAsyncCancelContext(
                &pNTTransactState->pAcb->AsyncCancelContext);
    }

    if (pNTTransactState->pEcpList)
    {
        IoRtlEcpListFree(&pNTTransactState->pEcpList);
    }

    if (pNTTransactState->pFilename)
    {
        if (pNTTransactState->pFilename->FileName)
        {
            SrvFreeMemory(pNTTransactState->pFilename->FileName);
        }

        SrvFreeMemory(pNTTransactState->pFilename);
    }

    // TODO: Free the following if set
    // pSecurityQOS;

    if (pNTTransactState->hFile)
    {
        IoCloseFile(pNTTransactState->hFile);
    }

    if (pNTTransactState->bRemoveFileFromTree)
    {
        NTSTATUS ntStatus2 = 0;

        SrvFileResetOplockState(pNTTransactState->pFile);

        ntStatus2 = SrvTreeRemoveFile(
                        pNTTransactState->pTree,
                        pNTTransactState->pFile->fid);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to remove file from tree [Tid:%d][Fid:%d][code:%d]",
                            pNTTransactState->pTree->tid,
                            pNTTransactState->pFile->fid,
                            ntStatus2);
        }
    }

    if (pNTTransactState->pFile)
    {
        SrvFileRelease(pNTTransactState->pFile);
    }

    if (pNTTransactState->pRootDirectory)
    {
        SrvFileRelease(pNTTransactState->pRootDirectory);
    }

    if (pNTTransactState->pTree)
    {
        SrvTreeRelease(pNTTransactState->pTree);
    }

    if (pNTTransactState->pSession)
    {
        SrvSessionRelease(pNTTransactState->pSession);
    }

    if (pNTTransactState->pSecurityDescriptor2)
    {
        SrvFreeMemory(pNTTransactState->pSecurityDescriptor2);
    }

    if (pNTTransactState->pResponseBuffer)
    {
        SrvFreeMemory(pNTTransactState->pResponseBuffer);
    }

    if (pNTTransactState->pMutex)
    {
        pthread_mutex_destroy(&pNTTransactState->mutex);
    }

    SrvFreeMemory(pNTTransactState);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
