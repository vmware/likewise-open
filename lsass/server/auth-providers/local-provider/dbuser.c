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

// Start from here

DWORD
LsaProviderLocal_DbUpdateHash_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszDbStatement,
    PBYTE  pHash,
    DWORD  dwHashLen
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    DWORD dwHashVal_1 = 0;
    DWORD dwHashVal_2 = 0;
    DWORD dwHashVal_3 = 0;
    DWORD dwHashVal_4 = 0;
    PBYTE pTmpHash = NULL;

    if (dwHashLen != 16) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // TODO: Adjust for endian-ness
    pTmpHash = pHash;
    memcpy(&dwHashVal_1, pTmpHash, sizeof(dwHashVal_1));
    pTmpHash += sizeof(dwHashVal_1);
    memcpy(&dwHashVal_2, pTmpHash, sizeof(dwHashVal_2));
    pTmpHash += sizeof(dwHashVal_2);
    memcpy(&dwHashVal_3, pTmpHash, sizeof(dwHashVal_3));
    pTmpHash += sizeof(dwHashVal_3);
    memcpy(&dwHashVal_4, pTmpHash, sizeof(dwHashVal_4));

    pszQuery = sqlite3_mprintf(pszDbStatement,
                               dwHashVal_1,
                               dwHashVal_2,
                               dwHashVal_3,
                               dwHashVal_4,
                               time(NULL),
                               uid);

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

    return dwError;

error:

    if (pszError) {
        LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}


DWORD
LsaProviderLocal_DbUpdateNTHash_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    PBYTE  pHash,
    DWORD  dwHashLen
    )
{
    return LsaProviderLocal_DbUpdateHash_Unsafe(
                hDb,
                uid,
                DB_QUERY_UPDATE_NT_OWF_FOR_UID,
                pHash,
                dwHashLen
                );
}

DWORD
LsaProviderLocal_DbChangePassword(
    HANDLE hDb,
    uid_t uid,
    PCSTR pszPassword
    )
{
    DWORD dwError = 0;
    PBYTE pNTHash = NULL;
    DWORD dwNTHashLen = 0;
    PBYTE pLMHash = NULL;
#ifdef NOT_YET
    DWORD dwLMHashLen = 0;
#endif
    BOOLEAN bReleaseLock = FALSE;

#ifdef NOT_YET
    dwError = LsaSrvComputeLMHash(
                      pszPassword,
                      &pLMHash,
                      &dwLMHashLen);
    BAIL_ON_LSA_ERROR(dwError);
#endif

    dwError = LsaSrvComputeNTHash(
                      pszPassword,
                      &pNTHash,
                      &dwNTHashLen);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_RW_WRITER_LOCK;
    bReleaseLock = TRUE;

    // TODO: Implement the update within a transaction

    dwError = LsaProviderLocal_DbUpdateNTHash_Unsafe(
                        hDb,
                        uid,
                        pNTHash,
                        dwNTHashLen);
    BAIL_ON_LSA_ERROR(dwError);

#ifdef NOT_YET
    dwError = LsaProviderLocal_DbUpdateLMHash_Unsafe(
                        hDb,
                        uid,
                        pLMHash,
                        dwLMHashLen);
    BAIL_ON_LSA_ERROR(dwError);
#endif

cleanup:

    if (bReleaseLock) {
       LEAVE_RW_WRITER_LOCK;
    }

    LSA_SAFE_FREE_MEMORY(pLMHash);
    LSA_SAFE_FREE_MEMORY(pNTHash);

    return dwError;

error:

    goto cleanup;
}


