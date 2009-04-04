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
 *        lpgroup.c
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
#include "includes.h"

DWORD
LocalDirFindGroupByName(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    switch(dwGroupInfoLevel)
    {
        case 0:

            dwError = LocalDirFindGroupByName_0(
                                hDb,
                                pszDomain,
                                pszGroupName,
                                ppGroupInfo
                                );
            break;

        case 1:

            dwError = LocalDirFindGroupByName_1(
                                hDb,
                                pszDomain,
                                pszGroupName,
                                ppGroupInfo
                                );
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirFindGroupByName_0(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
LocalDirFindGroupByName_1(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_0(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirGetGroupsForUser_1(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirEnumGroups_0(
    HANDLE    hDb,
    DWORD     dwOffset,
    DWORD     dwLimit,
    PDWORD    pdwNumGroupsFound,
    PVOID**   pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirEnumGroups_1(
    HANDLE    hDb,
    DWORD     dwOffset,
    DWORD     dwLimit,
    PDWORD    pdwNumGroupsFound,
    PVOID**   pppGroupInfoList
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirEnumGroups(
    HANDLE  hDb,
    DWORD   dwGroupInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    switch(dwGroupInfoLevel)
    {
        case 0:

            dwError = LocalDirEnumGroups_0(
                            hDb,
                            dwStartingRecordId,
                            nMaxGroups,
                            pdwGroupsFound,
                            pppGroupInfoList
                            );
            break;

        case 1:

            dwError = LocalDirEnumGroups_1(
                            hDb,
                            dwStartingRecordId,
                            nMaxGroups,
                            pdwGroupsFound,
                            pppGroupInfoList
                            );
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirFindGroupById(
    HANDLE  hDb,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    switch(dwGroupInfoLevel)
    {
        case 0:

            dwError = LocalDirFindGroupById_0(hDb, gid, ppGroupInfo);
            break;

        case 1:

            dwError = LocalDirFindGroupById_1(hDb, gid, ppGroupInfo);
            break;

        default:

            dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

            break;
    }

    return dwError;
}

DWORD
LocalDirFindGroupById_0(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirFindGroupById_1(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
LocalDirAddGroup(
    HANDLE hDb,
    PCSTR  pszDomain,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    )
{
    DWORD dwError = 0;
    PLSA_GROUP_INFO_1 pGroup = NULL;

    if (dwGroupInfoLevel != 1)
    {
        dwError = LSA_ERROR_INVALID_GROUP_INFO_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGroup = (PLSA_GROUP_INFO_1)pGroupInfo;

    if (IsNullOrEmptyString(pGroup->pszName)) {
        dwError = LSA_ERROR_INVALID_GROUP_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
LocalDirDeleteGroup(
    HANDLE hDb,
    gid_t  gid
    )
{
    DWORD dwError = 0;

    return dwError;
}


