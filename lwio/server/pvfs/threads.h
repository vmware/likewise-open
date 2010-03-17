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
 *        threads.h
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Thread pool header file
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#ifndef __PVFS_THREADS_H__
#define __PVFS_THREADS_H__

/* Worker thread types */

typedef struct _PVFS_WORKER_THREAD {

    pthread_t   hThread;

} PVFS_WORKER, *PPVFS_WORKER;

typedef struct _PVFS_WORKER_POOL {

    DWORD PoolSize;
#ifdef _PVFS_DEVELOPER_DEBUG
    LONG Available;
#endif
    PPVFS_WORKER IoWorkers;
    PVFS_WORKER PriorityWorker;

} PVFS_WORKER_POOL, *PPVSF_WORKER_POOL;

/* Work Queue data type */

typedef struct _PVFS_WORK_QUEUE {

    pthread_mutex_t Mutex;
    pthread_cond_t  ItemsAvailable;
    pthread_cond_t  SpaceAvailable;

    BOOLEAN bWait;

    PPVFS_LIST pQueue;

} PVFS_WORK_QUEUE, *PPVFS_WORK_QUEUE;


/* Functions */

NTSTATUS
PvfsInitWorkerThreads(
    VOID
    );

NTSTATUS
PvfsInitOplockThreads(
    VOID
    );

NTSTATUS
PvfsInitWorkQueue(
    PPVFS_WORK_QUEUE *ppWorkQueue,
    LONG Size,
    PPVFS_LIST_FREE_DATA_FN pfnFreeData,
    BOOLEAN bWaitSemantics
    );

NTSTATUS
PvfsAddWorkItem(
    PPVFS_WORK_QUEUE pWorkQueue,
    PPVFS_WORK_CONTEXT pWork
    );

NTSTATUS
PvfsNextWorkItem(
    PPVFS_WORK_QUEUE pWorkQueue,
    PPVFS_WORK_CONTEXT *ppWork
    );


#endif    /* __PVFS_THREADS_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
