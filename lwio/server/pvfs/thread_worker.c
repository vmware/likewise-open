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

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        thread_worker.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Worker thread pool
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static PVOID
PvfsWorkerDoWork(
    PVOID pArgs
    );


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsInitWorkerThreads(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int i = 0;
    int unixerr = 0;
    int ret = 0;

    gWorkPool.PoolSize = gPvfsDriverConfig.WorkerThreadPoolSize;
    gWorkPool.Available = gWorkPool.PoolSize + 1;

    /* Both of these will be blocking queues */

    ntError = PvfsInitWorkQueue(
                  &gpPvfsInternalWorkQueue,
                  0, /* unlimited */
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreeWorkContext,
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsInitWorkQueue(
                  &gpPvfsIoWorkQueue,
                  PVFS_WORKERS_MAX_WORK_ITEMS,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreeWorkContext,
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory(
                  (PVOID*)&gWorkPool.IoWorkers,
                  gWorkPool.PoolSize * sizeof(PVFS_WORKER));
    BAIL_ON_NT_STATUS(ntError);

    /* I/O Worker Threads */

    for (i=0; i<gWorkPool.PoolSize; i++)
    {
        gWorkPool.IoWorkers[i].bTerminate = FALSE;
        ret = pthread_create(
                  &gWorkPool.IoWorkers[i].hThread,
                  NULL,
                  &PvfsWorkerDoWork,
                  (PVOID)gpPvfsIoWorkQueue);
        if (ret != 0) {
            PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
        }
    }

    /* One priority internal work queue thread */

    gWorkPool.PriorityWorker.bTerminate = FALSE;
    ret = pthread_create(
              &gWorkPool.PriorityWorker.hThread,
              NULL,
              &PvfsWorkerDoWork,
              (PVOID)gpPvfsInternalWorkQueue);
    if (ret != 0) {
        PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsStopWorkerThreads(
    VOID
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    LONG count  = 0;
    PPVFS_WORK_CONTEXT pWorkContext = NULL;

    for (count=0; count < gWorkPool.PoolSize; count++)
    {
        ntError = PvfsCreateWorkContext(
                      &pWorkContext,
                      PVFS_WORK_CTX_FLAG_TERMINATE,
                      NULL,
                      NULL,
                      NULL);
        /* Don't bail on error since we are trying to shutdown */
        if (ntError == STATUS_SUCCESS)
        {
            ntError = PvfsScheduleIoWorkItem(pWorkContext);
            pWorkContext = NULL;
        }

        PvfsFreeWorkContext(&pWorkContext);
    }

    ntError = PvfsCreateWorkContext(
                  &pWorkContext,
                  PVFS_WORK_CTX_FLAG_TERMINATE,
                  NULL,
                  NULL,
                  NULL);
    /* Don't bail on error since we are trying to shutdown */
    if (ntError == STATUS_SUCCESS)
    {
        ntError = PvfsAddWorkItem(
                      gpPvfsInternalWorkQueue,
                      (PVOID)pWorkContext);
        pWorkContext = NULL;
    }

    PvfsFreeWorkContext(&pWorkContext);

    /* Wait on threads to shutdown */

    for (count=0; count < gWorkPool.PoolSize; count++)
    {
        pthread_join(gWorkPool.IoWorkers[count].hThread, NULL);
    }
    pthread_join(gWorkPool.PriorityWorker.hThread, NULL);

    return STATUS_SUCCESS;
}


/***********************************************************************
 **********************************************************************/

static
PVOID
PvfsWorkerDoWork(
    PVOID pQueue
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;
    PPVFS_WORK_QUEUE pWorkQueue = (PPVFS_WORK_QUEUE)pQueue;
    BOOLEAN bActive = FALSE;
    LONG AvailableThreads = 0;

    while(1)
    {
        pWorkCtx = NULL;
        pIrpCtx = NULL;
        bActive = FALSE;

        ntError = PvfsNextWorkItem(pWorkQueue, &pWorkCtx);

        /* If the work item is NULL, try again next time around.  */

        if ((ntError != STATUS_SUCCESS) || (pWorkCtx == NULL))
        {
            continue;
        }

        /* Check for one more available thread which would be the
           internal priority thread */

        AvailableThreads = InterlockedDecrement(&gWorkPool.Available);

        if (gPvfsDriverConfig.EnableDriverDebug && AvailableThreads <= 1)
        {
            LWIO_LOG_ERROR(
                "%s: Worker thread count exhausted! (%d)\n",
                PVFS_LOG_HEADER,
                AvailableThreads);
        }

        if (IsSetFlag(pWorkCtx->Flags, PVFS_WORK_CTX_FLAG_TERMINATE))
        {
            /* All Done */
            PvfsFreeWorkContext(&pWorkCtx);
            return NULL;
        }

        /* Deal with IRPs slightly differently than non IRP work items */

        if (IsSetFlag(pWorkCtx->Flags, PVFS_WORK_CTX_FLAG_IRP_CONTEXT))
        {
            pIrpCtx = (PPVFS_IRP_CONTEXT)pWorkCtx->pContext;

            PvfsQueueCancelIrpIfRequested(pIrpCtx);

            bActive = PvfsIrpContextMarkIfNotSetFlag(
                          pIrpCtx,
                          PVFS_IRP_CTX_FLAG_CANCELLED,
                          PVFS_IRP_CTX_FLAG_ACTIVE);

            if (bActive)
            {
                ntError = pWorkCtx->pfnCompletion(pWorkCtx->pContext);
            }
            else
            {
                ntError = STATUS_CANCELLED;
            }

            /* Check to to see if the request was requeued.  We finish cancelled IRPs
               here since it is expected that any item in the I/O queue is handled in
               full in this loop.  Other pended IRPs (e.g. oplocks) that may be cancelled
               will appear in other PVFS_QUEUE_TYPE's */

            if (ntError != STATUS_PENDING)
            {
                pIrpCtx->pIrp->IoStatusBlock.Status = ntError;

                PvfsAsyncIrpComplete(pIrpCtx);
            }
        }
        else
        {
            ntError = pWorkCtx->pfnCompletion(pWorkCtx->pContext);
        }

        PvfsFreeWorkContext(&pWorkCtx);

        InterlockedIncrement(&gWorkPool.Available);
    }

    return NULL;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
