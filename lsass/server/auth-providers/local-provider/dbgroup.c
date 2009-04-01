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
 *        User/Group Database Interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "localprovider.h"

DWORD
LsaLPDbFindGroupByName(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

    switch(dwGroupInfoLevel)
    {
        case 0:
        {
            dwError = LsaLPDbFindGroupByName_0(
                                hDb,
                                pszDomain,
                                pszGroupName,
                                ppGroupInfo
                                );
            break;
        }
        case 1:
        {
            dwError = LsaLPDbFindGroupByName_1(
                                hDb,
                                pszDomain,
                                pszGroupName,
                                ppGroupInfo
                                );
            break;
        }
    }

    return dwError;
}



DWORD
LsaLPDbFindGroupByName_0(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaLPDbFindGroupByName_0_Unsafe(
                     hDb,
                     pszDomain,
                     pszGroupName,
                     ppGroupInfo
                     );

    LEAVE_RW_READER_LOCK;

    return dwError;
}


DWORD
LsaLPDbFindGroupByName_1(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaLPDbFindGroupByName_1_Unsafe(
                     hDb,
                     pszDomain,
                     pszGroupName,
                     ppGroupInfo
                     );

    LEAVE_RW_READER_LOCK;

    return dwError;
}

DWORD
LsaLPDbGetGroupsForUser_0(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaLPDbGetGroupsForUser_0_Unsafe(
                     hDb,
                     uid,
                     pdwGroupsFound,
                     pppGroupInfoList
                     );

    LEAVE_RW_READER_LOCK;

    return dwError;
}



DWORD
LsaLPDbGetGroupsForUser_1(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaLPDbGetGroupsForUser_1_Unsafe(
                     hDb,
                     uid,
                     pdwGroupsFound,
                     pppGroupInfoList
                     );

    LEAVE_RW_READER_LOCK;

    return dwError;
}



DWORD
LsaLPDbFindGroupByName_0_Unsafe(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD nExpectedCols = 2;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    DWORD dwNumGroupsFound = 0;
    DWORD dwGroupInfoLevel = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_GROUP_0_BY_NAME,
                               pszGroupName);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                               );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbWriteToGroupInfo_0_Unsafe(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppGroupInfoList,
                    &dwNumGroupsFound
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = *ppGroupInfoList;
    *ppGroupInfoList = NULL;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    *ppGroupInfo = NULL;

    goto cleanup;
}


DWORD
LsaLPDbFindGroupByName_1_Unsafe(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD nExpectedCols = 3;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    DWORD dwNumGroupsFound = 0;
    DWORD dwGroupInfoLevel = 1;
    DWORD iGroup = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_GROUP_1_BY_NAME,
                               pszGroupName);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                               );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbWriteToGroupInfo_1_Unsafe(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppGroupInfoList,
                    &dwNumGroupsFound
                    );
    BAIL_ON_LSA_ERROR(dwError);

    for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
    {
        PLSA_GROUP_INFO_1 pGroupInfo = *(ppGroupInfoList + iGroup);

        if (pszQuery) {
            sqlite3_free(pszQuery);
            pszQuery = NULL;
        }

        if (ppszResult) {
            sqlite3_free_table(ppszResult);
            ppszResult = NULL;
        }

        // Find the group members
        nRows = 0;
        nCols = 0;
        nExpectedCols = 1;
        pszQuery = sqlite3_mprintf(DB_QUERY_FIND_GROUP_MEMBERS_BY_GID,
                               pGroupInfo->gid);

        dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                               );
        BAIL_ON_LSA_ERROR(dwError);

        if (nRows) {
            if (nCols != nExpectedCols) {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaLPDbWriteMembersToGroupInfo_1(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        pGroupInfo
                        );
            BAIL_ON_LSA_ERROR(dwError);
        } else {
            pGroupInfo->ppszMembers = NULL;
        }
    }

    *ppGroupInfo = *ppGroupInfoList;
    *ppGroupInfoList = NULL;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    *ppGroupInfo = NULL;

    goto cleanup;
}



