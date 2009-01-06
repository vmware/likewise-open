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

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT pDriverObject,
    PIRP pIrp
	)
{
    NTSTATUS ntStatus = 0;

    pDriverObject->MajorFunction[IRP_MJ_CREATE] = PvfsCreateFile;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = PvfsCloseFile;
    pDriverObject->MajorFunction[IRP_MJ_READ] = PvfsReadFile;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = PvfsWriteFile;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PvfsDeviceControl;
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = PvfsCleanup;
    
    ntStatus = IoCreateDevice(
                    pDriverObject,
                    0,
                    DeviceName,
                    FILE_DEVICE_DISK_FILE_SYSTEM,
                    0,
                    &PvfsGlobalData.pDeviceObject,
                    );
    BAIL_ON_NT_STATUS(ntStatus);

error:    
    return ntStatus;
}



