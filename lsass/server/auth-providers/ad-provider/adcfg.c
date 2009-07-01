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

static
DWORD
AD_SetConfig_EnableEventLog(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_LoginShellTemplate(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_HomeDirTemplate(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_CheckPunctuationChar(
    IN PCSTR pszName,
    IN PCSTR pszValue,
    IN BOOLEAN bAllowSpace
    );

static
DWORD
AD_SetConfig_SpaceReplacement(
    IN OUT PLSA_AD_CONFIG pConfig,
    IN PCSTR          pszName,
    IN PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_DomainSeparator(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_CachePurgeTimeout(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_MachinePasswordLifespan(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_CacheEntryExpiry(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_LDAPSignAndSeal(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_RequireMembershipOf(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_AssumeDefaultDomain(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_SyncSystemTime(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_LogNetworkConnectionEvents(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_CreateK5Login(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_CreateHomeDir(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_RefreshUserCreds(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_SkelDirs(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_Umask(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_HomedirPrefix(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_CellSupport(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_TrimUserMembershipEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_NssGroupMembersCacheOnlyEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_NssUserMembershipCacheOnlyEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_NssEnumerationEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    );

static
DWORD
AD_SetConfig_DomainManagerCheckDomainOnlineSeconds(
    IN PLSA_AD_CONFIG pConfig,
    IN PCSTR pszName,
    IN PCSTR pszValue
    );

static
DWORD
AD_SetConfig_DomainManagerUnknownDomainCacheTimeoutSeconds(
    IN PLSA_AD_CONFIG pConfig,
    IN PCSTR pszName,
    IN PCSTR pszValue
    );

static AD_CONFIG_HANDLER gADConfigHandlers[] =
{
    {"enable-eventlog",               &AD_SetConfig_EnableEventLog},
    {"login-shell-template",          &AD_SetConfig_LoginShellTemplate},
    {"homedir-template",              &AD_SetConfig_HomeDirTemplate},
    {"space-replacement",             &AD_SetConfig_SpaceReplacement},
    {"domain-separator",              &AD_SetConfig_DomainSeparator},
    {"cache-purge-timeout",           &AD_SetConfig_CachePurgeTimeout},
    {"machine-password-lifespan",     &AD_SetConfig_MachinePasswordLifespan},
    {"cache-entry-expiry",            &AD_SetConfig_CacheEntryExpiry},
    {"ldap-sign-and-seal",            &AD_SetConfig_LDAPSignAndSeal},
    {"require-membership-of",         &AD_SetConfig_RequireMembershipOf},
    {"assume-default-domain",         &AD_SetConfig_AssumeDefaultDomain},
    {"sync-system-time",              &AD_SetConfig_SyncSystemTime},
    {"log-network-connection-events", &AD_SetConfig_LogNetworkConnectionEvents},
    {"create-k5login",                &AD_SetConfig_CreateK5Login},
    {"create-homedir",                &AD_SetConfig_CreateHomeDir},
    {"skeleton-dirs",                 &AD_SetConfig_SkelDirs},
    {"homedir-umask",                 &AD_SetConfig_Umask},
    {"homedir-prefix",                &AD_SetConfig_HomedirPrefix},
    {"refresh-user-credentials",      &AD_SetConfig_RefreshUserCreds},
    {"cell-support",                  &AD_SetConfig_CellSupport},
    {"trim-user-membership",          &AD_SetConfig_TrimUserMembershipEnabled},
    {"nss-group-members-query-cache-only",   &AD_SetConfig_NssGroupMembersCacheOnlyEnabled},
    {"nss-user-membership-query-cache-only", &AD_SetConfig_NssUserMembershipCacheOnlyEnabled},
    {"nss-enumeration-enabled",              &AD_SetConfig_NssEnumerationEnabled},
    {"domain-manager-check-domain-online-interval", &AD_SetConfig_DomainManagerCheckDomainOnlineSeconds},
    {"domain-manager-unknown-domain-cache-timeout", &AD_SetConfig_DomainManagerUnknownDomainCacheTimeoutSeconds},
};

DWORD
AD_TransferConfigContents(
    PLSA_AD_CONFIG pSrcConfig,
    PLSA_AD_CONFIG pDstConfig
    )
{
    AD_FreeConfigContents(pDstConfig);

    *pDstConfig = *pSrcConfig;

    memset(pSrcConfig, 0, sizeof(LSA_AD_CONFIG));

    return 0;
}

DWORD
AD_InitializeConfig(
    PLSA_AD_CONFIG pConfig
    )
{
    DWORD dwError = 0;

    memset(pConfig, 0, sizeof(LSA_AD_CONFIG));

    pConfig->bAssumeDefaultDomain = FALSE;
    pConfig->bCreateHomeDir   = TRUE;
    pConfig->bCreateK5Login   = TRUE;
    pConfig->bLDAPSignAndSeal = FALSE;
    pConfig->bSyncSystemTime  = TRUE;
    /* Leave chSpaceReplacement and chDomainSeparator set as '\0' for now.
     * After the config file is parsed, they will be assigned default values
     * if they are still set to '\0'.
     *
     * This is done so that their values
     * can be swapped (chSpaceReplacement=\ chDomainSeparator=^), but not
     * assigned to the same value.
     */
    pConfig->dwCacheReaperTimeoutSecs = AD_CACHE_REAPER_TIMEOUT_DEFAULT_SECS;
    pConfig->dwCacheEntryExpirySecs   = AD_CACHE_ENTRY_EXPIRY_DEFAULT_SECS;
    pConfig->dwMachinePasswordSyncLifetime = AD_MACHINE_PASSWORD_SYNC_DEFAULT_SECS;
    pConfig->dwUmask          = AD_DEFAULT_UMASK;

    pConfig->bEnableEventLog = FALSE;
    pConfig->bShouldLogNetworkConnectionEvents = TRUE;
    pConfig->bRefreshUserCreds = TRUE;
    pConfig->CellSupport = AD_CELL_SUPPORT_FULL;
    pConfig->bTrimUserMembershipEnabled = TRUE;
    pConfig->bNssGroupMembersCacheOnlyEnabled = TRUE;
    pConfig->bNssUserMembershipCacheOnlyEnabled = FALSE;
    pConfig->bNssEnumerationEnabled = FALSE;

    pConfig->DomainManager.dwCheckDomainOnlineSeconds = 5 * LSA_SECONDS_IN_MINUTE;
    pConfig->DomainManager.dwUnknownDomainCacheTimeoutSeconds = 1 * LSA_SECONDS_IN_HOUR;

    dwError = LsaAllocateString(
                    AD_DEFAULT_SHELL,
                    &pConfig->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    AD_DEFAULT_HOMEDIR_PREFIX,
                    &pConfig->pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    AD_DEFAULT_HOMEDIR_TEMPLATE,
                    &pConfig->pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    AD_DEFAULT_SKELDIRS,
                    &pConfig->pszSkelDirs);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    AD_FreeConfigContents(pConfig);

    goto cleanup;
}

VOID
AD_FreeConfig(
    PLSA_AD_CONFIG pConfig
    )
{
    AD_FreeConfigContents(pConfig);
    LsaFreeMemory(pConfig);
}

VOID
AD_FreeConfigContents(
    PLSA_AD_CONFIG pConfig
    )
{
    LSA_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    LSA_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);
    LSA_SAFE_FREE_STRING(pConfig->pszShell);
    LSA_SAFE_FREE_STRING(pConfig->pszSkelDirs);

    if (pConfig->pUnresolvedMemberList)
    {
        LsaDLinkedListForEach(
                        pConfig->pUnresolvedMemberList,
                        &AD_FreeConfigMemberInList,
                        NULL);
        LsaDLinkedListFree(pConfig->pUnresolvedMemberList);
        pConfig->pUnresolvedMemberList = NULL;
    }
}

VOID
AD_FreeConfigMemberInList(
    PVOID pItem,
    PVOID pUserData
    )
{
    LSA_SAFE_FREE_MEMORY(pItem);
}

DWORD
AD_ParseConfigFile(
    PCSTR          pszConfigFilePath,
    PLSA_AD_CONFIG pConfig
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    dwError = LsaParseConfigFile(
                pszConfigFilePath,
                LSA_CFG_OPTION_STRIP_ALL,
                &AD_ConfigStartSection,
                NULL,
                &AD_ConfigNameValuePair,
                NULL,
                pConfig);
    BAIL_ON_LSA_ERROR(dwError);

    if (pConfig->chSpaceReplacement == 0)
    {
        pConfig->chSpaceReplacement = AD_SPACE_REPLACEMENT_DEFAULT;
    }
    if (pConfig->chDomainSeparator == 0)
    {
        pConfig->chDomainSeparator = LSA_DOMAIN_SEPARATOR_DEFAULT;
    }

    if (pConfig->chSpaceReplacement == pConfig->chDomainSeparator)
    {
        LSA_LOG_ERROR("Error: space-replacement and domain-separator are set to '%c' in the config file. Their values must be unique.",
                        pConfig->chSpaceReplacement);
        dwError = LSA_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

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
        (strncasecmp(pszSectionName, AD_CFG_TAG_AUTH_PROVIDER, sizeof(AD_CFG_TAG_AUTH_PROVIDER)-1) &&
         strncasecmp(pszSectionName, "global", sizeof("global")-1)))
    {
        bSkipSection = TRUE;
        goto done;
    }

    if (!strncasecmp(pszSectionName, AD_CFG_TAG_AUTH_PROVIDER, sizeof(AD_CFG_TAG_AUTH_PROVIDER)-1))
    {
        pszLibName = pszSectionName + sizeof(AD_CFG_TAG_AUTH_PROVIDER) - 1;
        if (IsNullOrEmptyString(pszLibName) ||
            strcasecmp(pszLibName, AD_CFG_TAG_AD_PROVIDER)) {
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
AD_ConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    )
{
    DWORD dwError = 0;

    if (!IsNullOrEmptyString(pszName))
    {
        DWORD iHandler = 0;
        DWORD nHandlers = sizeof(gADConfigHandlers)/sizeof(gADConfigHandlers[0]);

        for (; iHandler < nHandlers; iHandler++)
        {
            if (!strcasecmp(gADConfigHandlers[iHandler].pszId, pszName))
            {
                gADConfigHandlers[iHandler].pfnHandler(
                                (PLSA_AD_CONFIG)pData,
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
AD_SetConfig_EnableEventLog(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bEnableEventLog = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_LoginShellTemplate(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszShell = NULL;

    BAIL_ON_INVALID_STRING(pszValue);

    if (access(pszValue, X_OK) != 0)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(
                    pszValue,
                    &pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_STRING(pConfig->pszShell);

    pConfig->pszShell = pszShell;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszShell);

    goto cleanup;
}

static
DWORD
AD_SetConfig_HomeDirTemplate(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PSTR  pszTemplate = NULL;

    BAIL_ON_INVALID_STRING(pszValue);

    dwError = LsaAllocateString(
                    pszValue,
                    &pszTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);

    pConfig->pszHomedirTemplate = pszTemplate;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszTemplate);

    goto cleanup;
}

static
DWORD
AD_CheckPunctuationChar(
    IN PCSTR pszName,
    IN PCSTR pszValue,
    IN BOOLEAN bAllowSpace
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    BAIL_ON_INVALID_STRING(pszValue);

    if (pszValue[0] == 0 || pszValue[1] != 0)
    {
        LSA_LOG_ERROR(
                "Error: '%s' is an invalid setting for %s. %s may only be set to a single character.",
                pszValue,
                pszName,
                pszName);
        dwError = LSA_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!ispunct((int)pszValue[0]) && !(bAllowSpace && pszValue[0] == ' '))
    {
        LSA_LOG_ERROR(
                "Error: %s must be set to a punctuation character%s; the value provided is '%s'.",
                pszName,
                bAllowSpace ? " or space" : "",
                pszValue);
        dwError = LSA_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszValue[0] == '@')
    {
        LSA_LOG_ERROR(
                "Error: %s may not be set to @; the value provided is '%s'.",
                pszName,
                pszValue);
        dwError = LSA_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_SetConfig_SpaceReplacement(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_STRING(pszValue);

    dwError = AD_CheckPunctuationChar(
                    pszName,
                    pszValue,
                    TRUE);
    BAIL_ON_LSA_ERROR(dwError);

    pConfig->chSpaceReplacement = pszValue[0];

error:

    return dwError;
}

static
DWORD
AD_SetConfig_DomainSeparator(
    IN OUT PLSA_AD_CONFIG pConfig,
    IN PCSTR          pszName,
    IN PCSTR          pszValue
    )
{
    DWORD dwError = 0;

    BAIL_ON_INVALID_STRING(pszValue);

    dwError = AD_CheckPunctuationChar(
                    pszName,
                    pszValue,
                    FALSE);
    BAIL_ON_LSA_ERROR(dwError);

    pConfig->chDomainSeparator = pszValue[0];

error:

    return dwError;
}

static
DWORD
AD_SetConfig_CachePurgeTimeout(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    DWORD dwCacheReaperTimeoutSecs = 0;

    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LsaParseDateString(
                        pszValue,
                        &dwCacheReaperTimeoutSecs);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwCacheReaperTimeoutSecs < AD_CACHE_REAPER_TIMEOUT_MINIMUM_SECS)
    {
        LSA_LOG_ERROR("Failed to set CacheReaperTimeoutSecs to %u.  Minimum is %u.",
                      dwCacheReaperTimeoutSecs,
                      AD_CACHE_REAPER_TIMEOUT_MINIMUM_SECS);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwCacheReaperTimeoutSecs > AD_CACHE_REAPER_TIMEOUT_MAXIMUM_SECS)
    {
        LSA_LOG_ERROR("Failed to set CacheReaperTimeoutSecs to %u.  Maximum is %u.",
                      dwCacheReaperTimeoutSecs,
                      AD_CACHE_REAPER_TIMEOUT_MAXIMUM_SECS);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->dwCacheReaperTimeoutSecs = dwCacheReaperTimeoutSecs;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_SetConfig_MachinePasswordLifespan(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    DWORD dwMachinePasswordSyncPwdLifetime = 0;

    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LsaParseDateString(
                        pszValue,
                        &dwMachinePasswordSyncPwdLifetime);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwMachinePasswordSyncPwdLifetime < AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS)
    {
        LSA_LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Minimum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwMachinePasswordSyncPwdLifetime > AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS)
    {
        LSA_LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Maximum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->dwMachinePasswordSyncLifetime = dwMachinePasswordSyncPwdLifetime;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_SetConfig_CacheEntryExpiry(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    DWORD dwExpirySecs = 0;

    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LsaParseDateString(
                    pszValue,
                    &dwExpirySecs);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwExpirySecs < AD_CACHE_ENTRY_EXPIRY_MINIMUM_SECS)
    {
        LSA_LOG_ERROR("Failed to set CacheEntryExpiry to %u.  Minimum is %u.",
                        dwExpirySecs,
                        AD_CACHE_ENTRY_EXPIRY_MINIMUM_SECS);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwExpirySecs > AD_CACHE_ENTRY_EXPIRY_MAXIMUM_SECS)
    {
        LSA_LOG_ERROR("Failed to set CacheEntryExpiry to %u.  Maximum is %u.",
                        dwExpirySecs,
                        AD_CACHE_ENTRY_EXPIRY_MAXIMUM_SECS);
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->dwCacheEntryExpirySecs = dwExpirySecs;

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_SetConfig_LDAPSignAndSeal(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bLDAPSignAndSeal = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_RequireMembershipOf(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    PCSTR pszIter = pszValue;
    size_t stLen = 0;
    PSTR  pszMember = NULL;

    if (IsNullOrEmptyString(pszValue))
    {
        goto cleanup;
    }

    while ((stLen = strcspn(pszIter, ",")) != 0)
    {
        dwError = LsaStrndup(
                        pszIter,
                        stLen,
                        &pszMember);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStripWhitespace(pszMember, TRUE, TRUE);

        if (*pszMember)
        {
            dwError = LsaDLinkedListAppend(
                            &pConfig->pUnresolvedMemberList,
                            pszMember);
            BAIL_ON_LSA_ERROR(dwError);

            pszMember = NULL;
        }
        else
        {
            LSA_SAFE_FREE_STRING(pszMember);
        }

        pszIter += stLen;

        stLen = strspn(pszIter, ",");

        pszIter += stLen;
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszMember);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_SetConfig_AssumeDefaultDomain(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bAssumeDefaultDomain = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_SyncSystemTime(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bSyncSystemTime = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_LogNetworkConnectionEvents(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bShouldLogNetworkConnectionEvents = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_CreateK5Login(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bCreateK5Login = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_CreateHomeDir(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bCreateHomeDir = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_RefreshUserCreds(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bRefreshUserCreds = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_SkelDirs(
    PLSA_AD_CONFIG pConfig,
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
DWORD
AD_SetConfig_Umask(
    PLSA_AD_CONFIG pConfig,
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
        pConfig->dwUmask = dwOct;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_SetConfig_HomedirPrefix(
    PLSA_AD_CONFIG pConfig,
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
AD_SetConfig_CellSupport(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    DWORD dwError = 0;
    if (!strcasecmp(pszValue, "unprovisioned"))
    {
        pConfig->CellSupport = AD_CELL_SUPPORT_UNPROVISIONED;
    }
#if 0
    else if (!strcasecmp(pszValue, "file"))
    {
        pConfig->CellSupport = AD_CELL_SUPPORT_FILE;
    }
#endif
    else if (!strcasecmp(pszValue, "full"))
    {
        pConfig->CellSupport = AD_CELL_SUPPORT_FULL;
    }
    else if (!strcasecmp(pszValue, "default-schema"))
    {
        pConfig->CellSupport = AD_CELL_SUPPORT_DEFAULT_SCHEMA;
    }
    else
    {
        LSA_LOG_ERROR("Invalid value for cell-support parameter");
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_SetConfig_TrimUserMembershipEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bTrimUserMembershipEnabled = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_NssGroupMembersCacheOnlyEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bNssGroupMembersCacheOnlyEnabled = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_NssUserMembershipCacheOnlyEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bNssUserMembershipCacheOnlyEnabled = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_NssEnumerationEnabled(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
    )
{
    pConfig->bNssEnumerationEnabled = AD_GetBooleanConfigValue(pszValue);

    return 0;
}

static
DWORD
AD_SetConfig_DomainManagerCheckDomainOnlineSeconds(
    IN PLSA_AD_CONFIG pConfig,
    IN PCSTR pszName,
    IN PCSTR pszValue
    )
{
    DWORD dwError = 0;
    DWORD result = 0;

    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LsaParseDateString(pszValue, &result);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->DomainManager.dwCheckDomainOnlineSeconds = result;

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
AD_SetConfig_DomainManagerUnknownDomainCacheTimeoutSeconds(
    IN PLSA_AD_CONFIG pConfig,
    IN PCSTR pszName,
    IN PCSTR pszValue
    )
{
    DWORD dwError = 0;
    DWORD result = 0;

    if (!IsNullOrEmptyString(pszValue))
    {
        dwError = LsaParseDateString(pszValue, &result);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->DomainManager.dwUnknownDomainCacheTimeoutSeconds = result;

cleanup:
    return dwError;

error:
    goto cleanup;
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
AD_GetUnprovisionedModeShell(
    PSTR* ppszUnprovisionedModeShell
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszUnprovisionedModeShell = NULL;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    if (!IsNullOrEmptyString(gpLsaAdProviderState->config.pszShell))
    {
        dwError = LsaAllocateString(
                        gpLsaAdProviderState->config.pszShell,
                        &pszUnprovisionedModeShell);
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
AD_GetHomedirPrefixPath(
    PSTR* ppszPath
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR  pszHomedirPrefixPath = NULL;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    if (!IsNullOrEmptyString(gpLsaAdProviderState->config.pszHomedirPrefix))
    {
        dwError = LsaAllocateString(
                        gpLsaAdProviderState->config.pszHomedirPrefix,
                        &pszHomedirPrefixPath
                        );
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszPath = pszHomedirPrefixPath;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwError;

error:

    *ppszPath = NULL;

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

    if (!IsNullOrEmptyString(gpLsaAdProviderState->config.pszHomedirTemplate))
    {
        dwError = LsaAllocateString(
                        gpLsaAdProviderState->config.pszHomedirTemplate,
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
                    &pszConfigFilePathLocal);
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

CHAR
AD_GetSpaceReplacement(
    VOID
    )
{
    BOOLEAN bInLock = FALSE;
    CHAR chSaved = 0;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    chSaved = gpLsaAdProviderState->config.chSpaceReplacement;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return chSaved;
}

DWORD
AD_GetCacheReaperTimeoutSecs(
    VOID
    )
{
    DWORD dwCacheReaperTimeoutSecs = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwCacheReaperTimeoutSecs = gpLsaAdProviderState->config.dwCacheReaperTimeoutSecs;
    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwCacheReaperTimeoutSecs;
}

DWORD
AD_GetMachinePasswordSyncPwdLifetime(
    VOID
    )
{
    DWORD dwMachinePasswordSyncPwdLifetime = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwMachinePasswordSyncPwdLifetime = gpLsaAdProviderState->config.dwMachinePasswordSyncLifetime;
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
    dwClockDriftSecs = gpLsaAdProviderState->dwMaxAllowedClockDriftSeconds;
    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwClockDriftSecs;
}

DWORD
AD_GetCacheEntryExpirySeconds(
    VOID
    )
{
    DWORD dwResult = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    dwResult = gpLsaAdProviderState->config.dwCacheEntryExpirySecs;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwResult;
}

DWORD
AD_GetUmask(
    VOID
    )
{
    DWORD dwResult = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);
    dwResult = gpLsaAdProviderState->config.dwUmask;
    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwResult;
}

DWORD
AD_GetSkelDirs(
    PSTR* ppszSkelDirs
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR pszSkelDirs = NULL;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    if (!IsNullOrEmptyString(gpLsaAdProviderState->config.pszSkelDirs))
    {
        dwError = LsaAllocateString(
                        gpLsaAdProviderState->config.pszSkelDirs,
                        &pszSkelDirs);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppszSkelDirs = pszSkelDirs;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwError;

error:

    *ppszSkelDirs = NULL;

    goto cleanup;
}

BOOLEAN
AD_GetLDAPSignAndSeal(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bResult = gpLsaAdProviderState->config.bLDAPSignAndSeal;

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

static
BOOLEAN
AD_IsInMembersList_InLock(
    PCSTR pszMember
    )
{
    PDLINKEDLIST pIter = NULL;
    BOOLEAN      bInList = FALSE;

    for (pIter = gpLsaAdProviderState->config.pUnresolvedMemberList;
         pIter;
         pIter = pIter->pNext)
    {
        if (!strcmp(pszMember, (PSTR)pIter->pItem))
        {
            bInList = TRUE;
            break;
        }
    }

    return bInList;
}

static
VOID
AD_DeleteFromMembersList_InLock(
    PCSTR pszMember
    )
{
    PDLINKEDLIST pIter = NULL;
    PVOID        pItem = NULL;

    for (pIter = gpLsaAdProviderState->config.pUnresolvedMemberList;
         pIter;
         pIter = pIter->pNext)
    {
        if (!strcmp(pszMember, (PSTR)pIter->pItem))
        {
            pItem = pIter->pItem;
            break;
        }
    }

    if (pItem)
    {
        LsaDLinkedListDelete(&gpLsaAdProviderState->config.pUnresolvedMemberList,
                             pItem);

        LsaFreeMemory(pItem);
    }
}

VOID
AD_DeleteFromMembersList(
    PCSTR pszMember
    )
{
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    AD_DeleteFromMembersList_InLock(pszMember);

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
}

static
void
AD_FreeHashStringKey(
    const LSA_HASH_ENTRY *pEntry)
{
    PSTR pszKeyCopy = (PSTR)pEntry->pKey;
    LSA_SAFE_FREE_STRING(pszKeyCopy);
}

static
DWORD
AD_CopyHashStringKey(
    const LSA_HASH_ENTRY *pEntry,
    LSA_HASH_ENTRY       *pEntryCopy
    )
{
    DWORD dwError = 0;
    PSTR  pszKeyCopy = NULL;

    dwError = LsaAllocateString(
                    (PSTR)pEntry->pKey,
                    &pszKeyCopy);
    BAIL_ON_LSA_ERROR(dwError);

    pEntryCopy->pKey = pszKeyCopy;
    pEntryCopy->pValue = pszKeyCopy;

cleanup:

    return dwError;

error:

    LSA_SAFE_FREE_STRING(pszKeyCopy);

    goto cleanup;
}

DWORD
AD_AddAllowedMember(
    IN PCSTR               pszSID,
    IN PSTR                pszMember,
    IN OUT PLSA_HASH_TABLE *ppAllowedMemberList
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    PSTR  pszValue = NULL;
    PSTR  pszSIDCopy = NULL;
    PSTR  pszMemberCopy = NULL;
    PLSA_HASH_TABLE pAllowedMemberList = *ppAllowedMemberList;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (!gpAllowedSIDs)
    {
        dwError = LsaHashCreate(
                        11,
                        LsaHashCaselessStringCompare,
                        LsaHashCaselessString,
                        AD_FreeHashStringKey,
                        AD_CopyHashStringKey,
                        &gpAllowedSIDs);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!pAllowedMemberList)
    {
        dwError = LsaHashCreate(
                        11,
                        LsaHashCaselessStringCompare,
                        LsaHashCaselessString,
                        AD_FreeHashStringKey,
                        AD_CopyHashStringKey,
                        &pAllowedMemberList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(
                    pszSID,
                    &pszSIDCopy);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    pszMember,
                    &pszMemberCopy);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashSetValue(
                    pAllowedMemberList,
                    pszSIDCopy,
                    pszMemberCopy);
    BAIL_ON_LSA_ERROR(dwError);

    pszSIDCopy = NULL;
    pszMemberCopy = NULL;

    if ( AD_IsInMembersList_InLock(pszMember) )
    {
        dwError = LsaHashGetValue(
                      gpAllowedSIDs,
                      pszSID,
                      (PVOID*)&pszValue);
        if (dwError == ENOENT)
        {
            dwError = LsaAllocateString(
                          pszSID,
                          &pszSIDCopy);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaAllocateString(
                          pszMember,
                          &pszMemberCopy);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaHashSetValue(
                          gpAllowedSIDs,
                          pszSIDCopy,
                          pszMemberCopy);
            BAIL_ON_LSA_ERROR(dwError);

            pszSIDCopy = NULL;
            pszMemberCopy = NULL;
        }

        AD_DeleteFromMembersList_InLock(pszMember);
    }

    *ppAllowedMemberList = pAllowedMemberList;

cleanup:

    LSA_SAFE_FREE_STRING(pszSIDCopy);
    LSA_SAFE_FREE_STRING(pszMemberCopy);

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;

error:

    if ( ! *ppAllowedMemberList )
    {
        LsaHashSafeFree(&pAllowedMemberList);
    }

    goto cleanup;
}

DWORD
AD_GetMemberLists(
    PSTR** pppszMembers,
    PDWORD pdwNumMembers,
    PLSA_HASH_TABLE* ppAllowedMemberList
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;
    DWORD dwNumMembers = 0;
    PDLINKEDLIST pIter = NULL;
    PSTR* ppszMembers = NULL;
    PLSA_HASH_TABLE pAllowedMemberList = NULL;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    for (pIter = gpLsaAdProviderState->config.pUnresolvedMemberList; pIter; pIter = pIter->pNext)
    {
        dwNumMembers++;
    }

    if (dwNumMembers)
    {
        DWORD iMember = 0;

        dwError = LsaAllocateMemory(
                        dwNumMembers * sizeof(PSTR),
                        (PVOID*)&ppszMembers);
        BAIL_ON_LSA_ERROR(dwError);

        for (pIter = gpLsaAdProviderState->config.pUnresolvedMemberList;
             pIter;
             pIter = pIter->pNext, iMember++)
        {
            dwError = LsaAllocateString(
                            (PSTR)pIter->pItem,
                            &ppszMembers[iMember]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    if ( gpAllowedSIDs )
    {
        dwError = LsaHashCopy(
                      gpAllowedSIDs,
                      &pAllowedMemberList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppszMembers = ppszMembers;
    *pdwNumMembers = dwNumMembers;
    *ppAllowedMemberList = pAllowedMemberList;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwError;

error:

    if (ppszMembers)
    {
        LsaFreeStringArray(ppszMembers, dwNumMembers);
    }

    *pppszMembers = NULL;
    *pdwNumMembers = 0;
    *ppAllowedMemberList = NULL;

    LsaHashSafeFree(&pAllowedMemberList);

    goto cleanup;
}

static
BOOLEAN
AD_ShouldFilterUserLoginsByGroup_InLock(
    VOID
    )
{
    BOOLEAN bFilter = FALSE;

    if (gpAllowedSIDs || gpLsaAdProviderState->config.pUnresolvedMemberList)
    {
        bFilter = TRUE;
    }

    return bFilter;
}

BOOLEAN
AD_ShouldFilterUserLoginsByGroup(
    VOID
    )
{
    BOOLEAN bFilter = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bFilter = AD_ShouldFilterUserLoginsByGroup_InLock();

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bFilter;
}

BOOLEAN
AD_IsMemberAllowed(
    PCSTR           pszSID,
    PLSA_HASH_TABLE pAllowedMemberList
    )
{
    BOOLEAN bAllowed = FALSE;
    PSTR    pszValue = NULL;

    if (!AD_ShouldFilterUserLoginsByGroup() ||
        (pAllowedMemberList &&
         !LsaHashGetValue(
                        pAllowedMemberList,
                        pszSID,
                        (PVOID*)&pszValue)))
    {
        bAllowed = TRUE;
    }

    return bAllowed;
}

VOID
AD_FreeAllowedSIDs_InLock(
    VOID)
{
    if (gpAllowedSIDs)
    {
        LsaHashSafeFree(&gpAllowedSIDs);
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

    bAssumeDefaultDomain = gpLsaAdProviderState->config.bAssumeDefaultDomain;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bAssumeDefaultDomain;
}

BOOLEAN
AD_ShouldSyncSystemTime(
    VOID
    )
{
    BOOLEAN bSyncSystemTime = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bSyncSystemTime = gpLsaAdProviderState->config.bSyncSystemTime;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bSyncSystemTime;
}

BOOLEAN
AD_EventlogEnabled(
    VOID
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bResult = gpLsaAdProviderState->config.bEnableEventLog;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bResult;
}

BOOLEAN
AD_ShouldLogNetworkConnectionEvents(
    VOID
    )
{
    BOOLEAN bResult = TRUE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bResult = gpLsaAdProviderState->config.bShouldLogNetworkConnectionEvents;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bResult;
}

BOOLEAN
AD_ShouldCreateK5Login(
    VOID
    )
{
    BOOLEAN bResult = TRUE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bResult = gpLsaAdProviderState->config.bCreateK5Login;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bResult;
}

BOOLEAN
AD_ShouldCreateHomeDir(
    VOID
    )
{
    BOOLEAN bCreateHomeDir = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bCreateHomeDir = gpLsaAdProviderState->config.bCreateHomeDir;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bCreateHomeDir;
}

BOOLEAN
AD_ShouldRefreshUserCreds(
    VOID
    )
{
    BOOLEAN bRefreshUserCreds = FALSE;
    BOOLEAN bInLock          = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    bRefreshUserCreds = gpLsaAdProviderState->config.bRefreshUserCreds;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return bRefreshUserCreds;
}

AD_CELL_SUPPORT
AD_GetCellSupport(
    VOID
    )
{
    AD_CELL_SUPPORT result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    result = gpLsaAdProviderState->config.CellSupport;

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return result;
}


BOOLEAN
AD_GetTrimUserMembershipEnabled(
    VOID
    )
{
    BOOLEAN result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    result = gpLsaAdProviderState->config.bTrimUserMembershipEnabled;

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return result;
}

BOOLEAN
AD_GetNssGroupMembersCacheOnlyEnabled(
    VOID
    )
{
    BOOLEAN result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    result = gpLsaAdProviderState->config.bNssGroupMembersCacheOnlyEnabled;

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return result;
}

BOOLEAN
AD_GetNssUserMembershipCacheOnlyEnabled(
    VOID
    )
{
    BOOLEAN result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    result = gpLsaAdProviderState->config.bNssUserMembershipCacheOnlyEnabled;

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return result;
}

BOOLEAN
AD_GetNssEnumerationEnabled(
    VOID
    )
{
    BOOLEAN result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    result = gpLsaAdProviderState->config.bNssEnumerationEnabled;

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return result;
}

DWORD
AD_GetDomainManagerCheckDomainOnlineSeconds(
    VOID
    )
{
    DWORD result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
    result = gpLsaAdProviderState->config.DomainManager.dwCheckDomainOnlineSeconds;
    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return result;
}

DWORD
AD_GetDomainManagerUnknownDomainCacheTimeoutSeconds(
    VOID
    )
{
    DWORD result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);
    result = gpLsaAdProviderState->config.DomainManager.dwUnknownDomainCacheTimeoutSeconds;
    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return result;
}
