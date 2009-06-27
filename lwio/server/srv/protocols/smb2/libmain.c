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

NTSTATUS
SrvProtocolInit_SMB_V2(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;

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
    PSMB_PACKET pSmbResponse = NULL;

    switch (pSmbRequest->pSMB2Header->command)
    {
        case COM2_NEGOTIATE:

            // Handled at a higher layer
            ntStatus = STATUS_INTERNAL_ERROR;

            break;

        case COM2_ECHO:
        case COM2_SESSION_SETUP:

            {
                LWIO_SRV_CONN_STATE connState = SrvConnectionGetState(pConnection);

                if ((connState != LWIO_SRV_CONN_STATE_NEGOTIATE) &&
                    (connState != LWIO_SRV_CONN_STATE_READY))
                {
                    ntStatus = STATUS_INVALID_SERVER_STATE;
                }
            }

            break;

        default:

            if (SrvConnectionGetState(pConnection) != LWIO_SRV_CONN_STATE_READY)
            {
                ntStatus = STATUS_INVALID_SERVER_STATE;
            }

            break;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pSmbRequest->pSMB2Header->command)
    {
        case COM2_SESSION_SETUP:

            ntStatus = SrvProcessSessionSetup_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM2_LOGOFF:

            ntStatus = SrvProcessLogoff_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM2_TREE_CONNECT:

            ntStatus = SrvProcessTreeConnect_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM2_TREE_DISCONNECT:

            ntStatus = SrvProcessTreeDisconnect_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM2_CREATE:

            ntStatus = SrvProcessCreate_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM2_CLOSE:

            ntStatus = SrvProcessClose_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM2_FLUSH:

            ntStatus = SrvProcessFlush_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM2_WRITE:

            ntStatus = SrvProcessWrite_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM2_ECHO:

            ntStatus = SrvProcessEcho_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM2_GETINFO:

            ntStatus = SrvProcessGetInfo_SMB_V2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        default:

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;
    }

    if (ntStatus)
    {
        ntStatus = SrvBuildErrorResponse_SMB_V2(
                        pConnection,
                        pSmbRequest->pSMB2Header,
                        ntStatus,
                        &pSmbResponse);
        BAIL_ON_NT_STATUS(ntStatus);
    }

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
SrvProtocolShutdown_SMB_V2(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    return status;
}

