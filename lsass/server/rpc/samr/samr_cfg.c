/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        samr_cfg.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        Samr rpc server configuration
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */


#include "includes.h"


static
DWORD
SamrSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );


static
DWORD
SamrSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );


static
DWORD
SamrSrvConfigSetLpcSocketPath(
    PSAMR_SRV_CONFIG pConfig,
    PCSTR            pszName,
    PCSTR            pszValue
    );


static SAMR_SRV_CONFIG_HANDLER gSamrSrvConfigHandlers[] =
{
    { "lpc-socket-path",                &SamrSrvConfigSetLpcSocketPath }
};


DWORD
SamrSrvInitialiseConfig(
    PSAMR_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(*pConfig));

    pConfig->pszLpcSocketPath = LSA_DEFAULT_LPC_SOCKET_PATH;

    return dwError;
}


DWORD
SamrSrvParseConfigFile(
    PCSTR pszConfigFilePath,
    PSAMR_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;

    dwError = LsaParseConfigFile(pszConfigFilePath,
                                LSA_CFG_OPTION_STRIP_ALL,
                                &SamrSrvConfigStartSection,
                                NULL,
                                &SamrSrvConfigNameValuePair,
                                NULL,
                                pConfig);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}


static
DWORD
SamrSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    if (IsNullOrEmptyString(pszSectionName) ||
        strncasecmp(pszSectionName, LSA_CFG_TAG_RPC_SERVER,
                    sizeof(LSA_CFG_TAG_RPC_SERVER) - 1)) {
        *pbSkipSection = TRUE;
        goto cleanup;
    }

cleanup:
    return dwError;
}


static
DWORD
SamrSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwNumHandlers = 0;

    if (IsNullOrEmptyString(pszName)) {
        *pbContinue = TRUE;
        goto cleanup;
    }

    dwNumHandlers = sizeof(gSamrSrvConfigHandlers)
                    /sizeof(gSamrSrvConfigHandlers[0]);

    for (i = 0; i < dwNumHandlers; i++) {
        if (!strcasecmp(gSamrSrvConfigHandlers[i].pszId, pszName)) {
            gSamrSrvConfigHandlers[i].pFnHandler((PSAMR_SRV_CONFIG)pData,
                                                         pszName,
                                                         pszValue);
            break;
        }
    }


cleanup:
    return dwError;
}


static
DWORD
SamrSrvConfigSetLpcSocketPath(
    PSAMR_SRV_CONFIG pConfig,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;

    dwError = LsaAllocateString(pszValue,
                                &pConfig->pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    pConfig->pszLpcSocketPath = NULL;
    goto cleanup;
}


DWORD
SamrSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (IsNullOrEmptyString(gSamrSrvConfig.pszLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LsaAllocateString(gSamrSrvConfig.pszLpcSocketPath,
                                &pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszLpcSocketPath = pszLpcSocketPath;

cleanup:
    GLOBAL_DATA_UNLOCK(bLocked);
    return dwError;

error:
    goto cleanup;
}


DWORD
SamrSrvSetConfigFilePath(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;
    PSTR pszPath = NULL;

    if (IsNullOrEmptyString(pszConfigFilePath)) {
        goto cleanup;
    }

    dwError = LsaAllocateString(pszConfigFilePath,
                                &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    GLOBAL_DATA_LOCK(locked);

    gpszConfigFilePath = pszPath;

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return dwError;

error:
    if (pszPath) {
        LSA_SAFE_FREE_STRING(pszPath);
    }

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
