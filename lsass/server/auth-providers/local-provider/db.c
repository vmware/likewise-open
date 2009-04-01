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
LsaLPDbCheckGroupMembershipRecord_Unsafe(
    HANDLE   hDb,
    uid_t    uid,
    gid_t    gid,
    PBOOLEAN pbExists)
{   
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int nRows = 0;
    int nCols = 0;
    PSTR* ppszResult = NULL;
    
    pszQuery = sqlite3_mprintf(DB_QUERY_CHECK_GROUP_MEMBERSHIP,
                               uid,
                               gid);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                               );
    BAIL_ON_LSA_ERROR(dwError);
    
    if (!nRows) {
        *pbExists = FALSE;
        goto cleanup;
    }
    
    // The login id is unique. This cannot be!
    if ((nRows > 1) || (nCols != 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *pbExists = (atoi(ppszResult[1]) > 0);
    
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
    
    *pbExists = FALSE;

    goto cleanup;
}


DWORD
LsaLPDbGetNextAvailableUid_Unsafe(
    HANDLE hDb,
    uid_t* pUID
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int nRows = 0;
    int nCols = 0;
    PSTR* ppszResult = NULL;
    uid_t avblUID = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_MAX_UID);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                                );
    BAIL_ON_LSA_ERROR(dwError);
        
    if (!nRows) {
       // Where should we start?
       avblUID = 10000;
    } else if ((nRows > 1) || (nCols != 1)) {
       dwError = LSA_ERROR_DATA_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    } else if (nRows == 1){
       avblUID = (IsNullOrEmptyString(ppszResult[1]) ? 0 : atoi(ppszResult[1]));
       avblUID = (avblUID ? avblUID + 1 : 10000);
    }
    
    *pUID = avblUID;
        
cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }
        
    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    return dwError;
    
error:

    *pUID = 0;
    
    goto cleanup;
}


DWORD
LsaLPDbGetNextAvailableGid_Unsafe(
    HANDLE hDb,
    gid_t* pGID
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR pszQuery = NULL;
    PSTR pszError = NULL;
    int nRows = 0;
    int nCols = 0;
    PSTR* ppszResult = NULL;
    gid_t avblGID = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_MAX_GID);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                                );
    BAIL_ON_LSA_ERROR(dwError);
        
    if (!nRows) {
       // Where should we start?
       avblGID = 10000;
    } else if ((nRows > 1) || (nCols != 1)) {
       dwError = LSA_ERROR_DATA_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    } else if (nRows == 1){
       avblGID = (IsNullOrEmptyString(ppszResult[1]) ? 0 : atoi(ppszResult[1]));
       avblGID = (avblGID ? avblGID + 1 : 10000);
    }
    
    *pGID = avblGID;
        
cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }
        
    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    return dwError;
    
error:

    *pGID = 0;
    
    goto cleanup;
}

DWORD
LsaLPDbGetLMHash_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    PBYTE* ppHash,
    PDWORD pdwHashLen
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PBYTE pHash = NULL;
    DWORD dwHashLen = 16;
    PSTR  pszQuery = NULL;
    PSTR* ppszResult = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR  pszError = NULL;
    DWORD nExpectedCols = 4;
    DWORD iVal = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_LM_OWF_FOR_UID, uid);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(BYTE) * dwHashLen,
                    (PVOID*)&pHash);
    BAIL_ON_LSA_ERROR(dwError);

    // Skip the column headers
    for (iVal = 0; iVal < nExpectedCols; iVal++)
    {
        DWORD dwHashValue = atoi(ppszResult[nExpectedCols + iVal]);
        memcpy(pHash + (iVal * sizeof(dwHashValue)),
               &dwHashValue,
               sizeof(dwHashValue)); 
    }
    
    *ppHash = pHash;
    *pdwHashLen = dwHashLen;
    
cleanup:

    return dwError;
    
error:

    *ppHash = NULL;
    *pdwHashLen = 0;
    
    if (pHash) {
        LsaFreeMemory(pHash);
    }

    goto cleanup;
}

DWORD
LsaLPDbGetNTHash_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    PBYTE* ppHash,
    PDWORD pdwHashLen
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PBYTE pHash = NULL;
    DWORD dwHashLen = 16;
    PSTR  pszQuery = NULL;
    PSTR* ppszResult = NULL;
    int   nRows = 0;
    int   nCols = 0;
    PSTR  pszError = NULL;
    DWORD nExpectedCols = 4;
    DWORD iVal = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_NT_OWF_FOR_UID, uid);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    if (!nRows) {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    if ((nCols != nExpectedCols) || (nRows > 1)) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(BYTE) * dwHashLen,
                    (PVOID*)&pHash);
    BAIL_ON_LSA_ERROR(dwError);

    // Skip the column headers
    for (iVal = 0; iVal < nExpectedCols; iVal++)
    {
        DWORD dwHashValue = atoi(ppszResult[nExpectedCols + iVal]);
        memcpy(pHash + (iVal * sizeof(dwHashValue)),
               &dwHashValue,
               sizeof(dwHashValue)); 
    }
    
    *ppHash = pHash;
    *pdwHashLen = dwHashLen;
    
