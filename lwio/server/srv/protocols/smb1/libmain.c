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
 *        Protocols API - SMBV1
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
PCSTR
SrvGetCommandDescription_SMB_V1(
    USHORT usCommand
    );

NTSTATUS
SrvProtocolInit_SMB_V1(
    PSMB_PROD_CONS_QUEUE pWorkQueue
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    /* Configuration setup should always come first as other initalization
     * routines may rely on configuration parameters to be set */
    status = SrvConfigSetupInitial_SMB_V1();
    BAIL_ON_NT_STATUS(status);

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    gProtocolGlobals_SMB_V1.pWorkQueue = pWorkQueue;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
SrvProtocolExecute_SMB_V1(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT_SMB_V1 pSmb1Context = NULL;
    UCHAR    ucCurrentCommand = 0;
    BOOLEAN  bPopStatMessage  = FALSE;

    if (!pExecContext->pProtocolContext->pSmb1Context)
    {
        SRV_LOG_CALL_DEBUG( pExecContext->pLogContext,
                            SMB_PROTOCOL_VERSION_1,
                            0xFFFF,
                            &SrvLogRequest_SMB_V1,
                            pExecContext);

        ntStatus = SrvBuildExecContext_SMB_V1(
                        pExecContext->pConnection,
                        pExecContext->pSmbRequest,
                        &pExecContext->pProtocolContext->pSmb1Context);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    pSmb1Context = pExecContext->pProtocolContext->pSmb1Context;

    if (!pExecContext->pSmbResponse)
    {
        ntStatus = SMBPacketAllocate(
                        pExecContext->pConnection->hPacketAllocator,
                        &pExecContext->pSmbResponse);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBPacketBufferAllocate(
                        pExecContext->pConnection->hPacketAllocator,
                        (64 * 1024) + 4096,
                        &pExecContext->pSmbResponse->pRawBuffer,
                        &pExecContext->pSmbResponse->bufferLen);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SrvInitPacket_SMB_V1(pExecContext->pSmbResponse, TRUE);
        BAIL_ON_NT_STATUS(ntStatus);

        pExecContext->pSmbResponse->sequence =
                            pExecContext->pSmbRequest->sequence + 1;
    }

    for (;
         pSmb1Context->iMsg < pSmb1Context->ulNumRequests;
         pSmb1Context->iMsg++)
    {
        ULONG iMsg = pSmb1Context->iMsg;
        PSRV_MESSAGE_SMB_V1 pRequest = &pSmb1Context->pRequests[iMsg];
        PSRV_MESSAGE_SMB_V1 pResponse = &pSmb1Context->pResponses[iMsg];
        PSRV_MESSAGE_SMB_V1 pPrevResponse = NULL;

        if (iMsg > 0)
        {
            pPrevResponse = &pSmb1Context->pResponses[iMsg-1];

            if (pPrevResponse->pAndXHeader)
            {
                pPrevResponse->pAndXHeader->andXCommand = pRequest->ucCommand;
            }

            if (pPrevResponse->ulMessageSize % 4)
            {
                ULONG ulBytesAvailable = 0;
                USHORT usAlign = 4 - (pPrevResponse->ulMessageSize % 4);

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

                    if (pPrevResponse->pAndXHeader)
                    {
                        pPrevResponse->pAndXHeader->andXOffset =
                                        (USHORT)pPrevResponse->ulMessageSize;
                    }
                }
            }

            // TODO: Should AndX responses use this?
            pResponse->pHeader = pSmb1Context->pResponses[0].pHeader;
        }

        pResponse->ulOffset = pExecContext->pSmbResponse->bufferUsed -
                              sizeof(NETBIOS_HEADER);
        pResponse->pBuffer =  pExecContext->pSmbResponse->pRawBuffer +
                              sizeof(NETBIOS_HEADER) + pResponse->ulOffset;

        pResponse->ulMessageSize = 0;
        pResponse->ulBytesAvailable =   pExecContext->pSmbResponse->bufferLen -
                                        pExecContext->pSmbResponse->bufferUsed -
                                        sizeof(NETBIOS_HEADER);

        SRV_LOG_VERBOSE(pExecContext->pLogContext,
                        SMB_PROTOCOL_VERSION_1,
                        pRequest->ucCommand,
                        "command:%u(%s),uid(%u),mid(%u),pid(%u),tid(%u)",
                        pRequest->ucCommand,
                        SrvGetCommandDescription_SMB_V1(pRequest->ucCommand),
                        pRequest->pHeader->uid,
                        pRequest->pHeader->mid,
                        SMB_V1_GET_PROCESS_ID(pRequest->pHeader),
                        pRequest->pHeader->tid);

        if (pExecContext->pStatInfo)
        {
            ucCurrentCommand = pRequest->ucCommand;

            ntStatus = SrvStatisticsPushMessage(
                            pExecContext->pStatInfo,
                            ucCurrentCommand,
                            pRequest->ulMessageSize);
            BAIL_ON_NT_STATUS(ntStatus);

            bPopStatMessage = TRUE;
        }

        switch (pRequest->ucCommand)
        {
            case COM_NEGOTIATE:

                // Handled at a higher layer
                ntStatus = STATUS_INTERNAL_ERROR;

                break;

            case COM_SESSION_SETUP_ANDX:

                switch (SrvConnectionGetState(pExecContext->pConnection))
                {
                    case LWIO_SRV_CONN_STATE_NEGOTIATE:
                    case LWIO_SRV_CONN_STATE_READY:

                        break;

                    default:

                        ntStatus = STATUS_INVALID_SERVER_STATE;
                        break;
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

        switch (pRequest->ucCommand)
        {
            case COM_SESSION_SETUP_ANDX:

                ntStatus = SrvProcessSessionSetup(pExecContext);

                break;

            case COM_TREE_CONNECT_ANDX:

                ntStatus = SrvProcessTreeConnectAndX(pExecContext);

                break;

            case COM_OPEN_ANDX:

                ntStatus = SrvProcessOpenAndX(pExecContext);

                break;

            case COM_NT_CREATE_ANDX:

                ntStatus = SrvProcessNTCreateAndX(pExecContext);

                break;

            case COM_NT_CANCEL:

                ntStatus = SrvProcessNTCancel(pExecContext);

                break;

            case COM_LW_OPLOCK:

                ntStatus = SrvProcessOplock(pExecContext);

                break;

            case COM_LOCKING_ANDX:

                ntStatus = SrvProcessLockAndX(pExecContext);

                break;

            case COM_READ:

                ntStatus = SrvProcessRead(pExecContext);

                break;

            case COM_READ_ANDX:

                ntStatus = SrvProcessReadAndX(pExecContext);

                break;

            case COM_WRITE:

                ntStatus = SrvProcessWrite(pExecContext);

                break;

            case COM_WRITE_ANDX:

                ntStatus = SrvProcessWriteAndX(pExecContext);

                break;

            case COM_SET_INFORMATION:

                ntStatus = SrvProcessSetInformation(pExecContext);

                break;

            case COM_SET_INFORMATION2:

                ntStatus = SrvProcessSetInformation2(pExecContext);

                break;

            case COM_QUERY_INFORMATION2:

                ntStatus = SrvProcessQueryInformation2(pExecContext);

                break;

            case COM_TRANSACTION:

                ntStatus = SrvProcessTransaction(pExecContext);

                break;

            case COM_TRANSACTION2:

                ntStatus = SrvProcessTransaction2(pExecContext);

                break;

            case COM_FIND_CLOSE2:

                ntStatus = SrvProcessFindClose2(pExecContext);

                break;

            case COM_CLOSE:

                ntStatus = SrvProcessCloseAndX(pExecContext);

                break;

            case COM_CREATE_DIRECTORY:

                ntStatus = SrvProcessCreateDirectory(pExecContext);

                break;

            case COM_DELETE_DIRECTORY:

                ntStatus = SrvProcessDeleteDirectory(pExecContext);

                break;

            case COM_DELETE:

                ntStatus = SrvProcessDelete(pExecContext);

                break;

            case COM_RENAME:

                ntStatus = SrvProcessRename(pExecContext);

                break;

            case COM_NT_RENAME:

                ntStatus = SrvProcessNtRename(pExecContext);

                break;

            case COM_NT_TRANSACT:

                if (pExecContext->bInternal)
                {
                    ntStatus = SrvProcessNtTransactInternal(pExecContext);
                }
                else
                {
                    ntStatus = SrvProcessNtTransact(pExecContext);
                }

                break;

            case COM_TREE_DISCONNECT:

                ntStatus = SrvProcessTreeDisconnectAndX(pExecContext);

                break;

            case COM_ECHO:

                ntStatus = SrvProcessEchoAndX(pExecContext);

                break;

            case COM_FLUSH:

                ntStatus = SrvProcessFlush(pExecContext);

                break;

            case COM_LOGOFF_ANDX:

                ntStatus = SrvProcessLogoffAndX(pExecContext);

                break;

            case COM_CHECK_DIRECTORY:

                ntStatus = SrvProcessCheckDirectory(pExecContext);

                break;

            default:

                ntStatus = STATUS_NOT_SUPPORTED;

                break;
        }

        SRV_LOG_VERBOSE(
                pExecContext->pLogContext,
                SMB_PROTOCOL_VERSION_1,
                pRequest->ucCommand,
                "command:%u(%s),uid(%u),mid(%u),pid(%u),tid(%u),internal(%s),status(0x%x:%s)",
                pRequest->ucCommand,
                LWIO_SAFE_LOG_STRING(SrvGetCommandDescription_SMB_V1(pRequest->ucCommand)),
                pRequest->pHeader->uid,
                pRequest->pHeader->mid,
                SMB_V1_GET_PROCESS_ID(pRequest->pHeader),
                pRequest->pHeader->tid,
                pExecContext->bInternal? "TRUE" : "FALSE",
                ntStatus,
                LWIO_SAFE_LOG_STRING(LwNtStatusToName(ntStatus)));

        switch (ntStatus)
        {
            case STATUS_PENDING:

                break;

            case STATUS_SUCCESS:

                if (pExecContext->pProtocolContext->pSmb1Context->hState &&
                    pExecContext->pProtocolContext->pSmb1Context->pfnStateRelease)
                {
                    pExecContext->pProtocolContext->pSmb1Context->pfnStateRelease(pExecContext->pProtocolContext->pSmb1Context->hState);
                    pExecContext->pProtocolContext->pSmb1Context->hState = NULL;
                }

                break;

            default:

                if (pExecContext->bInternal)
                    break;

                if (iMsg == 0)
                {
                    ntStatus = SrvBuildErrorResponse_SMB_V1(
                                    pExecContext->pConnection,
                                    pRequest->pHeader,
                                    ntStatus,
                                    pResponse);
                }
                else
                {
                    /* Set the status for the whole chain */
                    pExecContext->pSmbResponse->pSMBHeader->error = ntStatus;

                    /* Terminate the chain */
                    if (pPrevResponse->pAndXHeader)
                    {
                        pPrevResponse->pAndXHeader->andXCommand =
                            COM_NO_ANDX_COMMAND;
                        pPrevResponse->pAndXHeader->andXOffset = 0;
                    }

                    /* Ensure we don't send any part of this AndX */
                    pResponse->ulMessageSize = 0;
                }

                /* Terminate loop */
                pSmb1Context->iMsg = pSmb1Context->ulNumRequests;
                ntStatus = 0;

                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        /* Don't set ANDX offsets for failure responses */

        if (pResponse->pHeader &&
	        (pResponse->pHeader->error == STATUS_SUCCESS) &&
	        pResponse->pAndXHeader)
        {
            pResponse->pAndXHeader->andXOffset = pResponse->ulMessageSize;
        }

        pExecContext->pSmbResponse->bufferUsed += pResponse->ulMessageSize;

        if (bPopStatMessage)
        {
            NTSTATUS ntStatus2 =
                    SrvStatisticsPopMessage(
                            pExecContext->pStatInfo,
                            ucCurrentCommand,
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
            ucCurrentCommand = 0;
        }
    }

    ntStatus = SMBPacketMarshallFooter(pExecContext->pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    if (pSmb1Context)
    {
        PSRV_MESSAGE_SMB_V1 pRequest =
                                &pSmb1Context->pRequests[pSmb1Context->iMsg];

        SRV_LOG_VERBOSE(
            pExecContext->pLogContext,
            SMB_PROTOCOL_VERSION_1,
            pRequest->ucCommand,
            "command:%u(%s),uid(%u),mid(%u),pid(%u),tid(%u),internal(%s),status(0x%x:%s)",
            pRequest->ucCommand,
            LWIO_SAFE_LOG_STRING(SrvGetCommandDescription_SMB_V1(pRequest->ucCommand)),
            pRequest->pHeader->uid,
            pRequest->pHeader->mid,
            SMB_V1_GET_PROCESS_ID(pRequest->pHeader),
            pRequest->pHeader->tid,
            pExecContext->bInternal? "TRUE" : "FALSE",
            ntStatus,
            LWIO_SAFE_LOG_STRING(LwNtStatusToDescription(ntStatus)));
    }

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
                                ucCurrentCommand,
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
SrvProtocolCloseFile_SMB_V1(
    PLWIO_SRV_TREE pTree,
    PLWIO_SRV_FILE pFile
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (!pTree || !pFile)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SrvFileRundown(pFile);

error:

    return ntStatus;
}

NTSTATUS
SrvBuildExecContext_SMB_V1(
    PLWIO_SRV_CONNECTION      pConnection,
    PSMB_PACKET               pSmbRequest,
    PSRV_EXEC_CONTEXT_SMB_V1* ppSmb1Context
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG    ulNumRequests    = 0;
    ULONG    iRequest         = 0;
    ULONG    ulBytesAvailable = pSmbRequest->bufferUsed;
    PBYTE    pBuffer          = pSmbRequest->pRawBuffer;
    UCHAR    ucAndXCommand    = 0xFF;
    PSRV_EXEC_CONTEXT_SMB_V1 pSmb1Context = NULL;

    if (ulBytesAvailable < sizeof(NETBIOS_HEADER))
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pBuffer += sizeof(NETBIOS_HEADER);
    ulBytesAvailable -= sizeof(NETBIOS_HEADER);

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_EXEC_CONTEXT_SMB_V1),
                    (PVOID*)&pSmb1Context);
    BAIL_ON_NT_STATUS(ntStatus);

    while (pBuffer)
    {
        PSMB_HEADER  pHeader     = NULL; // Do not free
        PBYTE        pWordCount  = NULL; // Do not free
        PANDX_HEADER pAndXHeader = NULL; // Do not free
        ULONG        ulOffset    = 0;
        USHORT       usBytesUsed = 0;

        ulNumRequests++;

        switch (ulNumRequests)
        {
            case 1:

                ntStatus = SrvUnmarshalHeader_SMB_V1(
                                pBuffer,
                                ulOffset,
                                ulBytesAvailable,
                                &pHeader,
                                &pAndXHeader,
                                &usBytesUsed);

                break;

            default:

                ntStatus = SrvUnmarshalHeaderAndX_SMB_V1(
                                pBuffer,
                                ulOffset,
                                ulBytesAvailable,
                                &pWordCount,
                                &pAndXHeader,
                                &usBytesUsed);

                break;

        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (pAndXHeader)
        {
            switch (pAndXHeader->andXCommand)
            {
                case COM_NO_ANDX_COMMAND:

                    pBuffer = NULL;

                    break;

                default:

                    if (!pAndXHeader->andXOffset ||
                        (ulBytesAvailable < pAndXHeader->andXOffset) ||
                        !SMBIsAndXCommand(pAndXHeader->andXCommand))
                    {
                        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                        BAIL_ON_NT_STATUS(ntStatus);
                    }

                    pBuffer          += pAndXHeader->andXOffset;
                    ulBytesAvailable -= pAndXHeader->andXOffset;

                    break;
            }
        }
        else
        {
            pBuffer = NULL;
        }
    }

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_MESSAGE_SMB_V1) * ulNumRequests,
                    (PVOID*)&pSmb1Context->pRequests);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmb1Context->ulNumRequests = ulNumRequests;

    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_MESSAGE_SMB_V1) * ulNumRequests,
                    (PVOID*)&pSmb1Context->pResponses);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmb1Context->ulNumResponses = ulNumRequests;

    pBuffer = pSmbRequest->pRawBuffer + sizeof(NETBIOS_HEADER);
    ulBytesAvailable = pSmbRequest->bufferUsed - sizeof(NETBIOS_HEADER);

    for (; iRequest < ulNumRequests; iRequest++)
    {
        PSRV_MESSAGE_SMB_V1 pMessage      = &pSmb1Context->pRequests[iRequest];
        PSRV_MESSAGE_SMB_V1 pResponse     = &pSmb1Context->pResponses[iRequest];
        ULONG               ulOffset      = 0;

        pMessage->ulSerialNum = pResponse->ulSerialNum = iRequest;
        pMessage->pBuffer     = pBuffer;

        switch (iRequest)
        {
            case 0 :

                ntStatus = SrvUnmarshalHeader_SMB_V1(
                                pMessage->pBuffer,
                                ulOffset,
                                ulBytesAvailable,
                                &pMessage->pHeader,
                                &pMessage->pAndXHeader,
                                &pMessage->usHeaderSize);
                BAIL_ON_NT_STATUS(ntStatus);

                pMessage->pWordCount = &pMessage->pHeader->wordCount;
                pMessage->ucCommand  = pMessage->pHeader->command;

                break;

            default:

                ntStatus = SrvVerifyAndXCommandSequence(
                                pSmb1Context->pRequests[iRequest-1].ucCommand,
                                ucAndXCommand);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = SrvUnmarshalHeaderAndX_SMB_V1(
                                pMessage->pBuffer,
                                ulOffset,
                                ulBytesAvailable,
                                &pMessage->pWordCount,
                                &pMessage->pAndXHeader,
                                &pMessage->usHeaderSize);
                BAIL_ON_NT_STATUS(ntStatus);

                pMessage->pHeader   = pSmb1Context->pRequests[0].pHeader;
                pMessage->ucCommand = ucAndXCommand;

                break;
        }

        if (pMessage->pAndXHeader &&
            pMessage->pAndXHeader->andXOffset &&
            pMessage->pAndXHeader->andXCommand != COM_NO_ANDX_COMMAND)
        {
            ucAndXCommand = pMessage->pAndXHeader->andXCommand;

            pMessage->ulMessageSize    = pMessage->pAndXHeader->andXOffset;
            pMessage->ulBytesAvailable = pMessage->pAndXHeader->andXOffset;

            pBuffer          += pMessage->pAndXHeader->andXOffset;
            ulBytesAvailable -= pMessage->pAndXHeader->andXOffset;
        }
        else
        {
            pMessage->ulMessageSize    = ulBytesAvailable;
            pMessage->ulBytesAvailable = ulBytesAvailable;
        }
    }

    *ppSmb1Context = pSmb1Context;

cleanup:

    return ntStatus;

error:

    *ppSmb1Context = NULL;

    if (pSmb1Context)
    {
        SrvProtocolFreeContext_SMB_V1(pSmb1Context);
    }

    goto cleanup;
}

VOID
SrvProtocolFreeContext_SMB_V1(
    PSRV_EXEC_CONTEXT_SMB_V1 pProtocolContext
    )
{
    if (pProtocolContext->hState)
    {
        pProtocolContext->pfnStateRelease(pProtocolContext->hState);
    }

    if (pProtocolContext->pFile)
    {
        SrvFileRelease(pProtocolContext->pFile);
    }

    if (pProtocolContext->pTree)
    {
        SrvTreeRelease(pProtocolContext->pTree);
    }

    if (pProtocolContext->pSession)
    {
        SrvSessionRelease(pProtocolContext->pSession);
    }

    if (pProtocolContext->pResponses)
    {
        SrvFreeMemory(pProtocolContext->pResponses);
    }

    if (pProtocolContext->pRequests)
    {
        SrvFreeMemory(pProtocolContext->pRequests);
    }

    SrvFreeMemory(pProtocolContext);
}

NTSTATUS
SrvProtocolBuildErrorResponse_SMB_V1(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_HEADER          pRequestHeader,
    NTSTATUS             errorStatus,
    PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_PACKET pSmbResponse = NULL;
    SRV_MESSAGE_SMB_V1 response = {0};

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    (64 * 1024) + 4096,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvInitPacket_SMB_V1(pSmbResponse, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    response.pBuffer = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
    response.ulBytesAvailable = pSmbResponse->bufferLen - pSmbResponse->bufferUsed;

    ntStatus = SrvBuildErrorResponse_SMB_V1(
                    pConnection,
                    pRequestHeader,
                    errorStatus,
                    &response);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += response.ulMessageSize;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(pConnection->hPacketAllocator, pSmbResponse);
    }

    goto cleanup;
}

NTSTATUS
SrvBuildErrorResponse_SMB_V1(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_HEADER          pRequestHeader,
    NTSTATUS             errorStatus,
    PSRV_MESSAGE_SMB_V1  pSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PERROR_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    PBYTE  pBuffer          = pSmbResponse->pBuffer;
    ULONG  ulOffset         = 0;
    ULONG  ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG  ulTotalBytesUsed = 0;
    USHORT usBytesUsed      = 0;

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    pRequestHeader->command,
                    errorStatus,
                    TRUE,
                    pConnection->serverProperties.Capabilities,
                    pRequestHeader->tid,
                    SMB_V1_GET_PROCESS_ID(pRequestHeader),
                    pRequestHeader->uid,
                    pRequestHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pWordCount,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pBuffer          += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    *pSmbResponse->pWordCount = 0;

    ntStatus = WireMarshallErrorResponse(
                    pBuffer,
                    ulBytesAvailable,
                    ulOffset,
                    &pResponseHeader,
                    &usBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponseHeader->byteCount = 0;

    // pBuffer          += usBytesUsed;
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

    goto cleanup;
}

VOID
SrvProtocolShutdown_SMB_V1(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    gProtocolGlobals_SMB_V1.pWorkQueue = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    /* Configuration shutdown should always come last as other shutdown
     * routines may rely on configuration parameters to be set */
    SrvConfigShutdown_SMB_V1();
}

static
PCSTR
SrvGetCommandDescription_SMB_V1(
    USHORT usCommand
    )
{
    static struct
    {
        USHORT usCommand;
        PCSTR  pszDescription;
    } commandLookup[] =
    {
        {
            COM_CREATE_DIRECTORY,
            COM_CREATE_DIRECTORY_DESC
        },
        {
            COM_DELETE_DIRECTORY,
            COM_DELETE_DIRECTORY_DESC
        },
        {
            COM_OPEN,
            COM_OPEN_DESC
        },
        {
            COM_CREATE,
            COM_CREATE_DESC
        },
        {
            COM_CLOSE,
            COM_CLOSE_DESC
        },
        {
            COM_FLUSH,
            COM_FLUSH_DESC
        },
        {
            COM_DELETE,
            COM_DELETE_DESC
        },
        {
            COM_RENAME,
            COM_RENAME_DESC
        },
        {
            COM_QUERY_INFORMATION,
            COM_QUERY_INFORMATION_DESC
        },
        {
            COM_SET_INFORMATION,
            COM_SET_INFORMATION_DESC
        },
        {
            COM_READ,
            COM_READ_DESC
        },
        {
            COM_WRITE,
            COM_WRITE_DESC
        },
        {
            COM_LOCK_BYTE_RANGE,
            COM_LOCK_BYTE_RANGE_DESC
        },
        {
            COM_UNLOCK_BYTE_RANGE,
            COM_UNLOCK_BYTE_RANGE_DESC
        },
        {
            COM_CREATE_TEMPORARY,
            COM_CREATE_TEMPORARY_DESC
        },
        {
            COM_CREATE_NEW,
            COM_CREATE_NEW_DESC
        },
        {
            COM_CHECK_DIRECTORY,
            COM_CHECK_DIRECTORY_DESC
        },
        {
            COM_PROCESS_EXIT,
            COM_PROCESS_EXIT_DESC
        },
        {
            COM_SEEK,
            COM_SEEK_DESC
        },
        {
            COM_LOCK_AND_READ,
            COM_LOCK_AND_READ_DESC
        },
        {
            COM_WRITE_AND_UNLOCK,
            COM_WRITE_AND_UNLOCK_DESC
        },
        {
            COM_READ_RAW,
            COM_READ_RAW_DESC
        },
        {
            COM_READ_MPX,
            COM_READ_MPX_DESC
        },
        {
            COM_READ_MPX_SECONDARY,
            COM_READ_MPX_SECONDARY_DESC
        },
        {
            COM_WRITE_RAW,
            COM_WRITE_RAW_DESC
        },
        {
            COM_WRITE_MPX,
            COM_WRITE_MPX_DESC
        },
        {
            COM_WRITE_MPX_SECONDARY,
            COM_WRITE_MPX_SECONDARY_DESC
        },
        {
            COM_WRITE_COMPLETE,
            COM_WRITE_COMPLETE_DESC
        },
        {
            COM_QUERY_SERVER,
            COM_QUERY_SERVER_DESC
        },
        {
            COM_SET_INFORMATION2,
            COM_SET_INFORMATION2_DESC
        },
        {
            COM_QUERY_INFORMATION2,
            COM_QUERY_INFORMATION2_DESC
        },
        {
            COM_LOCKING_ANDX,
            COM_LOCKING_ANDX_DESC
        },
        {
            COM_TRANSACTION,
            COM_TRANSACTION_DESC
        },
        {
            COM_TRANSACTION_SECONDARY,
            COM_TRANSACTION_SECONDARY_DESC
        },
        {
            COM_IOCTL,
            COM_IOCTL_DESC
        },
        {
            COM_IOCTL_SECONDARY,
            COM_IOCTL_SECONDARY_DESC
        },
        {
            COM_COPY,
            COM_COPY_DESC
        },
        {
            COM_MOVE,
            COM_MOVE_DESC
        },
        {
            COM_ECHO,
            COM_ECHO_DESC
        },
        {
            COM_WRITE_AND_CLOSE,
            COM_WRITE_AND_CLOSE_DESC
        },
        {
            COM_OPEN_ANDX,
            COM_OPEN_ANDX_DESC
        },
        {
            COM_READ_ANDX,
            COM_READ_ANDX_DESC
        },
        {
            COM_WRITE_ANDX,
            COM_WRITE_ANDX_DESC
        },
        {
            COM_NEW_FILE_SIZE,
            COM_NEW_FILE_SIZE_DESC
        },
        {
            COM_CLOSE_AND_TREE_DISC,
            COM_CLOSE_AND_TREE_DISC_DESC
        },
        {
            COM_TRANSACTION2,
            COM_TRANSACTION2_DESC
        },
        {
            COM_TRANSACTION2_SECONDARY,
            COM_TRANSACTION2_SECONDARY_DESC
        },
        {
            COM_FIND_CLOSE2,
            COM_FIND_CLOSE2_DESC
        },
        {
            COM_FIND_NOTIFY_CLOSE,
            COM_FIND_NOTIFY_CLOSE_DESC
        },
        {
            COM_TREE_CONNECT,
            COM_TREE_CONNECT_DESC
        },
        {
            COM_TREE_DISCONNECT,
            COM_TREE_DISCONNECT_DESC
        },
        {
            COM_NEGOTIATE,
            COM_NEGOTIATE_DESC
        },
        {
            COM_SESSION_SETUP_ANDX,
            COM_SESSION_SETUP_ANDX_DESC
        },
        {
            COM_LOGOFF_ANDX,
            COM_LOGOFF_ANDX_DESC
        },
        {
            COM_TREE_CONNECT_ANDX,
            COM_TREE_CONNECT_ANDX_DESC
        },
        {
            COM_QUERY_INFORMATION_DISK,
            COM_QUERY_INFORMATION_DISK_DESC
        },
        {
            COM_SEARCH,
            COM_SEARCH_DESC
        },
        {
            COM_FIND,
            COM_FIND_DESC
        },
        {
            COM_FIND_UNIQUE,
            COM_FIND_UNIQUE_DESC
        },
        {
            COM_FIND_CLOSE,
            COM_FIND_CLOSE_DESC
        },
        {
            COM_NT_TRANSACT,
            COM_NT_TRANSACT_DESC
        },
        {
            COM_NT_TRANSACT_SECONDARY,
            COM_NT_TRANSACT_SECONDARY_DESC
        },
        {
            COM_NT_CREATE_ANDX,
            COM_NT_CREATE_ANDX_DESC
        },
        {
            COM_NT_CANCEL,
            COM_NT_CANCEL_DESC
        },
        {
            COM_NT_RENAME,
            COM_NT_RENAME_DESC
        },
        {
            COM_OPEN_PRINT_FILE,
            COM_OPEN_PRINT_FILE_DESC
        },
        {
            COM_WRITE_PRINT_FILE,
            COM_WRITE_PRINT_FILE_DESC
        },
        {
            COM_CLOSE_PRINT_FILE,
            COM_CLOSE_PRINT_FILE_DESC
        },
        {
            COM_GET_PRINT_QUEUE,
            COM_GET_PRINT_QUEUE_DESC
        },
        {
            COM_READ_BULK,
            COM_READ_BULK_DESC
        },
        {
            COM_WRITE_BULK,
            COM_WRITE_BULK_DESC
        },
        {
            COM_WRITE_BULK_DATA,
            COM_WRITE_BULK_DATA_DESC
        },
        {
            COM_LW_OPLOCK,
            COM_LW_OPLOCK_DESC
        }
    };
    PCSTR pszDescription = NULL;
    ULONG iDesc = 0;

    for (; iDesc < sizeof(commandLookup)/sizeof(commandLookup[0]); iDesc++)
    {
        if (commandLookup[iDesc].usCommand == usCommand)
        {
            pszDescription = commandLookup[iDesc].pszDescription;
            break;
        }
    }

    return (pszDescription ? pszDescription : "SMB1_UNKNOWN_COMMAND");
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
