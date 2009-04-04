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
 *        lsassdb.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Local Authentication Provider
 *
 *        Directory User Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

DWORD
LocalDirFindUserByName(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszUserName,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;

    switch(dwUserInfoLevel)
    {
        case 0:

            dwError = LocalDirFindUserByName_0(
                            hDb,
                            pszUserName,
                            ppUserInfo
                            );
            break;

        case 1:

            dwError = LocalDirFindUserByName_1(
                            hDb,
                            pszUserName,
                            ppUserInfo
                            );
            break;

        case 2:

            dwError = LocalDirFindUserByName_2(
                            hDb,
                            pszUserName,
                            ppUserInfo
                            );
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirFindUserByName_0(
    HANDLE hDb,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
LocalDirFindUserByName_1(
    HANDLE hDb,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
LocalDirFindUserByName_2(
    HANDLE hDb,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirFindUserById(
    HANDLE hLocal,
    uid_t  uid,
    DWORD  dwInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

    switch(dwInfoLevel)
    {
        case 0:

            dwError = LocalDirFindUserById_0(hLocal, uid, ppUserInfo);

            break;

        case 1:

            dwError = LocalDirFindUserById_1(hLocal, uid, ppUserInfo);

            break;

        case 2:

            dwError = LocalDirFindUserById_2(hLocal, uid, ppUserInfo);

            break;
    }

    return dwError;
}

DWORD
LocalDirFindUserById_0(
    HANDLE hLocal,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirFindUserById_1(
    HANDLE hLocal,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirFindUserById_2(
    HANDLE hLocal,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirEnumUsers(
    HANDLE  hLocal,
    DWORD   dwInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxUsers,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

    switch (dwInfoLevel)
    {
        case 0:
        {
            dwError = LocalDirEnumUsers_0(
                                hLocal,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
        case 1:
        {
            dwError = LocalDirEnumUsers_1(
                                hLocal,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
        case 2:
        {
            dwError = LocalDirEnumUsers_2(
                                hLocal,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
    }

    return dwError;
}

DWORD
LocalDirEnumUsers_0(
    HANDLE  hLocal,
    DWORD   dwOffset,
    DWORD   dwLimit,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirEnumUsers_1(
    HANDLE hLocal,
    DWORD  dwOffset,
    DWORD  dwLimit,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
LocalDirEnumUsers_2(
    HANDLE  hLocal,
    DWORD   dwOffset,
    DWORD   dwLimit,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirGetGroupsForUser(
    HANDLE  hLocal,
    uid_t   uid,
    DWORD   dwInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

    switch(dwInfoLevel)
    {
        case 0:
        {
            dwError = LocalDirGetGroupsForUser_0(
                                hLocal,
                                uid,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;
        }
        case 1:
        {
            dwError = LocalDirGetGroupsForUser_1(
                                hLocal,
                                uid,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;
        }

    }

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_0_Unsafe(
    HANDLE  hLocal,
    uid_t   uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_1_Unsafe(
    HANDLE  hLocal,
    uid_t   uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirChangePassword(
    HANDLE hLocal,
    uid_t  uid,
    PCSTR  pszPassword
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirAddUser(
    HANDLE hLocal,
    DWORD  dwInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirModifyUser(
    HANDLE             hLocal,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
#if 0
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    // TODO: Implement this in a database transaction

    dwError = LocalFindUserById(
                        hDb,
                        pUserModInfo->uid,
                        dwUserInfoLevel,
                        &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserModInfo->actions.bEnableUser) {
        dwError = LocalEnableUser(
                       hDb,
                       pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bDisableUser) {
        dwError = LocalDisableUser(
                        hDb,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bUnlockUser) {
        dwError = LocalUnlockUser(
                        hDb,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetChangePasswordOnNextLogon) {
        dwError = LocalSetChangePasswordOnNextLogon(
                        hDb,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LocalSetPasswordExpires(
                        hDb,
                        pUserModInfo->uid,
                        TRUE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetAccountExpiryDate) {
        dwError = LocalSetAccountExpiryDate(
                        hDb,
                        pUserModInfo->uid,
                        pUserModInfo->pszExpiryDate);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bRemoveFromGroups) {
        dwError = LocalRemoveFromGroups(
                        hDb,
                        pUserModInfo->uid,
                        pUserModInfo->pszRemoveFromGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bAddToGroups) {
        dwError = LocalAddToGroups(
                        hDb,
                        pUserModInfo->uid,
                        pUserModInfo->pszAddToGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetPasswordMustExpire) {
        dwError = LocalSetPasswordExpires(
                        hDb,
                        pUserModInfo->uid,
                        TRUE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetPasswordNeverExpires) {
        dwError = LocalSetPasswordExpires(
                        hDb,
                        pUserModInfo->uid,
                        FALSE);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    if (pUserInfo) {
       LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
#else
    return dwError;
#endif
}

DWORD
LocalCreateHomeDirectory(
    PLSA_USER_INFO_0 pUserInfo
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;
    mode_t  umask = 022;
    mode_t  perms = (S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
    BOOLEAN bRemoveDir = FALSE;

    if (IsNullOrEmptyString(pUserInfo->pszHomedir))
    {
       LSA_LOG_ERROR("The user's [Uid:%ld] home directory is not defined",
                     (long)pUserInfo->uid);
       dwError = LSA_ERROR_FAILED_CREATE_HOMEDIR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaCheckDirectoryExists(
                    pUserInfo->pszHomedir,
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
       dwError = LsaCreateDirectory(
                    pUserInfo->pszHomedir,
                    perms & (~umask));
       BAIL_ON_LSA_ERROR(dwError);

       bRemoveDir = TRUE;

       dwError = LsaChangeOwner(
                    pUserInfo->pszHomedir,
                    pUserInfo->uid,
                    pUserInfo->gid);
       BAIL_ON_LSA_ERROR(dwError);

       bRemoveDir = FALSE;

       dwError = LocalProvisionHomeDir(
                       pUserInfo->uid,
                       pUserInfo->gid,
                       pUserInfo->pszHomedir);
       BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    if (bRemoveDir) {
       LsaRemoveDirectory(pUserInfo->pszHomedir);
    }

    goto cleanup;
}

DWORD
LocalProvisionHomeDir(
    uid_t ownerUid,
    gid_t ownerGid,
    PCSTR pszHomedirPath
    )
{
    DWORD   dwError = 0;
    BOOLEAN bExists = FALSE;

    dwError = LsaCheckDirectoryExists(
                    "/etc/skel",
                    &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (bExists)
    {
        dwError = LsaCopyDirectory(
                    "/etc/skel",
                    ownerUid,
                    ownerGid,
                    pszHomedirPath);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalCheckAccountFlags(
    PLSA_USER_INFO_2 pUserInfo2
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    BAIL_ON_INVALID_POINTER(pUserInfo2);

    if (pUserInfo2->bAccountDisabled)
    {
        dwError = LSA_ERROR_ACCOUNT_DISABLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountLocked)
    {
        dwError = LSA_ERROR_ACCOUNT_LOCKED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bAccountExpired)
    {
        dwError = LSA_ERROR_ACCOUNT_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserInfo2->bPasswordExpired)
    {
        dwError = LSA_ERROR_PASSWORD_EXPIRED;
        BAIL_ON_LSA_ERROR(dwError);
    }

error:

    return dwError;
}


