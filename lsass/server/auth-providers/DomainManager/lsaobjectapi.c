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


#include "stdafx.h"


DWORD
ADOpenHandle(
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
ADCloseHandle(
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
ADServicesDomain(
    PWSTR pszDomain
    )
{
	BOOLEAN bResult = FALSE;
    return bResult;
}

DWORD
ADAuthenticateUser(
    HANDLE hProvider,
    PWSTR  pszLoginId,
    PWSTR  pszPassword
    )
{

    return dwError;
}

DWORD
ADAuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    )
{
}

DWORD
ADValidateUser(
    HANDLE hProvider,
    PWSTR  pszLoginId,
    PWSTR  pszPassword
    )
{
    return dwError;

error:

    goto cleanup;
}

DWORD
ADCheckUserInList(
    HANDLE hProvider,
    PWSTR  pszUserName,
    PWSTR  pszListName
    )
{
    DWORD  dwError = 0;

    return dwError;

}

DWORD
ADFindUserByName(
    HANDLE  hProvider,
    PWSTR   pszUserName,
	PLSA_SECURITY_OBJECT *ppLsaSecurityObject

    )
{
    DWORD   dwError = 0;

	return dwError;
}

DWORD
ADFindUserById(
    HANDLE  hProvider,
    uid_t   uid,
	PLSA_SECURITY_OBJECT *ppLsaSecurityObject
    )
{
    DWORD   dwError = 0;

}


DWORD
ADBeginEnumUsers(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{

}

DWORD
ADEnumUsers(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumUsers,
    PDWORD  pdwUsersFound,
    PLSA_SECURITY_OBJECT * ppLsaSecurityObjects
    )
{
    DWORD dwError = 0;

	return dwError;
}

VOID
ADEndEnumUsers(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    return;
}



DWORD
ADFindGroupByName(
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
ADGetExpandedGroupUsersEx(
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
ADFindGroupById(
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
ADGetUserGroupObjectMembership(
    IN HANDLE hProvider,
    IN PLSA_SECURITY_OBJECT pUserInfo,
    IN BOOLEAN bIsCacheOnlyMode,
    OUT size_t* psNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppResult
    )
{
    DWORD dwError = 0;
}

DWORD
ADGetGroupsForUser(
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
ADBeginEnumGroups(
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
ADEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxGroups,
	PLSA_SECURITY_OBJECT *ppLsaSecurityObjects,
    PDWORD  pdwGroupsFound
    )
{
    DWORD dwError = 0;
}

VOID
ADEndEnumGroups(
    HANDLE hProvider,
    HANDLE hResume
    )
{
	return;
}

DWORD
ADChangePassword(
    HANDLE hProvider,
    PWSTR pszLoginId,
    PWSTR pszPassword,
    PWSTR pszOldPassword
    )
{

    return dwError;
}

DWORD
ADSetPassword(
    HANDLE hProvider,
    PWSTR pszLoginId,
    PWSTR pszPassword
    )
{
    return LW_ERROR_NOT_HANDLED;
}


DWORD
ADAddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADDeleteUser(
    HANDLE hProvider,
    uid_t  uid
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADAddGroup(
    HANDLE hProvider,
    DWORD dwGroupInfoLevel,
    PVOID pGroupInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADDeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{
    return LW_ERROR_NOT_HANDLED;
}

DWORD
ADOpenSession(
    HANDLE hProvider,
    PWSTR  pszUserName
    )
{
    DWORD dwError = 0;
}

DWORD
ADCloseSession(
    HANDLE hProvider,
    PWSTR  pszUserName
    )
{
}

DWORD
ADGetNamesBySidList(
    HANDLE hProvider,
	PWSTR *ppszSidList,
	DWORD dwCount,
	PLSA_SECURITY_OBJECT * ppLsaSecurityObjects,
	DWORD * pdwCount
    )
{
}

DWORD
ADFindNSSArtefactByKey(
    HANDLE hProvider,
    PWSTR  pszKeyName,
    PWSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{
}

DWORD
ADBeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PWSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{
}

DWORD
ADEnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    )
{
}

VOID
ADEndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    )
{

}

DWORD
ADGetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    )
{
}



DWORD
ADRefreshConfiguration(
    HANDLE hProvider
    )
{
}

DWORD
ADProviderIoControl(
    HANDLE  hProvider,
    uid_t   peerUID,
    uid_t   peerGID,
    DWORD   dwIoControlCode,
    DWORD   dwInputBufferSize,
    PVOID   pInputBuffer,
    DWORD* pdwOutputBufferSize,
    PVOID* ppOutputBuffer
    )
{
}

DWORD
ADGetGroupMembershipByProvider(
    HANDLE     hProvider,
    PWSTR      pszSid,
	PLSA_SECURITY_OBJECT * ppLsaSecurityObjects,
    PDWORD    pdwGroupsCount
    )
{
    return LW_ERROR_NOT_HANDLED;
}


DWORD
ADFindUserObjectByName(
    IN HANDLE  hProvider,
    IN PWSTR   pszLoginId,
    OUT PLSA_SECURITY_OBJECT* ppResult
    )
{
    DWORD dwError = 0;

	return dwError;
}

DWORD
ADFindGroupObjectByName(
    HANDLE  hProvider,
    PWSTR   pszGroupName,
    PLSA_SECURITY_OBJECT* ppResult
    )
{
	DWORD dwError = 0;

	return dwError;
}

