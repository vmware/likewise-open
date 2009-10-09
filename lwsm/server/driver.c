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
    LWIO_DRIVER_STATUS driverStatus = 0;
    PSM_TABLE_ENTRY pLwioEntry = NULL;
    LW_SERVICE_STATUS lwioStatus;
    WCHAR wszLwio[] = {'l', 'w', 'i', 'o', '\0'};

    dwError = LwSmTableGetEntry(wszLwio, &pLwioEntry);
    BAIL_ON_ERROR(dwError);

    dwError = LwSmTableGetEntryStatus(pLwioEntry, &lwioStatus);
    BAIL_ON_ERROR(dwError);

    pStatus->home = LW_SERVICE_HOME_IO_MANAGER;
    pStatus->pid = lwioStatus.pid;

    switch (lwioStatus.state)
    {
    case LW_SERVICE_STATE_RUNNING:
        dwError = LwNtStatusToWin32Error(LwIoGetDriverStatus(
                                             pEntry->pInfo->pwszName,
                                             &driverStatus));
        BAIL_ON_ERROR(dwError);
        switch (driverStatus)
        {
        case LWIO_DRIVER_LOADED:
            pStatus->state = LW_SERVICE_STATE_RUNNING;
            break;
        case LWIO_DRIVER_UNLOADED:
            pStatus->state = LW_SERVICE_STATE_STOPPED;
            break;
        default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_ERROR(dwError);
            break;
        }
        break;
    case LW_SERVICE_STATE_DEAD:
        pStatus->state = LW_SERVICE_STATE_STOPPED;
        break;
    default:
        pStatus->state = LW_SERVICE_STATE_STOPPED;
        break;
    }

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
    .pfnRefresh = LwSmDriverRefresh,
    .pfnConstruct = LwSmDriverConstruct,
    .pfnDestruct = LwSmDriverDestruct
};
