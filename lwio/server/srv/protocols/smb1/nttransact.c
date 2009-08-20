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
SrvBuildNTTransactState(
    PNT_TRANSACTION_REQUEST_HEADER pRequestHeader,
    PUSHORT                        pusBytecount,
    PUSHORT                        pSetup,
    PBYTE                          pParameters,
    PBYTE                          pData,
    PSRV_NTTRANSACT_STATE_SMB_V1*  ppNTTransactState
    );

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

    if (pNTTransactState->pFile)
    {
        SrvFileRelease(pNTTransactState->pFile);
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

