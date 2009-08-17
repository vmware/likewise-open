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
 *        dssetup_srv.c
 *
 * Abstract:
 *
 *        Remote Procedure Call (RPC) Server Interface
 *
 *        DsSetup rpc server configuration
 *
 * Authors: Rafal Szczesniak (rafal@likewise.com)
 */

#include "includes.h"


static
DWORD
DsrSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );


static
DWORD
DsrSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );


static
DWORD
LsaSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );


static
DWORD
LsaSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );


static
DWORD
DsrSrvConfigSetLpcSocketPath(
    PDSSETUP_SRV_CONFIG pConfig,
    PCSTR            pszName,
    PCSTR            pszValue
    );


static
DWORD
DsrSrvConfigSetLsaLpcSocketPath(
    PDSSETUP_SRV_CONFIG pConfig,
    PCSTR pszName,
    PCSTR pszValue
    );


static DSSETUP_SRV_CONFIG_HANDLER gDsrSrvConfigHandlers[] =
{
    { "lpc-socket-path",                &DsrSrvConfigSetLpcSocketPath }
};


static DSSETUP_SRV_CONFIG_HANDLER gLsaSrvConfigHandlers[] =
{
    { "lpc-socket-path",                &DsrSrvConfigSetLsaLpcSocketPath }
};


DWORD
DsrSrvInitialiseConfig(
    PDSSETUP_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(*pConfig));

    pConfig->pszLpcSocketPath     = LSA_DEFAULT_LPC_SOCKET_PATH;
    pConfig->pszLsaLpcSocketPath  = LSA_DEFAULT_LPC_SOCKET_PATH;

    return dwError;
}


DWORD
DsrSrvParseConfigFile(
    PCSTR pszConfigFilePath,
    PDSSETUP_SRV_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    dwError = LsaParseConfigFile(pszConfigFilePath,
                                 LSA_CFG_OPTION_STRIP_ALL,
                                 &DsrSrvConfigStartSection,
                                 NULL,
                                 &DsrSrvConfigNameValuePair,
                                 NULL,
                                 pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaParseConfigFile(pszConfigFilePath,
				 LSA_CFG_OPTION_STRIP_ALL,
				 &LsaSrvConfigStartSection,
				 NULL,
				 &LsaSrvConfigNameValuePair,
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
DsrSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PCSTR pszLibName = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszSectionName) ||
        strncasecmp(pszSectionName, LSA_CFG_TAG_RPC_SERVER,
                    sizeof(LSA_CFG_TAG_RPC_SERVER) - 1)) {
        *pbSkipSection = TRUE;
        goto cleanup;
    }

    if (!strncasecmp(pszSectionName, LSA_CFG_TAG_RPC_SERVER,
                     sizeof(LSA_CFG_TAG_RPC_SERVER) - 1)) {

        pszLibName = pszSectionName + sizeof(LSA_CFG_TAG_RPC_SERVER) - 1;
        if (LW_IS_NULL_OR_EMPTY_STR(pszLibName) ||
            strcasecmp(pszLibName, LSA_CFG_TAG_DSR_RPC_SERVER)) {
            *pbSkipSection = TRUE;
            goto cleanup;
        }
    }

cleanup:
    *pbContinue = TRUE;
    return dwError;
}


