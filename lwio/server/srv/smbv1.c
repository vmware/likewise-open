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
SMBSrvProcessRequest_V1(
    PLWIO_SRV_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PLWIO_SRV_CONNECTION pConnection = pContext->pConnection;

    switch (pContext->pRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

            if (SrvConnectionGetState(pConnection) != LWIO_SRV_CONN_STATE_INITIAL)
            {
                ntStatus = STATUS_INVALID_SERVER_STATE;
            }

            break;

        case COM_SESSION_SETUP_ANDX:

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

    switch (pContext->pRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

                ntStatus = SrvProcessNegotiate(
                                pContext,
                                &pSmbResponse);

                break;

        case COM_SESSION_SETUP_ANDX:

            ntStatus = SrvProcessSessionSetup(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_TREE_CONNECT_ANDX:

            ntStatus = SrvProcessTreeConnectAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_OPEN_ANDX:

            ntStatus = SrvProcessOpenAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_NT_CREATE_ANDX:

            ntStatus = SrvProcessNTCreateAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_LOCKING_ANDX:

            ntStatus = SrvProcessLockAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_READ:

            ntStatus = SrvProcessRead(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_READ_ANDX:

            ntStatus = SrvProcessReadAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_WRITE:

            ntStatus = SrvProcessWrite(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_WRITE_ANDX:

            ntStatus = SrvProcessWriteAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_TRANSACTION:

            ntStatus = SrvProcessTransaction(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_TRANSACTION2:

            ntStatus = SrvProcessTransaction2(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_FIND_CLOSE2:

            ntStatus = SrvProcessFindClose2(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_CLOSE:

            ntStatus = SrvProcessCloseAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_CREATE_DIRECTORY:

            ntStatus = SrvProcessCreateDirectory(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_DELETE_DIRECTORY:

            ntStatus = SrvProcessDeleteDirectory(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_DELETE:

            ntStatus = SrvProcessDelete(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_RENAME:

            ntStatus = SrvProcessRename(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_NT_TRANSACT:

            ntStatus = SrvProcessNtTransact(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_TREE_DISCONNECT:

            ntStatus = SrvProcessTreeDisconnectAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_ECHO:

            ntStatus = SrvProcessEchoAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_FLUSH:

            ntStatus = SrvProcessFlush(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_LOGOFF_ANDX:

            ntStatus = SrvProcessLogoffAndX(
                            pContext,
                            &pSmbResponse);

            break;

        case COM_CHECK_DIRECTORY:

            ntStatus = SrvProcessCheckDirectory(
                            pContext,
                            &pSmbResponse);

            break;

#if 0

        case SMB_NT_CANCEL:

            ntStatus = SmbNTCancel(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_NT_TRANSACT_CREATE:

            ntStatus = SmbNTTransactCreate(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CREATE_TEMPORARY:

            ntStatus = SmbCreateTemporary(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_SEEK:

            ntStatus = SmbProcessSeek(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CLOSE_AND_TREE_DISCONNECT:

            ntStatus = SmbProcessCloseAndTreeDisconnect(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_NT_RENAME:

            ntStatus = SmbProcessNTRename(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_MOVE:

            ntStatus = SmbProcessCopy(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_COPY:

            ntStatus = SmbProcessCopy(
                            pSmbRequest,
                            pSmbResponse);

            break;
#endif

        default:

            ntStatus = STATUS_NOT_IMPLEMENTED;

            break;
    }

    if (ntStatus)
    {
        ntStatus = SrvWorkerBuildErrorResponse(
                        pContext,
                        ntStatus,
                        &pSmbResponse);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvConnectionWriteMessage(
                    pContext->pConnection,
                    pContext->ulRequestSequence+1,
                    pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pSmbResponse)
    {
        SMBPacketFree(
            pContext->pConnection->hPacketAllocator,
            pSmbResponse);
    }

    return ntStatus;

error:

    goto cleanup;
}