DWORD
LsaLPDbEnumGroups_0(
    HANDLE    hDb,
    DWORD     dwOffset,
    DWORD     dwLimit,
    PDWORD    pdwNumGroupsFound,
    PVOID**   pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 2;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    DWORD iGroup = 0;
    DWORD dwNumGroupsFound = 0;
    DWORD dwGroupInfoLevel = 0;

    ENTER_RW_READER_LOCK;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_FIND_GROUPS_0_LIMIT,
                    dwLimit,
                    dwOffset
                    );

    dwError = sqlite3_get_table(
                    pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
       dwError = LSA_ERROR_NO_MORE_GROUPS;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > dwLimit)) {
       dwError = LSA_ERROR_DATA_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbWriteToGroupInfo_0_Unsafe(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppGroupInfoList,
                    &dwNumGroupsFound);
    BAIL_ON_LSA_ERROR(dwError);

    // Find the group members for each group
    nExpectedCols = 1;
    for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
    {
        PLSA_GROUP_INFO_0 pGrp = NULL;

        if (pszQuery) {
           sqlite3_free(pszQuery);
           pszQuery = NULL;
        }

        if (ppszResult) {
            sqlite3_free_table(ppszResult);
            ppszResult = NULL;
        }

        pGrp = (PLSA_GROUP_INFO_0)*(ppGroupInfoList+iGroup);
    }

    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    LEAVE_RW_READER_LOCK;

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    if (ppGroupInfoList) {
       LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;

    goto cleanup;
}


DWORD
LsaLPDbEnumGroups_1(
    HANDLE    hDb,
    DWORD     dwOffset,
    DWORD     dwLimit,
    PDWORD    pdwNumGroupsFound,
    PVOID**   pppGroupInfoList
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 3;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    DWORD iGroup = 0;
    DWORD dwNumGroupsFound = 0;
    DWORD dwGroupInfoLevel = 1;

    ENTER_RW_READER_LOCK;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_FIND_GROUPS_1_LIMIT,
                    dwLimit,
                    dwOffset
                    );

    dwError = sqlite3_get_table(
                    pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError
                    );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
       dwError = LSA_ERROR_NO_MORE_GROUPS;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > dwLimit)) {
       dwError = LSA_ERROR_DATA_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbWriteToGroupInfo_1_Unsafe(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppGroupInfoList,
                    &dwNumGroupsFound);
    BAIL_ON_LSA_ERROR(dwError);

    // Find the group members for each group
    nExpectedCols = 1;
    for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
    {
        PLSA_GROUP_INFO_1 pGrp = NULL;

        if (pszQuery) {
           sqlite3_free(pszQuery);
           pszQuery = NULL;
        }

        if (ppszResult) {
            sqlite3_free_table(ppszResult);
            ppszResult = NULL;
        }

        pGrp = (PLSA_GROUP_INFO_1)*(ppGroupInfoList+iGroup);

        // Find the group members
        nRows = 0;
        nCols = 0;
        pszQuery = sqlite3_mprintf(DB_QUERY_FIND_GROUP_MEMBERS_BY_GID,
                                   pGrp->gid);

        dwError = sqlite3_get_table(pDbHandle,
                                    pszQuery,
                                    &ppszResult,
                                    &nRows,
                                    &nCols,
                                    &pszError
                                   );
        BAIL_ON_LSA_ERROR(dwError);

        if (nRows) {

            if (nCols != nExpectedCols) {
               dwError = LSA_ERROR_DATA_ERROR;
               BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaLPDbWriteMembersToGroupInfo_1(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        pGrp
                        );
            BAIL_ON_LSA_ERROR(dwError);
        } else {
            pGrp->ppszMembers = NULL;
        }
    }

    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    LEAVE_RW_READER_LOCK;

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    if (ppGroupInfoList) {
       LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }

    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;

    goto cleanup;
}

DWORD
LsaLPDbEnumGroups(
    HANDLE  hDb,
    DWORD   dwGroupInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxGroups,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

    switch(dwGroupInfoLevel)
    {
        case 0:
        {
            dwError = LsaLPDbEnumGroups_0(
                            hDb,
                            dwStartingRecordId,
                            nMaxGroups,
                            pdwGroupsFound,
                            pppGroupInfoList
                            );
            break;
        }
        case 1:
        {
            dwError = LsaLPDbEnumGroups_1(
                            hDb,
                            dwStartingRecordId,
                            nMaxGroups,
                            pdwGroupsFound,
                            pppGroupInfoList
                            );
            break;
        }
    }

    return dwError;
}


DWORD
LsaLPDbFindGroupById(
    HANDLE  hDb,
    gid_t   gid,
    DWORD   dwGroupInfoLevel,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

    switch(dwGroupInfoLevel)
    {
        case 0:
        {
            dwError = LsaLPDbFindGroupById_0(hDb, gid, ppGroupInfo);
            break;
        }
        case 1:
        {
            dwError = LsaLPDbFindGroupById_1(hDb, gid, ppGroupInfo);
            break;
        }
    }

    return dwError;
}


static
DWORD
LsaLPDbFindGroupById_0(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaLPDbFindGroupById_0_Unsafe(
                       hDb,
                       gid,
                       ppGroupInfo
                       );

    LEAVE_RW_READER_LOCK;

    return dwError;
}

static
DWORD
LsaLPDbFindGroupById_1(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaLPDbFindGroupById_1_Unsafe(
                       hDb,
                       gid,
                       ppGroupInfo
                       );

    LEAVE_RW_READER_LOCK;

    return dwError;
}


static
DWORD
LsaLPDbFindGroupById_0_Unsafe(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD nExpectedCols = 2;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    DWORD dwNumGroupsFound = 0;
    DWORD dwGroupInfoLevel = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_GROUP_0_BY_GID,  gid);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                               );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbWriteToGroupInfo_0_Unsafe(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppGroupInfoList,
                    &dwNumGroupsFound
                    );
    BAIL_ON_LSA_ERROR(dwError);

    *ppGroupInfo = *ppGroupInfoList;
    *ppGroupInfoList = NULL;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    *ppGroupInfo = NULL;

    goto cleanup;
}

