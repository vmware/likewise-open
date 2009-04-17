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
DWORD
LocalCfgSetHomedirPrefix(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetHomedirTemplate(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetCreateHomedir(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetHomedirUmask(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
LocalCfgSetSkeletonDirs(
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
    {"login-shell-template",         &LocalCfgSetDefaultLoginShell},
    {"homedir-prefix",               &LocalCfgSetHomedirPrefix},
    {"homedir-template",             &LocalCfgSetHomedirTemplate},
    {"create-homedir",               &LocalCfgSetCreateHomedir},
    {"homedir-umask",                &LocalCfgSetHomedirUmask},
    {"skeleton-dirs",                &LocalCfgSetSkeletonDirs}
};

DWORD
LocalCfgInitialize(
    PLOCAL_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    PCSTR pszDefaultLoginShell      = LOCAL_CFG_DEFAULT_LOGIN_SHELL;
    PCSTR pszDefaultHomedirPrefix   = LOCAL_CFG_DEFAULT_HOMEDIR_PREFIX;
    PCSTR pszDefaultHomedirTemplate = LOCAL_CFG_DEFAULT_HOMEDIR_TEMPLATE;
    PCSTR pszDefaultSkelDirs        = LOCAL_CFG_DEFAULT_SKELETON_DIRS;

    memset(pConfig, 0, sizeof(LOCAL_CONFIG));

    pConfig->bEnableEventLog = FALSE;
    pConfig->dwMaxGroupNestingLevel = LOCAL_CFG_MAX_GROUP_NESTING_LEVEL_DEFAULT;

    dwError = LsaAllocateString(
                    pszDefaultLoginShell,
                    &pConfig->pszLoginShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    pszDefaultHomedirPrefix,
                    &pConfig->pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    pszDefaultHomedirTemplate,
                    &pConfig->pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    pConfig->bCreateHomedir = LOCAL_CFG_DEFAULT_CREATE_HOMEDIR;
    pConfig->dwHomedirUMask = LOCAL_CFG_DEFAULT_HOMEDIR_UMASK;

    dwError = LsaAllocateString(
                    pszDefaultSkelDirs,
                    &pConfig->pszSkelDirs);
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

DWORD
LocalCfgGetHomedirPrefix(
    PSTR* ppszHomedirPrefix
    )
{
    DWORD dwError = 0;
    PSTR  pszHomedirPrefix = NULL;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    dwError = LsaAllocateString(
                    gLPGlobals.cfg.pszHomedirPrefix,
                    &pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszHomedirPrefix = pszHomedirPrefix;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    return dwError;

error:

    *ppszHomedirPrefix = NULL;

    LSA_SAFE_FREE_STRING(pszHomedirPrefix);

    goto cleanup;
}

DWORD
LocalCfgGetHomedirTemplate(
    PSTR* ppszHomedirTemplate
    )
{
    DWORD dwError = 0;
    PSTR  pszHomedirTemplate = NULL;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    dwError = LsaAllocateString(
                    gLPGlobals.cfg.pszHomedirTemplate,
                    &pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszHomedirTemplate = pszHomedirTemplate;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    return dwError;

error:

    *ppszHomedirTemplate = NULL;

    LSA_SAFE_FREE_STRING(pszHomedirTemplate);

    goto cleanup;
}

DWORD
LocalCfgGetHomedirUmask(
    PDWORD pdwUmask
    )
{
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    *pdwUmask = gLPGlobals.cfg.dwHomedirUMask;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    return 0;

}

DWORD
LocalCfgMustCreateHomedir(
    PBOOLEAN pbCreateHomedir
    )
{
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    *pbCreateHomedir = gLPGlobals.cfg.bCreateHomedir;

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    return 0;
}

DWORD
LocalCfgGetSkeletonDirs(
    PSTR* ppszSkelDirs
    )
{
    DWORD dwError = 0;
    PSTR  pszSkelDirs = NULL;
    BOOLEAN bInLock = FALSE;

    LOCAL_LOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    dwError = LsaAllocateString(
                    gLPGlobals.cfg.pszSkelDirs,
                    &pszSkelDirs);
    BAIL_ON_LSA_ERROR(dwError);

    *ppszSkelDirs = pszSkelDirs;

cleanup:

    LOCAL_UNLOCK_MUTEX(bInLock, &gLPGlobals.mutex);

    return dwError;

error:

    *ppszSkelDirs = NULL;

    LSA_SAFE_FREE_STRING(pszSkelDirs);

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
    LSA_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    LSA_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);
    LSA_SAFE_FREE_STRING(pConfig->pszSkelDirs);
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
DWORD
LocalCfgSetHomedirPrefix(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR pszHomedirPrefix = NULL;

    BAIL_ON_INVALID_STRING(pszValue);

    dwError = LsaAllocateString(
                pszValue,
                &pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStripWhitespace(pszHomedirPrefix, TRUE, TRUE);

    BAIL_ON_INVALID_STRING(pszHomedirPrefix);

    if (*pszHomedirPrefix != '/')
    {
        LSA_LOG_ERROR("Invalid home directory prefix [%s]", pszHomedirPrefix);
        goto error;
    }

    LSA_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    pConfig->pszHomedirPrefix = pszHomedirPrefix;

cleanup:

    return 0;

error:

    LSA_SAFE_FREE_STRING(pszHomedirPrefix);

    goto cleanup;
}

static
DWORD
LocalCfgSetHomedirTemplate(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszHomedirTemplate = NULL;

    if ( !IsNullOrEmptyString(pszValue) )
    {
        dwError = LsaAllocateString(
                      pszValue,
                      &pszHomedirTemplate);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);

    pConfig->pszHomedirTemplate = pszHomedirTemplate;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszHomedirTemplate);

    goto cleanup;
}

static
DWORD
LocalCfgSetCreateHomedir(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bCreateHomedir = LocalCfgGetBooleanValue(pszValue);

    return 0;
}

static
DWORD
LocalCfgSetHomedirUmask(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PCSTR cp = NULL;
    DWORD dwOct = 0;
    DWORD dwVal = 0;
    DWORD dwCnt = 0;
    char  cp2[2];

    // Convert the umask octal string to a decimal number

    cp2[1] = 0;

    for ( cp = pszValue, dwCnt = 0 ; isdigit((int)*cp) ; cp++, dwCnt++ )
    {
        dwOct *= 8;

        cp2[0] = *cp;
        dwVal = atoi(cp2);

        if ( dwVal > 7 )
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
        }
        BAIL_ON_LSA_ERROR(dwError);

        dwOct += dwVal;
    }

    if ( dwCnt > 4 )
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    // Disallow 07xx since the user should always have
    // access to his home directory.
    if ( (dwOct & 0700) == 0700 )
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    else
    {
        pConfig->dwHomedirUMask = dwOct;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LocalCfgSetSkeletonDirs(
    PLOCAL_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszSkelDirs = NULL;

    if ( !IsNullOrEmptyString(pszValue) )
    {
        dwError = LsaAllocateString(
                      pszValue,
                      &pszSkelDirs);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_SAFE_FREE_STRING(pConfig->pszSkelDirs);

    pConfig->pszSkelDirs = pszSkelDirs;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszSkelDirs);

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


