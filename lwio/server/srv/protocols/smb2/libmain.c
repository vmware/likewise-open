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
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvProcessRequestSpecific_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
NTSTATUS
SrvSendInterimResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    );

static
PCSTR
SrvGetCommandDescription_SMB_V2(
    ULONG ulCommand
    );

NTSTATUS
SrvProtocolInit_SMB_V2(
    PSMB_PROD_CONS_QUEUE pWorkQueue
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V2.mutex);

    gProtocolGlobals_SMB_V2.pWorkQueue = pWorkQueue;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V2.mutex);

    return status;
}

NTSTATUS
SrvProtocolExecute_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                 ntStatus     = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION     pConnection  = pExecContext->pConnection;
    PSRV_EXEC_CONTEXT_SMB_V2 pSmb2Context = NULL;
    USHORT   usCurrentCommand = 0;
    BOOLEAN  bPopStatMessage  = FALSE;

    if (!pExecContext->pProtocolContext->pSmb2Context)
    {
        SRV_LOG_CALL_DEBUG( pExecContext->pLogContext,
                            SMB_PROTOCOL_VERSION_2,
                            0xFFFF,
                            &SrvLogRequest_SMB_V2,
                            pExecContext);

        ntStatus = SrvBuildExecContext_SMB_V2(
                        pExecContext->pConnection,
                        pExecContext->pSmbRequest,
                        &pExecContext->pProtocolContext->pSmb2Context);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    pSmb2Context = pExecContext->pProtocolContext->pSmb2Context;

    if (!pExecContext->pSmbResponse)
    {
        ntStatus = SMBPacketAllocate(
                        pExecContext->pConnection->hPacketAllocator,
                        &pExecContext->pSmbResponse);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBPacketBufferAllocate(
                        pConnection->hPacketAllocator,
                        (64 * 1024) + 4096,
                        &pExecContext->pSmbResponse->pRawBuffer,
                        &pExecContext->pSmbResponse->bufferLen);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMB2InitPacket(pExecContext->pSmbResponse, TRUE);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (;
         pSmb2Context->iMsg < pSmb2Context->ulNumRequests;
         pSmb2Context->iMsg++)
    {
        ULONG iMsg = pSmb2Context->iMsg;
        PSRV_MESSAGE_SMB_V2 pRequest = &pSmb2Context->pRequests[iMsg];
        PSRV_MESSAGE_SMB_V2 pResponse = &pSmb2Context->pResponses[iMsg];
        PSRV_MESSAGE_SMB_V2 pPrevResponse = NULL;

        if (iMsg > 0)
        {
            pPrevResponse = &pSmb2Context->pResponses[iMsg-1];
        }

        if (pPrevResponse && (pPrevResponse->ulMessageSize % 8))
        {
            ULONG ulBytesAvailable = 0;
            USHORT usAlign = 8 - (pPrevResponse->ulMessageSize % 8);

            ulBytesAvailable = pExecContext->pSmbResponse->bufferLen -
                               pExecContext->pSmbResponse->bufferUsed;

            if (ulBytesAvailable < usAlign)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                break;
            }
            else
            {
                pExecContext->pSmbResponse->bufferUsed += usAlign;
                pPrevResponse->ulMessageSize += usAlign;
                pPrevResponse->pHeader->ulChainOffset =
                                    pPrevResponse->ulMessageSize;
            }
        }

        pResponse->pBuffer =  pExecContext->pSmbResponse->pRawBuffer +
                              pExecContext->pSmbResponse->bufferUsed;

        pResponse->ulMessageSize = 0;
        pResponse->ulBytesAvailable =   pExecContext->pSmbResponse->bufferLen -
                                        pExecContext->pSmbResponse->bufferUsed;

        if (pExecContext->pStatInfo)
        {
            usCurrentCommand = pRequest->pHeader->command;

            ntStatus = SrvStatisticsPushMessage(
                            pExecContext->pStatInfo,
                            usCurrentCommand,
                            pRequest->ulMessageSize);
            BAIL_ON_NT_STATUS(ntStatus);

            bPopStatMessage = TRUE;
        }

        ntStatus = SrvProcessRequestSpecific_SMB_V2(pExecContext);

        switch (ntStatus)
        {
            case STATUS_PENDING:

                break;

            case STATUS_SUCCESS:

                if (pExecContext->pProtocolContext->pSmb2Context->hState &&
                    pExecContext->pProtocolContext->pSmb2Context->pfnStateRelease)
                {
                    pExecContext->pProtocolContext->pSmb2Context->pfnStateRelease(pExecContext->pProtocolContext->pSmb2Context->hState);
                    pExecContext->pProtocolContext->pSmb2Context->hState = NULL;
                    pExecContext->pProtocolContext->pSmb2Context->pfnStateRelease = NULL;
                }

                break;

            default:

                if (pExecContext->pProtocolContext->pSmb2Context->hState &&
                    pExecContext->pProtocolContext->pSmb2Context->pfnStateRelease)
                {
                    pExecContext->pProtocolContext->pSmb2Context->pfnStateRelease(pExecContext->pProtocolContext->pSmb2Context->hState);
                    pExecContext->pProtocolContext->pSmb2Context->hState = NULL;
                    pExecContext->pProtocolContext->pSmb2Context->pfnStateRelease = NULL;
                }

                if (!pExecContext->bInternal)
                {
                    ntStatus = SrvBuildErrorResponse_SMB_V2(
                                    pExecContext,
                                    pExecContext->ullAsyncId,
                                    ntStatus);
                }

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (pPrevResponse && pPrevResponse->pHeader)
        {
            pPrevResponse->pHeader->ulChainOffset =
                pResponse->ulMessageSize ? pPrevResponse->ulMessageSize : 0;
        }

        pExecContext->pSmbResponse->bufferUsed += pResponse->ulMessageSize;

        if (bPopStatMessage)
        {
            NTSTATUS ntStatus2 =
                    SrvStatisticsPopMessage(
                            pExecContext->pStatInfo,
                            usCurrentCommand,
                            (pResponse->ulMessageSize ?
                                    pResponse->ulMessageSize :
                                    pResponse->ulZctMessageSize),
                            ntStatus);
            if (ntStatus2)
            {
                LWIO_LOG_ERROR( "Error: Failed to notify statistics "
                                "module on end of message processing "
                                "[error: %u]", ntStatus2);
            }

            bPopStatMessage = FALSE;
            usCurrentCommand = 0;
        }
    }

    ntStatus = SMBPacketMarshallFooter(pExecContext->pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    switch (ntStatus)
    {
        case STATUS_PENDING:

            break;

        default:

            if (bPopStatMessage)
            {
                NTSTATUS ntStatus2 =
                        SrvStatisticsPopMessage(
                                pExecContext->pStatInfo,
                                usCurrentCommand,
                                pExecContext->pSmbResponse->bufferUsed,
                                ntStatus);
                if (ntStatus2)
                {
                    LWIO_LOG_ERROR( "Error: Failed to notify statistics "
                                    "module on end of message processing "
                                    "[error: %u]", ntStatus2);
                }
            }

            break;
    }

    goto cleanup;
}

NTSTATUS
SrvProtocolCloseFile_SMB_V2(
    PLWIO_SRV_TREE_2 pTree,
    PLWIO_SRV_FILE_2 pFile
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pTree || !pFile)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SrvFile2ResetOplockState(pFile);

    ntStatus = SrvTree2RemoveFile(
                    pTree,
                    &pFile->fid);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvFile2Rundown(pFile);

error:

    return ntStatus;
}

static
NTSTATUS
SrvProcessRequestSpecific_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus     = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2     = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg         = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest  = &pCtxSmb2->pRequests[iMsg];

    LWIO_LOG_VERBOSE("Executing command [%s:%d]",
                     SrvGetCommandDescription_SMB_V2(pSmbRequest->pHeader->command),
                     pSmbRequest->pHeader->command);

    if (!iMsg &&
        LwIsSetFlag(pSmbRequest->pHeader->ulFlags,SMB2_FLAGS_RELATED_OPERATION))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Clear the context items before processing an un-related message
    if (iMsg &&
        !LwIsSetFlag(pSmbRequest->pHeader->ulFlags,SMB2_FLAGS_RELATED_OPERATION))
    {
        if (pCtxSmb2->pFile)
        {
            SrvFile2Release(pCtxSmb2->pFile);
            pCtxSmb2->pFile = NULL;
        }
        if (pCtxSmb2->pTree)
        {
            SrvTree2Release(pCtxSmb2->pTree);
            pCtxSmb2->pTree = NULL;
        }
        if (pCtxSmb2->pSession)
        {
            SrvSession2Release(pCtxSmb2->pSession);
            pCtxSmb2->pSession = NULL;
        }

        pCtxSmb2->bFileOpened = FALSE;
        pCtxSmb2->bFileClosed = FALSE;
    }

    switch (pSmbRequest->pHeader->command)
    {
        case COM2_NEGOTIATE:

            ntStatus = SrvProcessNegotiate_SMB_V2(pExecContext);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvConnectionSetProtocolVersion(
                            pExecContext->pConnection,
                            SMB_PROTOCOL_VERSION_2);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvConnectionSetState(
                    pExecContext->pConnection,
                    LWIO_SRV_CONN_STATE_NEGOTIATE);

            break;

        case COM2_ECHO:
        case COM2_SESSION_SETUP:

            {
                switch (SrvConnectionGetState(pExecContext->pConnection))
                {
                    case LWIO_SRV_CONN_STATE_NEGOTIATE:
                    case LWIO_SRV_CONN_STATE_READY:

                        break;

                    default:

                        ntStatus = STATUS_INVALID_SERVER_STATE;

                        break;
                }
            }

            break;

        default:

            switch (SrvConnectionGetState(pExecContext->pConnection))
            {
                case LWIO_SRV_CONN_STATE_READY:

                    break;

                default:

                    ntStatus = STATUS_INVALID_SERVER_STATE;

                    break;
            }

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pSmbRequest->pHeader->command)
    {
        case COM2_NEGOTIATE:

            break;

        case COM2_SESSION_SETUP:

            ntStatus = SrvProcessSessionSetup_SMB_V2(pExecContext);

            break;

        case COM2_LOGOFF:

            ntStatus = SrvProcessLogoff_SMB_V2(pExecContext);

            break;

        case COM2_TREE_CONNECT:

            ntStatus = SrvProcessTreeConnect_SMB_V2(pExecContext);

            break;

        case COM2_TREE_DISCONNECT:

            ntStatus = SrvProcessTreeDisconnect_SMB_V2(pExecContext);

            break;

        case COM2_CREATE:

            ntStatus = SrvProcessCreate_SMB_V2(pExecContext);
            if ((ntStatus == STATUS_PENDING) && pExecContext->pInterimResponse)
            {
                NTSTATUS ntStatus2 = STATUS_SUCCESS;

                ntStatus2 = SrvSendInterimResponse_SMB_V2(pExecContext);
                if (ntStatus2 != STATUS_SUCCESS)
                {
                    ntStatus = ntStatus2;
                    BAIL_ON_NT_STATUS(ntStatus);
                }
            }

            break;

        case COM2_CLOSE:

            ntStatus = SrvProcessClose_SMB_V2(pExecContext);

            break;

        case COM2_FLUSH:

            ntStatus = SrvProcessFlush_SMB_V2(pExecContext);

            break;

        case COM2_READ:

            ntStatus = SrvProcessRead_SMB_V2(pExecContext);

            break;

        case COM2_WRITE:

            ntStatus = SrvProcessWrite_SMB_V2(pExecContext);

            break;

        case COM2_LOCK:

            ntStatus = SrvProcessLock_SMB_V2(pExecContext);
            if ((ntStatus == STATUS_PENDING) && pExecContext->pInterimResponse)
            {
                NTSTATUS ntStatus2 = STATUS_SUCCESS;

                ntStatus2 = SrvSendInterimResponse_SMB_V2(pExecContext);
                if (ntStatus2 != STATUS_SUCCESS)
                {
                    ntStatus = ntStatus2;
                    BAIL_ON_NT_STATUS(ntStatus);
                }
            }

            break;

        case COM2_IOCTL:

            ntStatus = SrvProcessIOCTL_SMB_V2(pExecContext);

            break;

        case COM2_CANCEL:

            ntStatus = SrvProcessCancel_SMB_V2(pExecContext);

            break;

        case COM2_ECHO:

            ntStatus = SrvProcessEcho_SMB_V2(pExecContext);

            break;

        case COM2_FIND:

            ntStatus = SrvProcessFind_SMB_V2(pExecContext);

            break;

        case COM2_NOTIFY:

            if (pExecContext->bInternal)
            {
                ntStatus = SrvProcessNotifyCompletion_SMB_V2(pExecContext);
            }
            else
            {
                ntStatus = SrvProcessNotify_SMB_V2(pExecContext);
                if ((ntStatus == STATUS_PENDING) &&
                    pExecContext->pInterimResponse)
                {
                    ntStatus = SrvSendInterimResponse_SMB_V2(pExecContext);
                    BAIL_ON_NT_STATUS(ntStatus);
                }
            }

            break;

        case COM2_GETINFO:

            ntStatus = SrvProcessGetInfo_SMB_V2(pExecContext);

            break;

        case COM2_SETINFO:

            ntStatus = SrvProcessSetInfo_SMB_V2(pExecContext);

            break;

        case COM2_BREAK:

            if (pExecContext->bInternal)
            {
                ntStatus = SrvProcessOplock_SMB_V2(pExecContext);
            }
            else
            {
                ntStatus = SrvProcessOplockBreak_SMB_V2(pExecContext);
            }

            break;

        default:

            ntStatus = STATUS_NOT_SUPPORTED;

            break;
    }

error:

    return ntStatus;
}

