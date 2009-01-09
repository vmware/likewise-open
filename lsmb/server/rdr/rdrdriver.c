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
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "pvfs.h"

#define DO_TEST
//#undef DO_TEST

#ifdef DO_TEST
// TODO-Remove this test code once iomgr2 is further along.
#include "ioapi.h"
#include "iostring.h"

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
            ntStatus = RdrCreate(
                            DeviceHandle,
                            pIrp
                            );
            break;

        case IRP_TYPE_CLOSE:
            ntStatus = RdrClose(
                            DeviceHandle,
                            pIrp
                            );
            break;


        case IRP_TYPE_READ:
            ntStatus = RdrRead(
                            DeviceHandle,
                            pIrp
                            );
             break;

        case IRP_TYPE_WRITE:
            ntStatus = RdrWrite(
                            DeviceHandle,
                            pIrp
                            );
            break;

        case IRP_TYPE_IO_CONTROL:
            break;

        case IRP_TYPE_FS_CONTROL:
            ntStatus = RdrFsCtrl(
                            DeviceHandle,
                            pIrp
                            );
            break;
        case IRP_TYPE_FLUSH:
        case IRP_TYPE_QUERY_INFORMATION:
            ntStatus = RdrQueryInformation(
                            DeviceHandle,
                            pIrp
                            );
            break;
        case IRP_TYPE_SET_INFORMATION:
            ntStatus = RdrSetInformation(
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
                              "rdr",
                              NULL);
    GOTO_CLEANUP_ON_STATUS_EE(ntStatus, EE);

    DoTest("/rdr");

cleanup:
    IO_LOG_ENTER_LEAVE_STATUS_EE(ntStatus, EE);
    return ntStatus;
}
