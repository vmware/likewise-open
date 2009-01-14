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
 *        IO Test (IT) Driver
 *
 *        Driver Entry Function
 *
 * Authors: Danilo Almeida (dalmeida@likewisesoftware.com)
 */

#include "includes.h"

#include "ioapi.h"
#include <lw/rtlstring.h>

static
NTSTATUS
DoStartupTest(
    PCSTR pszPath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_FILE_HANDLE fileHandle = NULL;
    IO_STATUS_BLOCK ioStatusBlock = { 0 };
    IO_FILE_NAME fileName = { 0 };
    PWSTR filePath = NULL;

    status = RtlWC16StringAllocateFromCString(&filePath, pszPath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    fileName.FileName = filePath;

    status = IoCreateFile(&fileHandle,
                          NULL,
                          &ioStatusBlock,
                          NULL,
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
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    RtlWC16StringFree(&filePath);

    if (fileHandle)
    {
        IoCloseFile(fileHandle);
    }

    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}

VOID
ItDriverShutdown(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    IO_LOG_ENTER_LEAVE("");
}

NTSTATUS
ItDriverDispatch(
    IN IO_DEVICE_HANDLE DeviceHandle,
    IN PIRP pIrp
    )
{
    NTSTATUS status = STATUS_NOT_IMPLEMENTED;
    int EE = 0;

    switch (pIrp->Type)
    {
        case IRP_TYPE_CREATE:
            status = ItDispatchCreate(pIrp);
            break;

        case IRP_TYPE_CLOSE:
            status = ItDispatchClose(pIrp);
            break;

        case IRP_TYPE_READ:
            status = ItDispatchRead(pIrp);
            break;

        case IRP_TYPE_WRITE:
            status = ItDispatchWrite(pIrp);
            break;

        case IRP_TYPE_DEVICE_IO_CONTROL:
            status = ItDispatchDeviceIoControl(pIrp);
            break;

        case IRP_TYPE_FS_CONTROL:
            status = ItDispatchFsControl(pIrp);
            break;

        case IRP_TYPE_FLUSH_BUFFERS:
            status = ItDispatchFlushBuffers(pIrp);
            break;

        case IRP_TYPE_QUERY_INFORMATION:
            status = ItDispatchQueryInformation(pIrp);
            break;

        case IRP_TYPE_SET_INFORMATION:
            status = ItDispatchSetInformation(pIrp);
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    LOG_LEAVE_IF_STATUS_EE_EX(status, EE, "Type = %u", pIrp->Type);
    return status;
}

// TODO -- Add driver context
// TODO -- Add device context

NTSTATUS
DriverEntry(
    IN IO_DRIVER_HANDLE DriverHandle,
    IN ULONG InterfaceVersion
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    IO_DEVICE_HANDLE deviceHandle = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != InterfaceVersion)
    {
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = IoDriverInitialize(DriverHandle,
                                NULL,
                                ItDriverShutdown,
                                ItDriverDispatch);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IoDeviceCreate(&deviceHandle,
                            DriverHandle,
                            IOTEST_DEVICE_NAME,
                            NULL);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    assert(DoStartupTest(IOTEST_DEVICE_PATH) != STATUS_SUCCESS);
    assert(DoStartupTest(IOTEST_PATH_ALLOW) == STATUS_SUCCESS);

cleanup:
    LOG_LEAVE_IF_STATUS_EE(status, EE);
    return status;
}
