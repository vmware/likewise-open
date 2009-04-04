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
#ifndef __LP_USER_H__
#define __LP_USER_H__

DWORD
LocalDirFindUserByName(
    HANDLE  hProvider,
    PCSTR   pszNetBIOSName,
    PCSTR   pszUserName,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    );

DWORD
LocalDirFindUserByName_0(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    );

DWORD
LocalDirFindUserByName_1(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    );

DWORD
LocalDirFindUserByName_2(
    HANDLE hProvider,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    );

DWORD
LocalDirFindUserById(
    HANDLE hProvider,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    );

DWORD
LocalDirFindUserById_0(
    HANDLE hProvider,
    uid_t  uid,
    PVOID* ppUserInfo
    );

DWORD
LocalDirFindUserById_1(
    HANDLE hProvider,
    uid_t  uid,
    PVOID* ppUserInfo
    );

DWORD
LocalDirFindUserById_2(
    HANDLE hProvider,
    uid_t  uid,
    PVOID* ppUserInfo
    );

DWORD
LocalDirEnumUsers(
    HANDLE  hProvider,
    DWORD   dwUserInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxUsers,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LocalDirEnumUsers_0(
    HANDLE hProvider,
    DWORD  dwOffset,
    DWORD  dwLimit,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LocalDirEnumUsers_1(
    HANDLE hProvider,
    DWORD  dwOffset,
    DWORD  dwLimit,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LocalDirEnumUsers_2(
    HANDLE  hLocal,
    DWORD   dwOffset,
    DWORD   dwLimit,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    );

DWORD
LocalDirAddUser(
    HANDLE hProvider,
    DWORD  dwInfoLevel,
    PVOID  pUserInfo
    );

DWORD
LocalDirAddUser_0(
    PLSA_USER_INFO_0     pUserInfo
    );

DWORD
LocalDirAddUser_1(
    PLSA_USER_INFO_1     pUserInfo
    );

DWORD
LocalDirAddUser_2(
    PLSA_USER_INFO_2     pUserInfo
    );

DWORD
LocalCreateHomeDirectory(
    PLSA_USER_INFO_0 pUserInfo
    );

DWORD
LocalProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    );

DWORD
LocalCheckAccountFlags(
    PLSA_USER_INFO_2 pUserInfo2
    );

#endif /* __LP_USER_H__ */
