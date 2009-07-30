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
SrvBuildRequestChain_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB2_REQUEST        pRequest
    );

static
PCSTR
SrvGetCommandDescription_SMB_V2(
    ULONG ulCommand
    );

NTSTATUS
SrvProtocolInit_SMB_V2(
    PSMB_PROD_CONS_QUEUE pAsyncWorkQueue
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V2.mutex);

    gProtocolGlobals_SMB_V2.pAsyncWorkQueue = pAsyncWorkQueue;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V2.mutex);

    return status;
}

NTSTATUS
SrvProtocolExecute_SMB_V2(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest,
    OUT PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SMB2_REQUEST request = {0};
    ULONG        iRequest = 0;
    SMB2_CONTEXT context = {0};

    ntStatus = SrvBuildRequestChain_SMB_V2(
                    pConnection,
                    pSmbRequest,
                    &request);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &request.pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &request.pResponse->pRawBuffer,
                    &request.pResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2InitPacket(request.pResponse, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvInitContextContents_SMB_V2(pConnection, &context);
    BAIL_ON_NT_STATUS(ntStatus);

    for (; iRequest < request.ulNumChainedRequests; iRequest++)
    {
        PSMB2_MESSAGE pRequest = &request.pChainedRequests[iRequest];
        PSMB2_MESSAGE pResponse = &request.pChainedResponses[iRequest];
        PSMB2_MESSAGE pPrevResponse = (iRequest > 0 ? &request.pChainedResponses[iRequest-1] : NULL);
        ULONG         ulResponseSize_starting = request.pResponse->bufferUsed;
        ULONG         ulResponseSize_ending = request.pResponse->bufferUsed;

        if (pPrevResponse && (pPrevResponse->ulSize % 8))
        {
            ULONG ulBytesAvailable = 0;
            USHORT usAlign = 8 - (pPrevResponse->ulSize % 8);

            ulBytesAvailable = request.pResponse->bufferLen -
                               request.pResponse->bufferUsed;

            if (ulBytesAvailable < usAlign)
            {
                ntStatus = STATUS_INVALID_BUFFER_SIZE;
                break;
            }
            else
            {
                request.pResponse->bufferUsed += usAlign;
                pPrevResponse->ulSize += usAlign;
            }
        }

        pResponse->pHeader = (PSMB2_HEADER)(request.pResponse->pRawBuffer +
                                            request.pResponse->bufferUsed);

        LWIO_LOG_VERBOSE("Executing command [%s:%d]",
                         SrvGetCommandDescription_SMB_V2(pRequest->pHeader->command),
                         pRequest->pHeader->command);

        switch (pRequest->pHeader->command)
        {
            case COM2_NEGOTIATE:

                ntStatus = SrvProcessNegotiate_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);
                BAIL_ON_NT_STATUS(ntStatus);

                ntStatus = SrvConnectionSetProtocolVersion(
                                pConnection,
                                SMB_PROTOCOL_VERSION_2);
                BAIL_ON_NT_STATUS(ntStatus);

                SrvConnectionSetState(pConnection, LWIO_SRV_CONN_STATE_NEGOTIATE);

                break;

            case COM2_ECHO:
            case COM2_SESSION_SETUP:

                {
                    LWIO_SRV_CONN_STATE connState = SrvConnectionGetState(pConnection);

                    if ((connState != LWIO_SRV_CONN_STATE_NEGOTIATE) &&
                        (connState != LWIO_SRV_CONN_STATE_READY))
                    {
                        ntStatus = STATUS_INVALID_SERVER_STATE;
                        BAIL_ON_NT_STATUS(ntStatus);
                    }
                }

                break;

            default:

                if (SrvConnectionGetState(pConnection) != LWIO_SRV_CONN_STATE_READY)
                {
                    ntStatus = STATUS_INVALID_SERVER_STATE;
                    BAIL_ON_NT_STATUS(ntStatus);
                }

                break;
        }

        switch (pRequest->pHeader->command)
        {
            case COM2_SESSION_SETUP:

                ntStatus = SrvProcessSessionSetup_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_LOGOFF:

                ntStatus = SrvProcessLogoff_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_TREE_CONNECT:

                ntStatus = SrvProcessTreeConnect_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_TREE_DISCONNECT:

                ntStatus = SrvProcessTreeDisconnect_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_CREATE:

                ntStatus = SrvProcessCreate_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_CLOSE:

                ntStatus = SrvProcessClose_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_FLUSH:

                ntStatus = SrvProcessFlush_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_READ:

                ntStatus = SrvProcessRead_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_WRITE:

                ntStatus = SrvProcessWrite_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_LOCK:

                ntStatus = SrvProcessLock_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_IOCTL:

                ntStatus = SrvProcessIOCTL_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_ECHO:

                ntStatus = SrvProcessEcho_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_FIND:

                ntStatus = SrvProcessFind_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_GETINFO:

                ntStatus = SrvProcessGetInfo_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            case COM2_SETINFO:

                ntStatus = SrvProcessSetInfo_SMB_V2(
                                &context,
                                pRequest,
                                request.pResponse);

                break;

            default:

                ntStatus = STATUS_NOT_IMPLEMENTED;

                break;
        }

        if (ntStatus != STATUS_SUCCESS)
        {
            ntStatus = SrvBuildErrorResponse_SMB_V2(
                            pConnection,
                            pRequest->pHeader,
                            ntStatus,
                            NULL,
                            0,
                            request.pResponse);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ulResponseSize_ending = request.pResponse->bufferUsed;
        pResponse->ulSize = ulResponseSize_ending - ulResponseSize_starting;

        if (pPrevResponse)
        {
            pPrevResponse->pHeader->ulChainOffset = pPrevResponse->ulSize;
        }
    }

    ntStatus = SMB2MarshalFooter(request.pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = request.pResponse;
    request.pResponse = NULL;

cleanup:

    SrvFreeContextContents_SMB_V2(&context);

    SRV_SAFE_FREE_MEMORY(request.pChainedRequests);
    SRV_SAFE_FREE_MEMORY(request.pChainedResponses);

    if (request.pResponse)
    {
        SMBPacketFree(request.pConnection->hPacketAllocator, request.pResponse);
    }

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvBuildRequestChain_SMB_V2(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB2_REQUEST        pRequest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG    ulNumChainedRequests = 0;
    ULONG    iRequest = 0;
    ULONG    ulPrevOffset = 0;
    ULONG    ulPacketSize = pSmbRequest->bufferLen - sizeof(NETBIOS_HEADER);
    PSMB2_HEADER pHeader = NULL;
    PSMB2_HEADER pPrevHeader = NULL;
    PSMB2_MESSAGE pRequestMessages = NULL;
    PSMB2_MESSAGE pResponseMessages = NULL;
    UCHAR         smb2magic[4] = {0xFE, 'S','M','B'};

    pHeader = pSmbRequest->pSMB2Header;

    while (pHeader)
    {
        ulNumChainedRequests++;

        if (pHeader->ulChainOffset)
        {
            ULONG ulCurOffset = 0;
            ULONG ulBytesAvailable = 0;

            ulCurOffset = ulPrevOffset + pHeader->ulChainOffset;

            if ((ulCurOffset < ulPrevOffset) || (ulCurOffset > ulPacketSize))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulBytesAvailable = ulPacketSize - ulCurOffset;

            if (ulBytesAvailable < sizeof(SMB2_HEADER))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            ulPrevOffset = ulCurOffset;

            pHeader = (PSMB2_HEADER)((PBYTE)pSmbRequest->pSMB2Header +
                                      ulCurOffset);

            if (memcmp(&smb2magic[0], &pHeader->smb[0], sizeof(smb2magic)))
            {
                ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }
        else
        {
            pHeader = NULL;
        }
    }

    ntStatus = SrvAllocateMemory(
                    sizeof(SMB2_MESSAGE) * ulNumChainedRequests,
                    (PVOID*)&pRequestMessages);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvAllocateMemory(
                    sizeof(SMB2_MESSAGE) * ulNumChainedRequests,
                    (PVOID*)&pResponseMessages);
    BAIL_ON_NT_STATUS(ntStatus);

    pPrevHeader = NULL;

    for (; iRequest < ulNumChainedRequests; iRequest++)
    {
        PSMB2_MESSAGE pMessage = &pRequestMessages[iRequest];

        if (!pPrevHeader)
        {
            pMessage->pHeader = pSmbRequest->pSMB2Header;
            pMessage->ulSize = ulPacketSize;
        }
        else
        {
            pMessage->pHeader = (PSMB2_HEADER)(((PBYTE)pPrevHeader)
                                               + pPrevHeader->ulChainOffset);
            if (pMessage->pHeader->ulChainOffset)
            {
                pMessage->ulSize = pMessage->pHeader->ulChainOffset -
                                   pPrevHeader->ulChainOffset;
            }
            else
            {
                pMessage->ulSize = ulPacketSize -
                                   ((PBYTE)pMessage->pHeader -
                                    (PBYTE)pSmbRequest->pSMB2Header);
            }
        }

        // None of the chained requests may be async
        if ((ulNumChainedRequests > 1) &&
            (pMessage->pHeader->ulFlags & SMB2_FLAGS_ASYNC_COMMAND))
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pPrevHeader = pMessage->pHeader;
    }


    pRequest->pConnection = pConnection;
    pRequest->pRequest    = pSmbRequest;
    pRequest->pChainedRequests = pRequestMessages;
    pRequest->ulNumChainedRequests = ulNumChainedRequests;
    pRequest->pChainedResponses = pResponseMessages;
    pRequest->ulNumChainedResponses = ulNumChainedRequests;

cleanup:

    return ntStatus;

error:

    pRequest->pConnection = NULL;
    pRequest->pRequest    = NULL;
    pRequest->pChainedRequests = NULL;
    pRequest->ulNumChainedRequests = 0;
    pRequest->pChainedResponses = NULL;
    pRequest->ulNumChainedResponses = 0;

    SRV_SAFE_FREE_MEMORY(pRequestMessages);
    SRV_SAFE_FREE_MEMORY(pResponseMessages);

    goto cleanup;
}

NTSTATUS
SrvProtocolShutdown_SMB_V2(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V2.mutex);

    gProtocolGlobals_SMB_V2.pAsyncWorkQueue = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V2.mutex);

    return status;
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

