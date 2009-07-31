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

NTSTATUS
SrvProtocolInit_SMB_V1(
    PSMB_PROD_CONS_QUEUE pAsyncWorkQueue
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    gProtocolGlobals_SMB_V1.pAsyncWorkQueue = pAsyncWorkQueue;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    return status;
}

NTSTATUS
SrvProtocolExecute_SMB_V1(
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

            // Handled at a higher layer
            ntStatus = STATUS_INTERNAL_ERROR;

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

    switch (pSmbRequest->pSMBHeader->command)
    {
        case COM_SESSION_SETUP_ANDX:

            ntStatus = SrvProcessSessionSetup(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_TREE_CONNECT_ANDX:

            ntStatus = SrvProcessTreeConnectAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_OPEN_ANDX:

            ntStatus = SrvProcessOpenAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_NT_CREATE_ANDX:

            ntStatus = SrvProcessNTCreateAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_LOCKING_ANDX:

            ntStatus = SrvProcessLockAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_READ:

            ntStatus = SrvProcessRead(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_READ_ANDX:

            ntStatus = SrvProcessReadAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_WRITE:

            ntStatus = SrvProcessWrite(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_WRITE_ANDX:

            ntStatus = SrvProcessWriteAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_TRANSACTION:

            ntStatus = SrvProcessTransaction(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_TRANSACTION2:

            ntStatus = SrvProcessTransaction2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_FIND_CLOSE2:

            ntStatus = SrvProcessFindClose2(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_CLOSE:

            ntStatus = SrvProcessCloseAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_CREATE_DIRECTORY:

            ntStatus = SrvProcessCreateDirectory(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_DELETE_DIRECTORY:

            ntStatus = SrvProcessDeleteDirectory(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_DELETE:

            ntStatus = SrvProcessDelete(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_RENAME:

            ntStatus = SrvProcessRename(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_NT_TRANSACT:

            ntStatus = SrvProcessNtTransact(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_TREE_DISCONNECT:

            ntStatus = SrvProcessTreeDisconnectAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_ECHO:

            ntStatus = SrvProcessEchoAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_FLUSH:

            ntStatus = SrvProcessFlush(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_LOGOFF_ANDX:

            ntStatus = SrvProcessLogoffAndX(
                            pConnection,
                            pSmbRequest,
                            &pSmbResponse);

            break;

        case COM_CHECK_DIRECTORY:

            ntStatus = SrvProcessCheckDirectory(
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
        ntStatus = SrvProtocolBuildErrorResponse(
                        pConnection,
                        pSmbRequest->pSMBHeader->command,
                        pSmbRequest->pSMBHeader->tid,
                        pSmbRequest->pSMBHeader->pid,
                        pSmbRequest->pSMBHeader->uid,
                        pSmbRequest->pSMBHeader->mid,
                        ntStatus,
                        &pSmbResponse);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pSmbResponse) {
        pSmbResponse->sequence = pSmbRequest->sequence + 1;
    }

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
SrvProtocolBuildErrorResponse(
    PLWIO_SRV_CONNECTION pConnection,
    BYTE                 ucCommand,
    USHORT               usTid,
    USHORT               usPid,
    USHORT               usUid,
    USHORT               usMid,
    NTSTATUS             errorStatus,
    PSMB_PACKET*         ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PERROR_RESPONSE_HEADER pResponseHeader = NULL; // Do not free
    ULONG       ulParamBytesUsed = 0;

    ntStatus = SMBPacketAllocate(
                    pConnection->hPacketAllocator,
                    &pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketBufferAllocate(
                    pConnection->hPacketAllocator,
                    64 * 1024,
                    &pSmbResponse->pRawBuffer,
                    &pSmbResponse->bufferLen);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBPacketMarshallHeader(
                pSmbResponse->pRawBuffer,
                pSmbResponse->bufferLen,
                ucCommand,
                errorStatus,
                TRUE,
                usTid,
                usPid,
                usUid,
                usMid,
                pConnection->serverProperties.bRequireSecuritySignatures,
                pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->pSMBHeader->wordCount = 0;

    ntStatus = WireMarshallErrorResponse(
                    pSmbResponse->pParams,
                    pSmbResponse->bufferLen - pSmbResponse->bufferUsed,
                    (PBYTE)pSmbResponse->pParams - (PBYTE)pSmbResponse->pSMBHeader,
                    &pResponseHeader,
                    &ulParamBytesUsed);
    BAIL_ON_NT_STATUS(ntStatus);

    pSmbResponse->bufferUsed += (USHORT)ulParamBytesUsed;

    pResponseHeader->byteCount = 0;

    ntStatus = SMBPacketMarshallFooter(pSmbResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSmbResponse = pSmbResponse;

cleanup:

    return ntStatus;

error:

    *ppSmbResponse = NULL;

    if (pSmbResponse)
    {
        SMBPacketRelease(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}

NTSTATUS
SrvProtocolShutdown_SMB_V1(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    gProtocolGlobals_SMB_V1.pAsyncWorkQueue = NULL;

    LWIO_UNLOCK_MUTEX(bInLock, &gProtocolGlobals_SMB_V1.mutex);

    return status;
}