static
DWORD
DsrSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwNumHandlers = 0;

    if (LW_IS_NULL_OR_EMPTY_STR(pszName)) {
        *pbContinue = TRUE;
        goto cleanup;
    }

    dwNumHandlers = sizeof(gDsrSrvConfigHandlers)
                    /sizeof(gDsrSrvConfigHandlers[0]);

    for (i = 0; i < dwNumHandlers; i++) {
        if (!strcasecmp(gDsrSrvConfigHandlers[i].pszId, pszName)) {
            gDsrSrvConfigHandlers[i].pFnHandler((PDSSETUP_SRV_CONFIG)pData,
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
LsaSrvConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PCSTR pszLibName = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszSectionName) ||
        strncasecmp(pszSectionName, LSA_CFG_TAG_RPC_SERVER,
                    sizeof(LSA_CFG_TAG_RPC_SERVER) - 1)) {
        *pbSkipSection = TRUE;
        goto cleanup;
    }

    if (!strncasecmp(pszSectionName, LSA_CFG_TAG_RPC_SERVER,
                     sizeof(LSA_CFG_TAG_RPC_SERVER) - 1)) {

        pszLibName = pszSectionName + sizeof(LSA_CFG_TAG_RPC_SERVER) - 1;
        if (LW_IS_NULL_OR_EMPTY_STR(pszLibName) ||
            strcasecmp(pszLibName, LSA_CFG_TAG_LSA_RPC_SERVER)) {
            *pbSkipSection = TRUE;
            goto cleanup;
        }
    }

cleanup:
    *pbContinue = TRUE;
    return dwError;
}


static
DWORD
LsaSrvConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwNumHandlers = 0;

    if (LW_IS_NULL_OR_EMPTY_STR(pszName)) {
        *pbContinue = TRUE;
        goto cleanup;
    }

    dwNumHandlers = sizeof(gLsaSrvConfigHandlers)
                    /sizeof(gLsaSrvConfigHandlers[0]);

    for (i = 0; i < dwNumHandlers; i++) {
        if (!strcasecmp(gLsaSrvConfigHandlers[i].pszId, pszName)) {
            gLsaSrvConfigHandlers[i].pFnHandler((PDSSETUP_SRV_CONFIG)pData,
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
DsrSrvConfigSetLpcSocketPath(
    PDSSETUP_SRV_CONFIG pConfig,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    DWORD dwError = 0;

    dwError = LwAllocateString(pszValue,
                                &pConfig->pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    pConfig->pszLpcSocketPath = NULL;
    goto cleanup;
}


static
DWORD
DsrSrvConfigSetLsaLpcSocketPath(
    PDSSETUP_SRV_CONFIG pConfig,
    PCSTR pszName,
    PCSTR pszValue
    )
{
    DWORD dwError = 0;

    dwError = LwAllocateString(pszValue,
                                &pConfig->pszLsaLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    pConfig->pszLsaLpcSocketPath = NULL;
    goto cleanup;
}


DWORD
DsrSrvConfigGetLpcSocketPath(
    PSTR *ppszLpcSocketPath
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gDsrSrvConfig.pszLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gDsrSrvConfig.pszLpcSocketPath,
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
DsrSrvConfigGetLsaLpcSocketPath(
    PSTR *ppszLsaLpcSocketPath
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    BOOL bLocked = 0;
    PSTR pszLpcSocketPath = NULL;

    GLOBAL_DATA_LOCK(bLocked);

    if (LW_IS_NULL_OR_EMPTY_STR(gDsrSrvConfig.pszLsaLpcSocketPath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(gDsrSrvConfig.pszLsaLpcSocketPath,
                                &pszLpcSocketPath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszLsaLpcSocketPath = pszLpcSocketPath;

cleanup:
    GLOBAL_DATA_UNLOCK(bLocked);
    return dwError;

error:
    goto cleanup;
}


DWORD
DsrSrvSetConfigFilePath(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;
    NTSTATUS status = STATUS_SUCCESS;
    int locked = 0;
    PSTR pszPath = NULL;

    if (LW_IS_NULL_OR_EMPTY_STR(pszConfigFilePath)) {
        goto cleanup;
    }

    dwError = LwAllocateString(pszConfigFilePath,
                                &pszPath);
    BAIL_ON_LSA_ERROR(dwError);

    GLOBAL_DATA_LOCK(locked);

    gpszConfigFilePath = pszPath;

cleanup:
    GLOBAL_DATA_UNLOCK(locked);

    return dwError;

error:
    if (pszPath) {
        LW_SAFE_FREE_STRING(pszPath);
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
