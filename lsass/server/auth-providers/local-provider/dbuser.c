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

#define LW_ACCOUNT_DISABLED       1
#define LW_CANNOT_CHANGE_PASSWORD (1 << 1)
#define LW_PASSWORD_CANNOT_EXPIRE (1 << 2)
#define LW_ACCOUNT_LOCKED_OUT     (1 << 3)

#define LOCAL_USER_SID_FORMAT "S-1-22-1-%ld"
#define LOCAL_GROUP_SID_FORMAT "S-1-22-2-%ld"

DWORD
LsaProviderLocal_DbFindUserByName(
    HANDLE  hDb,
    PCSTR   pszDomain,
    PCSTR   pszUserName,
    DWORD   dwUserInfoLevel,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

    switch(dwUserInfoLevel)
    {
        case 0:
        {
            dwError = LsaProviderLocal_DbFindUserByName_0(
                            hDb,
                            pszUserName,
                            ppUserInfo
                            );
            break;
        }
        case 1:
        {
            dwError = LsaProviderLocal_DbFindUserByName_1(
                            hDb,
                            pszUserName,
                            ppUserInfo
                            );
            break;
        }
        case 2:
        {
            dwError = LsaProviderLocal_DbFindUserByName_2(
                            hDb,
                            pszUserName,
                            ppUserInfo
                            );
            break;
        }
    }

    return dwError;
}


DWORD
LsaProviderLocal_DbEnumUsers_0(
    HANDLE hDb,
    DWORD  dwOffset,
    DWORD  dwLimit,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 7;
    PLSA_USER_INFO_0* ppUserInfoList = NULL;
    DWORD dwNumUsersFound = 0;
    DWORD dwUserInfoLevel = 0;

    ENTER_RW_READER_LOCK;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_FIND_USERS_0_LIMIT,
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
        dwError = LSA_ERROR_NO_MORE_USERS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > dwLimit)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbWriteToUserInfo_0_Unsafe(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        &ppUserInfoList,
                        &dwNumUsersFound);
    BAIL_ON_LSA_ERROR(dwError);

    *pppUserInfoList = (PVOID*)ppUserInfoList;
    *pdwNumUsersFound = dwNumUsersFound;

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

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }

    *pppUserInfoList = NULL;
    *pdwNumUsersFound = 0;

    goto cleanup;
}



DWORD
LsaProviderLocal_DbFindUserByName_0(
    HANDLE hDb,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaProviderLocal_DbFindUserByName_0_Unsafe(
                    hDb,
                    pszUserName,
                    ppUserInfo
                    );

    LEAVE_RW_READER_LOCK;

    return dwError;
}


DWORD
LsaProviderLocal_DbFindUserByName_1(
    HANDLE hDb,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 7;
    PLSA_USER_INFO_1* ppUserInfoList = NULL;
    DWORD dwNumUsersFound = 0;
    DWORD dwUserInfoLevel = 1;
    PBYTE pNTHash = NULL;
    DWORD dwNTHashLen = 0;
    PBYTE pLMHash = NULL;
    DWORD dwLMHashLen = 0;
    DWORD iUser = 0;

    ENTER_RW_READER_LOCK;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_USER_1_BY_NAME,
                               pszUserName);

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
       dwError = LSA_ERROR_NO_SUCH_USER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbWriteToUserInfo_1_Unsafe(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        &ppUserInfoList,
                        &dwNumUsersFound
                        );
    BAIL_ON_LSA_ERROR(dwError);

    for (iUser = 0; iUser < dwNumUsersFound; iUser++)
    {
        PLSA_USER_INFO_1 pUserInfo = *(ppUserInfoList + iUser);

        dwError = LsaProviderLocal_DbGetLMHash_Unsafe(
                        hDb,
                        pUserInfo->uid,
                        &pLMHash,
                        &dwLMHashLen
                        );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaProviderLocal_DbGetNTHash_Unsafe(
                        hDb,
                        pUserInfo->uid,
                        &pNTHash,
                        &dwNTHashLen
                        );
        BAIL_ON_LSA_ERROR(dwError);

        pUserInfo->pLMHash = pLMHash;
        pLMHash = NULL;
        pUserInfo->dwLMHashLen = dwLMHashLen;
        pUserInfo->pNTHash = pNTHash;
        pNTHash = NULL;
        pUserInfo->dwNTHashLen = dwNTHashLen;
    }

    *ppUserInfo = *ppUserInfoList;
    *ppUserInfoList = NULL;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }

    LEAVE_RW_READER_LOCK;

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    LSA_SAFE_FREE_MEMORY(pLMHash);
    LSA_SAFE_FREE_MEMORY(pNTHash);

    *ppUserInfo = NULL;

    goto cleanup;
}


