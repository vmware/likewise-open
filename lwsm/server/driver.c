/*
 * Copyright (c) Likewise Software.  All rights Reserved.
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

/*
 * Module Name:
 *
 *        driver.c
 *
 * Abstract:
 *
 *        Logic for managing lwio drivers
 *
 * Authors: Brian Koropoff (bkoropoff@likewise.com)
 *
 */

#include "includes.h"

static
DWORD
LwSmDriverStart(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;

    dwError = LwNtStatusToWin32Error(LwIoLoadDriver(pEntry->pInfo->pwszName));
    BAIL_ON_ERROR(dwError);

    LwSmTableNotifyEntryChanged(pEntry);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmDriverStop(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;

    dwError = LwNtStatusToWin32Error(LwIoUnloadDriver(pEntry->pInfo->pwszName));
    BAIL_ON_ERROR(dwError);

    LwSmTableNotifyEntryChanged(pEntry);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmDriverGetStatus(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_STATUS pStatus
    )
{
    DWORD dwError = 0;
    LWIO_DRIVER_STATUS status = 0;

    dwError = LwNtStatusToWin32Error(LwIoGetDriverStatus(
                                         pEntry->pInfo->pwszName,
                                         &status));

    if (dwError)
    {
        *pStatus = LW_SERVICE_STOPPED;
        dwError = 0;
    }
    else switch (status)
    {
    case LWIO_DRIVER_LOADED:
        *pStatus = LW_SERVICE_RUNNING;
        break;
    case LWIO_DRIVER_UNLOADED:
        *pStatus = LW_SERVICE_STOPPED;
        break;
    default:
        dwError = LW_ERROR_INTERNAL;
        BAIL_ON_ERROR(dwError);
        break;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmDriverGetProcess(
    PSM_TABLE_ENTRY pEntry,
    PLW_SERVICE_PROCESS pProcess,
    pid_t* pPid
    )
{
    DWORD dwError = 0;
    WCHAR wszLwio[] = {'l', 'w', 'i', 'o', '\0'};
    PSM_TABLE_ENTRY pLwioEntry = NULL;

    dwError = LwSmTableGetEntry(wszLwio, &pLwioEntry);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmTableGetEntryProcess(pLwioEntry, pProcess, pPid);
    BAIL_ON_ERROR(dwError);

    *pProcess = LW_SERVICE_PROCESS_IO_MANAGER;

cleanup:

    if (pLwioEntry)
    {
        LwSmTableReleaseEntry(pLwioEntry);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LwSmDriverRefresh(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;

    return dwError;
}

static
DWORD
LwSmDriverConstruct(
    PSM_TABLE_ENTRY pEntry
    )
{
    DWORD dwError = 0;

    return dwError;
}

static
VOID
LwSmDriverDestruct(
    PSM_TABLE_ENTRY pEntry
    )
{
    return;
}

SM_OBJECT_VTBL gDriverVtbl =
{
    .pfnStart = LwSmDriverStart,
    .pfnStop = LwSmDriverStop,
    .pfnGetStatus = LwSmDriverGetStatus,
    .pfnGetProcess = LwSmDriverGetProcess,
    .pfnRefresh = LwSmDriverRefresh,
    .pfnConstruct = LwSmDriverConstruct,
    .pfnDestruct = LwSmDriverDestruct
};
