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
 *        SMB V1 API
 *
 *        Configuration
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 * Instructions:
 *
 *  To add a new SMBv1 configuration parameters:
 *
 *      1) In defs.h add a SRV_CONFIG_SMB_V1 initialization entry to
 *         the SRV_CONFIG_TABLE_INITIALIZER_SMB_V1 define
 *
 *      2) In defs.h add a define for the default value of the new parameter.
 *
 *      3) In structs.h add a variable to the _SRV_CONFIG_SMB_V1 struct
 *         in alphabetic order.
 *
 *      4) In config.c add an default initialization to
 *         SrvConfigGetDefaults_SMB_V1().
 *
 *      5) In lwio/etc/lwiod.reg.in add a default value to the registry import
 *         under the key:
 *
 *         [HKEY_THIS_MACHINE\Services\lwio\Parameters\Drivers\srv\smb1]
 *
 *      6) In prototypes.h add a getter function definition in the // config.c
 *         sectio of the file.
 *
 *      7) In config.c add a getter function definition at the end of the
 *         file.
 *
 *      8) Use the getter function to access the parameter in the rest of SRV.
 */

#include "includes.h"

/* Forward declarations */

static
VOID
SrvConfigGetDefaults_SMB_V1(
    OUT PSRV_CONFIG_SMB_V1 pConfig
    );

static
VOID
SrvConfigFree_SMB_V1(
    IN PSRV_CONFIG_SMB_V1 pConfig
    );

static
VOID
SrvConfigFreeContents_SMB_V1(
    IN PSRV_CONFIG_SMB_V1 pConfig
    );

/* File Globals */

/* Code */

NTSTATUS
SrvConfigSetupInitial_SMB_V1(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    gProtocolGlobals_SMB_V1.pConfigLock = &gProtocolGlobals_SMB_V1.configLock;

    pthread_rwlock_init(gProtocolGlobals_SMB_V1.pConfigLock, NULL);

    ntStatus = SrvConfigRefresh_SMB_V1();
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    pthread_rwlock_destroy(gProtocolGlobals_SMB_V1.pConfigLock);
    gProtocolGlobals_SMB_V1.pConfigLock = NULL;
    goto cleanup;
}

NTSTATUS
SrvConfigRefresh_SMB_V1(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    SRV_CONFIG_SMB_V1 Config = {};
    PSRV_CONFIG_SMB_V1 pConfig = &Config;
    LWIO_CONFIG_TABLE ConfigTable[] = SRV_CONFIG_TABLE_INITIALIZER_SMB_V1;
    DWORD dwNumConfig = sizeof(ConfigTable) / sizeof(LWIO_CONFIG_TABLE);
    BOOLEAN bInLock = FALSE;

    SrvConfigGetDefaults_SMB_V1(pConfig);

    ntStatus = LwIoProcessConfig(
        "Services\\lwio\\Parameters\\Drivers\\srv\\smb1",
        "Policy\\Services\\lwio\\Parameters\\Drivers\\srv\\smb1",
        ConfigTable,
        dwNumConfig,
        TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, gProtocolGlobals_SMB_V1.pConfigLock);
    SrvConfigFree_SMB_V1(&gProtocolGlobals_SMB_V1.config);
    memcpy(&gProtocolGlobals_SMB_V1.config, pConfig, sizeof(SRV_CONFIG_SMB_V1));
    LWIO_UNLOCK_RWMUTEX(bInLock, gProtocolGlobals_SMB_V1.pConfigLock);

cleanup:
    return ntStatus;

error:
    SrvConfigFree_SMB_V1(pConfig);
    goto cleanup;
}

VOID
SrvConfigShutdown_SMB_V1(
    VOID
    )
{
    SrvConfigFree_SMB_V1(&gProtocolGlobals_SMB_V1.config);

    if (gProtocolGlobals_SMB_V1.pConfigLock)
    {
        pthread_rwlock_destroy(gProtocolGlobals_SMB_V1.pConfigLock);
        gProtocolGlobals_SMB_V1.pConfigLock = NULL;
    }
}

static
VOID
SrvConfigGetDefaults_SMB_V1(
    OUT PSRV_CONFIG_SMB_V1 pConfig
    )
{
    assert(pConfig);

    /* Add new config parameters in alphabetic order. */
    pConfig->dwOplockTimeoutMillisecs = SRV_DEFAULT_TIMEOUT_MSECS_SMB_V1;
}

static
VOID
SrvConfigFree_SMB_V1(
    IN PSRV_CONFIG_SMB_V1 pConfig
    )
{
    SrvConfigFreeContents_SMB_V1(pConfig);
}

static
VOID
SrvConfigFreeContents_SMB_V1(
    IN PSRV_CONFIG_SMB_V1 pConfig
    )
{
    LWIO_CONFIG_TABLE ConfigTable[] = SRV_CONFIG_TABLE_INITIALIZER_SMB_V1;
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

DWORD
SrvConfigGetOplockTimeoutMillisecs_SMB_V1(
    VOID
    )
{
    DWORD dwTimeout = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, gProtocolGlobals_SMB_V1.pConfigLock);

    dwTimeout = gProtocolGlobals_SMB_V1.config.dwOplockTimeoutMillisecs;

    LWIO_UNLOCK_RWMUTEX(bInLock, gProtocolGlobals_SMB_V1.pConfigLock);

    return dwTimeout;
}
