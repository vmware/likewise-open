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

VOID
SrvDbInitGlobals(
    VOID
    )
{
    pthread_rwlock_init(&g_dbLock, NULL);
}

//
// TODO: Implement a DB Handle Pool
//
DWORD
SrvShareDbOpen(
    PHANDLE phDb
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = NULL;

    dwError = sqlite3_open(LSMB_SRV_SHARE_DB, &pDbHandle);
    BAIL_ON_LSA_ERROR(dwError);

    *phDb = (HANDLE)pDbHandle;

cleanup:

    return dwError;

error:

    *(phDb) = (HANDLE)NULL;

    if (pDbHandle) {
        sqlite3_close(pDbHandle);
    }

    goto cleanup;
}

VOID
SrvShareDbClose(
    HANDLE hDb
    )
{
    sqlite3* pDbHandle = (sqlite3*)hDb;
    if (pDbHandle) {
       sqlite3_close(pDbHandle);
    }
}

static
DWORD
SrvShareDbWriteToShareInfo(
        PSTR* ppszResult,
        int nRows,
        int nCols,
        DWORD nHeaderColsToSkip,
        PSHARE_INFO* ppShareInfo,
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
                        sizeof(SHARE_INFO),
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
SrvShareDbFindByName(
    HANDLE hDb,
    PCSTR  pszShareName,
    PSHARE_INFO* ppShareInfo
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
SrvShareDbCreate(
    VOID
    )
{
    DWORD dwError = 0;
    HANDLE hDb = (HANDLE)NULL;
    sqlite3* pDbHandle = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;

    dwError = LsaCheckFileExists(LSMB_SRV_SHARE_DB, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    // TODO: Implement an upgrade scenario
    if (bExists) {
       goto cleanup;
    }

    dwError = SMBCheckDirectoryExists(LSMB_SRV_DB_DIR, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists) {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base LSASS folder */
        dwError = SMBCreateDirectory(LSMB_SRV_DB_DIR, cacheDirMode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = SMBChangeOwnerAndPermissions(LSMB_SRV_DB_DIR, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SMBSrvDbOpen(&hDb);
    BAIL_ON_LSA_ERROR(dwError);

    pDbHandle = (sqlite3*)hDb;

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_SHARES_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = SMBChangeOwnerAndPermissions(LSMB_SRV_SHARE_DB, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL) {
       SMBSrvDbClose(hDb);
    }

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError)) {
       LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

DWORD
SrvShareDbAdd(
    HANDLE hDb,
    PCSTR  pszShareName,
    PCSTR  pszPath,
    PCSTR  pszComment
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR pszError = NULL;
    PSTR pszQuery = NULL;
    BOOLEAN bReleaseLock = FALSE;

    ENTER_RW_WRITER_LOCK;
    bReleaseLock = TRUE;

    pszQuery = sqlite3_mprintf(DB_QUERY_INSERT_SHARE,
                               pszShareName,
                               pszPath,
                               pszComment);

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

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError)) {
       SMB_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

DWORD
SrvShareDbDelete(
    HANDLE hDb,
    PCSTR  pszShareName
    )
{
    DWORD dwError = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;

    ENTER_RW_WRITER_LOCK;

    pszQuery = sqlite3_mprintf(DB_QUERY_DELETE_SHARE, pszShareName);

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
SrvShareDbGetCount(
    HANDLE hDb,
    PINT   pShareCount
    )
{
    DWORD dwError = 0;
    INT   nShareCount = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;
    PSTR pszError = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;

    dwError = sqlite3_get_table(pDbHandle,
                                DB_QUERY_COUNT_EXISTING_SHARES,
                                &ppszResult,
                                &nShareCount,
                                &nCols,
                                &pszError);
    BAIL_ON_LSA_ERROR(dwError);

    if (nCols != 1) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pShareCount = nShareCount;

cleanup:

    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }

    return dwError;

error:

    *pShareCount = 0;

    if (pszError) {
       sqlite3_free(pszError);
    }

    goto cleanup;
}

