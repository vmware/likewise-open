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

#if 0
pthread_rwlock_t gADGlobalDataLock;

PSTR gpszADProviderName = "lsa-activedirectory-provider";

PSTR gpszConfigFilePath = NULL;

BOOLEAN gbShutdownProvider = FALSE;

PAD_PROVIDER_DATA gpADProviderData = NULL;

pthread_t       gCacheReaperThread;
pthread_mutex_t gCacheReaperThreadLock      = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  gCacheReaperThreadCondition = PTHREAD_COND_INITIALIZER;
pthread_t*      gpCacheReaperThread = NULL;

/*
 * Machine Password
 */
DWORD gdwMachinePasswordSyncThreadWaitSecs           = 30 * LSA_SECONDS_IN_MINUTE;

pthread_t       gMachinePasswordSyncThread;
pthread_mutex_t gMachinePasswordSyncThreadLock       = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  gMachinePasswordSyncThreadCondition  = PTHREAD_COND_INITIALIZER;
pthread_t*      gpMachinePasswordSyncThread          = NULL;

DWORD gdwClockDriftSecs = 60;

HANDLE ghPasswordStore = (HANDLE)NULL;

DWORD gdwMachineTGTExpiry = 0;

double gdwMachineTGTExpiryGraceSeconds = (60 * 60);
#endif

LSA_PROVIDER_FUNCTION_TABLE gADProviderAPITable =
    {
            &ActiveDirectoryOpenProvider,
            &ActiveDirectoryCloseProvider,
            &ActiveDirectoryServicesDomain,
            &ActiveDirectoryAuthenticateUser,
            &ActiveDirectoryAuthenticateUserEx,
            &ActiveDirectoryValidateUser,
            &ActiveDirectoryCheckUserInList,
            &ActiveDirectoryFindUserByName,
            &ActiveDirectoryFindUserById,
            &ActiveDirectoryBeginEnumUsers,
            &ActiveDirectoryEnumUsers,
            &ActiveDirectoryEndEnumUsers,
            &ActiveDirectoryFindGroupByName,
            &ActiveDirectoryFindGroupById,
            &ActiveDirectoryGetGroupsForUser,
            &ActiveDirectoryBeginEnumGroups,
            &ActiveDirectoryEnumGroups,
            &ActiveDirectoryEndEnumGroups,
            &ActiveDirectoryChangePassword,
            &ActiveDirectoryAddUser,
            &ActiveDirectoryModifyUser,
            &ActiveDirectoryDeleteUser,
            &ActiveDirectoryAddGroup,
            &ActiveDirectoryDeleteGroup,
            &ActiveDirectoryOpenSession,
            &ActiveDirectoryCloseSession,
            &ActiveDirectoryGetNamesBySidList,
            &ActiveDirectoryFindNSSArtefactByKey,
            &ActiveDirectoryBeginEnumNSSArtefacts,
            &ActiveDirectoryEnumNSSArtefacts,
            &ActiveDirectoryEndEnumNSSArtefacts,
            &ActiveDirectoryGetStatus,
            &ActiveDirectoryFreeStatus,
            &ActiveDirectoryRefreshConfiguration,
            &ActiveDirectoryProviderIoControl
    };

#if 0
PLSA_HASH_TABLE gpAllowedSIDs   = NULL;

// please put all new globals in the following structure:
PLSA_AD_PROVIDER_STATE gpLsaAdProviderState = NULL;
#endif

