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
 *        localcfg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Wrappers for accessing global configuration variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 */
#include "localprovider.h"

static
DWORD
Local_SetConfig_EnableEventLog(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
Local_SetConfig_PasswordLifespan(
    PLOCAL_CONFIG pConfig,
    PCSTR         pszName,
    PCSTR         pszValue
    );

static
DWORD
Local_SetConfig_PasswordChangeWarningTime(
    PLOCAL_CONFIG pConfig,
    PCSTR         pszName,
    PCSTR         pszValue
    );

static LOCAL_CONFIG_HANDLER gLocalConfigHandlers[] =
{
    {"enable-eventlog",            &Local_SetConfig_EnableEventLog},
    {"password-lifespan",     &Local_SetConfig_PasswordLifespan},
    {"password-change-warning-time",  &Local_SetConfig_PasswordChangeWarningTime}
};

DWORD
LsaProviderLocal_InitializeConfig(
    PLOCAL_CONFIG pConfig
    )
{
    memset(pConfig, 0, sizeof(LOCAL_CONFIG));

    pConfig->dwPasswdChangeInterval = LOCAL_PASSWORD_CHANGE_INTERVAL_DEFAULT;
    pConfig->dwPasswdChangeWarningTime = LOCAL_PASSWORD_CHANGE_WARNING_TIME_DEFAULT;
    
    pConfig->bEnableEventLog = FALSE;

    return 0;
}

DWORD
LsaProviderLocal_TransferConfigContents(
    PLOCAL_CONFIG pSrcConfig,
    PLOCAL_CONFIG pDstConfig
    )
{
    memset(pDstConfig, 0, sizeof(LOCAL_CONFIG));

    *pDstConfig = *pSrcConfig;

    memset(pSrcConfig, 0, sizeof(LOCAL_CONFIG));

    return 0;
}

VOID
LsaProviderLocal_FreeConfig(
    PLOCAL_CONFIG pConfig
    )
{
    LsaProviderLocal_FreeConfigContents(pConfig);
    LsaFreeMemory(pConfig);
}

VOID
LsaProviderLocal_FreeConfigContents(
    PLOCAL_CONFIG pConfig
    )
{
    // Nothing to do yet.
}

DWORD
LsaProviderLocal_ParseConfigFile(
    PCSTR pszConfigFilePath,
    PLOCAL_CONFIG pConfig
    )
{
    return LsaParseConfigFile(
                pszConfigFilePath,
                LSA_CFG_OPTION_STRIP_ALL,
                &LsaProviderLocal_ConfigStartSection,
                NULL,
                &LsaProviderLocal_ConfigNameValuePair,
                NULL,
                pConfig);
}

DWORD
LsaProviderLocal_ConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    PCSTR pszLibName = NULL;
    BOOLEAN bContinue = TRUE;
    BOOLEAN bSkipSection = FALSE;
    
    if (IsNullOrEmptyString(pszSectionName) ||
        (strncasecmp(pszSectionName, LOCAL_CFG_TAG_AUTH_PROVIDER, sizeof(LOCAL_CFG_TAG_AUTH_PROVIDER)-1) &&
         strncasecmp(pszSectionName, "global", sizeof("global")-1)))
    {
        bSkipSection = TRUE;
        goto done;
    }
    
    if (!strncasecmp(pszSectionName, LOCAL_CFG_TAG_AUTH_PROVIDER, sizeof(LOCAL_CFG_TAG_AUTH_PROVIDER)-1))
    {
        pszLibName = pszSectionName + sizeof(LOCAL_CFG_TAG_AUTH_PROVIDER) - 1;
        if (IsNullOrEmptyString(pszLibName) ||
            strcasecmp(pszLibName, LOCAL_CFG_TAG_LOCAL_PROVIDER)) {
            bSkipSection = TRUE;
            goto done;
        }
    }

done:

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;
}

DWORD
LsaProviderLocal_ConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    DWORD iHandler = 0;
    DWORD nHandlers = sizeof(gLocalConfigHandlers)/sizeof(gLocalConfigHandlers[0]);
    
    if (!IsNullOrEmptyString(pszName))
    {
        for (; iHandler < nHandlers; iHandler++)
        {
            if (!strcasecmp(gLocalConfigHandlers[iHandler].pszId, pszName))
            {
                gLocalConfigHandlers[iHandler].pfnHandler(
                                (PLOCAL_CONFIG)pData,
                                pszName,
                                pszValue);
                break;
            }
        }
    }

    *pbContinue = TRUE;

    return dwError;
}

