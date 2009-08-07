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
SrvBuildExecContext_SMB_V2(
    PLWIO_SRV_CONNECTION      pConnection,
    PSMB_PACKET               pSmbRequest,
    PSRV_EXEC_CONTEXT_SMB_V2* ppSmb2Context
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

    if (!pExecContext->pProtocolContext->pSmb2Context)
    {
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
                                        pExecContext->pSmbResponse->bufferUsed -
                                        sizeof(NETBIOS_HEADER);

        LWIO_LOG_VERBOSE("Executing command [%s:%d]",
                         SrvGetCommandDescription_SMB_V2(pRequest->pHeader->command),
                         pRequest->pHeader->command);

        switch (pRequest->pHeader->command)
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

        switch (pRequest->pHeader->command)
        {
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

                break;

            case COM2_IOCTL:

                ntStatus = SrvProcessIOCTL_SMB_V2(pExecContext);

                break;

            case COM2_ECHO:

                ntStatus = SrvProcessEcho_SMB_V2(pExecContext);

                break;

            case COM2_FIND:

                ntStatus = SrvProcessFind_SMB_V2(pExecContext);

                break;

            case COM2_GETINFO:

                ntStatus = SrvProcessGetInfo_SMB_V2(pExecContext);

                break;

            case COM2_SETINFO:

                ntStatus = SrvProcessSetInfo_SMB_V2(pExecContext);

                break;

            default:

                ntStatus = STATUS_NOT_SUPPORTED;

                break;
        }

        switch (ntStatus)
        {
            case STATUS_PENDING:

                break;

            case STATUS_SUCCESS:

                break;

            default:

                ntStatus = SrvBuildErrorResponse_SMB_V2(
                                pExecContext,
                                ntStatus,
                                NULL,
                                0);
                break;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (pPrevResponse && pPrevResponse->pHeader)
        {
            pPrevResponse->pHeader->ulChainOffset =
                                    pPrevResponse->ulMessageSize;
        }

        pExecContext->pSmbResponse->bufferUsed += pResponse->ulMessageSize;
    }

    ntStatus = SMB2MarshalFooter(pExecContext->pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

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

    SrvFreeMemory(pProtocolContext);
}

static
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
    ULONG    ulBytesAvailable = pSmbRequest->bufferLen;
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
    ulBytesAvailable = pSmbRequest->bufferLen - sizeof(NETBIOS_HEADER);

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

NTSTATUS
SrvProtocolShutdown_SMB_V2(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V2.mutex);

    gProtocolGlobals_SMB_V2.pWorkQueue = NULL;

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

