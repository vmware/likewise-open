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
 *        adcfg.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 *        Wrappers for accessing global configuration variables
 *
 * Authors: Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "adprovider.h"

DWORD
AD_ConfigStartSection(
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
        strncasecmp(pszSectionName, AD_CFG_TAG_AUTH_PROVIDER, sizeof(AD_CFG_TAG_AUTH_PROVIDER)-1))
    {
        bSkipSection = TRUE;
        goto done;
    }

    pszLibName = pszSectionName + sizeof(AD_CFG_TAG_AUTH_PROVIDER) - 1;
    if (IsNullOrEmptyString(pszLibName) ||
        strcasecmp(pszLibName, AD_CFG_TAG_AD_PROVIDER)) {
        bSkipSection = TRUE;
        goto done;
    }

done:

    *pbSkipSection = bSkipSection;
    *pbContinue = bContinue;

    return dwError;
}

DWORD
AD_ConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    PSTR pszLoginShellTemplate = NULL;
    PSTR pszHomedirTemplate = NULL;
    CHAR cValidatedSeparator = gcSeparatorDefault;
    DWORD dwCachePurgeTimeout = 0;
    DWORD dwCacheEntryExpiry = 0;
    BOOLEAN bSetCacheEntryExpiry = FALSE;
    DWORD dwMachinePasswordLifespan = 0;
    BOOLEAN bSetLDAPSignAndSeal = FALSE;
    BOOLEAN bLDAPSignAndSeal = FALSE;
    BOOLEAN bSetAssumeDefaultDomain = FALSE;
    BOOLEAN bAssumeDefaultDomain = FALSE;
    BOOLEAN bSetSyncSystemTime = FALSE;
    BOOLEAN bSyncSystemTime = FALSE;

    if (IsNullOrEmptyString(pszName) || IsNullOrEmptyString(pszValue))
    {
        goto done;
    }

    if (!strcasecmp(pszName, "login-shell-template"))
    {
        dwError = LsaAllocateString(
                        pszValue,
                        &pszLoginShellTemplate);
    }
    else if (!strcasecmp(pszName, "homedir-template"))
    {
        dwError = LsaAllocateString(
                        pszValue,
                        &pszHomedirTemplate);
    }
    else if (!strcasecmp(pszName, "separator-character"))
    {
        if (strlen(pszValue) != 1)
        {
            LSA_LOG_ERROR("AD Provider: The separator-character parameter must be a single character; recieved \"%s\"",
                            pszValue);
            dwError = LSA_ERROR_INVALID_CONFIG;
        }
        else
        {
            dwError = LsaValidateSeparatorCharacter(
                        pszValue[0],
                        &cValidatedSeparator);
        }
    }
    else if (!strcasecmp(pszName, "cache-purge-timeout"))
    {
        dwError = LsaParseDateString(
                        pszValue,
                        &dwCachePurgeTimeout
                        );
    }
    else if (!strcasecmp(pszName, "machine-password-lifespan"))
    {
        dwError = LsaParseDateString(
                        pszValue,
                        &dwMachinePasswordLifespan);
    }
    else if (!strcasecmp(pszName, "cache-entry-expiry"))
    {
        bSetCacheEntryExpiry = TRUE;

        dwError = LsaParseDateString(
                        pszValue,
                        &dwCacheEntryExpiry);
    }
    else if (!strcasecmp(pszName, "ldap-sign-and-seal"))
    {
        bSetLDAPSignAndSeal = TRUE;

        bLDAPSignAndSeal = AD_GetBooleanConfigValue(pszValue);
    }
    else if (!strcasecmp(pszName, "restrict-login-to-group"))
    {
        dwError = AD_AddAllowedGroup(pszValue);
    }
    else if (!strcasecmp(pszName, "assume-default-domain"))
    {
        bSetAssumeDefaultDomain = TRUE;

        bAssumeDefaultDomain = AD_GetBooleanConfigValue(pszValue);
    }
    else if (!strcasecmp(pszName, "sync-system-time"))
    {
        bSetSyncSystemTime = TRUE;

        bSyncSystemTime = AD_GetBooleanConfigValue(pszValue);
    }

    if (dwError != 0)
    {
        LSA_LOG_ERROR("AD Provider: Failed to parse value of %s [%s]",
                pszName, pszValue);
        dwError = 0;
        goto done;
    }

    if (bSetCacheEntryExpiry)
    {
       dwError = AD_SetCacheEntryExpirySeconds(dwCacheEntryExpiry);
    }
    else if (pszLoginShellTemplate != NULL)
    {
        dwError = AD_SetUnprovisionedModeShell(
                        pszLoginShellTemplate
                        );
    }
    else if (pszHomedirTemplate != NULL)
    {
        dwError = AD_SetUnprovisionedModeHomedirTemplate(
                        pszHomedirTemplate
                        );
    }
    else if (cValidatedSeparator != gcSeparatorDefault)
    {
        dwError = AD_SetSeparator(
                        cValidatedSeparator
                        );
    }
    else if (dwCachePurgeTimeout != 0)
    {
        dwError = AD_SetCacheReaperTimeoutSecs(
                        dwCachePurgeTimeout
                        );
    }
    else if (dwMachinePasswordLifespan != 0)
    {
        dwError = AD_SetMachinePasswordSyncPwdLifetime(
                        dwMachinePasswordLifespan
                        );
    }
    else if (bSetLDAPSignAndSeal)
    {
        dwError = AD_SetLDAPSignAndSeal(bLDAPSignAndSeal);
    }
    else if (bSetAssumeDefaultDomain)
    {
        AD_SetAssumeDefaultDomain(bAssumeDefaultDomain);
    }
    else if (bSetSyncSystemTime)
    {
        AD_SetSyncSystemTime(bSyncSystemTime);
    }

    if (dwError != 0)
    {
        LSA_LOG_ERROR("AD Provider: Failed to assign value of %s [%s] to global variable",
                pszName, pszValue);
        dwError = 0;
        goto done;
    }

