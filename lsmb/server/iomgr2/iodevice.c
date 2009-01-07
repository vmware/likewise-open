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

#include "iop.h"

NTSTATUS
IoDeviceCreate(
    OUT PIO_DEVICE_HANDLE pDeviceHandle,
    IN IO_DRIVER_HANDLE DriverHandle,
    IN PCSTR pszName,
    IN OPTIONAL PVOID DeviceContext
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIO_DEVICE_OBJECT pDeviceObject = NULL;
    PIO_DEVICE_OBJECT pFoundDevice = NULL;
    IO_UNICODE_STRING deviceName = { 0 };

    if (!DriverHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (IsNullOrEmptyString(pszName))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = IoUnicodeStringCreateFromCString(&deviceName, pszName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // TODO-Add locking

    pFoundDevice = IopRootFindDevice(DriverHandle->Root, &deviceName);
    if (pFoundDevice)
    {
        status = STATUS_DUPLICATE_NAME;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = IO_ALLOCATE(&pDeviceObject, IO_DEVICE_OBJECT, sizeof(*pDeviceObject));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // cannot fail.

    pDeviceObject->ReferenceCount = 1;
    pDeviceObject->Driver = DriverHandle;
    pDeviceObject->DeviceName = deviceName;
    IoMemoryZero(&deviceName, sizeof(deviceName));
    pDeviceObject->Context = DeviceContext;
    LwListInit(&pDeviceObject->IrpList);

    IopDriverInsertDevice(pDeviceObject->Driver, &pDeviceObject->DriverLinks);
    IopRootInsertDevice(pDeviceObject->Driver->Root, &pDeviceObject->RootLinks);

cleanup:
    IoUnicodeStringFree(&deviceName);

    IO_LOG_ENTER_LEAVE_STATUS_EE(status, EE);
    return status;
}

VOID
IoDeviceDelete(
    IN OUT PIO_DEVICE_HANDLE pDeviceHandle
    )
{
    PIO_DEVICE_OBJECT pDeviceObject = *pDeviceHandle;

    assert(pDeviceObject);

    if (pDeviceObject)
    {
        // TODO - tear down I/O
        // TODO - refcounts?
        IopDriverRemoveDevice(pDeviceObject->Driver, &pDeviceObject->DriverLinks);
        IopRootInsertDevice(pDeviceObject->Driver->Root, &pDeviceObject->RootLinks);
        IoUnicodeStringFree(&pDeviceObject->DeviceName);
        IoMemoryFree(pDeviceObject);
        *pDeviceHandle = NULL;
    }
}

PVOID
IoDeviceGetContext(
    IN IO_DEVICE_HANDLE DeviceHandle
    )
{
    return DeviceHandle->Context;
}

