#include "iop.h"

VOID
IopDriverUnload(
    IN OUT PIO_DRIVER_OBJECT* ppDriverObject
    )
{
    PIO_DRIVER_OBJECT pDriverObject = *ppDriverObject;

    if (pDriverObject)
    {
        if (IsSetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_READY))
        {
            // TODO -- Add code to cancel IO and wait for IO to complete.
        }
        if (IsSetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED))
        {
            pDriverObject->Callback.Shutdown(pDriverObject);

            // TODO -- Add code to remove devices
            // TODO -- refcount?
        }
        if (pDriverObject->LibraryHandle)
        {
            int err = dlclose(pDriverObject->LibraryHandle);
            if (err)
            {
                SMB_LOG_ERROR("Failed to dlclose() for driver '%s' from '%s'",
                              pDriverObject->Config->pszName,
                              pDriverObject->Config->pszPath);
            }
        }
        IoFree(pDriverObject);
        *ppDriverObject = NULL;
    }
}

NTSTATUS
IopDriverLoad(
    OUT PIO_DRIVER_OBJECT* ppDriverObject,
    IN PIOP_ROOT_STATE pRoot,
    IN PIOP_DRIVER_CONFIG pDriverConfig
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PCSTR pszError = NULL;
    PIO_DRIVER_OBJECT pDriverObject = NULL;
    PCSTR pszPath = pDriverObject->Config->pszPath;
    PCSTR pszName = pDriverObject->Config->pszName;

    status = IO_ALLOCATE(&pDriverObject, IO_DRIVER_OBJECT, sizeof(*pDriverObject));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInit(&pDriverObject->DriverObjectsListLinks);

    pDriverObject->ReferenceCount = 1;
    pDriverObject->Root = pRoot;
    pDriverObject->Config = pDriverConfig;

    dlerror();

    pDriverObject->LibraryHandle = dlopen(pszPath, RTLD_NOW | RTLD_GLOBAL);
    if (!pDriverObject->Config->pszPath)
    {
        pszError = dlerror();

        SMB_LOG_ERROR("Failed to load driver '%s' from '%s' (%s)",
                      pszName, pszPath, SMB_SAFE_LOG_STRING(pszError));

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    dlerror();
    pDriverObject->DriverEntry = (PIO_DRIVER_ENTRY)dlsym(pDriverObject->LibraryHandle, IO_DRIVER_ENTRY_FUNCTION_NAME);
    if (!pDriverObject->DriverEntry)
    {
        pszError = dlerror();

        SMB_LOG_ERROR("Failed to load " IO_DRIVER_ENTRY_FUNCTION_NAME " function for driver %s from %s (%s)",
                      pszName, pszPath, SMB_SAFE_LOG_STRING(pszError));

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    status = pDriverObject->DriverEntry(pDriverObject, IO_DRIVER_ENTRY_INTERFACE_VERSION);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    if (!IsSetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED))
    {
        SMB_LOG_ERROR(IO_DRIVER_ENTRY_FUNCTION_NAME " did not initialize driver '%s' from '%s'",
                      pszName, pszPath);

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    LwListInsertTail(&pRoot->DriverObjectList, &pDriverObject->DriverObjectsListLinks);
    SetFlag(pDriverObject->Flags, IO_DRIVER_OBJECT_FLAG_READY);

cleanup:
    if (status)
    {
        IopDriverUnload(&pDriverObject);
    }

    *ppDriverObject = pDriverObject;

    IO_LOG_LEAVE_STATUS_ON_EE(status, EE);
    return status;
}

NTSTATUS
IoDriverInitialize(
    IN OUT IO_DRIVER_HANDLE DriverHandle,
    IN OPTIONAL PVOID DriverContext,
    IN PIO_DRIVER_SHUTDOWN_CALLBACK ShutdownCallback,
    IN PIO_DRIVER_DISPATCH_CALLBACK DispatchCallback
    )
{
    NTSTATUS status = 0;
    int EE = 0;

    if (!ShutdownCallback || !DispatchCallback)
    {
        status = STATUS_INVALID_PARAMETER;
        GOTO_CLEANUP_EE(EE);
    }

    if (IsSetFlag(DriverHandle->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED))
    {
        // Already initialized.
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    DriverHandle->Callback.Shutdown = ShutdownCallback;
    DriverHandle->Callback.Dispatch = DispatchCallback;
    DriverHandle->Context = DriverContext;

    SetFlag(DriverHandle->Flags, IO_DRIVER_OBJECT_FLAG_INITIALIZED);

cleanup:
    IO_LOG_LEAVE_STATUS_ON_EE(status, EE);
    return status;
}

PCSTR
IoDriverGetName(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    return DriverHandle->Config->pszName;
}

PVOID
IoDriverGetContext(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    return DriverHandle->Context;
}
