/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        pam-config.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Pluggable Authentication Module
 *
 *        Configuration API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

#define LSA_PAM_LOGON_RIGHTS_DENIED_MESSAGE "Access denied"


static
LSA_PAM_CONFIG gStagingConfig;

#if 0
static const PCSTR gLogLevels[] =
{
    "disabled",
    "always",
    "error",
    "warning",
    "info",
    "verbose",
    "debug"
};

static
LSA_CONFIG gConfigDescription[] =
{
    {   "LogLevel",
        TRUE,
        LsaTypeEnum,
        PAM_LOG_LEVEL_DISABLED,
        PAM_LOG_LEVEL_DEBUG,
        gLogLevels,
        &(gStagingConfig.dwLogLevel)
    },
    {
        "DisplayMOTD",
        TRUE,
        LsaTypeBoolean,
        0,
        -1,
        NULL,
        &(gStagingConfig.bLsaPamDisplayMOTD)
    },
    {
        "UserNotAllowedError",
        TRUE,
        LsaTypeString,
        0,
        -1,
        NULL,
        &(gStagingConfig.pszAccessDeniedMessage)
    }
};
#endif

DWORD
LsaPamReadRegistry(
    PLSA_PAM_CONFIG* ppConfig
    )
{
    DWORD dwError = 0;
    PLSA_PAM_CONFIG pConfig = NULL;

    dwError = LwAllocateMemory(sizeof(LSA_PAM_CONFIG), (PVOID*) &pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    memset(&gStagingConfig, 0, sizeof(LSA_PAM_CONFIG));
    dwError = LsaPamInitializeConfig(&gStagingConfig);
    BAIL_ON_LSA_ERROR(dwError);

#if 0
    dwError = LsaProcessConfig(
                "Services\\lsass\\Parameters\\PAM",
                "Policy\\Services\\lsass\\Parameters\\PAM",
                gConfigDescription,
                sizeof(gConfigDescription)/sizeof(gConfigDescription[0]));
    BAIL_ON_LSA_ERROR(dwError);
#endif

    *pConfig = gStagingConfig;
    memset(&gStagingConfig, 0, sizeof(LSA_PAM_CONFIG));

    *ppConfig = pConfig;

cleanup:

    return dwError;

error:

    if ( pConfig )
    {
        LsaPamFreeConfigContents(pConfig);
        LW_SAFE_FREE_MEMORY(pConfig);
    }

    LsaPamFreeConfigContents(&gStagingConfig);

    goto cleanup;
}

DWORD
LsaPamInitializeConfig(
    PLSA_PAM_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    PSTR  pszMessage = NULL;

    memset(pConfig, 0, sizeof(LSA_PAM_CONFIG));

    pConfig->bLsaPamDisplayMOTD = FALSE;
    pConfig->dwLogLevel = PAM_LOG_LEVEL_ERROR;

    dwError = LwAllocateString(
                    LSA_PAM_LOGON_RIGHTS_DENIED_MESSAGE,
                    &pszMessage);
    BAIL_ON_LSA_ERROR(dwError);

    LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
    pConfig->pszAccessDeniedMessage = pszMessage;

error:

    return dwError;
}

VOID
LsaPamFreeConfig(
    PLSA_PAM_CONFIG pConfig
    )
{
    LsaPamFreeConfigContents(pConfig);
    LwFreeMemory(pConfig);
}

VOID
LsaPamFreeConfigContents(
    PLSA_PAM_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszAccessDeniedMessage);
}