DWORD
LsaProviderLocal_DbFindUserByName_2(
    HANDLE hDb,
    PCSTR  pszUserName,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 10;
    PLSA_USER_INFO_2* ppUserInfoList = NULL;
    DWORD dwNumUsersFound = 0;
    DWORD dwUserInfoLevel = 2;
    PBYTE pNTHash = NULL;
    DWORD dwNTHashLen = 0;
    PBYTE pLMHash = NULL;
    DWORD dwLMHashLen = 0;
    DWORD iUser = 0;

    ENTER_RW_READER_LOCK;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_USER_2_BY_NAME,
                               pszUserName);

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
       dwError = LSA_ERROR_NO_SUCH_USER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbWriteToUserInfo_2_Unsafe(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        &ppUserInfoList,
                        &dwNumUsersFound
                        );
    BAIL_ON_LSA_ERROR(dwError);

    for (iUser = 0; iUser < dwNumUsersFound; iUser++)
    {
        PLSA_USER_INFO_2 pUserInfo = *(ppUserInfoList + iUser);

        dwError = LsaProviderLocal_DbGetLMHash_Unsafe(
                        hDb,
                        pUserInfo->uid,
                        &pLMHash,
                        &dwLMHashLen
                        );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaProviderLocal_DbGetNTHash_Unsafe(
                        hDb,
                        pUserInfo->uid,
                        &pNTHash,
                        &dwNTHashLen
                        );
        BAIL_ON_LSA_ERROR(dwError);

        pUserInfo->pLMHash = pLMHash;
        pLMHash = NULL;
        pUserInfo->dwLMHashLen = dwLMHashLen;
        pUserInfo->pNTHash = pNTHash;
        pNTHash = NULL;
        pUserInfo->dwNTHashLen = dwNTHashLen;
    }

    *ppUserInfo = *ppUserInfoList;
    *ppUserInfoList = NULL;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }

    LEAVE_RW_READER_LOCK;

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    LSA_SAFE_FREE_MEMORY(pNTHash);
    LSA_SAFE_FREE_MEMORY(pLMHash);

    *ppUserInfo = NULL;

    goto cleanup;
}


DWORD
LsaProviderLocal_DbEnumUsers_1(
    HANDLE hDb,
    DWORD  dwOffset,
    DWORD  dwLimit,
    PDWORD pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 7;
    PLSA_USER_INFO_1* ppUserInfoList = NULL;
    DWORD iUser = 0;
    DWORD dwNumUsersFound = 0;
    DWORD dwUserInfoLevel = 1;
    PBYTE pLMHash = NULL;
    DWORD dwLMHashLen = 0;
    PBYTE pNTHash = NULL;
    DWORD dwNTHashLen = 0;

    ENTER_RW_READER_LOCK;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_FIND_USERS_1_LIMIT,
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
        dwError = LSA_ERROR_NO_MORE_USERS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > dwLimit)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbWriteToUserInfo_1_Unsafe(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        &ppUserInfoList,
                        &dwNumUsersFound
                        );
    BAIL_ON_LSA_ERROR(dwError);

    for (iUser = 0; iUser < dwNumUsersFound; iUser++)
    {
        PLSA_USER_INFO_1 pUserInfo = *(ppUserInfoList+iUser);

        dwError = LsaProviderLocal_DbGetLMHash_Unsafe(
                            hDb,
                            pUserInfo->uid,
                            &pLMHash,
                            &dwLMHashLen
                            );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaProviderLocal_DbGetNTHash_Unsafe(
                            hDb,
                            pUserInfo->uid,
                            &pNTHash,
                            &dwNTHashLen
                            );
        BAIL_ON_LSA_ERROR(dwError);

        pUserInfo->pLMHash = pLMHash;
        pLMHash = NULL;
        pUserInfo->dwLMHashLen = dwLMHashLen;
        pUserInfo->pNTHash = pNTHash;
        pNTHash = NULL;
        pUserInfo->dwNTHashLen = dwNTHashLen;
    }

    *pppUserInfoList = (PVOID*)ppUserInfoList;
    *pdwNumUsersFound = dwNumUsersFound;

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

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }

    LSA_SAFE_FREE_MEMORY(pLMHash);
    LSA_SAFE_FREE_MEMORY(pNTHash);

    *pppUserInfoList = NULL;
    *pdwNumUsersFound = 0;

    goto cleanup;
}