static
DWORD
Local_SetConfig_EnableEventLog(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bEnableEventLog = LsaProviderLocal_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
Local_SetConfig_PasswordLifespan(
    PLOCAL_CONFIG pConfig,
    PCSTR         pszName,
    PCSTR         pszValue
    )
{
    DWORD dwError = 0;
    DWORD dwPasswdChangeInterval = 0;

    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LsaParseDateString(
                        pszValue,
                        &dwPasswdChangeInterval);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwPasswdChangeInterval < LOCAL_PASSWORD_CHANGE_INTERVAL_MINIMUM)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeInterval to %u.  Minimum is %u.",
                        dwPasswdChangeInterval,
                        LOCAL_PASSWORD_CHANGE_INTERVAL_MINIMUM);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwPasswdChangeInterval > LOCAL_PASSWORD_CHANGE_INTERVAL_MAXIMUM)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeInterval to %u.  Maximum is %u.",
                        dwPasswdChangeInterval,
                        LOCAL_PASSWORD_CHANGE_INTERVAL_MAXIMUM);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->dwPasswdChangeInterval = dwPasswdChangeInterval;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
Local_SetConfig_PasswordChangeWarningTime(
    PLOCAL_CONFIG pConfig,
    PCSTR         pszName,
    PCSTR         pszValue
    )
{
    DWORD dwError = 0;
    DWORD dwPasswdChangeWarningTime = 0;

    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LsaParseDateString(
                        pszValue,
                        &dwPasswdChangeWarningTime);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwPasswdChangeWarningTime < LOCAL_PASSWORD_CHANGE_WARNING_TIME_MINIMUM)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeWarningTime to %u.  Minimum is %u.",
                        dwPasswdChangeWarningTime,
                        LOCAL_PASSWORD_CHANGE_WARNING_TIME_MINIMUM);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwPasswdChangeWarningTime > LOCAL_PASSWORD_CHANGE_WARNING_TIME_MAXIMUM)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeWarningTime to %u.  Maximum is %u.",
                        dwPasswdChangeWarningTime,
                        LOCAL_PASSWORD_CHANGE_WARNING_TIME_MAXIMUM);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->dwPasswdChangeWarningTime = dwPasswdChangeWarningTime;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaProviderLocal_SetConfigFilePath(
    PCSTR pszConfigFilePath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszConfigFilePathLocal = NULL;

    BAIL_ON_INVALID_STRING(pszConfigFilePath);

    dwError = LsaAllocateString(
                    pszConfigFilePath,
                    &pszConfigFilePathLocal
                    );
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    LSA_SAFE_FREE_STRING(gpszConfigFilePath);

    gpszConfigFilePath = pszConfigFilePathLocal;

cleanup:

    LEAVE_LOCAL_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszConfigFilePathLocal);

    goto cleanup;
}

DWORD
LsaProviderLocal_GetConfigFilePath(
    PSTR* ppszConfigFilePath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszConfigFilePath = NULL;

    ENTER_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    if (!IsNullOrEmptyString(gpszConfigFilePath))
    {
        dwError = LsaAllocateString(
                        gpszConfigFilePath,
                        &pszConfigFilePath
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszConfigFilePath = pszConfigFilePath;

cleanup:

    LEAVE_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwError;

error:

    *ppszConfigFilePath = NULL;

    goto cleanup;
}

DWORD
LsaProviderLocal_GetPasswdChangeInterval(
    VOID
    )
{
    DWORD dwPasswdChangeInterval = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwPasswdChangeInterval = gLocalConfig.dwPasswdChangeInterval;
    LEAVE_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwPasswdChangeInterval;
}

DWORD
LsaProviderLocal_GetPasswdChangeWarningTime(
    VOID
    )
{
    DWORD dwPasswdChangeWarningTime = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwPasswdChangeWarningTime = gLocalConfig.dwPasswdChangeWarningTime;
    LEAVE_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwPasswdChangeWarningTime;
}

BOOLEAN
LsaProviderLocal_GetBooleanConfigValue(
    PCSTR pszValue
    )
{
    BOOLEAN bResult = FALSE;

    if (!IsNullOrEmptyString(pszValue) &&
        (!strcasecmp(pszValue, "true") ||
         !strcasecmp(pszValue, "1") ||
         (*pszValue == 'y') ||
         (*pszValue == 'Y')))
    {
        bResult = TRUE;
    }

    return bResult;
}

BOOLEAN
LsaProviderLocal_EventlogEnabled(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bResult = gLocalConfig.bEnableEventLog;

    LEAVE_LOCAL_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bResult;
}

