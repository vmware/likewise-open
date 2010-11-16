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
NTSTATUS
SrvGetNextExecutionContext(
    struct timespec*   pTimespec,
    PSRV_EXEC_CONTEXT* ppContext
    );

static
BOOLEAN
SrvWorkerMustStop(
    PLWIO_SRV_WORKER_CONTEXT pContext
    );

static
VOID
SrvWorkerIndicateStopContext(
    PLWIO_SRV_WORKER_CONTEXT pContext
    );

NTSTATUS
SrvWorkerInit(
    PLWIO_SRV_WORKER pWorker,
    ULONG            ulCpu
    )
{
    NTSTATUS ntStatus = 0;
    LONG lError = 0;
    pthread_attr_t threadAttr;
    pthread_attr_t* pThreadAttr = NULL;

    memset(&pWorker->context, 0, sizeof(pWorker->context));

    pthread_mutex_init(&pWorker->context.mutex, NULL);
    pWorker->context.pMutex = &pWorker->context.mutex;

    pWorker->context.bStop = FALSE;
    pWorker->context.workerId = pWorker->workerId;

    // Create threadAttr with affinity to ulCpu, ignoring errors
    lError = pthread_attr_init(&threadAttr);
    ntStatus = LwErrnoToNtStatus(lError);
    if (!NT_SUCCESS(ntStatus))
    {
        LWIO_LOG_ERROR("Error initializing pthread_attr_t");
        ntStatus = STATUS_SUCCESS;
    }
    else
    {
        ntStatus = LwRtlSetAffinityThreadAttribute(&threadAttr, ulCpu);
        if (!NT_SUCCESS(ntStatus))
        {
            LWIO_LOG_ERROR("Error setting thread affinity");
            ntStatus = STATUS_SUCCESS;
        }

        pThreadAttr = &threadAttr;
    }

    ntStatus = pthread_create(
                    &pWorker->worker,
                    pThreadAttr,
                    &SrvWorkerMain,
                    &pWorker->context);
    BAIL_ON_NT_STATUS(ntStatus);

    pWorker->pWorker = &pWorker->worker;

error:

    if (pThreadAttr)
    {
        pthread_attr_destroy(&threadAttr);
    }

    return ntStatus;
}

static
PVOID
SrvWorkerMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_WORKER_CONTEXT pContext = (PLWIO_SRV_WORKER_CONTEXT)pData;
    PSRV_EXEC_CONTEXT pExecContext = NULL;
    struct timespec ts = {0, 0};

    LWIO_LOG_DEBUG("Srv worker [id:%u] starting", pContext->workerId);

    while (!SrvWorkerMustStop(pContext))
    {
        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

        ntStatus = SrvGetNextExecutionContext(&ts, &pExecContext);
        if (ntStatus == STATUS_IO_TIMEOUT)
        {
            ntStatus = 0;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (SrvWorkerMustStop(pContext))
        {
            break;
        }

        if (pExecContext)
        {
            if (SrvIsValidExecContext(pExecContext))
            {
                NTSTATUS ntStatus2 = SrvProtocolExecute(pExecContext);
                if (ntStatus2)
                {
                    LWIO_LOG_VERBOSE("Failed to execute server task [code:%d]", ntStatus2);
                }
            }

            SrvReleaseExecContext(pExecContext);
            pExecContext = NULL;
        }
    }

cleanup:

    if (pExecContext)
    {
        SrvReleaseExecContext(pExecContext);
    }

    LWIO_LOG_DEBUG("Srv worker [id:%u] stopping", pContext->workerId);

    return NULL;

error:

    goto cleanup;
}

VOID
SrvWorkerIndicateStop(
    PLWIO_SRV_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        SrvWorkerIndicateStopContext(&pWorker->context);
    }
}

VOID
SrvWorkerFreeContents(
    PLWIO_SRV_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        // Someone must have already called SrvWorkerIndicateStop
        // and unblocked the prod/cons queue.

        pthread_join(pWorker->worker, NULL);
    }

    if (pWorker->context.pMutex)
    {
        pthread_mutex_destroy(pWorker->context.pMutex);
        pWorker->context.pMutex = NULL;
    }
}

static
NTSTATUS
SrvGetNextExecutionContext(
    struct timespec*   pTimespec,
    PSRV_EXEC_CONTEXT* ppContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pContext = NULL;

    status = SrvProdConsTimedDequeue(
                    &gSMBSrvGlobals.workQueue,
                    pTimespec,
                    (PVOID*)&pContext);
    if (status != STATUS_SUCCESS)
    {
        goto error;
    }

    *ppContext = pContext;

cleanup:

    return status;

error:

    *ppContext = NULL;

    goto cleanup;
}

static
BOOLEAN
SrvWorkerMustStop(
    PLWIO_SRV_WORKER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pContext->mutex);

    bStop = pContext->bStop;

    pthread_mutex_unlock(&pContext->mutex);

    return bStop;
}

static
VOID
SrvWorkerIndicateStopContext(
    PLWIO_SRV_WORKER_CONTEXT pContext
    )
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_mutex_unlock(&pContext->mutex);
}
