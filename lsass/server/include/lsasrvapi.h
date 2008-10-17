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
 *        lsasrvapi.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Server API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSASRVAPI_H__
#define __LSASRVAPI_H__

DWORD
LsaSrvApiInit(
    PCSTR pszConfigFilePath
    );

DWORD
LsaSrvApiShutdown(
    VOID
    );

DWORD
LsaSrvOpenServer(
    uid_t peerUID,
    gid_t peerGID,
    PHANDLE phServer
    );

void
LsaSrvCloseServer(
    HANDLE hServer
    );

DWORD
LsaSrvAuthenticateUser(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
LsaSrvValidateUser(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword
    );

DWORD
LsaSrvCheckUserInList(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    );

DWORD
LsaSrvChangePassword(
    HANDLE hServer,
    PCSTR  pszLoginId,
    PCSTR  pszPassword,
    PCSTR  pszOldPassword
    );

DWORD
LsaSrvFindGroupByName(
    HANDLE  hServer,
    PCSTR   pszGroupname,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );

DWORD
LsaSrvFindGroupById(
    HANDLE  hServer,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );

DWORD
LsaSrvGetGroupsForUser(
    HANDLE hServer,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaSrvFindUserByName(
    HANDLE  hServer,
    PCSTR   pszUsername,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
LsaSrvAddGroup(
    HANDLE hServer,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    );

DWORD
LsaSrvDeleteGroup(
    HANDLE hServer,
    gid_t  gid
    );

DWORD
LsaSrvAddUser(
    HANDLE hServer,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    );

DWORD
LsaSrvModifyUser(
    HANDLE hServer,
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LsaSrvDeleteUser(
    HANDLE hServer,
    uid_t  uid
    );

DWORD
LsaSrvFindUserById(
    HANDLE hServer,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaSrvBeginEnumUsers(
    HANDLE hServer,
    DWORD  dwUserInfoLevel,
    DWORD  dwNumMaxUsers,
    PSTR*  ppszGUID
    );

DWORD
LsaSrvEnumUsers(
    HANDLE  hServer,
    PCSTR   pszGUID,
    PDWORD  pdwUserInfoLevel,
    PVOID** pppUserInfoList,
    PDWORD  pdwNumUsersFound
    );

DWORD
LsaSrvEndEnumUsers(
    HANDLE hServer,
    PCSTR  pszGUID
    );

DWORD
LsaSrvBeginEnumGroups(
    HANDLE hServer,
    DWORD  dwGroupInfoLevel,
    DWORD  dwNumMaxGroups,
    PSTR*  ppszGUID
    );

DWORD
LsaSrvEnumGroups(
    HANDLE  hServer,
    PCSTR   pszGUID,
    PDWORD  pdwGroupInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroupsFound
    );

DWORD
LsaSrvEndEnumGroups(
    HANDLE hServer,
    PCSTR  pszGUID
    );

DWORD
LsaSrvBeginEnumNSSArtefacts(
    HANDLE hServer,
    DWORD  dwMapType,
    DWORD  dwGroupInfoLevel,
    DWORD  dwNumMaxGroups,
    PSTR*  ppszGUID
    );

DWORD
LsaSrvEnumNSSArtefacts(
    HANDLE  hServer,
    PCSTR   pszGUID,
    PDWORD  pdwGroupInfoLevel,
    PVOID** pppGroupInfoList,
    PDWORD  pdwNumGroupsFound
    );

DWORD
LsaSrvEndEnumNSSArtefacts(
    HANDLE hServer,
    PCSTR  pszGUID
    );

DWORD
LsaSrvComputeLMHash(
    PCSTR pszPassword,
    PBYTE* ppszHash,
    PDWORD pdwHashLen
    );

DWORD
LsaSrvComputeNTHash(
    PCSTR pszPassword,
    PBYTE* ppszHash,
    PDWORD pdwHashLen
    );

DWORD
LsaSrvOpenSession(
    HANDLE hServer,
    PCSTR  pszLoginId
    );

DWORD
LsaSrvCloseSession(
    HANDLE hServer,
    PCSTR  pszLoginId
    );

DWORD
LsaSrvGetNamesBySidList(
    HANDLE hServer,
    size_t sCount,
    PSTR* ppszSidList,
    PSTR** pppszDomainNames,
    PSTR** pppszSamAccounts,
    ADAccountType **ppTypes);

DWORD
LsaSrvGetLogInfo(
    HANDLE hServer,
    PLSA_LOG_INFO* ppLogInfo
    );

DWORD
LsaSrvSetLogInfo(
    HANDLE hServer,
    PLSA_LOG_INFO pLogInfo
    );

DWORD
LsaSrvGetMetrics(
    HANDLE hServer,
    DWORD  dwInfoLevel,
    PVOID* ppMetricPack
    );

DWORD
LsaSrvGetStatus(
    HANDLE hServer,
    PLSASTATUS* ppLsaStatus
    );

DWORD
LsaSrvRefreshConfiguration(
    HANDLE hServer
    );

#endif /* __LSASRVAPI_H__ */

