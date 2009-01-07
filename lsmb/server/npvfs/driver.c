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

#include "npvfs.h"

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT pDriverObject,
    PIRP pIrp
	)
{
	NTSTATUS ntStatus = 0;

    pDriverObject->MajorFunction[IRP_MJ_CREATE] = NpfsCreateFile;
    pDriverObject->MajorFunction[IRP_MJ_CLOSE] = NpfsCloseFile;
    pDriverObject->MajorFunction[IRP_MJ_READ] = NpfsReadFile;
    pDriverObject->MajorFunction[IRP_MJ_WRITE] = NpfsWriteFile;
    pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = NpfsDeviceControl;
    pDriverObject->MajorFunction[IRP_MJ_CLEANUP] = NpfsCleanup;
    
    ntStatus = IoCreateDevice(
                    pDriverObject,
                    0,
                    DeviceName,
                    FILE_DEVICE_DISK_FILE_SYSTEM,
                    0,
                    &NpfsGlobalData.pDeviceObject,
                    );
    BAIL_ON_NT_STATUS(ntStatus);
error:    
    
                        
	return ntStatus;
}



