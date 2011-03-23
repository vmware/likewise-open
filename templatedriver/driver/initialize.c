/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software
 * All rights reserved.
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
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        initialize.c
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - templatedriver
 *
 *        Initialization routines definition
 *
 * Authors: Evgeny Popovich (epopovich@likewise.com)
 */

#include "includes.h"

/* forward declarations */

static
NTSTATUS
TemplateDriverReadConfig(
    PLWIO_TEMPLATEDRIVER_CONFIG pConfig
    );

static
NTSTATUS
TemplateDriverReadConfigUlong(
    HANDLE hRegConnection,
    HKEY hKey,
    PSTR pszValueName,
    ULONG ulMinValue,
    ULONG ulMaxValue,
    PULONG pulResult
    );

/* implementation */

NTSTATUS
TemplateDriverInitialize(
    IO_DEVICE_HANDLE hDevice
    )
{
    NTSTATUS ntStatus = 0;

    TEMPLATEDRIVER_LOG_INFO("%s", __func__);

    memset(&gTemplateDriverGlobals, 0, sizeof(gTemplateDriverGlobals));

    pthread_mutex_init(&gTemplateDriverGlobals.mutex, NULL);
    gTemplateDriverGlobals.pMutex = &gTemplateDriverGlobals.mutex;

    ntStatus = TemplateDriverReadConfig(&gTemplateDriverGlobals.config);
    BAIL_ON_NT_STATUS(ntStatus);

    gTemplateDriverGlobals.hDevice = hDevice;

error:

    return ntStatus;
}

// Sample registry access
static
NTSTATUS
TemplateDriverReadConfig(
    PLWIO_TEMPLATEDRIVER_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    HANDLE hRegConnection = NULL;
    HKEY hKey = NULL;

    ntStatus = NtRegOpenServer(&hRegConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwNtRegOpenKeyExA(
                    hRegConnection,
                    NULL,
                    HKEY_THIS_MACHINE "Services\\lwio\\Parameters\\Drivers\\templatedriver",
                    0,
                    KEY_READ,
                    &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    // Ignore error as it may not exist; we can still use default.
    ntStatus = TemplateDriverReadConfigUlong(hRegConnection, hKey, "SampleKey",
                                  0, 1000000, &pConfig->ulSampleValue);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hRegConnection)
    {
        if (hKey)
        {
            ntStatus = LwNtRegCloseKey(hRegConnection, hKey);
            ntStatus = STATUS_SUCCESS;
        }

        NtRegCloseServer(hRegConnection);
    }

    return ntStatus;

error:

    TEMPLATEDRIVER_LOG_ERROR("Invalid TEMPLATEDRIVER registry configuration [error code: %u]",
                             ntStatus);

    goto cleanup;
}

VOID
TemplateDriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    TEMPLATEDRIVER_LOG_INFO("%s", __func__);

    if (gTemplateDriverGlobals.pMutex)
    {
        pthread_mutex_lock(gTemplateDriverGlobals.pMutex);

        // Shutdown driver-specific structures here
        // Make sure not to bail out without unlocking

        pthread_mutex_unlock(gTemplateDriverGlobals.pMutex);
        gTemplateDriverGlobals.pMutex = NULL;
    }

    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (gTemplateDriverGlobals.hDevice)
    {
        IoDeviceDelete(&gTemplateDriverGlobals.hDevice);
    }

    return;

error:

    TEMPLATEDRIVER_LOG_ERROR("[templatedriver] driver failed to stop. [code: %d]",
                   ntStatus);
    
    goto cleanup;
}

NTSTATUS
TemplateDriverRefresh(
    IN IO_DRIVER_HANDLE DriverHandle
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    TEMPLATEDRIVER_LOG_INFO("%s", __func__);

    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}


static
NTSTATUS
TemplateDriverReadConfigUlong(
    HANDLE hRegConnection,
    HKEY hKey,
    PSTR pszValueName,
    ULONG ulMinValue,
    ULONG ulMaxValue,
    PULONG pulResult
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG ulResult = 0;
    ULONG ulResultSize = sizeof(ulResult);

    ntStatus = LwNtRegGetValueA(hRegConnection, hKey, NULL, pszValueName,
                                RRF_RT_REG_DWORD, NULL, &ulResult,
                                &ulResultSize);
    BAIL_ON_NT_STATUS(ntStatus);

    if (ulResult < ulMinValue || ulResult > ulMaxValue)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    *pulResult = ulResult;

    return ntStatus;

error:

    TEMPLATEDRIVER_LOG_ERROR("Invalid TEMPLATEDRIVER registry configuration when reading %s"
                   ". [error code: %u]", pszValueName, ntStatus);

    ulResult = 0;

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

