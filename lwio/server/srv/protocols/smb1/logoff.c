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

NTSTATUS
SrvProcessLogoffAndX(
    PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS                   ntStatus        = 0;
    PLWIO_SRV_CONNECTION       pConnection     = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol    = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V1   pCtxSmb1        = pCtxProtocol->pSmb1Context;
    ULONG                      iMsg            = pCtxSmb1->iMsg;
    PSRV_MESSAGE_SMB_V1        pSmbRequest     = &pCtxSmb1->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V1        pSmbResponse    = &pCtxSmb1->pResponses[iMsg];
    PLOGOFF_RESPONSE_HEADER    pResponseHeader = NULL; // Do not free
    PBYTE pOutBuffer       = pSmbResponse->pBuffer;
    ULONG ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG ulOffset         = 0;
    ULONG ulTotalBytesUsed = 0;

    ntStatus = SrvConnectionRemoveSession(
                    pConnection,
                    pSmbRequest->pHeader->uid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvMarshalHeader_SMB_V1(
                    pOutBuffer,
                    ulOffset,
                    ulBytesAvailable,
                    COM_LOGOFF_ANDX,
                    STATUS_SUCCESS,
                    TRUE,
                    pSmbRequest->pHeader->tid,
                    pSmbRequest->pHeader->pid,
                    pSmbRequest->pHeader->uid,
                    pSmbRequest->pHeader->mid,
                    pConnection->serverProperties.bRequireSecuritySignatures,
                    &pSmbResponse->pHeader,
                    &pSmbResponse->pAndXHeader,
                    &pSmbResponse->usHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->usHeaderSize;
    ulOffset         += pSmbResponse->usHeaderSize;
    ulBytesAvailable -= pSmbResponse->usHeaderSize;
    ulTotalBytesUsed += pSmbResponse->usHeaderSize;

    pSmbResponse->pHeader->wordCount = 2;

    if (ulBytesAvailable < sizeof(LOGOFF_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponseHeader = (PLOGOFF_RESPONSE_HEADER)pOutBuffer;

    // pOutBuffer       += sizeof(LOGOFF_RESPONSE_HEADER);
    // ulOffset         += sizeof(LOGOFF_RESPONSE_HEADER);
    // ulBytesAvailable -= sizeof(LOGOFF_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(LOGOFF_RESPONSE_HEADER);

    pResponseHeader->byteCount = 0;

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



