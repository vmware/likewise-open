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
 *        users.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Active Directory Mini Provider Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Arlene Berry (aberry@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "lsaopen.h"

#if 0
DWORD
(*CHECKUSERINLIST)(
    HANDLE hMiniProvider,
    PCSTR  pszLoginId,
    PCSTR  pszListName
    );
#endif

DWORD
LikewiseOpenLookupUserByName(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    PCSTR   pszLoginId,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LikewiseOpenLookupUserById(
    HANDLE  hMiniProvider,
    uid_t   uid,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

#if 0
DWORD
LikewiseOpenGetGroupsForUser(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    uid_t   uid,
    LSA_FIND_FLAGS
    FindFlags,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    dwConnectMode = GetConnectMode();

    switch(dwConnectMode) {

        case ONLINE:
                LikewiseOpenOnlineGetGroupsForUsers();
                break;



        case OFFLINE:

    return dwError;
}
#endif

DWORD
LikewiseOpenBeginEnumUsers(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    DWORD   dwInfoLevel,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    return (dwError);
}

DWORD
LikewiseOpenEnumUsers(
    HANDLE  hMiniProvider,
    DWORD dwConnectMode,
    HANDLE  hResume,
    DWORD   dwMaxUsers,
    PDWORD  pdwUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

VOID
LikewiseOpenEndEnumUsers(
    HANDLE hMiniProvider,
    DWORD dwConnectMode,
    HANDLE hResume
    )
{

    return;
}

DWORD
OnlineGetUsersByName()
{
    DWORD dwError = 0;

    return dwError;
}

