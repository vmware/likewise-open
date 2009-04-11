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
    PVOID*  ppGroupInfo
    );

DWORD
LocalDirFindGroupByName_0(
    HANDLE  hProvider,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );

DWORD
LocalDirGetGroupsForUser(
    HANDLE  hProvider,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LocalDirFindGroupByName_1(
    HANDLE  hProvider,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );

DWORD
LocalDirGetGroupsForUser_0(
    HANDLE  hProvider,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );


DWORD
LocalDirGetGroupsForUser_1(
    HANDLE  hProvider,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LocalDirFindGroupByName_0(
    HANDLE  hProvider,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    );

DWORD
LocalDirFindGroupByName_1(
    HANDLE  hProvider,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
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
LocalDirFindGroupById(
    HANDLE  hProvider,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    );

DWORD
LocalDirFindGroupById_0(
    HANDLE hProvider,
    gid_t  gid,
    PVOID* ppGroupInfo
    );

DWORD
LocalDirFindGroupById_1(
    HANDLE hProvider,
    gid_t  gid,
    PVOID* ppGroupInfo
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
LocalDirDeleteGroup(
    HANDLE hProvider,
    PWSTR  pwszGroupDN
    );

#endif /* __LP_GROUP_H__ */

