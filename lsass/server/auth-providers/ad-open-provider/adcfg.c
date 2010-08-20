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
AD_CheckPunctuationChar(
    IN PCSTR pszName,
    IN PCSTR pszValue,
    IN BOOLEAN bAllowSpace
    );

static
DWORD
AD_SetConfig_SpaceReplacement(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    PCSTR          pszValue
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
AD_SetConfig_Umask(
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
AD_SetConfig_MachinePasswordLifespan(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    DWORD          dwMachinePasswordSyncPwdLifetime
    );


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
    pConfig->dwCacheSizeCap           = 0;
    pConfig->dwMachinePasswordSyncLifetime = AD_MACHINE_PASSWORD_SYNC_DEFAULT_SECS;
    pConfig->dwUmask          = AD_DEFAULT_UMASK;

    pConfig->bEnableEventLog = FALSE;
    pConfig->bShouldLogNetworkConnectionEvents = TRUE;
    pConfig->bRefreshUserCreds = TRUE;
    pConfig->CellSupport = AD_CELL_SUPPORT_UNPROVISIONED;
    pConfig->CacheBackend = AD_CACHE_IN_MEMORY;
    pConfig->bTrimUserMembershipEnabled = TRUE;
    pConfig->bNssGroupMembersCacheOnlyEnabled = TRUE;
    pConfig->bNssUserMembershipCacheOnlyEnabled = FALSE;
    pConfig->bNssEnumerationEnabled = FALSE;

    pConfig->DomainManager.dwCheckDomainOnlineSeconds = 5 * LSA_SECONDS_IN_MINUTE;
    pConfig->DomainManager.dwUnknownDomainCacheTimeoutSeconds = 1 * LSA_SECONDS_IN_HOUR;

    dwError = LwAllocateString(
                    AD_DEFAULT_SHELL,
                    &pConfig->pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    AD_DEFAULT_HOMEDIR_PREFIX,
                    &pConfig->pszHomedirPrefix);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
                    AD_DEFAULT_HOMEDIR_TEMPLATE,
                    &pConfig->pszHomedirTemplate);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
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
    LwFreeMemory(pConfig);
}

VOID
AD_FreeConfigContents(
    PLSA_AD_CONFIG pConfig
    )
{
    LW_SAFE_FREE_STRING(pConfig->pszHomedirPrefix);
    LW_SAFE_FREE_STRING(pConfig->pszHomedirTemplate);
    LW_SAFE_FREE_STRING(pConfig->pszShell);
    LW_SAFE_FREE_STRING(pConfig->pszSkelDirs);

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
    LW_SAFE_FREE_MEMORY(pItem);
}

