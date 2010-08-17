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
 *        config.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Configuration
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
SrvElementsConfigGetDefaults(
    OUT PSRV_ELEMENTS_CONFIG pConfig
    );

static
VOID
SrvElementsConfigFree(
    IN PSRV_ELEMENTS_CONFIG pConfig
    );

static
VOID
SrvElementsConfigFreeContents(
    IN PSRV_ELEMENTS_CONFIG pConfig
    );

NTSTATUS
SrvElementsConfigSetupInitial(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    pthread_rwlock_init(&gSrvElements.configLock, NULL);
    gSrvElements.pConfigLock = &gSrvElements.configLock;

    ntStatus = SrvElementsConfigRefresh();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    SrvElementsConfigFree(&gSrvElements.config);

    goto cleanup;
}

NTSTATUS
SrvElementsConfigRefresh(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_ELEMENTS_CONFIG config = {};
    PSRV_ELEMENTS_CONFIG pConfig = &config;
    LWIO_CONFIG_TABLE configTable[] = SRV_ELEMENTS_CONFIG_TABLE_INITIALIZER;
    ULONG   ulNumConfig = sizeof(configTable) / sizeof(LWIO_CONFIG_TABLE);
    BOOLEAN bInLock = FALSE;

    SrvElementsConfigGetDefaults(pConfig);

    ntStatus = LwIoProcessConfig(
                    "Services\\lwio\\Parameters\\Drivers\\srv\\smb2",
                    "Policy\\Services\\lwio\\Parameters\\Drivers\\srv\\smb2",
                    configTable,
                    ulNumConfig,
                    TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSrvElements.configLock);

    SrvElementsConfigFree(&gSrvElements.config);
    memcpy(&gSrvElements.config, pConfig, sizeof(SRV_ELEMENTS_CONFIG));

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSrvElements.configLock);

cleanup:

    return ntStatus;

error:

    SrvElementsConfigFree(pConfig);

    goto cleanup;
}

static
VOID
SrvElementsConfigGetDefaults(
    PSRV_ELEMENTS_CONFIG pConfig
    )
{
    /* Add new config parameters in alphabetic order. */
    pConfig->bClientAddressEcpEnabled = FALSE;
    pConfig->bOEMSessionEcpEnabled    = FALSE;
    pConfig->bShareNameEcpEnabled     = TRUE;
    pConfig->usClientCreditLimit = LWIO_DEFAULT_CLIENT_CREDIT_LIMIT;
    pConfig->ulGlobalCreditLimit = LWIO_DEFAULT_GLOBAL_CREDIT_LIMIT;
}

static
VOID
SrvElementsConfigFree(
    PSRV_ELEMENTS_CONFIG pConfig
    )
{
    SrvElementsConfigFreeContents(pConfig);
}

static
VOID
SrvElementsConfigFreeContents(
    PSRV_ELEMENTS_CONFIG pConfig
    )
{
    LWIO_CONFIG_TABLE configTable[] = SRV_ELEMENTS_CONFIG_TABLE_INITIALIZER;
    ULONG ulNumConfig = sizeof(configTable) / sizeof(LWIO_CONFIG_TABLE);
    ULONG ulEntry = 0;

    for (ulEntry = 0; ulEntry < ulNumConfig; ulEntry++)
    {
        if (configTable[ulEntry].Type == LwIoTypeString)
        {
            PSTR pszString = configTable[ulEntry].pValue;
            if (pszString != NULL)
            {
                SrvFreeMemory(pszString);
            }
        }
        if (configTable[ulEntry].Type == LwIoTypeMultiString)
        {
            PSTR *ppszStrings = configTable[ulEntry].pValue;
            if (ppszStrings != NULL)
            {
                LwIoMultiStringFree(ppszStrings);
            }
        }

    }
}

/* Config Getter Functions */

ULONG
SrvElementsConfigGetGlobalCreditLimit(
    VOID
    )
{
    ULONG   ulCreditLimit = 0;
    BOOLEAN bInLock   = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, gSrvElements.pConfigLock);

    ulCreditLimit = gSrvElements.config.ulGlobalCreditLimit;

    LWIO_UNLOCK_RWMUTEX(bInLock, gSrvElements.pConfigLock);

    return ulCreditLimit;
}

BOOLEAN
SrvElementsGetShareNameEcpEnabled(
    VOID
    )
{
    BOOLEAN bEnabled = FALSE;
    BOOLEAN bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, gSrvElements.pConfigLock);

    bEnabled = gSrvElements.config.bShareNameEcpEnabled;

    LWIO_UNLOCK_RWMUTEX(bInLock, gSrvElements.pConfigLock);

    return bEnabled;
}

BOOLEAN
SrvElementsGetClientAddressEcpEnabled(
    VOID
    )
{
    BOOLEAN bEnabled = FALSE;
    BOOLEAN bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, gSrvElements.pConfigLock);

    bEnabled = gSrvElements.config.bClientAddressEcpEnabled;

    LWIO_UNLOCK_RWMUTEX(bInLock, gSrvElements.pConfigLock);

    return bEnabled;
}

BOOLEAN
SrvElementsGetOEMSessionEcpEnabled(
    VOID
    )
{
    BOOLEAN bEnabled = FALSE;
    BOOLEAN bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, gSrvElements.pConfigLock);

    bEnabled = gSrvElements.config.bOEMSessionEcpEnabled;

    LWIO_UNLOCK_RWMUTEX(bInLock, gSrvElements.pConfigLock);

    return bEnabled;
}

USHORT
SrvElementsConfigGetClientCreditLimit(
    VOID
    )
{
    USHORT   usCreditLimit = 0;
    BOOLEAN bInLock   = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, gSrvElements.pConfigLock);

    usCreditLimit = gSrvElements.config.usClientCreditLimit;

    LWIO_UNLOCK_RWMUTEX(bInLock, gSrvElements.pConfigLock);

    return usCreditLimit;
}

VOID
SrvElementsConfigShutdown(
    VOID
    )
{
    SrvElementsConfigFree(&gSrvElements.config);

    if (gSrvElements.pConfigLock)
    {
        pthread_rwlock_destroy(gSrvElements.pConfigLock);
        gSrvElements.pConfigLock = NULL;
    }
}
