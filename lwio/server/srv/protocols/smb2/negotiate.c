/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        negotiate.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols API - SMBV2
 *
 *        Negotiate
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */
#include "includes.h"

static
NTSTATUS
SrvMarshalNegotiateResponse_SMB_V2(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN ULONG SequenceNumber,
    IN USHORT ProtocolDialect,
    IN PBYTE pSecurityToken,
    IN ULONG SecurityTokenLength,
    IN OUT PSRV_MESSAGE_SMB_V2  pSmbResponse
    );

NTSTATUS
SrvProcessNegotiate_SMB_V2(
    IN OUT PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_CONNECTION pConnection = pExecContext->pConnection;
    PSRV_PROTOCOL_EXEC_CONTEXT pCtxProtocol  = pExecContext->pProtocolContext;
    PSRV_EXEC_CONTEXT_SMB_V2   pCtxSmb2      = pCtxProtocol->pSmb2Context;
    ULONG                      iMsg          = pCtxSmb2->iMsg;
    PSRV_MESSAGE_SMB_V2        pSmbRequest   = &pCtxSmb2->pRequests[iMsg];
    PSRV_MESSAGE_SMB_V2        pSmbResponse  = &pCtxSmb2->pResponses[iMsg];
    PSMB2_NEGOTIATE_REQUEST_HEADER pNegotiateRequestHeader = NULL;// Do not free
    PUSHORT pusDialects      = NULL; // Do not free
    USHORT  iDialect         = 0;
    SMB_PROTOCOL_VERSION protocolVersion = SMB_PROTOCOL_VERSION_UNKNOWN;
    SMB_PROTOCOL_DIALECT protocolDialect = SMB_PROTOCOL_DIALECT_UNKNOWN;
    USHORT usDialect = 0;

    if (pExecContext->bInline)
    {
        ntStatus = SrvScheduleExecContext(pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = STATUS_PENDING;

        goto cleanup;
    }

    ntStatus = SMB2UnmarshalNegotiateRequest(
                        pSmbRequest,
                        &pNegotiateRequestHeader,
                        &pusDialects);
    BAIL_ON_NT_STATUS(ntStatus);

    SRV_LOG_DEBUG(
            pExecContext->pLogContext,
            SMB_PROTOCOL_VERSION_2,
            pSmbRequest->pHeader->command,
            "Negotiate request params: "
            "command(%u),uid(%llu),cmd-seq(%llu),pid(%u),tid(%u),"
            "credits(%u),flags(0x%x),chain-offset(%u),"
            "capabilities(0x%x),dialect-count(%u),security-mode(%u)",
            pSmbRequest->pHeader->command,
            (long long)pSmbRequest->pHeader->ullSessionId,
            (long long)pSmbRequest->pHeader->ullCommandSequence,
            pSmbRequest->pHeader->ulPid,
            pSmbRequest->pHeader->ulTid,
            pSmbRequest->pHeader->usCredits,
            pSmbRequest->pHeader->ulFlags,
            pSmbRequest->pHeader->ulChainOffset,
            pNegotiateRequestHeader->ulCapabilities,
            pNegotiateRequestHeader->usDialectCount,
            pNegotiateRequestHeader->usSecurityMode);

    if (!pNegotiateRequestHeader->usDialectCount)
    {
        ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    for (iDialect = pNegotiateRequestHeader->usDialectCount - 1;
         iDialect >= 0;
         iDialect--)
    {
        usDialect = pusDialects[iDialect];

        switch (usDialect)
        {
            case SMB2_NEGOTIATE_DIALECT_V2:
                protocolVersion = SMB_PROTOCOL_VERSION_2;
                protocolDialect = SMB_PROTOCOL_DIALECT_SMB_2_0;
                break;

            case SMB2_NEGOTIATE_DIALECT_V2_1:
                protocolVersion = SMB_PROTOCOL_VERSION_2;
                protocolDialect = SMB_PROTOCOL_DIALECT_SMB_2_1;
                break;
        }

        if (protocolVersion != SMB_PROTOCOL_VERSION_UNKNOWN)
        {
            break;
        }
    }

    if (protocolVersion != SMB_PROTOCOL_VERSION_UNKNOWN)
    {
        PBYTE pNegHintsBlob = NULL; /* Do not free */
        ULONG ulNegHintsLength = 0;

        SRV_LOG_DEBUG(
            pExecContext->pLogContext,
            SMB_PROTOCOL_VERSION_2,
            pSmbRequest->pHeader->command,
            "Negotiate dialect selected: ",
            "command(%u),uid(%llu),cmd-seq(%llu),pid(%u),tid(%u),"
            "credits(%u),flags(0x%x),chain-offset(%u),dialect(0x%x)",
            pSmbRequest->pHeader->command,
            (long long)pSmbRequest->pHeader->ullSessionId,
            (long long)pSmbRequest->pHeader->ullCommandSequence,
            pSmbRequest->pHeader->ulPid,
            pSmbRequest->pHeader->ulTid,
            pSmbRequest->pHeader->usCredits,
            pSmbRequest->pHeader->ulFlags,
            pSmbRequest->pHeader->ulChainOffset,
            usDialect);

        ntStatus = SrvGssNegHints(&pNegHintsBlob, &ulNegHintsLength);

        /* Microsoft clients ignore the security blob on the neg prot response
           so don't fail here if we can't get a negHintsBlob */

        if (ntStatus == STATUS_SUCCESS)
        {
            ntStatus = SrvMarshalNegotiateResponse_SMB_V2(
                            pConnection,
                            pSmbRequest->pHeader->ullCommandSequence,
                            usDialect,
                            pNegHintsBlob,
                            ulNegHintsLength,
                            pSmbResponse);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }
    else
    {
        // TODO: Figure out the right response here
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvConnectionSetProtocolVersion(
                   pExecContext->pConnection,
                   protocolVersion,
                   protocolDialect);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvBuildNegotiateResponse_SMB_V2(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN PSMB_PACKET pSmbRequest,
    IN SMB_PROTOCOL_DIALECT Dialect,
    OUT PSMB_PACKET* ppSmbResponse
    )
{
    NTSTATUS    ntStatus         = STATUS_SUCCESS;
    PSMB_PACKET pSmbResponse     = NULL;
    PBYTE       pNegHintsBlob    = NULL; /* Do not free */
    ULONG       ulNegHintsLength = 0;
    SRV_MESSAGE_SMB_V2 response  = {0};
    USHORT dialect = 0;
    ULONG64 sequenceNumber = 0;

    ntStatus = SrvCreditorAcquireCredit(pConnection->pCreditor, 0);
    BAIL_ON_NT_STATUS(ntStatus);

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

    ntStatus = SMB2InitPacket(pSmbResponse, FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvGssNegHints(&pNegHintsBlob, &ulNegHintsLength);

    switch (Dialect)
    {
        case SMB_PROTOCOL_DIALECT_SMB_2_0:
            dialect = SMB_NEGOTIATE_RESPONSE_DIALECT_V2_0;
            break;

        case SMB_PROTOCOL_DIALECT_SMB_2_1:
            dialect = SMB_NEGOTIATE_RESPONSE_DIALECT_V2_1;
            break;

        default:
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Microsoft clients ignore the security blob on the neg prot response
       so don't fail here if we can't get a negHintsBlob */

    if (ntStatus == STATUS_SUCCESS)
    {
        response.pBuffer = pSmbResponse->pRawBuffer + pSmbResponse->bufferUsed;
        response.ulBytesAvailable =
                        pSmbResponse->bufferLen - pSmbResponse->bufferUsed;

        ntStatus = SrvMarshalNegotiateResponse_SMB_V2(
                        pConnection,
                        sequenceNumber,
                        dialect,
                        pNegHintsBlob,
                        ulNegHintsLength,
                        &response);
        BAIL_ON_NT_STATUS(ntStatus);

        pSmbResponse->bufferUsed += response.ulMessageSize;

        ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    }
    else
    {
        ntStatus = STATUS_NOT_SUPPORTED;
    }
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

static
NTSTATUS
SrvMarshalNegotiateResponse_SMB_V2(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN ULONG SequenceNumber,
    IN USHORT ProtocolDialect,
    IN PBYTE pSecurityToken,
    IN ULONG SecurityTokenLength,
    IN OUT PSRV_MESSAGE_SMB_V2  pSmbResponse
    )
{
    NTSTATUS ntStatus         = STATUS_SUCCESS;
    PSMB2_NEGOTIATE_RESPONSE_HEADER pNegotiateHeader = NULL; // Do not free
    PSRV_PROPERTIES pServerProperties = &pConnection->serverProperties;
    PBYTE  pDataCursor      = NULL;
    PBYTE  pOutBuffer       = pSmbResponse->pBuffer;
    ULONG  ulBytesAvailable = pSmbResponse->ulBytesAvailable;
    ULONG  ulOffset         = 0;
    ULONG  ulBytesUsed      = 0;
    ULONG  ulTotalBytesUsed = 0;
    LONG64 llCurTime        = 0LL;
    USHORT usCreditsGranted = 0;

    ntStatus = SrvCreditorAdjustCredits(
                    pConnection->pCreditor,
                    SequenceNumber,
                    0,
                    1,
                    &usCreditsGranted);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMB2MarshalHeader(
                pOutBuffer,
                ulOffset,
                ulBytesAvailable,
                COM2_NEGOTIATE,
                1, /* usEpoch      */
                usCreditsGranted,
                0, /* usPid        */
                0, /* ullMid       */
                0, /* usTid        */
                0, /* ullSessionId */
                0, /* Async Id     */
                0, /* status       */
                TRUE, /* response */
                FALSE,
                &pSmbResponse->pHeader,
                &pSmbResponse->ulHeaderSize);
    BAIL_ON_NT_STATUS(ntStatus);

    pOutBuffer       += pSmbResponse->ulHeaderSize;
    ulOffset         += pSmbResponse->ulHeaderSize;
    ulBytesAvailable -= pSmbResponse->ulHeaderSize;
    ulTotalBytesUsed += pSmbResponse->ulHeaderSize;

    if (ulBytesAvailable < sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER))
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pNegotiateHeader = (PSMB2_NEGOTIATE_RESPONSE_HEADER)pOutBuffer;

    ulBytesUsed       = sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);
    pOutBuffer       += sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);
    ulOffset         += sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);
    ulBytesAvailable -= sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);
    ulTotalBytesUsed += sizeof(SMB2_NEGOTIATE_RESPONSE_HEADER);

    pNegotiateHeader->usDialect = ProtocolDialect;

    pNegotiateHeader->ucFlags = 0;

    // Always set the "Signing Supported" flag for SMbv2

    pNegotiateHeader->ucFlags |= SMB2_NEGOTIATE_SECURITY_FLAG_SIGNING_ENABLED;

    if (pServerProperties->bRequireSecuritySignatures)
    {
        pNegotiateHeader->ucFlags |= SMB2_NEGOTIATE_SECURITY_FLAG_SIGNING_REQUIRED;
    }

    pNegotiateHeader->ulMaxReadSize = pServerProperties->MaxBufferSize;
    pNegotiateHeader->ulMaxWriteSize = pServerProperties->MaxBufferSize;
    pNegotiateHeader->ulMaxTxSize = pServerProperties->MaxBufferSize;

    pNegotiateHeader->ulCapabilities = 0;

    switch (ProtocolDialect)
    {
        case SMB_NEGOTIATE_RESPONSE_DIALECT_V2_1:
        case SMB2_NEGOTIATE_DIALECT_V2_1:
            // pNegotiateHeader->ulCapabilities |= SMB2_NEGOTIATE_CAPABILITY_FLAG_LEASING;
            break;
    }


    ntStatus = WireGetCurrentNTTime(&llCurTime);
    BAIL_ON_NT_STATUS(ntStatus);

    pNegotiateHeader->ullCurrentTime = llCurTime;

    ntStatus = SrvElementsGetBootTime(&pNegotiateHeader->ullBootTime);
    BAIL_ON_NT_STATUS(ntStatus);

    memcpy(&pNegotiateHeader->serverGUID[0],
            pServerProperties->GUID,
            sizeof(pServerProperties->GUID));

    if (ulOffset % 8)
    {
        USHORT usAlign = (8 - (ulOffset % 8));

        if (ulBytesAvailable < usAlign)
        {
            ntStatus = STATUS_INVALID_BUFFER_SIZE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pOutBuffer       += usAlign;
        ulBytesUsed      += usAlign;
        ulOffset         += usAlign;
        ulBytesAvailable -= usAlign;
        ulTotalBytesUsed += usAlign;
    }

    pNegotiateHeader->usBlobOffset = ulOffset;
    pNegotiateHeader->usBlobLength = SecurityTokenLength;

    if (ulBytesAvailable < SecurityTokenLength)
    {
        ntStatus = STATUS_INVALID_BUFFER_SIZE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pDataCursor = pSmbResponse->pBuffer + ulOffset;

    memcpy(pDataCursor, pSecurityToken, SecurityTokenLength);

    pNegotiateHeader->usLength = ulBytesUsed + 1; /* add one for dynamic part */

    ulTotalBytesUsed += SecurityTokenLength;
    // ulBytesUsed += ulSessionKeyLength;

    pSmbResponse->ulMessageSize = ulTotalBytesUsed;

cleanup:

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