DWORD
LsaProviderLocal_DbAddUser(
    HANDLE hDb,
    DWORD  dwUserInfoLevel,
    PVOID  pUserInfo
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR pszError = NULL;
    PSTR pszQuery = NULL;
    PLSA_USER_INFO_0 pUser = NULL;
    BOOLEAN bReleaseLock = FALSE;
    PVOID pExistingUserInfo = NULL;
    PVOID pGroupInfo = NULL;
    DWORD  dwExistingUserInfoLevel = 0;
    DWORD  dwGroupInfoLevel = 0;

    if (dwUserInfoLevel != 0) {
        dwError = LSA_ERROR_INVALID_USER_INFO_LEVEL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pUser = (PLSA_USER_INFO_0)pUserInfo;

    ENTER_RW_WRITER_LOCK;
    bReleaseLock = TRUE;

    if (IsNullOrEmptyString(pUser->pszName)) {
        dwError = LSA_ERROR_INVALID_LOGIN_ID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (IsNullOrEmptyString(pUser->pszHomedir)) {
        dwError = LSA_ERROR_INVALID_HOMEDIR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    // This will not let root be added - which is ok.
    if (!pUser->uid) {

       // We got a write lock; nobody can steal this uid from us
       dwError = LsaProviderLocal_DbGetNextAvailableUid_Unsafe(
                           hDb,
                           &pUser->uid
                           );
       BAIL_ON_LSA_ERROR(dwError);

    } else {

       dwError = LsaProviderLocal_DbFindUserById_0_Unsafe(
                           hDb,
                           pUser->uid,
                           &pExistingUserInfo
                           );
       if (dwError) {
          if (dwError == LSA_ERROR_NO_SUCH_USER) {
              dwError = 0;
          } else {
             BAIL_ON_LSA_ERROR(dwError);
          }
       } else {
          dwError = LSA_ERROR_USER_EXISTS;
          BAIL_ON_LSA_ERROR(dwError);
       }
    }

    dwError = LsaProviderLocal_DbFindUserByName_0_Unsafe(
                           hDb,
                           pUser->pszName,
                           &pExistingUserInfo);
    if (dwError) {
           if (dwError == LSA_ERROR_NO_SUCH_USER) {
               dwError = 0;
           } else {
               BAIL_ON_LSA_ERROR(dwError);
           }
    } else {
           dwError = LSA_ERROR_USER_EXISTS;
           BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaProviderLocal_DbFindGroupById_0_Unsafe(
                        hDb,
                        pUser->gid,
                        &pGroupInfo
                        );
    BAIL_ON_LSA_ERROR(dwError);

    pszQuery = sqlite3_mprintf(DB_QUERY_INSERT_USER,
                               pUser->pszName,
                               pUser->pszPasswd,
                               pUser->uid,
                               pUser->gid,
                               pUser->pszGecos,
                               pUser->pszHomedir,
                               pUser->pszShell,
                               pUser->pszName);

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

    if (bReleaseLock) {
        LEAVE_RW_WRITER_LOCK;
    }

    if (pExistingUserInfo) {
        LsaFreeUserInfo(dwExistingUserInfoLevel, pExistingUserInfo);
    }

    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError)) {
       LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

DWORD
LsaProviderLocal_DbModifyUser(
    HANDLE hDb,
    PLSA_USER_MOD_INFO pUserModInfo
    )
{
    DWORD dwError = 0;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;

    // TODO: Implement this in a database transaction

    dwError = LsaProviderLocal_DbFindUserById(
                        hDb,
                        pUserModInfo->uid,
                        dwUserInfoLevel,
                        &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);

    if (pUserModInfo->actions.bEnableUser) {
        dwError = LsaProviderLocal_DbEnableUser(
                       hDb,
                       pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bDisableUser) {
        dwError = LsaProviderLocal_DbDisableUser(
                        hDb,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bUnlockUser) {
        dwError = LsaProviderLocal_DbUnlockUser(
                        hDb,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetChangePasswordOnNextLogon) {
        dwError = LsaProviderLocal_DbSetChangePasswordOnNextLogon(
                        hDb,
                        pUserModInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaProviderLocal_DbSetPasswordExpires(
                        hDb,
                        pUserModInfo->uid,
                        TRUE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetAccountExpiryDate) {
        dwError = LsaProviderLocal_DbSetAccountExpiryDate(
                        hDb,
                        pUserModInfo->uid,
                        pUserModInfo->pszExpiryDate);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bRemoveFromGroups) {
        dwError = LsaProviderLocal_DbRemoveFromGroups(
                        hDb,
                        pUserModInfo->uid,
                        pUserModInfo->pszRemoveFromGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bAddToGroups) {
        dwError = LsaProviderLocal_DbAddToGroups(
                        hDb,
                        pUserModInfo->uid,
                        pUserModInfo->pszAddToGroups);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetPasswordMustExpire) {
        dwError = LsaProviderLocal_DbSetPasswordExpires(
                        hDb,
                        pUserModInfo->uid,
                        TRUE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    if (pUserModInfo->actions.bSetPasswordNeverExpires) {
        dwError = LsaProviderLocal_DbSetPasswordExpires(
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
}
