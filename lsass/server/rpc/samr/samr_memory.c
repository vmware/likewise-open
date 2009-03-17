#include "includes.h"


NTSTATUS
SamrSrvInitMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (!bSamrSrvInitialised && !pMemRoot) {
        pMemRoot = talloc(NULL, 0, NULL);
        BAIL_ON_NO_MEMORY(pMemRoot);
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


NTSTATUS
SamrSrvDestroyMemory(
    void
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;

    GLOBAL_DATA_LOCK(locked);

    if (bSamrSrvInitialised && pMemRoot) {
        tfree(pMemRoot);
    }

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return status;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
