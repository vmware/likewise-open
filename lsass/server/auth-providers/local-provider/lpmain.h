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
 *        lpmain.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Main
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LPMAIN_H__
#define __LPMAIN_H__

DWORD
LsaInitializeProvider(
    PCSTR                         pszConfigFilePath,
    PSTR*                         ppszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE* ppFunctionTable
    );

DWORD
LocalOpenHandle(
    uid_t uid,
    gid_t gid,
    PHANDLE phProvider
    );

VOID
LocalCloseHandle(
    HANDLE hProvider
    );

BOOLEAN
LocalServicesDomain(
    PCSTR pszDomain
    );

DWORD
LocalAuthenticateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
LocalAuthenticateUserEx(
    HANDLE hProvider,
    PLSA_AUTH_USER_PARAMS pUserParams,
    PLSA_AUTH_USER_INFO *ppUserInfo
    );

DWORD
LocalValidateUser(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
LocalCheckUserInList(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    );

DWORD
LocalFindUserByName(
    HANDLE  hProvider,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
LocalFindUserById(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
LocalBeginEnumUsers(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    );

DWORD
LocalEnumUsers(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxNumRecords,
    PDWORD   pdwUsersFound,
    PVOID**  pppUserInfoList
    );

VOID
LocalEndEnumUsers(
    HANDLE hProvider,
    HANDLE hResume
    );

DWORD
LocalFindGroupByName(
    IN HANDLE hProvider,
    IN PCSTR pszGroupName,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    );

DWORD
LocalFindGroupById(
    IN HANDLE hProvider,
    IN gid_t gid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PVOID* ppGroupInfo
    );

DWORD
LocalGetGroupsForUser(
    IN HANDLE hProvider,
    IN uid_t uid,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    IN PDWORD pdwGroupsFound,
    IN PVOID** pppGroupInfoList
    );

DWORD
LocalBeginEnumGroups(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    BOOLEAN bCheckOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    );

DWORD
LocalEnumGroups(
    HANDLE   hProvider,
    HANDLE   hResume,
    DWORD    dwMaxGroups,
    PDWORD   pdwGroupsFound,
    PVOID**  pppGroupInfoList
    );

VOID
LocalEndEnumGroups(
    HANDLE hProvider,
    HANDLE hResume
    );

DWORD
LocalChangePassword(
    HANDLE hProvider,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    );

DWORD
LocalAddUser(
    HANDLE hProvider,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    );

DWORD
LocalModifyUser(
    HANDLE hProvider,
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LocalDeleteUser(
    HANDLE hProvider,
    uid_t  uid
    );

DWORD
LocalAddGroup(
    HANDLE hProvider,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    );

DWORD
LocalDeleteGroup(
    HANDLE hProvider,
    gid_t  gid
    );

DWORD
LocalOpenSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    );

DWORD
LocalCloseSession(
    HANDLE hProvider,
    PCSTR  pszLoginId
    );

DWORD
LsaShutdownProvider(
    PSTR pszProviderName,
    PLSA_PROVIDER_FUNCTION_TABLE pFnTable
    );

DWORD
LocalGetNamesBySidList(
    HANDLE          hProvider,
    size_t          sCount,
    PSTR*           ppszSidList,
    PSTR**          pppszDomainNames,
    PSTR**          pppszSamAccounts,
    ADAccountType** ppTypes);

DWORD
LocalFindNSSArtefactByKey(
    HANDLE hProvider,
    PCSTR  pszKeyName,
    PCSTR  pszMapName,
    DWORD  dwInfoLevel,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PVOID* ppNSSArtefactInfo
    );

DWORD
LocalBeginEnumNSSArtefacts(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PCSTR   pszMapName,
    LSA_NIS_MAP_QUERY_FLAGS dwFlags,
    PHANDLE phResume
    );

DWORD
LocalEnumNSSArtefacts(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   dwMaxNSSArtefacts,
    PDWORD  pdwNSSArtefactsFound,
    PVOID** pppNSSArtefactInfoList
    );

VOID
LocalEndEnumNSSArtefacts(
    HANDLE hProvider,
    HANDLE hResume
    );

DWORD
LocalGetStatus(
    HANDLE hProvider,
    PLSA_AUTH_PROVIDER_STATUS* ppProviderStatus
    );

VOID
LocalFreeStatus(
    PLSA_AUTH_PROVIDER_STATUS pProviderStatus
    );

DWORD
LocalRefreshConfiguration(
    HANDLE hProvider
    );

DWORD
LocalIoControl(
    IN HANDLE  hProvider,
    IN uid_t   peerUID,
    IN uid_t   peerGID,
    IN DWORD   dwIoControlCode,
    IN DWORD   dwInputBufferSize,
    IN PVOID   pInputBuffer,
    OUT DWORD* pdwOutputBufferSize,
    OUT PVOID* ppOutputBuffer
    );

#endif /* __PROVIDER_MAIN_H__ */
