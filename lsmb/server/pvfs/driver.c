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
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;
    int EE = 0;

    switch (pIrp->Type)
    {
        case IRP_TYPE_CREATE:
            ntStatus = PvfsCreate(
                            DeviceHandle,
                            pIrp
                            );
            break;

        case IRP_TYPE_CLOSE:
            ntStatus = PvfsClose(
                            DeviceHandle,
                            pIrp
                            );
            break;


        case IRP_TYPE_READ:
            ntStatus = PvfsRead(
			  DeviceHandle,
			  pIrp
			  );
             break;

        case IRP_TYPE_WRITE:
            ntStatus = PvfsWrite(
                          DeviceHandle,
                          pIrp
                          );
            break;

        case IRP_TYPE_IO_CONTROL:
            break;

        case IRP_TYPE_FS_CONTROL:
            ntStatus = PvfsFsCtrl(
                            DeviceHandle,
			    pIrp
			    );
            break;
        case IRP_TYPE_FLUSH:
        case IRP_TYPE_QUERY_INFORMATION:
            ntStatus = PvfsQueryInformation(
                            DeviceHandle,
                            pIrp
                            );
            break;
        case IRP_TYPE_SET_INFORMATION:
            ntStatus = PvfsSetInformation(
                            DeviceHandle,
                            pIrp
                            );
            break;
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
