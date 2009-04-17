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
LocalCfgSetDefaultLoginShell(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
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
    {"login-shell-template",         &LocalCfgSetDefaultLoginShell}
};

DWORD
LocalCfgInitialize(
    PLOCAL_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    PCSTR pszDefaultLoginShell = LOCAL_CFG_DEFAULT_LOGIN_SHELL;

    memset(pConfig, 0, sizeof(LOCAL_CONFIG));

    pConfig->bEnableEventLog = FALSE;
    pConfig->dwMaxGroupNestingLevel = LOCAL_CFG_MAX_GROUP_NESTING_LEVEL_DEFAULT;

    dwError = LsaAllocateString(
                    pszDefaultLoginShell,
                    &pConfig->pszLoginShell);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
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
LocalCfgGetMaxPasswordAge(
    PLONG64 pllMaxPwdAge
    )
{
    DWORD  dwError = 0;
    LONG64 llMaxPwdAge = 0;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    llMaxPwdAge = gLPGlobals.llMaxPwdAge;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    *pllMaxPwdAge = llMaxPwdAge;

    return dwError;
}

DWORD
LocalCfgGetPasswordChangeWarningTime(
    PLONG64 pllPasswdChangeWarningTime
    )
{
    DWORD dwError = 0;
    LONG64 llPasswdChangeWarningTime = 0;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    llPasswdChangeWarningTime = gLPGlobals.llPwdChangeTime;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    *pllPasswdChangeWarningTime = llPasswdChangeWarningTime;

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

DWORD
LocalCfgGetMaxGroupNestingLevel(
    PDWORD pdwNestingLevel
    )
{
    DWORD dwError = 0;
    DWORD dwMaxGroupNestingLevel = LOCAL_CFG_MAX_GROUP_NESTING_LEVEL_DEFAULT;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    dwMaxGroupNestingLevel = gLPGlobals.cfg.dwMaxGroupNestingLevel;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    *pdwNestingLevel = dwMaxGroupNestingLevel;

    return dwError;
}

DWORD
LocalCfgGetDefaultShell(
    PSTR* ppszLoginShell
    )
{
    DWORD dwError = 0;
    PSTR  pszLoginShell = NULL;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    dwError = LsaAllocateString(
                    gLPGlobals.cfg.pszLoginShell,
                    &pszLoginShell);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszLoginShell = pszLoginShell;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    return dwError;

error:

    *ppszLoginShell = NULL;

    LSA_SAFE_FREE_STRING(pszLoginShell);

    goto cleanup;
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
    LSA_SAFE_FREE_STRING(pConfig->pszLoginShell);
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
    pConfig->bEnableEventLog =  LocalCfgGetBooleanValue(pszValue);

    return 0;
}

static
DWORD
LocalCfgSetDefaultLoginShell(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszLoginShell = NULL;

    BAIL_ON_INVALID_STRING(pszValue);

    if (access(pszValue, X_OK) != 0)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(
                    pszValue,
                    &pszLoginShell);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_STRING(pConfig->pszLoginShell);

    pConfig->pszLoginShell = pszLoginShell;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszLoginShell);

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


