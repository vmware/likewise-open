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
    PSTR pszDeviceNameCopy = NULL;

    if (!DriverHandle)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    if (!IsNullOrEmptyString(pszName))
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    // TODO-Add locking

    pFoundDevice = IopRootFindDevice(DriverHandle->Root, pszName);
    if (pFoundDevice)
    {
        status = STATUS_DUPLICATE_NAME;
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

    status = IopDuplicateString(&pszDeviceNameCopy, pszName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IO_ALLOCATE(&pDeviceObject, IO_DEVICE_OBJECT, sizeof(*pDeviceObject));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    // cannot fail.

    pDeviceObject->ReferenceCount = 1;
    pDeviceObject->Driver = DriverHandle;
    pDeviceObject->DeviceName = pszDeviceNameCopy;
    pszDeviceNameCopy = NULL;
    pDeviceObject->Context = DeviceContext;
    LwListInit(&pDeviceObject->IrpList);

    IopDriverInsertDevice(pDeviceObject->Driver, &pDeviceObject->DriverLinks);
    IopRootInsertDevice(pDeviceObject->Driver->Root, &pDeviceObject->RootLinks);

cleanup:
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
        IO_FREE(&pDeviceObject->DeviceName);
        IoFree(pDeviceObject);
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

