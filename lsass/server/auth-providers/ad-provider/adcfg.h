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
 *        adcfg.h
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
 *
 */
#ifndef __AD_CFG_H__
#define __AD_CFG_H__

typedef DWORD (*PFN_AD_CONFIG_HANDLER)(
                    PLSA_AD_CONFIG pConfig,
                    PCSTR          pszName,
                    PCSTR          pszValue
                    );

typedef struct __AD_CONFIG_HANDLER
{
    PCSTR                 pszId;
    PFN_AD_CONFIG_HANDLER pfnHandler;
} AD_CONFIG_HANDLER, *PAD_CONFIG_HANDLER;

DWORD
AD_TransferConfigContents(
    PLSA_AD_CONFIG pSrcConfig,
    PLSA_AD_CONFIG pDstConfig
    );

DWORD
AD_InitializeConfig(
    PLSA_AD_CONFIG pConfig
    );

VOID
AD_FreeConfig(
    PLSA_AD_CONFIG pConfig
    );

VOID
AD_FreeConfigContents(
    PLSA_AD_CONFIG pConfig
    );

VOID
AD_FreeConfigMemberInList(
    PVOID pItem,
    PVOID pUserData
    );

DWORD
AD_ParseConfigFile(
    PCSTR          pszConfigFilePath,
    PLSA_AD_CONFIG pConfig
    );

DWORD
AD_ConfigStartSection(
    PCSTR    pszSectionName,
    PVOID    pData,
    PBOOLEAN pbSkipSection,
    PBOOLEAN pbContinue
    );

DWORD
AD_ConfigNameValuePair(
    PCSTR    pszName,
    PCSTR    pszValue,
    PVOID    pData,
    PBOOLEAN pbContinue
    );

BOOLEAN
AD_GetBooleanConfigValue(
    PCSTR pszValue
    );

DWORD
AD_SetConfigFilePath(
    PCSTR pszConfigFilePath
    );

DWORD
AD_GetConfigFilePath(
    PSTR* ppszConfigFilePath
    );

DWORD
AD_GetUnprovisionedModeShell(
    PSTR* ppszUnprovisionedModeShell
    );

DWORD
AD_GetHomedirPrefixPath(
    PSTR* ppszPath
    );

DWORD
AD_GetUnprovisionedModeHomedirTemplate(
    PSTR* ppszUnprovisionedModeHomedirTemplate
    );

CHAR
AD_GetSpaceReplacement(
    VOID
    );

DWORD
AD_GetCacheReaperTimeoutSecs(
    VOID
    );

DWORD
AD_GetMachinePasswordSyncPwdLifetime(
    VOID
    );

DWORD
AD_GetClockDriftSeconds(
    VOID
    );

DWORD
AD_GetCacheEntryExpirySeconds(
    VOID
    );

DWORD
AD_GetUmask(
    VOID
    );

DWORD
AD_GetSkelDirs(
    PSTR* ppszSkelDirs
    );

BOOLEAN
AD_GetLDAPSignAndSeal(
    VOID
    );

double
AD_GetMachineTGTGraceSeconds(
    VOID
    );

DWORD
AD_AddAllowedMember(
    PCSTR pszSID
    );

VOID
AD_DeleteFromMembersList(
    PCSTR pszMember
    );

DWORD
AD_GetAllowedMembersList(
    PSTR** pppszMembers,
    PDWORD pdwNumMembers
    );

BOOLEAN
AD_ShouldFilterUserLoginsByGroup(
    VOID
    );

BOOLEAN
AD_IsMemberAllowed(
    PCSTR pszSID
    );

VOID
AD_FreeAllowedSIDs_InLock(
    VOID);

BOOLEAN
AD_ShouldAssumeDefaultDomain(
    VOID
    );

BOOLEAN
AD_ShouldSyncSystemTime(
    VOID
    );

BOOLEAN
AD_EventlogEnabled(
    VOID
    );

BOOLEAN
AD_ShouldLogNetworkConnectionEvents(
    VOID
    );

BOOLEAN
AD_ShouldCreateK5Login(
    VOID
    );

BOOLEAN
AD_ShouldCreateHomeDir(
    VOID
    );

BOOLEAN
AD_ShouldRefreshUserCreds(
    VOID
    );

AD_CELL_SUPPORT
AD_GetCellSupport(
    VOID
    );

BOOLEAN
AD_GetTrimUserMembershipEnabled(
    VOID
    );

BOOLEAN
AD_GetNssGroupMembersCacheOnlyEnabled(
    VOID
    );

BOOLEAN
AD_GetNssUserMembershipCacheOnlyEnabled(
    VOID
    );

#endif /* __AD_CFG_H__ */

