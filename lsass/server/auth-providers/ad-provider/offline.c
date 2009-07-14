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


BOOLEAN
AD_IsOffline(
    VOID
    )
{
    return LsaDmIsDomainOffline(NULL);
}

DWORD
AD_OfflineAuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pUserInfo = NULL;
    PLSA_PASSWORD_VERIFIER pVerifier = NULL;
    PSTR pszEnteredPasswordVerifier = NULL;
    PBYTE pbHash = NULL;

    dwError = AD_FindUserObjectByName(
                hProvider,
                pszUserName,
                &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_VerifyUserAccountCanLogin(
                pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheGetPasswordVerifier(
                gpLsaAdProviderState->hCacheConnection,
                pUserInfo->pszObjectSid,
                &pVerifier
                );
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GetCachedPasswordHash(
                pUserInfo->pszSamAccountName,
                pszPassword,
                &pbHash);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaByteArrayToHexStr(
                pbHash,
                16,
                &pszEnteredPasswordVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    if (strcmp(pszEnteredPasswordVerifier, pVerifier->pszPasswordVerifier))
    {
        dwError = LSA_ERROR_PASSWORD_MISMATCH;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaUmAddUser(
                  pUserInfo->userInfo.uid,
                  pszPassword,
                  0);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    ADCacheSafeFreeObject(&pUserInfo);
    LSA_DB_SAFE_FREE_PASSWORD_VERIFIER(pVerifier);
    LSA_SAFE_FREE_STRING(pszEnteredPasswordVerifier);
    LSA_SAFE_FREE_MEMORY(pbHash);

    return dwError;

error:

    goto cleanup;
}

DWORD
AD_OfflineFindUserObjectById(
    IN HANDLE hProvider,
    IN uid_t uid,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;

    if (uid == 0)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheFindUserById(
            gpLsaAdProviderState->hCacheConnection,
            uid,
            &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    *ppResult = pCachedUser;

cleanup:

    return dwError;

error:

    *ppResult = NULL;
    ADCacheSafeFreeObject(&pCachedUser);

    LSA_REMAP_FIND_USER_BY_ID_ERROR(dwError, TRUE, uid);

    goto cleanup;
}

DWORD
AD_OfflineEnumUsers(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    )
{
    return LSA_ERROR_NO_MORE_USERS;
}

DWORD
AD_OfflineFindGroupObjectByName(
    IN HANDLE hProvider,
    IN PCSTR pszGroupName,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PSTR pszGroupNameCopy = NULL;
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    PLSA_SECURITY_OBJECT pGroupObject = NULL;

    BAIL_ON_INVALID_STRING(pszGroupName);

    if (!strcasecmp(pszGroupName, "root"))
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateString(
                    pszGroupName,
                    &pszGroupNameCopy);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrCharReplace(pszGroupNameCopy, AD_GetSpaceReplacement(), ' ');

    dwError = LsaCrackDomainQualifiedName(
                        pszGroupNameCopy,
                        gpADProviderData->szDomain,
                        &pGroupNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheFindGroupByName(
                gpLsaAdProviderState->hCacheConnection,
                pGroupNameInfo,
                &pGroupObject);
    BAIL_ON_LSA_ERROR(dwError);

    *ppResult = pGroupObject;

cleanup:

    if (pGroupNameInfo)
    {
        LsaFreeNameInfo(pGroupNameInfo);
    }
    LSA_SAFE_FREE_STRING(pszGroupNameCopy);

    return dwError;

error:

    *ppResult = NULL;
    ADCacheSafeFreeObject(&pGroupObject);

    LSA_REMAP_FIND_GROUP_BY_NAME_ERROR(dwError, TRUE, pszGroupName);

    goto cleanup;
}

DWORD
AD_OfflineFindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN BOOLEAN bIsCacheOnlyMode,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_SECURITY_OBJECT pGroupObject = NULL;

    if (gid == 0)
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheFindGroupById(
                gpLsaAdProviderState->hCacheConnection,
                gid,
                &pGroupObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GroupObjectToGroupInfo(
                hProvider,
                pGroupObject,
                bIsCacheOnlyMode,
                dwGroupInfoLevel,
                ppGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    ADCacheSafeFreeObject(&pGroupObject);

    return dwError;

error:
    *ppGroupInfo = NULL;

    LSA_REMAP_FIND_GROUP_BY_ID_ERROR(dwError, TRUE, gid);

    goto cleanup;
}

DWORD
AD_OfflineGetUserGroupObjectMembership(
    IN HANDLE hProvider,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppResult
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t sUserGroupMembershipsCount = 0;
    PLSA_GROUP_MEMBERSHIP* ppUserGroupMemberships = NULL;
    size_t sParentSidsCount = 0;
    // Do not free ppszParentSids at it points to ppUserGroupMemberships data.
    PSTR* ppszParentSids = NULL;
    size_t sGroupObjectsCount = 0;
    PLSA_SECURITY_OBJECT* ppGroupObjects = NULL;

    dwError = ADCacheGetGroupsForUser(
                gpLsaAdProviderState->hCacheConnection,
                pUserInfo->pszObjectSid,
                AD_GetTrimUserMembershipEnabled(),
                &sUserGroupMembershipsCount,
                &ppUserGroupMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Gather up the SIDs.
    //

    dwError = AD_GatherSidsFromGroupMemberships(
                TRUE,
                NULL,
                sUserGroupMembershipsCount,
                ppUserGroupMemberships,
                &sParentSidsCount,
                &ppszParentSids);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Resolve the SIDs to objects.
    //

    dwError = AD_OfflineFindObjectsBySidList(
                sParentSidsCount,
                ppszParentSids,
                &ppGroupObjects);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Filter out any objects that could not be resolved.
    //

    sGroupObjectsCount = sParentSidsCount;
    AD_FilterNullEntries(ppGroupObjects, &sGroupObjectsCount);

    *pppResult = ppGroupObjects;
    *psNumGroupsFound = sGroupObjectsCount;

cleanup:
    ADCacheSafeFreeGroupMembershipList(sUserGroupMembershipsCount, &ppUserGroupMemberships);
    LSA_SAFE_FREE_MEMORY(ppszParentSids);

    return dwError;

error:

    *pppResult = NULL;
    *psNumGroupsFound = 0;

    LSA_LOG_ERROR("Failed to find memberships for user '%s\\%s' (error = %d)",
                  pUserInfo->pszNetbiosDomainName,
                  pUserInfo->pszSamAccountName,
                  dwError);

    ADCacheSafeFreeObjectList(sGroupObjectsCount, &ppGroupObjects);

    goto cleanup;
}

DWORD
AD_OfflineEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    return LSA_ERROR_NO_MORE_GROUPS;
}

DWORD
AD_OfflineChangePassword(
    HANDLE hProvider,
    PCSTR pszUserName,
    PCSTR pszPassword,
    PCSTR pszOldPassword
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
AD_OfflineGetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes
    )
{
    return LSA_ERROR_NOT_HANDLED;
}

DWORD
AD_OfflineFindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    *ppNSSArtefactInfo = NULL;

    return LSA_ERROR_NO_SUCH_NSS_MAP;
}

DWORD
AD_OfflineEnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
    return LSA_ERROR_NO_MORE_GROUPS;
}

DWORD
AD_OfflineFindUserObjectByName(
    IN HANDLE  hProvider,
    IN PCSTR   pszLoginId,
    OUT PLSA_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    PSTR  pszLoginId_copy = NULL;
    PLSA_SECURITY_OBJECT pCachedUser = NULL;

    BAIL_ON_INVALID_STRING(pszLoginId);

    dwError = LsaAllocateString(
                    pszLoginId,
                    &pszLoginId_copy);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrCharReplace(pszLoginId_copy, AD_GetSpaceReplacement(),' ');

    dwError = LsaCrackDomainQualifiedName(
                        pszLoginId_copy,
                        gpADProviderData->szDomain,
                        &pUserNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheFindUserByName(
            gpLsaAdProviderState->hCacheConnection,
            pUserNameInfo,
            &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    *ppCachedUser = pCachedUser;

cleanup:

    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }
    LSA_SAFE_FREE_STRING(pszLoginId_copy);

    return dwError;

error:

    *ppCachedUser = NULL;

    ADCacheSafeFreeObject(&pCachedUser);

    LSA_REMAP_FIND_USER_BY_NAME_ERROR(dwError, TRUE, pszLoginId);

    goto cleanup;
}

DWORD
AD_OfflineInitializeOperatingMode(
    OUT PAD_PROVIDER_DATA* ppProviderData,
    IN PCSTR pszDomain,
    IN PCSTR pszHostName
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PAD_PROVIDER_DATA pProviderData = NULL;
    PDLINKEDLIST pDomains = NULL;
    const DLINKEDLIST* pPos = NULL;
    const LSA_DM_ENUM_DOMAIN_INFO* pDomain = NULL;

    dwError = ADState_GetDomainTrustList(
        gpLsaAdProviderState->hStateConnection,
        &pDomains);
    BAIL_ON_LSA_ERROR(dwError);

    pPos = pDomains;
    while (pPos != NULL)
    {
        pDomain = (const LSA_DM_ENUM_DOMAIN_INFO*)pPos->pItem;

        dwError = LsaDmAddTrustedDomain(
            pDomain->pszDnsDomainName,
            pDomain->pszNetbiosDomainName,
            pDomain->pSid,
            pDomain->pGuid,
            pDomain->pszTrusteeDnsDomainName,
            pDomain->dwTrustFlags,
            pDomain->dwTrustType,
            pDomain->dwTrustAttributes,
            pDomain->dwTrustDirection,
            pDomain->dwTrustMode,
            IsSetFlag(pDomain->Flags, LSA_DM_DOMAIN_FLAG_TRANSITIVE_1WAY_CHILD) ? TRUE : FALSE,
            pDomain->pszForestName,
            NULL
            );
        BAIL_ON_LSA_ERROR(dwError);

        pPos = pPos->pNext;
    }

    dwError = ADState_GetProviderData(
                gpLsaAdProviderState->hStateConnection,
                &pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    *ppProviderData = pProviderData;

cleanup:
    ADState_FreeEnumDomainInfoList(pDomains);
    return dwError;

error:
    *ppProviderData = NULL;

    if (pProviderData)
    {
        ADProviderFreeProviderData(pProviderData);
        pProviderData = NULL;
    }

    goto cleanup;
}
