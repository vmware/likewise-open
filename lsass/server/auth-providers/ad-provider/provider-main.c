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
#include "provider-main_p.h"

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
    PSTR pszKrb5CcPath = NULL;
    BOOLEAN bIsDomainOffline = FALSE;

    pthread_rwlock_init(&gADGlobalDataLock, NULL);

    dwError = LsaAdProviderStateCreate(&gpLsaAdProviderState);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaKrb5Init(
                LsaAdProviderLsaKrb5IsOfflineCallback,
                LsaAdProviderLsaKrb5TransitionOfflineCallback);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_NetInitMemory();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_SetUnprovisionedModeShell(
                        "/bin/sh");
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = AD_SetUnprovisionedModeHomedirTemplate("/home/%D/%U");
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pszConfigFilePath)) {
        dwError = LsaParseConfigFile(
                    pszConfigFilePath,
                    LSA_CFG_OPTION_STRIP_ALL,
                    &AD_ConfigStartSection,
                    NULL,
                    &AD_ConfigNameValuePair,
                    NULL,
                    NULL);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = AD_SetConfigFilePath(pszConfigFilePath);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADInitMachinePasswordSync();
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaDnsGetHostInfo(&pszHostname);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaStrToUpper(pszHostname);
    
    dwError = LsaKrb5GetMachineCreds(
                    pszHostname,
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName);
    BAIL_ON_LSA_ERROR(dwError);

    // Initialize domain manager before doing any network stuff.
    dwError = LsaDmInitialize(TRUE, 5 * 60);
    BAIL_ON_LSA_ERROR(dwError); 

    dwError = AD_TestNetworkConnection(pszDomainDnsName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDmWrapLdapPingTcp(pszDomainDnsName);
    switch (dwError)
    {
        case LSA_ERROR_DOMAIN_IS_OFFLINE:
        case LWNET_ERROR_FAILED_FIND_DC:
            bIsDomainOffline = TRUE;
            dwError = 0;
            break;
       default:
            break;
    }
    BAIL_ON_LSA_ERROR(dwError);   

    if (bIsDomainOffline)
    {
        dwError = AD_MachineCredentialsCacheClear();
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = AD_MachineCredentialsCacheInitialize();
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaKrb5GetSystemCachePath(KRB5_File_Cache, &pszKrb5CcPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaKrb5SetProcessDefaultCachePath(pszKrb5CcPath);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_Initialize();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_InitializeOperatingMode(
                pszDomainDnsName,
                pszHostname,
                bIsDomainOffline);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADInitCacheReaper();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADStartMachinePasswordSync();
    BAIL_ON_LSA_ERROR(dwError);

    *ppFunctionTable = &gADProviderAPITable;
    *ppszProviderName = gpszADProviderName;

cleanup:

    LSA_SAFE_FREE_STRING(pszHostname);
    LSA_SAFE_FREE_STRING(pszUsername);
    LSA_SAFE_CLEAR_FREE_STRING(pszPassword);
    LSA_SAFE_FREE_STRING(pszDomainDnsName);
    LSA_SAFE_FREE_STRING(pszKrb5CcPath);

    return dwError;

error:

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
    
    dwError = ADShutdownCacheReaper();
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = ADShutdownMachinePasswordSync();
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_NetShutdownMemory();
    BAIL_ON_LSA_ERROR(dwError);
    
    ENTER_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    LSA_SAFE_FREE_STRING(gpszUnprovisionedModeShell);
    LSA_SAFE_FREE_STRING(gpszUnprovisionedModeHomedirTemplate);
    
    if (gpADProviderData->pCellList) {
        LsaDLinkedListForEach(
                        gpADProviderData->pCellList,
                        &AD_FreeLinkedCellInfoInList,
                        NULL);
        LsaDLinkedListFree(
                        gpADProviderData->pCellList
                        );
    }  

    LsaDmCleanup();

    dwError = ADCacheDB_Shutdown();
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = LsaKrb5Shutdown();
    BAIL_ON_LSA_ERROR(dwError);
    
    AD_FreeAllowedGroups_InLock();

    LSA_SAFE_FREE_STRING(gpszConfigFilePath);

    LsaAdProviderStateDestroy(gpLsaAdProviderState);
    gpLsaAdProviderState = NULL;

cleanup:

    LEAVE_AD_GLOBAL_DATA_RW_WRITER_LOCK(bInLock);

    return dwError;
    
error:
    
    goto cleanup;
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
    if (pContext) {
        
        AD_FreeStateList(pContext->pGroupEnumStateList);
        AD_FreeStateList(pContext->pUserEnumStateList);
        
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
    if (AD_IsOffline())
    {
        return AD_OfflineAuthenticateUser(
            hProvider,
            pszLoginId,
            pszPassword);
    }
    else
    {
        return AD_OnlineAuthenticateUser(
            hProvider,
            pszLoginId,
            pszPassword);
    }
    
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
    DWORD dwUserInfoLevel = 2;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    
    dwError = LsaCrackDomainQualifiedName(
                    pszLoginId,
                    gpADProviderData->szDomain,
                    &pLoginInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!AD_ServicesDomain(pLoginInfo->pszDomainNetBiosName)) {
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = AD_FindUserByName(
                    hProvider,
                    pszLoginId,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (pUserInfo->bPasswordExpired) {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
cleanup:

    if (pLoginInfo) {
        LsaFreeNameInfo(pLoginInfo);
    }
    
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, (PVOID)pUserInfo);
    }

    return dwError;

error:

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
    PAD_SECURITY_OBJECT pInObjectForm = NULL;
    
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
    
    *ppUserInfo = pUserInfo;
    
cleanup:

    ADCacheDB_SafeFreeObject(&pInObjectForm);

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
    PAD_SECURITY_OBJECT pInObjectForm = NULL;
    
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
    
    *ppUserInfo = pUserInfo;
    
cleanup:

    ADCacheDB_SafeFreeObject(&pInObjectForm);

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
AD_FindUserObjectById(
    IN HANDLE  hProvider,
    IN uid_t   uid,
    OUT PAD_SECURITY_OBJECT* ppResult
    )
{
    if (AD_IsOffline())
    {
        return AD_OfflineFindUserObjectById(
            hProvider,
            uid,
            ppResult);
    }
    else
    {
        return AD_OnlineFindUserObjectById(
            hProvider,
            uid,
            ppResult);
    }
}

DWORD
AD_BeginEnumUsers(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = NULL;
  
    dwError = AD_AddUserState(
                        hProvider,
                        pszGUID,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);    
    
    pEnumState->pCookie = NULL;    
    pEnumState->bMorePages = TRUE;
    
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
    if (AD_IsOffline())
    {
        return AD_OfflineEnumUsers(
            hProvider,
            hResume,
            dwMaxNumUsers,
            pdwUsersFound,
            pppUserInfoList);
    }
    else
    {
        return AD_OnlineEnumUsers(
            hProvider,
            hResume,
            dwMaxNumUsers,
            pdwUsersFound,
            pppUserInfoList);
    }
}

VOID
AD_EndEnumUsers(
    HANDLE hProvider,   
    PCSTR  pszGUID
    )
{    
    AD_FreeUserState(hProvider, pszGUID);
}

DWORD
AD_FindGroupByName(
    HANDLE  hProvider,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD   dwError = 0;
    PSTR    pszGroupName_copy = NULL;
    BOOLEAN bTryAgain = FALSE;
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    PVOID   pGroupInfo = NULL;
    
    dwError = LsaAllocateString(
                    pszGroupName,
                    &pszGroupName_copy);
    BAIL_ON_LSA_ERROR(dwError);
    
    do
    {
        bTryAgain = FALSE;
        
        LsaStrCharReplace(pszGroupName_copy, AD_GetSeparator(),' ');
        
        if (pGroupNameInfo)
        {
            LsaFreeNameInfo(pGroupNameInfo);
            pGroupNameInfo = NULL;
        }
        
        dwError = LsaCrackDomainQualifiedName(
                            pszGroupName_copy,
                            gpADProviderData->szDomain,
                            &pGroupNameInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        if ((pGroupNameInfo->nameType == NameType_Alias) &&
            !strcasecmp(pGroupNameInfo->pszName, "root")) {
            dwError = LSA_ERROR_NO_SUCH_GROUP;
            BAIL_ON_LSA_ERROR(dwError);
        }
        
        if (AD_IsOffline())
        {
            dwError = AD_OfflineFindGroupByName(
                            hProvider,
                            pszGroupName_copy,
                            dwGroupInfoLevel,
                            &pGroupInfo);
        }
        else
        {
            dwError = AD_OnlineFindGroupByName(
                            hProvider,
                            pszGroupName_copy,
                            dwGroupInfoLevel,
                            &pGroupInfo);
        }
        
        if ((dwError == LSA_ERROR_NO_SUCH_GROUP) &&
            (pGroupNameInfo->nameType == NameType_Alias) &&
            (AD_ShouldAssumeDefaultDomain()))
        {
            LSA_SAFE_FREE_STRING(pszGroupName_copy);
            
            dwError = LsaAllocateStringPrintf(
                            &pszGroupName_copy,
                            "%s\\%s",
                            gpADProviderData->szShortDomain,
                            pszGroupName);
            BAIL_ON_LSA_ERROR(dwError);

            bTryAgain = TRUE;
        }

    } while (bTryAgain);
    
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppGroupInfo = pGroupInfo;
    
cleanup:

    LSA_SAFE_FREE_STRING(pszGroupName_copy);
    
    if (pGroupNameInfo)
    {
        LsaFreeNameInfo(pGroupNameInfo);
    }

    return dwError;
    
error:
    
    *ppGroupInfo = NULL;
    
    if (pGroupInfo)
    {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }
    
    goto cleanup;
}

DWORD
AD_FindGroupById(
    HANDLE  hProvider,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    if (AD_IsOffline())
    {
        return AD_OfflineFindGroupById(
            hProvider,
            gid,
            dwGroupInfoLevel,
            ppGroupInfo);
    }
    else
    {
        return AD_OnlineFindGroupById(
            hProvider,
            gid,
            dwGroupInfoLevel,
            ppGroupInfo);
    }
}


DWORD
AD_GetUserGroupMembership(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    if (AD_IsOffline())
    {
        return AD_OfflineGetUserGroupMembership(
            hProvider,
            uid,
            dwGroupInfoLevel,
            pdwNumGroupsFound,
            pppGroupInfoList);
    }
    else
    {
        return AD_OnlineGetUserGroupMembership(
            hProvider,
            uid,
            dwGroupInfoLevel,
            pdwNumGroupsFound,
            pppGroupInfoList);
    }
}

DWORD
AD_BeginEnumGroups(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = NULL;
    
    dwError = AD_AddGroupState(
                        hProvider,
                        pszGUID,
                        dwInfoLevel,
                        &pEnumState);
    BAIL_ON_LSA_ERROR(dwError);
    
    pEnumState->pCookie = NULL;  
    pEnumState->bMorePages = TRUE;
    
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
    if (AD_IsOffline())
    {
        return AD_OfflineEnumGroups(
            hProvider,
            hResume,
            dwMaxGroups,
            pdwGroupsFound,
            pppGroupInfoList);
    }
    else
    {
        return AD_OnlineEnumGroups(
            hProvider,
            hResume,
            dwMaxGroups,
            pdwGroupsFound,
            pppGroupInfoList);
    }
}

VOID
AD_EndEnumGroups(
    HANDLE hProvider,
    PCSTR  pszGUID
    )
{
    AD_FreeGroupState(hProvider, pszGUID);
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
AD_DeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    return LSA_ERROR_NOT_HANDLED;
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
    
    if (gbCreateK5Login) {
        
        dwError = AD_CreateK5Login(
                    (PLSA_USER_INFO_1)pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
    }
    
cleanup:
    if (pLoginInfo) {
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
AD_BeginEnumNSSArtefacts(
    HANDLE  hProvider,
    PCSTR   pszGUID,
    DWORD   dwInfoLevel,
    DWORD   dwMapType,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;
    PAD_ENUM_STATE pEnumState = NULL;
    
    switch (gpADProviderData->dwDirectoryMode) 
    {
        case DEFAULT_MODE:
        case CELL_MODE:
            
            dwError = AD_AddNSSArtefactState(
                                hProvider,
                                pszGUID,
                                dwInfoLevel,
                                dwMapType,
                                &pEnumState);
            BAIL_ON_LSA_ERROR(dwError);
            
            pEnumState->pCookie = NULL;  
            pEnumState->bMorePages = TRUE;
        
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
    PCSTR  pszGUID
    )
{
    AD_FreeNSSArtefactState(hProvider, pszGUID);
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
    
    dwError = LsaDmQueryState(&pProviderStatus->dwNetworkCheckInterval, NULL);
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
        dwError = AD_SidToString(
                        pDomainInfo->pSid,
                        &pTrustedDomainInfo->pszDomainSID);
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
    PSTR pszConfigFilePath = NULL;
    
    dwError = AD_GetConfigFilePath(&pszConfigFilePath);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!IsNullOrEmptyString(pszConfigFilePath)) {
        dwError = LsaParseConfigFile(
                    pszConfigFilePath,
                    LSA_CFG_OPTION_STRIP_ALL,
                    &AD_ConfigStartSection,
                    NULL,
                    &AD_ConfigNameValuePair,
                    NULL,
                    NULL);
        BAIL_ON_LSA_ERROR(dwError);
    }

    
cleanup:

    LSA_SAFE_FREE_STRING(pszConfigFilePath);
    
    return dwError;

error:

    goto cleanup;
    
}

DWORD
AD_FindUserObjectByName(
    IN HANDLE  hProvider,
    IN PCSTR   pszLoginId,    
    OUT PAD_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PSTR pszLoginId_copy = NULL;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    PAD_SECURITY_OBJECT pResult = NULL;

    if (!strcasecmp(pszLoginId, "root"))
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaAllocateString(
                    pszLoginId,
                    &pszLoginId_copy);
    BAIL_ON_LSA_ERROR(dwError);
    
    LsaStrCharReplace(pszLoginId_copy, AD_GetSeparator(),' ');
        
    if (AD_IsOffline())
    {
        dwError = AD_OfflineFindUserObjectByName(
                        hProvider,
                        pszLoginId_copy,
                        &pResult);
    }
    else
    {
        dwError = AD_OnlineFindUserObjectByName(
                        hProvider,
                        pszLoginId_copy,
                        &pResult);
    }

    if (dwError == LSA_ERROR_NO_SUCH_USER &&
        AD_ShouldAssumeDefaultDomain())
    {
        dwError = LsaCrackDomainQualifiedName(
                            pszLoginId_copy,
                            gpADProviderData->szDomain,
                            &pUserNameInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (pUserNameInfo->nameType == NameType_Alias)
        {
            LSA_SAFE_FREE_STRING(pszLoginId_copy);
            
            dwError = LsaAllocateStringPrintf(
                            &pszLoginId_copy,
                            "%s\\%s",
                            gpADProviderData->szShortDomain,
                            pszLoginId);
            BAIL_ON_LSA_ERROR(dwError);

            LsaStrCharReplace(pszLoginId_copy, AD_GetSeparator(),' ');

            if (AD_IsOffline())
            {
                dwError = AD_OfflineFindUserObjectByName(
                                hProvider,
                                pszLoginId_copy,
                                &pResult);
            }
            else
            {
                dwError = AD_OnlineFindUserObjectByName(
                                hProvider,
                                pszLoginId_copy,
                                &pResult);
            }
        }
    }
    
    BAIL_ON_LSA_ERROR(dwError);
    
    *ppResult = pResult;
    
cleanup:

    LSA_SAFE_FREE_STRING(pszLoginId_copy);
    
    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }

    return dwError;
    
error:

    *ppResult = NULL;
    ADCacheDB_SafeFreeObject(&pResult);

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

    if (bIsDomainOffline || AD_IsOffline())
    {
        dwError = AD_OfflineInitializeOperatingMode(
                pszDomain,
                pszHostName);
        BAIL_ON_LSA_ERROR(dwError);

        if (bIsDomainOffline)
        {
            // Note that we can only transition offline
            // now that we set up the domains in the domain manager.
            dwError = LsaDmTransitionOffline(pszDomain);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else
    {
        dwError = AD_OnlineInitializeOperatingMode(
                pszDomain,
                pszHostName);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:
    
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
        if (pState->MachineCreds.pMutex)
        {
            pthread_mutex_destroy(pState->MachineCreds.pMutex);
            pState->MachineCreds.pMutex = NULL;
        }
        LsaFreeMemory(pState);
    }
}

static
DWORD
AD_MachineCredentialsCacheClear()
{
    return LsaKrb5CleanupMachineSession();
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
    dwError = LsaKrb5GetMachineCreds(
                    pszHostname,
                    &pszUsername,
                    &pszPassword,
                    &pszDomainDnsName);
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
                    pszPassword,
                    pszDomainDnsName,
                    pszDomainDnsName,
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

    return dwError;

error:
    goto cleanup;
}

static
BOOLEAN
LsaAdProviderLsaKrb5IsOfflineCallback(
    IN PCSTR pszRealm
    )
{
    return LsaDmIsDomainOffline(pszRealm);
}

static
VOID
LsaAdProviderLsaKrb5TransitionOfflineCallback(
    IN PCSTR pszRealm
    )
{
    LsaDmTransitionOffline(pszRealm);
}
