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

/* File Globals */

static PVFS_WORKER_POOL gWorkPool;

/* Forward declarations */

static PVOID
PvfsWorkerDoWork(
    PVOID pArgs
    );


/* Code */

/************************************************************
  **********************************************************/

NTSTATUS
PvfsInitWorkerThreads(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int i = 0;
    int unixerr = 0;
    int ret = 0;

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
                  PVFS_WORKERS_NUMBER_THREADS * sizeof(PVFS_WORKER));
    BAIL_ON_NT_STATUS(ntError);

    /* I/O Worker Threads */

    gWorkPool.PoolSize = PVFS_WORKERS_NUMBER_THREADS;
    for (i=0; i<gWorkPool.PoolSize; i++)
    {
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

static PVOID
PvfsWorkerDoWork(
    PVOID pQueue
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;
    BOOL bInLock = FALSE;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;
    PPVFS_WORK_QUEUE pWorkQueue = (PPVFS_WORK_QUEUE)pQueue;

    while(1)
    {
        bInLock = FALSE;
        pWorkCtx = NULL;
        pIrpCtx = NULL;

        /*
         * Pull from the internal work queue first.  Fallback to the
         * I/O Work queue if the internal one is empty.  The activity
         * in the internal work queue should never cause starvation
         * of the I/O queue since the internal queue is driven by
         * additional state from IRPs.
         */

        ntError = PvfsNextWorkItem(pWorkQueue, &pWorkCtx);

        /* If the work item is NULL, try again next time around.  */

        if ((ntError != STATUS_SUCCESS) || (pWorkCtx == NULL))
        {
            continue;
        }

        /* Deal with IRPs slightly differently than non IRP work items */

        if (pWorkCtx->bIsIrpContext)
        {
            pIrpCtx = (PPVFS_IRP_CONTEXT)pWorkCtx->pContext;

            LWIO_LOCK_MUTEX(bInLock, &pIrpCtx->Mutex);

            if (pIrpCtx->bIsCancelled)
            {
                ntError = STATUS_CANCELLED;
            }
            else
            {
                pIrpCtx->bInProgress = TRUE;
                ntError = pWorkCtx->pfnCompletion(pWorkCtx->pContext);
            }

            LWIO_UNLOCK_MUTEX(bInLock, &pIrpCtx->Mutex);

            /* Check to to see if the request was requeued */

            if (ntError != STATUS_PENDING)
            {
                pIrpCtx->pIrp->IoStatusBlock.Status = ntError;

                PvfsAsyncIrpComplete(pIrpCtx);
                PvfsFreeIrpContext(&pIrpCtx);
            }

            /* The IRP has been completed or pended again.  In either case,
               the work context* is done and releases */

            pWorkCtx->pContext = NULL;
        }
        else
        {
            ntError = pWorkCtx->pfnCompletion(pWorkCtx->pContext);
        }

        PvfsFreeWorkContext(&pWorkCtx);
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
