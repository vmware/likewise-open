#include "iop.h"

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

        IopConfigFreeConfig(&pRoot->Config);
    }
}

NTSTATUS
IopRootCreate(
    OUT PIOP_ROOT_STATE* ppRoot,
    IN PCSTR pszConfigFilePath
    )
{
    NTSTATUS status = 0;
    int EE = 0;
    PLW_LIST_LINKS pLinks = NULL;
    PIO_DRIVER_OBJECT pDriverObject = NULL;
    PIOP_ROOT_STATE pRoot = NULL;

    status = IO_ALLOCATE(&pRoot, IOP_ROOT_STATE, sizeof(*pRoot));
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    LwListInit(&pRoot->DriverObjectList);
    LwListInit(&pRoot->DeviceObjectList);

    status = IopConfigParse(&pRoot->Config, pszConfigFilePath);
    GOTO_CLEANUP_ON_STATUS_EE(status, EE);

    for (pLinks = pRoot->Config->DriverConfigList.Next;
         pLinks != &pRoot->Config->DriverConfigList;
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
