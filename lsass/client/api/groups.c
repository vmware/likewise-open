/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        groups.c
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Group Lookup and Management API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
            Wei Fu (wfu@likewisesoftware.com)
 */
#include "client.h"

LSASS_API
DWORD
LsaAddGroup(
    HANDLE hLsaConnection,
    PVOID  pGroupInfo,
    DWORD  dwGroupInfoLevel
    )
{
    return LsaTransactAddGroup(
            hLsaConnection,
            pGroupInfo,
            dwGroupInfoLevel);
}

LSASS_API
DWORD
LsaModifyGroup(
    HANDLE hLsaConnection,
    PLSA_GROUP_MOD_INFO pGroupModInfo
    )
{
    return LsaTransactModifyGroup(
            hLsaConnection,
            pGroupModInfo);
}


LSASS_API
DWORD
LsaFindGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszGroupName,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszGroupName);

    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(ppGroupInfo);

    dwError = LsaTransactFindGroupByName(
                hLsaConnection,
                pszGroupName,
                FindFlags,
                dwGroupInfoLevel,
                &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (ppGroupInfo)
    {
        *ppGroupInfo = pGroupInfo;
    }

    return dwError;
}

LSASS_API
DWORD
LsaFindGroupById(
    HANDLE hLsaConnection,
    gid_t  gid,
    LSA_FIND_FLAGS FindFlags,
    DWORD  dwGroupInfoLevel,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);

    dwError = LsaValidateGroupInfoLevel(dwGroupInfoLevel);
    BAIL_ON_LSA_ERROR(dwError);

    BAIL_ON_INVALID_POINTER(ppGroupInfo);

    dwError = LsaTransactFindGroupById(
                hLsaConnection,
                gid,
                FindFlags,
                dwGroupInfoLevel,
                &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

error:

    if (ppGroupInfo)
    {
        *ppGroupInfo = pGroupInfo;
    }

    return dwError;
}

LSASS_API
DWORD
LsaBeginEnumGroups(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactBeginEnumGroups(
                hLsaConnection,
                dwGroupInfoLevel,
                dwMaxNumGroups,
                //By default, do not checkOnline for group membership when enumerating
                FALSE,
                FindFlags,
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
LsaBeginEnumGroupsWithCheckOnlineOption(
    HANDLE  hLsaConnection,
    DWORD   dwGroupInfoLevel,
    DWORD   dwMaxNumGroups,
    BOOLEAN bCheckGroupMembersOnline,
    LSA_FIND_FLAGS FindFlags,
    PHANDLE phResume
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactBeginEnumGroups(
                hLsaConnection,
                dwGroupInfoLevel,
                dwMaxNumGroups,
                bCheckGroupMembersOnline,
                FindFlags,
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
LsaEnumGroups(
    HANDLE  hLsaConnection,
    HANDLE  hResume,
    PDWORD  pdwNumGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactEnumGroups(
                hLsaConnection,
                hResume,
                pdwNumGroupsFound,
                pppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *pdwNumGroupsFound = 0;
    *pppGroupInfoList = NULL;

    goto cleanup;
}

LSASS_API
DWORD
LsaEndEnumGroups(
    HANDLE hLsaConnection,
    HANDLE hResume
    )
{
    return LsaTransactEndEnumGroups(
                hLsaConnection,
                hResume);
}

LSASS_API
DWORD
LsaDeleteGroupById(
    HANDLE hLsaConnection,
    gid_t  gid
    )
{
    return LsaTransactDeleteGroupById(
            hLsaConnection,
            gid);
}

LSASS_API
DWORD
LsaDeleteGroupByName(
    HANDLE hLsaConnection,
    PCSTR  pszName
    )
{
    DWORD dwError = 0;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;

    if (geteuid() != 0)
    {
        dwError = EACCES;
        BAIL_ON_LSA_ERROR(dwError);
    }

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszName);

    dwError = LsaValidateGroupName(pszName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindGroupByName(
                    hLsaConnection,
                    pszName,
                    0,
                    dwGroupInfoLevel,
                    &pGroupInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDeleteGroupById(
                    hLsaConnection,
                    ((PLSA_GROUP_INFO_0)pGroupInfo)->gid);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;

error:

    goto cleanup;
}

LSASS_API
DWORD
LsaGetGidsForUserByName(
    HANDLE  hLsaConnection,
    PCSTR   pszUserName,
    PDWORD  pdwGroupFound,
    gid_t** ppGidResults
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    DWORD dwGroupInfoLevel = 0;
    DWORD dwGroupFound = 0;
    gid_t* pGidResult = NULL;
    PVOID*  ppGroupInfoList = NULL;
    DWORD iGroup = 0;

    BAIL_ON_INVALID_HANDLE(hLsaConnection);
    BAIL_ON_INVALID_STRING(pszUserName);
    BAIL_ON_INVALID_POINTER(ppGidResults);

    dwError = LsaValidateUserName(pszUserName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaFindUserByName(
                  hLsaConnection,
                  pszUserName,
                  dwUserInfoLevel,
                  &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaGetGroupsForUserById(
                hLsaConnection,
                ((PLSA_USER_INFO_0)pUserInfo)->uid,
                LSA_FIND_FLAGS_NSS,
                dwGroupInfoLevel,
                &dwGroupFound,
                &ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwAllocateMemory(
                    sizeof(gid_t) * dwGroupFound,
                    (PVOID*)&pGidResult);
    BAIL_ON_LSA_ERROR(dwError);

    switch(dwGroupInfoLevel)
    {
        case 0:

            for (iGroup = 0; iGroup < dwGroupFound; iGroup++) {
                   *(pGidResult+iGroup) = ((PLSA_GROUP_INFO_0)(*(ppGroupInfoList+iGroup)))->gid;
            }

            break;

        default:
            dwError = LW_ERROR_UNSUPPORTED_GROUP_LEVEL;
            BAIL_ON_LSA_ERROR(dwError);
            break;
    }

    *ppGidResults = pGidResult;
    *pdwGroupFound = dwGroupFound;

cleanup:

    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwGroupFound);
    }

    return dwError;

error:

    *ppGidResults = NULL;
    *pdwGroupFound = 0;

    goto cleanup;
}

LSASS_API
DWORD
LsaGetGroupsForUserByName(
    IN HANDLE hLsaConnection,
    IN PCSTR pszUserName,
    IN LSA_FIND_FLAGS FindFlags,
    IN DWORD dwGroupInfoLevel,
    OUT PDWORD pdwGroupsFound,
    OUT PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactGetGroupsForUser(
                    hLsaConnection,
                    pszUserName,
                    0,
                    FindFlags,
                    dwGroupInfoLevel,
                    pdwGroupsFound,
                    pppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

   goto cleanup;
}

LSASS_API
DWORD
LsaGetGroupsForUserById(
    HANDLE  hLsaConnection,
    uid_t   uid,
    LSA_FIND_FLAGS FindFlags,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    dwError = LsaTransactGetGroupsForUser(
               hLsaConnection,
               NULL,
               uid,
               FindFlags,
               dwGroupInfoLevel,
               pdwGroupsFound,
               pppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    *pdwGroupsFound = 0;
    *pppGroupInfoList = NULL;

   goto cleanup;
}

VOID
LsaFreeEnumObjectsInfo(
    PLSA_ENUM_OBJECTS_INFO pInfo
    )
{
    LW_SAFE_FREE_STRING(pInfo->pszGUID);
    LwFreeMemory(pInfo);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
