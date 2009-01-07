/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        createnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

NTSTATUS
DriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    );

VOID
DriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    );

NTSTATUS
DriverEntry(
    IN IO_DRIVER_HANDLE hDriver,
    IN ULONG ulInterfaceVersion
    )
{
    NTSTATUS ntStatus = 0;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != ulInterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoDriverInitialize(
                    hDriver,
                    NULL,
                    DriverShutdown,
                    DriverDispatch);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBSrvListenerStart();

error:

    return ntStatus;
}

NTSTATUS
DriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;

    switch (pIrp->Type)
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
            BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

VOID
DriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    )
{
    NTSTATUS ntStatus = SMBSrvListenerStop();

    if (ntStatus)
    {
        SMB_LOG_ERROR("[srv] driver failed to stop. [code: %d]", ntStatus);
    }
}



