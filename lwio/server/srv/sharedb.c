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
SrvShareDbAcquireContext(
	PLWIO_SRV_SHARE_DB_CONTEXT* ppDbContext
    );

static
VOID
SrvShareDbReleaseContext(
	PLWIO_SRV_SHARE_DB_CONTEXT pDbContext
    );

static
VOID
SrvShareDbFreeContext(
	PLWIO_SRV_SHARE_DB_CONTEXT pDbContext
    );

static
NTSTATUS
SrvShareDbCreate(
    VOID
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
    sqlite3_stmt*    pSqlStatement,
    PSHARE_DB_INFO** pppShareInfoList,
    PULONG           pulNumSharesFound
    );

NTSTATUS
SrvShareDbInit(
    VOID
    )
{
    NTSTATUS ntStatus = 0;

    pthread_rwlock_init(&gSMBSrvGlobals.dbMutex, NULL);
    gSMBSrvGlobals.pDbMutex = &gSMBSrvGlobals.dbMutex;

    ntStatus = SrvShareDbCreate();
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
SrvShareDbAcquireContext(
	PLWIO_SRV_SHARE_DB_CONTEXT* ppDbContext
    )
{
	NTSTATUS ntStatus = 0;
	BOOLEAN bInLock = FALSE;
	PLWIO_SRV_SHARE_DB_CONTEXT pDbContext = NULL;

	LWIO_LOCK_MUTEX(bInLock, &gSMBSrvGlobals.mutex);

	if (gSMBSrvGlobals.ulNumDbContexts)
	{
		pDbContext = gSMBSrvGlobals.pDbContextList;
		gSMBSrvGlobals.pDbContextList = gSMBSrvGlobals.pDbContextList->pNext;
		pDbContext->pNext = NULL;

		gSMBSrvGlobals.ulNumDbContexts--;

		LWIO_UNLOCK_MUTEX(bInLock, &gSMBSrvGlobals.mutex);
	}
	else
	{
		PCSTR pszShareDbPath = LWIO_SRV_SHARE_DB;

        ntStatus = LW_RTL_ALLOCATE(
                      &pDbContext,
                      LWIO_SRV_SHARE_DB_CONTEXT,
                      sizeof(LWIO_SRV_SHARE_DB_CONTEXT));
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = sqlite3_open(
						pszShareDbPath,
                        &pDbContext->pDbHandle);
        if (ntStatus)
        {
		LWIO_LOG_ERROR("Sqlite3 Error (code: %d): %s",
		                        ntStatus,
		                        "Failed to open database handle""");
		ntStatus = STATUS_INTERNAL_DB_ERROR;
        }
        BAIL_ON_NT_STATUS(ntStatus);
	}

	*ppDbContext = pDbContext;

cleanup:

	LWIO_UNLOCK_MUTEX(bInLock, &gSMBSrvGlobals.mutex);

	return ntStatus;

error:

	*ppDbContext = NULL;

	if (pDbContext)
	{
		SrvShareDbFreeContext(pDbContext);
	}

	goto cleanup;
}

static
VOID
SrvShareDbReleaseContext(
	PLWIO_SRV_SHARE_DB_CONTEXT pDbContext
    )
{
	BOOLEAN bInLock = FALSE;

	LWIO_LOCK_MUTEX(bInLock, &gSMBSrvGlobals.mutex);

	if (gSMBSrvGlobals.ulNumDbContexts < gSMBSrvGlobals.ulMaxNumDbContexts)
	{
		SrvShareDbFreeContext(pDbContext);
	}
	else
	{
		pDbContext->pNext = gSMBSrvGlobals.pDbContextList;
		gSMBSrvGlobals.pDbContextList = pDbContext;

		gSMBSrvGlobals.ulNumDbContexts++;
	}

	LWIO_UNLOCK_MUTEX(bInLock, &gSMBSrvGlobals.mutex);
}

static
VOID
SrvShareDbFreeContext(
	PLWIO_SRV_SHARE_DB_CONTEXT pDbContext
	)
{
	if (pDbContext->pInsertStmt)
	{
		sqlite3_finalize(pDbContext->pInsertStmt);
	}
	if (pDbContext->pEnumStmt)
	{
		sqlite3_finalize(pDbContext->pEnumStmt);
	}
	if (pDbContext->pDeleteStmt)
	{
		sqlite3_finalize(pDbContext->pDeleteStmt);
	}
	if (pDbContext->pCountStmt)
	{
		sqlite3_finalize(pDbContext->pCountStmt);
	}
	if (pDbContext->pFindStmt)
	{
		sqlite3_finalize(pDbContext->pFindStmt);
	}
	if (pDbContext->pDbHandle)
	{
		sqlite3_close(pDbContext->pDbHandle);
	}

	LwRtlMemoryFree(pDbContext);
}

static
NTSTATUS
SrvShareDbCreate(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    PSTR pszError = NULL;
    PSTR pszFileSystemRoot = NULL;
    CHAR szTmpFileSystemRoot[] = LWIO_SRV_FILE_SYSTEM_ROOT_A;
    CHAR szPipeSystemRoot[] = LWIO_SRV_PIPE_SYSTEM_ROOT_A;
    BOOLEAN bExists = FALSE;
    PCSTR pszShareDBPath = LWIO_SRV_SHARE_DB;
    PCSTR pszShareDBDir = LWIO_SRV_DB_DIR;
    BOOLEAN bInLock = FALSE;

    ntStatus = SMBCheckFileExists(pszShareDBPath, &bExists);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO: Implement an upgrade scenario
    if (bExists)
    {
       goto cleanup;
    }

    ntStatus = SMBCheckDirectoryExists(pszShareDBDir, &bExists);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base folder */
        ntStatus = SMBCreateDirectory(pszShareDBDir, cacheDirMode);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* restrict access to u+rwx to the db folder */
    ntStatus = SMBChangeOwnerAndPermissions(pszShareDBDir, 0, 0, S_IRWXU);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbAcquireContext(&pDbContext);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSMBSrvGlobals.dbMutex);

    ntStatus = sqlite3_exec(
					   pDbContext->pDbHandle,
					   DB_QUERY_CREATE_SHARES_TABLE,
					   NULL,
					   NULL,
					   &pszError);
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_DB(ntStatus, pDbContext->pDbHandle);

    ntStatus = SMBChangeOwnerAndPermissions(
					pszShareDBPath,
                    0,
                    0,
                    S_IRWXU);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateStringPrintf(
                    &pszFileSystemRoot,
                    "%s%s%s",
                    &szTmpFileSystemRoot[0],
                    (((szTmpFileSystemRoot[strlen(&szTmpFileSystemRoot[0])-1] == '/') ||
                      (szTmpFileSystemRoot[strlen(&szTmpFileSystemRoot[0])-1] == '\\')) ? "" : "\\"),
                    LWIO_SRV_DEFAULT_SHARE_PATH_A);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvShareDbReleaseContext(pDbContext);
    pDbContext = NULL;

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSMBSrvGlobals.dbMutex);

    ntStatus = SrvShareDbAdd(
                    "IPC$",
                    &szPipeSystemRoot[0],
                    "Remote IPC",
                    NULL,
                    0,
                    "IPC");
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbAdd(
                    "C$",
                    pszFileSystemRoot,
                    "Default Share",
                    NULL,
                    0,
                    "A:");
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    if (pDbContext)
    {
	SrvShareDbReleaseContext(pDbContext);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSMBSrvGlobals.dbMutex);

    if (pszFileSystemRoot)
    {
        LwRtlMemoryFree(pszFileSystemRoot);
    }

    return ntStatus;

