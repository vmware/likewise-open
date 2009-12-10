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

static
DWORD
AD_JoinDomain(
    HANDLE  hProvider,
    uid_t   peerUID,
    gid_t   peerGID,
    DWORD   dwInputBufferSize,
    PVOID   pInputBuffer,
    DWORD* pdwOutputBufferSize,
    PVOID* ppOutputBuffer
    );

static
DWORD
AD_TransitionJoined(
    PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_Deactivate(
    PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_TransitionNotJoined(
    PLSA_AD_PROVIDER_STATE pState
    );

void
InitADCacheFunctionTable(
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheProviderTable
    );

static
VOID
LsaAdProviderStateAcquireRead(
    PLSA_AD_PROVIDER_STATE pState
    );

static
VOID
LsaAdProviderStateAcquireWrite(
    PLSA_AD_PROVIDER_STATE pState
    );

static
void
LsaAdProviderStateRelease(
    PLSA_AD_PROVIDER_STATE pState
    );

static
void*
LsaAdStartupThread(
    void* pData
    );

static
DWORD
AD_Activate(
    PLSA_AD_PROVIDER_STATE pState
    );

static
DWORD
AD_InitializeProvider(
    OUT PCSTR* ppszProviderName,
    OUT PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable
    )
{
    DWORD dwError = 0;
    LSA_AD_CONFIG config = {0};
    pthread_t startThread;

    pthread_rwlock_init(&gADGlobalDataLock, NULL);

    dwError = LsaAdProviderStateCreate(&gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_NetInitMemory();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_InitializeConfig(&config);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_ReadRegistry(&config);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_TransferConfigContents(
                    &config,
                    &gpLsaAdProviderState->config);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSetDomainSeparator(
                gpLsaAdProviderState->config.chDomainSeparator);
    BAIL_ON_LSA_ERROR(dwError);

    LsaAdProviderLogConfigReloadEvent();

    InitADCacheFunctionTable(gpCacheProvider);

    dwError = LwKrb5SetProcessDefaultCachePath(LSASS_CACHE_PATH);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_OpenDb(
                &gpLsaAdProviderState->hStateConnection);
    BAIL_ON_LSA_ERROR(dwError);

    switch (gpLsaAdProviderState->config.CacheBackend)
    {
        default:
#ifdef AD_CACHE_ENABLE_SQLITE
        case AD_CACHE_SQLITE:
            dwError = ADCacheOpen(
                        LSASS_AD_SQLITE_CACHE_DB,
                        &gpLsaAdProviderState->hCacheConnection);
            BAIL_ON_LSA_ERROR(dwError);
            break;
#endif
        case AD_CACHE_IN_MEMORY:
            dwError = ADCacheOpen(
                            LSASS_AD_MEMORY_CACHE_DB,
                            &gpLsaAdProviderState->hCacheConnection);
            BAIL_ON_LSA_ERROR(dwError);
            dwError = MemCacheSetSizeCap(
                            gpLsaAdProviderState->hCacheConnection,
                            AD_GetCacheSizeCap());
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    dwError = ADUnprovPlugin_Initialize();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_create(
                                      &startThread,
                                      NULL,
                                      LsaAdStartupThread,
                                      gpLsaAdProviderState));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_detach(startThread));
    BAIL_ON_LSA_ERROR(dwError);

    *ppszProviderName = gpszADProviderName;
    *ppFunctionTable = &gADProviderAPITable;

cleanup:

    AD_FreeConfigContents(&config);

    return dwError;

error:

    LsaAdProviderStateDestroy(gpLsaAdProviderState);
    gpLsaAdProviderState = NULL;

    if (gpADProviderData)
    {
        ADProviderFreeProviderData(gpADProviderData);
        gpADProviderData = NULL;
    }

    *ppszProviderName = NULL;
    *ppFunctionTable = NULL;

    goto cleanup;
}

static
void*
LsaAdStartupThread(
    void* pData
    )
{
    DWORD dwError = 0;
    PLSA_AD_PROVIDER_STATE pState = (PLSA_AD_PROVIDER_STATE) pData;

    LsaAdProviderStateAcquireWrite(pState);

    dwError = LwKrb5GetMachineCreds(NULL, NULL, NULL, NULL);
    if (dwError == 0)
    {
        dwError = AD_Activate(pState);
        if (dwError == 0)
        {
            pState->joinState = LSA_AD_JOINED;
        }
    }
    else
    {
        pState->joinState = LSA_AD_NOT_JOINED;
    }

    LsaAdProviderStateRelease(pState);

    return NULL;
}

DWORD
AD_ShutdownProvider(
    VOID
    )
{
    DWORD dwError = 0;

    LsaAdProviderStateAcquireWrite(gpLsaAdProviderState);
    if (gpLsaAdProviderState->joinState == LSA_AD_JOINED)
    {
        AD_Deactivate(gpLsaAdProviderState);
    }
    LsaAdProviderStateRelease(gpLsaAdProviderState);

    ADUnprovPlugin_Cleanup();

    dwError = AD_NetShutdownMemory();
    if (dwError)
    {
        LSA_LOG_DEBUG("AD Provider Shutdown: Failed to shutdown net memory (error = %d)", dwError);
        dwError = 0;
    }

    AD_FreeAllowedSIDs_InLock();

    // This will clean up media sense too.
    LsaAdProviderStateDestroy(gpLsaAdProviderState);
    gpLsaAdProviderState = NULL;

    return dwError;
}

static
DWORD
AD_Activate(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    PSTR  pszHostname = NULL;
    PSTR  pszUsername = NULL;
    PSTR  pszPassword = NULL;
    PSTR  pszDomainDnsName = NULL;
    PSTR  pszHostDnsDomain = NULL;
    BOOLEAN bIsDomainOffline = FALSE;

    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);

    LwStrToUpper(pszHostname);

    dwError = LwKrb5GetMachineCreds(
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
    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        bIsDomainOffline = TRUE;
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsDomainOffline)
    {
        dwError = AD_MachineCredentialsCacheInitialize();
        if (dwError == LW_ERROR_CLOCK_SKEW)
        {
            bIsDomainOffline = TRUE;
            dwError = LW_ERROR_SUCCESS;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (bIsDomainOffline)
    {
        dwError = AD_MachineCredentialsCacheClear();
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_InitializeOperatingMode(
                pszDomainDnsName,
                pszUsername,
                bIsDomainOffline);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaUmInitialize();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADInitMachinePasswordSync();
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

cleanup:

    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszUsername);
    LW_SAFE_CLEAR_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszDomainDnsName);
    LW_SAFE_FREE_STRING(pszHostDnsDomain);

    return dwError;

error:

    if (AD_EventlogEnabled())
    {
        LsaAdProviderLogServiceStartEvent(
                           pszHostname,
                           pszDomainDnsName,
                           bIsDomainOffline,
                           dwError);
    }

    ADShutdownMachinePasswordSync();

    LsaDmCleanup();

    goto cleanup;
}

static
DWORD
AD_TransitionJoined(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;


    dwError = AD_Activate(pState);
    BAIL_ON_LSA_ERROR(dwError);

    pState->joinState = LSA_AD_JOINED;

error:

    return dwError;
}

static
DWORD
AD_Deactivate(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    ADShutdownMachinePasswordSync();
    AD_MachineCredentialsCacheClear();

    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (gpADProviderData)
    {
        ADProviderFreeProviderData(gpADProviderData);
        gpADProviderData = NULL;
    }

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (pState->MediaSenseHandle)
    {
        MediaSenseStop(&pState->MediaSenseHandle);
        pState->MediaSenseHandle = NULL;
    }

    LsaUmCleanup();

    LsaDmCleanup();

    return dwError;
}

static
DWORD
AD_TransitionNotJoined(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    dwError = ADCacheEmptyCache(pState->hCacheConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADState_EmptyDb(pState->hStateConnection);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_Deactivate(pState);
    BAIL_ON_LSA_ERROR(dwError);

    pState->joinState = LSA_AD_NOT_JOINED;

error:

    return dwError;
}

DWORD
AD_OpenHandle(
    uid_t   uid,
    gid_t   gid,
    pid_t   pid,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PAD_PROVIDER_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(
                    sizeof(AD_PROVIDER_CONTEXT),
                    (PVOID*)&pContext);
    BAIL_ON_LSA_ERROR(dwError);

    pContext->uid = uid;
    pContext->gid = gid;
    pContext->pid = pid;

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
        LwFreeMemory(pContext);
    }
}

BOOLEAN
AD_ServicesDomain(
    PCSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        goto cleanup;
    }

    //
    // Added Trusted domains support
    //
    if (LW_IS_NULL_OR_EMPTY_STR(pszDomain) ||
        LW_IS_NULL_OR_EMPTY_STR(gpADProviderData->szDomain) ||
        LW_IS_NULL_OR_EMPTY_STR(gpADProviderData->szShortDomain)) {
       goto cleanup;
    }

    bResult = LsaDmIsDomainPresent(pszDomain);
    if (!bResult)
    {
        LSA_LOG_INFO("AD_ServicesDomain was passed unknown domain '%s'", pszDomain);
    }

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline())
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineAuthenticateUser(
            hProvider,
            pszLoginId,
            pszPassword);
    }

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineAuthenticateUser(
            hProvider,
            pszLoginId,
            pszPassword);
    }

error:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;
}

DWORD
AD_AuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
    DWORD dwError = LW_ERROR_INTERNAL;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserParams->pszDomain)
    {
        BOOLEAN bFoundDomain = FALSE;

        dwError = AD_ServicesDomainWithDiscovery(
                        pUserParams->pszDomain,
                        &bFoundDomain);
        BAIL_ON_LSA_ERROR(dwError);

        if (!bFoundDomain)
        {
            dwError = LW_ERROR_NOT_HANDLED;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    dwError = LsaDmWrapAuthenticateUserEx(
                      gpADProviderData->szDomain,
                      pUserParams,
                      ppUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;

error:
    /* On this one, it is a good idea to fallback to
       the local provider */

    if (dwError == LW_ERROR_RPC_NETLOGON_FAILED) {
        dwError = LW_ERROR_NOT_HANDLED;
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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    gpADProviderData->szDomain,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (!AD_ServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LW_ERROR_NOT_HANDLED;
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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    ADCacheSafeFreeObject(&pInObjectForm);

    return dwError;

error:

    if ((dwError == LW_ERROR_DUPLICATE_USERNAME ||
         dwError == LW_ERROR_DUPLICATE_USER_OR_GROUP)
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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline())
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineFindUserObjectById(
                    hProvider,
                    uid,
                    ppResult);
    }

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineFindUserObjectById(
                    hProvider,
                    uid,
                    ppResult);
    }

error:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_CreateUserState(
                        hProvider,
                        dwInfoLevel,
                        FindFlags,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);

    LwInitCookie(&pEnumState->Cookie);

    *phResume = (HANDLE)pEnumState;

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
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
    if ( dwError == LW_ERROR_NOT_HANDLED )
    {
        // no more results found
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if ( dwObjectCount == request->dwMaxNumUsers )
    {
        dwError = LwAllocateString(
                      ppUserObjectList[dwObjectCount - 1]->pszObjectSid,
                      &response.pszResume);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ( dwObjectCount )
    {
        dwError = LwAllocateMemory(sizeof(*ppUserInfoList) * dwObjectCount,
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
                dwError = LW_ERROR_INVALID_PARAMETER;
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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    if ( context )
    {
        lwmsg_context_delete(context);
    }

    LW_SAFE_FREE_STRING(response.pszResume);

    return dwError;

error:

    *pdwOutputBufferSize = 0;
    *ppOutputBuffer = NULL;

    if ( pBlob )
    {
        LwFreeMemory(pBlob);
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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!strcasecmp(pszLoginId, "root"))
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_RemoveUserByNameFromCacheInternal(
                  hProvider,
                  pszLoginId);
    if (dwError == LW_ERROR_NO_SUCH_USER &&
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
            dwError = LW_ERROR_NO_SUCH_USER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    LW_SAFE_FREE_STRING(pszLocalLoginId);
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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    ADCacheSafeFreeObject(&pUserInfo);

    return dwError;

error:

    if ((dwError == LW_ERROR_DUPLICATE_USERNAME ||
         dwError == LW_ERROR_DUPLICATE_USER_OR_GROUP)
        && AD_EventlogEnabled())
    {
        LsaSrvLogUserIDConflictEvent(
            uid,
            gpszADProviderName,
            dwError);
    }

    goto cleanup;
}

static
DWORD
AD_PreJoinDomain(
    HANDLE hProvider,
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    switch (pState->joinState)
    {
    case LSA_AD_UNKNOWN:
    case LSA_AD_NOT_JOINED:
        break;
    case LSA_AD_JOINED:
        dwError = AD_TransitionNotJoined(pState);
        BAIL_ON_LSA_ERROR(dwError);
        break;
    }

error:

    return dwError;
}

static
DWORD
AD_PostJoinDomain(
    HANDLE hProvider,
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    dwError = AD_TransitionJoined(pState);
    BAIL_ON_LSA_ERROR(dwError);

error:

    return dwError;
}


static
DWORD
AD_JoinDomain(
    HANDLE  hProvider,
    uid_t   peerUID,
    gid_t   peerGID,
    DWORD   dwInputBufferSize,
    PVOID   pInputBuffer,
    DWORD* pdwOutputBufferSize,
    PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    LWMsgDataContext* pDataContext = NULL;
    PLSA_AD_IPC_JOIN_DOMAIN_REQ pRequest = NULL;
    PSTR pszMessage = NULL;
    BOOLEAN bLocked = FALSE;

    if (peerUID != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(NULL, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetJoinDomainReqSpec(),
                                  pInputBuffer,
                                  (size_t) dwInputBufferSize,
                                  OUT_PPVOID(&pRequest)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_print_graph_alloc(
                                  pDataContext,
                                  LsaAdIPCGetJoinDomainReqSpec(),
                                  pRequest,
                                  &pszMessage));
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_TRACE("Domain join request: %s", pszMessage);

    LsaAdProviderStateAcquireWrite(gpLsaAdProviderState);
    bLocked = TRUE;

    dwError = AD_PreJoinDomain(hProvider, gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNetJoinDomain(
        pRequest->pszHostname,
        pRequest->pszHostDnsDomain,
        pRequest->pszDomain,
        pRequest->pszOU,
        pRequest->pszUsername,
        pRequest->pszPassword,
        pRequest->pszOSName,
        pRequest->pszOSVersion,
        pRequest->pszOSServicePack,
        pRequest->dwFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_PostJoinDomain(hProvider, gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_INFO("Joined domain: %s", pRequest->pszDomain);

cleanup:

    if (bLocked)
    {
        LsaAdProviderStateRelease(gpLsaAdProviderState);
    }

    LW_SAFE_FREE_MEMORY(pszMessage);

    if (pRequest)
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetJoinDomainReqSpec(),
            pRequest);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_PreLeaveDomain(
    HANDLE hProvider,
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    switch (pState->joinState)
    {
    case LSA_AD_UNKNOWN:
    case LSA_AD_NOT_JOINED:
        dwError = LW_ERROR_NOT_JOINED_TO_AD;
        BAIL_ON_LSA_ERROR(dwError);
        break;
    case LSA_AD_JOINED:
        AD_TransitionNotJoined(pState);
        break;
    }

error:

    return dwError;
}

static
DWORD
AD_PostLeaveDomain(
    HANDLE hProvider,
    PLSA_AD_PROVIDER_STATE pState
    )
{
    DWORD dwError = 0;

    return dwError;
}

static
DWORD
AD_LeaveDomain(
    HANDLE  hProvider,
    uid_t   peerUID,
    gid_t   peerGID,
    DWORD   dwInputBufferSize,
    PVOID   pInputBuffer,
    DWORD* pdwOutputBufferSize,
    PVOID* ppOutputBuffer
    )
{
    DWORD dwError = 0;
    LWMsgDataContext* pDataContext = NULL;
    PLSA_AD_IPC_LEAVE_DOMAIN_REQ pRequest = NULL;
    PSTR pszMessage = NULL;
    BOOLEAN bLocked = FALSE;

    if (peerUID != 0)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_data_context_new(NULL, &pDataContext));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_unmarshal_flat(
                                  pDataContext,
                                  LsaAdIPCGetLeaveDomainReqSpec(),
                                  pInputBuffer,
                                  (size_t) dwInputBufferSize,
                                  OUT_PPVOID(&pRequest)));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_data_print_graph_alloc(
                                  pDataContext,
                                  LsaAdIPCGetLeaveDomainReqSpec(),
                                  pRequest,
                                  &pszMessage));
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_TRACE("Domain leave request: %s", pszMessage);

    LsaAdProviderStateAcquireWrite(gpLsaAdProviderState);
    bLocked = TRUE;

    dwError = AD_PreLeaveDomain(hProvider, gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaNetLeaveDomain(
        pRequest->pszUsername,
        pRequest->pszPassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_PostLeaveDomain(hProvider, gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_LOG_INFO("Left domain\n");

cleanup:

    if (bLocked)
    {
        LsaAdProviderStateRelease(gpLsaAdProviderState);
    }

    LW_SAFE_FREE_MEMORY(pszMessage);

    if (pRequest)
    {
        lwmsg_data_free_graph(
            pDataContext,
            LsaAdIPCGetLeaveDomainReqSpec(),
            pRequest);
    }

    if (pDataContext)
    {
        lwmsg_data_context_delete(pDataContext);
    }

    return dwError;

error:

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
    DWORD dwError = 0;
    BOOLEAN bIsCacheOnlyMode = FALSE;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsCacheOnlyMode = AD_GetNssGroupMembersCacheOnlyEnabled();
    }

    dwError = AD_FindGroupByNameWithCacheMode(
                hProvider,
                pszGroupName,
                bIsCacheOnlyMode,
                dwGroupInfoLevel,
                ppGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;
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
    DWORD dwError = LW_ERROR_SUCCESS;
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
    DWORD dwError = LW_ERROR_SUCCESS;

    if (AD_IsOffline())
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
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

    if (dwError == LW_ERROR_DOMAIN_IS_OFFLINE)
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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsCacheOnlyMode = AD_GetNssGroupMembersCacheOnlyEnabled();
    }

    if (AD_IsOffline())
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
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

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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
    DWORD                 dwIndex = 0;
    DWORD                 dwInfoCount = 0;
    PLSA_SECURITY_OBJECT* ppGroupObjectList = NULL;
    PVOID*                ppGroupInfoList = NULL;
    PVOID                 pBlob = NULL;
    size_t                BlobSize;
    LWMsgContext*         context = NULL;
    LWMsgDataContext*      pDataContext = NULL;
    PLSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_REQ request = NULL;
    LSA_AD_IPC_ENUM_GROUPS_FROM_CACHE_RESP response;
    LSA_GROUP_INFO_LIST   result;

    memset(&response, 0, sizeof(response));
    memset(&result, 0, sizeof(result));

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
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
    if ( dwError == LW_ERROR_NOT_HANDLED )
    {
        // no more results found
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if ( dwObjectCount == request->dwMaxNumGroups )
    {
        dwError = LwAllocateString(
                      ppGroupObjectList[dwObjectCount - 1]->pszObjectSid,
                      &response.pszResume);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ( dwObjectCount )
    {
        dwError = LwAllocateMemory(sizeof(*ppGroupInfoList) * dwObjectCount,
                                    (PVOID*)&ppGroupInfoList);
        BAIL_ON_LSA_ERROR(dwError);

        // marshal the GroupInfoList data
        for (dwIndex = 0, dwInfoCount = 0; dwIndex < dwObjectCount; dwIndex++)
        {
            dwError = AD_GroupObjectToGroupInfo(
                          hProvider,
                          ppGroupObjectList[dwIndex],
                          TRUE,
                          request->dwInfoLevel,
                          &ppGroupInfoList[dwInfoCount]);
            if (dwError == LW_ERROR_OBJECT_NOT_ENABLED)
            {
                dwError = 0;
            }
            else
            {
                BAIL_ON_LSA_ERROR(dwError);

                dwInfoCount++;
            }

            ADCacheSafeFreeObject(&ppGroupObjectList[dwIndex]);
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
                dwError = LW_ERROR_INVALID_PARAMETER;
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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LW_SAFE_FREE_STRING(response.pszResume);

    return dwError;

error:

    *pdwOutputBufferSize = 0;
    *ppOutputBuffer = NULL;

    if ( pBlob )
    {
        LwFreeMemory(pBlob);
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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    LW_SAFE_FREE_STRING(pszFreeGroupName);
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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // restrict access to root
    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
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

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
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
            if (dwError == LW_ERROR_NO_SUCH_OBJECT)
            {
                dwError = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);
            break;
        default:
            dwError = LW_ERROR_INVALID_GROUP_INFO_LEVEL;
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
    LW_SAFE_FREE_STRING(pszFullDomainName);

    return dwError;

error:
    *ppGroupInfo = NULL;

    goto cleanup;
}


static
VOID
FreeHashEntryGroupInfoLevel0(
    IN OUT const LSA_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        LsaFreeGroupInfo(0, pEntry->pValue);
    }
}

static
VOID
FreeHashEntryGroupInfoLevel1(
    IN OUT const LSA_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        LsaFreeGroupInfo(1, pEntry->pValue);
    }
}

static
PCSTR
GetSidFromGroupInfo(
    IN DWORD dwGroupInfoLevel,
    IN PVOID pGroupInfo
    )
{
    switch (dwGroupInfoLevel)
    {
        case 0:
            return ((PLSA_GROUP_INFO_0)pGroupInfo)->pszSid;
        case 1:
            return ((PLSA_GROUP_INFO_1)pGroupInfo)->pszSid;
        default:
            // Should never happen
            return NULL;
    }
}

static
DWORD
CopyGroupsFromListToHash(
    IN OUT PLSA_HASH_TABLE pGroupHash,
    IN DWORD dwGroupInfoLevel,
    IN DWORD dwGroupsCount,
    IN PVOID* ppGroupInfoList
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PCSTR pszGroupSid = NULL;
    PVOID pGroupInfo = NULL;

    for (i = 0; i < dwGroupsCount; i++)
    {
        pszGroupSid = GetSidFromGroupInfo(dwGroupInfoLevel, ppGroupInfoList[i]);

        dwError = LsaHashGetValue(
                        pGroupHash,
                        pszGroupSid,
                        &pGroupInfo);
        if (dwError == ENOENT)
        {
            dwError = LsaAllocateGroupInfo(
                            &pGroupInfo,
                            dwGroupInfoLevel,
                            ppGroupInfoList[i]);
            BAIL_ON_LSA_ERROR(dwError);

            pszGroupSid = GetSidFromGroupInfo(dwGroupInfoLevel, pGroupInfo);

            dwError = LsaHashSetValue(
                            pGroupHash,
                            (PVOID) pszGroupSid,
                            pGroupInfo);
            BAIL_ON_LSA_ERROR(dwError);
        }

        pGroupInfo = NULL;
    }

cleanup:
    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;

error:
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
    size_t sIndex = 0;
    BOOLEAN bIsCacheOnlyMode = FALSE;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    DWORD dwLocalGroupsCount = 0;
    DWORD i = 0;
    PVOID pGroupInfo = NULL;
    PVOID* ppLocalGroupsInfo = NULL;
    PLSA_HASH_TABLE pUserMemberships = NULL;
    LSA_HASH_ITERATOR hashIterator = { 0 };
    LSA_HASH_ENTRY* pHashEntry = NULL;
    LSA_HASH_FREE_ENTRY pfnFreeHashEntry = NULL;
    DWORD dwNumGroupsFound = 0;
    PVOID* ppGroupInfoList = NULL;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (FindFlags & LSA_FIND_FLAGS_NSS)
    {
        bIsCacheOnlyMode = AD_GetNssUserMembershipCacheOnlyEnabled();
    }

    switch (dwGroupInfoLevel)
    {
        case 0:
            pfnFreeHashEntry = FreeHashEntryGroupInfoLevel0;
            break;
        case 1:
            pfnFreeHashEntry = FreeHashEntryGroupInfoLevel1;
            break;
        default:
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
    }

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

    // Get user's local groups
    dwError = LsaSrvGetGroupMembershipByProvider(
                hProvider,
                LSA_PROVIDER_TAG_LOCAL,
                pUserInfo->pszObjectSid,
                dwGroupInfoLevel,
                &dwLocalGroupsCount,
                &ppLocalGroupsInfo);
    if ((LW_ERROR_INVALID_AUTH_PROVIDER == dwError) ||
        (LW_ERROR_NO_SUCH_USER  == dwError))
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    13,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    pfnFreeHashEntry,
                    NULL,
                    &pUserMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    // Add user's direct local memberships to hash table
    dwError = CopyGroupsFromListToHash(
                    pUserMemberships,
                    dwGroupInfoLevel,
                    dwLocalGroupsCount,
                    ppLocalGroupsInfo);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // For each domain group for the user:
    //
    // 1) Convert the data into the appropriate info level.
    //
    // 2) Get the local groups to which that domain group belongs.
    //

    for (sIndex = 0; sIndex < sGroupObjectsCount; sIndex++)
    {
        PVOID singleGroupInfoArray[1] = { NULL };

        if (ppGroupObjects[sIndex]->type != AccountType_Group)
        {
            LSA_LOG_DEBUG("Skipping non-group SID %s (type = %d)",
                          LSA_SAFE_LOG_STRING(ppGroupObjects[sIndex]->pszObjectSid),
                          ppGroupObjects[sIndex]->type);
            continue;
        }

        if (pGroupInfo)
        {
            LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
            pGroupInfo = NULL;
        }

        dwError = AD_GroupObjectToGroupInfo(
                    hProvider,
                    ppGroupObjects[sIndex],
                    bIsCacheOnlyMode,
                    dwGroupInfoLevel,
                    &pGroupInfo);
        if (dwError == LW_ERROR_OBJECT_NOT_ENABLED)
        {
            // Filter this group from the list
            dwError = LW_ERROR_SUCCESS;
            continue;
        }
        BAIL_ON_LSA_ERROR(dwError);

        singleGroupInfoArray[0] = pGroupInfo;

        // Add domain memberships to hash table
        dwError = CopyGroupsFromListToHash(
                        pUserMemberships,
                        dwGroupInfoLevel,
                        1,
                        singleGroupInfoArray);
        BAIL_ON_LSA_ERROR(dwError);

        // Get domain group's local groups.  This handles domain
        // groups being a member of local groups.

        if (ppLocalGroupsInfo)
        {
            LsaFreeGroupInfoList(dwGroupInfoLevel,
                                 ppLocalGroupsInfo,
                                 dwLocalGroupsCount);
            ppLocalGroupsInfo = NULL;
            dwLocalGroupsCount = 0;
        }

        dwError = LsaSrvGetGroupMembershipByProvider(
                    hProvider,
                    LSA_PROVIDER_TAG_LOCAL,
                    ppGroupObjects[sIndex]->pszObjectSid,
                    dwGroupInfoLevel,
                    &dwLocalGroupsCount,
                    &ppLocalGroupsInfo);
        if ((LW_ERROR_INVALID_AUTH_PROVIDER == dwError) ||
            (LW_ERROR_NO_SUCH_USER  == dwError))
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        // Add domain group's local memberships to hash table
        dwError = CopyGroupsFromListToHash(
                        pUserMemberships,
                        dwGroupInfoLevel,
                        dwLocalGroupsCount,
                        ppLocalGroupsInfo);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaHashGetIterator(pUserMemberships,
                                 &hashIterator);
    BAIL_ON_LSA_ERROR(dwError);

    // TODO: Get a count directly from hash table abstraction/
    while (LsaHashNext(&hashIterator)) {
        dwNumGroupsFound++;
    }

    dwError = LwAllocateMemory(
                 sizeof(ppGroupInfoList[0]) * dwNumGroupsFound,
                 (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashGetIterator(pUserMemberships, &hashIterator);
    BAIL_ON_LSA_ERROR(dwError);

    for (i = 0; (pHashEntry = LsaHashNext(&hashIterator)) != NULL; i++)
    {
        ppGroupInfoList[i] = pHashEntry->pValue;
        // Since the data is moved, need to NULL it to avoid
        // double-free.
        pHashEntry->pValue = NULL;
        /* It is safe to remove the entry just returned by
           LsaHashNext() from the hash table because the
           iterator already points to the next item */
        LsaHashRemoveKey(pUserMemberships, pHashEntry->pKey);
    }

    if (AD_ShouldAssumeDefaultDomain())
    {
        DWORD iGroup = 0;

        for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
        {
            dwError = AD_SetGroupCanonicalNamesToAliases(
                            gpADProviderData->szShortDomain,
                            dwGroupInfoLevel,
                            ppGroupInfoList[iGroup]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
        pGroupInfo = NULL;
    }

    if (ppLocalGroupsInfo)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel,
                             ppLocalGroupsInfo,
                             dwLocalGroupsCount);
        ppLocalGroupsInfo = NULL;
        dwLocalGroupsCount = 0;
    }

    if (pUserMemberships)
    {
        dwError = LsaHashGetIterator(pUserMemberships, &hashIterator);
        assert(dwError == LW_ERROR_SUCCESS);

        while ((pHashEntry = LsaHashNext(&hashIterator)) != NULL)
        {
            LsaFreeGroupInfo(dwGroupInfoLevel, pHashEntry->pValue);
        }

        LsaHashSafeFree(&pUserMemberships);
    }

    ADCacheSafeFreeObjectList(sGroupObjectsCount, &ppGroupObjects);
    ADCacheSafeFreeObject(&pUserInfo);

    *pdwNumGroupsFound = dwNumGroupsFound;
    *pppGroupInfoList = ppGroupInfoList;

    return dwError;

error:
    if (ppGroupInfoList)
    {
        LsaFreeGroupInfoList(dwGroupInfoLevel,
                             ppGroupInfoList,
                             dwNumGroupsFound);
    }

    dwNumGroupsFound = 0;
    ppGroupInfoList = NULL;

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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
    DWORD dwError = 0;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline())
    {
        dwError = AD_OfflineChangePassword(
            hProvider,
            pszLoginId,
            pszPassword,
            pszOldPassword);
    }
    else
    {
        dwError = AD_OnlineChangePassword(
            hProvider,
            pszLoginId,
            pszPassword,
            pszOldPassword);
    }

error:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;
}

DWORD
AD_SetPassword(
    HANDLE hProvider,
    PCSTR pszLoginId,
    PCSTR pszPassword
    )
{
    return LW_ERROR_NOT_HANDLED;
}


DWORD
AD_AddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_ModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_DeleteUser(
    HANDLE hProvider,
    uid_t  uid
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_AddGroup(
    HANDLE hProvider,
    DWORD dwGroupInfoLevel,
    PVOID pGroupInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_ModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_DeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
AD_EmptyCache(
    IN HANDLE hProvider,
    IN uid_t  peerUID,
    IN gid_t  peerGID
    )
{
    DWORD dwError = 0;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (peerUID)
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheEmptyCache(
                  gpLsaAdProviderState->hCacheConnection);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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
    DWORD dwError = 0;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline())
    {
        dwError = AD_OfflineGetNamesBySidList(
            hProvider,
            sCount,
            ppszSidList,
            pppszDomainNames,
            pppszSamAccounts,
            ppTypes);
    }
    else
    {
        dwError = AD_OnlineGetNamesBySidList(
            hProvider,
            sCount,
            ppszSidList,
            pppszDomainNames,
            pppszSamAccounts,
            ppTypes);
    }

error:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;
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
    DWORD dwError = 0;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline())
    {
        dwError = AD_OfflineFindNSSArtefactByKey(
                        hProvider,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
    }
    else
    {
        dwError = AD_OnlineFindNSSArtefactByKey(
                        hProvider,
                        pszKeyName,
                        pszMapName,
                        dwInfoLevel,
                        dwFlags,
                        ppNSSArtefactInfo);
    }

error:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;
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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (!dwFlags)
    {
        dwError = LW_ERROR_INVALID_PARAMETER;
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

            dwError = LW_ERROR_NOT_SUPPORTED;
            break;
    }

    *phResume = (HANDLE)pEnumState;

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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
    DWORD dwError = 0;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }


    if (AD_IsOffline())
    {
        dwError = AD_OfflineEnumNSSArtefacts(
            hProvider,
            hResume,
            dwMaxNSSArtefacts,
            pdwNSSArtefactsFound,
            pppNSSArtefactInfoList);
    }
    else
    {
        dwError = AD_OnlineEnumNSSArtefacts(
            hProvider,
            hResume,
            dwMaxNSSArtefacts,
            pdwNSSArtefactsFound,
            pppNSSArtefactInfoList);
    }

error:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;
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

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(
                   sizeof(LSA_AUTH_PROVIDER_STATUS),
                   (PVOID*)&pProviderStatus);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateString(
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

            if (!LW_IS_NULL_OR_EMPTY_STR(gpADProviderData->cell.szCellDN))
            {
                dwError = LwAllocateString(
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

    if (!LW_IS_NULL_OR_EMPTY_STR(gpADProviderData->szDomain))
    {
        dwError = LwAllocateString(
                        gpADProviderData->szDomain,
                        &pProviderStatus->pszDomain);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LWNetGetDCName(
                        NULL,
                        gpADProviderData->szDomain,
                        NULL,
                        DS_BACKGROUND_ONLY,
                        &pDCInfo);
        if (ERROR_NO_SUCH_DOMAIN == dwError)
        {
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);

        if (pDCInfo)
        {
            if (!LW_IS_NULL_OR_EMPTY_STR(pDCInfo->pszDnsForestName))
            {
                dwError = LwAllocateString(
                                pDCInfo->pszDnsForestName,
                                &pProviderStatus->pszForest);
                BAIL_ON_LSA_ERROR(dwError);
            }

            if (!LW_IS_NULL_OR_EMPTY_STR(pDCInfo->pszDCSiteName))
            {
                dwError = LwAllocateString(
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

    LsaAdProviderStateRelease(gpLsaAdProviderState);

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

        dwError = LwAllocateMemory(
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

    dwError = LwStrDupOrNull(
                    pDomainInfo->pszDnsDomainName,
                    &pTrustedDomainInfo->pszDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
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

        dwError = LwAllocateString(
                        szGUID,
                        &pTrustedDomainInfo->pszDomainGUID);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwStrDupOrNull(
                    pDomainInfo->pszTrusteeDnsDomainName,
                    &pTrustedDomainInfo->pszTrusteeDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    pTrustedDomainInfo->dwTrustFlags = pDomainInfo->dwTrustFlags;
    pTrustedDomainInfo->dwTrustType = pDomainInfo->dwTrustType;
    pTrustedDomainInfo->dwTrustAttributes = pDomainInfo->dwTrustAttributes;
    pTrustedDomainInfo->dwTrustDirection = pDomainInfo->dwTrustDirection;
    pTrustedDomainInfo->dwTrustMode = pDomainInfo->dwTrustMode;

    dwError = LwStrDupOrNull(
                    pDomainInfo->pszForestName,
                    &pTrustedDomainInfo->pszForestName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
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
        if (ERROR_NO_SUCH_DOMAIN == dwError)
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
                dwError = LwStrDupOrNull(
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
        if (ERROR_NO_SUCH_DOMAIN == dwError)
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

    dwError = LwAllocateMemory(
                    sizeof(LSA_DC_INFO),
                    (PVOID*)&pDestDCInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pDCInfo->pszName,
                    &pDestDCInfo->pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
                    pDCInfo->pszAddress,
                    &pDestDCInfo->pszAddress);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwStrDupOrNull(
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
    LW_SAFE_FREE_STRING(pProviderStatus->pszId);
    LW_SAFE_FREE_STRING(pProviderStatus->pszDomain);
    LW_SAFE_FREE_STRING(pProviderStatus->pszForest);
    LW_SAFE_FREE_STRING(pProviderStatus->pszSite);
    LW_SAFE_FREE_STRING(pProviderStatus->pszCell);

    if (pProviderStatus->pTrustedDomainInfoArray)
    {
        LsaFreeDomainInfoArray(
                        pProviderStatus->dwNumTrustedDomains,
                        pProviderStatus->pTrustedDomainInfoArray);
    }

    LwFreeMemory(pProviderStatus);
}

DWORD
AD_RefreshConfiguration(
    HANDLE hProvider
    )
{
    DWORD dwError = 0;
    LSA_AD_CONFIG config = {0};
    BOOLEAN bInLock = FALSE;
    BOOLEAN bUpdateCap = FALSE;

    dwError = AD_InitializeConfig(&config);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_ReadRegistry(&config);
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

    if (gpLsaAdProviderState->config.CacheBackend == AD_CACHE_IN_MEMORY)
    {
        bUpdateCap = TRUE;
    }

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    if (bUpdateCap)
    {
        dwError = MemCacheSetSizeCap(
                        gpLsaAdProviderState->hCacheConnection,
                        AD_GetCacheSizeCap());
        BAIL_ON_LSA_ERROR(dwError);
    }
    LsaAdProviderLogConfigReloadEvent();
    LsaAdProviderLogRequireMembershipOfChangeEvent(hProvider);
    LsaAdProviderLogEventLogEnableChangeEvent();

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

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
        case LSA_AD_IO_JOINDOMAIN:
            dwError = AD_JoinDomain(
                          hProvider,
                          peerUID,
                          peerGID,
                          dwInputBufferSize,
                          pInputBuffer,
                          pdwOutputBufferSize,
                          ppOutputBuffer);
            break;
        case LSA_AD_IO_LEAVEDOMAIN:
            dwError = AD_LeaveDomain(
                          hProvider,
                          peerUID,
                          peerGID,
                          dwInputBufferSize,
                          pInputBuffer,
                          pdwOutputBufferSize,
                          ppOutputBuffer);
            break;
        default:
            dwError = LW_ERROR_NOT_HANDLED;
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

DWORD
AD_GetGroupMembershipByProvider(
    IN HANDLE     hProvider,
    IN PCSTR      pszSid,
    IN DWORD      dwGroupInfoLevel,
    OUT PDWORD    pdwGroupsCount,
    OUT PVOID   **pppMembershipInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
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
        dwError = LwAllocateString(
                        pszName,
                        &pszLocalName);
        BAIL_ON_LSA_ERROR(dwError);

        LwStrCharReplace(pszLocalName, chSeparator, ' ');

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
    LW_SAFE_FREE_STRING(pszLocalName);

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
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineFindUserObjectByName(
                        hProvider,
                        pszUseLoginId,
                        &pResult);
    }

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineFindUserObjectByName(
                        hProvider,
                        pszUseLoginId,
                        &pResult);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pResult;

cleanup:
    LW_SAFE_FREE_STRING(pszFreeLoginId);
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
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindUserObjectByNameInternal(
                hProvider,
                pszLoginId,
                &pResult);
    if (dwError == LW_ERROR_NO_SUCH_USER &&
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
            dwError = LW_ERROR_NO_SUCH_USER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pResult;

cleanup:

    LW_SAFE_FREE_STRING(pszLocalLoginId);
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

    LW_SAFE_FREE_STRING(pszFreeLoginId);
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
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineFindGroupObjectByName(
                        hProvider,
                        pszUseGroupName,
                        &pResult);
    }

    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
    {
        dwError = AD_OfflineFindGroupObjectByName(
                        hProvider,
                        pszUseGroupName,
                        &pResult);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pResult;

cleanup:
    LW_SAFE_FREE_STRING(pszFreeGroupName);
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
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_FindGroupObjectByNameInternal(
                hProvider,
                pszGroupName,
                &pResult);
    if (dwError == LW_ERROR_NO_SUCH_GROUP &&
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
            dwError = LW_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppResult = pResult;

cleanup:

    LW_SAFE_FREE_STRING(pszLocalGroupName);
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

static
VOID
AD_FilterBuiltinObjects(
    IN DWORD dwCount,
    PLSA_SECURITY_OBJECT* ppObjects
    )
{
    DWORD dwIndex = 0;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppObjects[dwIndex] &&
            AdIsSpecialDomainSidPrefix(ppObjects[dwIndex]->pszObjectSid))
        {
            ADCacheSafeFreeObject(&ppObjects[dwIndex]);
        }
    }
}

static
DWORD
AD_UpdateObject(
    IN OUT PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    struct timeval current_tv = {0};
    UINT64 u64current_NTtime = 0;
    int64_t qwNanosecsToPasswordExpiry = 0;

    switch(pObject->type)
    {
    case AccountType_User:
        if (gettimeofday(&current_tv, NULL) < 0)
        {
            dwError = errno;
            BAIL_ON_LSA_ERROR(dwError);
        }
        ADConvertTimeUnix2Nt(current_tv.tv_sec,
                             &u64current_NTtime);

        if (pObject->userInfo.bIsAccountInfoKnown)
        {
            if (pObject->userInfo.qwAccountExpires != 0LL &&
                pObject->userInfo.qwAccountExpires != 9223372036854775807LL &&
                u64current_NTtime >= pObject->userInfo.qwAccountExpires)
            {
                pObject->userInfo.bAccountExpired = TRUE;
            }

            if (gpADProviderData->adMaxPwdAge != 0)
            {
                pObject->userInfo.qwMaxPwdAge = gpADProviderData->adMaxPwdAge;

                qwNanosecsToPasswordExpiry = gpADProviderData->adMaxPwdAge -
                    (u64current_NTtime - pObject->userInfo.qwPwdLastSet);

                if ((!pObject->userInfo.bPasswordNeverExpires &&
                     gpADProviderData->adMaxPwdAge != 0 &&
                     qwNanosecsToPasswordExpiry < 0) ||
                    pObject->userInfo.qwPwdLastSet == 0)
                {
                    //password is expired already
                    pObject->userInfo.bPasswordExpired = TRUE;
                }
            }
        }

        if (!pObject->userInfo.pszUnixName)
        {
            dwError = ADMarshalGetCanonicalName(pObject, &pObject->userInfo.pszUnixName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        break;
    case AccountType_Group:
        if (!pObject->groupInfo.pszUnixName)
        {
            dwError = ADMarshalGetCanonicalName(pObject, &pObject->groupInfo.pszUnixName);
            BAIL_ON_LSA_ERROR(dwError);
        }
        break;
    default:
        break;
    }


cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
AD_UpdateObjects(
    IN DWORD dwCount,
    IN OUT PLSA_SECURITY_OBJECT* ppObjects)
{
    DWORD dwError = 0;
    DWORD dwIndex = 0;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        if (ppObjects[dwIndex])
        {
            dwError = AD_UpdateObject(ppObjects[dwIndex]);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

error:

    return dwError;
}

DWORD
AD_FindObjects(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN OPTIONAL LSA_OBJECT_TYPE ObjectType,
    IN LSA_QUERY_TYPE QueryType,
    IN DWORD dwCount,
    IN LSA_QUERY_LIST QueryList,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline())
    {
        dwError = AD_OfflineFindObjects(
            hProvider,
            FindFlags,
            ObjectType,
            QueryType,
            dwCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = AD_OnlineFindObjects(
            hProvider,
            FindFlags,
            ObjectType,
            QueryType,
            dwCount,
            QueryList,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (ppObjects)
    {
        dwError = AD_UpdateObjects(dwCount, ppObjects);
        BAIL_ON_LSA_ERROR(dwError);

        AD_FilterBuiltinObjects(dwCount, ppObjects);
    }

    *pppObjects = ppObjects;

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;

error:

    *pppObjects = NULL;

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwCount, ppObjects);
    }

    goto cleanup;
}

DWORD
AD_OpenEnumObjects(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN LSA_OBJECT_TYPE ObjectType,
    IN OPTIONAL PCSTR pszDomainName
    )
{
    DWORD dwError = 0;
    PAD_ENUM_HANDLE pEnum = NULL;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
    BAIL_ON_LSA_ERROR(dwError);

    pEnum->Type = AD_ENUM_HANDLE_OBJECTS;
    pEnum->FindFlags = FindFlags;
    pEnum->ObjectType = ObjectType;

    if (ObjectType == LSA_OBJECT_TYPE_UNDEFINED)
    {
        pEnum->CurrentObjectType = LSA_OBJECT_TYPE_USER;
    }
    else
    {
        pEnum->CurrentObjectType = ObjectType;
    }

    LwInitCookie(&pEnum->Cookie);

    *phEnum = pEnum;

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;

error:

    *phEnum = NULL;

    if (pEnum)
    {
        AD_CloseEnum(pEnum);
    }

    goto cleanup;
}

DWORD
AD_EnumObjects(
    IN HANDLE hEnum,
    IN DWORD dwMaxObjectsCount,
    OUT PDWORD pdwObjectsCount,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    DWORD dwObjectsCount = 0;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline())
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = AD_OnlineEnumObjects(
            hEnum,
            dwMaxObjectsCount,
            &dwObjectsCount,
            &ppObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = AD_UpdateObjects(dwObjectsCount, ppObjects);
    BAIL_ON_LSA_ERROR(dwError);


    *pdwObjectsCount = dwObjectsCount;
    *pppObjects = ppObjects;

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;

error:

    *pdwObjectsCount = 0;
    *pppObjects = NULL;

    if (ppObjects)
    {
        LsaUtilFreeSecurityObjectList(dwObjectsCount, ppObjects);
    }

    goto cleanup;
}

DWORD
AD_OpenEnumMembers(
    IN HANDLE hProvider,
    OUT PHANDLE phEnum,
    IN LSA_FIND_FLAGS FindFlags,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PAD_ENUM_HANDLE pEnum = NULL;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AdIsSpecialDomainSidPrefix(pszSid))
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateMemory(sizeof(*pEnum), OUT_PPVOID(&pEnum));
    BAIL_ON_LSA_ERROR(dwError);

    pEnum->Type = AD_ENUM_HANDLE_MEMBERS;
    pEnum->FindFlags = FindFlags;

    LwInitCookie(&pEnum->Cookie);


    if (AD_IsOffline())
    {
        dwError = AD_OfflineGetGroupMemberSids(
            hProvider,
            FindFlags,
            pszSid,
            &pEnum->dwSidCount,
            &pEnum->ppszSids);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = AD_OnlineGetGroupMemberSids(
            hProvider,
            FindFlags,
            pszSid,
            &pEnum->dwSidCount,
            &pEnum->ppszSids);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *phEnum = pEnum;

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;

error:

    *phEnum = NULL;

    if (pEnum)
    {
        AD_CloseEnum(pEnum);
    }

    goto cleanup;
}

DWORD
AD_EnumMembers(
    IN HANDLE hEnum,
    IN DWORD dwMaxMemberSidCount,
    OUT PDWORD pdwMemberSidCount,
    OUT PSTR** pppszMemberSids
    )
{
   DWORD dwError = 0;
   PAD_ENUM_HANDLE pEnum = hEnum;
   DWORD dwMemberSidCount = dwMaxMemberSidCount;
   PSTR* ppszMemberSids = NULL;

   if (dwMemberSidCount > pEnum->dwSidCount - pEnum->dwSidIndex)
   {
       dwMemberSidCount = pEnum->dwSidCount - pEnum->dwSidIndex;
   }

   if (dwMemberSidCount == 0)
   {
       dwError = ERROR_NO_MORE_ITEMS;
       BAIL_ON_LSA_ERROR(dwError);
   }

   dwError = LwAllocateMemory(
       sizeof(*ppszMemberSids) * dwMemberSidCount,
       OUT_PPVOID(&ppszMemberSids));
   BAIL_ON_LSA_ERROR(dwError);

   memcpy(ppszMemberSids, pEnum->ppszSids + pEnum->dwSidIndex, sizeof(*ppszMemberSids) * dwMemberSidCount);
   memset(pEnum->ppszSids, 0, sizeof(*ppszMemberSids) * dwMemberSidCount);

   pEnum->dwSidIndex += dwMemberSidCount;

   *pdwMemberSidCount = dwMemberSidCount;
   *pppszMemberSids = ppszMemberSids;

cleanup:

   return dwError;

error:

   if (ppszMemberSids)
   {
       LwFreeStringArray(ppszMemberSids, dwMemberSidCount);
   }

   goto cleanup;
}

DWORD
AD_QueryMemberOf(
    IN HANDLE hProvider,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwSidCount,
    IN PSTR* ppszSids,
    OUT PDWORD pdwGroupSidCount,
    OUT PSTR** pppszGroupSids
    )
{
    DWORD dwError = 0;

    LsaAdProviderStateAcquireRead(gpLsaAdProviderState);

    if (gpLsaAdProviderState->joinState != LSA_AD_JOINED)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (AD_IsOffline())
    {
        dwError = AD_OfflineQueryMemberOf(
            hProvider,
            FindFlags,
            dwSidCount,
            ppszSids,
            pdwGroupSidCount,
            pppszGroupSids);
    }
    else
    {
        dwError = AD_OnlineQueryMemberOf(
            hProvider,
            FindFlags,
            dwSidCount,
            ppszSids,
            pdwGroupSidCount,
            pppszGroupSids);
    }

cleanup:

    LsaAdProviderStateRelease(gpLsaAdProviderState);

    return dwError;

error:

    goto cleanup;
}

VOID
AD_CloseEnum(
    IN OUT HANDLE hEnum
    )
{
    PAD_ENUM_HANDLE pEnum = hEnum;
    if (pEnum)
    {
        LwFreeCookieContents(&pEnum->Cookie);
        if (pEnum->ppszSids)
        {
            LwFreeStringArray(pEnum->ppszSids, pEnum->dwSidCount);
        }
        LwFreeMemory(pEnum);
    }
}

DWORD
AD_InitializeOperatingMode(
    IN PCSTR pszDomain,
    IN PCSTR pszHostName,
    IN BOOLEAN bIsDomainOffline
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PAD_PROVIDER_DATA pProviderData = NULL;

    if (bIsDomainOffline || AD_IsOffline())
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
    }
    else
    {
        dwError = AD_OnlineInitializeOperatingMode(
                &pProviderData,
                pszDomain,
                pszHostName);
    }
    // If we are offline, do the offline case
    if (LW_ERROR_DOMAIN_IS_OFFLINE == dwError)
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

    dwError = LwAllocateMemory(sizeof(*pState), (PVOID)&pState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwMapErrnoToLwError(pthread_mutex_init(&pState->MachineCreds.Mutex, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    pState->MachineCreds.pMutex = &pState->MachineCreds.Mutex;

    dwError = AD_InitializeConfig(&pState->config);
    BAIL_ON_LSA_ERROR(dwError);

    pState->dwMaxAllowedClockDriftSeconds = AD_MAX_ALLOWED_CLOCK_DRIFT_SECONDS;

    dwError = LwMapErrnoToLwError(pthread_rwlock_init(&pState->stateLock, NULL));
    BAIL_ON_LSA_ERROR(dwError);

    pState->pStateLock = &pState->stateLock;

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

        if (pState->MediaSenseHandle)
        {
            MediaSenseStop(&pState->MediaSenseHandle);
            pState->MediaSenseHandle = NULL;
        }

        if (pState->MachineCreds.pMutex)
        {
            pthread_mutex_destroy(pState->MachineCreds.pMutex);
            pState->MachineCreds.pMutex = NULL;
        }
        AD_FreeConfigContents(&pState->config);

        if (pState->pStateLock)
        {
            pthread_rwlock_destroy(pState->pStateLock);
        }

        LwFreeMemory(pState);
    }
}

static
VOID
LsaAdProviderStateAcquireRead(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    LW_ASSERT(pthread_rwlock_rdlock(pState->pStateLock) == 0);
}

static
VOID
LsaAdProviderStateAcquireWrite(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    LW_ASSERT(pthread_rwlock_wrlock(pState->pStateLock) == 0);
}

static
void
LsaAdProviderStateRelease(
    PLSA_AD_PROVIDER_STATE pState
    )
{
    LW_ASSERT(pthread_rwlock_unlock(pState->pStateLock) == 0);
}

static
DWORD
AD_MachineCredentialsCacheClear()
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    pthread_mutex_lock(gpLsaAdProviderState->MachineCreds.pMutex);
    bInLock = TRUE;

    if (gpLsaAdProviderState->MachineCreds.bIsInitialized)
    {
        dwError = LwKrb5CleanupMachineSession();
        BAIL_ON_LSA_ERROR(dwError);
        gpLsaAdProviderState->MachineCreds.bIsInitialized = FALSE;
    }

error:

    if (bInLock)
    {
        pthread_mutex_unlock(gpLsaAdProviderState->MachineCreds.pMutex);
    }

    return dwError;
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

    LwStrToUpper(pszHostname);

    // Read password info before acquiring the lock.
    dwError = LwKrb5GetMachineCreds(
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName,
                    &pszHostDnsDomain);
    BAIL_ON_LSA_ERROR(dwError);

    if (LsaDmIsDomainOffline(pszDomainDnsName))
    {
        dwError = LW_ERROR_DOMAIN_IS_OFFLINE;
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

    dwError = LwKrb5SetProcessDefaultCachePath(LSASS_CACHE_PATH);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwSetupMachineSession(
                    pszUsername,
                    pszPassword,
                    pszDomainDnsName,
                    pszHostDnsDomain,
                    &dwGoodUntilTime);
    if (dwError)
    {
        if (dwError == LW_ERROR_DOMAIN_IS_OFFLINE)
        {
            LsaDmTransitionOffline(pszDomainDnsName, FALSE);
        }

        ADSetMachineTGTExpiryError();
    }
    BAIL_ON_LSA_ERROR(dwError);

    ADSetMachineTGTExpiry(dwGoodUntilTime);

    gpLsaAdProviderState->MachineCreds.bIsInitialized = TRUE;

cleanup:
    if (bIsAcquired)
    {
        pthread_mutex_unlock(gpLsaAdProviderState->MachineCreds.pMutex);
    }

    LW_SAFE_FREE_STRING(pszHostname);
    LW_SAFE_FREE_STRING(pszUsername);
    LW_SAFE_CLEAR_FREE_STRING(pszPassword);
    LW_SAFE_FREE_STRING(pszDomainDnsName);
    LW_SAFE_FREE_STRING(pszHostDnsDomain);

    return dwError;

error:

    goto cleanup;
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

    dwError = LwAllocateStringPrintf(
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

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszData);

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

        dwError = LwAllocateStringPrintf(
                     &pszNewMemberList,
                     "%s        %s\r\n",
                     pszMemberList ? pszMemberList : "",
                     LSA_SAFE_LOG_STRING((PSTR)pIter->pItem));
        BAIL_ON_LSA_ERROR(dwError);

        LW_SAFE_FREE_STRING(pszMemberList);
        pszMemberList = pszNewMemberList;
        pszNewMemberList = NULL;
    }

    dwError = LwAllocateStringPrintf(
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

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszMemberList);

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

            dwError = LwAllocateStringPrintf(
                         &pszNewMemberList,
                         "%s        %s\r\n",
                         pszMemberList ? pszMemberList : "",
                         LSA_SAFE_LOG_STRING(pHashEntry->pValue));
            BAIL_ON_LSA_ERROR(dwError);

            LW_SAFE_FREE_STRING(pszMemberList);
            pszMemberList = pszNewMemberList;
            pszNewMemberList = NULL;
        }
    }
    else
    {
            dwError = LwAllocateStringPrintf(
                         &pszMemberList,
                         "        <No login restrictions specified>\r\n");
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
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

    LW_SAFE_FREE_STRING(pszDescription);
    LW_SAFE_FREE_STRING(pszMemberList);
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

    dwError = LwAllocateStringPrintf(
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

    LW_SAFE_FREE_STRING(pszDescription);

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
            if (dwError == LW_ERROR_SUCCESS)
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
            dwError = LW_ERROR_SUCCESS;

            ADCacheSafeFreeObject(&pGroupInfo);
            dwError = AD_FindGroupObjectByName(
                            hProvider,
                            pszMember,
                            &pGroupInfo);
            if (dwError == LW_ERROR_SUCCESS)
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
            dwError = LW_ERROR_SUCCESS;
        }
    }

    *ppAllowedMemberList = (PVOID)pAllowedMemberList;

cleanup:

    if (ppszMembers)
    {
        LwFreeStringArray(ppszMembers, dwNumMembers);
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

            dwError = LW_ERROR_UNSUPPORTED_USER_LEVEL;
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

            dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;
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
       for (; ppszMembers && !LW_IS_NULL_OR_EMPTY_STR(*ppszMembers); ppszMembers++)
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
        (!LW_IS_NULL_OR_EMPTY_STR(pszCanonicalName + dwDomainNameLen + 1)))
    {
        PCSTR pszIndex = pszCanonicalName + dwDomainNameLen + 1;

        while (!LW_IS_NULL_OR_EMPTY_STR(pszIndex))
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
    switch (gpLsaAdProviderState->config.CacheBackend)
    {
        default:
            LSA_LOG_DEBUG("Unknown cache backend. Switching to default");
#ifdef AD_CACHE_ENABLE_SQLITE
        case AD_CACHE_SQLITE:
            InitializeDbCacheProvider(
                pCacheProviderTable
                );
            break;
#endif
        case AD_CACHE_IN_MEMORY:
            InitializeMemCacheProvider(
                pCacheProviderTable
                );
            break;
    }
}

DWORD
LsaInitializeProvider(
    OUT PCSTR* ppszProviderName,
    OUT PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable
    )
{
    return AD_InitializeProvider(ppszProviderName, ppFunctionTable);
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
