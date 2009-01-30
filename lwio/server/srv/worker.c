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

NTSTATUS
SrvWorkerFreeContents(
    PSMB_SRV_WORKER pWorker
    )
{
    NTSTATUS ntStatus = 0;

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

    return ntStatus;
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

    switch (pContext->pRequest->pSMBHeader->command)
    {
        case COM_NEGOTIATE:

                ntStatus = SrvProcessNegotiate(pContext);

                break;

        case COM_SESSION_SETUP_ANDX:

            ntStatus = SrvProcessSessionSetup(pContext);

            break;

        case COM_TREE_CONNECT_ANDX:

            ntStatus = SrvProcessTreeConnectAndX(pContext);

            break;

        case COM_NT_CREATE_ANDX:

            ntStatus = SrvProcessNTCreateAndX(pContext);

            break;

        case COM_READ_ANDX:

            ntStatus = SrvProcessReadAndX(pContext);

            break;

        case COM_WRITE_ANDX:

            ntStatus = SrvProcessWriteAndX(pContext);

            break;

        case COM_TRANSACTION2:

            ntStatus = SrvProcessTransaction2(pContext);

            break;

        case COM_CLOSE:

            ntStatus = SrvProcessCloseAndX(pContext);

            break;

        case COM_TREE_DISCONNECT:

            ntStatus = SrvProcessTreeDisconnectAndX(pContext);

            break;

        case COM_ECHO:

            ntStatus = SrvProcessEchoAndX(pContext);

            break;

        case COM_LOGOFF_ANDX:

            ntStatus = SrvProcessLogoffAndX(pContext);

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

        case SMB_LOCKING_ANDX:

            ntStatus = SmbProcessLockingAndX(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_SEEK:

            ntStatus = SmbProcessSeek(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_FLUSH:

            ntStatus = SmbProcessFlush(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CLOSE_AND_TREE_DISCONNECT:

            ntStatus = SmbProcessCloseAndTreeDisconnect(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_DELETE:

            ntStatus = SmbProcessDelete(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_RENAME:

            ntStatus = SmbProcessRename(
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

        case  SMB_TRANS2_CREATE_DIRECTORY:

            ntStatus = SmbProcessTrans2CreateDirectory(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_DELETE_DIRECTORY:

            ntStatus = SmbProcessDeleteDirectory(
                            pSmbRequest,
                            pSmbResponse);

            break;

        case SMB_CHECK_DIRECTORY:

            ntStatus = SmbProcessCheckDirectory(
                            pSmbRequest,
                            pSmbResponse);

            break;
#endif

    }

    return ntStatus;
}