error:

    if (pszError)
    {
       LWIO_LOG_ERROR("%s", pszError);
       sqlite3_free(pszError);
    }

    goto cleanup;
}

NTSTATUS
SrvShareDbAdd(
    PCSTR                pszShareName,
    PCSTR                pszPath,
    PCSTR                pszComment,
    PBYTE                pSecDesc,
    ULONG                ulSecDescLen,
    PCSTR                pszService
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSMBSrvGlobals.dbMutex);

    ntStatus = SrvShareDbAdd_inlock(
                    pszShareName,
                    pszPath,
                    pszComment,
                    pSecDesc,
                    ulSecDescLen,
                    pszService);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSMBSrvGlobals.dbMutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvShareDbAdd_inlock(
	PCSTR                pszShareName,
	PCSTR                pszPath,
	PCSTR                pszComment,
	PBYTE                pSecDesc,
	ULONG                ulSecDescLen,
	PCSTR                pszService
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    int  iCol = 1;
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

    ntStatus = SrvShareDbAcquireContext(&pDbContext);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pDbContext->pInsertStmt)
    {
	PCSTR pszSqlTemplate = "INSERT INTO " LWIO_SRV_SHARES_DB_TABLE_NAME \
                               "(" LWIO_SRV_SHARES_DB_COL_NAME ","          \
                                   LWIO_SRV_SHARES_DB_COL_PATH ","          \
                                   LWIO_SRV_SHARES_DB_COL_COMMENT ","       \
                                   LWIO_SRV_SHARES_DB_COL_SECDESC ","       \
                                   LWIO_SRV_SHARES_DB_COL_SERVICE           \
                               ") VALUES (?1, ?2, ?3, ?4, ?5)";
	ntStatus = sqlite3_prepare_v2(
						pDbContext->pDbHandle,
						pszSqlTemplate,
						-1,
						&pDbContext->pInsertStmt,
						NULL);
	BAIL_ON_LWIO_SRV_SQLITE_ERROR_DB(ntStatus, pDbContext->pDbHandle);
    }

    ntStatus = sqlite3_bind_text(
					pDbContext->pInsertStmt,
					iCol++,
					pszShareName,
					-1,
					SQLITE_TRANSIENT);
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pInsertStmt);

    ntStatus = sqlite3_bind_text(
					pDbContext->pInsertStmt,
					iCol++,
					pszPath,
					-1,
					SQLITE_TRANSIENT);
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pInsertStmt);

    if (pszComment)
    {
        ntStatus = sqlite3_bind_text(
					pDbContext->pInsertStmt,
					iCol++,
					pszComment,
					-1,
					SQLITE_TRANSIENT);
    }
    else
    {
	ntStatus = sqlite3_bind_null(pDbContext->pInsertStmt, iCol++);
    }
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pInsertStmt);

	if (pSecDesc)
	{
		ntStatus = sqlite3_bind_blob(
						pDbContext->pInsertStmt,
						iCol++,
						pSecDesc,
						ulSecDescLen,
						SQLITE_TRANSIENT);
	}
    else
    {
	ntStatus = sqlite3_bind_null(pDbContext->pInsertStmt, iCol++);
    }
	BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pInsertStmt);

    ntStatus = sqlite3_bind_text(
					pDbContext->pInsertStmt,
					iCol++,
					pszService,
					-1,
					SQLITE_TRANSIENT);
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pInsertStmt);

    if ((ntStatus = sqlite3_step(pDbContext->pInsertStmt)) == SQLITE_DONE)
    {
	ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pInsertStmt);

