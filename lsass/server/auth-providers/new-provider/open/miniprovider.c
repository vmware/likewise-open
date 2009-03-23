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
 *        lsaprovider.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Mini Provider Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Arlene Berry (aberry@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

DWORD
LikewiseOpenOpenMiniProvider
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phMiniProvider
    )
{
    DWORD dwError = 0;

    return dwError;

}

VOID
LikewiseOpenCloseHandle
    HANDLE hMiniProvider
    )
{


}

DWORD
(*AUTHENTICATEUSER)(
    HANDLE hMiniProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
(*AUTHENTICATEUSEREX)(
    HANDLE hMiniProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    );

DWORD
(*CHECKUSERINLIST)(
    HANDLE hMiniProvider,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    );
DWORD
(*LOOKUPUSERBYNAME)(
    HANDLE  hMiniProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
LikewiseOpenLookupUserById(
    HANDLE  hMiniProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{

}

DWORD
LikewiseOpenLookupGroupByName(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    PCSTR   pszLoginId,
    LSA_FIND_FLAGS FindFlags,
    DWORD   dwUserInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    return (dwError);
}

DWORD
LikewiseOpenLookupGroupById(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    gid_t   gid,
    LSA_FIND_FLAGS FindFlags,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LikewiseOpenGetGroupsForUser(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    uid_t   uid,
    LSA_FIND_FLAGS
    FindFlags,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LikewiseOpenBeginEnumUsers(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    DWORD   dwInfoLevel,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

                    );
    return (dwError);
}

DWORD
LikewiseOpenEnumUsers(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    HANDLE  hResume,
    DWORD   dwMaxUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

VOID
LikewiseOpenEndEnumUsers(
    HANDLE hMiniProvider,
    DWORD dwConnectMode,
    HANDLE hResume
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LikewiseOpenBeginEnumGroups(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
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
LikewiseOpenEnumGroups(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    HANDLE  hResume,
    DWORD   dwMaxNumGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return (dwError);
}

VOID
LikewiseOpenEndEnumGroups
    HANDLE hMiniProvider,
    DWORD dwConnectMode,
    HANDLE hResume
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LikewiseOpenChangePassword(
    HANDLE hMiniProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    )
{

}

DWORD
LikewiseOpenGetNamesBySidList(
    HANDLE hMiniProvider,
    DWORD dwConnectMode,
    size_t sCount,
    PSTR*  ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes
    )
{

}

DWORD
LikewiseOpenLookupNssArtefactByKey
    HANDLE hMiniProvider,
    DWORD dwConnectMode,
    PCSTR  pszKeyName, PCSTR
    pszMapName, DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    )
{


}

DWORD
LikewiseOpenBeginEnumNssArtefacts(
    HANDLE  hMiniProvider,
    DWORD   dwConnectMode,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    )
{


}

DWORD
LikewiseOpenEnumNssArtefacts(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    HANDLE  hResume,
    DWORD   dwMaxNumGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{

}

VOID
LikewiseOpenEndEnumNssArtefacts(
    HANDLE hMiniProvider,
    DWORD dwConnectMode,
    HANDLE hResume
    )
{

}

DWORD
InitializeMiniProvider(
    PCSTR pszConfigFilePath,
    PSTR* ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFnTable
    )
{

}

DWORD
ShutdownMiniProvider(
    PSTR pszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
    )
{

}


