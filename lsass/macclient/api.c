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
 *        api.c
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
#include "includes.h"

DWORD
LsaMacOpenServer(
    PHANDLE phConnection
    )
{
    return LsaOpenServer(phConnection);
}

DWORD
LsaMacGetGroupsForUserName(
    HANDLE  hLsaConnection,
    PCSTR   pszUserName,    
    PDWORD  pdwGroupFound,
    gid_t** ppGidResults
    )
{
    return LsaGetGroupsForUserName(
                    hLsaConnection,
                    pszUserName,
                    pdwGroupFound,
                    ppGidResults);
}

DWORD
LsaMacFindGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszGroupName,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    return LsaFindGroupByName(
                    hLsaConnection,
                    pszGroupName,
                    dwGroupInfoLevel,
                    ppGroupInfo);
}

DWORD
LsaMacFindGroupById(
    HANDLE hLsaConnection,
    gid_t  gid,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    return LsaFindGroupById(
                    hLsaConnection,
                    gid,
                    dwGroupInfoLevel,
                    ppGroupInfo);
}

DWORD
LsaMacBeginEnumGroups(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    PHANDLE phResume
    )
{
    return LsaBeginEnumGroups(
                    hLsaConnection,
                    dwGroupInfoLevel,
                    dwMaxNumGroups,
                    phResume);
}

DWORD
LsaMacEnumGroups(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupsInfoList
    )
{
    return LsaMacEnumGroups(
                    hLsaConnection,
                    hResume,
                    pdwNumGroupsFound,
                    pppGroupsInfoList);
}

DWORD
LsaMacEndEnumGroups(
    HANDLE  hLsaConnection,
    HANDLE  hResume
    )
{
    return LsaEndEnumGroups(
                    hLsaConnection,
                    hResume);
}

DWORD
LsaMacFindUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    return LsaFindUserByName(
                    hLsaConnection,
                    pszName,
                    dwUserInfoLevel,
                    ppUserInfo);
}

DWORD
LsaMacFindUserById(
    HANDLE hLsaConnection,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    return LsaFindUserById(
                    hLsaConnection,
                    uid,
                    dwUserInfoLevel,
                    ppUserInfo);
}

DWORD
LsaMacGetNamesBySidList(
    HANDLE          hLsaConnection,
    size_t          sCount,
    PSTR*           ppszSidList,
    PLSA_SID_INFO*  ppSIDInfoList
    )
{
    return LsaGetNamesBySidList(
                    hLsaConnection,
                    sCount,
                    ppszSidList,
                    ppSIDInfoList);
}

DWORD
LsaMacBeginEnumUsers(
    HANDLE  hLsaConnection,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    PHANDLE phResume
    )
{
    return LsaBeginEnumUsers(
                    hLsaConnection,
                    dwUserInfoLevel,
                    dwMaxNumUsers,
                    phResume);
}

DWORD
LsaMacEnumUsers(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    return LsaEnumUsers(
                    hLsaConnection,
                    hResume,
                    pdwNumUsersFound,
                    pppUserInfoList);
}

DWORD
LsaMacEndEnumUsers(
    HANDLE hLsaConnection,
    HANDLE hResume
    )
{
    return LsaEndEnumUsers(
                    hLsaConnection,
                    hResume);
}

DWORD
LsaMacAuthenticateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    )
{
    return LsaAuthenticateUser(
                    hLsaConnection,
                    pszLoginName,
                    pszPassword);
}

DWORD
LsaMacValidateUser(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszPassword
    )
{
    return LsaValidateUser(
                    hLsaConnection,
                    pszLoginName,
                    pszPassword);
}

DWORD
LsaMacChangePassword(
    HANDLE hLsaConnection,
    PCSTR  pszLoginName,
    PCSTR  pszNewPassword,
    PCSTR  pszOldPassword
    )
{
    return LsaChangePassword(
                    hLsaConnection,
                    pszLoginName,
                    pszNewPassword,
                    pszOldPassword);
}

DWORD
LsaMacOpenSession(
    HANDLE hLsaConnection,
    PCSTR  pszLoginId
    )
{
    return LsaOpenSession(
                    hLsaConnection,
                    pszLoginId);
}

DWORD
LsaMacCloseSession(
    HANDLE hLsaConnection,
    PCSTR  pszLoginId
    )
{
    return LsaCloseSession(
                    hLsaConnection,
                    pszLoginId);
}
    
VOID
LsaMacFreeGroupInfoList(
    DWORD  dwLevel,
    PVOID* pGroupInfoList,
    DWORD  dwNumGroups
    )
{
    LsaFreeGroupInfoList(
                    dwLevel,
                    pGroupInfoList,
                    dwNumGroups);
}

VOID
LsaMacFreeGroupInfo(
    DWORD dwLevel,
    PVOID pGroupInfo
    )
{
    LsaFreeGroupInfo(
                    dwLevel,
                    pGroupInfo);
}

VOID
LsaMacFreeUserInfoList(
    DWORD  dwLevel,
    PVOID* pUserInfoList,
    DWORD  dwNumUsers
    )
{
    LsaFreeUserInfoList(
                    dwLevel,
                    pUserInfoList,
                    dwNumUsers);
}

VOID
LsaMacFreeUserInfo(
    DWORD dwLevel,
    PVOID pUserInfo
    )
{
    LsaFreeUserInfo(
                    dwLevel,
                    pUserInfo);
}

VOID
LsaMacFreeSIDInfoList(
    PLSA_SID_INFO  ppSIDInfoList,
    size_t         stNumSID
    )
{
    LsaFreeSIDInfoList(
                    ppSIDInfoList,
                    stNumSID);
}

VOID
LsaMacFreeSIDInfo(
    PLSA_SID_INFO pSIDInfo
    )
{
    LsaFreeSIDInfo(pSIDInfo);
}

DWORD
LsaMacCloseServer(
    HANDLE hConnection
    )
{
    return LsaCloseServer(hConnection);
}