cleanup:

    if (pDbContext)
    {
	if (pDbContext->pInsertStmt)
	{
		sqlite3_reset(pDbContext->pInsertStmt);
	}
	SrvShareDbReleaseContext(pDbContext);
    }

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvShareMapFromWindowsPath(
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
    )
{
    NTSTATUS  ntStatus = 0;
    wchar16_t wszBackslash[2] = {'\\', 0};
    wchar16_t wszFwdslash[2] = {'/', 0};
    wchar16_t wszColon[2] = {':', 0};
    PWSTR     pwszPathReadCursor = pwszInputPath;
    PWSTR     pwszPathWriteCursor = NULL;
    PWSTR     pwszPath = NULL;
    size_t    sInputPathLen = 0;
    size_t    sFSPrefixLen = 3;
    size_t    sFSRootLen = 0;
    size_t    sRequiredLen = 0;
    wchar16_t wszFileSystemRoot[] = LWIO_SRV_FILE_SYSTEM_ROOT_W;

    if (!pwszInputPath || !*pwszInputPath)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    sInputPathLen = wc16slen(pwszInputPath);

    sFSRootLen = wc16slen(&wszFileSystemRoot[0]);

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
    memcpy((PBYTE)pwszPathWriteCursor, (PBYTE)&wszFileSystemRoot[0], sFSRootLen * sizeof(wchar16_t));
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
    PWSTR  pwszInputPath,
    PWSTR* ppwszPath
    )
{
    NTSTATUS ntStatus = 0;
    PWSTR pwszPath = NULL;
    wchar16_t wszFileSystemPrefix[] = LWIO_SRV_FILE_SYSTEM_PREFIX_W;
    wchar16_t wszFileSystemRoot[] = LWIO_SRV_FILE_SYSTEM_ROOT_W;

    ntStatus = SrvShareMapSpecificToWindowsPath(
                    &wszFileSystemPrefix[0],
                    &wszFileSystemRoot[0],
                    pwszInputPath,
                    &pwszPath);
    if (ntStatus == STATUS_OBJECT_PATH_SYNTAX_BAD)
    {
	wchar16_t wszPipeSystemRoot[] = LWIO_SRV_PIPE_SYSTEM_ROOT_W;

        ntStatus = SrvShareMapSpecificToWindowsPath(
                            &wszFileSystemPrefix[0],
                            &wszPipeSystemRoot[0],
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
    wchar16_t wszBackslash[2] = {'\\', 0};
    wchar16_t wszFwdslash[2] = {'/', 0};
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
    ULONG            ulOffset,
    ULONG            ulLimit,
    PSHARE_DB_INFO** pppShareInfoList,
    PULONG           pulNumSharesFound
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSMBSrvGlobals.dbMutex);

    ntStatus = SrvShareDbEnum_inlock(
                      ulOffset,
                      ulLimit,
                      pppShareInfoList,
                      pulNumSharesFound);

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSMBSrvGlobals.dbMutex);

    return ntStatus;
}