DWORD
AD_ReadRegistry(
    PLSA_AD_CONFIG pConfig
    )
{
    DWORD dwError = 0;
    PSTR pszSpaceReplacement = NULL;
    PSTR pszDomainSeparator = NULL;
    PSTR pszUmask = NULL;
    PSTR pszUnresolvedMemberList = NULL;
    DWORD dwMachinePasswordSyncLifetime = 0;
    LSA_AD_CONFIG StagingConfig;

    const PCSTR CellSupport[] = {
        "unprovisioned"
    };

    const PCSTR CacheBackend[] =
    {
        "sqlite",
        "memory"
    };

    LSA_CONFIG ADConfigDescription[] =
    {
        {
            "SpaceReplacement",
            TRUE,
            LsaTypeString,
            0,
            MAXDWORD,
            NULL,
            &pszSpaceReplacement
        },
        {
            "DomainSeparator",
            TRUE,
            LsaTypeString,
            0,
            MAXDWORD,
            NULL,
            &pszDomainSeparator
        },
        {
            "HomeDirUmask",
            TRUE,
            LsaTypeString,
            0,
            MAXDWORD,
            NULL,
            &pszUmask
        },
        {
            "RequireMembershipOf",
            TRUE,
            LsaTypeMultiString,
            0,
            MAXDWORD,
            NULL,
            &pszUnresolvedMemberList
        },
        {
            "LoginShellTemplate",
            TRUE,
            LsaTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszShell
        },
        {
            "HomeDirTemplate",
            TRUE,
            LsaTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszHomedirTemplate
        },
        {
            "CachePurgeTimeout",
            TRUE,
            LsaTypeDword,
            AD_CACHE_REAPER_TIMEOUT_MINIMUM_SECS,
            AD_CACHE_REAPER_TIMEOUT_MAXIMUM_SECS,
            NULL,
            &StagingConfig.dwCacheReaperTimeoutSecs
        },
        {
            "MachinePasswordLifespan",
            TRUE,
            LsaTypeDword,
            0,  /* Valid range is 0 or [AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS,*/
            MAXDWORD, /* AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS] */
            NULL,
            &dwMachinePasswordSyncLifetime
        },
        {
            "CacheEntryExpiry",
            TRUE,
            LsaTypeDword,
            AD_CACHE_ENTRY_EXPIRY_MINIMUM_SECS,
            AD_CACHE_ENTRY_EXPIRY_MAXIMUM_SECS,
            NULL,
            &StagingConfig.dwCacheEntryExpirySecs
        },
        {
            "MemoryCacheSizeCap",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.dwCacheSizeCap
        },
        {
            "LdapSignAndSeal",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bLDAPSignAndSeal
        },
        {
            "AssumeDefaultDomain",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bAssumeDefaultDomain
        },
        {
            "SyncSystemTime",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bSyncSystemTime
        },
        {
            "LogNetworkConnectionEvents",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bShouldLogNetworkConnectionEvents
        },
        {
            "CreateK5Login",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bCreateK5Login
        },
        {
            "CreateHomeDir",
            TRUE,
            LsaTypeBoolean,
            0,
            -1,
            NULL,
            &StagingConfig.bCreateHomeDir
        },
        {
            "SkeletonDirs",
            TRUE,
            LsaTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszSkelDirs
        },
        {
            "HomeDirPrefix",
            TRUE,
            LsaTypeString,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.pszHomedirPrefix
        },
        {
            "RefreshUserCredentials",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bRefreshUserCreds
        },
        {
            "TrimUserMembership",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bTrimUserMembershipEnabled
        },
        {
            "CellSupport",
            TRUE,
            LsaTypeEnum,
            AD_CELL_SUPPORT_UNPROVISIONED,
            AD_CELL_SUPPORT_UNPROVISIONED,
            CellSupport,
            &StagingConfig.CellSupport
        },
        {
            "CacheType",
            TRUE,
            LsaTypeEnum,
            AD_CACHE_SQLITE,
            AD_CACHE_IN_MEMORY,
            CacheBackend,
            &StagingConfig.CacheBackend
        },
        {
            "NssGroupMembersQueryCacheOnly",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bNssGroupMembersCacheOnlyEnabled
        },
        {
            "NssUserMembershipQueryCacheOnly",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bNssUserMembershipCacheOnlyEnabled
        },
        {
            "NssEnumerationEnabled",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bNssEnumerationEnabled
        },
        {
            "DomainManagerCheckDomainOnlineInterval",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.DomainManager.dwCheckDomainOnlineSeconds
        },
        {
            "DomainManagerUnknownDomainCacheTimeout",
            TRUE,
            LsaTypeDword,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.DomainManager.dwUnknownDomainCacheTimeoutSeconds
        }
    };

    LSA_CONFIG LsaConfigDescription[] =
    {
        {
            "EnableEventlog",
            TRUE,
            LsaTypeBoolean,
            0,
            MAXDWORD,
            NULL,
            &StagingConfig.bEnableEventLog
        }
    };

    dwError = AD_InitializeConfig(&StagingConfig);
    BAIL_ON_LSA_ERROR(dwError);

    dwMachinePasswordSyncLifetime = AD_MACHINE_PASSWORD_SYNC_DEFAULT_SECS;

    dwError = LsaProcessConfig(
                "Services\\lsass\\Parameters\\Providers\\ActiveDirectory",
                "Policy\\Services\\lsass\\Parameters\\Providers\\ActiveDirectory",
                ADConfigDescription,
                sizeof(ADConfigDescription)/sizeof(ADConfigDescription[0]));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProcessConfig(
                "Services\\lsass\\Parameters",
                "Policy\\Services\\lsass\\Parameters",
                LsaConfigDescription,
                sizeof(LsaConfigDescription)/sizeof(LsaConfigDescription[0]));
    BAIL_ON_LSA_ERROR(dwError);

    /* Special handling is used for some values. */
    AD_SetConfig_SpaceReplacement(
            &StagingConfig,
            "SpaceReplacement",
            pszSpaceReplacement);

    AD_SetConfig_DomainSeparator(
            &StagingConfig,
            "DomainSeparator",
            pszDomainSeparator);

    AD_SetConfig_Umask(
            &StagingConfig,
            "HomeDirUmask",
            pszUmask);

    AD_SetConfig_RequireMembershipOf(
            &StagingConfig,
            "RequireMembershipOf",
            pszUnresolvedMemberList);

    AD_SetConfig_MachinePasswordLifespan(
            &StagingConfig,
            "MachinePasswordLifespan",
            dwMachinePasswordSyncLifetime);

    if (StagingConfig.chSpaceReplacement == 0)
    {
        StagingConfig.chSpaceReplacement = AD_SPACE_REPLACEMENT_DEFAULT;
    }
    if (StagingConfig.chDomainSeparator == 0)
    {
        StagingConfig.chDomainSeparator = LSA_DOMAIN_SEPARATOR_DEFAULT;
    }

    if (StagingConfig.chSpaceReplacement == StagingConfig.chDomainSeparator)
    {
        LSA_LOG_ERROR("Error: %s and %s are both set to '%c' in the config file. Their values must be unique.",
                        "SpaceReplacement",
                        "DomainSeparator",
                        StagingConfig.chSpaceReplacement);
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

    AD_TransferConfigContents(&StagingConfig, pConfig);

cleanup:

    LW_SAFE_FREE_STRING(pszSpaceReplacement);
    LW_SAFE_FREE_STRING(pszDomainSeparator);
    LW_SAFE_FREE_STRING(pszUmask);
    LW_SAFE_FREE_STRING(pszUnresolvedMemberList);

    AD_FreeConfigContents(&StagingConfig);

    return dwError;

error:
    AD_FreeConfigContents(pConfig);
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
    DWORD dwError = LW_ERROR_SUCCESS;

    BAIL_ON_INVALID_STRING(pszValue);

    if (pszValue[0] == 0 || pszValue[1] != 0)
    {
        LSA_LOG_ERROR(
                "Error: '%s' is an invalid setting for %s. "
                "%s may only be set to a single character.",
                pszValue,
                pszName,
                pszName);
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!ispunct((int)pszValue[0]) && !(bAllowSpace && pszValue[0] == ' '))
    {
        LSA_LOG_ERROR(
                "Error: %s must be set to a punctuation character%s; the value provided is '%s'.",
                pszName,
                bAllowSpace ? " or space" : "",
                pszValue);
        dwError = LW_ERROR_INVALID_CONFIG;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pszValue[0] == '@')
    {
        LSA_LOG_ERROR(
                "Error: %s may not be set to @; the value provided is '%s'.",
                pszName,
                pszValue);
        dwError = LW_ERROR_INVALID_CONFIG;
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

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        goto error;
    }

    dwError = AD_CheckPunctuationChar(
                    pszName,
                    pszValue,
                    TRUE);
    if (dwError)
    {
        goto error;
    }

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

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        goto error;
    }

    dwError = AD_CheckPunctuationChar(
                    pszName,
                    pszValue,
                    FALSE);
    if (dwError)
    {
        goto error;
    }

    pConfig->chDomainSeparator = pszValue[0];

error:

    return dwError;
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

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
    {
        goto error;
    }

    cp2[1] = 0;

    for ( cp = pszValue, dwCnt = 0 ; isdigit((int)*cp) ; cp++, dwCnt++ )
    {
        dwOct *= 8;

        cp2[0] = *cp;
        dwVal = atoi(cp2);

        if ( dwVal > 7 )
        {
            LSA_LOG_ERROR("Invalid Umask [%s]", pszValue);
            goto error;
        }

        dwOct += dwVal;
    }

    if ( dwCnt > 4 )
    {
        LSA_LOG_ERROR("Invalid Umask [%s]", pszValue);
        goto error;
    }

    // Disallow 07xx since the user should always have
    // access to his home directory.
    if ( (dwOct & 0700) == 0700 )
    {
        LSA_LOG_ERROR("Invalid Umask [%s]. User cannot access home directory.",
                pszValue);
        goto error;
    }
    else
    {
        pConfig->dwUmask = dwOct;
    }

cleanup:

    return dwError;

error:

    goto cleanup;
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
    PSTR  pszMember = NULL;

    pszIter = pszValue;
    while (pszIter != NULL && *pszIter != '\0')
    {
        dwError = LwStrDupOrNull(
                        pszIter,
                        &pszMember);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDLinkedListAppend(
                        &pConfig->pUnresolvedMemberList,
                        pszMember);
        BAIL_ON_LSA_ERROR(dwError);

        pszMember = NULL;

        pszIter += strlen(pszIter) + 1;
    }

cleanup:

    LW_SAFE_FREE_STRING(pszMember);

    return dwError;

error:

    goto cleanup;
}


