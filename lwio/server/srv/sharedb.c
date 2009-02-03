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
#include "sharedbquery.h"

static
NTSTATUS
SrvShareDbCreate(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext
    );

static
VOID
SrvShareDbFreeInfo(
    PSHARE_DB_INFO pShareInfo
    );

static
NTSTATUS
SrvShareDbWriteToShareInfo(
    PSTR*          ppszResult,
    int            nRows,
    int            nCols,
    ULONG          nHeaderColsToSkip,
    PSHARE_DB_INFO** pppShareInfoList,
    PULONG         pulNumSharesFound
    );

NTSTATUS
SrvShareDbInit(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext
    )
{
    pthread_rwlock_init(&pShareDBContext->mutex, NULL);
    pShareDBContext->pMutex = &pShareDBContext->mutex;

    return SrvShareDbCreate(pShareDBContext);
}

static
NTSTATUS
SrvShareDbCreate(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext
    )
{
    NTSTATUS ntStatus = 0;
    HANDLE hDb = (HANDLE)NULL;
    sqlite3* pDbHandle = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;

    ntStatus = SMBCheckFileExists(LWIO_SRV_SHARE_DB, &bExists);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: Implement an upgrade scenario
    if (bExists)
    {
       goto cleanup;
    }

    ntStatus = SMBCheckDirectoryExists(LWIO_SRV_DB_DIR, &bExists);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base folder */
        ntStatus = SMBCreateDirectory(LWIO_SRV_DB_DIR, cacheDirMode);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* restrict access to u+rwx to the db folder */
    ntStatus = SMBChangeOwnerAndPermissions(LWIO_SRV_DB_DIR, 0, 0, S_IRWXU);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbOpen(
                    pShareDBContext,
                    &hDb);
    BAIL_ON_NT_STATUS(ntStatus);

    pDbHandle = (sqlite3*)hDb;

    ntStatus = sqlite3_exec(pDbHandle,
                           DB_QUERY_CREATE_SHARES_TABLE,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBChangeOwnerAndPermissions(
                    LWIO_SRV_SHARE_DB,
                    0,
                    0,
                    S_IRWXU);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbAdd(
                    pShareDBContext,
                    hDb,
                    "IPC$",
                    "\\npvfs",
                    "Root of Named Pipe Virtual File System",
                    NULL,
                    "IPC");
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbAdd(
                    pShareDBContext,
                    hDb,
                    "C$",
                    "\\pvfs\\root",
                    "Root of Posix Virtual File System",
                    NULL,
                    "A:");
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (hDb != (HANDLE)NULL)
    {
       SrvShareDbClose(pShareDBContext, hDb);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    return ntStatus;

error:

    if (!IsNullOrEmptyString(pszError))
    {
       SMB_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

//
// TODO: Implement a DB Handle Pool
//
NTSTATUS
SrvShareDbOpen(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext,
    PHANDLE phDb
    )
{
    NTSTATUS ntStatus = 0;
    sqlite3* pDbHandle = NULL;

    ntStatus = sqlite3_open(
                    LWIO_SRV_SHARE_DB,
                    &pDbHandle);
    BAIL_ON_NT_STATUS(ntStatus);

    *phDb = (HANDLE)pDbHandle;

cleanup:

    return ntStatus;

error:

    *(phDb) = (HANDLE)NULL;

    if (pDbHandle)
    {
        sqlite3_close(pDbHandle);
    }

    goto cleanup;
}

NTSTATUS
SrvShareDbAdd(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE hDb,
    PCSTR  pszShareName,
    PCSTR  pszPath,
    PCSTR  pszComment,
    PCSTR  pszSid,
    PCSTR  pszService
    )
{
    NTSTATUS ntStatus = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR pszError = NULL;
    PSTR pszQuery = NULL;
    BOOLEAN bInLock = FALSE;
    SHARE_SERVICE service = SHARE_SERVICE_UNKNOWN;

    if (IsNullOrEmptyString(pszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
    }
    if (IsNullOrEmptyString(pszPath))
    {
        ntStatus = STATUS_INVALID_PARAMETER_4;
    }

    ntStatus = SrvShareGetServiceId(pszService, &service);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_INVALID_PARAMETER_6;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareDBContext->mutex);

    pszQuery = sqlite3_mprintf(DB_QUERY_INSERT_SHARE,
                               pszShareName,
                               pszPath,
                               pszComment,
                               pszSid,
                               pszService);

    ntStatus = sqlite3_exec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pszQuery)
    {
       sqlite3_free(pszQuery);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    return ntStatus;

error:

    if (!IsNullOrEmptyString(pszError))
    {
       SMB_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

NTSTATUS
SrvShareDbEnum(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE           hDb,
    ULONG            ulOffset,
    ULONG            ulLimit,
    PSHARE_DB_INFO** pppShareInfoList,
    PULONG           pulNumSharesFound
    )
{
    NTSTATUS ntStatus = 0;
    PSTR     pszQuery = NULL;
    PSTR     pszError = NULL;
    PSTR*    ppszResult = NULL;
    PSHARE_DB_INFO* ppShareInfoList = NULL;
    BOOLEAN  bInLock = FALSE;
    int      nRows = 0;
    int      nCols = 0;
    int      nExpectedCols = 5;
    ULONG    ulNumSharesFound = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pShareDBContext->mutex);

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_FIND_SHARES_LIMIT,
                    ulLimit,
                    ulOffset);

    ntStatus = sqlite3_get_table(
                    pDbHandle,
                    pszQuery,
                    &ppszResult,
                    &nRows,
                    &nCols,
                    &pszError);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!nRows)
    {
       ntStatus = STATUS_NO_MORE_ENTRIES;
       BAIL_ON_NT_STATUS(ntStatus);
    }

    if ((nCols != nExpectedCols) || (nRows > ulLimit))
    {
       ntStatus = STATUS_DATA_ERROR;
       BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvShareDbWriteToShareInfo(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppShareInfoList,
                    &ulNumSharesFound);
    BAIL_ON_NT_STATUS(ntStatus);

    *pppShareInfoList =  ppShareInfoList;
    *pulNumSharesFound = ulNumSharesFound;

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

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    return ntStatus;

error:

    if (pszError)
    {
        SMB_LOG_ERROR("%s", pszError);
    }

    if (ppShareInfoList)
    {
        SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    goto cleanup;
}

NTSTATUS
SrvShareDbFindByName(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE          hDb,
    PCSTR           pszShareName,
    PSHARE_DB_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    PSTR     pszQuery = NULL;
    PSTR     pszError = NULL;
    PSTR*    ppszResult = NULL;
    PSHARE_DB_INFO* ppShareInfoList = NULL;
    BOOLEAN  bInLock = FALSE;
    int      nRows = 0;
    int      nCols = 0;
    int      nExpectedCols = 5;
    ULONG    ulNumSharesFound = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;

    if (IsNullOrEmptyString(pszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pShareDBContext->mutex);

    pszQuery = sqlite3_mprintf(DB_QUERY_LOOKUP_SHARE_BY_NAME, pszShareName);

    ntStatus = sqlite3_get_table(pDbHandle,
                                pszQuery,
                                &ppszResult,
                                &nRows,
                                &nCols,
                                &pszError);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!nRows)
    {
       ntStatus = STATUS_NOT_FOUND;
       BAIL_ON_NT_STATUS(ntStatus);
    }

    if ((nCols != nExpectedCols) || (nRows > 1))
    {
       ntStatus = STATUS_DATA_ERROR;
       BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvShareDbWriteToShareInfo(
                    ppszResult,
                    nRows,
                    nCols,
                    nExpectedCols,
                    &ppShareInfoList,
                    &ulNumSharesFound);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppShareInfo = *ppShareInfoList;
    *ppShareInfoList = NULL;

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

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    if (ppShareInfoList)
    {
        SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    return ntStatus;

error:

    if (pszError)
    {
        SMB_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

static
NTSTATUS
SrvShareDbWriteToShareInfo(
    PSTR*            ppszResult,
    int              nRows,
    int              nCols,
    ULONG            nHeaderColsToSkip,
    PSHARE_DB_INFO** pppShareInfoList,
    PULONG           pulNumSharesFound
    )
{
    NTSTATUS ntStatus = 0;
    ULONG iCol = 0, iRow = 0;
    ULONG iVal = nHeaderColsToSkip;
    PSHARE_DB_INFO* ppShareInfoList = NULL;
    PSHARE_DB_INFO pShareInfo = NULL;
    ULONG ulNumSharesFound = nRows;

    ntStatus = SMBAllocateMemory(
                    sizeof(PSHARE_DB_INFO) * nRows,
                    (PVOID*)&ppShareInfoList);
    BAIL_ON_NT_STATUS(ntStatus);

    for (iRow = 0; iRow < nRows; iRow++)
    {
        ntStatus = SMBAllocateMemory(
                        sizeof(SHARE_DB_INFO),
                        (PVOID*)&pShareInfo);
        BAIL_ON_NT_STATUS(ntStatus);

        pShareInfo->refcount = 1;

        pthread_rwlock_init(&pShareInfo->mutex, NULL);
        pShareInfo->pMutex = &pShareInfo->mutex;

        for (iCol = 0; iCol < nCols; iCol++)
        {
            switch(iCol)
            {
                case 0: /* ShareName */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       ntStatus = SMBMbsToWc16s(
                                       ppszResult[iVal],
                                       &pShareInfo->pwszName);
                       BAIL_ON_NT_STATUS(ntStatus);
                    }
                    break;
                }
                case 1: /* PathName */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       ntStatus = SMBMbsToWc16s(
                                       ppszResult[iVal],
                                       &pShareInfo->pwszPath);
                       BAIL_ON_NT_STATUS(ntStatus);
                    }
                    break;
                }
                case 2: /* Comment */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       ntStatus = SMBMbsToWc16s(
                                       ppszResult[iVal],
                                       &pShareInfo->pwszComment);
                       BAIL_ON_NT_STATUS(ntStatus);
                    }
                    break;
                }
                case 3: /* Security Descriptor */
                {
                    if (!IsNullOrEmptyString(ppszResult[iVal]))
                    {
                       ntStatus = SMBMbsToWc16s(
                                       ppszResult[iVal],
                                       &pShareInfo->pwszSID);
                       BAIL_ON_NT_STATUS(ntStatus);
                    }
                    break;
                }
                case 4: /* service */
                {
                    ntStatus = SrvShareGetServiceId(
                                    ppszResult[iVal],
                                    &pShareInfo->service);
                    BAIL_ON_NT_STATUS(ntStatus);

                    break;
                }
            }
            iVal++;
        }

        *(ppShareInfoList + iRow) = pShareInfo;
        pShareInfo = NULL;
    }

    *pppShareInfoList = ppShareInfoList;
    *pulNumSharesFound = ulNumSharesFound;

cleanup:

    if (pShareInfo)
    {
        SrvShareDbReleaseInfo(pShareInfo);
    }

    return ntStatus;

error:

    *pppShareInfoList = NULL;
    *pulNumSharesFound = 0;

    if (ppShareInfoList)
    {
        SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    goto cleanup;
}

NTSTATUS
SrvShareDbDelete(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE hDb,
    PCSTR  pszShareName
    )
{
    NTSTATUS ntStatus = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    PSTR  pszQuery = NULL;
    PSTR  pszError = NULL;
    BOOLEAN bInLock = FALSE;

    if (IsNullOrEmptyString(pszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareDBContext->mutex);

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_DELETE_SHARE,
                    pszShareName);

    ntStatus = sqlite3_exec(pDbHandle,
                           pszQuery,
                           NULL,
                           NULL,
                           &pszError);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pszQuery)
    {
       sqlite3_free(pszQuery);
    }

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    return ntStatus;

error:

    if (pszError)
    {
        SMB_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

NTSTATUS
SrvShareDbGetCount(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext,
    HANDLE  hDb,
    PULONG  pulNumShares
    )
{
    NTSTATUS ntStatus = 0;
    INT   nShareCount = 0;
    int   nCols = 0;
    PSTR* ppszResult = NULL;
    PSTR pszError = NULL;
    sqlite3* pDbHandle = (sqlite3*)hDb;
    BOOLEAN bInLock = FALSE;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pShareDBContext->mutex);

    ntStatus = sqlite3_get_table(pDbHandle,
                                DB_QUERY_COUNT_EXISTING_SHARES,
                                &ppszResult,
                                &nShareCount,
                                &nCols,
                                &pszError);
    BAIL_ON_NT_STATUS(ntStatus);

    if (nCols != 1)
    {
        ntStatus = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pulNumShares = nShareCount;

cleanup:

    if (ppszResult) {
        sqlite3_free_table(ppszResult);
    }

    if (pszError)
    {
       sqlite3_free(pszError);
    }

    SMB_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    return ntStatus;

error:

    *pulNumShares = 0;

    if (pszError)
    {
        SMB_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

VOID
SrvShareDbFreeInfoList(
    PSHARE_DB_INFO* ppShareInfoList,
    ULONG           ulNumShares
    )
{
    ULONG iShare = 0;

    for (; iShare < ulNumShares; iShare++)
    {
        PSHARE_DB_INFO pShareInfo = *(ppShareInfoList + iShare);

        if (pShareInfo)
        {
            SrvShareDbReleaseInfo(pShareInfo);
        }
    }

    SMBFreeMemory(ppShareInfoList);
}

VOID
SrvShareDbReleaseInfo(
    PSHARE_DB_INFO pShareInfo
    )
{
    if (InterlockedDecrement(&pShareInfo->refcount) == 0)
    {
        SrvShareDbFreeInfo(pShareInfo);
    }
}

static
VOID
SrvShareDbFreeInfo(
    PSHARE_DB_INFO pShareInfo
    )
{
    if (pShareInfo->pMutex)
    {
        pthread_rwlock_destroy(&pShareInfo->mutex);
    }

    SMB_SAFE_FREE_MEMORY(pShareInfo->pwszName);
    SMB_SAFE_FREE_MEMORY(pShareInfo->pwszPath);
    SMB_SAFE_FREE_MEMORY(pShareInfo->pwszSID);
    SMB_SAFE_FREE_MEMORY(pShareInfo->pwszComment);

    SMBFreeMemory(pShareInfo);
}

VOID
SrvShareDbClose(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext,
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
    if (pShareDBContext->pMutex)
    {
        pthread_rwlock_destroy(&pShareDBContext->mutex);
        pShareDBContext->pMutex = NULL;
    }
}
