/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
:q
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
 *        Likewise Distributed File System Driver (DFS)
 *
 *        Driver Entry Function
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"

/* Forward declarations */

static
VOID
DfsDriverShutdown(
    IN IO_DRIVER_HANDLE DriverHandle
    );

static
NTSTATUS
DfsDriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    );

static NTSTATUS
DfsDriverInitialize(
    VOID
    );



/***********************************************************************
 **********************************************************************/

NTSTATUS
IO_DRIVER_ENTRY(dfs)(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    IO_DEVICE_HANDLE deviceHandle = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != InterfaceVersion)
    {
        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoDriverInitialize(
                  DriverHandle,
                  NULL,
                  DfsDriverShutdown,
                  DfsDriverDispatch);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoDeviceCreate(
                   &deviceHandle,
                   DriverHandle,
                   "dfs",
                   NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = DfsDriverInitialize();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
VOID
DfsDriverShutdown(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    IO_LOG_ENTER_LEAVE("");
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
DfsDriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDFS_IRP_CONTEXT pIrpContext = NULL;

    ntStatus = DfsAllocateIrpContext(&pIrpContext, pIrp);
    BAIL_ON_NT_STATUS(ntStatus);

    switch (pIrpContext->pIrp->Type)
    {
    case IRP_TYPE_CREATE:
        ntStatus = DfsCreate(pIrpContext);
        break;

    case IRP_TYPE_CLOSE:
        ntStatus = DfsClose(pIrpContext);
        break;

    case IRP_TYPE_FS_CONTROL:
        ntStatus = DfsFsIoControl(pIrpContext);
        break;

    case IRP_TYPE_FLUSH_BUFFERS:
    case IRP_TYPE_LOCK_CONTROL:
    case IRP_TYPE_READ:
    case IRP_TYPE_WRITE:
    case IRP_TYPE_SET_INFORMATION:
    case IRP_TYPE_QUERY_INFORMATION:
    case IRP_TYPE_QUERY_DIRECTORY:
    case IRP_TYPE_QUERY_VOLUME_INFORMATION:
    case IRP_TYPE_QUERY_SECURITY:
    case IRP_TYPE_SET_SECURITY:
    case IRP_TYPE_READ_DIRECTORY_CHANGE:
    case IRP_TYPE_DEVICE_IO_CONTROL:
    case IRP_TYPE_SET_VOLUME_INFORMATION:
    case IRP_TYPE_QUERY_QUOTA:
    case IRP_TYPE_SET_QUOTA:
        ntStatus = STATUS_NOT_SUPPORTED;
        break;

    default:
        ntStatus = STATUS_INVALID_PARAMETER;
        break;
    }

    if ((ntStatus != STATUS_SUCCESS) && (ntStatus != STATUS_PENDING))
    {
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (ntStatus != STATUS_PENDING)
    {
        pIrp->IoStatusBlock.Status = ntStatus;
    }

    DfsReleaseIrpContext(&pIrpContext);

    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
DfsDriverInitialize(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = DfsInitializeFCBTable();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

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
