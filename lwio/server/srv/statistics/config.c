/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        config.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Statistics Logging Module
 *
 *        Configuration
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvStatsConfigRead(
    PSRV_STATISTICS_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_STATISTICS_CONFIG config = { 0 };
    PLWIO_CONFIG_REG pReg = NULL;
    BOOLEAN bUsePolicy = TRUE;

    ntStatus = SrvStatsConfigInitContents(&config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwIoOpenConfig(
                "Services\\lwio\\Parameters\\Drivers\\srv\\statistics",
                "Policy\\Services\\lwio\\Parameters\\Drivers\\srv\\statistics",
                &pReg);
    if (ntStatus)
    {
        LWIO_LOG_ERROR(
            "Failed to access device statistics configuration [error code: %u]",
            ntStatus);

        ntStatus = STATUS_DEVICE_CONFIGURATION_ERROR;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    /* Ignore error as it may not exist; we can still use default. */
    LwIoReadConfigString(
            pReg,
            "Path",
            bUsePolicy,
            &config.pszProviderPath);

    LwIoReadConfigBoolean(
            pReg,
            "EnableLogging",
            bUsePolicy,
            &config.bEnableLogging);

    ntStatus = SrvStatsConfigTransferContents(&config, pConfig);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pReg)
    {
        LwIoCloseConfig(pReg);
    }

    return ntStatus;

error:

    SrvStatsConfigFreeContents(&config);

    goto cleanup;
}

NTSTATUS
SrvStatsConfigInitContents(
    PSRV_STATISTICS_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    SrvStatsConfigFreeContents(pConfig);

    pConfig->bEnableLogging  = FALSE;
    pConfig->pszProviderPath = NULL;

    return ntStatus;
}

NTSTATUS
SrvStatsConfigTransferContents(
    PSRV_STATISTICS_CONFIG pSrc,
    PSRV_STATISTICS_CONFIG pDest
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    SrvStatsConfigFreeContents(pDest);

    *pDest = *pSrc;

    memset(pSrc, 0, sizeof(*pSrc));

    return ntStatus;
}

BOOLEAN
SrvStatsConfigLoggingEnabled(
    VOID
    )
{
    BOOLEAN bEnabled = FALSE;
    BOOLEAN bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSrvStatGlobals.mutex);

    bEnabled = (gSrvStatGlobals.config.bEnableLogging &&
                gSrvStatGlobals.pStatFnTable);

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvStatGlobals.mutex);

    return bEnabled;
}

VOID
SrvStatsConfigFreeContents(
    PSRV_STATISTICS_CONFIG pConfig
    )
{
    LwRtlCStringFree(&pConfig->pszProviderPath);

    memset(pConfig, 0, sizeof(*pConfig));
}