NTSTATUS
SrvShareDbEnum_inlock(
    ULONG            ulOffset,
    ULONG            ulLimit,
    PSHARE_DB_INFO** pppShareInfoList,
    PULONG           pulNumSharesFound
    )
{
    NTSTATUS ntStatus = 0;
    PSHARE_DB_INFO* ppShareInfoList = NULL;
    ULONG    ulNumSharesFound = 0;
    PLWIO_SRV_SHARE_DB_CONTEXT pDbContext = NULL;

    ntStatus = SrvShareDbAcquireContext(&pDbContext);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pDbContext->pEnumStmt)
    {
		PCSTR pszQueryTemplate = "SELECT " LWIO_SRV_SHARES_DB_COL_NAME    ","  \
				                           LWIO_SRV_SHARES_DB_COL_PATH    ","  \
				                           LWIO_SRV_SHARES_DB_COL_COMMENT ","  \
				                           LWIO_SRV_SHARES_DB_COL_SECDESC ","  \
				                           LWIO_SRV_SHARES_DB_COL_SERVICE      \
		                         " FROM "  LWIO_SRV_SHARES_DB_TABLE_NAME       \
		                         " ORDER BY " LWIO_SRV_SHARES_DB_COL_NAME      \
		                         " LIMIT ?1 OFFSET ?2";

		ntStatus = sqlite3_prepare_v2(
						pDbContext->pDbHandle,
						pszQueryTemplate,
						-1,
						&pDbContext->pEnumStmt,
						NULL);
		BAIL_ON_LWIO_SRV_SQLITE_ERROR_DB(ntStatus, pDbContext->pDbHandle);
    }

    ntStatus = sqlite3_bind_int(pDbContext->pEnumStmt, 1, ulLimit);
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pEnumStmt);

    ntStatus = sqlite3_bind_int(pDbContext->pEnumStmt, 2, ulOffset);
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pEnumStmt);

    ntStatus = SrvShareDbWriteToShareInfo(
					pDbContext->pEnumStmt,
                    &ppShareInfoList,
                    &ulNumSharesFound);
    BAIL_ON_NT_STATUS(ntStatus);

    *pppShareInfoList =  ppShareInfoList;
    *pulNumSharesFound = ulNumSharesFound;

