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

static
PVOID
SrvWorkerMain(
    PVOID pData
    );

static
BOOLEAN
SrvWorkerMustStop(
    PSMB_SRV_WORKER_CONTEXT pContext
    );

static
NTSTATUS
SrvWorkerStop(
    PSMB_SRV_WORKER_CONTEXT pContext
    );

static
NTSTATUS
SrvWorkerExecute(
    PLWIO_SRV_CONTEXT pContext
    );

static
NTSTATUS
SrvWorkerBuildErrorResponse(
    PLWIO_SRV_CONTEXT pContext,
    NTSTATUS          errorStatus,
    PSMB_PACKET*      ppSmbResponse
    );

NTSTATUS
SrvWorkerInit(
    PSMB_PROD_CONS_QUEUE pWorkQueue,
    PSMB_SRV_WORKER pWorker
    )
{
    NTSTATUS ntStatus = 0;

    memset(&pWorker->context, 0, sizeof(pWorker->context));

    pthread_mutex_init(&pWorker->context.mutex, NULL);
    pWorker->context.pMutex = &pWorker->context.mutex;

    pWorker->context.bStop = FALSE;
    pWorker->context.pWorkQueue = pWorkQueue;
    pWorker->context.workerId = pWorker->workerId;

    ntStatus = pthread_create(
                    &pWorker->worker,
                    NULL,
                    &SrvWorkerMain,
                    &pWorker->context);
    BAIL_ON_NT_STATUS(ntStatus);

    pWorker->pWorker = &pWorker->worker;

error:

    return ntStatus;
}

static
PVOID
SrvWorkerMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_WORKER_CONTEXT pContext = (PSMB_SRV_WORKER_CONTEXT)pData;
    PLWIO_SRV_CONTEXT pIOContext = NULL;
    struct timespec ts = {0, 0};

    SMB_LOG_DEBUG("Srv worker [id:%u] starting", pContext->workerId);

    while (!SrvWorkerMustStop(pContext))
    {
        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

        if (pIOContext)
        {
            SrvContextFree(pIOContext);
            pIOContext = NULL;
        }

        ntStatus = SrvProdConsTimedDequeue(
                        pContext->pWorkQueue,
                        &ts,
                        (PVOID*)&pIOContext);
        if (ntStatus == STATUS_IO_TIMEOUT)
        {
            ntStatus = 0;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (SrvWorkerMustStop(pContext))
        {
            break;
        }

        if (pIOContext && pIOContext->pRequest)
        {
            NTSTATUS ntStatus2 = SrvWorkerExecute(pIOContext);
            if (ntStatus2)
            {
                SMB_LOG_ERROR("Failed to execute server task [code:%d]", ntStatus2);
            }
        }
    }

cleanup:

    if (pIOContext)
    {
        SrvContextFree(pIOContext);
    }

    SMB_LOG_DEBUG("Srv worker [id:%u] stopping", pContext->workerId);

    return NULL;

error:

    goto cleanup;
}

VOID
SrvWorkerIndicateStop(
    PSMB_SRV_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        SrvWorkerStop(&pWorker->context);
    }
}

VOID
SrvWorkerFreeContents(
    PSMB_SRV_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        SrvWorkerStop(&pWorker->context);

        pthread_join(pWorker->worker, NULL);
    }

    if (pWorker->context.pMutex)
    {
        pthread_mutex_destroy(pWorker->context.pMutex);
        pWorker->context.pMutex = NULL;
    }

    // Don't free items in the work queue which will be
    // deleted by the owner of the queue
}

static
BOOLEAN
SrvWorkerMustStop(
    PSMB_SRV_WORKER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pContext->mutex);

    bStop = pContext->bStop;

    pthread_mutex_unlock(&pContext->mutex);

    return bStop;
}

static
NTSTATUS
SrvWorkerStop(
    PSMB_SRV_WORKER_CONTEXT pContext
    )
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_mutex_unlock(&pContext->mutex);

    return 0;
}

static
NTSTATUS
SrvWorkerExecute(
    PLWIO_SRV_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_PACKET pSmbResponse = NULL;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;

    switch (pContext->pRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

            if (SrvConnectionGetState(pConnection) != SMB_SRV_CONN_STATE_INITIAL)
            {
                ntStatus = STATUS_INVALID_SERVER_STATE;
            }

            break;

        case COM_SESSION_SETUP_ANDX:

            {
                SMB_SRV_CONN_STATE connState = SrvConnectionGetState(pConnection);

                if ((connState != SMB_SRV_CONN_STATE_NEGOTIATE) &&
                    (connState != SMB_SRV_CONN_STATE_READY))
                {
                    ntStatus = STATUS_INVALID_SERVER_STATE;
                }
            }

            break;

        default:

            if (SrvConnectionGetState(pConnection) != SMB_SRV_CONN_STATE_READY)
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

        case SMB_CHECK_DIRECTORY:

            ntStatus = SmbProcessCheckDirectory(
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

static
NTSTATUS
SrvWorkerBuildErrorResponse(
    PLWIO_SRV_CONTEXT pContext,
    NTSTATUS          errorStatus,
    PSMB_PACKET*      ppSmbResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_CONNECTION pConnection = pContext->pConnection;
    PSMB_PACKET pSmbRequest = pContext->pRequest;
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
                pSmbRequest->pSMBHeader->command,
                errorStatus,
                TRUE,
                pSmbRequest->pSMBHeader->tid,
                pSmbRequest->pSMBHeader->pid,
                pSmbRequest->pSMBHeader->uid,
                pSmbRequest->pSMBHeader->mid,
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
        SMBPacketFree(
            pConnection->hPacketAllocator,
            pSmbResponse);
    }

    goto cleanup;
}