cleanup:

    return dwError;
    
error:

    *ppHash = NULL;
    *pdwHashLen = 0;
    
    if (pHash) {
        LsaFreeMemory(pHash);
    }

    goto cleanup;
}


DWORD
LsaLPDbWriteToGroupInfo_0_Unsafe(
    PSTR*  ppszResult,
    int    nRows,
    int    nCols,
    DWORD  nHeaderColsToSkip,
    PLSA_GROUP_INFO_0** pppGroupInfoList,
    PDWORD pdwNumGroupsFound
    )
{
    DWORD dwError = 0;
    DWORD iCol = 0, iRow = 0;
    DWORD iVal = nHeaderColsToSkip;
    DWORD dwGroupInfoLevel = 0;
    PLSA_GROUP_INFO_0* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_0 pGroupInfo = NULL;
    DWORD dwNumGroupsFound = nRows;
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_GROUP_INFO_0) * nRows,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iRow = 0; iRow < nRows; iRow++) {
        
        dwError = LsaAllocateMemory(
                        sizeof(LSA_GROUP_INFO_0),
                        (PVOID*)&pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (iCol = 0; iCol < nCols; iCol++) {
            switch(iCol) {
                case 0: /* Name */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pGroupInfo->pszName);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 1: /* Gid */
                {
                    pGroupInfo->gid = atoi(ppszResult[iVal]);
                    break;
                }
            }
            iVal++;
        }
        
        dwError = LsaAllocateStringPrintf(
        				&pGroupInfo->pszSid,
        				LOCAL_GROUP_SID_FORMAT,
        				pGroupInfo->gid);
        BAIL_ON_LSA_ERROR(dwError);
        
        *(ppGroupInfoList + iRow) = pGroupInfo;
        pGroupInfo = NULL;
    }
    
    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;
    
cleanup:
    
    return dwError;
    
error:

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }
    
    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;
    
    goto cleanup;
}


DWORD
LsaLPDbWriteToGroupInfo_1_Unsafe(
    PSTR*  ppszResult,
    int    nRows,
    int    nCols,
    DWORD  nHeaderColsToSkip,
    PLSA_GROUP_INFO_1** pppGroupInfoList,
    PDWORD pdwNumGroupsFound
    )
{
    DWORD dwError = 0;
    DWORD iCol = 0, iRow = 0;
    DWORD iVal = nHeaderColsToSkip;
    DWORD dwGroupInfoLevel = 1;
    PLSA_GROUP_INFO_1* ppGroupInfoList = NULL;
    PLSA_GROUP_INFO_1 pGroupInfo = NULL;
    DWORD dwNumGroupsFound = nRows;
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_GROUP_INFO_1) * nRows,
                    (PVOID*)&ppGroupInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iRow = 0; iRow < nRows; iRow++) {
        
        dwError = LsaAllocateMemory(
                        sizeof(LSA_GROUP_INFO_1),
                        (PVOID*)&pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (iCol = 0; iCol < nCols; iCol++) {
            switch(iCol) {
                case 0: /* Name */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pGroupInfo->pszName);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 1: /* Passwd */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pGroupInfo->pszPasswd);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 2: /* Gid */
                {
                    pGroupInfo->gid = atoi(ppszResult[iVal]);
                    break;
                }
            }
            iVal++;
        }
        
        dwError = LsaAllocateStringPrintf(
        				&pGroupInfo->pszSid,
        				LOCAL_GROUP_SID_FORMAT,
        				pGroupInfo->gid);
        BAIL_ON_LSA_ERROR(dwError);
        
        *(ppGroupInfoList + iRow) = pGroupInfo;
        pGroupInfo = NULL;
    }
    
    *pppGroupInfoList = ppGroupInfoList;
    *pdwNumGroupsFound = dwNumGroupsFound;
    
cleanup:
    
    return dwError;
    
error:

    if (ppGroupInfoList) {
        LsaFreeGroupInfoList(dwGroupInfoLevel, (PVOID*)ppGroupInfoList, dwNumGroupsFound);
    }
    
    if (pGroupInfo) {
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }
    
    *pppGroupInfoList = NULL;
    *pdwNumGroupsFound = 0;
    
    goto cleanup;
}


