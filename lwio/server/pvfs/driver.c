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
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Driver Entry Function
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static VOID
PvfsDriverShutdown(
    IN IO_DRIVER_HANDLE DriverHandle
    );

static NTSTATUS
PvfsDriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    );

static NTSTATUS
PvfsDriverInitialize(
    VOID
    );


/* Code */

/************************************************************
  **********************************************************/

NTSTATUS
IO_DRIVER_ENTRY(pvfs)(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IO_DEVICE_HANDLE deviceHandle = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != InterfaceVersion)
    {
        ntError = STATUS_DEVICE_CONFIGURATION_ERROR;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = IoDriverInitialize(DriverHandle,
                                 NULL,
                                 PvfsDriverShutdown,
                                 PvfsDriverDispatch);
    BAIL_ON_NT_STATUS(ntError);

    ntError = IoDeviceCreate(&deviceHandle,
                             DriverHandle,
                             "pvfs",
                             NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsDriverInitialize();
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/************************************************************
 Driver Exit Function
 ***********************************************************/

static VOID
PvfsDriverShutdown(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    PvfsDestroyFCBTable();

    pthread_rwlock_destroy(&gPathCacheRwLock);

    IO_LOG_ENTER_LEAVE("");
}

/************************************************************
 Driver Dispatch Routine
 ***********************************************************/


static NTSTATUS
PvfsDriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    ntError = PvfsAllocateIrpContext(&pIrpCtx, pIrp);
    BAIL_ON_NT_STATUS(ntError);

    switch (pIrpCtx->pIrp->Type)
    {
    /* Converted to ssync model */
    case IRP_TYPE_CREATE:
        ntError = PvfsAsyncCreate(pIrpCtx);
        break;
    case IRP_TYPE_READ:
        ntError = PvfsAsyncRead(pIrpCtx);
        break;
    case IRP_TYPE_WRITE:
        ntError = PvfsAsyncWrite(pIrpCtx);
        break;
    case IRP_TYPE_FLUSH_BUFFERS:
        ntError = PvfsAsyncFlushBuffers(pIrpCtx);
        break;
    case IRP_TYPE_LOCK_CONTROL:
        ntError = PvfsDispatchLockControl(pIrpCtx);
        break;
    case IRP_TYPE_FS_CONTROL:
        ntError = PvfsDispatchFsIoControl(pIrpCtx);
        break;
    case IRP_TYPE_QUERY_INFORMATION:
        ntError = PvfsAsyncQueryInformationFile(pIrpCtx);
        break;
    case IRP_TYPE_SET_INFORMATION:
        ntError = PvfsAsyncSetInformationFile(pIrpCtx);
        break;
    case IRP_TYPE_QUERY_DIRECTORY:
        ntError = PvfsAsyncQueryDirInformation(pIrpCtx);
        break;
    case IRP_TYPE_QUERY_VOLUME_INFORMATION:
        ntError = PvfsAsyncQueryVolumeInformation(pIrpCtx);
        break;
    case IRP_TYPE_QUERY_SECURITY:
        ntError = PvfsAsyncQuerySecurityFile(pIrpCtx);
        break;
    case IRP_TYPE_SET_SECURITY:
        ntError = PvfsAsyncSetSecurityFile(pIrpCtx);
        break;

    /* Currently only support synchronous calls */
    case IRP_TYPE_CLOSE:
        ntError = PvfsClose(pIrpCtx);
        break;
    case IRP_TYPE_DEVICE_IO_CONTROL:
        ntError = PvfsDeviceIoControl(pIrpCtx);
        break;

    /* Not implemented */
    case IRP_TYPE_SET_VOLUME_INFORMATION:
        ntError = PvfsSetVolumeInformation(pIrpCtx);
        break;
    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }

    if ((ntError != STATUS_SUCCESS) && (ntError != STATUS_PENDING)) {
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:

    if (ntError != STATUS_PENDING) {
        pIrp->IoStatusBlock.Status = ntError;
        PvfsFreeIrpContext(&pIrpCtx);
    }

    return ntError;

error:
    goto cleanup;
}


/************************************************************
 ***********************************************************/

static NTSTATUS
PvfsDriverInitialize(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    ntError = PvfsInitializeFCBTable();
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsInitWorkerThreads();
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsPathCacheInit();
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
