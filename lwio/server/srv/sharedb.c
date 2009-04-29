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
NTSTATUS
SrvShareMapSpecificToWindowsPath(
    PWSTR  pwszFSPrefix,
    PWSTR  pwszRootPath,
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
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
    NTSTATUS ntStatus = 0;

    pthread_rwlock_init(&pShareDBContext->mutex, NULL);
    pShareDBContext->pMutex = &pShareDBContext->mutex;

    ntStatus = SMBMbsToWc16s(
                    LWIO_SRV_FILE_SYSTEM_PREFIX,
                    &pShareDBContext->pwszFileSystemPrefix);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBMbsToWc16s(
                    LWIO_SRV_FILE_SYSTEM_ROOT,
                    &pShareDBContext->pwszFileSystemRoot);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBMbsToWc16s(
                    LWIO_SRV_PIPE_SYSTEM_ROOT,
                    &pShareDBContext->pwszPipeSystemRoot);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbCreate(pShareDBContext);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
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
    PSTR pszFileSystemRoot = NULL;
    PSTR pszTmpFileSystemRoot = NULL;
    PSTR pszPipeSystemRoot = NULL;
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

    ntStatus = SMBWc16sToMbs(
                    pShareDBContext->pwszFileSystemRoot,
                    &pszTmpFileSystemRoot);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateStringPrintf(
                    &pszFileSystemRoot,
                    "%s%s%s",
                    pszTmpFileSystemRoot,
                    (((pszTmpFileSystemRoot[strlen(pszTmpFileSystemRoot)-1] == '/') ||
                      (pszTmpFileSystemRoot[strlen(pszTmpFileSystemRoot)-1] == '\\')) ? "" : "\\"),
                    LWIO_SRV_DEFAULT_SHARE_PATH);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBWc16sToMbs(
                    pShareDBContext->pwszPipeSystemRoot,
                    &pszPipeSystemRoot);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbAdd(
                    pShareDBContext,
                    hDb,
                    "IPC$",
                    pszPipeSystemRoot,
                    "Remote IPC",
                    NULL,
                    "IPC");
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbAdd(
                    pShareDBContext,
                    hDb,
                    "C$",
                    pszFileSystemRoot,
                    "Default Share",
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

    if (pszFileSystemRoot)
    {
        LwRtlMemoryFree(pszFileSystemRoot);
    }

    if (pszTmpFileSystemRoot)
    {
        LwRtlMemoryFree(pszTmpFileSystemRoot);
    }

    if (pszPipeSystemRoot)
    {
        LwRtlMemoryFree(pszPipeSystemRoot);
    }

    return ntStatus;

error:

    if (!IsNullOrEmptyString(pszError))
    {
       LWIO_LOG_ERROR("%s", pszError);
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
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareDBContext->mutex);

    ntStatus = SrvShareDbAdd_inlock(
                    pShareDBContext,
                    hDb,
                    pszShareName,
                    pszPath,
                    pszComment,
                    pszSid,
                    pszService);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvShareDbAdd_inlock(
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

    /* Protect against NULL comment strings */

    pszQuery = sqlite3_mprintf(DB_QUERY_INSERT_SHARE,
                               pszShareName,
                               pszPath,
                               pszComment ? pszComment : "",
                               pszSid,
                               pszService);

    if (pszQuery == NULL)
    {
        ntStatus = STATUS_NO_MEMORY;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    return ntStatus;

error:

    if (!IsNullOrEmptyString(pszError))
    {
       LWIO_LOG_ERROR("%s", pszError);
    }

    goto cleanup;
}

NTSTATUS
SrvShareMapFromWindowsPath(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext,
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
    )
{
    NTSTATUS  ntStatus = 0;
    wchar16_t wszBackslash[2] = {0, 0};
    wchar16_t wszFwdslash[2] = {0, 0};
    wchar16_t wszColon[2] = {0, 0};
    PWSTR     pwszPathReadCursor = pwszInputPath;
    PWSTR     pwszPathWriteCursor = NULL;
    PWSTR     pwszPath = NULL;
    size_t    sInputPathLen = 0;
    size_t    sFSPrefixLen = 3;
    size_t    sFSRootLen = 0;
    size_t    sRequiredLen = 0;

    if (!pwszInputPath || !*pwszInputPath)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    sInputPathLen = wc16slen(pwszInputPath);

    wcstowc16s(&wszBackslash[0], L"\\", 1);
    wcstowc16s(&wszFwdslash[0], L"/", 1);
    wcstowc16s(&wszColon[0], L":", 1);

    sFSRootLen = wc16slen(pShareDBContext->pwszFileSystemRoot);

    if ((sInputPathLen < sFSPrefixLen) ||
        ((pwszInputPath[1] != wszColon[0]) &&
         !(pwszInputPath[2] == wszBackslash[0] ||
           pwszInputPath[2] == wszFwdslash[0])))
    {
        ntStatus = STATUS_OBJECT_PATH_SYNTAX_BAD;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        sRequiredLen += sFSRootLen * sizeof(wchar16_t);
        pwszPathReadCursor += sFSPrefixLen;
    }

    if (!pwszPathReadCursor || !*pwszPathReadCursor)
    {
        ntStatus = STATUS_OBJECT_PATH_SYNTAX_BAD;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    sRequiredLen += sizeof(wchar16_t); // path delimiter

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) || (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    // The rest of the path
    sRequiredLen += wc16slen(pwszPathReadCursor) * sizeof(wchar16_t);
    sRequiredLen += sizeof(wchar16_t);

    ntStatus = LW_RTL_ALLOCATE(
                    &pwszPath,
                    wchar16_t,
                    sRequiredLen);
    BAIL_ON_NT_STATUS(ntStatus);

    pwszPathReadCursor = pwszInputPath;
    pwszPathWriteCursor = pwszPath;

    pwszPathReadCursor += sFSPrefixLen;
    memcpy((PBYTE)pwszPathWriteCursor, (PBYTE)pShareDBContext->pwszFileSystemRoot, sFSRootLen * sizeof(wchar16_t));
    pwszPathWriteCursor += sFSRootLen;
    *pwszPathWriteCursor++ = wszBackslash[0];

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) || (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    while (pwszPathReadCursor && *pwszPathReadCursor)
    {
        if (*pwszPathReadCursor == wszFwdslash[0])
        {
            *pwszPathWriteCursor++ = wszBackslash[0];
        }
        else
        {
            *pwszPathWriteCursor++ = *pwszPathReadCursor;
        }
        pwszPathReadCursor++;
    }

    *ppwszPath = pwszPath;

cleanup:

    return ntStatus;

error:

    *ppwszPath = NULL;

    if (pwszPath)
    {
        LwRtlMemoryFree(pwszPath);
    }

    goto cleanup;
}

NTSTATUS
SrvShareMapToWindowsPath(
    PSMB_SRV_SHARE_DB_CONTEXT pShareDBContext,
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
    )
{
    NTSTATUS ntStatus = 0;
    PWSTR pwszPath = NULL;

    ntStatus = SrvShareMapSpecificToWindowsPath(
                    pShareDBContext->pwszFileSystemPrefix,
                    pShareDBContext->pwszFileSystemRoot,
                    pwszInputPath,
                    &pwszPath);
    if (ntStatus == STATUS_OBJECT_PATH_SYNTAX_BAD)
    {
        ntStatus = SrvShareMapSpecificToWindowsPath(
                            pShareDBContext->pwszFileSystemPrefix,
                            pShareDBContext->pwszPipeSystemRoot,
                            pwszInputPath,
                            &pwszPath);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppwszPath = pwszPath;

cleanup:

    return ntStatus;

error:

    *ppwszPath = NULL;

    if (pwszPath)
    {
        LwRtlMemoryFree(pwszPath);
    }

    goto cleanup;
}

static
NTSTATUS
SrvShareMapSpecificToWindowsPath(
    PWSTR  pwszFSPrefix,
    PWSTR  pwszRootPath,
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
    )
{
    NTSTATUS  ntStatus = 0;
    wchar16_t wszBackslash[2] = {0, 0};
    wchar16_t wszFwdslash[2] = {0, 0};
    PWSTR     pwszPathReadCursor = pwszInputPath;
    PWSTR     pwszPathWriteCursor = NULL;
    PWSTR     pwszPath = NULL;
    size_t    sInputPathLen = 0;
    size_t    sFSPrefixLen = 0;
    size_t    sFSRootLen = 0;
    size_t    sRequiredLen = 0;

    if (!pwszInputPath || !*pwszInputPath)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    sInputPathLen = wc16slen(pwszInputPath);

    wcstowc16s(&wszBackslash[0], L"\\", 1);
    wcstowc16s(&wszFwdslash[0], L"/", 1);

    sFSPrefixLen = wc16slen(pwszFSPrefix);
    sFSRootLen = wc16slen(pwszRootPath);

    if ((sInputPathLen < sFSRootLen) ||
        memcmp((PBYTE)pwszPathReadCursor,
               (PBYTE)pwszRootPath,
               sFSRootLen * sizeof(wchar16_t)))
    {
        ntStatus = STATUS_OBJECT_PATH_SYNTAX_BAD;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        sRequiredLen += sFSPrefixLen * sizeof(wchar16_t);
        pwszPathReadCursor += sFSRootLen;
    }

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) || (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    // The rest of the path
    sRequiredLen += wc16slen(pwszPathReadCursor) * sizeof(wchar16_t);
    sRequiredLen += sizeof(wchar16_t);

    ntStatus = LW_RTL_ALLOCATE(
                    &pwszPath,
                    wchar16_t,
                    sRequiredLen);
    BAIL_ON_NT_STATUS(ntStatus);

    pwszPathReadCursor = pwszInputPath;
    pwszPathWriteCursor = pwszPath;

    pwszPathReadCursor += sFSRootLen;
    memcpy((PBYTE)pwszPathWriteCursor, (PBYTE)pwszFSPrefix, sFSPrefixLen * sizeof(wchar16_t));
    pwszPathWriteCursor += sFSPrefixLen;

    while (pwszPathReadCursor &&
           *pwszPathReadCursor &&
           ((*pwszPathReadCursor == wszBackslash[0]) || (*pwszPathReadCursor == wszFwdslash[0])))
    {
        pwszPathReadCursor++;
    }

    while (pwszPathReadCursor && *pwszPathReadCursor)
    {
        if (*pwszPathReadCursor == wszFwdslash[0])
        {
            *pwszPathWriteCursor++ = wszBackslash[0];
        }
        else
        {
            *pwszPathWriteCursor++ = *pwszPathReadCursor;
        }
        pwszPathReadCursor++;
    }

    *ppwszPath = pwszPath;

cleanup:

    return ntStatus;

error:

    *ppwszPath = NULL;

    if (pwszPath)
    {
        LwRtlMemoryFree(pwszPath);
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
    BOOLEAN  bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareDBContext->mutex);

    ntStatus = SrvShareDbEnum_inlock(
                      pShareDBContext,
                      hDb,
                      ulOffset,
                      ulLimit,
                      pppShareInfoList,
                      pulNumSharesFound
		      );

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    return ntStatus;
}


NTSTATUS
SrvShareDbEnum_inlock(
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
    int      nRows = 0;
    int      nCols = 0;
    int      nExpectedCols = 5;
    ULONG    ulNumSharesFound = 0;
    sqlite3* pDbHandle = (sqlite3*)hDb;

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_FIND_SHARES_LIMIT,
                    ulLimit,
                    ulOffset);

    if (pszQuery == NULL)
    {
        ntStatus = STATUS_NO_MEMORY;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    return ntStatus;

error:

    if (pszError)
    {
        LWIO_LOG_ERROR("%s", pszError);
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

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareDBContext->mutex);

    pszQuery = sqlite3_mprintf(DB_QUERY_LOOKUP_SHARE_BY_NAME, pszShareName);

    if (pszQuery == NULL)
    {
        ntStatus = STATUS_NO_MEMORY;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    if (ppShareInfoList)
    {
        SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    return ntStatus;

error:

    if (pszError)
    {
        LWIO_LOG_ERROR("%s", pszError);
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

    ntStatus = LW_RTL_ALLOCATE(
                    &ppShareInfoList,
                    PSHARE_DB_INFO,
                    sizeof(PSHARE_DB_INFO) * nRows);
    BAIL_ON_NT_STATUS(ntStatus);

    for (iRow = 0; iRow < nRows; iRow++)
    {
        ntStatus = LW_RTL_ALLOCATE(
                        &pShareInfo,
                        SHARE_DB_INFO,
                        sizeof(SHARE_DB_INFO));
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
                    else
                    {
                       /* Deal with empty comments */
                       ntStatus = SMBMbsToWc16s(
                                       "",
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
    //    BOOLEAN bInLock = FALSE;

    if (IsNullOrEmptyString(pszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    //    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pShareDBContext->mutex);

    pszQuery = sqlite3_mprintf(
                    DB_QUERY_DELETE_SHARE,
                    pszShareName);

    if (pszQuery == NULL)
    {
        ntStatus = STATUS_NO_MEMORY;
        BAIL_ON_NT_STATUS(ntStatus);
    }

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

    //    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    return ntStatus;

error:

    if (pszError)
    {
        LWIO_LOG_ERROR("%s", pszError);
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

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pShareDBContext->mutex);

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

    LWIO_UNLOCK_RWMUTEX(bInLock, &pShareDBContext->mutex);

    return ntStatus;

error:

    *pulNumShares = 0;

    if (pszError)
    {
        LWIO_LOG_ERROR("%s", pszError);
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

    LwRtlMemoryFree(ppShareInfoList);
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

    if (pShareInfo->pwszName)
    {
        LwRtlMemoryFree(pShareInfo->pwszName);
    }
    if (pShareInfo->pwszPath)
    {
        LwRtlMemoryFree(pShareInfo->pwszPath);
    }
    if (pShareInfo->pwszSID)
    {
        LwRtlMemoryFree(pShareInfo->pwszSID);
    }
    if (pShareInfo->pwszComment)
    {
        LwRtlMemoryFree(pShareInfo->pwszComment);
    }

    LwRtlMemoryFree(pShareInfo);
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

    if (pShareDBContext->pwszFileSystemPrefix)
    {
        LwRtlMemoryFree(pShareDBContext->pwszFileSystemPrefix);
    }

    if (pShareDBContext->pwszFileSystemRoot)
    {
        LwRtlMemoryFree(pShareDBContext->pwszFileSystemRoot);
    }

    if (pShareDBContext->pwszPipeSystemRoot)
    {
        LwRtlMemoryFree(pShareDBContext->pwszPipeSystemRoot);
    }
}
