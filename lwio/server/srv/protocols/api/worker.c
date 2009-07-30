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
SrvProtocolWorkerMain(
    PVOID pData
    );

static
BOOLEAN
SrvProtocolWorkerMustStop(
    PLWIO_SRV_PROTOCOL_WORKER_CONTEXT pContext
    );

static
NTSTATUS
SrvProtocolWorkerStop(
    PLWIO_SRV_PROTOCOL_WORKER_CONTEXT pContext
    );

NTSTATUS
SrvProtocolWorkerInit(
    PLWIO_SRV_PROTOCOL_WORKER pWorker,
    PSMB_PROD_CONS_QUEUE      pWorkQueue
    )
{
    NTSTATUS ntStatus = 0;

    memset(&pWorker->context, 0, sizeof(pWorker->context));

    pthread_mutex_init(&pWorker->context.mutex, NULL);
    pWorker->context.pMutex = &pWorker->context.mutex;

    pWorker->context.bStop = FALSE;
    pWorker->context.workerId = pWorker->workerId;
    pWorker->context.pWorkQueue = pWorkQueue;

    ntStatus = pthread_create(
                    &pWorker->worker,
                    NULL,
                    &SrvProtocolWorkerMain,
                    &pWorker->context);
    BAIL_ON_NT_STATUS(ntStatus);

    pWorker->pWorker = &pWorker->worker;

error:

    return ntStatus;
}

NTSTATUS
SrvProtocolBuildWorkItem(
    HANDLE hData,
    PFN_SRV_PROTOCOL_WORK_ITEM_EXECUTE pfnExecute,
    PFN_SRV_PROTOCOL_WORK_ITEM_RELEASE pfnRelease,
    PSRV_PROTOCOL_WORK_ITEM* ppWorkItem
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_PROTOCOL_WORK_ITEM pWorkItem = NULL;

    // TODO: Change to using free list
    //
    ntStatus = SrvAllocateMemory(
                    sizeof(SRV_PROTOCOL_WORK_ITEM),
                    (PVOID*)&pWorkItem);
    BAIL_ON_NT_STATUS(ntStatus);

    pWorkItem->hData = hData;
    pWorkItem->pfnExecute = pfnExecute;
    pWorkItem->pfnRelease = pfnRelease;

    *ppWorkItem = pWorkItem;

cleanup:

    return ntStatus;

error:

    *ppWorkItem = NULL;

    if (pWorkItem)
    {
        SrvFreeMemory(pWorkItem);
    }

    goto cleanup;
}

NTSTATUS
SrvProtocolEnqueueWorkItem(
    PSRV_PROTOCOL_WORK_ITEM pWorkItem
    )
{
    return SrvProdConsEnqueue(
                    &gProtocolApiGlobals.asyncWorkQueue,
                    pWorkItem);
}

VOID
SrvProtocolFreeWorkItem(
    PSRV_PROTOCOL_WORK_ITEM pWorkItem
    )
{
    if (pWorkItem->pfnRelease && pWorkItem->hData)
    {
        pWorkItem->pfnRelease(pWorkItem->hData);
    }
    SrvFreeMemory(pWorkItem);
}

static
PVOID
SrvProtocolWorkerMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_PROTOCOL_WORKER_CONTEXT pContext =
                    (PLWIO_SRV_PROTOCOL_WORKER_CONTEXT)pData;
    struct timespec ts = {0, 0};
    PSRV_PROTOCOL_WORK_ITEM  pWorkItem = NULL;

    LWIO_LOG_DEBUG("Srv protocol worker [id:%u] starting", pContext->workerId);

    while (!SrvProtocolWorkerMustStop(pContext))
    {
        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

        ntStatus = SrvProdConsTimedDequeue(
                            pContext->pWorkQueue,
                            &ts,
                            (PVOID*)&pWorkItem);
        if (ntStatus == STATUS_IO_TIMEOUT)
        {
            ntStatus = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (SrvProtocolWorkerMustStop(pContext))
        {
            break;
        }

        if (pWorkItem)
        {
            if (pWorkItem->pfnExecute)
            {
                pWorkItem->pfnExecute(pWorkItem->hData);
            }

            SrvProtocolFreeWorkItem(pWorkItem);
            pWorkItem = NULL;
        }
    }

cleanup:

    LWIO_LOG_DEBUG("Srv protocol worker [id:%u] stopping", pContext->workerId);

    if (pWorkItem)
    {
        SrvProtocolFreeWorkItem(pWorkItem);
    }

    return NULL;

error:

    LWIO_LOG_ERROR("Srv protocol worker [id:%u] stopping. [status:0x%x]",
                   pContext->workerId,
                   ntStatus);

    goto cleanup;
}

VOID
SrvProtocolWorkerIndicateStop(
    PLWIO_SRV_PROTOCOL_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        SrvProtocolWorkerStop(&pWorker->context);
    }
}

VOID
SrvProtocolWorkerFreeContents(
    PLWIO_SRV_PROTOCOL_WORKER pWorker
    )
{
    if (pWorker->pWorker)
    {
        SrvProtocolWorkerStop(&pWorker->context);

        pthread_join(pWorker->worker, NULL);
    }

    if (pWorker->context.pMutex)
    {
        pthread_mutex_destroy(pWorker->context.pMutex);
        pWorker->context.pMutex = NULL;
    }
}

static
BOOLEAN
SrvProtocolWorkerMustStop(
    PLWIO_SRV_PROTOCOL_WORKER_CONTEXT pContext
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
SrvProtocolWorkerStop(
    PLWIO_SRV_PROTOCOL_WORKER_CONTEXT pContext
    )
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_mutex_unlock(&pContext->mutex);

    return 0;
}

