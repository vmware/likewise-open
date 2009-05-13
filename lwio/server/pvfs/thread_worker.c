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

static VOID
FreeWorkItem(
    PVOID *ppItem
    );

static PVOID
PvfsWorkerDoWork(
    PVOID pArgs
    );

static NTSTATUS
DispatchIrp(
PPVFS_IRP_CONTEXT pIrpCtx
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

    ntError = PvfsInitWorkQueue(&gpPvfsIoWorkQueue,
                                1,
                                &FreeWorkItem);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory((PVOID*)&gWorkPool.Workers,
                                 PVFS_NUMBER_WORKER_THREADS * sizeof(PVFS_WORKER));
    BAIL_ON_NT_STATUS(ntError);

    gWorkPool.PoolSize = PVFS_NUMBER_WORKER_THREADS;

    for (i=0; i<gWorkPool.PoolSize; i++)
    {
        int ret = 0;

        ret = pthread_create(&gWorkPool.Workers[i].hThread,
                             NULL,
                             &PvfsWorkerDoWork,
                             NULL);
        if (ret != 0) {
            PVFS_BAIL_ON_UNIX_ERROR(unixerr, ntError);
        }
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/************************************************************
  **********************************************************/

static VOID
FreeWorkItem(
    PVOID *ppItem
    )
{
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    if ((ppItem == NULL) || (*ppItem == NULL)) {
        return;
    }

    pIrpCtx = (PPVFS_IRP_CONTEXT)(*ppItem);

    PvfsFreeIrpContext(&pIrpCtx);

    return;
}

/************************************************************
  **********************************************************/

static PVOID
PvfsWorkerDoWork(
    PVOID pArgs
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;
    PVOID pData = NULL;
    BOOL bInLock = FALSE;

    while(1)
    {
        bInLock = FALSE;
        pData = NULL;
        pIrpCtx = NULL;

        /* Failing to get the next work item is really bad.
           Should not happen */

        ntError = PvfsNextWorkItem(gpPvfsIoWorkQueue, &pData);
        BAIL_ON_NT_STATUS(ntError);

        pIrpCtx = (PPVFS_IRP_CONTEXT)pData;

        /* Handle the IRP, save the return code,
           and signal the driver that we are done */

        LWIO_LOCK_MUTEX(bInLock, &pIrpCtx->Mutex);

        pIrpCtx->ntError = DispatchIrp(pIrpCtx);

        pIrpCtx->bFinished = TRUE;
        pthread_cond_signal(&pIrpCtx->Event);

        LWIO_UNLOCK_MUTEX(bInLock, &pIrpCtx->Mutex);
    }

cleanup:
    return NULL;

error:
    goto cleanup;
}


/************************************************************
  **********************************************************/

static NTSTATUS
DispatchIrp(
PPVFS_IRP_CONTEXT pIrpCtx
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_DEVICE_HANDLE DeviceHandle = (IO_DEVICE_HANDLE)NULL;

    switch (pIrpCtx->pIrp->Type)
    {
    case IRP_TYPE_CREATE:
        ntError = PvfsCreate(DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_CLOSE:
        ntError = PvfsClose(DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_READ:
        ntError = PvfsRead(DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_WRITE:
        ntError = PvfsWrite(DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_DEVICE_IO_CONTROL:
        ntError = STATUS_NOT_IMPLEMENTED;
        break;

    case IRP_TYPE_FS_CONTROL:
        ntError = PvfsFsCtrl(DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_FLUSH_BUFFERS:
        ntError = PvfsFlushBuffers(DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_QUERY_INFORMATION:
        ntError = PvfsQuerySetInformation(PVFS_QUERY, DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_SET_INFORMATION:
        ntError = PvfsQuerySetInformation(PVFS_SET, DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_QUERY_DIRECTORY:
        ntError = PvfsQueryDirInformation(DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_QUERY_VOLUME_INFORMATION:
        ntError = PvfsQueryVolumeInformation(DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_LOCK_CONTROL:
        ntError = PvfsLockControl(DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_QUERY_SECURITY:
        ntError = PvfsQuerySetSecurityFile(PVFS_QUERY, DeviceHandle, pIrpCtx);
        break;

    case IRP_TYPE_SET_SECURITY:
        ntError = PvfsQuerySetSecurityFile(PVFS_SET, DeviceHandle, pIrpCtx);
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}




/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
