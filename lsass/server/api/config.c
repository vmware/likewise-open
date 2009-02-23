/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
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
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Configuration API (Server)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "api.h"

DWORD
LsaSrvRefreshConfiguration(
    HANDLE hServer
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PLSA_AUTH_PROVIDER pProvider = NULL;
    HANDLE hProvider = (HANDLE)NULL;
    PLSA_SRV_API_STATE pServerState = (PLSA_SRV_API_STATE)hServer;
    PSTR pszConfigFilePath = NULL;
    BOOLEAN bUnlockConfigLock = FALSE;
    LSA_SRV_API_CONFIG apiConfig;

    if (pServerState->peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaSrvApiInitConfig(&apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiGetConfigFilePath(&pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiReadConfig(
                    pszConfigFilePath,
                    &apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_lock(&gAPIConfigLock);
    bUnlockConfigLock = TRUE;

    dwError = LsaSrvApiTransferConfigContents(
                    &apiConfig,
                    &gAPIConfig);
    BAIL_ON_LSA_ERROR(dwError);

    pthread_mutex_unlock(&gAPIConfigLock);
    bUnlockConfigLock = FALSE;

    ENTER_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    dwError = LSA_ERROR_NOT_HANDLED;

    for (pProvider = gpAuthProviderList;
         pProvider;
         pProvider = pProvider->pNext)
    {
        dwError = LsaSrvOpenProvider(hServer, pProvider, &hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = pProvider->pFnTable->pfnRefreshConfiguration(
                                        hProvider);
        BAIL_ON_LSA_ERROR(dwError);

        LsaSrvCloseProvider(pProvider, hProvider);
        hProvider = (HANDLE)NULL;
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszConfigFilePath);

    if (hProvider != (HANDLE)NULL) {
        LsaSrvCloseProvider(pProvider, hProvider);
    }

    LEAVE_AUTH_PROVIDER_LIST_READER_LOCK(bInLock);

    LsaSrvApiFreeConfigContents(&apiConfig);

    if (bUnlockConfigLock)
    {
        pthread_mutex_unlock(&gAPIConfigLock);
    }

    return(dwError);

error:

    goto cleanup;

}

DWORD
LsaSrvApiGetConfigFilePath(
    PSTR* ppszConfigFilePath
    )
{
    DWORD dwError = 0;
    PSTR  pszConfigFilePath = NULL;

    pthread_mutex_lock(&gAPIConfigLock);

    BAIL_ON_INVALID_STRING(gpszConfigFilePath);

    dwError = LsaAllocateString(
                    gpszConfigFilePath,
                    &pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszConfigFilePath = pszConfigFilePath;

cleanup:

    pthread_mutex_unlock(&gAPIConfigLock);

    return dwError;

error:

    *ppszConfigFilePath = NULL;

    goto cleanup;
}

DWORD
LsaSrvApiInitConfig(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    LsaSrvApiFreeConfigContents(pConfig);

    pConfig->bEnableEventLog = FALSE;
    pConfig->bLogNetworkConnectionEvents = TRUE;

    return 0;
}

DWORD
LsaSrvApiReadConfig(
    PCSTR pszConfigFilePath,
    PLSA_SRV_API_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    LSA_SRV_API_CONFIG apiConfig;

    BAIL_ON_INVALID_STRING(pszConfigFilePath);

    dwError = LsaSrvApiInitConfig(&apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaParseConfigFile(
                    pszConfigFilePath,
                    LSA_CFG_OPTION_STRIP_ALL,
                    &LsaSrvApiConfigStartSection,
                    NULL,
                    &LsaSrvApiConfigNameValuePair,
                    NULL,
                    &apiConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSrvApiTransferConfigContents(
                    &apiConfig,
                    pConfig);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaSrvApiFreeConfigContents(&apiConfig);

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaSrvApiConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;

    if (IsNullOrEmptyString(pszSectionName) ||
        (strncasecmp(pszSectionName, "global", sizeof("global")-1)))
    {
        bSkipSection = TRUE;
    }

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;
}

DWORD
LsaSrvApiConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PLSA_SRV_API_CONFIG pConfig = (PLSA_SRV_API_CONFIG)pData;

    BAIL_ON_INVALID_POINTER(pConfig);
    BAIL_ON_INVALID_STRING(pszName);

    if (!strcasecmp(pszName, "enable-eventlog"))
    {
        if (!IsNullOrEmptyString(pszValue) &&
            (!strcasecmp(pszValue, "true") ||
             !strcasecmp(pszValue, "1") ||
             (*pszValue == 'y') ||
             (*pszValue == 'Y')))
        {
            pConfig->bEnableEventLog = TRUE;
        }
        else
        {
            pConfig->bEnableEventLog = FALSE;
        }
    }
    else if (!strcasecmp(pszName, "log-network-connection-events"))
    {
        if (!IsNullOrEmptyString(pszValue) &&
            (!strcasecmp(pszValue, "false") ||
             !strcasecmp(pszValue, "0") ||
             (*pszValue == 'n') ||
             (*pszValue == 'N')))
        {
            pConfig->bLogNetworkConnectionEvents = FALSE;
        }
        else
        {
            pConfig->bLogNetworkConnectionEvents = TRUE;
        }
    }

    *pbContinue = TRUE;

cleanup:

    return dwError;

error:

    *pbContinue = FALSE;

    goto cleanup;
}

DWORD
LsaSrvApiTransferConfigContents(
    PLSA_SRV_API_CONFIG pSrc,
    PLSA_SRV_API_CONFIG pDest
    )
{
    LsaSrvApiFreeConfigContents(pDest);

    *pDest = *pSrc;

    LsaSrvApiFreeConfigContents(pSrc);

    return 0;
}

VOID
LsaSrvApiFreeConfigContents(
    PLSA_SRV_API_CONFIG pConfig
    )
{
    // Nothing to free right now
    memset(pConfig, 0, sizeof(*pConfig));
}

BOOLEAN
LsaSrvEventlogEnabled(
    VOID
    )
{
    BOOLEAN bResult = FALSE;

    pthread_mutex_lock(&gAPIConfigLock);

    bResult = gAPIConfig.bEnableEventLog;

    pthread_mutex_unlock(&gAPIConfigLock);

    return bResult;
}

VOID
LsaSrvEnableEventlog(
    BOOLEAN bValue
    )
{
    pthread_mutex_lock(&gAPIConfigLock);

    gAPIConfig.bEnableEventLog = bValue;

    pthread_mutex_unlock(&gAPIConfigLock);
}

BOOLEAN
LsaSrvShouldLogNetworkConnectionEvents(
    VOID
    )
{
    BOOLEAN bResult = TRUE;

    pthread_mutex_lock(&gAPIConfigLock);

    bResult = gAPIConfig.bLogNetworkConnectionEvents;

    pthread_mutex_unlock(&gAPIConfigLock);

    return bResult;
}

VOID
LsaSrvSetLogNetworkConnectionEvents(
    BOOLEAN bValue
    )
{
    pthread_mutex_lock(&gAPIConfigLock);

    gAPIConfig.bLogNetworkConnectionEvents = bValue;

    pthread_mutex_unlock(&gAPIConfigLock);
}