DWORD
LsaLPDbWriteMembersToGroupInfo_1(
    PSTR*  ppszResult,
    int    nRows,
    int    nCols,
    DWORD  nHeaderColsToSkip,
    PLSA_GROUP_INFO_1 pGroupInfo
    )
{
    DWORD dwError = 0;
    DWORD iCol = 0, iRow = 0;
    DWORD iVal = nHeaderColsToSkip;
    PSTR* ppszMembers = NULL;
    
    if (nCols != 1) {
        dwError = EINVAL;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    dwError = LsaAllocateMemory((sizeof(PSTR)*(nRows+1)), (PVOID*)&ppszMembers);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iRow = 0; iRow < nRows; iRow++) {
        for (iCol = 0; iCol < nCols; iCol++) {
            switch(iCol) {
                case 0: /* Name */
                {
                    // We are certain that the group member is non-null
                    dwError = LsaAllocateString(ppszResult[iVal], ppszMembers+iRow);
                    BAIL_ON_LSA_ERROR(dwError);
                    break;
                }
            }
            iVal++;
        }
    }
    
    pGroupInfo->ppszMembers = ppszMembers;
    
cleanup:

    return dwError;
    
error:

    if (ppszMembers) {
        // We allocated one more string (nRows + 1)
        // but, that terminating string is NULL
        // So, we free only nRows here. However,
        // it will free all the pointers allocated.
        LsaFreeStringArray(ppszMembers, nRows);
    }
    
    pGroupInfo->ppszMembers = NULL;
    
    goto cleanup;
}

DWORD
LsaLPDbWriteToUserInfo_0_Unsafe(
        PSTR* ppszResult,
        int nRows,
        int nCols,
        DWORD nHeaderColsToSkip,
        PLSA_USER_INFO_0** pppUserInfoList,
        PDWORD pdwNumUsersFound
        )
{
    DWORD dwError = 0;
    DWORD iCol = 0, iRow = 0;
    DWORD iVal = nHeaderColsToSkip;
    PLSA_USER_INFO_0* ppUserInfoList = NULL;
    PLSA_USER_INFO_0 pUserInfo = NULL;
    DWORD dwNumUsersFound = nRows;
    DWORD dwUserInfoLevel = 0;
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_USER_INFO_0) * dwNumUsersFound,
                    (PVOID*)&ppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iRow = 0; iRow < nRows; iRow++) {
        
        dwError = LsaAllocateMemory(
                        sizeof(LSA_USER_INFO_0),
                        (PVOID*)&pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        for (iCol = 0; iCol < nCols; iCol++) {
            switch(iCol) {
                case 0: /* Name */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszName);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 1: /* Passwd */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszPasswd);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 2: /* Uid */
                {
                    pUserInfo->uid = atoi(ppszResult[iVal]);
                    break;
                }
                case 3: /* Gid */
                {
                    pUserInfo->gid = atoi(ppszResult[iVal]);    
                    break;
                }
                case 4: /* Gecos */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszGecos);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 5: /* HomeDir */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszHomedir);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 6: /* Shell */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszShell);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
            }
            iVal++;
        }
        
        dwError = LsaAllocateStringPrintf(
        				&pUserInfo->pszSid,
        				LOCAL_USER_SID_FORMAT,
        				pUserInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
        
        *(ppUserInfoList + iRow) = pUserInfo;
        pUserInfo = NULL;
    }
    
    *pppUserInfoList = ppUserInfoList;
    *pdwNumUsersFound = dwNumUsersFound;
    
cleanup:
    
    return dwError;
    
error:

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    
    *pppUserInfoList = NULL;
    *pdwNumUsersFound = 0;

    goto cleanup;
}


DWORD
LsaLPDbWriteToUserInfo_1_Unsafe(
        PSTR* ppszResult,
        int nRows,
        int nCols,
        DWORD nHeaderColsToSkip,
        PLSA_USER_INFO_1** pppUserInfoList,
        PDWORD pdwNumUsersFound
        )
{
    DWORD dwError = 0;
    DWORD iCol = 0, iRow = 0;
    DWORD iVal = nHeaderColsToSkip;
    PLSA_USER_INFO_1* ppUserInfoList = NULL;
    PLSA_USER_INFO_1 pUserInfo = NULL;
    DWORD dwNumUsersFound = nRows;
    DWORD dwUserInfoLevel = 1;
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_USER_INFO_1) * dwNumUsersFound,
                    (PVOID*)&ppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iRow = 0; iRow < nRows; iRow++) {
        
        dwError = LsaAllocateMemory(
                    sizeof(LSA_USER_INFO_1),
                    (PVOID*)&pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        pUserInfo->bIsLocalUser = TRUE;
        
        for (iCol = 0; iCol < nCols; iCol++) {
            switch(iCol) {
                case 0: /* Name */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszName);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 1: /* Passwd */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszPasswd);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 2: /* Uid */
                {
                    pUserInfo->uid = atoi(ppszResult[iVal]);
                    break;
                }
                case 3: /* Gid */
                {
                    pUserInfo->gid = atoi(ppszResult[iVal]);    
                    break;
                }
                case 4: /* Gecos */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszGecos);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 5: /* HomeDir */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszHomedir);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 6: /* Shell */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszShell);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
            }
            iVal++;
        }
        
        dwError = LsaAllocateStringPrintf(
        				&pUserInfo->pszSid,
        				LOCAL_USER_SID_FORMAT,
        				pUserInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
        
        *(ppUserInfoList + iRow) = pUserInfo;
        pUserInfo = NULL;
    }
    
    *pppUserInfoList = ppUserInfoList;
    *pdwNumUsersFound = dwNumUsersFound;
    