static
DWORD
AD_SetConfig_MachinePasswordLifespan(
    PLSA_AD_CONFIG pConfig,
    PCSTR          pszName,
    DWORD          dwMachinePasswordSyncPwdLifetime
    )
{
    DWORD dwError = 0;

    if ((dwMachinePasswordSyncPwdLifetime != 0) &&
        (dwMachinePasswordSyncPwdLifetime < AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS))
    {
        LSA_LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Minimum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        AD_MACHINE_PASSWORD_SYNC_MINIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (dwMachinePasswordSyncPwdLifetime > AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS)
    {
        LSA_LOG_ERROR("Failed to set MachinePasswordSyncPwdLifetime to %u.  Maximum is %u.",
                        dwMachinePasswordSyncPwdLifetime,
                        AD_MACHINE_PASSWORD_SYNC_MAXIMUM_SECS);
        dwError = LW_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pConfig->dwMachinePasswordSyncLifetime = dwMachinePasswordSyncPwdLifetime;

cleanup:

    return dwError;

error:

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

    if (!LW_IS_NULL_OR_EMPTY_STR(gpLsaAdProviderState->config.pszShell))
    {
        dwError = LwAllocateString(
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

    if (!LW_IS_NULL_OR_EMPTY_STR(gpLsaAdProviderState->config.pszHomedirPrefix))
    {
        dwError = LwAllocateString(
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

    if (!LW_IS_NULL_OR_EMPTY_STR(gpLsaAdProviderState->config.pszHomedirTemplate))
    {
        dwError = LwAllocateString(
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

    if (!LW_IS_NULL_OR_EMPTY_STR(gpLsaAdProviderState->config.pszSkelDirs))
    {
        dwError = LwAllocateString(
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

        LwFreeMemory(pItem);
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
    LW_SAFE_FREE_STRING(pszKeyCopy);
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

    dwError = LwAllocateString(
                    (PSTR)pEntry->pKey,
                    &pszKeyCopy);
    BAIL_ON_LSA_ERROR(dwError);

    pEntryCopy->pKey = pszKeyCopy;
    pEntryCopy->pValue = pszKeyCopy;

cleanup:

    return dwError;

error:

    LW_SAFE_FREE_STRING(pszKeyCopy);

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
                        LsaHashCaselessStringHash,
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
                        LsaHashCaselessStringHash,
                        AD_FreeHashStringKey,
                        AD_CopyHashStringKey,
                        &pAllowedMemberList);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateString(
                    pszSID,
                    &pszSIDCopy);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
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
        if (dwError == ERROR_NOT_FOUND)
        {
            dwError = LwAllocateString(
                          pszSID,
                          &pszSIDCopy);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateString(
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

    LW_SAFE_FREE_STRING(pszSIDCopy);
    LW_SAFE_FREE_STRING(pszMemberCopy);

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

        dwError = LwAllocateMemory(
                        dwNumMembers * sizeof(PSTR),
                        (PVOID*)&ppszMembers);
        BAIL_ON_LSA_ERROR(dwError);

        for (pIter = gpLsaAdProviderState->config.pUnresolvedMemberList;
             pIter;
             pIter = pIter->pNext, iMember++)
        {
            dwError = LwAllocateString(
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
        LwFreeStringArray(ppszMembers, dwNumMembers);
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

AD_CACHE_BACKEND
AD_GetCacheBackend(
    VOID
    )
{
    AD_CACHE_BACKEND result = FALSE;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    result = gpLsaAdProviderState->config.CacheBackend;

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return result;
}

DWORD
AD_GetCacheSizeCap(
    VOID
    )
{
    DWORD dwResult = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    dwResult = gpLsaAdProviderState->config.dwCacheSizeCap;

    LEAVE_AD_GLOBAL_DATA_RW_READER_LOCK(bInLock);

    return dwResult;
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