DWORD
LsaProviderLocal_DbEnumUsers_2(
    HANDLE  hDb,
    DWORD   dwOffset,
    DWORD   dwLimit,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 10;
    PLSA_USER_INFO_2* ppUserInfoList = NULL;
    DWORD iUser = 0;
    DWORD dwNumUsersFound = 0;
    DWORD dwUserInfoLevel = 2;
    PBYTE pNTHash = NULL;
    PBYTE pLMHash = NULL;
    DWORD dwNTHashLen = 0;
    DWORD dwLMHashLen = 0;

    ENTER_RW_READER_LOCK;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_FIND_USERS_2_LIMIT,
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
        dwError = LSA_ERROR_NO_MORE_USERS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > dwLimit)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbWriteToUserInfo_2_Unsafe(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppUserInfoList,
                    &dwNumUsersFound
                    );
    BAIL_ON_LSA_ERROR(dwError);

    for (iUser = 0; iUser < dwNumUsersFound; iUser++)
    {
        PLSA_USER_INFO_2 pUserInfo = *(ppUserInfoList + iUser);

        dwError = LsaProviderLocal_DbGetLMHash_Unsafe(
                            hDb,
                            pUserInfo->uid,
                            &pLMHash,
                            &dwLMHashLen
                            );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaProviderLocal_DbGetNTHash_Unsafe(
                            hDb,
                            pUserInfo->uid,
                            &pNTHash,
                            &dwNTHashLen
                            );
        BAIL_ON_LSA_ERROR(dwError);

        pUserInfo->pLMHash = pLMHash;
        pLMHash = NULL;
        pUserInfo->dwLMHashLen = dwLMHashLen;
        pUserInfo->pNTHash = pNTHash;
        pNTHash = NULL;
        pUserInfo->dwNTHashLen = dwNTHashLen;
    }

    *pppUserInfoList = (PVOID*)ppUserInfoList;
    *pdwNumUsersFound = dwNumUsersFound;

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

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }

    LSA_SAFE_FREE_MEMORY(pNTHash);
    LSA_SAFE_FREE_MEMORY(pLMHash);

    *pppUserInfoList = NULL;
    *pdwNumUsersFound = 0;

    goto cleanup;
}

