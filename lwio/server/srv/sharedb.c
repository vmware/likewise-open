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
 *        sharedb.c
 *
 * Abstract:
 *
 *        Likewise Server Message Block (LSMB)
 *
 *        Server sub-system
 *
 *        Server share database interface
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static PSMB_SRV_SHARE_DB_CONTEXT gpShareDBContext = NULL;

#define SMB_SRV_LOCK_SHARE_DB_READ(bInLock) \
    if (!bInLock) { \
       assert(gpShareDBContext != NULL); \
       int thr_err = pthread_rwlock_rdlock(gpShareDBContext->pDbLock); \
       if (thr_err) { \
           SMB_LOG_ERROR("Failed to lock rw-mutex for read. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define SMB_SRV_LOCK_SHARE_DB_WRITE(bInLock) \
    if (!bInLock) { \
       assert(gpShareDBContext != NULL); \
       int thr_err = pthread_rwlock_wrlock(gpShareDBContext->pDbLock); \
       if (thr_err) { \
           SMB_LOG_ERROR("Failed to lock rw-mutex for write. Aborting program"); \
           abort(); \
       } \
       bInLock = TRUE; \
    }

#define SMB_SRV_UNLOCK_SHARE_DB(bInLock) \
    if (bInLock) { \
       assert(gpShareDBContext != NULL); \
       int thr_err = pthread_rwlock_unlock(gpShareDBContext->pDbLock); \
       if (thr_err) { \
           SMB_LOG_ERROR("Failed to unlock rw-mutex. Aborting program"); \
           abort(); \
       } \
       bInLock = FALSE; \
    }

static
DWORD
SrvShareDbCreate(
    VOID
    );

static
DWORD
SrvShareDbWriteToShareInfo(
    PSTR* ppszResult,
    int nRows,
    int nCols,
    DWORD nHeaderColsToSkip,
    PSHARE_INFO** pppShareInfoList,
    PDWORD pdwNumSharesFound
    );

DWORD
SrvShareDbInit(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext
    )
{
    pthread_rwlock_init(&pShareDBContext->dbLock, NULL);
    pShareDBContext->pDbLock = &pShareDBContext->dbLock;

    gpShareDBContext = pShareDBContext;

    return SrvShareDbCreate();
}

static
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

    dwError = SMBCheckFileExists(LWIO_SRV_SHARE_DB, &bExists);
    BAIL_ON_SMB_ERROR(dwError);

    // TODO: Implement an upgrade scenario
    if (bExists)
    {
       goto cleanup;
    }

    dwError = SMBCheckDirectoryExists(LWIO_SRV_DB_DIR, &bExists);
    BAIL_ON_SMB_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base folder */
        dwError = SMBCreateDirectory(LWIO_SRV_DB_DIR, cacheDirMode);
        BAIL_ON_SMB_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = SMBChangeOwnerAndPermissions(LWIO_SRV_DB_DIR, 0, 0, S_IRWXU);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBSrvDbOpen(&hDb);
    BAIL_ON_SMB_ERROR(dwError);

    pDbHandle = (sqlite3*)hDb;

    dwError = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_SHARES_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SMB_ERROR(dwError);

    dwError = SMBChangeOwnerAndPermissions(
                    LWIO_SRV_SHARE_DB,
                    0,
                    0,
                    S_IRWXU);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (hDb != (HANDLE)NULL)
    {
       SMBSrvDbClose(hDb);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError))
    {
       LSA_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
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

    dwError = sqlite3_open(LWIO_SRV_SHARE_DB, &pDbHandle);
    BAIL_ON_SMB_ERROR(dwError);

    *phDb = (HANDLE)pDbHandle;

cleanup:

    return dwError;

error:

    *(phDb) = (HANDLE)NULL;

    if (pDbHandle)
    {
        sqlite3_close(pDbHandle);
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
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_STRING(pszShareName);
    BAIL_ON_INVALID_STRING(pszPath);

    SMB_SRV_LOCK_SHARE_DB_WRITE(bInLock);

    pszQuery = sqlite3_mprintf(DB_QUERY_INSERT_SHARE,
                               pszShareName,
                               pszPath,
                               pszComment);

    dwError = sqlite3_exec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pszQuery)
    {
       sqlite3_free(pszQuery);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    SMB_SRV_UNLOCK_SHARE_DB(bInLock);

    return dwError;

error:

    if (!IsNullOrEmptyString(pszError))
    {
       SMB_LOG_ERROR("%s", pszError);
    }

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
    PSTR     pszQuery = NULL;
    PSTR     pszError = NULL;
    PSTR*    ppszResult = NULL;
    PSHARE_INFO* ppShareInfoList = NULL;
    BOOLEAN  bInLock = FALSE;
    int      nRows = 0;
    int      nCols = 0;
    int      nExpectedCols = 4;
    DWORD    dwNumSharesFound = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;

    BAIL_ON_INVALID_STRING(pszShareName);

    SMB_SRV_LOCK_SHARE_DB_READ(bInLock);

    pszQuery = sqlite3_mprintf(DB_QUERY_LOOKUP_SHARE_BY_NAME, pszShareName);

    dwError = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError);
    BAIL_ON_SMB_ERROR(dwError);

    if (!nRows)
    {
       dwError = SMB_ERROR_NO_SUCH_SHARE;
       BAIL_ON_SMB_ERROR(dwError);
    }

    if ((nCols != nExpectedCols) || (nRows > 1))
    {
       dwError = SMB_ERROR_DATA_ERROR;
       BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SrvShareDbWriteToShareInfo(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppShareInfoList,
                    &dwNumSharesFound);
    BAIL_ON_SMB_ERROR(dwError);

    *ppShareInfo = *ppShareInfoList;
    *ppShareInfoList = NULL;
    *pdwNumSharesFound = dwNumSharesFound;

cleanup:

    if (pszQuery)
    {
        sqlite3_free(pszQuery);
    }

    if (ppszResult) {
       sqlite3_free_table(ppszResult);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    SMB_SRV_UNLOCK_SHARE_DB(bInLock);

    if (ppShareInfoList)
    {
        SrvFreeShareInfoList(
                dwShareInfoLevel,
                (PVOID*)ppShareInfoList,
                dwNumSharesFound);
    }

    return dwError;

error:

    if (pszError)
    {
        SMB_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

static
DWORD
SrvShareDbWriteToShareInfo(
    PSTR* ppszResult,
    int nRows,
    int nCols,
    DWORD nHeaderColsToSkip,
    PSHARE_INFO** pppShareInfoList,
    PDWORD pdwNumSharesFound
    )
{
    DWORD dwError = 0;
    DWORD iCol = 0, iRow = 0;
    DWORD iVal = nHeaderColsToSkip;
    PSHARE_INFO* ppShareInfoList = NULL;
    PSHARE_INFO pShareInfo = NULL;
    DWORD dwNumSharesFound = nRows;
    DWORD dwShareInfoLevel = 0;

    dwError = SMBAllocateMemory(
                    sizeof(PSHARE_INFO) * dwNumSharesFound,
                    (PVOID*)&ppShareInfoList);
    BAIL_ON_SMB_ERROR(dwError);

    for (iRow = 0; iRow < nRows; iRow++)
    {
        dwError = SMBAllocateMemory(
                        sizeof(SHARE_INFO),
                        (PVOID*)&pShareInfo);
        BAIL_ON_SMB_ERROR(dwError);

        for (iCol = 0; iCol < nCols; iCol++)
        {
            switch(iCol)
            {
                case 0: /* ShareName */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       dwError = SMBAllocateString(
                                       ppszResult[iVal],
                                       &pShareInfo->pszShareName);
                       BAIL_ON_SMB_ERROR(dwError);
                    }
                    break;
                }
                case 1: /* PathName */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       dwError = SMBAllocateString(
                                       ppszResult[iVal],
                                       &pShareInfo->pszPath);
                       BAIL_ON_SMB_ERROR(dwError);
                    }
                    break;
                }
                case 2: /* Comment */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       dwError = SMBAllocateString(
                                       ppszResult[iVal],
                                       &pShareInfo->pszComment);
                       BAIL_ON_SMB_ERROR(dwError);
                    }
                    break;
                }
                case 3: /* Security Descriptor */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       dwError = SMBAllocateString(
                                       ppszResult[iVal],
                                       &pShareInfo->pszSID);
                       BAIL_ON_SMB_ERROR(dwError);
                    }
                    break;
                }
            }
            iVal++;
        }

        *(ppShareInfoList + iRow) = pShareInfo;
        pShareInfo = NULL;
    }

    *pppShareInfoList = ppShareInfoList;
    *pdwNumSharesFound = dwNumSharesFound;

