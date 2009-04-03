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
 *        lpcfg.c
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
#include "includes.h"

static
DWORD
LocalCfgStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

static
DWORD
LocalCfgNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

static
DWORD
LocalCfgEnableEventLog(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgPasswordLifespan(
    PLOCAL_CONFIG pConfig,
    PCSTR         pszName,
    PCSTR         pszValue
    );

static
DWORD
LocalCfgPasswordChangeWarningTime(
    PLOCAL_CONFIG pConfig,
    PCSTR         pszName,
    PCSTR         pszValue
    );

static
BOOLEAN
LocalCfgGetBooleanValue(
    PCSTR pszValue
    );

typedef DWORD (*PFN_LOCAL_CFG_HANDLER)(
                    PLOCAL_CONFIG pConfig,
                    PCSTR         pszName,
                    PCSTR         pszValue
                    );

typedef struct __LOCAL_CFG_HANDLER
{
    PCSTR                 pszId;
    PFN_LOCAL_CFG_HANDLER pfnHandler;

} LOCAL_CFG_HANDLER, *PLOCAL_CFG_HANDLER;

static LOCAL_CFG_HANDLER gLocalCfgHandlers[] =
{
    {"enable-eventlog",              &LocalCfgEnableEventLog},
    {"password-lifespan",            &LocalCfgPasswordLifespan},
    {"password-change-warning-time", &LocalCfgPasswordChangeWarningTime}
};

DWORD
LocalCfgInitialize(
    PLOCAL_CONFIG pConfig
    )
{
    memset(pConfig, 0, sizeof(LOCAL_CONFIG));

    pConfig->dwPasswdChangeInterval = LOCAL_PASSWD_CHANGE_INTERVAL_DEFAULT;
    pConfig->dwPasswdChangeWarningTime = LOCAL_PASSWD_CHANGE_WARNING_TIME_DEFAULT;

    pConfig->bEnableEventLog = FALSE;

    return 0;
}

DWORD
LocalCfgTransferContents(
    PLOCAL_CONFIG pSrcConfig,
    PLOCAL_CONFIG pDstConfig
    )
{
    memset(pDstConfig, 0, sizeof(LOCAL_CONFIG));

    *pDstConfig = *pSrcConfig;

    memset(pSrcConfig, 0, sizeof(LOCAL_CONFIG));

    return 0;
}

DWORD
LocalCfgParseFile(
    PCSTR         pszConfigFilePath,
    PLOCAL_CONFIG pConfig
    )
{
    return LsaParseConfigFile(
                pszConfigFilePath,
                LSA_CFG_OPTION_STRIP_ALL,
                &LocalCfgStartSection,
                NULL,
                &LocalCfgNameValuePair,
                NULL,
                pConfig);
}


DWORD
LocalCfgGetFilePath(
    PSTR* ppszConfigFilePath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszConfigFilePath = NULL;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    if (!IsNullOrEmptyString(gLPGlobals.pszConfigFilePath))
    {
        dwError = LsaAllocateString(
                        gLPGlobals.pszConfigFilePath,
                        &pszConfigFilePath);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszConfigFilePath = pszConfigFilePath;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    return dwError;

error:

    *ppszConfigFilePath = NULL;

    goto cleanup;
}

DWORD
LocalCfgGetPasswordChangeInterval(
    PDWORD pdwPasswdChangeInterval
    )
{
    DWORD dwError = 0;
    DWORD dwPasswdChangeInterval = 0;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    dwPasswdChangeInterval = gLPGlobals.cfg.dwPasswdChangeInterval;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    *pdwPasswdChangeInterval = dwPasswdChangeInterval;

    return dwError;
}

DWORD
LocalCfgGetPasswordChangeWarningTime(
    PDWORD pdwPasswdChangeWarningTime
    )
{
    DWORD dwError = 0;
    DWORD dwPasswdChangeWarningTime = 0;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    dwPasswdChangeWarningTime = gLPGlobals.cfg.dwPasswdChangeWarningTime;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    *pdwPasswdChangeWarningTime = dwPasswdChangeWarningTime;

    return dwError;
}

DWORD
LocalCfgIsEventlogEnabled(
    PBOOLEAN pbValue
    )
{
    DWORD dwError = 0;
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    bResult = gLPGlobals.cfg.bEnableEventLog;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    *pbValue = bResult;

    return dwError;
}

VOID
LocalCfgFree(
    PLOCAL_CONFIG pConfig
    )
{
    LocalCfgFreeContents(pConfig);
    LsaFreeMemory(pConfig);
}

VOID
LocalCfgFreeContents(
    PLOCAL_CONFIG pConfig
    )
{
    // Nothing to do yet.
}

static
DWORD
LocalCfgStartSection(
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
        (strncasecmp(pszSectionName,
                     LOCAL_CFG_TAG_AUTH_PROVIDER,
                     sizeof(LOCAL_CFG_TAG_AUTH_PROVIDER)-1) &&
         strncasecmp(pszSectionName, "global", sizeof("global")-1)))
    {
        bSkipSection = TRUE;
        goto done;
    }

    if (!strncasecmp(pszSectionName,
                     LOCAL_CFG_TAG_AUTH_PROVIDER,
                     sizeof(LOCAL_CFG_TAG_AUTH_PROVIDER)-1))
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

static
DWORD
LocalCfgNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;
    DWORD iHandler = 0;
    DWORD nHandlers = sizeof(gLocalCfgHandlers)/sizeof(gLocalCfgHandlers[0]);

    if (!IsNullOrEmptyString(pszName))
    {
        for (; iHandler < nHandlers; iHandler++)
        {
            if (!strcasecmp(gLocalCfgHandlers[iHandler].pszId, pszName))
            {
                gLocalCfgHandlers[iHandler].pfnHandler(
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
LocalCfgEnableEventLog(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bEnableEventLog = LocalCfgGetBooleanValue(pszValue);

    return 0;
}

static
DWORD
LocalCfgPasswordLifespan(
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

    if (dwPasswdChangeInterval < LOCAL_PASSWD_CHANGE_INTERVAL_MINIMUM)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeInterval to %u.  Minimum is %u.",
                        dwPasswdChangeInterval,
                        LOCAL_PASSWD_CHANGE_INTERVAL_MINIMUM);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwPasswdChangeInterval > LOCAL_PASSWD_CHANGE_INTERVAL_MAXIMUM)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeInterval to %u.  Maximum is %u.",
                        dwPasswdChangeInterval,
                        LOCAL_PASSWD_CHANGE_INTERVAL_MAXIMUM);
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
LocalCfgPasswordChangeWarningTime(
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

    if (dwPasswdChangeWarningTime < LOCAL_PASSWD_CHANGE_WARNING_TIME_MINIMUM)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeWarningTime to %u.  Minimum is %u.",
                        dwPasswdChangeWarningTime,
                        LOCAL_PASSWD_CHANGE_WARNING_TIME_MINIMUM);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwPasswdChangeWarningTime > LOCAL_PASSWD_CHANGE_WARNING_TIME_MAXIMUM)
    {
        LSA_LOG_ERROR("Failed to set PasswdChangeWarningTime to %u.  Maximum is %u.",
                        dwPasswdChangeWarningTime,
                        LOCAL_PASSWD_CHANGE_WARNING_TIME_MAXIMUM);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->dwPasswdChangeWarningTime = dwPasswdChangeWarningTime;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
BOOLEAN
LocalCfgGetBooleanValue(
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