DWORD
LsaProviderLocal_DbEnumUsers(
    HANDLE  hDb,
    DWORD   dwUserInfoLevel,
    DWORD   dwStartingRecordId,
    DWORD   nMaxUsers,
    PDWORD  pdwNumUsersFound,
    PVOID** pppUserInfoList
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

    switch (dwUserInfoLevel)
    {
        case 0:
        {
            dwError = LsaProviderLocal_DbEnumUsers_0(
                                hDb,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
        case 1:
        {
            dwError = LsaProviderLocal_DbEnumUsers_1(
                                hDb,
                                dwStartingRecordId,
                                nMaxUsers,
                                pdwNumUsersFound,
                                pppUserInfoList
                                );
            break;
        }
        case 2:
        {
            dwError = LsaProviderLocal_DbEnumUsers_2(
                                hDb,
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
LsaProviderLocal_DbFindUserById(
    HANDLE hDb,
    uid_t  uid,
    DWORD  dwUserInfoLevel,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_USER_LEVEL;

    switch(dwUserInfoLevel)
    {
        case 0:
        {
            dwError = LsaProviderLocal_DbFindUserById_0(hDb, uid, ppUserInfo);
            break;
        }
        case 1:
        {
            dwError = LsaProviderLocal_DbFindUserById_1(hDb, uid, ppUserInfo);
            break;
        }
        case 2:
        {
            dwError = LsaProviderLocal_DbFindUserById_2(hDb, uid, ppUserInfo);
            break;
        }
    }

    return dwError;
}

DWORD
LsaProviderLocal_DbGetGroupsForUser_0_Unsafe(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
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
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    DWORD dwNumGroupsFound = 0;
    DWORD dwGroupInfoLevel = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_USER_GROUPS_0_BY_UID,
                               uid);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                               );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
        goto cleanup;
    }

    if ((nCols != nExpectedCols)) {
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

    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pdwGroupsFound = dwNumGroupsFound;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    if (ppGroupInfoList) {
       LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }

    *pppGroupInfoList = NULL;
    *pdwGroupsFound = 0;

    goto cleanup;
}

DWORD
LsaProviderLocal_DbGetGroupsForUser_1_Unsafe(
    HANDLE  hDb,
    uid_t uid,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
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
    DWORD dwNumGroupsFound = 0;
    DWORD dwGroupInfoLevel = 1;
    DWORD iGroup = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_USER_GROUPS_1_BY_UID,
                               uid);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                               );
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
        goto cleanup;
    }

    if ((nCols != nExpectedCols)) {
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

    *pppGroupInfoList = (PVOID*)ppGroupInfoList;
    *pdwGroupsFound = dwNumGroupsFound;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    if (ppGroupInfoList) {
       LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }

    *pppGroupInfoList = NULL;
    *pdwGroupsFound = 0;

    goto cleanup;
}


DWORD
LsaProviderLocal_DbFindUserById_0_Unsafe(
    HANDLE  hDb,
    uid_t   uid,
    PVOID*  ppUserInfo
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 7;
    PLSA_USER_INFO_0* ppUserInfoList = NULL;
    DWORD dwUserInfoLevel = 0;
    DWORD dwNumUsersFound = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_USER_0_BY_UID,
                               uid);

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
       dwError = LSA_ERROR_NO_SUCH_USER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbWriteToUserInfo_0_Unsafe(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        &ppUserInfoList,
                        &dwNumUsersFound
                        );
    BAIL_ON_LSA_ERROR(dwError);

    *ppUserInfo = *ppUserInfoList;
    *ppUserInfoList = NULL;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    *ppUserInfo = NULL;

    goto cleanup;
}


DWORD
LsaProviderLocal_DbFindUserById_0(
    HANDLE hDb,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;

    ENTER_RW_READER_LOCK;

    dwError = LsaProviderLocal_DbFindUserById_0_Unsafe(
                hDb,
                uid,
                ppUserInfo
                );

    LEAVE_RW_READER_LOCK;

    return dwError;
}


DWORD
LsaProviderLocal_DbFindUserById_1(
    HANDLE hDb,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 7;
    PLSA_USER_INFO_1* ppUserInfoList = NULL;
    DWORD dwNumUsersFound = 0;
    DWORD dwUserInfoLevel = 1;
    PBYTE pLMHash = NULL;
    DWORD dwLMHashLen = 0;
    PBYTE pNTHash = NULL;
    DWORD dwNTHashLen = 0;
    DWORD iUser = 0;

    ENTER_RW_READER_LOCK;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_USER_1_BY_UID,
                               uid);

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
       dwError = LSA_ERROR_NO_SUCH_USER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbWriteToUserInfo_1_Unsafe(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        &ppUserInfoList,
                        &dwNumUsersFound);
    BAIL_ON_LSA_ERROR(dwError);

    for (iUser = 0; iUser < dwNumUsersFound; iUser++)
    {
        PLSA_USER_INFO_1 pUserInfo = *(ppUserInfoList+iUser);

        dwError = LsaProviderLocal_DbGetLMHash_Unsafe(
                            hDb,
                            pUserInfo->uid,
                            &pLMHash,
                            &dwLMHashLen);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaProviderLocal_DbGetNTHash_Unsafe(
                            hDb,
                            pUserInfo->uid,
                            &pNTHash,
                            &dwNTHashLen
                            );
        BAIL_ON_LSA_ERROR(dwError);

        pUserInfo->pLMHash = pLMHash;
        pLMHash = NULL;
        pUserInfo->dwLMHashLen = dwLMHashLen;
        pUserInfo->pNTHash = pNTHash;
        pNTHash = NULL;
        pUserInfo->dwNTHashLen = dwNTHashLen;
    }

    *ppUserInfo = *(ppUserInfoList);
    *ppUserInfoList = NULL;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }

    LEAVE_RW_READER_LOCK;

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    LSA_SAFE_FREE_MEMORY(pLMHash);
    LSA_SAFE_FREE_MEMORY(pNTHash);

    *ppUserInfo = NULL;

    goto cleanup;
}