cleanup:
    
    return dwError;
    
error:

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }

    *pppUserInfoList = NULL;
    *pdwNumUsersFound = 0;
    
    goto cleanup;
}


DWORD
LsaLPDbWriteToUserInfo_2_Unsafe(
    PSTR* ppszResult,
    int nRows,
    int nCols,
    DWORD nHeaderColsToSkip,
    PLSA_USER_INFO_2** pppUserInfoList,
    PDWORD pdwNumUsersFound
    )
{
    DWORD dwError = 0;
    DWORD iCol = 0, iRow = 0;
    DWORD iVal = nHeaderColsToSkip;
    PLSA_USER_INFO_2* ppUserInfoList = NULL;
    PLSA_USER_INFO_2 pUserInfo = NULL;
    DWORD dwNumUsersFound = nRows;
    DWORD dwUserInfoLevel = 2;
    
    dwError = LsaAllocateMemory(
                    sizeof(PLSA_USER_INFO_2) * dwNumUsersFound,
                    (PVOID*)&ppUserInfoList);
    BAIL_ON_LSA_ERROR(dwError);
    
    for (iRow = 0; iRow < nRows; iRow++) {
        DWORD dwUserInfoFlags = 0;
        
        dwError = LsaAllocateMemory(
                        sizeof(LSA_USER_INFO_2),
                        (PVOID*)&pUserInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        pUserInfo->bIsLocalUser = TRUE;
        
        for (iCol = 0; iCol < nCols; iCol++) {
            switch(iCol) {
                case 0: /* Name */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszName);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 1: /* Passwd */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszPasswd);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 2: /* Uid */
                {
                    pUserInfo->uid = atoi(ppszResult[iVal]);
                    break;
                }
                case 3: /* Gid */
                {
                    pUserInfo->gid = atoi(ppszResult[iVal]);    
                    break;
                }
                case 4: /* Gecos */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszGecos);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 5: /* HomeDir */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszHomedir);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 6: /* Shell */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal])) {
                       dwError = LsaAllocateString(ppszResult[iVal], &pUserInfo->pszShell);
                       BAIL_ON_LSA_ERROR(dwError);
                    }
                    break;
                }
                case 7: /* UserInfoFlags */
                {
                    dwUserInfoFlags = atoi(ppszResult[iVal]);
                    
                    pUserInfo->bUserCanChangePassword = ((dwUserInfoFlags & (DWORD)LW_CANNOT_CHANGE_PASSWORD) == 0);
                    
                    break;
                }
                case 8: /* Account Expiry */
                {
                    time_t accountExpirationTime = atoi(ppszResult[iVal]);

                    if (accountExpirationTime &&
                        (difftime(accountExpirationTime, time(NULL)) < 0)) {
                        pUserInfo->bAccountExpired = TRUE;
                    } else {
                        pUserInfo->bAccountExpired = FALSE;
                    }
                    
                    break;
                }
                case 9: /* Last Password Change Time */
                {
                    if ((dwUserInfoFlags & LW_PASSWORD_CANNOT_EXPIRE) != 0) {
                        
                        pUserInfo->bPasswordExpired = FALSE;
                        pUserInfo->bPasswordNeverExpires = TRUE;
                       
                    } else {
                        
                        DWORD  dwSecondsInDay = (24 * 60 * 60);
                        DWORD  dwSecondsToPasswdExpiry = 0;
                        time_t lastPasswdChangeTime = atoi(ppszResult[iVal]);
                        time_t curTime = time(NULL);
                        DWORD dwPasswdChangeInterval = LsaLPGetPasswdChangeInterval();
                        DWORD dwPasswdChangeWarningTime = LsaLPGetPasswdChangeWarningTime();

                        pUserInfo->bPasswordNeverExpires = FALSE;
                        
                        dwSecondsToPasswdExpiry = difftime(curTime, lastPasswdChangeTime);
                                                
                        if (dwSecondsToPasswdExpiry > 
                            (dwPasswdChangeInterval * dwSecondsInDay))
                        {
                            pUserInfo->bPasswordExpired = TRUE;
                        }
                        else
                        {
                            time_t dwTimeToStartPrompting = 0;
                            
                            pUserInfo->bPasswordExpired = FALSE;
                            
                            dwTimeToStartPrompting = (lastPasswdChangeTime + 
                                        dwPasswdChangeWarningTime);
                            if (curTime >= dwTimeToStartPrompting)
                            {
                                pUserInfo->bPromptPasswordChange = TRUE;
                            }
                            
                            pUserInfo->dwDaysToPasswordExpiry = 
                                (dwSecondsToPasswdExpiry > dwSecondsInDay ? (DWORD)(dwSecondsToPasswdExpiry/dwSecondsInDay) : 1);
                        }
                        
                        if (pUserInfo->bPasswordExpired) {
                            pUserInfo->bPromptPasswordChange = TRUE;
                        }
                    }
                    break;
                }
            }
            iVal++;
        }
        
        pUserInfo->bAccountDisabled = ((dwUserInfoFlags & LW_ACCOUNT_DISABLED) != 0);
        pUserInfo->bAccountLocked = ((dwUserInfoFlags & LW_ACCOUNT_LOCKED_OUT) != 0);
        
        dwError = LsaAllocateStringPrintf(
        				&pUserInfo->pszSid,
        				LOCAL_USER_SID_FORMAT,
        				pUserInfo->uid);
        BAIL_ON_LSA_ERROR(dwError);
            
        *(ppUserInfoList + iRow) = pUserInfo;
        pUserInfo = NULL;
    }
    
    *pppUserInfoList = ppUserInfoList;
    *pdwNumUsersFound = dwNumUsersFound;
    
