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
 *        users.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        User Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaAddUser(
    HANDLE hLsaConnection,
    PVOID  pUserInfo,
    DWORD  dwUserInfoLevel
    )
{
    return LsaTransactAddUser(
            hLsaConnection,
            pUserInfo,
            dwUserInfoLevel);
}

LSASS_API
DWORD
LsaModifyUser(
    HANDLE hLsaConnection,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    return LsaTransactModifyUser(
            hLsaConnection,
            pUserModInfo);
}

LSASS_API
DWORD
LsaFindUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactFindUserByName(
                hLsaConnection,
                pszName,
                dwUserInfoLevel,
                ppUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *ppUserInfo = NULL;

    goto cleanup;
}

LSASS_API
DWORD
LsaFindUserById(
    HANDLE hLsaConnection,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactFindUserById(
                hLsaConnection,
                uid,
                dwUserInfoLevel,
                ppUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *ppUserInfo = NULL;

    goto cleanup;
}

LSASS_API
DWORD
LsaBeginEnumUsers(
    HANDLE  hLsaConnection,
    DWORD   dwUserInfoLevel,
    DWORD   dwMaxNumUsers,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactBeginEnumUsers(
                hLsaConnection,
                dwUserInfoLevel,
                dwMaxNumUsers,
                phResume);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *phResume = (HANDLE)NULL;

    goto cleanup;
}

LSASS_API
DWORD
LsaEnumUsers(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactEnumUsers(
                hLsaConnection,
                hResume,
                pdwNumUsersFound,
                pppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *pdwNumUsersFound = 0;
    *pppUserInfoList = NULL;

    goto cleanup;
}

LSASS_API
DWORD
LsaEndEnumUsers(
    HANDLE hLsaConnection,
    HANDLE hResume
    )
{
    return LsaTransactEndEnumUsers(
                hLsaConnection,
                hResume);
}

LSASS_API
DWORD
LsaDeleteUserById(
    HANDLE hLsaConnection,
    uid_t  uid
    )
{
    return LsaTransactDeleteUserById(
            hLsaConnection,
            uid);
}

LSASS_API
DWORD
LsaDeleteUserByName(
    HANDLE hLsaConnection,
    PCSTR  pszName
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    if (geteuid() != 0)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszName);

    dwError = LsaFindUserByName(
                    hLsaConnection,
                    pszName,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDeleteUserById(
                    hLsaConnection,
                    ((PLSA_USER_INFO_0)pUserInfo)->uid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    return dwError;

error:

    goto cleanup;
}

VOID
LsaFreeEnumUsersInfo(
    PLSA_ENUM_USERS_INFO pInfo
    )
{
    LSA_SAFE_FREE_STRING(pInfo->pszGUID);
    LsaFreeMemory(pInfo);
}

LSASS_API
DWORD
LsaGetNamesBySidList(
    IN HANDLE hLsaConnection,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SID_INFO* ppSIDInfoList,
    OUT OPTIONAL CHAR *pchDomainSeparator
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactGetNamesBySidList(
                hLsaConnection,
                sCount,
                ppszSidList,
                ppSIDInfoList,
                pchDomainSeparator);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *ppSIDInfoList = NULL;

    goto cleanup;
}
