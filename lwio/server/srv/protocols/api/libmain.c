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
 *        Protocols API
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
SrvProtocolExecute_SMB_V1_Filter(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    );

NTSTATUS
SrvProtocolInit(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bSupportSMBV2 = FALSE;

    status = SrvProtocolConfigSupports_SMB_V2(&bSupportSMBV2);
    BAIL_ON_NT_STATUS(status);

    status = SrvProtocolInit_SMB_V1();
    BAIL_ON_NT_STATUS(status);

    if (bSupportSMBV2)
    {
        status = SrvProtocolInit_SMB_V2();
        BAIL_ON_NT_STATUS(status);
    }

error:

    return status;
}

NTSTATUS
SrvProtocolExecute(
    IN  PLWIO_SRV_CONNECTION pConnection,
    IN  PSMB_PACKET          pSmbRequest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_PACKET pSmbResponse = NULL;

    switch (pSmbRequest->packetType)
    {
        case SMB_PACKET_TYPE_SMB_1:

            ntStatus = SrvProtocolExecute_SMB_V1_Filter(
                                pConnection,
                                pSmbRequest,
                                &pSmbResponse);

            break;

        case SMB_PACKET_TYPE_SMB_2:

            ntStatus = SrvProtocolExecute_SMB_V2(
                                pConnection,
                                pSmbRequest,
                                &pSmbResponse);

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (pSmbResponse)
    {
        /* synchronous response */
        ntStatus = SrvTransportSendResponse(
                        pConnection,
                        pSmbResponse);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (pSmbResponse)
    {
        SMBPacketFree(pConnection->hPacketAllocator, pSmbResponse);
    }

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvProtocolExecute_SMB_V1_Filter(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pSmbRequest,
    PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_PACKET pSmbResponse = NULL;

    switch (pSmbRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

            if (SrvConnectionGetState(pConnection) != LWIO_SRV_CONN_STATE_INITIAL)
            {
                ntStatus = STATUS_INVALID_SERVER_STATE;
            }

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pSmbRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

                ntStatus = SrvProcessNegotiate(
                                pConnection,
                                pSmbRequest,
                                &pSmbResponse);

                if (ntStatus)
                {
                    ntStatus = SrvProtocolBuildErrorResponse(
                                    pConnection,
                                    pSmbRequest->pSMBHeader->command,
                                    pSmbRequest->pSMBHeader->tid,
                                    pSmbRequest->pSMBHeader->pid,
                                    pSmbRequest->pSMBHeader->uid,
                                    pSmbRequest->pSMBHeader->mid,
                                    ntStatus,
                                    &pSmbResponse);
                }

                break;

        default:

                ntStatus = SrvProtocolExecute_SMB_V1(
                                pConnection,
                                pSmbRequest,
                                &pSmbResponse);

                break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketFree(pConnection->hPacketAllocator, pSmbResponse);
    }

    goto cleanup;
}

NTSTATUS
SrvProtocolShutdown(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bSupportSMBV2 = FALSE;

    status = SrvProtocolConfigSupports_SMB_V2(&bSupportSMBV2);
    BAIL_ON_NT_STATUS(status);

    status = SrvProtocolShutdown_SMB_V1();
    BAIL_ON_NT_STATUS(status);

    if (bSupportSMBV2)
    {
        status = SrvProtocolInit_SMB_V2();
        BAIL_ON_NT_STATUS(status);
    }

error:

    return status;
}

