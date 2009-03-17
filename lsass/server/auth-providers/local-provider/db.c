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

static
DWORD
LsaProviderLocal_DbCheckGroupMembershipRecord_Unsafe(
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

static
DWORD
LsaProviderLocal_DbGetNextAvailableUid_Unsafe(
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

static
DWORD
LsaProviderLocal_DbGetNextAvailableGid_Unsafe(
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
LsaProviderLocal_DbGetLMHash_Unsafe(
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
LsaProviderLocal_DbGetNTHash_Unsafe(
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

static
DWORD
LsaProviderLocal_DbWriteToGroupInfo_0_Unsafe(
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

static
DWORD
LsaProviderLocal_DbWriteToGroupInfo_1_Unsafe(
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

static
DWORD
LsaProviderLocal_DbWriteMembersToGroupInfo_1(
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





static
DWORD
LsaProviderLocal_DbWriteToUserInfo_0_Unsafe(
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

static
DWORD
LsaProviderLocal_DbWriteToUserInfo_1_Unsafe(
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

static
DWORD
LsaProviderLocal_DbWriteToUserInfo_2_Unsafe(
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
                        DWORD dwPasswdChangeInterval = LsaProviderLocal_GetPasswdChangeInterval();                        
                        DWORD dwPasswdChangeWarningTime = LsaProviderLocal_GetPasswdChangeWarningTime();

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

static
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

static
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

static
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

static
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

static
DWORD
LsaProviderLocal_DbFindUserByName_0_Unsafe(
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


static
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

static
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
LsaProviderLocal_DbCreate(
    void
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    sqlite3* pDbHandle = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;

    dwError = LsaCheckFileExists(LSASS_DB, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    // TODO: Implement an upgrade scenario
    if (bExists) {
       goto cleanup;
    }

    dwError = LsaCheckDirectoryExists(LSASS_DB_DIR, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists) {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
        
        /* Allow go+rx to the base LSASS folder */
        dwError = LsaCreateDirectory(LSASS_DB_DIR, cacheDirMode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LsaChangeOwnerAndPermissions(LSASS_DB_DIR, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaProviderLocal_DbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);
    
    pDbHandle = (sqlite3*)hDb;
        
    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_GROUPMEMBERSHIP_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_GROUPS_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_GROUPS_INSERT_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_GROUPS_DELETE_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_USERS_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_USERS_INSERT_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);
        
    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_USERS_DELETE_TRIGGER,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangeOwnerAndPermissions(LSASS_DB, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
       LsaProviderLocal_DbClose(hDb);
    }

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError)) {
       LSA_LOG_ERROR("%s", pszError);
    }

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

DWORD
LsaProviderLocal_DbEnableUser(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwUserInfoFlags  &= (DWORD)~LW_ACCOUNT_DISABLED;
    
    dwError = LsaProviderLocal_DbSetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbDisableUser(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwUserInfoFlags  |= (DWORD)LW_ACCOUNT_DISABLED;
    
    dwError = LsaProviderLocal_DbSetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbIsUserEnabled(
    HANDLE hDb,
    uid_t  uid,
    PBOOLEAN pbEnabled
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_READER_LOCK;
    
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbUnlockUser(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwUserInfoFlags  &= (DWORD)~LW_ACCOUNT_LOCKED_OUT;
    
    dwError = LsaProviderLocal_DbSetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbLockUser(
    HANDLE hDb,
    uid_t  uid
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwUserInfoFlags  |= (DWORD)LW_ACCOUNT_LOCKED_OUT;
    
    dwError = LsaProviderLocal_DbSetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbIsUserLocked(
    HANDLE   hDb,
    uid_t    uid,
    PBOOLEAN pbLocked
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
     
    ENTER_RW_READER_LOCK;
     
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbAllowUserToChangePassword(
    HANDLE  hDb,
    uid_t   uid,
    BOOLEAN bAllow
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (bAllow) {
        dwUserInfoFlags &= (DWORD)~LW_CANNOT_CHANGE_PASSWORD;
    } else {
        dwUserInfoFlags |= (DWORD)LW_CANNOT_CHANGE_PASSWORD;
    }
    
    dwError = LsaProviderLocal_DbSetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbCanUserChangePassword(
    HANDLE  hDb,
    uid_t   uid,
    PBOOLEAN pbCanChangePassword
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
     
    ENTER_RW_READER_LOCK;
     
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbSetPasswordExpires(
    HANDLE  hDb,
    uid_t   uid,
    BOOLEAN bPasswordExpires
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
    
    ENTER_RW_WRITER_LOCK;
    
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
                    hDb,
                    uid,
                    &dwUserInfoFlags);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (bPasswordExpires) {
        dwUserInfoFlags &= (DWORD)~LW_PASSWORD_CANNOT_EXPIRE;
    } else {
        dwUserInfoFlags |= (DWORD)LW_PASSWORD_CANNOT_EXPIRE;
    }
    
    dwError = LsaProviderLocal_DbSetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbCheckPasswordExpires(
    HANDLE   hDb,
    uid_t    uid,
    PBOOLEAN pbPasswordExpires
    )
{
    DWORD dwError = 0;
    DWORD dwUserInfoFlags = 0;
      
    ENTER_RW_READER_LOCK;
      
    dwError = LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbSetChangePasswordOnNextLogon(
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
LsaProviderLocal_DbSetAccountExpiryDate(
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
LsaProviderLocal_DbRemoveFromGroups(
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
    
    dwError = LsaProviderLocal_DbFindUserById(
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
        dwError = LsaProviderLocal_DbFindGroupByName_0_Unsafe(
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
           dwError = LsaProviderLocal_DbRemoveGroupMembership_Unsafe(
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
       LsaDLinkedListForEach(pGIDList, &LsaProviderLocal_DbFreeGIDInList, NULL);
       LsaDLinkedListFree(pGIDList);
    }

    return dwError;
    
error:

    goto cleanup;
}
    
DWORD
LsaProviderLocal_DbAddToGroups(
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
    
    dwError = LsaProviderLocal_DbFindUserById(
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
        dwError = LsaProviderLocal_DbFindGroupByName_0_Unsafe(
                        hDb,
                        NULL,
                        pszToken,
                        &pGroupInfo);
        BAIL_ON_LSA_ERROR(dwError);
        
        if (((PLSA_USER_INFO_0)pUserInfo)->gid == ((PLSA_GROUP_INFO_0)pGroupInfo)->gid) {
           bMembershipExists = TRUE;
        } else {
           dwError = LsaProviderLocal_DbCheckGroupMembershipRecord_Unsafe(
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
           dwError = LsaProviderLocal_DbAddGroupMembership_Unsafe(
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
       LsaDLinkedListForEach(pGIDList, &LsaProviderLocal_DbFreeGIDInList, NULL);
       LsaDLinkedListFree(pGIDList);
    }

    return dwError;
    
error:

    goto cleanup;
}

VOID
LsaProviderLocal_DbFreeGIDInList(
    PVOID pGID,
    PVOID pUserData
    )
{
    LSA_SAFE_FREE_MEMORY(pGID);
}

DWORD
LsaProviderLocal_DbDeleteUser(
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
LsaProviderLocal_DbAddGroup(
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
       dwError = LsaProviderLocal_DbGetNextAvailableGid_Unsafe(
                           hDb,
                           &pGroup->gid
                           );
       BAIL_ON_LSA_ERROR(dwError);
    } else {
       dwError = LsaProviderLocal_DbFindGroupById_0_Unsafe(
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

    dwError = LsaProviderLocal_DbFindGroupByName_0_Unsafe(
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

static
DWORD
LsaProviderLocal_DetermineNumberUsersWithPrimaryGroup_Unsafe(
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

static
DWORD
LsaProviderLocal_DetermineNumberGroupMembers(
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

static
DWORD
LsaProviderLocal_DetermineGroupCanBeDeleted_Unsafe(
    HANDLE hDb,
    gid_t  gid,
    PBOOLEAN pbCanBeDeleted
    )
{
    DWORD dwError = 0;
    BOOLEAN bResult = FALSE;
    DWORD dwNUsers_w_primaryGroup = 0;
    DWORD dwNGroupMembers = 0;
    
    dwError = LsaProviderLocal_DetermineNumberUsersWithPrimaryGroup_Unsafe(
                    hDb,
                    gid,
                    &dwNUsers_w_primaryGroup);
    BAIL_ON_LSA_ERROR(dwError);
    
    if (dwNUsers_w_primaryGroup) {
        goto cleanup;
    }
    
    dwError = LsaProviderLocal_DetermineNumberGroupMembers(
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
LsaProviderLocal_DbDeleteGroup(
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
    
    dwError = LsaProviderLocal_DetermineGroupCanBeDeleted_Unsafe(
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

DWORD
LsaProviderLocal_DbRemoveGroupMembership_Unsafe(
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
LsaProviderLocal_DbAddGroupMembership_Unsafe(
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
LsaProviderLocal_DbGetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbSetUserInfoFlags_Unsafe(
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
LsaProviderLocal_DbGetUserCount(
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
LsaProviderLocal_DbGetGroupCount(
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
