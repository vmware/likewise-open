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

#define DO_TEST
//#undef DO_TEST

#ifdef DO_TEST
#include "ioapi.h"
#include "../iomgr2/iostring.h"

static
VOID
DoTest(
    PCSTR pszPath
    )
{
    NTSTATUS ntStatus = 0;
    int EE = 0;
    IO_FILE_HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IO_FILE_NAME fileName = { 0 };
    PWSTR filePath = NULL;

    ntStatus = IoWC16StringCreateFromCString(&filePath, pszPath);
    GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);

    fileName.FileName = filePath;

    ntStatus = IoCreateFile(&fileHandle,
                            NULL,
                            &ioStatusBlock,
                            &fileName,
                            0,
                            0,
                            0,
                            0,
                            0,
                            0,
                            NULL,
                            NULL,
                            NULL);
    GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);

cleanup:
    IO_FREE(&filePath);

    if (fileHandle)
    {
        IoCloseFile(fileHandle);
    }

    IO_LOG_ENTER_LEAVE_STATUS_EE(ntStatus, EE);
}
#else
#define DoTest(pszPath)
#endif

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
    IO_DEVICE_HANDLE deviceHandle = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != InterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);
    }

    ntStatus = IoDriverInitialize(DriverHandle,
                                  NULL,
                                  DriverShutdown,
                                  DriverDispatch);
    GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);

    ntStatus = IoDeviceCreate(&deviceHandle,
                              DriverHandle,
                              "pvfs",
                              NULL);
    GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);

    DoTest("/pvfs");

cleanup:
    IO_LOG_ENTER_LEAVE_STATUS_EE(ntStatus, EE);
    return ntStatus;
}
