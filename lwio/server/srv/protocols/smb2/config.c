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
 *        SMB V2 API
 *
 *        Configuration
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
SrvConfigGetDefaults_SMB_V2(
    OUT PSRV_CONFIG_SMB_V2 pConfig
    );

static
VOID
SrvConfigFree_SMB_V2(
    IN PSRV_CONFIG_SMB_V2 pConfig
    );

static
VOID
SrvConfigFreeContents_SMB_V2(
    IN PSRV_CONFIG_SMB_V2 pConfig
    );

NTSTATUS
SrvConfigSetupInitial_SMB_V2(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    gProtocolGlobals_SMB_V2.pConfigLock = &gProtocolGlobals_SMB_V2.configLock;

    pthread_rwlock_init(gProtocolGlobals_SMB_V2.pConfigLock, NULL);

    ntStatus = SrvConfigRefresh_SMB_V2();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    pthread_rwlock_destroy(gProtocolGlobals_SMB_V2.pConfigLock);
    gProtocolGlobals_SMB_V2.pConfigLock = NULL;
    goto cleanup;
}

NTSTATUS
SrvConfigRefresh_SMB_V2(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_CONFIG_SMB_V2 Config = {};
    PSRV_CONFIG_SMB_V2 pConfig = &Config;
    LWIO_CONFIG_TABLE ConfigTable[] = SRV_CONFIG_TABLE_INITIALIZER_SMB_V2;
    DWORD dwNumConfig = sizeof(ConfigTable) / sizeof(LWIO_CONFIG_TABLE);
    BOOLEAN bInLock = FALSE;

    SrvConfigGetDefaults_SMB_V2(pConfig);

    ntStatus = LwIoProcessConfig(
                    "Services\\lwio\\Parameters\\Drivers\\srv\\smb2",
                    "Policy\\Services\\lwio\\Parameters\\Drivers\\srv\\smb2",
                    ConfigTable,
                    dwNumConfig,
                    TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, gProtocolGlobals_SMB_V2.pConfigLock);
    SrvConfigFree_SMB_V2(&gProtocolGlobals_SMB_V2.config);
    memcpy(&gProtocolGlobals_SMB_V2.config, pConfig, sizeof(SRV_CONFIG_SMB_V2));
    LWIO_UNLOCK_RWMUTEX(bInLock, gProtocolGlobals_SMB_V2.pConfigLock);

cleanup:

    return ntStatus;

error:

    SrvConfigFree_SMB_V2(pConfig);
    goto cleanup;
}

static
VOID
SrvConfigGetDefaults_SMB_V2(
    OUT PSRV_CONFIG_SMB_V2 pConfig
    )
{
    /* Add new config parameters in alphabetic order. */
    pConfig->ulOplockTimeout     = LWIO_DEFAULT_TIMEOUT_MSECS_SMB_V2;
}

static
VOID
SrvConfigFree_SMB_V2(
    IN PSRV_CONFIG_SMB_V2 pConfig
    )
{
    SrvConfigFreeContents_SMB_V2(pConfig);
}

static
VOID
SrvConfigFreeContents_SMB_V2(
    IN PSRV_CONFIG_SMB_V2 pConfig
    )
{
    LWIO_CONFIG_TABLE ConfigTable[] = SRV_CONFIG_TABLE_INITIALIZER_SMB_V2;
    DWORD dwNumConfig = sizeof(ConfigTable) / sizeof(LWIO_CONFIG_TABLE);
    DWORD dwEntry = 0;

    for (dwEntry = 0; dwEntry < dwNumConfig; dwEntry++)
    {
        if (ConfigTable[dwEntry].Type == LwIoTypeString)
        {
            PSTR pszString = ConfigTable[dwEntry].pValue;
            if (pszString != NULL)
            {
                SrvFreeMemory(pszString);
            }
        }
        if (ConfigTable[dwEntry].Type == LwIoTypeMultiString)
        {
            PSTR *ppszStrings = ConfigTable[dwEntry].pValue;
            if (ppszStrings != NULL)
            {
                LwIoMultiStringFree(ppszStrings);
            }
        }

    }
}

/* Config Getter Functions */

ULONG
SrvConfigGetOplockTimeout_SMB_V2(
    VOID
    )
{
    ULONG   ulTimeout = 0;
    BOOLEAN bInLock   = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, gProtocolGlobals_SMB_V2.pConfigLock);

    ulTimeout = gProtocolGlobals_SMB_V2.config.ulOplockTimeout;

    LWIO_UNLOCK_RWMUTEX(bInLock, gProtocolGlobals_SMB_V2.pConfigLock);

    return ulTimeout;
}

VOID
SrvConfigShutdown_SMB_V2(
    VOID
    )
{
    SrvConfigFree_SMB_V2(&gProtocolGlobals_SMB_V2.config);

    if (gProtocolGlobals_SMB_V2.pConfigLock)
    {
        pthread_rwlock_destroy(gProtocolGlobals_SMB_V2.pConfigLock);
        gProtocolGlobals_SMB_V2.pConfigLock = NULL;
    }
}
