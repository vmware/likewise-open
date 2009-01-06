#include "config.h"
#include "lsmbsys.h"

//#include "lsmb/lsmb.h"

//#include "smbdef.h"

#include "iomgr2.h"
#include "lwlist.h"
#include "goto.h"
#include "smbutils.h"

#define IO_LOG_ENTER(Format, ...) \
    SMB_LOG_DEBUG("ENTER: " Format, ## __VA_ARGS__)

#define IO_LOG_LEAVE(Format, ...) \
    SMB_LOG_DEBUG("LEAVE: " Format, ## __VA_ARGS__)

#define IO_LOG_LEAVE_STATUS_ON_EE(status, EE) \
    do { \
        if (EE || status) \
        { \
            IO_LOG_LEAVE("-> 0x%08x (EE = %d)", status, EE); \
        } \
    } while (0)

typedef struct _IOP_DRIVER_CONFIG {
    PSTR pszName;
    PSTR pszPath;
    LW_LIST_LINKS Links;
} IOP_DRIVER_CONFIG, *PIOP_DRIVER_CONFIG;

typedef struct _IOP_CONFIG {
    LW_LIST_LINKS DriverConfigList;
    ULONG DriverCount;
} IOP_CONFIG, *PIOP_CONFIG;

typedef struct _IOP_CONFIG_PARSE_STATE {
    PIOP_CONFIG pConfig;
    PIOP_DRIVER_CONFIG pDriverConfig;
    NTSTATUS Status;
} IOP_CONFIG_PARSE_STATE, *PIOP_CONFIG_PARSE_STATE;

typedef struct _IOP_ROOT_STATE {
    PIOP_CONFIG Config;
    // Diagnostics Only
    ULONG DriverCount;
    LW_LIST_LINKS DriverObjectList;
    // Diagnostics Only
    ULONG DeviceCount;
    // Should really be a hash table...
    LW_LIST_LINKS DeviceObjectList;
} IOP_ROOT_STATE, *PIOP_ROOT_STATE;

typedef ULONG IO_DRIVER_OBJECT_FLAGS;

#define IO_DRIVER_OBJECT_FLAG_INITIALIZED 0x00000001
#define IO_DRIVER_OBJECT_FLAG_READY       0x00000002

struct _IO_DRIVER_OBJECT {
    LONG ReferenceCount;
    IO_DRIVER_OBJECT_FLAGS Flags;
    PIOP_ROOT_STATE Root;
    PIOP_DRIVER_CONFIG Config;
    LW_LIST_LINKS DeviceList;
    LW_LIST_LINKS DriverObjectsListLinks;

    PVOID LibraryHandle;
    PIO_DRIVER_ENTRY DriverEntry;
    struct {
        PIO_DRIVER_SHUTDOWN_CALLBACK Shutdown;
        PIO_DRIVER_DISPATCH_CALLBACK Dispatch;
    } Callback;
    PVOID Context;
};

struct _IO_DEVICE_OBJECT {
    LONG ReferenceCount;
    PCSTR DeviceName;
    LW_LIST_LINKS IrpList;
    PVOID Context;
};

PIOP_ROOT_STATE gpIoRoot = NULL;

PVOID
IoAllocate(
    IN size_t Size
    )
{
    PVOID pointer = NULL;
    SMBAllocateMemory(Size, &pointer);
    return pointer;
}


VOID
IoFree(
    PVOID Pointer
    )
{
    SMBFreeMemory(Pointer);
}

#define IO_ALLOCATE(PointerToPointer, Type, Size) \
    ( (*PointerToPointer) = (Type*) IoAllocate(Size), (*PointerToPointer) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES )

#define IO_FREE(PointerToPointer) \
    do { \
        if (*(PointerToPointer)) \
        { \
            IoFree(*(PointerToPointer)); \
            (*(PointerToPointer)) = NULL; \
        } \
    } while (0)

#define IOP_CONFIG_TAG_DRIVER "driver:"

static
NTSTATUS
IopDuplicateString(
    OUT PSTR* ppszNewString,
    IN PCSTR pszOriginalString
    )
{
    return SMBAllocateString(pszOriginalString, ppszNewString) ? STATUS_INSUFFICIENT_RESOURCES : STATUS_SUCCESS;
}

static
VOID
IopDriverConfigFree(
    IN OUT PIOP_DRIVER_CONFIG* ppDriverConfig
    )
{
    PIOP_DRIVER_CONFIG pDriverConfig = *ppDriverConfig;

    if (pDriverConfig)
    {
        IO_FREE(&pDriverConfig->pszName);
        IO_FREE(&pDriverConfig->pszPath);
        IoFree(pDriverConfig);
        *ppDriverConfig = NULL;
    }
}

static
DWORD
IopConfigParseStartSection(
    IN PCSTR pszSectionName,
    IN PVOID pData,
    OUT PBOOLEAN pbSkipSection,
    OUT PBOOLEAN pbContinue
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;
    PIOP_CONFIG_PARSE_STATE pState = (PIOP_CONFIG_PARSE_STATE) pData;
    PIOP_DRIVER_CONFIG pDriverConfig = NULL;
    PCSTR pszName = NULL;
    PLW_LIST_LINKS pLinks = NULL;

    assert(pszSectionName);
    assert(pState);
    assert(!pState->pDriverConfig);

    if (strncasecmp(pszSectionName, IOP_CONFIG_TAG_DRIVER, sizeof(IOP_CONFIG_TAG_DRIVER)-1))
    {
        bSkipSection = TRUE;
        GOTO_CLEANUP_EE(EE);
    }

    pszName = pszSectionName + sizeof(IOP_CONFIG_TAG_DRIVER) - 1;
    if (IsNullOrEmptyString(pszName))
    {
        SMB_LOG_ERROR("No driver name was specified");

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    // Check for duplicate driver config.
    for (pLinks = pState->pConfig->DriverConfigList.Next;
         pLinks != &pState->pConfig->DriverConfigList;
         pLinks = pLinks->Next)
    {
        PIOP_DRIVER_CONFIG pCheckDriverConfig = LW_STRUCT_FROM_FIELD(pLinks, IOP_DRIVER_CONFIG, Links);
        if (!strcasecmp(pCheckDriverConfig->pszName, pszName))
        {
            SMB_LOG_ERROR("Duplicate driver name '%s'", pszName);

            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }
    }

    status = IO_ALLOCATE(&pDriverConfig, IOP_DRIVER_CONFIG, sizeof(*pDriverConfig));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopDuplicateString(&pDriverConfig->pszName, pszName);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInsertTail(&pState->pConfig->DriverConfigList, &pDriverConfig->Links);
    pState->pConfig->DriverCount++;

    pState->pDriverConfig = pDriverConfig;

cleanup:
    if (status)
    {
        pState->Status = status;

        IopDriverConfigFree(&pDriverConfig);

        bContinue = FALSE;
        bSkipSection = TRUE;
    }

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    // TODO -- Error code mismatch?
    return status;
}

static
DWORD
IopConfigParseEndSection(
    IN PCSTR pszSectionName,
    IN PVOID pData,
    OUT PBOOLEAN pbContinue
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIOP_CONFIG_PARSE_STATE pState = (PIOP_CONFIG_PARSE_STATE) pData;
    BOOLEAN bContinue = TRUE;

    assert(pszSectionName);
    assert(pState);
    assert(pState->pDriverConfig);

    // Finished last driver, if any.
    if (pState->pDriverConfig)
    {
        if (!pState->pDriverConfig->pszPath)
        {
            status = STATUS_UNSUCCESSFUL;
            GOTO_CLEANUP_EE(EE);
        }
        pState->pDriverConfig = NULL;
    }

cleanup:
    if (status)
    {
        pState->Status = status;
        bContinue = FALSE;
    }

    *pbContinue = bContinue;

    // TODO -- Error code mismatch?
    return status;

}

static
DWORD
IopConfigParseNameValuePair(
    IN PCSTR pszName,
    IN PCSTR pszValue,
    IN PVOID pData,
    OUT PBOOLEAN pbContinue
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIOP_CONFIG_PARSE_STATE pState = (PIOP_CONFIG_PARSE_STATE) pData;
    BOOLEAN bContinue = TRUE;

    assert(pszName);
    assert(pszValue);
    assert(pState);
    assert(pState->pDriverConfig);

    if (strcasecmp(pszName, "path"))
    {
        GOTO_CLEANUP_EE(EE);
    }

    if (pState->pDriverConfig->pszPath)
    {
        SMB_LOG_ERROR("Path for driver '%s' is already defined as '%s'",
                      pState->pDriverConfig->pszName,
                      pState->pDriverConfig->pszPath);

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    if (IsNullOrEmptyString(pszValue))
    {
        SMB_LOG_ERROR("Empty path for driver '%s'",
                      pState->pDriverConfig->pszName);

        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_EE(EE);
    }

    status = IopDuplicateString(&pState->pDriverConfig->pszPath,
                                pszValue);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

cleanup:
    if (status)
    {
        pState->Status = status;

        bContinue = FALSE;
    }

    *pbContinue = bContinue;

    // TODO -- Error code mismatch?
    return status;
}

static
VOID
IopConfigFree(
    IN OUT PIOP_CONFIG* ppConfig
    )
{
    PIOP_CONFIG pConfig = *ppConfig;
    if (pConfig)
    {
        PLW_LIST_LINKS pLinks = NULL;

        for (pLinks = pConfig->DriverConfigList.Next;
             pLinks != &pConfig->DriverConfigList;
             pLinks = pLinks->Next)
        {
            PIOP_DRIVER_CONFIG pDriverConfig = LW_STRUCT_FROM_FIELD(pLinks, IOP_DRIVER_CONFIG, Links);

            LwListRemove(pLinks);
            IopDriverConfigFree(&pDriverConfig);
        }
        IoFree(pConfig);
        *ppConfig = NULL;
    }
}

static
NTSTATUS
IopConfigParse(
    OUT PIOP_CONFIG* ppConfig,
    IN PCSTR pszConfigFilePath
    )
{
    NTSTATUS status = 0;
    DWORD dwError = 0;
    PIOP_CONFIG pConfig = NULL;
    IOP_CONFIG_PARSE_STATE parseState = { 0 };

    status = IO_ALLOCATE(&pConfig, IOP_CONFIG, sizeof(*pConfig));
    GOTO_CLEANUP_ON_STATUS(status);

    parseState.pConfig = pConfig;

    dwError = SMBParseConfigFile(
                    pszConfigFilePath,
                    SMB_CFG_OPTION_STRIP_ALL,
                    IopConfigParseStartSection,
                    NULL,
                    IopConfigParseNameValuePair,
                    IopConfigParseEndSection,
                    &parseState);
    if (dwError)
    {
        // TODO-Error code issues?
        status = STATUS_UNSUCCESSFUL;
        GOTO_CLEANUP_ON_STATUS(status);
    }

    status = parseState.Status;
    GOTO_CLEANUP_ON_STATUS(status);

    assert(!parseState.pDriverConfig);

cleanup:
    assert(!(dwError && !status));

    if (status)
    {
        IopConfigFree(&pConfig);
    }

    *ppConfig = pConfig;

    return status;
}

static
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

static
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

static
VOID
IopRootFree(
    IN OUT PIOP_ROOT_STATE* ppRoot
    )
{
    PIOP_ROOT_STATE pRoot = *ppRoot;

    if (pRoot)
    {
        PLW_LIST_LINKS pLinks = NULL;

        // Unload drivers in reverse load order
        for (pLinks = pRoot->DriverObjectList.Prev;
             pLinks != &pRoot->DriverObjectList;
             pLinks = pLinks->Prev)
        {
            PIO_DRIVER_OBJECT pDriverObject = LW_STRUCT_FROM_FIELD(pLinks, IO_DRIVER_OBJECT, DriverObjectsListLinks);

            LwListRemove(pLinks);
            IopDriverUnload(&pDriverObject);
        }

        IopConfigFree(&pRoot->Config);
    }
}

static
NTSTATUS
IopRootCreate(
    OUT PIOP_ROOT_STATE* ppRoot,
    IN PCSTR pszConfigFilePath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PIOP_CONFIG pConfig = NULL;
    PLW_LIST_LINKS pLinks = NULL;
    PIO_DRIVER_OBJECT pDriverObject = NULL;
    PIOP_ROOT_STATE pRoot = NULL;

    status = IO_ALLOCATE(&pRoot, IOP_ROOT_STATE, sizeof(*pRoot));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    status = IopConfigParse(&pRoot->Config, pszConfigFilePath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    for (pLinks = pConfig->DriverConfigList.Next;
         pLinks != &pConfig->DriverConfigList;
         pLinks = pLinks->Next)
    {
        PIOP_DRIVER_CONFIG pDriverConfig = LW_STRUCT_FROM_FIELD(pLinks, IOP_DRIVER_CONFIG, Links);

        status = IopDriverLoad(&pDriverObject, pRoot, pDriverConfig);
        GOTO_CLEANUP_ON_STATUS_EE(status, EE);
    }

cleanup:
    if (status)
    {
        IopRootFree(&pRoot);
    }

    *ppRoot = pRoot;

    IO_LOG_LEAVE_STATUS_ON_EE(status, EE);
    return status;
}

VOID
IoCleanup(
    )
{
    IopRootFree(&gpIoRoot);
}

NTSTATUS
IoInitialize(
    IN PCSTR pszConfigFilePath
    )
{
    return IopRootCreate(&gpIoRoot, pszConfigFilePath);
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


