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
 *        Authentication Provider Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

DWORD
(*OPENHANDLE)(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phProvider
    );

VOID
(*CLOSEHANDLE)(
    HANDLE hProvider
    );

BOOLEAN
(*SERVICESDOMAIN) (
    PCSTR pszDomain
    );

DWORD
(*AUTHENTICATEUSER)(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
(*AUTHENTICATEUSEREX)(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    );

DWORD
(*VALIDATEUSER)(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
(*CHECKUSERINLIST)(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    );
DWORD
(*LOOKUPUSERBYNAME)(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
(*LOOKUPUSERBYID)(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
(*LOOKUPGROUPBYNAME)(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    LSA_FIND_FLAGS FindFlags,
    DWORD   dwUserInfoLevel,
    PVOID*  ppGroupInfo
    );

DWORD
(*LOOKUPGROUPBYID)(
    HANDLE  hProvider,
    gid_t   gid,
    LSA_FIND_FLAGS FindFlags,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{

}

DWORD
ActiveDirectoryGetGroupsForUser(
    HANDLE  hProvider,
    uid_t   uid,
    LSA_FIND_FLAGS
    FindFlags,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    dwError = MiniProviderGetGroupsForUser(
                    hMiniProvider,
                    uid,
                    FindFlags,
                    dwGroupInfoLevel,
                    pdwGroupsFound,
                    pppGroupInfoList
                    );
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
    DWORD dwError = 0;

    dwError = MiniProviderBeginEnumUsers(
                    hProvider,
                    dwConnectMode,
                    dwInfoLevel,
                    FindFlags,
                    phResume
                    );
    return (dwError);
}

DWORD
ActiveDirectoryEnumUsers(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    dwError = MiniProviderEnumUsers(
                    hProvider,
                    dwConnectMode,
                    dwInfoLevel,
                    FindFlags,
                    phResume
                    );

    return dwError;

}

VOID
ActiveDirectoryEndEnumUsers(
    HANDLE hProvider,
    HANDLE hResume
    )
{
    DWORD dwError = 0;

    dwError = MiniProviderEndEnumUsers(
                    hMiniProvider,
                    dwConnectMode,
                    hResume
                    );
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

    dwError = MiniProviderBeginEnumGroups(
                    hMiniProvider,
                    dwConnectMode,
                    dwInfoLevel,
                    bCheckGroupMembersOnline,
                    FindFlags,
                    phResume
                    );
    return dwError;
}

DWORD
ActiveDirectoryEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    dwError = MiniProviderEnumGroups(
                    hMiniProvider,
                    dwConnectState,
                    hResume,
                    dwMaxNumGroups,
                    pdwGroupsFound,
                    pppGroupInfoList
                    );
    return (dwError);
}

VOID
ActiveDirectoryEndEnumGroups
    HANDLE hProvider,
    HANDLE hResume
    )
{
    DWORD dwError = 0;

    dwError = MiniProviderEndEnumGroups(
                    hMiniProvider,
                    dwConnectState,
                    hResume
                    );
    return dwError;
}

DWORD
ActiveDirectoryChangePassword(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    )
{

}

DWORD
ActiveDirectoryAddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;

    return LSA_NOT_SUPPORTED;
}


DWORD
ActiveDirectoryModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{

}
DWORD
ActiveDirectoryDeleteUser(
    HANDLE hProvider,
    uid_t  uid
    );
DWORD
ActiveDirectoryAddGroup(
    HANDLE hProvider,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    );

DWORD
ActiveDirectoryDeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    )
{

}

DWORD
ActiveDirectoryOpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{

}

DWORD
ActiveDirectoryCloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    )
{

}

DWORD
ActiveDirectoryGetNamesBySidList(
    HANDLE hProvider,
    size_t sCount,
    PSTR*  ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes
    )
{

}

DWORD
(*LOOKUP_NSS_ARTEFACT_BY_KEY)(
    HANDLE hProvider,
    PCSTR  pszKeyName, PCSTR
    pszMapName, DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    );

DWORD
(*BEGIN_ENUM_NSS_ARTEFACTS)(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    );

DWORD
(*ENUMNSS_ARTEFACTS) (
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNumGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

VOID
(*END_ENUM_NSS_ARTEFACTS)( HANDLE hProvider, HANDLE hResume);

DWORD
(*GET_STATUS)( HANDLE hProvider, PLSA_AUTH_PROVIDER_STATUS* ppAuthProviderStatus);

VOID
(*FREE_STATUS)( PLSA_AUTH_PROVIDER_STATUS pAuthProviderStatus);

DWORD
(*REFRESH_CONFIGURATION)();

DWORD
(*PROVIDER_IO_CONTROL) ( HANDLE hProvider, uid_t  peerUid, gid_t  peerGID, DWORD  dwIoControlCode, DWORD  dwInputBufferSize, PVOID  pInputBuffer, DWORD* pdwOutputBufferSize, PVOID* ppOutputBuffer);

DWORD
(*INITIALIZEPROVIDER)( PCSTR pszConfigFilePath, PSTR* ppszProviderName, PLSA_PROVIDER_FUNCTION_TABLE* ppFnTable);

DWORD
(*SHUTDOWNPROVIDER)( PSTR pszProviderName, PLSA_PROVIDER_FUNCTION_TABLE pFnTable);