static
DWORD
LsaLPDbFindGroupById_1_Unsafe(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    DWORD nExpectedCols = 3;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    DWORD dwNumGroupsFound = 0;
    DWORD dwGroupInfoLevel = 1;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD iGroup = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_GROUP_1_BY_GID,  gid);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                               );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaLPDbWriteToGroupInfo_1_Unsafe(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppGroupInfoList,
                    &dwNumGroupsFound
                    );
    BAIL_ON_LSA_ERROR(dwError);

    for (iGroup = 0; iGroup < dwNumGroupsFound; iGroup++)
    {
        PLSA_GROUP_INFO_1 pGroupInfo = *(ppGroupInfoList + iGroup);

        if (pszQuery) {
            sqlite3_free(pszQuery);
            pszQuery = NULL;
        }

        if (ppszResult) {
            sqlite3_free_table(ppszResult);
            ppszResult = NULL;
        }

        // Find the group members
        nRows = 0;
        nCols = 0;
        nExpectedCols = 1;
        pszQuery = sqlite3_mprintf(DB_QUERY_FIND_GROUP_MEMBERS_BY_GID,
                                   pGroupInfo->gid);

        dwError = sqlite3_get_table(pDbHandle,
                                    pszQuery,
                                    &ppszResult,
                                    &nRows,
                                    &nCols,
                                    &pszError
                                   );
        BAIL_ON_LSA_ERROR(dwError);

        if (nRows) {
            if (nCols != nExpectedCols) {
                dwError = LSA_ERROR_DATA_ERROR;
                BAIL_ON_LSA_ERROR(dwError);
            }

            dwError = LsaLPDbWriteMembersToGroupInfo_1(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        pGroupInfo
                        );
            BAIL_ON_LSA_ERROR(dwError);
        } else {
            pGroupInfo->ppszMembers = NULL;
        }
    }

    *ppGroupInfo = *ppGroupInfoList;
    *ppGroupInfoList = NULL;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    *ppGroupInfo = NULL;

    goto cleanup;
}


DWORD
LsaLPDbAddGroup(
    HANDLE hDb,
    PCSTR  pszDomain,
    DWORD  dwGroupInfoLevel,
    PVOID  pGroupInfo
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    BOOLEAN bReleaseLock = FALSE;
    PVOID pExistingGroup = NULL;
    DWORD dwExistingGroupInfoLevel = 0;
    PSTR pszError = NULL;
    PSTR pszQuery = NULL;
    PLSA_GROUP_INFO_1 pGroup = NULL;

    if (dwGroupInfoLevel != 1) {
        dwError = LSA_ERROR_INVALID_GROUP_INFO_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pGroup = (PLSA_GROUP_INFO_1)pGroupInfo;

    if (IsNullOrEmptyString(pGroup->pszName)) {
        dwError = LSA_ERROR_INVALID_GROUP_NAME;
        BAIL_ON_LSA_ERROR(dwError);
    }

    ENTER_RW_WRITER_LOCK;
    bReleaseLock = TRUE;

    // This will not let root be added - which is ok.
    if (!pGroup->gid) {
       // We got a write lock; nobody can steal this gid from us
       dwError = LsaLPDbGetNextAvailableGid_Unsafe(
                           hDb,
                           &pGroup->gid
                           );
       BAIL_ON_LSA_ERROR(dwError);
    } else {
       dwError = LsaLPDbFindGroupById_0_Unsafe(
                       hDb,
                       pGroup->gid,
                       &pExistingGroup
                       );
       if (dwError) {
           if (dwError == LSA_ERROR_NO_SUCH_GROUP) {
               dwError = 0;
           } else {
               BAIL_ON_LSA_ERROR(dwError);
           }
       } else {
          dwError = LSA_ERROR_GROUP_EXISTS;
          BAIL_ON_LSA_ERROR(dwError);
       }
    }

    dwError = LsaLPDbFindGroupByName_0_Unsafe(
                    hDb,
                    pszDomain,
                    pGroup->pszName,
                    &pExistingGroup
                    );
    if (dwError) {
        if (dwError == LSA_ERROR_NO_SUCH_GROUP) {
            dwError = 0;
        } else {
            BAIL_ON_LSA_ERROR(dwError);
        }
    } else {
       dwError = LSA_ERROR_GROUP_EXISTS;
       BAIL_ON_LSA_ERROR(dwError);
    }

    pszQuery = sqlite3_mprintf(DB_QUERY_INSERT_GROUP,
                               pGroup->pszName,
                               pGroup->pszPasswd,
                               pGroup->gid);

    dwError = sqlite3_exec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    // TODO: Handle Group Members

cleanup:

    if (pszQuery) {
        sqlite3_free(pszQuery);
    }

    if (bReleaseLock) {
        LEAVE_RW_WRITER_LOCK;
    }

    if (pExistingGroup) {
        LsaFreeGroupInfo(dwExistingGroupInfoLevel, pExistingGroup);
    }

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError)) {
       LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}


