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
 *        work_queue.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Thread pool work queue
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/*********************************************************
 ********************************************************/

NTSTATUS
PvfsInitWorkQueue(
    PPVFS_WORK_QUEUE *ppWorkQueue,
    LONG Size,
    PPVFS_LIST_FREE_DATA_FN pfnFreeData,
    BOOLEAN bWaitSemantics
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_WORK_QUEUE pWorkQ = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pWorkQ, sizeof(PVFS_WORK_QUEUE), TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsListInit(&pWorkQ->pQueue, Size, pfnFreeData);
    BAIL_ON_NT_STATUS(ntError);

    pthread_mutex_init(&pWorkQ->Mutex, NULL);
    pthread_cond_init(&pWorkQ->ItemsAvailable, NULL);
    pthread_cond_init(&pWorkQ->SpaceAvailable, NULL);

    pWorkQ->bWait = bWaitSemantics;

    *ppWorkQueue = pWorkQ;
    pWorkQ = NULL;

cleanup:
    PVFS_FREE(&pWorkQ);

    return ntError;

error:
    goto cleanup;
}

/*********************************************************
 ********************************************************/

NTSTATUS
PvfsAddWorkItem(
    PPVFS_WORK_QUEUE pWorkQueue,
    PPVFS_WORK_CONTEXT pWork
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOL bInLock = FALSE;
    BOOL bSignal = FALSE;

    BAIL_ON_INVALID_PTR(pWorkQueue, ntError);
    BAIL_ON_INVALID_PTR(pWork, ntError);

    LWIO_LOCK_MUTEX(bInLock, &pWorkQueue->Mutex);

    if (pWorkQueue->bWait)
    {
        while (PvfsListIsFull(pWorkQueue->pQueue))
        {
            pthread_cond_wait(
                &pWorkQueue->SpaceAvailable,
                &pWorkQueue->Mutex);
        }

        if (PvfsListIsEmpty(pWorkQueue->pQueue))
        {
            bSignal = TRUE;
        }
    }

    ntError = PvfsListAddTail(pWorkQueue->pQueue, &pWork->WorkList);
    BAIL_ON_NT_STATUS(ntError);

    if (bSignal)
    {
        pthread_cond_broadcast(&pWorkQueue->ItemsAvailable);
    }

cleanup:
    LWIO_UNLOCK_MUTEX(bInLock, &pWorkQueue->Mutex);

    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsNextWorkItem(
    PPVFS_WORK_QUEUE pWorkQueue,
    PPVFS_WORK_CONTEXT *ppWork
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOL bInLock = FALSE;
    BOOL bSignal = FALSE;
    PLW_LIST_LINKS pData = NULL;

    BAIL_ON_INVALID_PTR(pWorkQueue, ntError);
    BAIL_ON_INVALID_PTR(ppWork, ntError);

    LWIO_LOCK_MUTEX(bInLock, &pWorkQueue->Mutex);

    if (pWorkQueue->bWait)
    {
        while (PvfsListIsEmpty(pWorkQueue->pQueue))
        {
            pthread_cond_wait(
                &pWorkQueue->ItemsAvailable,
                &pWorkQueue->Mutex);
        }

        if (PvfsListIsFull(pWorkQueue->pQueue))
        {
            bSignal = TRUE;
        }
    }

    ntError = PvfsListRemoveHead(pWorkQueue->pQueue, &pData);
    BAIL_ON_NT_STATUS(ntError);

    *ppWork = LW_STRUCT_FROM_FIELD(
                  pData,
                  PVFS_WORK_CONTEXT,
                  WorkList);

    if (bSignal)
    {
        pthread_cond_broadcast(&pWorkQueue->SpaceAvailable);
    }

cleanup:
    LWIO_UNLOCK_MUTEX(bInLock, &pWorkQueue->Mutex);

    return ntError;

error:
    goto cleanup;
}


#if 0 /* Unused for now */

static
VOID
PvfsMakeWorkItemTimeout(
    OUT struct timespec *pTimeout,
    IN  ULONG MilliSeconds
    )
{
    struct timeval Now = {0};
    struct timezone TZ = {0};

    gettimeofday(&Now, &TZ);

    pTimeout->tv_sec = Now.tv_sec;
    pTimeout->tv_nsec = Now.tv_usec * 1000;

    pTimeout->tv_nsec += (MilliSeconds * 1000 * 1000);
}

#endif

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
