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
 *        Likewise IO (LWIO)
 *
 *        Reference Statistics Logging Module (SRV)
 *
 *        Configuration
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
NTSTATUS
LwioSrvStatConfigRead(
    PSRV_STAT_HANDLER_CONFIG pConfig
    );

static
NTSTATUS
LwioSrvStatConfigInitContents(
    PSRV_STAT_HANDLER_CONFIG pConfig
    );

static
VOID
LwioSrvStatConfigTransferContents(
    PSRV_STAT_HANDLER_CONFIG pSrcConfig,
    PSRV_STAT_HANDLER_CONFIG pDstConfig
    );

NTSTATUS
LwioSrvStatConfigInit(
    VOID
    )
{
    NTSTATUS        ntStatus = STATUS_SUCCESS;
    SRV_STAT_HANDLER_CONFIG config   = {0};

    ntStatus = LwioSrvStatConfigInitContents(&config);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwioSrvStatConfigRead(&config);
    if (ntStatus == STATUS_SUCCESS)
    {
        LwioSrvStatConfigTransferContents(
                &config,
                &gSrvStatHandlerGlobals.config);
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    LwioSrvStatConfigClearContents(&config);

    ntStatus = STATUS_SUCCESS;

    goto cleanup;
}

static
NTSTATUS
LwioSrvStatConfigRead(
    PSRV_STAT_HANDLER_CONFIG pConfig
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PCSTR  pszConfigKey   = "Software\\Likewise\\lwiosrvstat";
    PCSTR  pszTargetName  = "Target";
    PSTR   pszTargetValue = NULL;
    PCSTR  pszPathName    = "Path";
    PSTR   pszPathValue   = NULL;
    HANDLE hConnection    = NULL;
    HKEY   hKey           = NULL;
    char   szValue[MAX_VALUE_LENGTH] = {0};
    ULONG  ulType         = 0;
    ULONG  ulSize         = 0;
    SRV_STAT_LOG_TARGET_TYPE logTargetType = SRV_STAT_LOG_TARGET_TYPE_NONE;

    ntStatus = NtRegOpenServer(&hConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = NtRegOpenKeyExA(
                    hConnection,
                    NULL,
                    HKEY_THIS_MACHINE,
                    0,
                    KEY_READ,
                    &hKey);
    BAIL_ON_NT_STATUS(ntStatus);

    ulSize = sizeof(szValue);

    ntStatus = NtRegGetValueA(
                    hConnection,
                    hKey,
                    pszConfigKey,
                    pszTargetName,
                    RRF_RT_REG_SZ,
                    &ulType,
                    szValue,
                    &ulSize);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCStringDuplicate(&pszTargetValue, szValue);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!strcasecmp(pszTargetValue, "file"))
    {
        logTargetType = SRV_STAT_LOG_TARGET_TYPE_FILE;
    }
    else
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (logTargetType == SRV_STAT_LOG_TARGET_TYPE_FILE)
    {
        ulSize = sizeof(szValue);

        ntStatus = NtRegGetValueA(
                        hConnection,
                        hKey,
                        pszConfigKey,
                        pszPathName,
                        RRF_RT_REG_SZ,
                        &ulType,
                        szValue,
                        &ulSize);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!strlen(szValue))
        {
            ntStatus = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = LwRtlCStringDuplicate(&pszPathValue, szValue);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pConfig->logTargetType = logTargetType;
    RTL_FREE(&pConfig->pszPath);
    pConfig->pszPath = pszPathValue;

cleanup:

    if (hConnection)
    {
        if ( hKey )
        {
            NtRegCloseKey(hConnection, hKey);
        }
        NtRegCloseServer(hConnection);
    }

    RTL_FREE(&pszTargetValue);

    return ntStatus;

error:

    RTL_FREE(&pszPathValue);

    goto cleanup;
}

static
NTSTATUS
LwioSrvStatConfigInitContents(
    PSRV_STAT_HANDLER_CONFIG pConfig
    )
{
    memset(pConfig, 0, sizeof(*pConfig));

    pConfig->logTargetType = SRV_STAT_LOG_TARGET_TYPE_NONE;

    return STATUS_SUCCESS;
}

static
VOID
LwioSrvStatConfigTransferContents(
    PSRV_STAT_HANDLER_CONFIG pSrcConfig,
    PSRV_STAT_HANDLER_CONFIG pDstConfig
    )
{
    LwioSrvStatConfigClearContents(pDstConfig);
    *pDstConfig = *pSrcConfig;
    memset(pSrcConfig, 0, sizeof(*pSrcConfig));
}

VOID
LwioSrvStatConfigShutdown(
    VOID
    )
{
    LwioSrvStatConfigClearContents(&gSrvStatHandlerGlobals.config);
}

VOID
LwioSrvStatConfigClearContents(
    PSRV_STAT_HANDLER_CONFIG pConfig
    )
{
    RTL_FREE(&pConfig->pszPath);
    memset(pConfig, 0, sizeof(*pConfig));
}
