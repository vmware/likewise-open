/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        api.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        LSA Client API (for Mac OS X)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __API_H__
#define __API_H__

DWORD
LsaMacOpenServer(
    PHANDLE phConnection
    );

DWORD
LsaMacGetGroupsForUserName(
    HANDLE  hLsaConnection,
    PCSTR   pszUserName,    
    PDWORD  pdwGroupFound,
    gid_t** ppGidResults
    );

DWORD
LsaMacFindGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszGroupName,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    );

DWORD
LsaMacFindGroupById(
    HANDLE hLsaConnection,
    gid_t  gid,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    );

DWORD
LsaMacBeginEnumGroups(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    PHANDLE phResume
    );

DWORD
LsaMacEnumGroups(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupsInfoList
    );

DWORD
LsaMacEndEnumGroups(
    HANDLE  hLsaConnection,
    HANDLE  hResume
    );

DWORD
LsaMacFindUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaMacFindUserById(
    HANDLE hLsaConnection,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaMacGetNamesBySidList(
    HANDLE          hLsaConnection,
    size_t          sCount,
    PSTR*           ppszSidList,
    PLSA_SID_INFO*  ppSIDInfoList
    );

DWORD
LsaMacBeginEnumUsers(
    HANDLE  hLsaConnection,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    PHANDLE phResume
    );

DWORD
LsaMacEnumUsers(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LsaMacEndEnumUsers(
    HANDLE hLsaConnection,
    HANDLE hResume
    );

DWORD
LsaMacAuthenticateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    );

DWORD
LsaMacValidateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    );

DWORD
LsaMacChangePassword(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszNewPassword,
    PCSTR  pszOldPassword
    );

DWORD
LsaMacOpenUserLogonSession(
    HANDLE hLsaConnection,
    PCSTR  pszLoginId
    );

DWORD
LsaMacCloseUserLogonSession(
    HANDLE hLsaConnection,
    PCSTR  pszLoginId
    );
    
VOID
LsaMacFreeGroupInfoList(
    DWORD  dwLevel,
    PVOID* pGroupInfoList,
    DWORD  dwNumGroups
    );

VOID
LsaMacFreeGroupInfo(
    DWORD dwLevel,
    PVOID pGroupInfo
    );

VOID
LsaMacFreeUserInfoList(
    DWORD  dwLevel,
    PVOID* pUserInfoList,
    DWORD  dwNumUsers
    );

VOID
LsaMacFreeUserInfo(
    DWORD dwLevel,
    PVOID pUserInfo
    );
VOID
LsaMacFreeSIDInfoList(
    PLSA_SID_INFO  ppSIDInfoList,
    size_t         stNumSID
    );

VOID
LsaMacFreeSIDInfo(
    PLSA_SID_INFO pSIDInfo
    );

DWORD
LsaMacCloseServer(
    HANDLE hConnection
    );

#endif /* __API_H__ */

