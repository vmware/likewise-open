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
 *        db.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Local Authentication Provider
 * 
 *        User/Group Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __LSASSDB_H__
#define __LSASSDB_H__

void
LsaProviderLocal_DbInitGlobals(
    void
    );

DWORD
LsaProviderLocal_DbCreate(
    void
    );

DWORD
LsaProviderLocal_DbOpen(
    PHANDLE phDb
    );

void
LsaProviderLocal_DbClose(
    HANDLE hDb
    );

DWORD
LsaProviderLocal_DbFindGroupById(
    HANDLE hDb,
    gid_t  gid,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    );

DWORD
LsaProviderLocal_DbFindGroupByName(
    HANDLE hDb,
    PCSTR  pszDomain,
    PCSTR  pszGroupName,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    );

DWORD
LsaProviderLocal_DbGetGroupsForUser_0_Unsafe(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaProviderLocal_DbGetGroupsForUser_0(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaProviderLocal_DbGetGroupsForUser(
    HANDLE  hDb,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaProviderLocal_DbEnumGroups(
    HANDLE  hDb,
    DWORD   dwGroupInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaProviderLocal_DbFindUserById(
    HANDLE hDb,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaProviderLocal_DbFindUserByName(
    HANDLE hDb,
    PCSTR  pszDomain,
    PCSTR  pszUserName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaProviderLocal_DbEnumUsers(
    HANDLE  hDb,
    DWORD   dwUserInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LsaProviderLocal_DbChangePassword(
    HANDLE hDb,
    uid_t uid,
    PCSTR pszPassword
    );

DWORD
LsaProviderLocal_DbAddUser(
    HANDLE hDb,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    );

DWORD
LsaProviderLocal_DbModifyUser(
    HANDLE hDb,
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LsaProviderLocal_DbEnableUser(
    HANDLE hDb,
    uid_t  uid
    );
    
DWORD
LsaProviderLocal_DbDisableUser(
    HANDLE hDb,
    uid_t  uid
    );

DWORD
LsaProviderLocal_DbIsUserEnabled(
    HANDLE hDb,
    uid_t  uid,
    PBOOLEAN pbEnabled
    );
    
DWORD
LsaProviderLocal_DbUnlockUser(
    HANDLE hDb,
    uid_t  uid
    );

DWORD
LsaProviderLocal_DbLockUser(
    HANDLE hDb,
    uid_t  uid
    );

DWORD
LsaProviderLocal_DbIsUserLocked(
    HANDLE   hDb,
    uid_t    uid,
    PBOOLEAN pbLocked
    );

DWORD
LsaProviderLocal_DbAllowUserToChangePassword(
    HANDLE  hDb,
    uid_t   uid,
    BOOLEAN bAllow
    );

DWORD
LsaProviderLocal_DbCanUserChangePassword(
    HANDLE  hDb,
    uid_t   uid,
    PBOOLEAN pbCanChangePassword
    );

DWORD
LsaProviderLocal_DbSetPasswordExpires(
    HANDLE  hDb,
    uid_t   uid,
    BOOLEAN bPasswordExpires
    );

DWORD
LsaProviderLocal_DbCheckPasswordExpires(
    HANDLE   hDb,
    uid_t    uid,
    PBOOLEAN pbPasswordExpires
    );
    
DWORD
LsaProviderLocal_DbSetChangePasswordOnNextLogon(
    HANDLE hDb,
    uid_t  uid
    );
    
DWORD
LsaProviderLocal_DbSetAccountExpiryDate(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszExpiryDate
    );
    
DWORD
LsaProviderLocal_DbRemoveFromGroups(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszGroupList
    );
    
DWORD
LsaProviderLocal_DbAddToGroups(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszGroupList
    );

VOID
LsaProviderLocal_DbFreeGIDInList(
    PVOID pGID,
    PVOID pUserData
    );

DWORD
LsaProviderLocal_DbDeleteUser(
    HANDLE hDb,
    uid_t  uid
    );

DWORD
LsaProviderLocal_DbAddGroup(
    HANDLE hDb,
    PCSTR  pszDomain,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    );

DWORD
LsaProviderLocal_DbDeleteGroup(
    HANDLE hDb,
    gid_t  gid
    );

DWORD
LsaProviderLocal_DbRemoveGroupMembership_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    gid_t  gid
    );

DWORD
LsaProviderLocal_DbAddGroupMembership_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    gid_t  gid
    );

DWORD
LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    PDWORD pdwUserInfoFlags
    );

DWORD
LsaProviderLocal_DbSetUserInfoFlags_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    DWORD  dwUserInfoFlags
    );

DWORD
LsaProviderLocal_DbGetUserCount(
    HANDLE hDb,
    PINT pUserCount);

DWORD
LsaProviderLocal_DbGetGroupCount(
    HANDLE hDb,
    PINT pGroupCount);

#endif /* __LSASSDB_H__ */

