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
 *        lpuser.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        User Management Routines
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LP_GROUP_H__
#define __LP_GROUP_H__

DWORD
LocalDirFindGroupByName(
    HANDLE  hProvider,
    PCSTR   pszDomainName,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PWSTR*  ppwszGroupDN,
    PVOID*  ppGroupInfo
    );

DWORD
LocalDirFindGroupByName_0(
    HANDLE  hProvider,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PWSTR*  ppwszGroupDN,
    PVOID*  ppGroupInfo
    );

DWORD
LocalDirFindGroupByName_1(
    HANDLE  hProvider,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PWSTR*  ppwszGroupDN,
    PVOID*  ppGroupInfo
    );

DWORD
LocalDirFindGroupById(
    HANDLE  hProvider,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PWSTR*  ppwszGroupDN,
    PVOID*  ppGroupInfo
    );

DWORD
LocalDirFindGroupById_0(
    HANDLE hProvider,
    gid_t  gid,
    PWSTR* ppwszGroupDN,
    PVOID* ppGroupInfo
    );

DWORD
LocalDirFindGroupById_1(
    HANDLE hProvider,
    gid_t  gid,
    PWSTR* ppwszGroupDN,
    PVOID* ppGroupInfo
    );

DWORD
LocalDirGetGroupsForUser(
    HANDLE  hProvider,
    PWSTR   pwszUserDN,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LocalDirGetGroupsForUser_0(
    HANDLE  hProvider,
    PWSTR   pwszUserDN,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LocalDirGetGroupsForUser_1(
    HANDLE  hProvider,
    PWSTR   pwszUserDN,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LocalDirBeginEnumGroups(
    HANDLE  hProvider,
    DWORD   dwInfoLevel,
    PHANDLE phResume
    );

DWORD
LocalDirEnumGroups(
    HANDLE  hProvider,
    HANDLE  hResume,
    DWORD   nMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LocalDirGetGroupMemberNames(
    PLOCAL_PROVIDER_CONTEXT pContext,
    PWSTR                   pwszGroupDN,
    PSTR**                  pppszMembers
    );

DWORD
LocalDirGetGroupMembers(
    PLOCAL_PROVIDER_CONTEXT        pContext,
    PWSTR                          pwszGroupDN,
    PLOCAL_PROVIDER_GROUP_MEMBER** pppGroupMembers,
    PDWORD                         pdwNumMembers
    );

DWORD
LocalDirAddGroup(
    HANDLE hProvider,
    DWORD  dwInfoLevel,
    PVOID  pGroupInfo
    );

DWORD
LocalDirAddGroup_0(
    HANDLE            hProvider,
    PLSA_GROUP_INFO_0 pGroupInfo
    );

DWORD
LocalDirAddGroup_1(
    HANDLE            hProvider,
    PLSA_GROUP_INFO_1 pGroupInfo
    );

DWORD
LocalDirModifyGroup(
    HANDLE hProvider,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    );

DWORD
LocalDirDeleteGroup(
    HANDLE hProvider,
    PWSTR  pwszGroupDN
    );

VOID
LocalDirFreeGroupMemberList(
    PLOCAL_PROVIDER_GROUP_MEMBER* ppMemberList,
    DWORD                         dwNumMembers
    );

VOID
LocalDirFreeGroupMember(
    PLOCAL_PROVIDER_GROUP_MEMBER pMember
    );

DWORD
LocalDirGetGroupMembershipByProvider(
    IN HANDLE    hProvider,
    IN PCSTR     pszSid,
    IN DWORD     dwGroupInfoLevel,
    OUT PDWORD   pdwGroupsCount,
    OUT PVOID  **pppMembershipInfo
    );

#endif /* __LP_GROUP_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