cleanup:

    return dwError;

error:

    if (ppShareInfoList)
    {
        SrvFreeShareInfoList(
                dwShareInfoLevel,
                (PVOID*)ppShareInfoList,
                dwNumSharesFound);
    }

    if (pShareInfo)
    {
        SrvFreeShareInfo(dwShareInfoLevel, pShareInfo);
    }

    *pppShareInfoList = NULL;
    *pdwNumSharesFound = 0;

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
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_STRING(pszShareName);

    SMB_SRV_LOCK_SHARE_DB_READ(bInLock);

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_DELETE_SHARE,
                    pszShareName);

    dwError = sqlite3_exec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_SMB_ERROR(dwError);

cleanup:

    if (pszQuery)
    {
       sqlite3_free(pszQuery);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    SMB_SRV_UNLOCK_SHARE_DB(bInLock);;

    return dwError;

error:

    if (pszError)
    {
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
    BOOLEAN bInLock = FALSE;

    SMB_SRV_LOCK_SHARE_DB_READ(bInLock);

    dwError = sqlite3_get_table(pDbHandle,
                                DB_QUERY_COUNT_EXISTING_SHARES,
                                &ppszResult,
                                &nShareCount,
                                &nCols,
                                &pszError);
    BAIL_ON_SMB_ERROR(dwError);

    if (nCols != 1) {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_SMB_ERROR(dwError);
    }

    *pShareCount = nShareCount;

cleanup:

    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }

    if (pszError)
    {
       sqlite3_free(pszError);
    }

    SMB_SRV_UNLOCK_SHARE_DB(bInLock);

    return dwError;

error:

    *pShareCount = 0;

    if (pszError)
    {
        SMB_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

VOID
SrvShareDbClose(
    HANDLE hDb
    )
{
    sqlite3_close((sqlite3*)hDb);
}

VOID
SrvShareDbShutdown(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext
    )
{
    if (pShareDBContext->pDbLock)
    {
        pthread_rwlock_destroy(&pShareDBContext->dbLock);
        pShareDBContext->pShareDBLock = NULL;
    }
}