static
NTSTATUS
SrvSendInterimResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = SrvProtocolTransportSendResponse(
                    pExecContext->pConnection,
                    pExecContext->pInterimResponse,
                    NULL /* Statistics info */);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMBPacketRelease(
                pExecContext->pConnection->hPacketAllocator,
                pExecContext->pInterimResponse);

    pExecContext->pInterimResponse = NULL;

    return ntStatus;

error:

    LWIO_LOG_ERROR("Failed to send auxiliary response "
                   "[code:0x%08x",
                   ntStatus);

    goto cleanup;
}

VOID
SrvProtocolFreeContext_SMB_V2(
    PSRV_EXEC_CONTEXT_SMB_V2 pProtocolContext
    )
{
    if (pProtocolContext->hState)
    {
        pProtocolContext->pfnStateRelease(pProtocolContext->hState);
    }

    if (pProtocolContext->pFile)
    {
        SrvFile2Release(pProtocolContext->pFile);
    }

    if (pProtocolContext->pTree)
    {
        SrvTree2Release(pProtocolContext->pTree);
    }

    if (pProtocolContext->pSession)
    {
        SrvSession2Release(pProtocolContext->pSession);
    }

    if (pProtocolContext->pResponses)
    {
        SrvFreeMemory(pProtocolContext->pResponses);
    }

    if (pProtocolContext->pRequests)
    {
        SrvFreeMemory(pProtocolContext->pRequests);
    }

    if (pProtocolContext->pErrorMessage)
    {
        SrvFreeMemory(pProtocolContext->pErrorMessage);
    }

    SrvFreeMemory(pProtocolContext);
}