cleanup:

    if (pDbContext)
    {
	if (pDbContext->pEnumStmt)
	{
		sqlite3_reset(pDbContext->pEnumStmt);
	}
	SrvShareDbReleaseContext(pDbContext);
    }

    return ntStatus;

error:

    if (ppShareInfoList)
    {
        SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
    }

    goto cleanup;
}

NTSTATUS
SrvShareDbFindByName(
    PCSTR           pszShareName,
    PSHARE_DB_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    PSHARE_DB_INFO* ppShareInfoList = NULL;
    BOOLEAN  bInLock = FALSE;
    ULONG    ulNumSharesFound = 0;
    PLWIO_SRV_SHARE_DB_CONTEXT pDbContext = NULL;

    if (IsNullOrEmptyString(pszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_1;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareDbAcquireContext(&pDbContext);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pDbContext->pFindStmt)
    {
	PCSTR pszQueryTemplate = "SELECT " LWIO_SRV_SHARES_DB_COL_NAME    ","  \
                                           LWIO_SRV_SHARES_DB_COL_PATH    ","  \
                                           LWIO_SRV_SHARES_DB_COL_COMMENT ","  \
                                           LWIO_SRV_SHARES_DB_COL_SECDESC ","  \
                                           LWIO_SRV_SHARES_DB_COL_SERVICE      \
                                 " FROM "  LWIO_SRV_SHARES_DB_TABLE_NAME       \
                    " WHERE UPPER(" LWIO_SRV_SHARES_DB_COL_NAME ") = UPPER(?1)";

	ntStatus = sqlite3_prepare_v2(
						pDbContext->pDbHandle,
						pszQueryTemplate,
						-1,
						&pDbContext->pFindStmt,
						NULL);
	BAIL_ON_LWIO_SRV_SQLITE_ERROR_DB(ntStatus, pDbContext->pDbHandle);
    }

    ntStatus = sqlite3_bind_text(
					pDbContext->pFindStmt,
					1,
					pszShareName,
					-1,
					SQLITE_TRANSIENT);
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pFindStmt);

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSMBSrvGlobals.dbMutex);

    ntStatus = SrvShareDbWriteToShareInfo(
                    pDbContext->pFindStmt,
                    &ppShareInfoList,
                    &ulNumSharesFound);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppShareInfo = *ppShareInfoList;

cleanup:

    if (pDbContext)
    {
	if (pDbContext->pFindStmt)
	{
		sqlite3_reset(pDbContext->pFindStmt);
	}
	SrvShareDbReleaseContext(pDbContext);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSMBSrvGlobals.dbMutex);

	if (ppShareInfoList)
	{
		SrvShareDbFreeInfoList(ppShareInfoList, ulNumSharesFound);
	}

    return ntStatus;

error:

	*ppShareInfo = NULL;

    goto cleanup;
}

