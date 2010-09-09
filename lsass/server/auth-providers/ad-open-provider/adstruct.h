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
 *        adstruct.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Private header for Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#ifndef __AD_STRUCT_H__
#define __AD_STRUCT_H__

typedef struct __AD_ENUM_STATE {
    DWORD dwInfoLevel;
    BOOLEAN bCheckGroupMembersOnline;
    LSA_FIND_FLAGS FindFlags;
    LSA_NIS_MAP_QUERY_FLAGS dwMapFlags;
    PSTR  pszMapName;

    LW_SEARCH_COOKIE Cookie;
} AD_ENUM_STATE, *PAD_ENUM_STATE;

typedef struct __AD_ENUM_HANDLE
{
    enum
    {
        AD_ENUM_HANDLE_OBJECTS,
        AD_ENUM_HANDLE_MEMBERS
    } Type;
    LSA_FIND_FLAGS FindFlags;
    LSA_OBJECT_TYPE ObjectType;
    LSA_OBJECT_TYPE CurrentObjectType;
    LW_SEARCH_COOKIE Cookie;
    PSTR* ppszSids;
    DWORD dwSidCount;
    DWORD dwSidIndex;
} AD_ENUM_HANDLE, *PAD_ENUM_HANDLE;

typedef struct __AD_PROVIDER_CONTEXT
{
    uid_t uid;
    gid_t gid;
    pid_t pid;
} AD_PROVIDER_CONTEXT, *PAD_PROVIDER_CONTEXT;

typedef struct _AD_LINKED_CELL_INFO
{
    PSTR    pszCellDN;
    PSTR    pszDomain;
    BOOLEAN bIsForestCell;
} AD_LINKED_CELL_INFO, *PAD_LINKED_CELL_INFO;

typedef struct _AD_PROVIDER_DATA
{
	DWORD dwDirectoryMode;
	ADConfigurationMode adConfigurationMode;
	UINT64 adMaxPwdAge;
	CHAR  szDomain[256];
	CHAR  szShortDomain[256];
	CHAR  szComputerDN[256];
    struct {
	  CHAR szCellDN[256];
	} cell;
        // Contains type PAD_LINKED_CELL_INFO
	PDLINKEDLIST pCellList;
} AD_PROVIDER_DATA, *PAD_PROVIDER_DATA;

typedef struct _LSA_AD_CONFIG {

    DWORD               dwCacheReaperTimeoutSecs;
    DWORD               dwCacheEntryExpirySecs;
    DWORD               dwCacheSizeCap;
    BOOLEAN             bEnableEventLog;
    BOOLEAN             bShouldLogNetworkConnectionEvents;
    BOOLEAN             bCreateK5Login;
    BOOLEAN             bCreateHomeDir;
    BOOLEAN             bLDAPSignAndSeal;
    BOOLEAN             bAssumeDefaultDomain;
    BOOLEAN             bSyncSystemTime;
    BOOLEAN             bRefreshUserCreds;
    DWORD               dwMachinePasswordSyncLifetime;
    PSTR                pszUserDomainPrefix;
    PSTR                pszShell;
    PSTR                pszHomedirPrefix;
    PSTR                pszHomedirTemplate;
    DWORD               dwUmask;
    PSTR                pszSkelDirs;
    PDLINKEDLIST        pUnresolvedMemberList;
    AD_CELL_SUPPORT     CellSupport;
    AD_CACHE_BACKEND    CacheBackend;
    BOOLEAN             bTrimUserMembershipEnabled;
    BOOLEAN             bNssGroupMembersCacheOnlyEnabled;
    BOOLEAN             bNssUserMembershipCacheOnlyEnabled;
    BOOLEAN             bNssEnumerationEnabled;
    struct {
        DWORD           dwCheckDomainOnlineSeconds;
        DWORD           dwUnknownDomainCacheTimeoutSeconds;
        BOOLEAN         bIgnoreAllTrusts;
        PSTR*           ppszTrustExceptionList;
        DWORD           dwTrustExceptionCount;
    } DomainManager;
} LSA_AD_CONFIG, *PLSA_AD_CONFIG;

struct _ADSTATE_CONNECTION;
typedef struct _ADSTATE_CONNECTION *ADSTATE_CONNECTION_HANDLE;

typedef struct _LSA_AD_PROVIDER_STATE {
    /// Tracks machine credentials state
    struct {
        BOOLEAN bIsInitialized;
        /// Mutex to protect renewing machine creds
        pthread_mutex_t Mutex;
        /// Pointer to above after it is initialized.  Use this to
        /// determine whe
        pthread_mutex_t* pMutex;
    } MachineCreds;
    MEDIA_SENSE_HANDLE MediaSenseHandle;
    LSA_AD_CONFIG      config;

    LSA_DB_HANDLE hCacheConnection;

    /// Used during transition to join to indicate that trusts
    /// are being discovered.
    /// TODO: Create a LSA_DM_ENGINE_STATE opaque pointer to
    ///       abstract out all access to global state from
    ///       LsaDmEngine code (e.g., primary domain,
    ///       store handle, etc.).  Move the internals
    ///       of this state in lsadmengine.c module.
    ///       Would need a LsaDmEngineState{Create,Destroy}
    ///       which would be called by AD_Activate/AD_Deactivate
    ///       (like LsaDmIntialize/LsaDmCleanup).
    struct {
        BOOLEAN bIsDiscoveringTrusts;
    } TrustDiscovery;

    ADSTATE_CONNECTION_HANDLE hStateConnection;
    DWORD dwMaxAllowedClockDriftSeconds;

    pthread_rwlock_t stateLock;
    enum
    {
        LSA_AD_UNKNOWN,
        LSA_AD_NOT_JOINED,
        LSA_AD_JOINED
    } joinState;
    pthread_rwlock_t* pStateLock;
} LSA_AD_PROVIDER_STATE, *PLSA_AD_PROVIDER_STATE;

#endif /* __AD_STRUCT_H__ */