done:

    *pbContinue = TRUE;

    LSA_SAFE_FREE_STRING(pszLoginShellTemplate);
    LSA_SAFE_FREE_STRING(pszHomedirTemplate);

    return dwError;
}

BOOLEAN
AD_GetBooleanConfigValue(
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

DWORD
AD_SetUnprovisionedModeShell(
    PCSTR pszUnprovisionedModeShell
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszUnprovisionedModeShellLocal = NULL;

    BAIL_ON_INVALID_STRING(pszUnprovisionedModeShell);

    if (access(pszUnprovisionedModeShell, X_OK) != 0)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(
                    pszUnprovisionedModeShell,
                    &pszUnprovisionedModeShellLocal
                    );
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    LSA_SAFE_FREE_STRING(gpszUnprovisionedModeShell);

    gpszUnprovisionedModeShell = pszUnprovisionedModeShellLocal;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszUnprovisionedModeShellLocal);

    goto cleanup;
}

DWORD
AD_GetUnprovisionedModeShell(
    PSTR* ppszUnprovisionedModeShell
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszUnprovisionedModeShell = NULL;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    if (!IsNullOrEmptyString(gpszUnprovisionedModeShell))
    {
        dwError = LsaAllocateString(
                        gpszUnprovisionedModeShell,
                        &pszUnprovisionedModeShell
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszUnprovisionedModeShell = pszUnprovisionedModeShell;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwError;

error:

    *ppszUnprovisionedModeShell = NULL;

    goto cleanup;
}

DWORD
AD_SetUnprovisionedModeHomedirTemplate(
    PCSTR pszUnprovisionedModeHomedirTemplate
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszUnprovisionedModeHomedirTemplateLocal = NULL;

    BAIL_ON_INVALID_STRING(pszUnprovisionedModeHomedirTemplate);

    dwError = LsaAllocateString(
                    pszUnprovisionedModeHomedirTemplate,
                    &pszUnprovisionedModeHomedirTemplateLocal
                    );
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    LSA_SAFE_FREE_STRING(gpszUnprovisionedModeHomedirTemplate);

    gpszUnprovisionedModeHomedirTemplate = pszUnprovisionedModeHomedirTemplateLocal;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszUnprovisionedModeHomedirTemplateLocal);

    goto cleanup;
}

DWORD
AD_GetUnprovisionedModeHomedirTemplate(
    PSTR* ppszUnprovisionedModeHomedirTemplate
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszUnprovisionedModeHomedirTemplate = NULL;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    if (!IsNullOrEmptyString(gpszUnprovisionedModeHomedirTemplate))
    {
        dwError = LsaAllocateString(
                        gpszUnprovisionedModeHomedirTemplate,
                        &pszUnprovisionedModeHomedirTemplate
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszUnprovisionedModeHomedirTemplate = pszUnprovisionedModeHomedirTemplate;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwError;

error:

    *ppszUnprovisionedModeHomedirTemplate = NULL;

    goto cleanup;
}

DWORD
AD_SetConfigFilePath(
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

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    LSA_SAFE_FREE_STRING(gpszConfigFilePath);

    gpszConfigFilePath = pszConfigFilePathLocal;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszConfigFilePathLocal);

    goto cleanup;
}

DWORD
AD_GetConfigFilePath(
    PSTR* ppszConfigFilePath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszConfigFilePath = NULL;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

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

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwError;

error:

    *ppszConfigFilePath = NULL;

    goto cleanup;
}

DWORD
AD_SetSeparator(
    CHAR cSeparator
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    CHAR cValidatedSeparator;

    dwError = LsaValidateSeparatorCharacter(cSeparator, &cValidatedSeparator);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    gcSeparator = cValidatedSeparator;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

CHAR
AD_GetSeparator(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;
    CHAR cSeparatorLocal = '^';

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    cSeparatorLocal = gcSeparator;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return cSeparatorLocal;
}

DWORD
AD_SetCacheReaperTimeoutSecs(
    DWORD dwCacheReaperTimeoutSecs
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    if (dwCacheReaperTimeoutSecs < gdwCacheReaperTimeoutSecsMinimum)
    {
        LSA_LOG_ERROR("Failed to set CacheReaperTimeoutSecs to %u.  Minimum is %u.",
                        dwCacheReaperTimeoutSecs,
                        gdwCacheReaperTimeoutSecsMinimum
                        );
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }

    if (dwCacheReaperTimeoutSecs > gdwCacheReaperTimeoutSecsMaximum)
    {
        LSA_LOG_ERROR("Failed to set CacheReaperTimeoutSecs to %u.  Maximum is %u.",
                        dwCacheReaperTimeoutSecs,
                        gdwCacheReaperTimeoutSecsMaximum
                        );
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    gdwCacheReaperTimeoutSecs = dwCacheReaperTimeoutSecs;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_GetCacheReaperTimeoutSecs(
    VOID
    )
{
    DWORD dwCacheReaperTimeoutSecs = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwCacheReaperTimeoutSecs = gdwCacheReaperTimeoutSecs;
    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwCacheReaperTimeoutSecs;
}

DWORD
AD_SetMachinePasswordSyncPwdLifetime(
    DWORD dwMachinePasswordSyncPwdLifetime
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    if (dwMachinePasswordSyncPwdLifetime < gdwMachinePasswordSyncPwdLifetimeMinimum)
    {
        LSA_LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Minimum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        gdwMachinePasswordSyncPwdLifetimeMinimum
                        );
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }

    if (dwMachinePasswordSyncPwdLifetime > gdwMachinePasswordSyncPwdLifetimeMaximum)
    {
        LSA_LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Maximum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        gdwMachinePasswordSyncPwdLifetimeMaximum
                        );
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    gdwMachinePasswordSyncPwdLifetime = dwMachinePasswordSyncPwdLifetime;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_GetMachinePasswordSyncPwdLifetime(
    VOID
    )
{
    DWORD dwMachinePasswordSyncPwdLifetime = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwMachinePasswordSyncPwdLifetime = gdwMachinePasswordSyncPwdLifetime;
    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwMachinePasswordSyncPwdLifetime;
}

DWORD
AD_GetClockDriftSeconds(
    VOID
    )
{
    DWORD dwClockDriftSecs = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwClockDriftSecs = gdwClockDriftSecs;
    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwClockDriftSecs;
}

DWORD
AD_SetCacheEntryExpirySeconds(
    DWORD dwExpirySecs
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    if (dwExpirySecs < gdwCacheEntryExpirySecsMinimum)
    {
        LSA_LOG_ERROR("Failed to set CacheEntryExpiry to %u.  Minimum is %u.",
                        dwExpirySecs,
                        gdwCacheEntryExpirySecsMinimum);
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }

    if (dwExpirySecs > gdwCacheEntryExpirySecsMaximum)
    {
        LSA_LOG_ERROR("Failed to set CacheEntryExpiry to %u.  Maximum is %u.",
                        dwExpirySecs,
                        gdwCacheEntryExpirySecsMaximum);
        dwError = LSA_ERROR_INVALID_PARAMETER;
    }
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    gdwCacheEntryExpirySecs = dwExpirySecs;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_GetCacheEntryExpirySeconds(
    VOID
    )
{
    DWORD dwResult = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    dwResult = gdwCacheEntryExpirySecs;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwResult;
}

DWORD
AD_SetLDAPSignAndSeal(
    BOOLEAN bValue
    )
{
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    gbLDAPSignAndSeal = bValue;

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return 0;
}

BOOLEAN
AD_GetLDAPSignAndSeal(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bResult = gbLDAPSignAndSeal;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bResult;
}

double
AD_GetMachineTGTGraceSeconds()
{
    double dGraceSeconds = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    dGraceSeconds = gdwMachineTGTExpiryGraceSeconds;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dGraceSeconds;
}

DWORD
AD_AddAllowedGroup(
    PCSTR pszNetbiosGroupName
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR  pszValue = NULL;
    PSTR  pszNetbiosGroupNameCopy = NULL;
    BOOLEAN bFreeValue = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (!gpAllowedGroups)
    {
        dwError = LsaHashCreate(
                        11,
                        LsaHashCaselessStringCompare,
                        LsaHashCaselessString,
                        AD_FreeHashStringKey,
                        &gpAllowedGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaHashGetValue(
                    gpAllowedGroups,
                    pszNetbiosGroupName,
                    (PVOID*)&pszValue);
    if (dwError == ENOENT)
    {
        dwError = LsaAllocateString(
                        pszNetbiosGroupName,
                        &pszNetbiosGroupNameCopy);
        BAIL_ON_LSA_ERROR(dwError);

        bFreeValue = TRUE;

        dwError = LsaHashSetValue(
                        gpAllowedGroups,
                        pszNetbiosGroupNameCopy,
                        pszNetbiosGroupNameCopy);
        BAIL_ON_LSA_ERROR(dwError);

        bFreeValue = FALSE;
    }

cleanup:

    if (bFreeValue)
    {
        LsaFreeString(pszNetbiosGroupNameCopy);
    }

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    goto cleanup;
}

BOOLEAN
AD_ShouldFilterUserLoginsByGroup(
    VOID
    )
{
    BOOLEAN bFilter = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bFilter = (gpAllowedGroups != NULL);

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bFilter;
}

BOOLEAN
AD_IsGroupAllowed(
    PCSTR pszNetbiosGroupName
    )
{
    BOOLEAN bAllowed = FALSE;
    BOOLEAN bInLock = FALSE;
    PSTR    pszValue = NULL;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    if (!gpAllowedGroups)
    {
        bAllowed = TRUE;
        goto cleanup;
    }

    if (LSA_ERROR_SUCCESS == LsaHashGetValue(
                                gpAllowedGroups,
                                pszNetbiosGroupName,
                                (PVOID*)&pszValue))
    {
        bAllowed = TRUE;
        goto cleanup;
    }

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bAllowed;
}

VOID
AD_FreeAllowedGroups_InLock(
    VOID)
{
    if (gpAllowedGroups)
    {
        LsaHashSafeFree(&gpAllowedGroups);
    }
}

BOOLEAN
AD_ShouldAssumeDefaultDomain(
    VOID
    )
{
    BOOLEAN bAssumeDefaultDomain = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bAssumeDefaultDomain = gbAssumeDefaultDomain;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bAssumeDefaultDomain;
}

VOID
AD_SetAssumeDefaultDomain(
    BOOLEAN bValue
    )
{
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    gbAssumeDefaultDomain = bValue;

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
}

BOOLEAN
AD_ShouldSyncSystemTime(
    VOID
    )
{
    BOOLEAN bSyncSystemTime = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bSyncSystemTime = gbSyncSystemTime;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bSyncSystemTime;
}

VOID
AD_SetSyncSystemTime(
    BOOLEAN bValue
    )
{
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    gbSyncSystemTime = bValue;

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
}
