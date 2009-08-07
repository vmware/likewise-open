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
 *        session_setup.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Session Setup
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */
#include "includes.h"

NTSTATUS
SrvProcessSessionSetup_SMB_V2(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PSMB2_SESSION_SETUP_REQUEST_HEADER pSessionSetupHeader = NULL;// Do not free
    PBYTE       pSecurityBlob = NULL; // Do not free
    ULONG       ulSecurityBlobLen = 0;
    PBYTE       pReplySecurityBlob = NULL;
    ULONG       ulReplySecurityBlobLength = 0;
    UNICODE_STRING uniUsername = {0};
    PBYTE pOutBuffer = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset    = 0;
    ULONG ulBytesUsed = 0;
    ULONG ulTotalBytesUsed = 0;

    if (pCtxSmb2->pSession)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMB2UnmarshallSessionSetup(
                    pSmbRequest,
                    &pSessionSetupHeader,
                    &pSecurityBlob,
                    &ulSecurityBlobLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGssNegotiate(
                    pConnection->hGssContext,
                    pConnection->hGssNegotiate,
                    pSecurityBlob,
                    ulSecurityBlobLen,
                    &pReplySecurityBlob,
                    &ulReplySecurityBlobLength);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM2_SESSION_SETUP,
                    0,
                    9,
                    pSmbRequest->pHeader->ulPid,
                    pSmbRequest->pHeader->ullCommandSequence,
                    pSmbRequest->pHeader->ulTid,
                    0LL,
                    STATUS_SUCCESS,
                    TRUE,
                    pSmbRequest->pHeader->ulFlags & SMB2_FLAGS_RELATED_OPERATION,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (!SrvGssNegotiateIsComplete(pConnection->hGssContext,
                                   pConnection->hGssNegotiate))
    {
        pSmbResponse->pHeader->error = STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        ntStatus = SrvConnection2CreateSession(
                        pConnection,
                        &pCtxSmb2->pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pConnection->pSessionKey)
        {
             ntStatus = SrvGssGetSessionDetails(
                             pConnection->hGssContext,
                             pConnection->hGssNegotiate,
                             &pConnection->pSessionKey,
                             &pConnection->ulSessionKeyLength,
                             &pCtxSmb2->pSession->pszClientPrincipalName);
        }
        else
        {
             ntStatus = SrvGssGetSessionDetails(
                             pConnection->hGssContext,
                             pConnection->hGssNegotiate,
                             NULL,
                             NULL,
                             &pCtxSmb2->pSession->pszClientPrincipalName);
        }
        BAIL_ON_NT_STATUS(ntStatus);

        /* Generate and store the IoSecurityContext */

        ntStatus = RtlUnicodeStringAllocateFromCString(
                       &uniUsername,
                       pCtxSmb2->pSession->pszClientPrincipalName);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = IoSecurityCreateSecurityContextFromUsername(
                       &pCtxSmb2->pSession->pIoSecurityContext,
                       &uniUsername);
        BAIL_ON_NT_STATUS(ntStatus);

        pSmbResponse->pHeader->ullSessionId = pCtxSmb2->pSession->ullUid;

        SrvConnectionSetState(pConnection, LWIO_SRV_CONN_STATE_READY);
    }

    ntStatus = SMB2MarshalSessionSetup(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    0,
                    pReplySecurityBlob,
                    ulReplySecurityBlobLength,
                    &ulBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    // pOutBuffer       += ulBytesUsed;
    // ulOffset         += ulBytesUsed;
    // ulBytesAvailable -= ulBytesUsed;
    ulTotalBytesUsed += ulBytesUsed;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

    RtlUnicodeStringFree(&uniUsername);

    SRV_SAFE_FREE_MEMORY(pReplySecurityBlob);

    return ntStatus;

error:

    if (ulTotalBytesUsed)
    {
        pSmbResponse->pHeader      = NULL;
        pSmbResponse->ulHeaderSize = 0;
        memset(pSmbResponse->pBuffer, 0, ulTotalBytesUsed);
    }

    pSmbResponse->ulMessageSize = 0;

    goto cleanup;
}