DWORD
LsaProviderLocal_DbFindUserById_2(
    HANDLE hDb,
    uid_t  uid,
    PVOID* ppUserInfo
    )
{
    DWORD dwError = 0;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int  nRows = 0;
    int  nCols = 0;
    PSTR* ppszResult = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    DWORD nExpectedCols = 10;
    PLSA_USER_INFO_2* ppUserInfoList = NULL;
    DWORD dwNumUsersFound = 0;
    DWORD dwUserInfoLevel = 2;
    PBYTE pNTHash = NULL;
    DWORD dwNTHashLen = 0;
    PBYTE pLMHash = NULL;
    DWORD dwLMHashLen = 0;
    DWORD iUser = 0;

    ENTER_RW_READER_LOCK;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_USER_2_BY_UID,
                               uid);

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
       dwError = LSA_ERROR_NO_SUCH_USER;
       BAIL_ON_LSA_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbWriteToUserInfo_2_Unsafe(
                        ppszResult,
                        nRows,
                        nCols,
                        nExpectedCols,
                        &ppUserInfoList,
                        &dwNumUsersFound
                        );
    BAIL_ON_LSA_ERROR(dwError);

    for (iUser = 0; iUser < dwNumUsersFound; iUser++)
    {
        PLSA_USER_INFO_2 pUserInfo = *(ppUserInfoList+iUser);

        dwError = LsaProviderLocal_DbGetLMHash_Unsafe(
                        hDb,
                        pUserInfo->uid,
                        &pLMHash,
                        &dwLMHashLen
                        );
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaProviderLocal_DbGetNTHash_Unsafe(
                        hDb,
                        pUserInfo->uid,
                        &pNTHash,
                        &dwNTHashLen
                        );
        BAIL_ON_LSA_ERROR(dwError);

        pUserInfo->pLMHash = pLMHash;
        pLMHash = NULL;
        pUserInfo->dwLMHashLen = dwLMHashLen;
        pUserInfo->pNTHash = pNTHash;
        pNTHash = NULL;
        pUserInfo->dwNTHashLen = dwNTHashLen;
    }

    *ppUserInfo = *ppUserInfoList;
    *ppUserInfoList = NULL;

cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }

    LEAVE_RW_READER_LOCK;

    return dwError;

error:

    if (pszError) {
       LSA_LOG_ERROR("%s", pszError);
    }

    LSA_SAFE_FREE_MEMORY(pNTHash);
    LSA_SAFE_FREE_MEMORY(pLMHash);

    *ppUserInfo = NULL;

    goto cleanup;
}

DWORD
LsaProviderLocal_DbGetGroupsForUser(
    HANDLE  hDb,
    uid_t   uid,
    DWORD   dwGroupInfoLevel,
    PDWORD  pdwGroupsFound,
    PVOID** pppGroupInfoList
    )
{
    DWORD dwError = LSA_ERROR_UNSUPPORTED_GROUP_LEVEL;

    switch(dwGroupInfoLevel)
    {
        case 0:
        {
            dwError = LsaProviderLocal_DbGetGroupsForUser_0(
                                hDb,
                                uid,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;
        }
        case 1:
        {
            dwError = LsaProviderLocal_DbGetGroupsForUser_1(
                                hDb,
                                uid,
                                pdwGroupsFound,
                                pppGroupInfoList
                                );
            break;
        }

    }

    return dwError;
}


