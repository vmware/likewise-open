/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "pvfs.h"

VOID
DriverShutdown(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    IO_LOG_ENTER_LEAVE("");
}

NTSTATUS
DriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP Irp
    )
{
    NTSTATUS ntStatus = 0;
    int EE = 0;

    switch (Irp->Type)
    {
    case IRP_TYPE_CREATE:
    case IRP_TYPE_CLOSE:
    case IRP_TYPE_READ:
    case IRP_TYPE_WRITE:
    case IRP_TYPE_IO_CONTROL:
    case IRP_TYPE_FS_CONTROL:
    case IRP_TYPE_FLUSH:
    case IRP_TYPE_QUERY_INFORMATION:
    case IRP_TYPE_SET_INFORMATION:
    default:
        ntStatus = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);
    }

cleanup:
    IO_LOG_ENTER_LEAVE_STATUS_EE(ntStatus, EE);
    return ntStatus;
}


NTSTATUS
DriverEntry(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    )
{
    NTSTATUS ntStatus = 0;
    int EE = 0;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != InterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);
    }

    ntStatus = IoDriverInitialize(DriverHandle,
                                  NULL,
                                  DriverShutdown,
                                  DriverDispatch);

cleanup:
    IO_LOG_ENTER_LEAVE_STATUS_EE(ntStatus, EE);
    return ntStatus;
}
