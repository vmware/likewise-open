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
 *        externs.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 *        Global Variables
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "adprovider.h"

pthread_rwlock_t gADGlobalDataLock;

PSTR gpszADProviderName = "lsa-activedirectory-provider";

PSTR gpszConfigFilePath = NULL;

/*
 * Kerberos
 */
BOOLEAN       gbCreateK5Login = TRUE;

PSTR gpszUnprovisionedModeShell = NULL;
PSTR gpszUnprovisionedModeHomedirTemplate = NULL;

BOOLEAN gbShutdownProvider = FALSE;

AD_PROVIDER_DATA  gADProviderData;
PAD_PROVIDER_DATA gpADProviderData = &gADProviderData;

/*
 * Output conventions
 */
const CHAR gcSeparatorDefault = '^';
CHAR gcSeparator = '^';

/*
 * Caching
 */
const DWORD gdwCacheReaperTimeoutSecsMinimum = 5 * LSA_SECONDS_IN_MINUTE;
const DWORD gdwCacheReaperTimeoutSecsDefault = 30 * LSA_SECONDS_IN_DAY;
const DWORD gdwCacheReaperTimeoutSecsMaximum = 60 * LSA_SECONDS_IN_DAY;
DWORD gdwCacheReaperTimeoutSecs              = 30 * LSA_SECONDS_IN_DAY;

const DWORD gdwCacheEntryExpirySecsMinimum = 0;
const DWORD gdwCacheEntryExpirySecsDefault = 5 * LSA_SECONDS_IN_MINUTE;
const DWORD gdwCacheEntryExpirySecsMaximum = 60 * LSA_SECONDS_IN_MINUTE;
DWORD gdwCacheEntryExpirySecs              = 5 * LSA_SECONDS_IN_MINUTE;

pthread_t       gCacheReaperThread;
pthread_mutex_t gCacheReaperThreadLock      = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  gCacheReaperThreadCondition = PTHREAD_COND_INITIALIZER;
pthread_t*      gpCacheReaperThread = NULL;

/*
 * Machine Password
 */
const DWORD gdwMachinePasswordSyncPwdLifetimeMinimum = LSA_SECONDS_IN_HOUR;
const DWORD gdwMachinePasswordSyncPwdLifetimeDefault = 30 * LSA_SECONDS_IN_DAY;
const DWORD gdwMachinePasswordSyncPwdLifetimeMaximum = 60 * LSA_SECONDS_IN_DAY;
DWORD gdwMachinePasswordSyncPwdLifetime              = 30 * LSA_SECONDS_IN_DAY;

DWORD gdwMachinePasswordSyncThreadWaitSecs           = 30 * LSA_SECONDS_IN_MINUTE;

pthread_t       gMachinePasswordSyncThread;
pthread_mutex_t gMachinePasswordSyncThreadLock       = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  gMachinePasswordSyncThreadCondition  = PTHREAD_COND_INITIALIZER;
pthread_t*      gpMachinePasswordSyncThread          = NULL;

DWORD gdwClockDriftSecs = 60;

HANDLE ghPasswordStore = (HANDLE)NULL;

BOOLEAN gbLDAPSignAndSeal = FALSE;

DWORD gdwMachineTGTExpiry = 0;

double gdwMachineTGTExpiryGraceSeconds = (60 * 60);

LSA_PROVIDER_FUNCTION_TABLE gADProviderAPITable =
    {
            &AD_OpenHandle,
            &AD_CloseHandle,
            &AD_ServicesDomain,
            &AD_AuthenticateUser,
            &AD_ValidateUser,
            &AD_FindUserByName,
            &AD_FindUserById,
            &AD_BeginEnumUsers,
            &AD_EnumUsers,
            &AD_EndEnumUsers,
            &AD_FindGroupByName,
            &AD_FindGroupById,
            &AD_GetUserGroupMembership,
            &AD_BeginEnumGroups,
            &AD_EnumGroups,
            &AD_EndEnumGroups,
            &AD_ChangePassword,
            &AD_AddUser,
            &AD_ModifyUser,
            &AD_DeleteUser,
            &AD_AddGroup,
            &AD_DeleteGroup,
            &AD_OpenSession,
            &AD_CloseSession,
            &AD_GetNamesBySidList,
            &AD_BeginEnumNSSArtefacts,
            &AD_EnumNSSArtefacts,
            &AD_EndEnumNSSArtefacts,
            &AD_GetStatus,
            &AD_FreeStatus,
            &AD_RefreshConfiguration
    };

CACHE_CONNECTION_HANDLE gpCacheConnection = NULL;

PLSA_HASH_TABLE gpAllowedGroups   = NULL;

BOOLEAN gbAssumeDefaultDomain = FALSE;

BOOLEAN gbSyncSystemTime = TRUE;

// please put all new globals in the following structure:
PLSA_AD_PROVIDER_STATE gpLsaAdProviderState = NULL;