cleanup:
    
    return dwError;
    
error:

    if (ppUserInfoList) {
        LsaFreeUserInfoList(dwUserInfoLevel, (PVOID*)ppUserInfoList, dwNumUsersFound);
    }
    if (pUserInfo) {
        LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    
    *pppUserInfoList = NULL;
    *pdwNumUsersFound = 0;
    
    goto cleanup;
}


DWORD
LsaLPDbFindUserByName_0_Unsafe(
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
    PLSA_USER_INFO_0* ppUserInfoList = NULL;
    DWORD dwUserInfoLevel = 0;
    DWORD dwNumUsersFound = 0;

    pszQuery = sqlite3_mprintf(DB_QUERY_FIND_USER_0_BY_NAME,
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
              
    dwError = LsaLPDbWriteToUserInfo_0_Unsafe(
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
LsaLPDbEnableUser(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwUserInfoFlags  &= (DWORD)~LW_ACCOUNT_DISABLED;
    
    dwError = LsaLPDbSetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LEAVE_RW_WRITER_LOCK;

    return dwError;
    
error:

    goto cleanup;
}
    
DWORD
LsaLPDbDisableUser(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwUserInfoFlags  |= (DWORD)LW_ACCOUNT_DISABLED;
    
    dwError = LsaLPDbSetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LEAVE_RW_WRITER_LOCK;

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaLPDbIsUserEnabled(
    HANDLE hDb,
    uid_t  uid,
    PBOOLEAN pbEnabled
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_READER_LOCK;
    
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                        hDb,
                        uid,
                        &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    *pbEnabled = !(dwUserInfoFlags & (DWORD)LW_ACCOUNT_DISABLED);
    
cleanup:

    LEAVE_RW_READER_LOCK;
    
    return dwError;
    
error:

    *pbEnabled = FALSE;
    
    goto cleanup;
}
    
DWORD
LsaLPDbUnlockUser(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwUserInfoFlags  &= (DWORD)~LW_ACCOUNT_LOCKED_OUT;
    
    dwError = LsaLPDbSetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LEAVE_RW_WRITER_LOCK;

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaLPDbLockUser(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwUserInfoFlags  |= (DWORD)LW_ACCOUNT_LOCKED_OUT;
    
    dwError = LsaLPDbSetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LEAVE_RW_WRITER_LOCK;

    return dwError;
    
error:

    goto cleanup;    
}

DWORD
LsaLPDbIsUserLocked(
    HANDLE   hDb,
    uid_t    uid,
    PBOOLEAN pbLocked
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
     
    ENTER_RW_READER_LOCK;
     
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                         hDb,
                         uid,
                         &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
     
    *pbLocked = ((dwUserInfoFlags & (DWORD)LW_ACCOUNT_LOCKED_OUT) != 0);
     
 cleanup:

     LEAVE_RW_READER_LOCK;
     
     return dwError;
     
 error:

     *pbLocked = FALSE;
     
     goto cleanup;    
}

DWORD
LsaLPDbAllowUserToChangePassword(
    HANDLE  hDb,
    uid_t   uid,
    BOOLEAN bAllow
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (bAllow) {
        dwUserInfoFlags &= (DWORD)~LW_CANNOT_CHANGE_PASSWORD;
    } else {
        dwUserInfoFlags |= (DWORD)LW_CANNOT_CHANGE_PASSWORD;
    }
    
    dwError = LsaLPDbSetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LEAVE_RW_WRITER_LOCK;

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaLPDbCanUserChangePassword(
    HANDLE  hDb,
    uid_t   uid,
    PBOOLEAN pbCanChangePassword
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
     
    ENTER_RW_READER_LOCK;
     
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                         hDb,
                         uid,
                         &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
     
    *pbCanChangePassword = ((dwUserInfoFlags & (DWORD)LW_CANNOT_CHANGE_PASSWORD) == 0);
     
 cleanup:

     LEAVE_RW_READER_LOCK;
     
     return dwError;
     
 error:

     *pbCanChangePassword = FALSE;
     
     goto cleanup;       
}

DWORD
LsaLPDbSetPasswordExpires(
    HANDLE  hDb,
    uid_t   uid,
    BOOLEAN bPasswordExpires
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (bPasswordExpires) {
        dwUserInfoFlags &= (DWORD)~LW_PASSWORD_CANNOT_EXPIRE;
    } else {
        dwUserInfoFlags |= (DWORD)LW_PASSWORD_CANNOT_EXPIRE;
    }
    
    dwError = LsaLPDbSetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LEAVE_RW_WRITER_LOCK;

    return dwError;
    
error:

    goto cleanup;
}

DWORD
LsaLPDbCheckPasswordExpires(
    HANDLE   hDb,
    uid_t    uid,
    PBOOLEAN pbPasswordExpires
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
      
    ENTER_RW_READER_LOCK;
      
    dwError = LsaLPDbGetUserInfoFlags_Unsafe(
                          hDb,
                          uid,
                          &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
      
    *pbPasswordExpires = ((dwUserInfoFlags & (DWORD)LW_PASSWORD_CANNOT_EXPIRE) == 0);
      
  cleanup:

     LEAVE_RW_READER_LOCK;
      
     return dwError;
      
  error:

     *pbPasswordExpires = FALSE;
      
     goto cleanup;      
}
    
DWORD
LsaLPDbSetChangePasswordOnNextLogon(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    BOOLEAN bInLock = FALSE;
    
    ENTER_RW_WRITER_LOCK;
    bInLock = TRUE;
    
    pszQuery = sqlite3_mprintf(DB_QUERY_UPDATE_LAST_PASSWORD_CHANGE_TIME,
                               0,
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
    
    if (bInLock) {
       LEAVE_RW_WRITER_LOCK;
    }

    return dwError;
    
error:

    if (!IsNullOrEmptyString(pszError)) {
        LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}
    
DWORD
LsaLPDbSetAccountExpiryDate(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszExpiryDate
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    BOOLEAN bInLock = FALSE;
    
    ENTER_RW_WRITER_LOCK;
    bInLock = TRUE;
    
    if (!IsNullOrEmptyString(pszExpiryDate)) {
        struct tm tmbuf = {0};
        
        if (strptime(pszExpiryDate, "%Y-%m-%d", &tmbuf) == NULL) {
           dwError = errno;
           BAIL_ON_LSA_ERROR(dwError);
        }
        
        pszQuery = sqlite3_mprintf(DB_QUERY_UPDATE_ACCOUNT_EXPIRY_DATE,
                               mktime(&tmbuf),
                               uid);
    } else {
        pszQuery = sqlite3_mprintf(DB_QUERY_UPDATE_ACCOUNT_EXPIRY_DATE_NEVER_EXPIRE,
                                   uid);
    }
    
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
    
    if (bInLock) {
       LEAVE_RW_WRITER_LOCK;
    }

    return dwError;
    
error:

    if (!IsNullOrEmptyString(pszError)) {
        LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}
    
DWORD
LsaLPDbRemoveFromGroups(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszGroupList
    )
{
    DWORD dwError = 0;
    PSTR  pszCopyGroupList = NULL;
    PSTR  pszToken = NULL;
    PSTR  pszTmp = NULL;
    BOOLEAN bInLock = FALSE;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    PDLINKEDLIST pGIDList = NULL;
    gid_t* pGID = NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    
    dwError = LsaLPDbFindUserById(
                    hDb,
                    uid,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    ENTER_RW_WRITER_LOCK;
    bInLock = TRUE;
    
    // TODO: Use Transactions
    
    dwError = LsaAllocateString(pszGroupList, &pszCopyGroupList);
    BAIL_ON_LSA_ERROR(dwError);
    
    pszToken = strtok_r(pszCopyGroupList, ",", &pszTmp);
    while (!IsNullOrEmptyString(pszToken))
    {
        LsaStripWhitespace(pszToken, TRUE, TRUE);
        // Check if the group exists
        dwError = LsaLPDbFindGroupByName_0_Unsafe(
                        hDb,
                        NULL,
                        pszToken,
                        &pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (((PLSA_USER_INFO_0)pUserInfo)->gid != ((PLSA_GROUP_INFO_1)pGroupInfo)->gid) {
        
            dwError = LsaAllocateMemory(sizeof(gid_t), (PVOID*)&pGID);
            BAIL_ON_LSA_ERROR(dwError);
        
            *pGID = ((PLSA_GROUP_INFO_0)pGroupInfo)->gid;
        
            dwError = LsaDLinkedListAppend(&pGIDList, pGID);
            BAIL_ON_LSA_ERROR(dwError);
        
            pGID = NULL;
        }
        
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
        pGroupInfo = NULL;
        
        pszToken = strtok_r(NULL, ",", &pszTmp);
    }
    
    if (pGIDList) {
        
       PDLINKEDLIST pListMember = pGIDList;
       
       for (; pListMember; pListMember = pListMember->pNext)
       {
           dwError = LsaLPDbRemoveGroupMembership_Unsafe(
                               hDb,
                               uid,
                               *((gid_t*)(pListMember->pItem)));
           BAIL_ON_LSA_ERROR(dwError);
       }
       
    }
    
cleanup:

    if (bInLock) {
       LEAVE_RW_WRITER_LOCK;
    }

    LSA_SAFE_FREE_MEMORY(pszCopyGroupList);
    LSA_SAFE_FREE_MEMORY(pGID);
    
    if (pGroupInfo) {
       LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }
    
    if (pUserInfo) {
       LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    
    if (pGIDList) {
       LsaDLinkedListForEach(pGIDList, &LsaLPDbFreeGIDInList, NULL);
       LsaDLinkedListFree(pGIDList);
    }

    return dwError;
    
error:

    goto cleanup;
}
    
DWORD
LsaLPDbAddToGroups(
    HANDLE hDb,
    uid_t  uid,
    PCSTR  pszGroupList
    )
{
    DWORD dwError = 0;
    PSTR  pszCopyGroupList = NULL;
    PSTR  pszToken = NULL;
    PSTR  pszTmp = NULL;
    BOOLEAN bInLock = FALSE;
    PVOID pGroupInfo = NULL;
    DWORD dwGroupInfoLevel = 0;
    PDLINKEDLIST pGIDList = NULL;
    gid_t* pGID = NULL;
    PVOID pUserInfo = NULL;
    DWORD dwUserInfoLevel = 0;
    
    dwError = LsaLPDbFindUserById(
                    hDb,
                    uid,
                    dwUserInfoLevel,
                    &pUserInfo);
    BAIL_ON_LSA_ERROR(dwError);
    
    ENTER_RW_WRITER_LOCK;
    bInLock = TRUE;
    
    // TODO: Use Transactions
    
    dwError = LsaAllocateString(pszGroupList, &pszCopyGroupList);
    BAIL_ON_LSA_ERROR(dwError);
    
    pszToken = strtok_r(pszCopyGroupList, ",", &pszTmp);
    while (!IsNullOrEmptyString(pszToken))
    {
        BOOLEAN bMembershipExists = FALSE;
        
        LsaStripWhitespace(pszToken, TRUE, TRUE);
        // Check if the group exists
        dwError = LsaLPDbFindGroupByName_0_Unsafe(
                        hDb,
                        NULL,
                        pszToken,
                        &pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (((PLSA_USER_INFO_0)pUserInfo)->gid == ((PLSA_GROUP_INFO_0)pGroupInfo)->gid) {
           bMembershipExists = TRUE;
        } else {
           dwError = LsaLPDbCheckGroupMembershipRecord_Unsafe(
                           hDb,
                           uid,
                           ((PLSA_GROUP_INFO_0)pGroupInfo)->gid,
                           &bMembershipExists);
        }
        
        if (!bMembershipExists) {
            
            dwError = LsaAllocateMemory(sizeof(gid_t), (PVOID*)&pGID);
            BAIL_ON_LSA_ERROR(dwError);
        
            *pGID = ((PLSA_GROUP_INFO_0)pGroupInfo)->gid;
        
            dwError = LsaDLinkedListAppend(&pGIDList, pGID);
            BAIL_ON_LSA_ERROR(dwError);
        
            pGID = NULL;
        }
        
        LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
        pGroupInfo = NULL;
        
        pszToken = strtok_r(NULL, ",", &pszTmp);
    }
    
    if (pGIDList) {
        
       PDLINKEDLIST pListMember = pGIDList;
       
       for (; pListMember; pListMember = pListMember->pNext)
       {
           dwError = LsaLPDbAddGroupMembership_Unsafe(
                               hDb,
                               uid,
                               *((gid_t*)(pListMember->pItem)));
           BAIL_ON_LSA_ERROR(dwError);
       }
       
    }
    
cleanup:

    if (bInLock) {
       LEAVE_RW_WRITER_LOCK;
    }

    LSA_SAFE_FREE_MEMORY(pszCopyGroupList);
    LSA_SAFE_FREE_MEMORY(pGID);
    
    if (pGroupInfo) {
       LsaFreeGroupInfo(dwGroupInfoLevel, pGroupInfo);
    }
    
    if (pUserInfo) {
       LsaFreeUserInfo(dwUserInfoLevel, pUserInfo);
    }
    
    if (pGIDList) {
       LsaDLinkedListForEach(pGIDList, &LsaLPDbFreeGIDInList, NULL);
       LsaDLinkedListFree(pGIDList);
    }

    return dwError;
    
error:

    goto cleanup;
}

VOID
LsaLPDbFreeGIDInList(
    PVOID pGID,
    PVOID pUserData
    )
{
    LSA_SAFE_FREE_MEMORY(pGID);
}

DWORD
LsaLPDbDeleteUser(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    
    ENTER_RW_WRITER_LOCK;
    
    pszQuery = sqlite3_mprintf(DB_QUERY_DELETE_USER, uid);
    
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

    if (pszError) {
        LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

DWORD
LsaLPDbRemoveGroupMembership_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    gid_t  gid
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR     pszError = NULL;
    PSTR     pszQuery = NULL;
    
    pszQuery = sqlite3_mprintf(DB_QUERY_REMOVE_USER_FROM_GROUP,
                               uid,
                               gid);
    
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

    if (!IsNullOrEmptyString(pszError)) {
       LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

DWORD
LsaLPDbAddGroupMembership_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    gid_t  gid
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR     pszError = NULL;
    PSTR     pszQuery = NULL;
    
    pszQuery = sqlite3_mprintf(DB_QUERY_ADD_GROUP_MEMBERSHIP,
                               uid,
                               gid);
    
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

    if (!IsNullOrEmptyString(pszError)) {
       LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

DWORD
LsaLPDbGetUserInfoFlags_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    PDWORD pdwUserInfoFlags
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR* ppszResult = NULL;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    int   nRows = 0;
    int   nCols = 0;
    DWORD dwUserInfoFlags = 0;
    
    pszQuery = sqlite3_mprintf(DB_QUERY_GET_USER_INFO_FLAGS, uid);
    
    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError
                                );
    BAIL_ON_LSA_ERROR(dwError);
        
    if (!nRows) {
       dwError = LSA_ERROR_NO_SUCH_USER;
    } else if ((nRows > 1) || (nCols != 1)) {
       dwError = LSA_ERROR_DATA_ERROR;
       BAIL_ON_LSA_ERROR(dwError);
    } else if (nRows == 1){
       dwUserInfoFlags = (IsNullOrEmptyString(ppszResult[1]) ? 0 : atoi(ppszResult[1]));
    }
    
    *pdwUserInfoFlags = dwUserInfoFlags;
        
cleanup:

    if (pszQuery) {
       sqlite3_free(pszQuery);
    }
        
    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    return dwError;
    
error:

    *pdwUserInfoFlags = 0;
    
    goto cleanup;
}

DWORD
LsaLPDbSetUserInfoFlags_Unsafe(
    HANDLE hDb,
    uid_t  uid,
    DWORD  dwUserInfoFlags
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR     pszError = NULL;
    PSTR     pszQuery = NULL;
    
    pszQuery = sqlite3_mprintf(DB_QUERY_UPDATE_USER_INFO_FLAGS,
                               dwUserInfoFlags,
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

    if (!IsNullOrEmptyString(pszError)) {
       LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

DWORD
LsaLPDbGetUserCount(
    HANDLE hDb,
    PINT pUserCount)
{
    DWORD dwError = 0;
    INT nUserCount = 0;    
    int nCols = 0;    
    PSTR* ppszResult = NULL;
    PSTR pszError = NULL;    
    sqlite3* pDbHandle = (sqlite3*)hDb;    
    
    dwError = sqlite3_get_table(pDbHandle,
                                DB_QUERY_COUNT_EXISTING_USERS,
                                &ppszResult,
                                &nUserCount,
                                &nCols,
                                &pszError);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (nCols != 1) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *pUserCount = nUserCount;
    
cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    
    return dwError;
    
error:

    *pUserCount = 0;

    if (pszError) {
       sqlite3_free(pszError);
    } 
    
    goto cleanup;
}

DWORD
LsaLPDbGetGroupCount(
    HANDLE hDb,
    PINT pGroupCount)
{
    DWORD dwError = 0;
    INT nGroupCount = 0;    
    int nCols = 0;    
    PSTR* ppszResult = NULL;
    PSTR pszError = NULL;    
    sqlite3* pDbHandle = (sqlite3*)hDb;    
    
    dwError = sqlite3_get_table(pDbHandle,
                                DB_QUERY_COUNT_EXISTING_GROUPS,
                                &ppszResult,
                                &nGroupCount,
                                &nCols,
                                &pszError);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (nCols != 1) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    
    *pGroupCount = nGroupCount;
    
cleanup:
    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }
    
    return dwError;
    
error:

    *pGroupCount = 0;

    if (pszError) {
       sqlite3_free(pszError);
    } 
    
    goto cleanup;
}
