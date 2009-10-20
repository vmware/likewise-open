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



DWORD
LSA_INITIALIZE_PROVIDER(ad)(
    PSTR* ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable
    )
{
    *ppFunctionTable = &gADProviderAPITable;
    *ppszProviderName = gpszADProviderName;

cleanup:


    return dwError;

error:

    *ppszProviderName = NULL;
    *ppFunctionTable = NULL;

    goto cleanup;
}


DWORD
LSA_SHUTDOWN_PROVIDER(ad)(
    PSTR pszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    DWORD dwError = 0;
    return dwError;
}


DWORD
ActiveDirectoryOpenHandle(
    uid_t   uid,
    gid_t   gid,
    pid_t   pid,
    PHANDLE phProvider
    )
{
    DWORD dwError = 0;
    PADPROVIDER_CONTEXT pContext = NULL;

    dwError = LwAllocateMemory(
                    sizeof(ADPROVIDER_CONTEXT),
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
        ADCloseHandle((HANDLE)pContext);
    }

    goto cleanup;
}

void
ActiveDirectoryCloseHandle(
    HANDLE hProvider
    )
{
    PADPROVIDER_CONTEXT pContext = (PADPROVIDER_CONTEXT)hProvider;
    if (pContext)
    {
        LwFreeMemory(pContext);
    }
}

BOOLEAN
ActiveDirectoryServicesDomain(
    PWSTR pszDomain
    )
{
    BOOLEAN bResult = FALSE;

    return bResult;
}

DWORD
ActiveDirectoryAuthenticateUser(
    HANDLE hProvider,
    PWSTR  pszLoginId,
    PWSTR  pszPassword
    )
{

    return dwError;
}

DWORD
ActiveDirectoryAuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
}

DWORD
ActiveDirectoryValidateUser(
    HANDLE hProvider,
    PWSTR  pszLoginId,
    PWSTR  pszPassword
    )
{
    return dwError;

}


DWORD
ActiveDirectoryCheckUserInList(
    HANDLE hProvider,
    PWSTR  pszUserName,
    PWSTR  pszListName
    )
{
    DWORD  dwError = 0;

    return dwError;

}

DWORD
ActiveDirectoryFindUserByName(
    HANDLE  hProvider,
    PWSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD   dwError = 0;

    return dwError;
}

DWORD
ActiveDirectoryFindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD   dwError = 0;

    dwError = ADFindUserById(
                    hProvider,
                    uid,
                    &pLsaSecurityObject
                    );
    BAIL_ON_ERROR(dwError);

    dwError = ConvertLsaSecurityObjectToInfoLevel(
                    dwUserInfoLevel,
                    pLsaSecurityObject,
                    ppUserInfo
                    );
    BAIL_ON_ERROR(dwError);

cleanup:

    if (pLsaSecurityObject) {

        LsaFreeSecurityObject(pLsaSecurityObject);
    }

error:

    return dwError;
}

DWORD
ActiveDirectoryBeginEnumUsers(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{

}

DWORD
ActiveDirectoryEnumUsers(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

	return dwError;
}

VOID
ActiveDirectoryEndEnumUsers(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    return;
}


DWORD
ActiveDirectoryFindGroupByName(
    IN HANDLE hProvider,
    IN PWSTR pszGroupName,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}


static
DWORD
ActiveDirectoryGetExpandedGroupUsersEx(
    IN HANDLE hProvider,
    IN PWSTR pszDomainName,
    IN BOOLEAN bIsOffline,
    IN PWSTR pszGroupSid,
    IN BOOLEAN bIsCacheOnlyMode,
    IN DWORD dwMaxDepth,
    OUT PBOOLEAN pbIsFullyExpanded,
    OUT size_t* psMemberUsersCount,
    OUT PLSA_SECURITY_OBJECT** pppMemberUsers
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;

    return dwError;
}
DWORD
ActiveDirectoryFindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
ActiveDirectoryGetUserGroupObjectMembership(
    IN HANDLE hProvider,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppResult
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
ActiveDirectoryGetGroupsForUser(
    IN HANDLE hProvider,
    IN OPTIONAL PWSTR pszUserName,
    IN OPTIONAL uid_t uid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    IN PDWORD pdwNumGroupsFound,
    IN PVOID** pppGroupInfoList
    )
{
	DWORD dwError = 0;

	return dwError;
}

DWORD
ActiveDirectoryBeginEnumGroups(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
	DWORD dwError = 0;

	return dwError;
}

DWORD
ActiveDirectoryEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

VOID
ActiveDirectoryEndEnumGroups(
    HANDLE hProvider,
    HANDLE hResume
    )
{
	return;
}

DWORD
ActiveDirectoryChangePassword(
    HANDLE hProvider,
    PWSTR pszLoginId,
    PWSTR pszPassword,
    PWSTR pszOldPassword
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
ActiveDirectorySetPassword(
    HANDLE hProvider,
    PWSTR pszLoginId,
    PWSTR pszPassword
    )
{
    return LW_ERROR_NOT_HANDLED;
}


DWORD
ActiveDirectoryAddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ActiveDirectoryModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ActiveDirectoryDeleteUser(
    HANDLE hProvider,
    uid_t  uid
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ActiveDirectoryAddGroup(
    HANDLE hProvider,
    DWORD dwGroupInfoLevel,
    PVOID pGroupInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ActiveDirectoryModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ActiveDirectoryDeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ActiveDirectoryOpenSession(
    HANDLE hProvider,
    PWSTR  pszLoginId
    )
{
    DWORD dwError = 0;
}

DWORD
ActiveDirectoryCloseSession(
    HANDLE hProvider,
    PWSTR  pszLoginId
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
ActiveDirectoryGetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes
    )
{
    DWORD dwError = 0;


    return dwError;
}

DWORD
ActiveDirectoryFindNSSArtefactByKey(
    HANDLE hProvider,
    PWSTR  pszKeyName,
    PWSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
ActiveDirectoryBeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PWSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
ActiveDirectoryEnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

VOID
ActiveDirectoryEndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    )
{

    DWORD dwError = 0;

    return dwError;
}

DWORD
ActiveDirectoryGetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    )
{
}



DWORD
ActiveDirectoryRefreshConfiguration(
    HANDLE hProvider
    )
{
}

DWORD
ActiveDirectoryProviderIoControl(
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
}

DWORD
ActiveDirectoryGetGroupMembershipByProvider(
    IN HANDLE     hProvider,
    IN PWSTR      pszSid,
    IN DWORD      dwGroupInfoLevel,
    OUT PDWORD    pdwGroupsCount,
    OUT PVOID   **pppMembershipInfo
    )
{


    DWORD dwError = 0;

    dwError = ADGetGroupMembershipByProvider(
                    hProvider,
                    pszSid,
                    &ppLsaSecurityObjects,
                    &dwNumObjects
                    );
    BAIL_ON_ERROR(dwError);

    return dwError;
}

