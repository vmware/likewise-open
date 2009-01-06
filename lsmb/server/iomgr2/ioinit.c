#include "iop.h"

PIOP_ROOT_STATE gpIoRoot = NULL;

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
