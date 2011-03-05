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

#include "includes.h"

static
NTSTATUS
SrvBuildNegotiateResponseForDialect(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN PSMB_PACKET pSmbRequest,
    IN PSTR* ppszDialectArray,
    IN ULONG ulNumDialects,
    OUT PSMB_PACKET* ppSmbResponse
    );

NTSTATUS
SrvProcessNegotiate(
    IN PSRV_EXEC_CONTEXT pExecContext
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONNECTION pConnection = pExecContext->pConnection;
    PSMB_PACKET pSmbRequest = pExecContext->pSmbRequest;
    PSMB_PACKET pSmbResponse = NULL;
    PSTR  pszDialectArray[128];
    ULONG ulNumDialects = 128;
    ULONG ulOffset = 0;

    if (pExecContext->bInline)
    {
        ntStatus = SrvScheduleExecContext(pExecContext);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = STATUS_PENDING;

        goto cleanup;
    }

    ulOffset = (PBYTE)pSmbRequest->pParams - (PBYTE)pSmbRequest->pSMBHeader;

    ntStatus = UnmarshallNegotiateRequest(
                    pSmbRequest->pParams,
                    pSmbRequest->pNetBIOSHeader->len - ulOffset,
                    (uint8_t**)&pszDialectArray,
                    &ulNumDialects);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvBuildNegotiateResponseForDialect(
                    pConnection,
                    pSmbRequest,
                    pszDialectArray,
                    ulNumDialects,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvConnectionSetState(pConnection, LWIO_SRV_CONN_STATE_NEGOTIATE);

    pExecContext->pSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    pExecContext->pSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

static
NTSTATUS
SrvBuildNegotiateResponseForDialect(
    IN PLWIO_SRV_CONNECTION pConnection,
    IN PSMB_PACKET pSmbRequest,
    IN PSTR* ppszDialectArray,
    IN ULONG ulNumDialects,
    OUT PSMB_PACKET* ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG dialectIdx = 0;
    BOOLEAN supportSMBV2 = SrvProtocolConfigIsSmb2Enabled();
    PSMB_PACKET pSmbResponse = NULL;
    SMB_PROTOCOL_VERSION protocolVersion = SMB_PROTOCOL_VERSION_UNKNOWN;
    SMB_PROTOCOL_DIALECT protocolDialect = SMB_PROTOCOL_DIALECT_UNKNOWN;

    for (dialectIdx = ulNumDialects-1; dialectIdx >= 0; dialectIdx--)
    {
        if (supportSMBV2 &&
            LwRtlCStringIsEqual(
                ppszDialectArray[dialectIdx],
                SRV_NEGOTIATE_DIALECT_STRING_SMB_2_1,
                TRUE))
        {
            protocolVersion = SMB_PROTOCOL_VERSION_2;
            protocolDialect = SMB_PROTOCOL_DIALECT_SMB_2_1;
        }
        else if (supportSMBV2 &&
                 LwRtlCStringIsEqual(
                     ppszDialectArray[dialectIdx],
                     SRV_NEGOTIATE_DIALECT_STRING_SMB_2,
                     TRUE))
        {
            protocolVersion = SMB_PROTOCOL_VERSION_2;
            protocolDialect = SMB_PROTOCOL_DIALECT_SMB_2_0;
        }
        else if (LwRtlCStringIsEqual(
                     ppszDialectArray[dialectIdx],
                     SRV_NEGOTIATE_DIALECT_STRING_NTLM_0_12,
                     TRUE))
        {
            protocolVersion = SMB_PROTOCOL_VERSION_1;
            protocolDialect = SMB_PROTOCOL_DIALECT_NTLM_0_12;
        }

        if (protocolVersion != SMB_PROTOCOL_VERSION_UNKNOWN)
        {
            // Found the dialect we want so exit loop
            break;
        }
    }

    switch (protocolVersion)
    {
        case SMB_PROTOCOL_VERSION_UNKNOWN:
            ntStatus = SrvBuildNegotiateResponse_SMB_V1_Invalid(
                           pConnection,
                           pSmbRequest,
                           &pSmbResponse);
            break;

        case SMB_PROTOCOL_VERSION_1:
            ntStatus = SrvBuildNegotiateResponse_SMB_V1_NTLM_0_12(
                           pConnection,
                           pSmbRequest,
                           protocolDialect,
                           dialectIdx,
                           &pSmbResponse);
            break;

        case SMB_PROTOCOL_VERSION_2:
            ntStatus = SrvBuildNegotiateResponse_SMB_V2(
                           pConnection,
                           pSmbRequest,
                           protocolDialect,
                           &pSmbResponse);
            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvConnectionSetProtocolVersion(
                   pConnection,
                   protocolVersion,
                   protocolDialect);
    BAIL_ON_NT_STATUS(ntStatus);

error:
    if (!NT_SUCCESS(ntStatus))
    {
        if (pSmbResponse)
        {
            SMBPacketRelease(pConnection->hPacketAllocator, pSmbResponse);
            pSmbResponse = NULL;
        }
    }

    *ppSmbResponse = pSmbResponse;

    return ntStatus;
}