DWORD
LsaLPDetermineNumberUsersWithPrimaryGroup_Unsafe(
    HANDLE hDb,
    gid_t  gid,
    PDWORD pdwNMembers
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR* ppszResult = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    DWORD dwNMembers = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_NUMBER_USERS_WITH_THIS_PRIMARY_GROUP, gid);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                                );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
       dwNMembers = 0;
    } else if ((nRows > 1) || (nCols != 1)) {
       dwError = LSA_ERROR_DATA_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    } else if (nRows == 1){
       dwNMembers = (IsNullOrEmptyString(ppszResult[1]) ? 0 : atoi(ppszResult[1]));
    }

    *pdwNMembers = dwNMembers;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    return dwError;

error:

    *pdwNMembers = 0;

    goto cleanup;
}


DWORD
LsaLPDetermineNumberGroupMembers(
    HANDLE hDb,
    gid_t  gid,
    PDWORD pdwNMembers
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR* ppszResult = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    DWORD dwNMembers = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_NUMBER_GROUP_MEMBERS, gid);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                                );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
       dwNMembers = 0;
    } else if ((nRows > 1) || (nCols != 1)) {
       dwError = LSA_ERROR_DATA_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    } else if (nRows == 1){
       dwNMembers = (IsNullOrEmptyString(ppszResult[1]) ? 0 : atoi(ppszResult[1]));
    }

    *pdwNMembers = dwNMembers;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    return dwError;

error:

    *pdwNMembers = 0;

    goto cleanup;
}


DWORD
LsaLPDetermineGroupCanBeDeleted_Unsafe(
    HANDLE hDb,
    gid_t  gid,
    PBOOLEAN pbCanBeDeleted
    )
{
    DWORD dwError = 0;
    BOOLEAN bResult = FALSE;
    DWORD dwNUsers_w_primaryGroup = 0;
    DWORD dwNGroupMembers = 0;

    dwError = LsaLPDetermineNumberUsersWithPrimaryGroup_Unsafe(
                    hDb,
                    gid,
                    &dwNUsers_w_primaryGroup);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNUsers_w_primaryGroup) {
        goto cleanup;
    }

    dwError = LsaLPDetermineNumberGroupMembers(
                    hDb,
                    gid,
                    &dwNGroupMembers);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwNGroupMembers) {
       goto cleanup;
    }

    bResult = TRUE;

cleanup:

    *pbCanBeDeleted = bResult;

    return dwError;

error:

    goto cleanup;
}

DWORD
LsaLPDbDeleteGroup(
    HANDLE hDb,
    gid_t  gid
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    BOOLEAN  bCanBeDeleted = FALSE;
    PSTR     pszError = NULL;
    PSTR     pszQuery = NULL;

    ENTER_RW_WRITER_LOCK;

    dwError = LsaLPDetermineGroupCanBeDeleted_Unsafe(
                        hDb,
                        gid,
                        &bCanBeDeleted);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bCanBeDeleted) {
        dwError = LSA_ERROR_GROUP_IN_USE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pszQuery = sqlite3_mprintf(DB_QUERY_DELETE_GROUP, gid);

    dwError = sqlite3_exec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (pszQuery) {
        sqlite3_free(pszQuery);
    }

    LEAVE_RW_WRITER_LOCK;

    return dwError;

error:

    goto cleanup;
}