static
NTSTATUS
SrvShareDbWriteToShareInfo(
    sqlite3_stmt*    pSqlStatement,
    PSHARE_DB_INFO** pppShareInfoList,
    PULONG           pulNumSharesFound
    )
{
    NTSTATUS ntStatus = 0;
    PSHARE_DB_INFO* ppShareInfoList = NULL;
    PSHARE_DB_INFO pShareInfo = NULL;
    ULONG ulNumSharesAllocated = 0;
    ULONG ulNumSharesAvailable = 0;
    ULONG iShare = 0;

    while ((ntStatus = sqlite3_step(pSqlStatement)) == SQLITE_ROW)
    {
	ULONG iCol = 0;

	if (!ulNumSharesAvailable)
	{
		PSHARE_DB_INFO* ppShareInfoListNew = NULL;
		ULONG ulNumSharesNew = 5;
		ULONG ulNumSharesAllocatedNew = ulNumSharesAllocated + ulNumSharesNew;

	    ntStatus = LW_RTL_ALLOCATE(
	                    &ppShareInfoListNew,
	                    PSHARE_DB_INFO,
	                    sizeof(PSHARE_DB_INFO) * ulNumSharesAllocatedNew);
	    BAIL_ON_NT_STATUS(ntStatus);

	    if (ppShareInfoList)
	    {
		memcpy((PBYTE)ppShareInfoListNew,
			   (PBYTE)ppShareInfoList,
			   sizeof(PSHARE_DB_INFO) * ulNumSharesAllocated);

		LwRtlMemoryFree(ppShareInfoList);
	    }

	    ppShareInfoList = ppShareInfoListNew;
	    ulNumSharesAllocated = ulNumSharesAllocatedNew;
	    ulNumSharesAvailable = ulNumSharesNew;
	}

        ntStatus = LW_RTL_ALLOCATE(
                        &pShareInfo,
                        SHARE_DB_INFO,
                        sizeof(SHARE_DB_INFO));
        BAIL_ON_NT_STATUS(ntStatus);

        pShareInfo->refcount = 1;

        pthread_rwlock_init(&pShareInfo->mutex, NULL);
        pShareInfo->pMutex = &pShareInfo->mutex;

        for (; iCol < 5; iCol++)
        {
		const unsigned char* pszStringVal = NULL;
		ULONG ulNumBytes = sqlite3_column_bytes(pSqlStatement, iCol);

            switch(iCol)
            {
                case 0: /* ShareName */

			if (ulNumBytes)
			{
						pszStringVal = sqlite3_column_text(pSqlStatement, iCol);

						ntStatus = SMBMbsToWc16s(
										   (PCSTR)pszStringVal,
										   &pShareInfo->pwszName);
						BAIL_ON_NT_STATUS(ntStatus);
			}

                    break;

                case 1: /* PathName */

			if (ulNumBytes)
			{
						pszStringVal = sqlite3_column_text(pSqlStatement, iCol);

						ntStatus = SMBMbsToWc16s(
										   (PCSTR)pszStringVal,
										   &pShareInfo->pwszPath);
						BAIL_ON_NT_STATUS(ntStatus);
			}

                    break;

                case 2: /* Comment */

			if (ulNumBytes)
			{
						pszStringVal = sqlite3_column_text(pSqlStatement, iCol);

						ntStatus = SMBMbsToWc16s(
										   (PCSTR)pszStringVal,
										   &pShareInfo->pwszComment);
                    }
                    else
                    {
                       /* Deal with empty comments */
                       ntStatus = SMBMbsToWc16s(
                                       "",
                                       &pShareInfo->pwszComment);
                    }
                    BAIL_ON_NT_STATUS(ntStatus);

                    break;

                case 3: /* Security Descriptor */

                    if (ulNumBytes)
                    {
			PCVOID pBlob = sqlite3_column_blob(pSqlStatement, iCol);

                        ntStatus = LW_RTL_ALLOCATE(
                                       &pShareInfo->pSecDesc,
                                       BYTE,
                                       ulNumBytes);
                        BAIL_ON_NT_STATUS(ntStatus);

                        memcpy(pShareInfo->pSecDesc, pBlob, ulNumBytes);
                        pShareInfo->ulSecDescLen = ulNumBytes;
                    }

                    break;

                case 4: /* service */

			if (ulNumBytes)
			{
				pszStringVal = sqlite3_column_text(pSqlStatement, iCol);
			}

                    ntStatus = SrvShareGetServiceId(
                                    (PCSTR)pszStringVal,
                                    &pShareInfo->service);
                    BAIL_ON_NT_STATUS(ntStatus);

                    break;
            }
        }

        *(ppShareInfoList + iShare++) = pShareInfo;
        pShareInfo = NULL;

        ulNumSharesAvailable--;
    }
    if (ntStatus == SQLITE_DONE)
    {
	ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pSqlStatement);

    *pppShareInfoList = ppShareInfoList;
    *pulNumSharesFound = iShare;

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
        SrvShareDbFreeInfoList(ppShareInfoList, iShare);
    }

    goto cleanup;
}

