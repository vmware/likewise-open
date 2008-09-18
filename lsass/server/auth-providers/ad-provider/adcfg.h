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
AD_SetUnprovisionedModeShell(
    PCSTR pszUnprovisionedModeShell
    );

DWORD
AD_GetUnprovisionedModeShell(
    PSTR* ppszUnprovisionedModeShell
    );

DWORD
AD_SetUnprovisionedModeHomedirTemplate(
    PCSTR pszUnprovisionedModeHomedirTemplate
    );

DWORD
AD_GetUnprovisionedModeHomedirTemplate(
    PSTR* ppszUnprovisionedModeHomedirTemplate
    );

DWORD
AD_SetSeparator(
    CHAR pszSeparator
    );

CHAR
AD_GetSeparator(
    VOID
    );

DWORD
AD_SetCacheReaperTimeoutSecs(
    DWORD dwCacheReaperTimeoutSecs
    );

DWORD
AD_GetCacheReaperTimeoutSecs(
    VOID
    );

DWORD
AD_SetMachinePasswordSyncPwdLifetime(
    DWORD dwMachinePasswordSyncPwdLifetime
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
AD_SetCacheEntryExpirySeconds(
    DWORD dwExpirySecs
    );

DWORD
AD_GetCacheEntryExpirySeconds(
    VOID
    );

DWORD
AD_SetLDAPSignAndSeal(
    BOOLEAN bValue
    );

BOOLEAN
AD_GetLDAPSignAndSeal(
    VOID
    );

double
AD_GetMachineTGTGraceSeconds();

DWORD
AD_AddAllowedGroup(
    PCSTR pszNetbiosGroupName
    );

BOOLEAN
AD_ShouldFilterUserLoginsByGroup(
    VOID
    );

BOOLEAN
AD_IsGroupAllowed(
    PCSTR pszNetbiosGroupName
    );

VOID
AD_FreeAllowedGroups_InLock(
    VOID);

BOOLEAN
AD_ShouldAssumeDefaultDomain(
    VOID
    );

VOID
AD_SetAssumeDefaultDomain(
    BOOLEAN bValue
    );

BOOLEAN
AD_ShouldSyncSystemTime(
    VOID
    );

VOID
AD_SetSyncSystemTime(
    BOOLEAN bValue
    );

#endif /* __AD_CFG_H__ */