NTSTATUS
SrvBuildInterimResponse_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext,
    ULONG64           ullAsyncId
    )
{
    NTSTATUS                   ntStatus      = STATUS_SUCCESS;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSMB2_HEADER               pHeader       = NULL;
    NTSTATUS                   errorStatus   = STATUS_PENDING;
    PSMB_PACKET pInterimResponse = NULL;
    PBYTE pOutBuffer             = NULL;
    ULONG ulOffset               = 0;
    ULONG ulBytesAvailable       = 0;
    ULONG ulBytesUsed            = 0;
    ULONG ulTotalBytesUsed       = 0;
    ULONG ulHeaderSize           = 0;

    ntStatus = SMBPacketAllocate(
                    pExecContext->pConnection->hPacketAllocator,
                    &pInterimResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pExecContext->pConnection->hPacketAllocator,
                    (64 * 1024) + 4096,
                    &pInterimResponse->pRawBuffer,
                    &pInterimResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2InitPacket(pInterimResponse, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer = pInterimResponse->pRawBuffer + sizeof(NETBIOS_HEADER);
    ulBytesAvailable = pInterimResponse->bufferLen - sizeof(NETBIOS_HEADER);

    ntStatus = SMB2MarshalHeader(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                pSmbRequest->pHeader->command,
                pSmbRequest->pHeader->usEpoch,
                1, /* credits */
                pSmbRequest->pHeader->ulPid,
                pSmbRequest->pHeader->ullCommandSequence,
                pSmbRequest->pHeader->ulTid,
                pSmbRequest->pHeader->ullSessionId,
                ullAsyncId,
                errorStatus,
                TRUE,
                LwIsSetFlag(
                    pSmbRequest->pHeader->ulFlags,
                    SMB2_FLAGS_RELATED_OPERATION),
                &pHeader,
                &ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += ulHeaderSize;
    ulOffset         += ulHeaderSize;
    ulBytesAvailable -= ulHeaderSize;
    ulTotalBytesUsed += ulHeaderSize;

    pHeader->error = errorStatus;

    ntStatus = SMB2MarshalError(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pCtxSmb2->pErrorMessage,
                    pCtxSmb2->ulErrorMessageLength,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += ulBytesUsed;
    // ulOffset         += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    pInterimResponse->bufferUsed += ulTotalBytesUsed;

    ntStatus = SMBPacketMarshallFooter(pInterimResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    if (pExecContext->pInterimResponse)
    {
        SMBPacketRelease(
                pExecContext->pConnection->hPacketAllocator,
                pExecContext->pInterimResponse);

        pExecContext->pInterimResponse = NULL;
    }

    pExecContext->pInterimResponse = pInterimResponse;
    pExecContext->ullAsyncId = ullAsyncId;

cleanup:

    return ntStatus;

error:

    if (pInterimResponse)
    {
        SMBPacketRelease(
            pExecContext->pConnection->hPacketAllocator,
            pInterimResponse);
    }

    goto cleanup;
}

NTSTATUS
SrvBuildExecContext_SMB_V2(
    PLWIO_SRV_CONNECTION      pConnection,
    PSMB_PACKET               pSmbRequest,
    PSRV_EXEC_CONTEXT_SMB_V2* ppSmb2Context
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG    ulNumRequests    = 0;
    ULONG    iRequest         = 0;
    ULONG    ulBytesAvailable = pSmbRequest->bufferUsed;
    PBYTE    pBuffer          = pSmbRequest->pRawBuffer;
    PSRV_EXEC_CONTEXT_SMB_V2 pSmb2Context = NULL;

    if (ulBytesAvailable < sizeof(NETBIOS_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBuffer += sizeof(NETBIOS_HEADER);
    ulBytesAvailable -= sizeof(NETBIOS_HEADER);

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_EXEC_CONTEXT_SMB_V2),
                    (PVOID*)&pSmb2Context);
    BAIL_ON_NT_STATUS(ntStatus);

    while (pBuffer)
    {
        PSMB2_HEADER pHeader     = NULL; // Do not free
        ULONG        ulOffset    = 0;
        ULONG        ulBytesUsed = 0;

        ulNumRequests++;

        ntStatus = SrvUnmarshalHeader_SMB_V2(
                        pBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        &pHeader,
                        &ulBytesUsed);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pHeader->ulChainOffset)
        {
            if (ulBytesAvailable < pHeader->ulChainOffset)
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pBuffer += pHeader->ulChainOffset;
            ulBytesAvailable -= pHeader->ulChainOffset;
        }
        else
        {
            pBuffer = NULL;
        }
    }

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_MESSAGE_SMB_V2) * ulNumRequests,
                    (PVOID*)&pSmb2Context->pRequests);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmb2Context->ulNumRequests = ulNumRequests;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_MESSAGE_SMB_V2) * ulNumRequests,
                    (PVOID*)&pSmb2Context->pResponses);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmb2Context->ulNumResponses = ulNumRequests;

    pBuffer = pSmbRequest->pRawBuffer + sizeof(NETBIOS_HEADER);
    ulBytesAvailable = pSmbRequest->bufferUsed - sizeof(NETBIOS_HEADER);

    for (; iRequest < ulNumRequests; iRequest++)
    {
        PSRV_MESSAGE_SMB_V2 pMessage = &pSmb2Context->pRequests[iRequest];
        ULONG ulOffset = 0;

        pMessage->pBuffer = pBuffer;

        ntStatus = SrvUnmarshalHeader_SMB_V2(
                        pMessage->pBuffer,
                        ulOffset,
                        ulBytesAvailable,
                        &pMessage->pHeader,
                        &pMessage->ulHeaderSize);
        BAIL_ON_NT_STATUS(ntStatus);

        if (pMessage->pHeader && pMessage->pHeader->ulChainOffset)
        {
            pMessage->ulMessageSize = pMessage->pHeader->ulChainOffset;
            pMessage->ulBytesAvailable = pMessage->pHeader->ulChainOffset;
            pBuffer += pMessage->pHeader->ulChainOffset;
            ulBytesAvailable -= pMessage->pHeader->ulChainOffset;
        }
        else
        {
            pMessage->ulMessageSize = ulBytesAvailable;
            pMessage->ulBytesAvailable = ulBytesAvailable;
        }
    }

    *ppSmb2Context = pSmb2Context;

cleanup:

    return ntStatus;

error:

    *ppSmb2Context = NULL;

    if (pSmb2Context)
    {
        SrvProtocolFreeContext_SMB_V2(pSmb2Context);
    }

    goto cleanup;
}

