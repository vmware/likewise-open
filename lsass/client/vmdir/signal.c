/*
 * Copyright (C) 2015 VMware, Inc. All rights reserved.
 */

#include "vmdirclient.h"

DWORD
LsaVmdirSignal(
    HANDLE hLsaConnection,
    DWORD dwFlags
    )
{
    DWORD dwError = 0;

    if (geteuid() != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderIoControl(
        hLsaConnection,
        LSA_PROVIDER_TAG_VMDIR,
        LSA_VMDIR_IO_SIGNAL,
        sizeof(dwFlags),
        &dwFlags,
        NULL,
        NULL);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}
