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
    PAD_SECURITY_OBJECT pUserInfo = NULL;
    PAD_PASSWORD_VERIFIER pVerifier = NULL;
    PSTR pszEnteredPasswordVerifier = NULL;
    PBYTE pbHash = NULL;
    HANDLE hDb = (HANDLE)NULL;

    dwError = AD_FindUserObjectByName(
                hProvider,
                pszUserName,
                &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_VerifyUserAccountCanLogin(
                pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_GetPasswordVerifier(
                hDb,
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

cleanup:

    ADCacheDB_SafeFreeObject(&pUserInfo);
    ADCACHEDB_SAFE_FREE_PASSWORD_VERIFIER(pVerifier);
    ADCacheDB_SafeCloseDb(&hDb);
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
    OUT PAD_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    HANDLE hDb = 0;
    PAD_SECURITY_OBJECT pCachedUser = NULL;

    if (uid == 0)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_FindUserById(
            hDb,
            uid,
            &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    *ppResult = pCachedUser;

cleanup:

    ADCacheDB_SafeCloseDb(&hDb);
    return dwError;

error:

    *ppResult = NULL;
    ADCacheDB_SafeFreeObject(&pCachedUser);

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
    OUT PAD_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;
    PSTR pszGroupNameCopy = NULL;
    PLSA_LOGIN_NAME_INFO pGroupNameInfo = NULL;
    HANDLE hDb = 0;
    PAD_SECURITY_OBJECT pGroupObject = NULL;

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

    LsaStrCharReplace(pszGroupNameCopy, AD_GetSeparator(), ' ');

    dwError = LsaCrackDomainQualifiedName(
                        pszGroupNameCopy,
                        gpADProviderData->szDomain,
                        &pGroupNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_FindGroupByName(
                hDb,
                pGroupNameInfo,
                &pGroupObject);
    BAIL_ON_LSA_ERROR(dwError);

    *ppResult = pGroupObject;

cleanup:

    ADCacheDB_SafeCloseDb(&hDb);
    if (pGroupNameInfo)
    {
        LsaFreeNameInfo(pGroupNameInfo);
    }
    LSA_SAFE_FREE_STRING(pszGroupNameCopy);

    return dwError;

error:

    *ppResult = NULL;
    ADCacheDB_SafeFreeObject(&pGroupObject);

    LSA_REMAP_FIND_GROUP_BY_NAME_ERROR(dwError, TRUE, pszGroupName);

    goto cleanup;
}

DWORD
AD_OfflineFindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    HANDLE hDb = 0;
    PAD_SECURITY_OBJECT pGroupObject = NULL;

    if (gid == 0)
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_FindGroupById(
                hDb,
                gid,
                &pGroupObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = AD_GroupObjectToGroupInfo(
                hProvider,
                pGroupObject,
                dwGroupInfoLevel,
                ppGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    ADCacheDB_SafeCloseDb(&hDb);
    ADCacheDB_SafeFreeObject(&pGroupObject);

    return dwError;

error:
    *ppGroupInfo = NULL;

    LSA_REMAP_FIND_GROUP_BY_ID_ERROR(dwError, TRUE, gid);

    goto cleanup;
}

DWORD
AD_OfflineGetExpandedGroupUsers(
    IN PCSTR pszGroupSid,
    IN DWORD dwMaxDepth,
    OUT PBOOLEAN pbIsFullyExpanded,
    OUT size_t* psMemberUsersCount,
    OUT PAD_SECURITY_OBJECT** pppMemberUsers
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    BOOLEAN bIsFullyExpanded = FALSE;
    PLSA_AD_GROUP_EXPANSION_DATA pExpansionData = NULL;
    PAD_SECURITY_OBJECT* ppGroupMembers = NULL;
    size_t sGroupMembersCount = 0;
    PAD_SECURITY_OBJECT pGroupToExpand = NULL;
    DWORD dwGroupToExpandDepth = 0;
    PCSTR pszGroupToExpandSid = NULL;
    PAD_SECURITY_OBJECT* ppExpandedUsers = NULL;
    size_t sExpandedUsersCount = 0;

    dwError = AD_GroupExpansionDataCreate(
                &pExpansionData,
                LSA_MAX(1, dwMaxDepth));
    BAIL_ON_LSA_ERROR(dwError);

    pszGroupToExpandSid = pszGroupSid;
    dwGroupToExpandDepth = 1;

    while (pszGroupToExpandSid)
    {
        dwError = AD_OfflineGetGroupMembers(
                    pszGroupToExpandSid,
                    &sGroupMembersCount,
                    &ppGroupMembers);
        BAIL_ON_LSA_ERROR(dwError);
        
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
    ADCacheDB_SafeFreeObjectList(sGroupMembersCount, &ppGroupMembers);

    if (pbIsFullyExpanded)
    {
        *pbIsFullyExpanded = bIsFullyExpanded;
    }

    *psMemberUsersCount = sExpandedUsersCount;
    *pppMemberUsers = ppExpandedUsers;

    return dwError;

error:
    ADCacheDB_SafeFreeObjectList(sExpandedUsersCount, &ppExpandedUsers);
    sExpandedUsersCount = 0;
    bIsFullyExpanded = FALSE;
    goto cleanup;
}

DWORD
AD_OfflineGetUserGroupObjectMembership(
    HANDLE hProvider,
    uid_t uid,
    size_t* psNumGroupsFound,
    PAD_SECURITY_OBJECT** pppResult
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    HANDLE hDb = 0;
    const DWORD dwUserInfoLevel = 0;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    size_t sUserGroupMembershipsCount = 0;
    PAD_GROUP_MEMBERSHIP* ppUserGroupMemberships = NULL;
    size_t sParentSidsCount = 0;
    // Do not free ppszParentSids at it points to ppUserGroupMemberships data.
    PSTR* ppszParentSids = NULL;
    size_t sGroupObjectsCount = 0;
    PAD_SECURITY_OBJECT* ppGroupObjects = NULL;

    dwError = AD_FindUserById(
                    hProvider,
                    uid,
                    dwUserInfoLevel,
                    (PVOID*)&pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_GetGroupsForUser(
                hDb,
                pUserInfo->pszSid,
                &sUserGroupMembershipsCount,
                &ppUserGroupMemberships);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Gather up the SIDs.
    //

    dwError = AD_GatherSidsFromGroupMemberships(
                TRUE,
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
    ADCacheDB_SafeCloseDb(&hDb);
    ADCacheDB_SafeFreeGroupMembershipList(sUserGroupMembershipsCount, &ppUserGroupMemberships);
    LSA_SAFE_FREE_MEMORY(ppszParentSids);

    if (pUserInfo)
    {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    *pppResult = NULL;
    *psNumGroupsFound = 0;

    LSA_LOG_ERROR("Failed to find user's group memberships of uid %d. [error code %d]",
                  uid, dwError);

    ADCacheDB_SafeFreeObjectList(sGroupObjectsCount, &ppGroupObjects);

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
    OUT PAD_SECURITY_OBJECT* ppCachedUser
    )
{
    DWORD dwError = 0;
    PLSA_LOGIN_NAME_INFO pUserNameInfo = NULL;
    PSTR  pszLoginId_copy = NULL;
    HANDLE hDb = (HANDLE)NULL;
    PAD_SECURITY_OBJECT pCachedUser = NULL;

    BAIL_ON_INVALID_STRING(pszLoginId);

    dwError = LsaAllocateString(
                    pszLoginId,
                    &pszLoginId_copy);
    BAIL_ON_LSA_ERROR(dwError);

    LsaStrCharReplace(pszLoginId_copy, AD_GetSeparator(),' ');

    dwError = LsaCrackDomainQualifiedName(
                        pszLoginId_copy,
                        gpADProviderData->szDomain,
                        &pUserNameInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_FindUserByName(
            hDb,
            pUserNameInfo,
            &pCachedUser);
    BAIL_ON_LSA_ERROR(dwError);

    *ppCachedUser = pCachedUser;

cleanup:

    ADCacheDB_SafeCloseDb(&hDb);
    if (pUserNameInfo)
    {
        LsaFreeNameInfo(pUserNameInfo);
    }
    LSA_SAFE_FREE_STRING(pszLoginId_copy);

    return dwError;

error:

    *ppCachedUser = NULL;

    ADCacheDB_SafeFreeObject(&pCachedUser);

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
    HANDLE hDb = (HANDLE)NULL;
    PAD_PROVIDER_DATA pProviderData = NULL;
    PDLINKEDLIST pDomains = NULL;
    const DLINKEDLIST* pPos = NULL;
    const LSA_DM_ENUM_DOMAIN_INFO* pDomain = NULL;

    dwError = ADCacheDB_OpenDb(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_GetDomainTrustList(
        hDb,
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
            pDomain->pszForestName,
            NULL
            );
        BAIL_ON_LSA_ERROR(dwError);

        pPos = pPos->pNext;
    }

    dwError = ADCacheDB_GetProviderData(
                hDb,
                &pProviderData);
    BAIL_ON_LSA_ERROR(dwError);

    *ppProviderData = pProviderData;

cleanup:
    ADCacheDB_FreeEnumDomainInfoList(pDomains);
    ADCacheDB_SafeCloseDb(&hDb);
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
