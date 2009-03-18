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
LsaProviderLocal_DbFindGroupByName(
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
            dwError = LsaProviderLocal_DbFindGroupByName_0(
                                hDb,
                                pszDomain,
                                pszGroupName,
                                ppGroupInfo
                                );
            break;
        }
        case 1:
        {
            dwError = LsaProviderLocal_DbFindGroupByName_1(
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
LsaProviderLocal_DbFindGroupByName_0(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaProviderLocal_DbFindGroupByName_0_Unsafe(
                     hDb,
                     pszDomain,
                     pszGroupName,
                     ppGroupInfo
                     );

    LEAVE_RW_READER_LOCK;

    return dwError;
}


DWORD
LsaProviderLocal_DbFindGroupByName_1(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszGroupName,
    PVOID*  ppGroupInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaProviderLocal_DbFindGroupByName_1_Unsafe(
                     hDb,
                     pszDomain,
                     pszGroupName,
                     ppGroupInfo
                     );

    LEAVE_RW_READER_LOCK;

    return dwError;
}

DWORD
LsaProviderLocal_DbGetGroupsForUser_0(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaProviderLocal_DbGetGroupsForUser_0_Unsafe(
                     hDb,
                     uid,
                     pdwGroupsFound,
                     pppGroupInfoList
                     );

    LEAVE_RW_READER_LOCK;

    return dwError;
}



DWORD
LsaProviderLocal_DbGetGroupsForUser_1(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaProviderLocal_DbGetGroupsForUser_1_Unsafe(
                     hDb,
                     uid,
                     pdwGroupsFound,
                     pppGroupInfoList
                     );

    LEAVE_RW_READER_LOCK;

    return dwError;
}



DWORD
LsaProviderLocal_DbFindGroupByName_0_Unsafe(
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

    dwError = LsaProviderLocal_DbWriteToGroupInfo_0_Unsafe(
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
LsaProviderLocal_DbFindGroupByName_1_Unsafe(
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

    dwError = LsaProviderLocal_DbWriteToGroupInfo_1_Unsafe(
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

            dwError = LsaProviderLocal_DbWriteMembersToGroupInfo_1(
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
LsaProviderLocal_DbEnumGroups_0(
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

    dwError = LsaProviderLocal_DbWriteToGroupInfo_0_Unsafe(
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
LsaProviderLocal_DbEnumGroups_1(
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

    dwError = LsaProviderLocal_DbWriteToGroupInfo_1_Unsafe(
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

            dwError = LsaProviderLocal_DbWriteMembersToGroupInfo_1(
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
LsaProviderLocal_DbEnumGroups(
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
            dwError = LsaProviderLocal_DbEnumGroups_0(
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
            dwError = LsaProviderLocal_DbEnumGroups_1(
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
LsaProviderLocal_DbFindGroupById(
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
            dwError = LsaProviderLocal_DbFindGroupById_0(hDb, gid, ppGroupInfo);
            break;
        }
        case 1:
        {
            dwError = LsaProviderLocal_DbFindGroupById_1(hDb, gid, ppGroupInfo);
            break;
        }
    }

    return dwError;
}


static
DWORD
LsaProviderLocal_DbFindGroupById_0(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaProviderLocal_DbFindGroupById_0_Unsafe(
                       hDb,
                       gid,
                       ppGroupInfo
                       );

    LEAVE_RW_READER_LOCK;

    return dwError;
}

static
DWORD
LsaProviderLocal_DbFindGroupById_1(
    HANDLE hDb,
    gid_t  gid,
    PVOID* ppGroupInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaProviderLocal_DbFindGroupById_1_Unsafe(
                       hDb,
                       gid,
                       ppGroupInfo
                       );

    LEAVE_RW_READER_LOCK;

    return dwError;
}


static
DWORD
LsaProviderLocal_DbFindGroupById_0_Unsafe(
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

    dwError = LsaProviderLocal_DbWriteToGroupInfo_0_Unsafe(
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
LsaProviderLocal_DbFindGroupById_1_Unsafe(
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

    dwError = LsaProviderLocal_DbWriteToGroupInfo_1_Unsafe(
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

            dwError = LsaProviderLocal_DbWriteMembersToGroupInfo_1(
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

