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