VOID
SrvProtocolShutdown_SMB_V2(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V2.mutex);

    gProtocolGlobals_SMB_V2.pWorkQueue = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V2.mutex);
}

static
PCSTR
SrvGetCommandDescription_SMB_V2(
    ULONG ulCommand
    )
{
    static struct
    {
        ULONG ulCommand;
        PCSTR pszDescription;
    } commandLookup[] =
    {
        {
            COM2_NEGOTIATE,
            COM2_NEGOTIATE_DESC
        },
        {
            COM2_SESSION_SETUP,
            COM2_SESSION_SETUP_DESC
        },
        {
            COM2_LOGOFF,
            COM2_LOGOFF_DESC
        },
        {
            COM2_TREE_CONNECT,
            COM2_TREE_CONNECT_DESC
        },
        {
            COM2_TREE_DISCONNECT,
            COM2_TREE_DISCONNECT_DESC
        },
        {
            COM2_CREATE,
            COM2_CREATE_DESC
        },
        {
            COM2_CLOSE,
            COM2_CLOSE_DESC
        },
        {
            COM2_FLUSH,
            COM2_FLUSH_DESC
        },
        {
            COM2_READ,
            COM2_READ_DESC
        },
        {
            COM2_WRITE,
            COM2_WRITE_DESC
        },
        {
            COM2_LOCK,
            COM2_LOCK_DESC
        },
        {
            COM2_IOCTL,
            COM2_IOCTL_DESC
        },
        {
            COM2_CANCEL,
            COM2_CANCEL_DESC
        },
        {
            COM2_ECHO,
            COM2_ECHO_DESC
        },
        {
            COM2_FIND,
            COM2_FIND_DESC
        },
        {
            COM2_NOTIFY,
            COM2_NOTIFY_DESC
        },
        {
            COM2_GETINFO,
            COM2_GETINFO_DESC
        },
        {
            COM2_SETINFO,
            COM2_SETINFO_DESC
        },
        {
            COM2_BREAK,
            COM2_BREAK_DESC
        }
    };
    PCSTR pszDescription = NULL;
    ULONG iDesc = 0;

    for (; iDesc < sizeof(commandLookup)/sizeof(commandLookup[0]); iDesc++)
    {
        if (commandLookup[iDesc].ulCommand == ulCommand)
        {
            pszDescription = commandLookup[iDesc].pszDescription;
            break;
        }
    }

    return (pszDescription ? pszDescription : "SMB2_UNKNOWN_COMMAND");
}