NTSTATUS
SrvShareDbDelete(
    PCSTR  pszShareName
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    BOOLEAN bInLock = FALSE;

    if (IsNullOrEmptyString(pszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvShareDbAcquireContext(&pDbContext);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gSMBSrvGlobals.dbMutex);

    if (!pDbContext->pDeleteStmt)
    {
	PCSTR pszQueryTemplate = "DELETE FROM " LWIO_SRV_SHARES_DB_TABLE_NAME \
	           " WHERE UPPER(" LWIO_SRV_SHARES_DB_COL_NAME ") = UPPER(?1)";

	ntStatus = sqlite3_prepare_v2(
						pDbContext->pDbHandle,
						pszQueryTemplate,
						-1,
						&pDbContext->pDeleteStmt,
						NULL);
	BAIL_ON_LWIO_SRV_SQLITE_ERROR_DB(ntStatus, pDbContext->pDbHandle);
    }

    ntStatus = sqlite3_bind_text(
					pDbContext->pDeleteStmt,
					1,
					pszShareName,
					-1,
					SQLITE_TRANSIENT);
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pDeleteStmt);

    if ((ntStatus = sqlite3_step(pDbContext->pDeleteStmt)) == SQLITE_DONE)
    {
	ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pDeleteStmt);

cleanup:

    if (pDbContext)
    {
	if (pDbContext->pDeleteStmt)
		{
		   sqlite3_reset(pDbContext->pDeleteStmt);
		}
	SrvShareDbReleaseContext(pDbContext);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSMBSrvGlobals.dbMutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvShareDbGetCount(
    PULONG  pulNumShares
    )
{
    NTSTATUS ntStatus = 0;
    INT   nShareCount = 0;
    PLWIO_SRV_SHARE_DB_CONTEXT pDbContext = NULL;
    BOOLEAN bInLock = FALSE;

    ntStatus = SrvShareDbAcquireContext(&pDbContext);
    BAIL_ON_NT_STATUS(ntStatus);

    if (!pDbContext->pCountStmt)
    {
	PCSTR pszQueryTemplate = "SELECT COUNT(*) FROM " \
								 LWIO_SRV_SHARES_DB_TABLE_NAME;


	ntStatus = sqlite3_prepare_v2(
						pDbContext->pDbHandle,
						pszQueryTemplate,
						-1,
						&pDbContext->pCountStmt,
						NULL);
	BAIL_ON_LWIO_SRV_SQLITE_ERROR_DB(ntStatus, pDbContext->pDbHandle);
    }

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gSMBSrvGlobals.dbMutex);

    if ((ntStatus = sqlite3_step(pDbContext->pCountStmt) == SQLITE_ROW))
    {
        if (sqlite3_column_count(pDbContext->pCountStmt) != 1)
        {
            ntStatus = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        nShareCount = sqlite3_column_int(pDbContext->pCountStmt, 0);

        ntStatus = STATUS_SUCCESS;
    }
    BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pCountStmt);

    *pulNumShares = nShareCount;

cleanup:

	if (pDbContext)
	{
		if (pDbContext->pCountStmt)
		{
			sqlite3_reset(pDbContext->pCountStmt);
		}
		SrvShareDbReleaseContext(pDbContext);
	}

    LWIO_UNLOCK_RWMUTEX(bInLock, &gSMBSrvGlobals.dbMutex);

    return ntStatus;

error:

    *pulNumShares = 0;

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
    if (pShareInfo->pSecDesc)
    {
        LwRtlMemoryFree(pShareInfo->pSecDesc);
    }
    if (pShareInfo->pwszComment)
    {
        LwRtlMemoryFree(pShareInfo->pwszComment);
    }

    LwRtlMemoryFree(pShareInfo);
}

VOID
SrvShareDbShutdown(
    VOID
    )
{
	if (gSMBSrvGlobals.pDbMutex)
    {
        pthread_rwlock_destroy(&gSMBSrvGlobals.dbMutex);
        gSMBSrvGlobals.pDbMutex = NULL;
    }

    while (gSMBSrvGlobals.pDbContextList)
    {
	PLWIO_SRV_SHARE_DB_CONTEXT pDbContext = gSMBSrvGlobals.pDbContextList;
	gSMBSrvGlobals.pDbContextList = gSMBSrvGlobals.pDbContextList->pNext;

	SrvShareDbFreeContext(pDbContext);
    }

    gSMBSrvGlobals.ulNumDbContexts = gSMBSrvGlobals.ulMaxNumDbContexts;
}
