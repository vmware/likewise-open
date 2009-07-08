/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        provider-main.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Authentication Provider
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */

#include "adprovider.h"

static
DWORD
AD_GetNameWithReplacedSeparators(
    IN PCSTR pszName,
    OUT PSTR* ppszFreeName,
    OUT PCSTR* ppszUseName
    );

static
DWORD
AD_RemoveUserByNameFromCacheInternal(
    IN HANDLE  hProvider,
    IN PCSTR   pszLoginId
    );

static
DWORD
LsaAdProviderStateCreate(
    OUT PLSA_AD_PROVIDER_STATE* ppState
    );

static
VOID
LsaAdProviderStateDestroy(
    IN OUT PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_MachineCredentialsCacheClear(
    VOID
    );

static
BOOLEAN
AD_MachineCredentialsCacheIsInitialized(
    VOID
    );

static
BOOLEAN
LsaAdProviderLwKrb5IsOfflineCallback(
    IN PCSTR pszRealm
    );

static
VOID
LsaAdProviderLwKrb5TransitionOfflineCallback(
    IN PCSTR pszRealm
    );

static
VOID
LsaAdProviderMediaSenseTransitionCallback(
    IN PVOID Context,
    IN BOOLEAN bIsOffline
    );

static
DWORD
AD_ResolveConfiguredLists(
    HANDLE          hProvider,
    PLSA_HASH_TABLE *ppAllowedMemberList
    );

static
VOID
LsaAdProviderLogServiceStartEvent(
    PCSTR   pszHostname,
    PCSTR   pszDomainDnsName,
    BOOLEAN bIsDomainOffline,
    DWORD   dwErrCode
    );

static
VOID
LsaAdProviderLogConfigReloadEvent(
    VOID
    );

static
VOID
LsaAdProviderLogRequireMembershipOfChangeEvent(
    HANDLE hProvider
    );

static
VOID
LsaAdProviderLogEventLogEnableChangeEvent(
    VOID
    );

static
DWORD
AD_SetUserCanonicalNameToAlias(
    PCSTR pszCurrentNetBIOSDomainName,
    DWORD dwUserInfoLevel,
    PVOID pUserInfo
    );

static
DWORD
AD_SetGroupCanonicalNamesToAliases(
    PCSTR pszCurrentNetBIOSDomainName,
    DWORD dwGroupInfoLevel,
    PVOID pGroupInfo
    );

static
DWORD
AD_SetCanonicalNameToAlias(
    PCSTR pszCurrentNetBIOSDomainName,
    PSTR  pszCanonicalName
    );


void
InitADCacheFunctionTable(
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheProviderTable
    );

DWORD
LsaInitializeProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable
    )
{
    DWORD dwError = 0;
    PSTR  pszHostname = NULL;
    PSTR  pszUsername = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszDomainDnsName = NULL;
    PSTR  pszHostDnsDomain = NULL;
    PSTR pszKrb5CcPath = NULL;
    BOOLEAN bIsDomainOffline = FALSE;
    LSA_AD_CONFIG config = {0};

    pthread_rwlock_init(&gADGlobalDataLock, NULL);

    InitADCacheFunctionTable(gpCacheProvider);

    dwError = LsaAdProviderStateCreate(&gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwKrb5Init(
                LsaAdProviderLwKrb5IsOfflineCallback,
                LsaAdProviderLwKrb5TransitionOfflineCallback);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_NetInitMemory();
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszConfigFilePath)) {

        dwError = AD_InitializeConfig(&config);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_ParseConfigFile(
                        pszConfigFilePath,
                        &config);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_TransferConfigContents(
                        &config,
                        &gpLsaAdProviderState->config);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaSetDomainSeparator(
                    gpLsaAdProviderState->config.chDomainSeparator);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_SetConfigFilePath(pszConfigFilePath);
        BAIL_ON_LSA_ERROR(dwError);

        LsaAdProviderLogConfigReloadEvent();
    }

    dwError = ADInitMachinePasswordSync();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszHostname);

    dwError = LwKrb5GetMachineCreds(
                    pszHostname,
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    // Initialize domain manager before doing any network stuff.
    dwError = LsaDmInitialize(
                    TRUE,
                    AD_GetDomainManagerCheckDomainOnlineSeconds(),
                    AD_GetDomainManagerUnknownDomainCacheTimeoutSeconds());
    BAIL_ON_LSA_ERROR(dwError);

    // Start media sense after starting up domain manager.
    dwError = MediaSenseStart(&gpLsaAdProviderState->MediaSenseHandle,
                              LsaAdProviderMediaSenseTransitionCallback,
                              NULL);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmWrapLdapPingTcp(pszDomainDnsName);
    if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        bIsDomainOffline = TRUE;
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsDomainOffline)
    {
        dwError = AD_MachineCredentialsCacheInitialize();
        if (dwError == LSA_ERROR_CLOCK_SKEW)
        {
            bIsDomainOffline = TRUE;
            dwError = LSA_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (bIsDomainOffline)
    {
        dwError = AD_MachineCredentialsCacheClear();
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwKrb5GetSystemCachePath(KRB5_File_Cache, &pszKrb5CcPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwKrb5SetProcessDefaultCachePath(pszKrb5CcPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_OpenDb(
                &gpLsaAdProviderState->hStateConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheOpen(
                LSASS_AD_CACHE,
                &gpLsaAdProviderState->hCacheConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_InitializeOperatingMode(
                pszDomainDnsName,
                pszUsername,
                bIsDomainOffline);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUmInitialize();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADInitCacheReaper();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADStartMachinePasswordSync();
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_EventlogEnabled())
    {
        LsaAdProviderLogServiceStartEvent(
                           pszHostname,
                           pszDomainDnsName,
                           bIsDomainOffline,
                           dwError);
    }

    dwError = ADUnprovPlugin_Initialize();
    BAIL_ON_LSA_ERROR(dwError);

    *ppFunctionTable = &gADProviderAPITable;
    *ppszProviderName = gpszADProviderName;

cleanup:

    AD_FreeConfigContents(&config);

    LSA_SAFE_FREE_STRING(pszHostname);
    LSA_SAFE_FREE_STRING(pszUsername);
    LSA_SAFE_CLEAR_FREE_STRING(pszPassword);
    LSA_SAFE_FREE_STRING(pszDomainDnsName);
    LSA_SAFE_FREE_STRING(pszHostDnsDomain);
    LSA_SAFE_FREE_STRING(pszKrb5CcPath);

    return dwError;

error:
    // ISSUE-2008/09/16-dalmeida -- We need to clean up everything, especially
    // any background threads!  We currently take care of cleaning up the
    // background threads, but we need to make the init/cleanup code
    // cleaner and more modular.  We cannot yet call LsaShutdownProvider()
    // because the latter does not necessarily handle partially initialized
    // state.  So for now, we just clean up what we can here.
    // I have some notes elsewhere on how to structure init/cleanup code
    // to make this cleaner.

    // ISSUE-2008/09/19-dalmeida -- The cache reaper and machine password
    // thread shutdown code should not pthread_cancel but use a proper
    // self-contained shutdown mechanism.  We need to make those modules
    // more self-contained wrt having their own shutdown logic.

    if (AD_EventlogEnabled())
    {
        LsaAdProviderLogServiceStartEvent(
                           pszHostname,
                           pszDomainDnsName,
                           bIsDomainOffline,
                           dwError);
    }

    ADShutdownCacheReaper();
    ADShutdownMachinePasswordSync();

    // This will clean up media sense too.
    LsaAdProviderStateDestroy(gpLsaAdProviderState);
    gpLsaAdProviderState = NULL;

    LsaDmCleanup();

    if (gpADProviderData)
    {
        ADProviderFreeProviderData(gpADProviderData);
        gpADProviderData = NULL;
    }

    *ppszProviderName = NULL;
    *ppFunctionTable = NULL;

    goto cleanup;
}

DWORD
LsaShutdownProvider(
    PSTR pszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    ADProviderSetShutdownFlag(TRUE);

    ADUnprovPlugin_Cleanup();

    ADShutdownCacheReaper();
    ADShutdownMachinePasswordSync();

    dwError = AD_NetShutdownMemory();
    if (dwError)
    {
        LSA_LOG_DEBUG("AD Provider Shutdown: Failed to shutdown net memory (error = %d)", dwError);
        dwError = 0;
    }

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (gpADProviderData)
    {
        ADProviderFreeProviderData(gpADProviderData);
        gpADProviderData = NULL;
    }

    dwError = LwKrb5Shutdown();
    if (dwError)
    {
        LSA_LOG_DEBUG("AD Provider Shutdown: Failed to shutdown krb5 (error = %d)", dwError);
        dwError = 0;
    }

    AD_FreeAllowedSIDs_InLock();

    LSA_SAFE_FREE_STRING(gpszConfigFilePath);

    // This will clean up media sense too.
    LsaAdProviderStateDestroy(gpLsaAdProviderState);
    gpLsaAdProviderState = NULL;

    LsaUmCleanup();

    // Clean up the domain manager last since it does not depend on any
    // other facilities provided by the AD provider.
    LsaDmCleanup();

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;
}

DWORD
AD_OpenHandle(
    uid_t   uid,
    gid_t   gid,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(AD_PROVIDER_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    pContext->uid = uid;
    pContext->gid = gid;

    *phProvider = (HANDLE)pContext;

cleanup:

    return dwError;

error:

    *phProvider = (HANDLE)NULL;

    if (pContext) {
        AD_CloseHandle((HANDLE)pContext);
    }

    goto cleanup;
}

void
AD_CloseHandle(
    HANDLE hProvider
    )
{
    PAD_PROVIDER_CONTEXT pContext = (PAD_PROVIDER_CONTEXT)hProvider;
    if (pContext)
    {
        LsaFreeMemory(pContext);
    }
}

BOOLEAN
AD_ServicesDomain(
    PCSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    //
    // Added Trusted domains support
    //
    if (IsNullOrEmptyString(pszDomain) ||
        IsNullOrEmptyString(gpADProviderData->szDomain) ||
        IsNullOrEmptyString(gpADProviderData->szShortDomain)) {
       goto cleanup;
    }

    bResult = LsaDmIsDomainPresent(pszDomain);
    if (!bResult)
    {
        LSA_LOG_INFO("AD_ServicesDomain was passed unknown domain '%s'", pszDomain);
    }

cleanup:
    return bResult;
}

DWORD
AD_AuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;

    if (AD_IsOffline())
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineAuthenticateUser(
            hProvider,
            pszLoginId,
            pszPassword);
    }

    if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineAuthenticateUser(
            hProvider,
            pszLoginId,
            pszPassword);
    }

    return dwError;
}

DWORD
AD_AuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_INTERNAL;

    dwError = LsaDmWrapAuthenticateUserEx(
                      gpADProviderData->szDomain,
                      pUserParams,
                      ppUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:
    /* On this one, it is a good idea to fallback to
       the local provider */

    if (dwError == LSA_ERROR_RPC_NETLOGON_FAILED) {
        dwError = LSA_ERROR_NOT_HANDLED;
    }

    goto cleanup;
}

DWORD
AD_ValidateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    gpADProviderData->szDomain,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!AD_ServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindUserObjectByName(
                hProvider,
                pszLoginId,
                &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_VerifyUserAccountCanLogin(
                pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    ADCacheSafeFreeObject(&pUserInfo);

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CheckUserInList(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PCSTR  pszListName
    )
{
    DWORD  dwError = 0;
    size_t  sNumGroupsFound = 0;
    PLSA_SECURITY_OBJECT* ppGroupList = NULL;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    size_t  iGroup = 0;
    PLSA_HASH_TABLE pAllowedMemberList = NULL;

    dwError = AD_ResolveConfiguredLists(
                  hProvider,
                  &pAllowedMemberList);
    BAIL_ON_LSA_ERROR(dwError);

    if (!AD_ShouldFilterUserLoginsByGroup())
    {
        goto cleanup;
    }

    dwError = AD_FindUserObjectByName(hProvider, pszUserName, &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_IsMemberAllowed(pUserInfo->pszObjectSid,
                           pAllowedMemberList))
    {
        goto cleanup;
    }

    dwError = AD_GetUserGroupObjectMembership(
                    hProvider,
                    pUserInfo,
                    FALSE,
                    &sNumGroupsFound,
                    &ppGroupList);
    BAIL_ON_LSA_ERROR(dwError);

    for (; iGroup < sNumGroupsFound; iGroup++)
    {
        if (AD_IsMemberAllowed(ppGroupList[iGroup]->pszObjectSid,
                               pAllowedMemberList))
        {
            goto cleanup;
        }
    }

    dwError = EACCES;
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    ADCacheSafeFreeObjectList(sNumGroupsFound, &ppGroupList);
    ADCacheSafeFreeObject(&pUserInfo);
    LsaHashSafeFree(&pAllowedMemberList);

    return dwError;

error:

    if (dwError == EACCES)
    {
        LSA_LOG_ERROR("Error: User [%s] not in restricted login list", pszUserName);
    }
    else
    {
        LSA_LOG_ERROR("Error: Failed to validate restricted membership. [Error code: %u]", dwError);
    }

    goto cleanup;
}

DWORD
AD_FindUserByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD   dwError = 0;
    PVOID   pUserInfo = NULL;
    PLSA_SECURITY_OBJECT pInObjectForm = NULL;

    dwError = AD_FindUserObjectByName(
                    hProvider,
                    pszLoginId,
                    &pInObjectForm);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADMarshalFromUserCache(
                    pInObjectForm,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_ShouldAssumeDefaultDomain())
    {
        dwError = AD_SetUserCanonicalNameToAlias(
                        gpADProviderData->szShortDomain,
                        dwUserInfoLevel,
                        pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppUserInfo = pUserInfo;

cleanup:

    ADCacheSafeFreeObject(&pInObjectForm);

    return dwError;

error:

    *ppUserInfo = NULL;

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
AD_FindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD   dwError = 0;
    PVOID   pUserInfo = NULL;
    PLSA_SECURITY_OBJECT pInObjectForm = NULL;

    dwError = AD_FindUserObjectById(
                hProvider,
                uid,
                &pInObjectForm);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADMarshalFromUserCache(
            pInObjectForm,
            dwUserInfoLevel,
            &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_ShouldAssumeDefaultDomain())
    {
        dwError = AD_SetUserCanonicalNameToAlias(
                        gpADProviderData->szShortDomain,
                        dwUserInfoLevel,
                        pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppUserInfo = pUserInfo;

cleanup:

    ADCacheSafeFreeObject(&pInObjectForm);

    return dwError;

error:

    if ((dwError == LSA_ERROR_DUPLICATE_USERNAME ||
         dwError == LSA_ERROR_DUPLICATE_USER_OR_GROUP)
        && AD_EventlogEnabled())
    {
        LsaSrvLogUserIDConflictEvent(
                      uid,
                      gpszADProviderName,
                      dwError);
    }

    *ppUserInfo = NULL;

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    goto cleanup;
}

DWORD
AD_FindUserObjectById(
    IN HANDLE  hProvider,
    IN uid_t   uid,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;

    if (AD_IsOffline())
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineFindUserObjectById(
                    hProvider,
                    uid,
                    ppResult);
    }

    if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineFindUserObjectById(
                    hProvider,
                    uid,
                    ppResult);
    }

    return dwError;
}

DWORD
AD_BeginEnumUsers(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = NULL;

    dwError = AD_CreateUserState(
                        hProvider,
                        dwInfoLevel,
                        FindFlags,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    LwInitCookie(&pEnumState->Cookie);

    *phResume = (HANDLE)pEnumState;

cleanup:

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    goto cleanup;
}

DWORD
AD_EnumUsers(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    DWORD dwNumUsersFound = 0;
    PVOID* ppUserInfoList = NULL;

    if (AD_IsOffline())
    {
        dwError = AD_OfflineEnumUsers(
                    hProvider,
                    hResume,
                    dwMaxNumUsers,
                    &dwNumUsersFound,
                    &ppUserInfoList);
    }
    else
    {
        dwError = AD_OnlineEnumUsers(
                    hProvider,
                    hResume,
                    dwMaxNumUsers,
                    &dwNumUsersFound,
                    &ppUserInfoList);
    }

    BAIL_ON_LSA_ERROR(dwError);

    if (AD_ShouldAssumeDefaultDomain())
    {
        DWORD iUser = 0;
        PAD_ENUM_STATE pEnumState = (PAD_ENUM_STATE)hResume;

        for (iUser = 0; iUser < dwNumUsersFound; iUser++)
        {
            PVOID pUserInfo = *(ppUserInfoList + iUser);

            dwError = AD_SetUserCanonicalNameToAlias(
                            gpADProviderData->szShortDomain,
                            pEnumState->dwInfoLevel,
                            pUserInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pppUserInfoList = ppUserInfoList;
    *pdwUsersFound = dwNumUsersFound;

cleanup:

    return dwError;

error:

    *pppUserInfoList = NULL;
    *pdwUsersFound = 0;

    if (ppUserInfoList)
    {
        PAD_ENUM_STATE pEnumState = (PAD_ENUM_STATE)hResume;

        LsaFreeUserInfoList(pEnumState->dwInfoLevel, ppUserInfoList, dwNumUsersFound);
    }

    goto cleanup;
}

VOID
AD_EndEnumUsers(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    AD_FreeUserState(hProvider, (PAD_ENUM_STATE) hResume);
}

DWORD
AD_EnumUsersFromCache(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD                 dwError = 0;
    DWORD                 dwObjectCount = 0;
    DWORD                 dwInfoCount = 0;
    PLSA_SECURITY_OBJECT* ppUserObjectList = NULL;
    PVOID*                ppUserInfoList = NULL;
    PVOID                 pBlob = NULL;
    size_t                BlobSize = 0;
    LWMsgContext*         context = NULL;
    LWMsgDataContext*      pDataContext = NULL;
    PLSA_AD_IPC_ENUM_USERS_FROM_CACHE_REQ request = NULL;
    LSA_AD_IPC_ENUM_USERS_FROM_CACHE_RESP response;
    LSA_USER_INFO_LIST    result;

    memset(&response, 0, sizeof(response));
    memset(&result, 0, sizeof(result));

    // restrict access to root
    if (peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaAdIPCGetEnumUsersFromCacheReqSpec(),
                              pInputBuffer,
                              dwInputBufferSize,
                              (PVOID*)&request));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheEnumUsersCache(
                  gpLsaAdProviderState->hCacheConnection,
                  request->dwMaxNumUsers,
                  request->pszResume,
                  &dwObjectCount,
                  &ppUserObjectList);
    if ( dwError == LSA_ERROR_NOT_HANDLED )
    {
        // no more results found
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if ( dwObjectCount == request->dwMaxNumUsers )
    {
        dwError = LsaAllocateString(
                      ppUserObjectList[dwObjectCount - 1]->pszSamAccountName,
                      &response.pszResume);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ( dwObjectCount )
    {
        dwError = LsaAllocateMemory(sizeof(*ppUserInfoList) * dwObjectCount,
                                    (PVOID*)&ppUserInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        // marshal the UserInfoList data
        for (dwInfoCount = 0; dwInfoCount < dwObjectCount; dwInfoCount++)
        {
            dwError = ADMarshalFromUserCache(
                          ppUserObjectList[dwInfoCount],
                          request->dwInfoLevel,
                          &ppUserInfoList[dwInfoCount]);
            BAIL_ON_LSA_ERROR(dwError);

            ADCacheSafeFreeObject(&ppUserObjectList[dwInfoCount]);
        }

        if (AD_ShouldAssumeDefaultDomain())
        {
            DWORD iUser = 0;

            for (iUser = 0; iUser < dwInfoCount; iUser++)
            {
                PVOID pUserInfo = *(ppUserInfoList + iUser);

                dwError = AD_SetUserCanonicalNameToAlias(
                              gpADProviderData->szShortDomain,
                              request->dwInfoLevel,
                              pUserInfo);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    // marshal the response data
    result.dwUserInfoLevel = request->dwInfoLevel;
    result.dwNumUsers = dwInfoCount;

    if ( dwInfoCount )
    {
        switch (result.dwUserInfoLevel)
        {
            case 0:
                result.ppUserInfoList.ppInfoList0 = (PLSA_USER_INFO_0*)ppUserInfoList;
                break;

            case 1:
                result.ppUserInfoList.ppInfoList1 = (PLSA_USER_INFO_1*)ppUserInfoList;
                break;

            case 2:
                result.ppUserInfoList.ppInfoList2 = (PLSA_USER_INFO_2*)ppUserInfoList;
                break;

            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }
    }
    response.pUserInfoList = &result;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              LsaAdIPCGetEnumUsersFromCacheRespSpec(),
                              &response,
                              &pBlob,
                              &BlobSize));
    BAIL_ON_LSA_ERROR(dwError);

    *pdwOutputBufferSize = BlobSize;
    *ppOutputBuffer = pBlob;

cleanup:

    ADCacheSafeFreeObjectList(dwObjectCount, &ppUserObjectList);

    if (ppUserInfoList)
    {
        LsaFreeUserInfoList(
            request->dwInfoLevel,
            (PVOID*)ppUserInfoList,
            dwInfoCount);
        ppUserInfoList = NULL;
        dwInfoCount = 0;
    }

    if ( request )
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetEnumUsersFromCacheReqSpec(),
            request);
    }

    if ( context )
    {
        lwmsg_context_delete(context);
    }

    LSA_SAFE_FREE_STRING(response.pszResume);

    return dwError;

error:

    *pdwOutputBufferSize = 0;
    *ppOutputBuffer = NULL;

    if ( pBlob )
    {
        LsaFreeMemory(pBlob);
    }

    goto cleanup;
}

DWORD
AD_RemoveUserByNameFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN PCSTR  pszLoginId
    )
{
    DWORD                dwError = 0;
    PSTR                 pszLocalLoginId = NULL;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;

    // restrict access to root
    if (peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pszLoginId, "root"))
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_RemoveUserByNameFromCacheInternal(
                  hProvider,
                  pszLoginId);
    if (dwError == LSA_ERROR_NO_SUCH_USER &&
        AD_ShouldAssumeDefaultDomain())
    {
        dwError = LsaCrackDomainQualifiedName(
                      pszLoginId,
                      gpADProviderData->szDomain,
                      &pUserNameInfo);
        BAIL_ON_LSA_ERROR(dwError);

        if (pUserNameInfo->nameType == NameType_Alias)
        {
            dwError = ADGetDomainQualifiedString(
                          gpADProviderData->szShortDomain,
                          pszLoginId,
                          &pszLocalLoginId);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = AD_RemoveUserByNameFromCacheInternal(
                          hProvider,
                          pszLocalLoginId);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_NO_SUCH_USER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszLocalLoginId);
    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_RemoveUserByIdFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN uid_t  uid
    )
{
    DWORD                dwError = 0;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;

    // restrict access to root
    if (peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_OfflineFindUserObjectById(
                  hProvider,
                  uid,
                  &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheRemoveUserBySid(
                  gpLsaAdProviderState->hCacheConnection,
                  pUserInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    ADCacheSafeFreeObject(&pUserInfo);

    return dwError;

error:

    if ((dwError == LSA_ERROR_DUPLICATE_USERNAME ||
         dwError == LSA_ERROR_DUPLICATE_USER_OR_GROUP)
        && AD_EventlogEnabled())
    {
        LsaSrvLogUserIDConflictEvent(
            uid,
            gpszADProviderName,
            dwError);
    }

    goto cleanup;
}

DWORD
AD_FindGroupByName(
    IN HANDLE hProvider,
    IN PCSTR pszGroupName,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    BOOLEAN bIsCacheOnlyMode = FALSE;

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsCacheOnlyMode = AD_GetNssGroupMembersCacheOnlyEnabled();
    }

    return AD_FindGroupByNameWithCacheMode(
                hProvider,
                pszGroupName,
                bIsCacheOnlyMode,
                dwGroupInfoLevel,
                ppGroupInfo);
}

DWORD
AD_FindGroupByNameWithCacheMode(
    IN HANDLE hProvider,
    IN PCSTR pszGroupName,
    IN BOOLEAN bIsCacheOnlyMode,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD   dwError = 0;
    PVOID   pGroupInfo = NULL;
    PLSA_SECURITY_OBJECT pInObjectForm = NULL;

    BAIL_ON_INVALID_STRING(pszGroupName);

    dwError = AD_FindGroupObjectByName(
                hProvider,
                pszGroupName,
                &pInObjectForm);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GroupObjectToGroupInfo(
                hProvider,
                pInObjectForm,
                bIsCacheOnlyMode,
                dwGroupInfoLevel,
                &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_ShouldAssumeDefaultDomain())
    {
        dwError = AD_SetGroupCanonicalNamesToAliases(
                        gpADProviderData->szShortDomain,
                        dwGroupInfoLevel,
                        pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppGroupInfo = pGroupInfo;

cleanup:

    ADCacheSafeFreeObject(&pInObjectForm);

    return dwError;

error:

    *ppGroupInfo = NULL;

    if (pGroupInfo)
    {
        LsaFreeUserInfo(dwGroupInfoLevel, pGroupInfo);
    }

    goto cleanup;
}

static
DWORD
AD_GetExpandedGroupUsersEx(
    IN HANDLE hProvider,
    IN PCSTR pszDomainName,
    IN BOOLEAN bIsOffline,
    IN PCSTR pszGroupSid,
    IN BOOLEAN bIsCacheOnlyMode,
    IN DWORD dwMaxDepth,
    OUT PBOOLEAN pbIsFullyExpanded,
    OUT size_t* psMemberUsersCount,
    OUT PLSA_SECURITY_OBJECT** pppMemberUsers
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    BOOLEAN bIsFullyExpanded = FALSE;
    PLSA_AD_GROUP_EXPANSION_DATA pExpansionData = NULL;
    PLSA_SECURITY_OBJECT* ppGroupMembers = NULL;
    size_t sGroupMembersCount = 0;
    PLSA_SECURITY_OBJECT pGroupToExpand = NULL;
    DWORD dwGroupToExpandDepth = 0;
    PCSTR pszGroupToExpandSid = NULL;
    PLSA_SECURITY_OBJECT* ppExpandedUsers = NULL;
    size_t sExpandedUsersCount = 0;

    dwError = AD_GroupExpansionDataCreate(
                &pExpansionData,
                LSA_MAX(1, dwMaxDepth));
    BAIL_ON_LSA_ERROR(dwError);

    pszGroupToExpandSid = pszGroupSid;
    dwGroupToExpandDepth = 1;

    while (pszGroupToExpandSid)
    {
        if (bIsOffline)
        {
            dwError = AD_OfflineGetGroupMembers(
                        pszGroupToExpandSid,
                        &sGroupMembersCount,
                        &ppGroupMembers);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            // ISSUE-2008/11/03-dalmeida -- Need to get domain from SID.
            dwError = AD_OnlineGetGroupMembers(
                        hProvider,
                        pszDomainName,
                        pszGroupToExpandSid,
                        bIsCacheOnlyMode,
                        &sGroupMembersCount,
                        &ppGroupMembers);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = AD_GroupExpansionDataAddExpansionResults(
                    pExpansionData,
                    dwGroupToExpandDepth,
                    &sGroupMembersCount,
                    &ppGroupMembers);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_GroupExpansionDataGetNextGroupToExpand(
                    pExpansionData,
                    &pGroupToExpand,
                    &dwGroupToExpandDepth);
        BAIL_ON_LSA_ERROR(dwError);

        if (pGroupToExpand)
        {
            pszGroupToExpandSid = pGroupToExpand->pszObjectSid;
        }
        else
        {
            pszGroupToExpandSid = NULL;
        }
    }

    dwError = AD_GroupExpansionDataGetResults(pExpansionData,
                                              &bIsFullyExpanded,
                                              &sExpandedUsersCount,
                                              &ppExpandedUsers);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    AD_GroupExpansionDataDestroy(pExpansionData);
    ADCacheSafeFreeObjectList(sGroupMembersCount, &ppGroupMembers);

    if (pbIsFullyExpanded)
    {
        *pbIsFullyExpanded = bIsFullyExpanded;
    }

    *psMemberUsersCount = sExpandedUsersCount;
    *pppMemberUsers = ppExpandedUsers;

    return dwError;

error:
    ADCacheSafeFreeObjectList(sExpandedUsersCount, &ppExpandedUsers);
    sExpandedUsersCount = 0;
    bIsFullyExpanded = FALSE;
    goto cleanup;
}

DWORD
AD_GetExpandedGroupUsers(
    IN HANDLE hProvider,
    IN PCSTR pszDomainName,
    IN PCSTR pszGroupSid,
    IN BOOLEAN bIsCacheOnlyMode,
    IN int iMaxDepth,
    OUT BOOLEAN* pbAllExpanded,
    OUT size_t* psCount,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    if (AD_IsOffline())
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_GetExpandedGroupUsersEx(
            hProvider,
            pszDomainName,
            FALSE,
            pszGroupSid,
            bIsCacheOnlyMode,
            iMaxDepth,
            pbAllExpanded,
            psCount,
            pppResults);
    }

    if (dwError == LSA_ERROR_DOMAIN_IS_OFFLINE)
    {
        dwError = AD_GetExpandedGroupUsersEx(
            hProvider,
            pszDomainName,
            TRUE,
            pszGroupSid,
            bIsCacheOnlyMode,
            iMaxDepth,
            pbAllExpanded,
            psCount,
            pppResults);
    }

    return dwError;
}

DWORD
AD_FindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    BOOLEAN bIsCacheOnlyMode = FALSE;

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsCacheOnlyMode = AD_GetNssGroupMembersCacheOnlyEnabled();
    }

    if (AD_IsOffline())
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineFindGroupById(
            hProvider,
            gid,
            bIsCacheOnlyMode,
            dwGroupInfoLevel,
            &pGroupInfo);
    }

    if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineFindGroupById(
            hProvider,
            gid,
            bIsCacheOnlyMode,
            dwGroupInfoLevel,
            &pGroupInfo);
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_ShouldAssumeDefaultDomain())
    {
        dwError = AD_SetGroupCanonicalNamesToAliases(
                        gpADProviderData->szShortDomain,
                        dwGroupInfoLevel,
                        pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppGroupInfo = pGroupInfo;

cleanup:

    return dwError;

error:

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    *ppGroupInfo = NULL;

    goto cleanup;
}

DWORD
AD_EnumGroupsFromCache(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN gid_t   peerGID,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD                 dwError = 0;
    DWORD                 dwObjectCount = 0;
    DWORD                 dwInfoCount = 0;
    PLSA_SECURITY_OBJECT* ppGroupObjectList = NULL;
    PVOID*                ppGroupInfoList = NULL;
    PVOID                 pBlob;
    size_t                BlobSize;
    LWMsgContext*         context = NULL;
    LWMsgDataContext*      pDataContext = NULL;
    PLSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ request = NULL;
    LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP response;
    LSA_GROUP_INFO_LIST   result;

    memset(&response, 0, sizeof(response));
    memset(&result, 0, sizeof(result));

    // restrict access to root
    if (peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_context_new(NULL, &context));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(context, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                              pDataContext,
                              LsaAdIPCGetEnumGroupsFromCacheReqSpec(),
                              pInputBuffer,
                              dwInputBufferSize,
                              (PVOID*)&request));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheEnumGroupsCache(
                  gpLsaAdProviderState->hCacheConnection,
                  request->dwMaxNumGroups,
                  request->pszResume,
                  &dwObjectCount,
                  &ppGroupObjectList);
    if ( dwError == LSA_ERROR_NOT_HANDLED )
    {
        // no more results found
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if ( dwObjectCount == request->dwMaxNumGroups )
    {
        dwError = LsaAllocateString(
                      ppGroupObjectList[dwObjectCount - 1]->pszSamAccountName,
                      &response.pszResume);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ( dwObjectCount )
    {
        dwError = LsaAllocateMemory(sizeof(*ppGroupInfoList) * dwObjectCount,
                                    (PVOID*)&ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        // marshal the GroupInfoList data
        for (dwInfoCount = 0; dwInfoCount < dwObjectCount; dwInfoCount++)
        {
            dwError = AD_GroupObjectToGroupInfo(
                          hProvider,
                          ppGroupObjectList[dwInfoCount],
                          TRUE,
                          request->dwInfoLevel,
                          &ppGroupInfoList[dwInfoCount]);
            BAIL_ON_LSA_ERROR(dwError);

            ADCacheSafeFreeObject(&ppGroupObjectList[dwInfoCount]);
        }

        if (AD_ShouldAssumeDefaultDomain())
        {
            DWORD iGroup = 0;

            for (iGroup = 0; iGroup < dwInfoCount; iGroup++)
            {
                PVOID pGroupInfo = *(ppGroupInfoList + iGroup);

                dwError = AD_SetGroupCanonicalNamesToAliases(
                              gpADProviderData->szShortDomain,
                              request->dwInfoLevel,
                              pGroupInfo);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    // marshal the response data
    result.dwGroupInfoLevel = request->dwInfoLevel;
    result.dwNumGroups = dwInfoCount;

    if ( dwInfoCount )
    {
        switch (result.dwGroupInfoLevel)
        {
            case 0:
                result.ppGroupInfoList.ppInfoList0 = (PLSA_GROUP_INFO_0*)ppGroupInfoList;
                break;

            case 1:
                result.ppGroupInfoList.ppInfoList1 = (PLSA_GROUP_INFO_1*)ppGroupInfoList;
                break;

            default:
                dwError = LSA_ERROR_INVALID_PARAMETER;
                BAIL_ON_LSA_ERROR(dwError);
        }
    }
    response.pGroupInfoList = &result;

    dwError = MAP_LWMSG_ERROR(lwmsg_data_marshal_flat_alloc(
                              pDataContext,
                              LsaAdIPCGetEnumGroupsFromCacheRespSpec(),
                              &response,
                              &pBlob,
                              &BlobSize));
    BAIL_ON_LSA_ERROR(dwError);

    *pdwOutputBufferSize = BlobSize;
    *ppOutputBuffer = pBlob;

cleanup:

    ADCacheSafeFreeObjectList(dwObjectCount, &ppGroupObjectList);

    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(
            request->dwInfoLevel,
            (PVOID*)ppGroupInfoList,
            dwInfoCount);
        ppGroupInfoList = NULL;
        dwInfoCount = 0;
    }

    if ( request )
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetEnumGroupsFromCacheReqSpec(),
            request);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if ( context )
    {
        lwmsg_context_delete(context);
    }

    LSA_SAFE_FREE_STRING(response.pszResume);

    return dwError;

error:

    *pdwOutputBufferSize = 0;
    *ppOutputBuffer = NULL;

    if ( pBlob )
    {
        LsaFreeMemory(pBlob);
    }

    goto cleanup;
}

DWORD
AD_RemoveGroupByNameFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN PCSTR  pszGroupName
    )
{
    DWORD                dwError = 0;
    PSTR                 pszFreeGroupName = NULL;
    PCSTR                pszUseGroupName = NULL;
    PLSA_SECURITY_OBJECT pGroupInfo = NULL;

    // restrict access to root
    if (peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_GetNameWithReplacedSeparators(
                  pszGroupName,
                  &pszFreeGroupName,
                  &pszUseGroupName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OfflineFindGroupObjectByName(
                  hProvider,
                  pszUseGroupName,
                  &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheRemoveGroupBySid(
                  gpLsaAdProviderState->hCacheConnection,
                  pGroupInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszFreeGroupName);
    ADCacheSafeFreeObject(&pGroupInfo);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_RemoveGroupByIdFromCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID,
    IN gid_t  gid
    )
{
    DWORD             dwError = 0;
    DWORD             dwGroupInfoLevel = 0;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    BOOLEAN           bIsCacheOnlyMode = TRUE;

    // restrict access to root
    if (peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_OfflineFindGroupById(
                  hProvider,
                  gid,
                  bIsCacheOnlyMode,
                  dwGroupInfoLevel,
                  (PVOID*)&pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheRemoveGroupBySid(
                  gpLsaAdProviderState->hCacheConnection,
                  pGroupInfo->pszSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(
            dwGroupInfoLevel,
            pGroupInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_GetUserGroupObjectMembership(
    IN HANDLE hProvider,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppResult
    )
{
    DWORD dwError = 0;

    if (AD_IsOffline())
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineGetUserGroupObjectMembership(
            hProvider,
            pUserInfo,
            bIsCacheOnlyMode,
            psNumGroupsFound,
            pppResult);
    }

    if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineGetUserGroupObjectMembership(
            hProvider,
            pUserInfo,
            psNumGroupsFound,
            pppResult);
    }

    return dwError;
}

DWORD
AD_GroupObjectToGroupInfo(
    IN HANDLE hProvider,
    IN PLSA_SECURITY_OBJECT pGroupObject,
    IN BOOLEAN bIsCacheOnlyMode,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    size_t sMembers = 0;
    PLSA_SECURITY_OBJECT *ppMembers = NULL;
    PSTR pszFullDomainName = NULL;

    switch (dwGroupInfoLevel)
    {
        case 0:
            // nothing to do
            break;
        case 1:
            // need to expand membership
            dwError = LsaDmEngineGetDomainNameAndSidByObjectSidWithDiscovery(
                         pGroupObject->pszObjectSid,
                         &pszFullDomainName,
                         NULL,
                         NULL);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = AD_GetExpandedGroupUsers(
                hProvider,
                pszFullDomainName,
                pGroupObject->pszObjectSid,
                bIsCacheOnlyMode,
                5,
                NULL,
                &sMembers,
                &ppMembers);
            if (dwError == LSA_ERROR_NO_SUCH_OBJECT)
            {
                dwError = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LSA_ERROR_INVALID_GROUP_INFO_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    dwError = ADMarshalFromGroupCache(
                pGroupObject,
                sMembers,
                ppMembers,
                dwGroupInfoLevel,
                ppGroupInfo
                );
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    ADCacheSafeFreeObjectList(sMembers, &ppMembers);
    LSA_SAFE_FREE_STRING(pszFullDomainName);

    return dwError;

error:
    *ppGroupInfo = NULL;

    goto cleanup;
}


DWORD
AD_GetGroupsForUser(
    IN HANDLE hProvider,
    IN OPTIONAL PCSTR pszUserName,
    IN OPTIONAL uid_t uid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    IN PDWORD pdwNumGroupsFound,
    IN PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppGroupObjects = NULL;
    size_t sGroupObjectsCount = 0;
    PVOID* ppGroupInfoList = NULL;
    size_t sIndex = 0;
    size_t sEnabledCount = 0;
    BOOLEAN bIsCacheOnlyMode = FALSE;
    HANDLE hLsaConnection = NULL;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    PSTR pszUserSID = NULL;
    PSTR pszUserDN = NULL;
    PSTR pszGroupSID = NULL;
    PSTR pszGroupDN = NULL;
    DWORD dwGroupsCount = 0;
    DWORD i = 0;
    PVOID *ppUserMembershipInfo = NULL;
    PVOID *ppGroupMembershipInfo = NULL;
    PLSA_HASH_TABLE pUserMemberships = NULL;
    LSA_HASH_ITERATOR hashIterator = {0};
    PVOID pMemberInfo = NULL;
    LSA_HASH_ENTRY *pHashEntry = NULL;
    DWORD dwUserMembershipCount = 0;
    PVOID *ppUserMembership = NULL;

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsCacheOnlyMode = AD_GetNssUserMembershipCacheOnlyEnabled();
    }

    dwError = LsaHashCreate(
                    13,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessString,
                    NULL,
                    NULL,
                    &pUserMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszUserName)
    {
        dwError = AD_FindUserObjectByName(
                        hProvider,
                        pszUserName,
                        &pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = AD_FindUserObjectById(
                        hProvider,
                        uid,
                        &pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_GetUserGroupObjectMembership(
                hProvider,
                pUserInfo,
                bIsCacheOnlyMode,
                &sGroupObjectsCount,
                &ppGroupObjects);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaOpenServer(&hLsaConnection);
    BAIL_ON_LSA_ERROR(dwError);

    pszUserSID = pUserInfo->pszObjectSid;
    pszUserDN  = pUserInfo->pszDN;

    /* Get all local groups the user is member of */
    dwError = LsaLocalGetGroupMembership(
                hLsaConnection,
                pszUserSID,
                pszUserDN,
                dwGroupInfoLevel,
                &dwGroupsCount,
                (PVOID**)&ppUserMembershipInfo);
    BAIL_ON_LSA_ERROR(dwError);

    /* Store membership list in a hash table */
    for (i = 0; i < dwGroupsCount; i++)
    {
        pszGroupSID = ((PLSA_GROUP_INFO_0)ppUserMembershipInfo[i])->pszSid;

        dwError = LsaHashGetValue(
                    pUserMemberships,
                    (PCVOID)pszGroupSID,
                    &pMemberInfo);
        if (dwError == ENOENT) {
            dwError = LsaAllocateGroupInfo(
                        &pMemberInfo,
                        dwGroupInfoLevel,
                        ppUserMembershipInfo[i]);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaHashSetValue(
                        pUserMemberships,
                        (PVOID)pszGroupSID,
                        pMemberInfo);
            BAIL_ON_LSA_ERROR(dwError);

            pMemberInfo = NULL;
        }
    }

    /*
     * Check all domain groups the user is member of and
     * convert the group objects into group info.
     */

    dwError = LsaAllocateMemory(
                sizeof(*ppGroupInfoList) * sGroupObjectsCount,
                (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    for (sIndex = 0; sIndex < sGroupObjectsCount; sIndex++)
    {
        if (ppGroupObjects[sIndex]->type != AccountType_Group)
        {
            LSA_LOG_DEBUG("Skipping non-group SID %s (type = %d)",
                          LSA_SAFE_LOG_STRING(ppGroupObjects[sIndex]->pszObjectSid),
                          ppGroupObjects[sIndex]->type);
            continue;
        }

        dwError = AD_GroupObjectToGroupInfo(
                    hProvider,
                    ppGroupObjects[sIndex],
                    bIsCacheOnlyMode,
                    dwGroupInfoLevel,
                    &ppGroupInfoList[sEnabledCount]);
        if (dwError == LSA_ERROR_OBJECT_NOT_ENABLED)
        {
            // Filter this group from the list
            dwError = LSA_ERROR_SUCCESS;
            continue;
        }
        BAIL_ON_LSA_ERROR(dwError);

        pszGroupSID = ((PLSA_GROUP_INFO_0)ppGroupInfoList[sEnabledCount])->pszSid;

        /* Store current group in the hash table */
        dwError = LsaHashGetValue(
                    pUserMemberships,
                    (PCVOID)pszGroupSID,
                    &pMemberInfo);
        if (dwError == ENOENT) {
            dwError = LsaAllocateGroupInfo(
                        &pMemberInfo,
                        dwGroupInfoLevel,
                        ppGroupInfoList[sEnabledCount]);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaHashSetValue(
                        pUserMemberships,
                        (PVOID)pszGroupSID,
                        pMemberInfo);
            BAIL_ON_LSA_ERROR(dwError);

            pMemberInfo = NULL;
        }

        pszGroupDN  = ppGroupObjects[sIndex]->pszDN;
        pszGroupSID = ppGroupObjects[sIndex]->pszObjectSid;

        /* Get all local groups that current group is member of. This resolves
           nested memberships of domain groups */
        dwError = LsaLocalGetGroupMembership(
                    hLsaConnection,
                    pszGroupSID,
                    pszGroupDN,
                    dwGroupInfoLevel,
                    &dwGroupsCount,
                    (PVOID**)&ppGroupMembershipInfo);
        BAIL_ON_LSA_ERROR(dwError);

        /* Store returned local groups in the hash table */
        for (i = 0; i < dwGroupsCount; i++)
        {
            pszGroupSID = ((PLSA_GROUP_INFO_0)ppGroupMembershipInfo[i])->pszSid;

            dwError = LsaHashGetValue(
                        pUserMemberships,
                        (PCVOID)pszGroupSID,
                        &pMemberInfo);
            if (dwError == ENOENT) {
                dwError = LsaAllocateGroupInfo(
                            &pMemberInfo,
                            dwGroupInfoLevel,
                            ppGroupMembershipInfo[i]);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaHashSetValue(
                            pUserMemberships,
                            (PVOID)pszGroupSID,
                            pMemberInfo);
                BAIL_ON_LSA_ERROR(dwError);

                pMemberInfo = NULL;
            }
        }

        sEnabledCount++;

        if (sEnabledCount == DWORD_MAX)
        {
            dwError = ERANGE;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (ppGroupMembershipInfo)
        {
            LsaFreeGroupInfoList(dwGroupInfoLevel,
                                 (PVOID*)ppGroupMembershipInfo,
                                 dwGroupsCount);
            ppGroupMembershipInfo = NULL;
        }
    }

    dwError = LsaHashGetIterator(pUserMemberships,
                                 &hashIterator);
    BAIL_ON_LSA_ERROR(dwError);

    while (LsaHashNext(&hashIterator)) {
        dwUserMembershipCount++;
    }

    dwError = LsaAllocateMemory(
                 sizeof(ppUserMembership[0]) * dwUserMembershipCount,
                 (PVOID*)&ppUserMembership);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashGetIterator(pUserMemberships, &hashIterator);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; (pHashEntry = LsaHashNext(&hashIterator)) != NULL; i++)
    {
        ppUserMembership[i] = pHashEntry->pValue;
        /* It is safe to remove the entry just returned by
           LsaHashNext() from the hash table because the
           iterator already points to the next item */
        LsaHashRemoveKey(pUserMemberships, pHashEntry->pKey);
    }

    if (AD_ShouldAssumeDefaultDomain())
    {
        DWORD iGroup = 0;

        for (iGroup = 0; iGroup < (DWORD)sEnabledCount; iGroup++)
        {
            PVOID pGroupInfo = *(ppUserMembership + iGroup);

            dwError = AD_SetGroupCanonicalNamesToAliases(
                            gpADProviderData->szShortDomain,
                            dwGroupInfoLevel,
                            pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pdwNumGroupsFound = dwUserMembershipCount;
    *pppGroupInfoList  = ppUserMembership;

cleanup:

    if (hLsaConnection)
    {
        LsaCloseServer(hLsaConnection);
    }

    ADCacheSafeFreeObject(&pUserInfo);

    if (ppUserMembershipInfo)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel,
                             (PVOID*)ppUserMembershipInfo,
                             dwGroupsCount);
    }

    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel,
                             ppGroupInfoList,
                             sGroupObjectsCount);
    }

    if (ppGroupMembershipInfo)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel,
                             (PVOID*)ppGroupMembershipInfo,
                             dwGroupsCount);
    }

    if (pUserMemberships)
    {
        dwError = LsaHashGetIterator(pUserMemberships, &hashIterator);
        assert(dwError == LSA_ERROR_SUCCESS);

        while ((pHashEntry = LsaHashNext(&hashIterator)) != NULL)
        {
            LsaFreeGroupInfo(dwGroupInfoLevel, pHashEntry->pValue);
        }

        LsaHashSafeFree(&pUserMemberships);
    }

    ADCacheSafeFreeObjectList(sGroupObjectsCount, &ppGroupObjects);
    return dwError;

error:

    if (ppUserMembership != NULL)
    {
        LsaFreeGroupInfoList(
            dwGroupInfoLevel,
            ppUserMembership,
            (DWORD)sEnabledCount);
    }

    *pdwNumGroupsFound = 0;
    *pppGroupInfoList = NULL;
    goto cleanup;
}

DWORD
AD_BeginEnumGroups(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = NULL;

    dwError = AD_CreateGroupState(
                        hProvider,
                        dwInfoLevel,
                        bCheckGroupMembersOnline,
                        FindFlags,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    LwInitCookie(&pEnumState->Cookie);

    *phResume = (HANDLE)pEnumState;

cleanup:

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    goto cleanup;
}

DWORD
AD_EnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PVOID* ppGroupInfoList = NULL;
    DWORD  dwNumGroupsFound = 0;

    if (AD_IsOffline())
    {
        dwError = AD_OfflineEnumGroups(
                    hProvider,
                    hResume,
                    dwMaxGroups,
                    &dwNumGroupsFound,
                    &ppGroupInfoList);
    }
    else
    {
        dwError = AD_OnlineEnumGroups(
                    hProvider,
                    hResume,
                    dwMaxGroups,
                    &dwNumGroupsFound,
                    &ppGroupInfoList);
    }

    BAIL_ON_LSA_ERROR(dwError);

    if (AD_ShouldAssumeDefaultDomain())
    {
        DWORD iGroup = 0;
        PAD_ENUM_STATE pEnumState = (PAD_ENUM_STATE)hResume;

        for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
        {
            PVOID pGroupInfo = *(ppGroupInfoList + iGroup);

            dwError = AD_SetGroupCanonicalNamesToAliases(
                            gpADProviderData->szShortDomain,
                            pEnumState->dwInfoLevel,
                            pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *pppGroupInfoList = ppGroupInfoList;
    *pdwGroupsFound = dwNumGroupsFound;

cleanup:

    return dwError;

error:

    *pppGroupInfoList = NULL;
    *pdwGroupsFound = 0;

    if (ppGroupInfoList)
    {
        PAD_ENUM_STATE pEnumState = (PAD_ENUM_STATE)hResume;

        LsaFreeGroupInfoList(pEnumState->dwInfoLevel, ppGroupInfoList, dwNumGroupsFound);
    }

    goto cleanup;
}

VOID
AD_EndEnumGroups(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    AD_FreeGroupState(hProvider, (PAD_ENUM_STATE) hResume);
}

DWORD
AD_ChangePassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    )
{
    if (AD_IsOffline())
    {
        return AD_OfflineChangePassword(
            hProvider,
            pszLoginId,
            pszPassword,
            pszOldPassword);
    }
    else
    {
        return AD_OnlineChangePassword(
            hProvider,
            pszLoginId,
            pszPassword,
            pszOldPassword);
    }
}

DWORD
AD_SetPassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword
    )
{
    return LSA_ERROR_NOT_HANDLED;
}


DWORD
AD_AddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
AD_ModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
AD_DeleteUser(
    HANDLE hProvider,
    uid_t  uid
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
AD_AddGroup(
    HANDLE hProvider,
    DWORD dwGroupInfoLevel,
    PVOID pGroupInfo
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
AD_ModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
AD_DeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
AD_EmptyCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID
    )
{
    DWORD dwError = 0;

    // restrict access to root
    if (peerUID)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheEmptyCache(
                  gpLsaAdProviderState->hCacheConnection);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_OpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pLoginInfo = NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 1;

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    gpADProviderData->szDomain,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_FindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_CreateHomeDirectory(
                    (PLSA_USER_INFO_1)pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_ShouldCreateK5Login()) {

        dwError = AD_CreateK5Login(
                    (PLSA_USER_INFO_1)pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);

    }

cleanup:

    if (pLoginInfo)
    {
        LsaFreeNameInfo(pLoginInfo);
    }

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_CloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    dwError = AD_FindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUmRemoveUser(
                  ((PLSA_USER_INFO_0)pUserInfo)->uid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_GetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes
    )
{
    if (AD_IsOffline())
    {
        return AD_OfflineGetNamesBySidList(
            hProvider,
            sCount,
            ppszSidList,
            pppszDomainNames,
            pppszSamAccounts,
            ppTypes);
    }
    else
    {
        return AD_OnlineGetNamesBySidList(
            hProvider,
            sCount,
            ppszSidList,
            pppszDomainNames,
            pppszSamAccounts,
            ppTypes);
    }
}

DWORD
AD_FindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    if (AD_IsOffline())
    {
        return AD_OfflineFindNSSArtefactByKey(
                        hProvider,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
    }
    else
    {
        return AD_OnlineFindNSSArtefactByKey(
                        hProvider,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
    }
}

DWORD
AD_BeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = NULL;

    if (!dwFlags)
    {
        dwError = LSA_ERROR_INVALID_PARAMETER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    switch (gpADProviderData->dwDirectoryMode)
    {
        case DEFAULT_MODE:
        case CELL_MODE:

            dwError = AD_CreateNSSArtefactState(
                                hProvider,
                                dwInfoLevel,
                                pszMapName,
                                dwFlags,
                                &pEnumState);
            BAIL_ON_LSA_ERROR(dwError);

            LwInitCookie(&pEnumState->Cookie);

            break;

        case UNPROVISIONED_MODE:

            dwError = LSA_ERROR_NOT_SUPPORTED;
            break;
    }

    *phResume = (HANDLE)pEnumState;

cleanup:

    return dwError;

error:

    *phResume = (HANDLE)NULL;

    goto cleanup;
}

DWORD
AD_EnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
    if (AD_IsOffline())
    {
        return AD_OfflineEnumNSSArtefacts(
            hProvider,
            hResume,
            dwMaxNSSArtefacts,
            pdwNSSArtefactsFound,
            pppNSSArtefactInfoList);
    }
    else
    {
        return AD_OnlineEnumNSSArtefacts(
            hProvider,
            hResume,
            dwMaxNSSArtefacts,
            pdwNSSArtefactsFound,
            pppNSSArtefactInfoList);
    }
}

VOID
AD_EndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    AD_FreeNSSArtefactState(hProvider, (PAD_ENUM_STATE)hResume);
}

DWORD
AD_GetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    )
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDCInfo = NULL;
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus = NULL;

    dwError = LsaAllocateMemory(
                   sizeof(LSA_AUTH_PROVIDER_STATUS),
                   (PVOID*)&pProviderStatus);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    gpszADProviderName,
                    &pProviderStatus->pszId);
    BAIL_ON_LSA_ERROR(dwError);

    switch (gpADProviderData->dwDirectoryMode)
    {
        case DEFAULT_MODE:

            pProviderStatus->mode = LSA_PROVIDER_MODE_DEFAULT_CELL;
            break;

        case CELL_MODE:

            pProviderStatus->mode = LSA_PROVIDER_MODE_NON_DEFAULT_CELL;

            if (!IsNullOrEmptyString(gpADProviderData->cell.szCellDN))
            {
                dwError = LsaAllocateString(
                                gpADProviderData->cell.szCellDN,
                                &pProviderStatus->pszCell);
                BAIL_ON_LSA_ERROR(dwError);
            }
            break;

        case UNPROVISIONED_MODE:

            pProviderStatus->mode = LSA_PROVIDER_MODE_UNPROVISIONED;
            break;

        default:

            pProviderStatus->mode = LSA_PROVIDER_MODE_UNKNOWN;
            break;
    }

    switch (gpADProviderData->adConfigurationMode)
    {
        case SchemaMode:

            pProviderStatus->subMode = LSA_AUTH_PROVIDER_SUBMODE_SCHEMA;
            break;

        case NonSchemaMode:

            pProviderStatus->subMode = LSA_AUTH_PROVIDER_SUBMODE_NONSCHEMA;
            break;

        default:

            pProviderStatus->subMode = LSA_AUTH_PROVIDER_SUBMODE_UNKNOWN;
            break;
    }

    if (!IsNullOrEmptyString(gpADProviderData->szDomain))
    {
        dwError = LsaAllocateString(
                        gpADProviderData->szDomain,
                        &pProviderStatus->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LWNetGetDCName(
                        NULL,
                        gpADProviderData->szDomain,
                        NULL,
                        DS_BACKGROUND_ONLY,
                        &pDCInfo);
        if (LWNET_ERROR_DOMAIN_NOT_FOUND == dwError)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (pDCInfo)
        {
            if (!IsNullOrEmptyString(pDCInfo->pszDnsForestName))
            {
                dwError = LsaAllocateString(
                                pDCInfo->pszDnsForestName,
                                &pProviderStatus->pszForest);
                BAIL_ON_LSA_ERROR(dwError);
            }

            if (!IsNullOrEmptyString(pDCInfo->pszDCSiteName))
            {
                dwError = LsaAllocateString(
                                pDCInfo->pszDCSiteName,
                                &pProviderStatus->pszSite);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    dwError = AD_GetTrustedDomainInfo(
                    &pProviderStatus->pTrustedDomainInfoArray,
                    &pProviderStatus->dwNumTrustedDomains);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_IsOffline())
    {
        pProviderStatus->status = LSA_AUTH_PROVIDER_STATUS_OFFLINE;
    }
    else
    {
        pProviderStatus->status = LSA_AUTH_PROVIDER_STATUS_ONLINE;
    }

    dwError = LsaDmQueryState(NULL, &pProviderStatus->dwNetworkCheckInterval, NULL);
    BAIL_ON_LSA_ERROR(dwError);

    *ppProviderStatus = pProviderStatus;

cleanup:

    LWNET_SAFE_FREE_DC_INFO(pDCInfo);

    return dwError;

error:

    *ppProviderStatus = NULL;

    if (pProviderStatus)
    {
        AD_FreeStatus(pProviderStatus);
    }

    goto cleanup;
}

DWORD
AD_GetTrustedDomainInfo(
    PLSA_TRUSTED_DOMAIN_INFO* ppDomainInfoArray,
    PDWORD pdwNumTrustedDomains
    )
{
    DWORD dwError = 0;
    PLSA_TRUSTED_DOMAIN_INFO pDomainInfoArray = NULL;
    DWORD dwCount = 0;
    PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo = NULL;

    dwError = LsaDmEnumDomainInfo(NULL, NULL, &ppDomainInfo, &dwCount);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwCount)
    {
        DWORD iDomain = 0;

        dwError = LsaAllocateMemory(
                        sizeof(pDomainInfoArray[0]) * dwCount,
                        (PVOID*)&pDomainInfoArray);
        BAIL_ON_LSA_ERROR(dwError);

        for (iDomain = 0; iDomain < dwCount; iDomain++)
        {
            dwError = AD_FillTrustedDomainInfo(
                        ppDomainInfo[iDomain],
                        &pDomainInfoArray[iDomain]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    *ppDomainInfoArray = pDomainInfoArray;
    *pdwNumTrustedDomains = dwCount;

cleanup:
    LsaDmFreeEnumDomainInfoArray(ppDomainInfo);

    return dwError;

error:

    *ppDomainInfoArray = NULL;
    *pdwNumTrustedDomains = 0;

    if (pDomainInfoArray)
    {
        LsaFreeDomainInfoArray(dwCount, pDomainInfoArray);
    }

    goto cleanup;
}

VOID
AD_FreeTrustedDomainsInList(
    PVOID pItem,
    PVOID pUserData
    )
{
    if (pItem)
    {
        LsaFreeDomainInfo((PLSA_TRUSTED_DOMAIN_INFO)pItem);
    }
}

DWORD
AD_FillTrustedDomainInfo(
    IN PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo,
    OUT PLSA_TRUSTED_DOMAIN_INFO pTrustedDomainInfo
    )
{
    DWORD dwError = 0;
    PLWNET_DC_INFO pDcInfo = NULL;
    // Do not free dcInfo as it just points to other data.
    LSA_DM_DC_INFO dcInfo = { 0 };

    dwError = LsaStrDupOrNull(
                    pDomainInfo->pszDnsDomainName,
                    &pTrustedDomainInfo->pszDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrDupOrNull(
                    pDomainInfo->pszNetbiosDomainName,
                    &pTrustedDomainInfo->pszNetbiosDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (pDomainInfo->pSid)
    {
        dwError = LsaAllocateCStringFromSid(
                        &pTrustedDomainInfo->pszDomainSID,
                        pDomainInfo->pSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pDomainInfo->pGuid)
    {
        CHAR szGUID[37] = "";

        uuid_unparse(*pDomainInfo->pGuid, szGUID);

        dwError = LsaAllocateString(
                        szGUID,
                        &pTrustedDomainInfo->pszDomainGUID);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaStrDupOrNull(
                    pDomainInfo->pszTrusteeDnsDomainName,
                    &pTrustedDomainInfo->pszTrusteeDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    pTrustedDomainInfo->dwTrustFlags = pDomainInfo->dwTrustFlags;
    pTrustedDomainInfo->dwTrustType = pDomainInfo->dwTrustType;
    pTrustedDomainInfo->dwTrustAttributes = pDomainInfo->dwTrustAttributes;
    pTrustedDomainInfo->dwTrustDirection = pDomainInfo->dwTrustDirection;
    pTrustedDomainInfo->dwTrustMode = pDomainInfo->dwTrustMode;

    dwError = LsaStrDupOrNull(
                    pDomainInfo->pszForestName,
                    &pTrustedDomainInfo->pszForestName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrDupOrNull(
                    pDomainInfo->pszClientSiteName,
                    &pTrustedDomainInfo->pszClientSiteName);
    BAIL_ON_LSA_ERROR(dwError);

    pTrustedDomainInfo->dwDomainFlags = pDomainInfo->Flags;

    if (pDomainInfo->DcInfo)
    {
        dwError = AD_BuildDCInfo(
                        pDomainInfo->DcInfo,
                        &pTrustedDomainInfo->pDCInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = LWNetGetDCName(
                    NULL,
                    pDomainInfo->pszDnsDomainName,
                    NULL,
                    DS_BACKGROUND_ONLY,
                    &pDcInfo);
        if (LWNET_ERROR_DOMAIN_NOT_FOUND == dwError)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (pDcInfo)
        {
            dcInfo.dwDsFlags = pDcInfo->dwFlags;
            dcInfo.pszName = pDcInfo->pszDomainControllerName;
            dcInfo.pszAddress = pDcInfo->pszDomainControllerAddress;
            dcInfo.pszSiteName = pDcInfo->pszDCSiteName;

            dwError = AD_BuildDCInfo(
                            &dcInfo,
                            &pTrustedDomainInfo->pDCInfo);
            BAIL_ON_LSA_ERROR(dwError);

            if (!pTrustedDomainInfo->pszClientSiteName)
            {
                dwError = LsaStrDupOrNull(
                            pDcInfo->pszClientSiteName,
                            &pTrustedDomainInfo->pszClientSiteName);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    if (pDomainInfo->GcInfo)
    {
        dwError = AD_BuildDCInfo(
                        pDomainInfo->GcInfo,
                        &pTrustedDomainInfo->pGCInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);
        dwError = LWNetGetDCName(
                    NULL,
                    pDomainInfo->pszForestName,
                    NULL,
                    DS_GC_SERVER_REQUIRED | DS_BACKGROUND_ONLY,
                    &pDcInfo);
        if (LWNET_ERROR_DOMAIN_NOT_FOUND == dwError)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (pDcInfo)
        {
            dcInfo.dwDsFlags = pDcInfo->dwFlags;
            dcInfo.pszName = pDcInfo->pszDomainControllerName;
            dcInfo.pszAddress = pDcInfo->pszDomainControllerAddress;
            dcInfo.pszSiteName = pDcInfo->pszDCSiteName;

            dwError = AD_BuildDCInfo(
                            &dcInfo,
                            &pTrustedDomainInfo->pGCInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:
    LWNET_SAFE_FREE_DC_INFO(pDcInfo);

    return dwError;

error:
    LsaFreeDomainInfoContents(pTrustedDomainInfo);
    goto cleanup;
}

DWORD
AD_BuildDCInfo(
    PLSA_DM_DC_INFO pDCInfo,
    PLSA_DC_INFO*   ppDCInfo
    )
{
    DWORD dwError = 0;
    PLSA_DC_INFO pDestDCInfo = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(LSA_DC_INFO),
                    (PVOID*)&pDestDCInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrDupOrNull(
                    pDCInfo->pszName,
                    &pDestDCInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrDupOrNull(
                    pDCInfo->pszAddress,
                    &pDestDCInfo->pszAddress);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaStrDupOrNull(
                    pDCInfo->pszSiteName,
                    &pDestDCInfo->pszSiteName);
    BAIL_ON_LSA_ERROR(dwError);

    pDestDCInfo->dwFlags = pDCInfo->dwDsFlags;

    *ppDCInfo = pDestDCInfo;

cleanup:

    return dwError;

error:

    *ppDCInfo = NULL;

    if (pDestDCInfo)
    {
        LsaFreeDCInfo(pDestDCInfo);
    }

    goto cleanup;
}

VOID
AD_FreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus
    )
{
    LSA_SAFE_FREE_STRING(pProviderStatus->pszId);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszDomain);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszForest);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszSite);
    LSA_SAFE_FREE_STRING(pProviderStatus->pszCell);

    if (pProviderStatus->pTrustedDomainInfoArray)
    {
        LsaFreeDomainInfoArray(
                        pProviderStatus->dwNumTrustedDomains,
                        pProviderStatus->pTrustedDomainInfoArray);
    }

    LsaFreeMemory(pProviderStatus);
}

DWORD
AD_RefreshConfiguration(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PSTR  pszConfigFilePath = NULL;
    LSA_AD_CONFIG config = {0};
    BOOLEAN bInLock = FALSE;

    dwError = AD_GetConfigFilePath(&pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);

    if (!IsNullOrEmptyString(pszConfigFilePath)) {
        dwError = AD_InitializeConfig(&config);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_ParseConfigFile(
                        pszConfigFilePath,
                        &config);
        BAIL_ON_LSA_ERROR(dwError);

        ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

        dwError = AD_TransferConfigContents(
                        &config,
                        &gpLsaAdProviderState->config);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaSetDomainSeparator(
                    gpLsaAdProviderState->config.chDomainSeparator);
        BAIL_ON_LSA_ERROR(dwError);

        AD_FreeAllowedSIDs_InLock();

        dwError = LsaDmSetState(
                        NULL,
                        &gpLsaAdProviderState->config.DomainManager.dwCheckDomainOnlineSeconds,
                        &gpLsaAdProviderState->config.DomainManager.dwUnknownDomainCacheTimeoutSeconds);
        BAIL_ON_LSA_ERROR(dwError);

        LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

        LsaAdProviderLogConfigReloadEvent();
        LsaAdProviderLogRequireMembershipOfChangeEvent(hProvider);
        LsaAdProviderLogEventLogEnableChangeEvent();
    }

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    LSA_SAFE_FREE_STRING(pszConfigFilePath);

    return dwError;

error:

    AD_FreeConfigContents(&config);

    goto cleanup;

}

DWORD
AD_ProviderIoControl(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN uid_t   peerGID,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;

    switch (dwIoControlCode)
    {
        case LSA_AD_IO_EMPTYCACHE:
            dwError = AD_EmptyCache(
                          hProvider,
                          peerUID,
                          peerGID);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_REMOVEUSERBYNAMECACHE:
            dwError = AD_RemoveUserByNameFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          (PCSTR)pInputBuffer);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_REMOVEUSERBYIDCACHE:
            dwError = AD_RemoveUserByIdFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          *(uid_t *)pInputBuffer);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_REMOVEGROUPBYNAMECACHE:
            dwError = AD_RemoveGroupByNameFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          (PCSTR)pInputBuffer);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_REMOVEGROUPBYIDCACHE:
            dwError = AD_RemoveGroupByIdFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          *(gid_t *)pInputBuffer);
            *pdwOutputBufferSize=0;
            *ppOutputBuffer = NULL;
            break;
        case LSA_AD_IO_ENUMUSERSCACHE:
            dwError = AD_EnumUsersFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          dwInputBufferSize,
                          pInputBuffer,
                          pdwOutputBufferSize,
                          ppOutputBuffer);
            break;
        case LSA_AD_IO_ENUMGROUPSCACHE:
            dwError = AD_EnumGroupsFromCache(
                          hProvider,
                          peerUID,
                          peerGID,
                          dwInputBufferSize,
                          pInputBuffer,
                          pdwOutputBufferSize,
                          ppOutputBuffer);
            break;
        default:
            dwError = LSA_ERROR_NOT_HANDLED;
            break;
    }
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    *pdwOutputBufferSize=0;
    *ppOutputBuffer = NULL;

    goto cleanup;
}

static
DWORD
AD_GetNameWithReplacedSeparators(
    IN PCSTR pszName,
    OUT PSTR* ppszFreeName,
    OUT PCSTR* ppszUseName
    )
{
    DWORD dwError = 0;
    // Capture the separator here so we consistent within
    // this function in case it changes.
    const CHAR chSeparator = AD_GetSpaceReplacement();
    PSTR pszLocalName = NULL;
    PCSTR pszUseName = NULL;

    if (strchr(pszName, chSeparator))
    {
        dwError = LsaAllocateString(
                        pszName,
                        &pszLocalName);
        BAIL_ON_LSA_ERROR(dwError);

        LsaStrCharReplace(pszLocalName, chSeparator, ' ');

        pszUseName = pszLocalName;
    }
    else
    {
        pszUseName = pszName;
    }

    *ppszFreeName = pszLocalName;
    *ppszUseName = pszUseName;

cleanup:
    return dwError;

error:
    LSA_SAFE_FREE_STRING(pszLocalName);

    *ppszFreeName = NULL;
    *ppszUseName = NULL;

    goto cleanup;
}

static
DWORD
AD_FindUserObjectByNameInternal(
    IN HANDLE  hProvider,
    IN PCSTR   pszLoginId,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PSTR pszFreeLoginId = NULL;
    PCSTR pszUseLoginId = NULL;
    PLSA_SECURITY_OBJECT pResult = NULL;

    dwError = AD_GetNameWithReplacedSeparators(
                pszLoginId,
                &pszFreeLoginId,
                &pszUseLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_IsOffline())
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineFindUserObjectByName(
                        hProvider,
                        pszUseLoginId,
                        &pResult);
    }

    if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineFindUserObjectByName(
                        hProvider,
                        pszUseLoginId,
                        &pResult);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pResult;

cleanup:
    LSA_SAFE_FREE_STRING(pszFreeLoginId);
    return dwError;

error:
    *ppResult = NULL;
    ADCacheSafeFreeObject(&pResult);

    goto cleanup;
}

DWORD
AD_FindUserObjectByName(
    IN HANDLE  hProvider,
    IN PCSTR   pszLoginId,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PSTR pszLocalLoginId = NULL;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    PLSA_SECURITY_OBJECT pResult = NULL;

    if (!strcasecmp(pszLoginId, "root"))
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindUserObjectByNameInternal(
                hProvider,
                pszLoginId,
                &pResult);
    if (dwError == LSA_ERROR_NO_SUCH_USER &&
        AD_ShouldAssumeDefaultDomain())
    {
        dwError = LsaCrackDomainQualifiedName(
                            pszLoginId,
                            gpADProviderData->szDomain,
                            &pUserNameInfo);
        BAIL_ON_LSA_ERROR(dwError);

        if (pUserNameInfo->nameType == NameType_Alias)
        {
            dwError = ADGetDomainQualifiedString(
                        gpADProviderData->szShortDomain,
                        pszLoginId,
                        &pszLocalLoginId);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = AD_FindUserObjectByNameInternal(
                        hProvider,
                        pszLocalLoginId,
                        &pResult);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_NO_SUCH_USER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pResult;

cleanup:

    LSA_SAFE_FREE_STRING(pszLocalLoginId);
    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }

    return dwError;

error:

    *ppResult = NULL;

    ADCacheSafeFreeObject(&pResult);

    goto cleanup;
}

static
DWORD
AD_RemoveUserByNameFromCacheInternal(
    IN HANDLE hProvider,
    IN PCSTR  pszLoginId
    )
{
    DWORD                dwError = 0;
    PSTR                 pszFreeLoginId = NULL;
    PCSTR                pszUseLoginId = NULL;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;

    dwError = AD_GetNameWithReplacedSeparators(
                  pszLoginId,
                  &pszFreeLoginId,
                  &pszUseLoginId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_OfflineFindUserObjectByName(
                  hProvider,
                  pszUseLoginId,
                  &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheRemoveUserBySid(
                  gpLsaAdProviderState->hCacheConnection,
                  pUserInfo->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_STRING(pszFreeLoginId);
    ADCacheSafeFreeObject(&pUserInfo);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_FindGroupObjectByNameInternal(
    IN HANDLE  hProvider,
    IN PCSTR   pszGroupName,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PSTR pszFreeGroupName = NULL;
    PCSTR pszUseGroupName = NULL;
    PLSA_SECURITY_OBJECT pResult = NULL;

    dwError = AD_GetNameWithReplacedSeparators(
                pszGroupName,
                &pszFreeGroupName,
                &pszUseGroupName);
    BAIL_ON_LSA_ERROR(dwError);

    if (AD_IsOffline())
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineFindGroupObjectByName(
                        hProvider,
                        pszUseGroupName,
                        &pResult);
    }

    if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineFindGroupObjectByName(
                        hProvider,
                        pszUseGroupName,
                        &pResult);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pResult;

cleanup:
    LSA_SAFE_FREE_STRING(pszFreeGroupName);
    return dwError;

error:
    *ppResult = NULL;
    ADCacheSafeFreeObject(&pResult);

    goto cleanup;
}

DWORD
AD_FindGroupObjectByName(
    IN HANDLE  hProvider,
    IN PCSTR   pszGroupName,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PSTR pszLocalGroupName = NULL;
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    PLSA_SECURITY_OBJECT pResult = NULL;

    if (!strcasecmp(pszGroupName, "root"))
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindGroupObjectByNameInternal(
                hProvider,
                pszGroupName,
                &pResult);
    if (dwError == LSA_ERROR_NO_SUCH_GROUP &&
        AD_ShouldAssumeDefaultDomain())
    {
        dwError = LsaCrackDomainQualifiedName(
                            pszGroupName,
                            gpADProviderData->szDomain,
                            &pGroupNameInfo);
        BAIL_ON_LSA_ERROR(dwError);

        if (pGroupNameInfo->nameType == NameType_Alias)
        {
            dwError = ADGetDomainQualifiedString(
                        gpADProviderData->szShortDomain,
                        pszGroupName,
                        &pszLocalGroupName);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = AD_FindGroupObjectByNameInternal(
                        hProvider,
                        pszLocalGroupName,
                        &pResult);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else
        {
            dwError = LSA_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pResult;

cleanup:

    LSA_SAFE_FREE_STRING(pszLocalGroupName);
    if (pGroupNameInfo)
    {
        LsaFreeNameInfo(pGroupNameInfo);
    }

    return dwError;

error:

    *ppResult = NULL;

    ADCacheSafeFreeObject(&pResult);

    goto cleanup;
}

DWORD
AD_InitializeOperatingMode(
    IN PCSTR pszDomain,
    IN PCSTR pszHostName,
    IN BOOLEAN bIsDomainOffline
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_PROVIDER_DATA pProviderData = NULL;

    if (bIsDomainOffline || AD_IsOffline())
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineInitializeOperatingMode(
                &pProviderData,
                pszDomain,
                pszHostName);
    }
    // If we are offline, do the offline case
    if (LSA_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineInitializeOperatingMode(
                &pProviderData,
                pszDomain,
                pszHostName);
        BAIL_ON_LSA_ERROR(dwError);

        if (bIsDomainOffline)
        {
            // The domain was originally offline, so we need to
            // tell the domain manager about it.
            // Note that we can only transition offline
            // now that we set up the domains in the domain manager.
            dwError = LsaDmTransitionOffline(pszDomain, FALSE);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        // check whether we failed for some other reason.
        BAIL_ON_LSA_ERROR(dwError);
    }

    gpADProviderData = pProviderData;

cleanup:
    return dwError;

error:
    // Note that gpADProviderData will already be NULL.

    if (pProviderData)
    {
        ADProviderFreeProviderData(pProviderData);
        pProviderData = NULL;
    }

    goto cleanup;
}

static
DWORD
LsaAdProviderStateCreate(
    OUT PLSA_AD_PROVIDER_STATE* ppState
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pState = NULL;

    dwError = LsaAllocateMemory(sizeof(*pState), (PVOID)&pState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_mutex_init(&pState->MachineCreds.Mutex, NULL);
    BAIL_ON_LSA_ERROR(dwError);

    pState->MachineCreds.pMutex = &pState->MachineCreds.Mutex;

    dwError = AD_InitializeConfig(&pState->config);
    BAIL_ON_LSA_ERROR(dwError);

    pState->dwMaxAllowedClockDriftSeconds = AD_MAX_ALLOWED_CLOCK_DRIFT_SECONDS;

    *ppState = pState;

cleanup:
    return dwError;

error:
    LsaAdProviderStateDestroy(pState);
    *ppState = NULL;
    goto cleanup;
}

static
VOID
LsaAdProviderStateDestroy(
    IN OUT PLSA_AD_PROVIDER_STATE pState
    )
{
    if (pState)
    {
        ADCacheSafeClose(&pState->hCacheConnection);
        ADState_SafeCloseDb(&pState->hStateConnection);

        MediaSenseStop(&pState->MediaSenseHandle);
        if (pState->MachineCreds.pMutex)
        {
            pthread_mutex_destroy(pState->MachineCreds.pMutex);
            pState->MachineCreds.pMutex = NULL;
        }
        AD_FreeConfigContents(&pState->config);
        LsaFreeMemory(pState);
    }
}

static
DWORD
AD_MachineCredentialsCacheClear()
{
    return LwKrb5CleanupMachineSession();
}

static
BOOLEAN
AD_MachineCredentialsCacheIsInitialized(
    VOID
    )
{
    BOOLEAN bIsInitialized = FALSE;

    pthread_mutex_lock(gpLsaAdProviderState->MachineCreds.pMutex);
    bIsInitialized = gpLsaAdProviderState->MachineCreds.bIsInitialized;
    pthread_mutex_unlock(gpLsaAdProviderState->MachineCreds.pMutex);

    return bIsInitialized;
}

DWORD
AD_MachineCredentialsCacheInitialize(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bIsAcquired = FALSE;
    PSTR pszHostname = NULL;
    PSTR pszUsername = NULL;
    PSTR pszPassword = NULL;
    PSTR pszDomainDnsName = NULL;
    PSTR pszHostDnsDomain = NULL;
    DWORD dwGoodUntilTime = 0;

    // Check before doing any work.
    if (AD_MachineCredentialsCacheIsInitialized())
    {
        goto cleanup;
    }

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrToUpper(pszHostname);

    // Read password info before acquiring the lock.
    dwError = LwKrb5GetMachineCreds(
                    pszHostname,
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaDmIsDomainOffline(pszDomainDnsName))
    {
        dwError = LSA_ERROR_DOMAIN_IS_OFFLINE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pthread_mutex_lock(gpLsaAdProviderState->MachineCreds.pMutex);
    bIsAcquired = TRUE;

    // Verify that state did not change now that we have the lock.
    if (gpLsaAdProviderState->MachineCreds.bIsInitialized)
    {
        goto cleanup;
    }

    ADSyncTimeToDC(pszDomainDnsName);

    dwError = LsaSetupMachineSession(
                    pszHostname,
                    pszUsername,
                    pszPassword,
                    pszDomainDnsName,
                    pszHostDnsDomain,
                    &dwGoodUntilTime);
    BAIL_ON_LSA_ERROR(dwError);

    ADSetMachineTGTExpiry(dwGoodUntilTime);

    gpLsaAdProviderState->MachineCreds.bIsInitialized = TRUE;

cleanup:
    if (bIsAcquired)
    {
        pthread_mutex_unlock(gpLsaAdProviderState->MachineCreds.pMutex);
    }

    LSA_SAFE_FREE_STRING(pszHostname);
    LSA_SAFE_FREE_STRING(pszUsername);
    LSA_SAFE_CLEAR_FREE_STRING(pszPassword);
    LSA_SAFE_FREE_STRING(pszDomainDnsName);
    LSA_SAFE_FREE_STRING(pszHostDnsDomain);

    return dwError;

error:
    goto cleanup;
}

static
BOOLEAN
LsaAdProviderLwKrb5IsOfflineCallback(
    IN PCSTR pszRealm
    )
{
    return LsaDmIsDomainOffline(pszRealm);
}

static
VOID
LsaAdProviderLwKrb5TransitionOfflineCallback(
    IN PCSTR pszRealm
    )
{
    LsaDmTransitionOffline(pszRealm, FALSE);
}

static
VOID
LsaAdProviderMediaSenseTransitionCallback(
    IN PVOID Context,
    IN BOOLEAN bIsOffline
    )
{
    if (bIsOffline)
    {
        LsaDmMediaSenseOffline();
    }
    else
    {
        LsaDmMediaSenseOnline();
    }
}

static
VOID
LsaAdProviderLogServiceStartEvent(
    PCSTR   pszHostname,
    PCSTR   pszDomainDnsName,
    BOOLEAN bIsDomainOffline,
    DWORD   dwErrCode
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszData = NULL;
    PLWNET_DC_INFO pDCInfo = NULL;
    PLWNET_DC_INFO pGCDCInfo = NULL;

    dwError = LWNetGetDCName(
                  NULL,
                  pszDomainDnsName,
                  NULL,
                  DS_BACKGROUND_ONLY,
                  &pDCInfo);

    if (pDCInfo)
    {
        dwError = LWNetGetDCName(
                      NULL,
                      pDCInfo->pszDnsForestName,
                      NULL,
                      DS_GC_SERVER_REQUIRED,
                      &pGCDCInfo);
    }

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider initialization %s.\r\n\r\n" \
                 "     Authentication provider:   %s\r\n\r\n" \
                 "     Hostname:                  %s\r\n" \
                 "     Domain:                    %s\r\n" \
                 "     Current Domain Controller: %s\r\n" \
                 "     Current Global Catalog:    %s\r\n" \
                 "     Offline Startup:           %s",
                 dwErrCode ? "failed" : "succeeded",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 LSA_SAFE_LOG_STRING(pszHostname),
                 LSA_SAFE_LOG_STRING(pszDomainDnsName),
                 (pDCInfo)   ? LSA_SAFE_LOG_STRING(pDCInfo->pszDomainControllerName)   : "(Unknown)" ,
                 (pGCDCInfo) ? LSA_SAFE_LOG_STRING(pGCDCInfo->pszDomainControllerName) : "(Unknown)" ,
                 bIsDomainOffline ? "Yes" : "No");
    BAIL_ON_LSA_ERROR(dwError);

    if (dwErrCode)
    {
        dwError = LsaGetErrorMessageForLoggingEvent(
                         dwErrCode,
                         &pszData);
        BAIL_ON_LSA_ERROR(dwError);

        LsaSrvLogServiceFailureEvent(
                 LSASS_EVENT_FAILED_PROVIDER_INITIALIZATION,
                 SERVICE_EVENT_CATEGORY,
                 pszDescription,
                 pszData);
    }
    else
    {
        LsaSrvLogServiceSuccessEvent(
                 LSASS_EVENT_SUCCESSFUL_PROVIDER_INITIALIZATION,
                 SERVICE_EVENT_CATEGORY,
                 pszDescription,
                 NULL);
    }

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);
    LSA_SAFE_FREE_STRING(pszData);

    LWNET_SAFE_FREE_DC_INFO(pDCInfo);
    LWNET_SAFE_FREE_DC_INFO(pGCDCInfo);

    return;

error:

    goto cleanup;
}

static
VOID
LsaAdProviderLogConfigReloadEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PSTR pszMemberList = NULL;
    PDLINKEDLIST pIter = NULL;

    for (pIter = gpLsaAdProviderState->config.pUnresolvedMemberList;
         pIter;
         pIter = pIter->pNext)
    {
        PSTR pszNewMemberList = NULL;

        dwError = LsaAllocateStringPrintf(
                     &pszNewMemberList,
                     "%s        %s\r\n",
                     pszMemberList ? pszMemberList : "",
                     LSA_SAFE_LOG_STRING((PSTR)pIter->pItem));
        BAIL_ON_LSA_ERROR(dwError);

        LSA_SAFE_FREE_STRING(pszMemberList);
        pszMemberList = pszNewMemberList;
        pszNewMemberList = NULL;
    }

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider configuration settings have been reloaded.\r\n\r\n" \
                 "     Authentication provider:           %s\r\n" \
                 "     Current settings are...\r\n" \
                 "     Cache reaper timeout (secs):       %d\r\n" \
                 "     Cache entry expiry (secs):         %d\r\n" \
                 "     Space replacement character:       '%c'\r\n" \
                 "     Domain separator character:        '%c'\r\n" \
                 "     Enable event log:                  %s\r\n" \
                 "     Logon membership requirements:     \r\n%s" \
                 "     Log network connection events:     %s\r\n" \
                 "     Create K5Login file:               %s\r\n" \
                 "     Create home directory:             %s\r\n" \
                 "     Sign and seal LDAP traffic:        %s\r\n" \
                 "     Assume default domain:             %s\r\n" \
                 "     Sync system time:                  %s\r\n" \
                 "     Refresh user credentials:          %s\r\n" \
                 "     Machine password sync lifetime:    %d\r\n" \
                 "     Default Shell:                     %s\r\n" \
                 "     Default home directory prefix:     %s\r\n" \
                 "     Home directory template:           %s\r\n" \
                 "     Umask:                             %d\r\n" \
                 "     Skeleton directory:                %s\r\n" \
                 "     Cell support:                      %s\r\n" \
                 "     Trim user membership:              %s\r\n" \
                 "     NSS group members from cache only: %s\r\n" \
                 "     NSS user members from cache only:  %s\r\n" \
                 "     NSS enumeration enabled:           %s\r\n"
                 "     Domain Manager check domain online (secs):          %d\r\n"
                 "     Domain Manager unknown domain cache timeout (secs): %d",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 gpLsaAdProviderState->config.dwCacheReaperTimeoutSecs,
                 gpLsaAdProviderState->config.dwCacheEntryExpirySecs,
                 gpLsaAdProviderState->config.chSpaceReplacement,
                 gpLsaAdProviderState->config.chDomainSeparator,
                 gpLsaAdProviderState->config.bEnableEventLog ? "true" : "false",
                 pszMemberList ? pszMemberList : "        <No login restrictions specified>\r\n",
                 gpLsaAdProviderState->config.bShouldLogNetworkConnectionEvents ? "true" : "false",
                 gpLsaAdProviderState->config.bCreateK5Login ? "true" : "false",
                 gpLsaAdProviderState->config.bCreateHomeDir ? "true" : "false",
                 gpLsaAdProviderState->config.bLDAPSignAndSeal ? "true" : "false",
                 gpLsaAdProviderState->config.bAssumeDefaultDomain ? "true" : "false",
                 gpLsaAdProviderState->config.bSyncSystemTime ? "true" : "false",
                 gpLsaAdProviderState->config.bRefreshUserCreds ? "true" : "false",
                 gpLsaAdProviderState->config.dwMachinePasswordSyncLifetime,
                 LSA_SAFE_LOG_STRING(gpLsaAdProviderState->config.pszShell),
                 LSA_SAFE_LOG_STRING(gpLsaAdProviderState->config.pszHomedirPrefix),
                 LSA_SAFE_LOG_STRING(gpLsaAdProviderState->config.pszHomedirTemplate),
                 gpLsaAdProviderState->config.dwUmask,
                 LSA_SAFE_LOG_STRING(gpLsaAdProviderState->config.pszSkelDirs),
                 gpLsaAdProviderState->config.CellSupport == AD_CELL_SUPPORT_UNINITIALIZED ? "Uninitialized" :
                 gpLsaAdProviderState->config.CellSupport == AD_CELL_SUPPORT_FULL ? "Full" :
                 gpLsaAdProviderState->config.CellSupport == AD_CELL_SUPPORT_FILE ? "File" :
                 gpLsaAdProviderState->config.CellSupport == AD_CELL_SUPPORT_UNPROVISIONED ? "Unprovisioned" : "Invalid",
                 gpLsaAdProviderState->config.bTrimUserMembershipEnabled ? "true" : "false",
                 gpLsaAdProviderState->config.bNssGroupMembersCacheOnlyEnabled ? "true" : "false",
                 gpLsaAdProviderState->config.bNssUserMembershipCacheOnlyEnabled ? "true" : "false",
                 gpLsaAdProviderState->config.bNssEnumerationEnabled ? "true" : "false",
                 gpLsaAdProviderState->config.DomainManager.dwCheckDomainOnlineSeconds,
                 gpLsaAdProviderState->config.DomainManager.dwUnknownDomainCacheTimeoutSeconds);
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
             LSASS_EVENT_INFO_SERVICE_CONFIGURATION_CHANGED,
             SERVICE_EVENT_CATEGORY,
             pszDescription,
             NULL);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);
    LSA_SAFE_FREE_STRING(pszMemberList);

    return;

error:

    goto cleanup;
}

static
VOID
LsaAdProviderLogRequireMembershipOfChangeEvent(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;
    PLSA_HASH_TABLE pAllowedMemberList = NULL;
    LSA_HASH_ITERATOR hashIterator = {0};
    LSA_HASH_ENTRY *pHashEntry = NULL;
    PSTR pszMemberList = NULL;
    DWORD i = 0;

    dwError = AD_ResolveConfiguredLists(
                  hProvider,
                  &pAllowedMemberList);
    BAIL_ON_LSA_ERROR(dwError);

    if (pAllowedMemberList != NULL)
    {
        dwError = LsaHashGetIterator(pAllowedMemberList, &hashIterator);
        BAIL_ON_LSA_ERROR(dwError);

        for (i = 0; (pHashEntry = LsaHashNext(&hashIterator)) != NULL; i++)
        {
            PSTR pszNewMemberList = NULL;

            dwError = LsaAllocateStringPrintf(
                         &pszNewMemberList,
                         "%s        %s\r\n",
                         pszMemberList ? pszMemberList : "",
                         LSA_SAFE_LOG_STRING(pHashEntry->pValue));
            BAIL_ON_LSA_ERROR(dwError);

            LSA_SAFE_FREE_STRING(pszMemberList);
            pszMemberList = pszNewMemberList;
            pszNewMemberList = NULL;
        }
    }
    else
    {
            dwError = LsaAllocateStringPrintf(
                         &pszMemberList,
                         "        <No login restrictions specified>\r\n");
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider login restriction settings have been reloaded.\r\n\r\n" \
                 "     Authentication provider:           %s\r\n" \
                 "     Current settings are...\r\n" \
                 "     require-membership-of:\r\n%s",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 LSA_SAFE_LOG_STRING(pszMemberList));
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
             LSASS_EVENT_INFO_REQUIRE_MEMBERSHIP_OF_UPDATED,
             SERVICE_EVENT_CATEGORY,
             pszDescription,
             NULL);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);
    LSA_SAFE_FREE_STRING(pszMemberList);
    LsaHashSafeFree(&pAllowedMemberList);

    return;

error:

    goto cleanup;
}

static
VOID
LsaAdProviderLogEventLogEnableChangeEvent(
    VOID
    )
{
    DWORD dwError = 0;
    PSTR pszDescription = NULL;

    dwError = LsaAllocateStringPrintf(
                 &pszDescription,
                 "Likewise authentication service provider auditing settings have been updated.\r\n\r\n" \
                 "     Authentication provider:           %s\r\n" \
                 "     Current settings are...\r\n" \
                 "     Enable event log:                  %s\r\n",
                 LSA_SAFE_LOG_STRING(gpszADProviderName),
                 gpLsaAdProviderState->config.bEnableEventLog ? "true" : "false");
    BAIL_ON_LSA_ERROR(dwError);

    LsaSrvLogServiceSuccessEvent(
             gpLsaAdProviderState->config.bEnableEventLog ?
                 LSASS_EVENT_INFO_AUDITING_CONFIGURATION_ENABLED :
                 LSASS_EVENT_INFO_AUDITING_CONFIGURATION_DISABLED,
             SERVICE_EVENT_CATEGORY,
             pszDescription,
             NULL);

cleanup:

    LSA_SAFE_FREE_STRING(pszDescription);

    return;

error:

    goto cleanup;
}

static
DWORD
AD_ResolveConfiguredLists(
    HANDLE          hProvider,
    PLSA_HASH_TABLE *ppAllowedMemberList
    )
{
    DWORD dwError = 0;
    DWORD iMember = 0;
    PSTR* ppszMembers = 0;
    DWORD dwNumMembers = 0;
    PLSA_HASH_TABLE pAllowedMemberList = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    PLSA_SECURITY_OBJECT pGroupInfo = NULL;
    PLSA_SECURITY_IDENTIFIER pSID = NULL;
    DWORD dwInfoLevel = 0;

    dwError = AD_GetMemberLists(
                    &ppszMembers,
                    &dwNumMembers,
                    &pAllowedMemberList);
    BAIL_ON_LSA_ERROR(dwError);

    for (iMember = 0; iMember < dwNumMembers; iMember++)
    {
        PSTR pszMember = *(ppszMembers + iMember);

        LSA_LOG_VERBOSE("Resolving entry [%s] for restricted login", pszMember);

        if (AD_STR_IS_SID(pszMember))
        {
            dwError = LsaAllocSecurityIdentifierFromString(
                            pszMember,
                            &pSID);
            if (dwError)
            {
                LSA_LOG_ERROR("Removing invalid SID entry [%s] from required membership list", pszMember);

                AD_DeleteFromMembersList(pszMember);
            }
            else
            {
                LSA_LOG_VERBOSE("Adding entry to allow login for SID [%s]", pszMember);

                dwError = AD_AddAllowedMember(
                              pszMember,
                              pszMember,
                              &pAllowedMemberList);
                BAIL_ON_LSA_ERROR(dwError);

                if (pSID)
                {
                    LsaFreeSecurityIdentifier(pSID);
                    pSID = NULL;
                }
            }
        }
        else // User or Group Name
        {
            dwError = AD_FindUserByName(
                            hProvider,
                            pszMember,
                            dwInfoLevel,
                            (PVOID*)&pUserInfo);
            if (dwError == LSA_ERROR_SUCCESS)
            {
                LSA_LOG_VERBOSE("Adding entry to allow login for user [%s]", pszMember);

                dwError = AD_AddAllowedMember(
                              pUserInfo->pszSid,
                              pszMember,
                              &pAllowedMemberList);
                BAIL_ON_LSA_ERROR(dwError);

                LsaFreeUserInfo(dwInfoLevel, pUserInfo);
                pUserInfo = NULL;

                continue;
            }
            dwError = LSA_ERROR_SUCCESS;

            ADCacheSafeFreeObject(&pGroupInfo);
            dwError = AD_FindGroupObjectByName(
                            hProvider,
                            pszMember,
                            &pGroupInfo);
            if (dwError == LSA_ERROR_SUCCESS)
            {
                LSA_LOG_VERBOSE("Adding entry to allow login for group [%s]", pszMember);

                dwError = AD_AddAllowedMember(
                              pGroupInfo->pszObjectSid,
                              pszMember,
                              &pAllowedMemberList);
                BAIL_ON_LSA_ERROR(dwError);

                continue;
            }
            LSA_LOG_WARNING("Restricted login list - couldn't resolve %s [%u]",
                            pszMember, dwError);
            dwError = LSA_ERROR_SUCCESS;
        }
    }

    *ppAllowedMemberList = (PVOID)pAllowedMemberList;

cleanup:

    if (ppszMembers)
    {
        LsaFreeStringArray(ppszMembers, dwNumMembers);
    }

    if (pSID)
    {
        LsaFreeSecurityIdentifier(pSID);
    }

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwInfoLevel, pUserInfo);
    }

    ADCacheSafeFreeObject(&pGroupInfo);

    return dwError;

error:

    *ppAllowedMemberList = NULL;

    LsaHashSafeFree(&pAllowedMemberList);

    goto cleanup;
}

static
DWORD
AD_SetUserCanonicalNameToAlias(
    PCSTR pszCurrentNetBIOSDomainName,
    DWORD dwUserInfoLevel,
    PVOID pUserInfo)
{
    DWORD dwError = 0;
    PSTR  pszCanonicalName = NULL;
    DWORD dwDomainNameLen = 0;

    BAIL_ON_INVALID_STRING(pszCurrentNetBIOSDomainName);

    dwDomainNameLen = strlen(pszCurrentNetBIOSDomainName);

    switch (dwUserInfoLevel)
    {
        case 0:
            {
                PLSA_USER_INFO_0 pUserInfo0 = (PLSA_USER_INFO_0)pUserInfo;
                pszCanonicalName = pUserInfo0->pszName;
            }

            break;

        case 1:
            {
                PLSA_USER_INFO_1 pUserInfo1 = (PLSA_USER_INFO_1)pUserInfo;
                pszCanonicalName = pUserInfo1->pszName;
            }

            break;

        case 2:
            {
                PLSA_USER_INFO_2 pUserInfo2 = (PLSA_USER_INFO_2)pUserInfo;
                pszCanonicalName = pUserInfo2->pszName;
            }

            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);

            break;
    }

    if (pszCanonicalName)
    {
        dwError = AD_SetCanonicalNameToAlias(
                        pszCurrentNetBIOSDomainName,
                        pszCanonicalName);
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    return dwError;
}

static
DWORD
AD_SetGroupCanonicalNamesToAliases(
    PCSTR pszCurrentNetBIOSDomainName,
    DWORD dwGroupInfoLevel,
    PVOID pGroupInfo)
{
    DWORD dwError = 0;
    PSTR  pszCanonicalName = NULL;
    DWORD dwDomainNameLen = 0;
    PSTR* ppszMembers = NULL;

    BAIL_ON_INVALID_STRING(pszCurrentNetBIOSDomainName);

    dwDomainNameLen = strlen(pszCurrentNetBIOSDomainName);

    switch (dwGroupInfoLevel)
    {
        case 0:
            {
                PLSA_GROUP_INFO_0 pGroupInfo0 = (PLSA_GROUP_INFO_0)pGroupInfo;
                pszCanonicalName = pGroupInfo0->pszName;
            }

            break;

        case 1:
            {
                PLSA_GROUP_INFO_1 pGroupInfo1 = (PLSA_GROUP_INFO_1)pGroupInfo;
                pszCanonicalName = pGroupInfo1->pszName;
                ppszMembers = pGroupInfo1->ppszMembers;
            }

            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);

            break;
    }

    if (pszCanonicalName)
    {
        dwError = AD_SetCanonicalNameToAlias(
                        pszCurrentNetBIOSDomainName,
                        pszCanonicalName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppszMembers)
    {
       for (; ppszMembers && !IsNullOrEmptyString(*ppszMembers); ppszMembers++)
       {
           dwError = AD_SetCanonicalNameToAlias(
                       pszCurrentNetBIOSDomainName,
                       *ppszMembers);
           BAIL_ON_LSA_ERROR(dwError);
       }
    }

error:

    return dwError;
}

static
DWORD
AD_SetCanonicalNameToAlias(
    PCSTR pszCurrentNetBIOSDomainName,
    PSTR  pszCanonicalName
    )
{
    DWORD dwError = 0;
    DWORD dwDomainNameLen = 0;

    BAIL_ON_INVALID_STRING(pszCurrentNetBIOSDomainName);

    dwDomainNameLen = strlen(pszCurrentNetBIOSDomainName);

    if (pszCanonicalName &&
        !strncasecmp(pszCanonicalName, pszCurrentNetBIOSDomainName, dwDomainNameLen) &&
        (*(pszCanonicalName + dwDomainNameLen) == LsaGetDomainSeparator()) &&
        (!IsNullOrEmptyString(pszCanonicalName + dwDomainNameLen + 1)))
    {
        PCSTR pszIndex = pszCanonicalName + dwDomainNameLen + 1;

        while (!IsNullOrEmptyString(pszIndex))
        {
            *pszCanonicalName++ = *pszIndex++;
        }
        *pszCanonicalName = '\0';
    }

error:

    return dwError;
}

void
InitADCacheFunctionTable(
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheProviderTable
    )
{
    InitializeDbCacheProvider(
        pCacheProviderTable
        );
    return;
}





/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
