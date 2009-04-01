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
LsaLPDbInitGlobals(
    void
    );

DWORD
LsaLPDbCreate(
    void
    );

DWORD
LsaLPDbOpen(
    PHANDLE phDb
    );

void
LsaLPDbClose(
    HANDLE hDb
    );

DWORD
LsaLPDbFindGroupById(
    HANDLE hDb,
    gid_t  gid,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    );

DWORD
LsaLPDbFindGroupByName(
    HANDLE hDb,
    PCSTR  pszDomain,
    PCSTR  pszGroupName,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    );

DWORD
LsaLPDbGetGroupsForUser_0_Unsafe(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaLPDbGetGroupsForUser_0(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaLPDbGetGroupsForUser(
    HANDLE  hDb,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaLPDbEnumGroups(
    HANDLE  hDb,
    DWORD   dwGroupInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    );

DWORD
LsaLPDbFindUserById(
    HANDLE hDb,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaLPDbFindUserByName(
    HANDLE hDb,
    PCSTR  pszDomain,
    PCSTR  pszUserName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LsaLPDbEnumUsers(
    HANDLE  hDb,
    DWORD   dwUserInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LsaLPDbChangePassword(
    HANDLE hDb,
    uid_t uid,
    PCSTR pszPassword
    );

DWORD
LsaLPDbAddUser(
    HANDLE hDb,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    );

DWORD
LsaLPDbModifyUser(
    HANDLE hDb,
    PLSA_USER_MOD_INFO pUserModInfo
    );

DWORD
LsaLPDbEnableUser(
    HANDLE hDb,
    uid_t  uid
    );
    
DWORD
LsaLPDbDisableUser(
    HANDLE hDb,
    uid_t  uid
    );

DWORD
LsaLPDbIsUserEnabled(
    HANDLE hDb,
    uid_t  uid,
    PBOOLEAN pbEnabled
    );
    
DWORD
LsaLPDbUnlockUser(
    HANDLE hDb,
    uid_t  uid
    );

DWORD
LsaLPDbLockUser(
    HANDLE hDb,
    uid_t  uid
    );

DWORD
LsaLPDbIsUserLocked(
    HANDLE   hDb,
    uid_t    uid,
    PBOOLEAN pbLocked
    );

DWORD
LsaLPDbAllowUserToChangePassword(
    HANDLE  hDb,
    uid_t   uid,
    BOOLEAN bAllow
    );

DWORD
LsaLPDbCanUserChangePassword(
    HANDLE  hDb,
    uid_t   uid,
    PBOOLEAN pbCanChangePassword
    );

DWORD
LsaLPDbSetPasswordExpires(
    HANDLE  hDb,
    uid_t   uid,
    BOOLEAN bPasswordExpires
    );

DWORD
LsaLPDbCheckPasswordExpires(
    HANDLE   hDb,
    uid_t    uid,
    PBOOLEAN pbPasswordExpires
    );
    
DWORD
LsaLPDbSetChangePasswordOnNextLogon(
    HANDLE hDb,
    uid_t  uid
    );
    
DWORD
LsaLPDbSetAccountExpiryDate(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszExpiryDate
    );
    
DWORD
LsaLPDbRemoveFromGroups(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszGroupList
    );
    
DWORD
LsaLPDbAddToGroups(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszGroupList
    );

VOID
LsaLPDbFreeGIDInList(
    PVOID pGID,
    PVOID pUserData
    );

DWORD
LsaLPDbDeleteUser(
    HANDLE hDb,
    uid_t  uid
    );

DWORD
LsaLPDbAddGroup(
    HANDLE hDb,
    PCSTR  pszDomain,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    );

DWORD
LsaLPDbDeleteGroup(
    HANDLE hDb,
    gid_t  gid
    );

DWORD
LsaLPDbRemoveGroupMembership_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    gid_t  gid
    );

DWORD
LsaLPDbAddGroupMembership_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    gid_t  gid
    );

DWORD
LsaLPDbGetUserInfoFlags_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    PDWORD pdwUserInfoFlags
    );

DWORD
LsaLPDbSetUserInfoFlags_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    DWORD  dwUserInfoFlags
    );

DWORD
LsaLPDbGetUserCount(
    HANDLE hDb,
    PINT pUserCount);

DWORD
LsaLPDbGetGroupCount(
    HANDLE hDb,
    PINT pGroupCount);

#endif /* __LSASSDB_H__ */

