#include "iop.h"

static volatile PIOP_ROOT_STATE gpIoRoot = NULL;

VOID
IoCleanup(
    )
{
    PIOP_ROOT_STATE pRoot = gpIoRoot;
    IopRootFree(&pRoot);
    gpIoRoot = pRoot;
}

NTSTATUS
IoInitialize(
    IN PCSTR pszConfigFilePath
    )
{
    NTSTATUS status = 0;
    PIOP_ROOT_STATE pRoot = NULL;

    status = IopRootCreate(&pRoot, pszConfigFilePath);
    GOTO_CLEANUP_ON_STATUS(status);

    gpIoRoot = pRoot;

    status = IopRootLoadDrivers(pRoot);
    GOTO_CLEANUP_ON_STATUS(status);

cleanup:
    if (status)
    {
        IopRootFree(&pRoot);
    }

    gpIoRoot = pRoot;

    return status;
}

NTSTATUS
IopParse(
    IN OUT PIO_FILE_NAME pFileName,
    OUT PIO_DEVICE_OBJECT* ppDevice
    )
{
    PIOP_ROOT_STATE pRoot = gpIoRoot;

    return IopRootParse(pRoot, pFileName, ppDevice);
}
