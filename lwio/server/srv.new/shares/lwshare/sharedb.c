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

static
NTSTATUS
SrvShareDbCreate(
    VOID
    );

static
NTSTATUS
SrvShareDbAdd_inlock(
    IN PSRV_SHARE_DB_CONTEXT pDbContext,
	IN PCSTR                 pszShareName,
	IN PCSTR                 pszPath,
	IN PCSTR                 pszComment,
	IN PBYTE                 pSecDesc,
	IN ULONG                 ulSecDescLen,
	IN PCSTR                 pszService
    );

static
NTSTATUS
SrvShareDbWriteToShareInfo(
    IN     sqlite3_stmt*     pSqlStatement,
    OUT    PSRV_SHARE_INFO** pppShareInfoList,
    IN OUT PULONG            pulNumSharesFound
    );

NTSTATUS
SrvShareDbInit(
    VOID
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = SrvShareDbCreate();
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}


static
NTSTATUS
SrvShareDbCreate(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_DB_CONTEXT pDbContext = NULL;
    PSTR pszError = NULL;
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

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gShareRepository_lwshare.dbMutex);

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

cleanup:

    if (pDbContext)
    {
	SrvShareDbReleaseContext(pDbContext);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gShareRepository_lwshare.dbMutex);

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
SrvShareDbOpen(
    OUT PHANDLE phRepository
    )
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PSRV_SHARE_DB_CONTEXT pDbContext = NULL;

	ntStatus = SrvShareDbAcquireContext(&pDbContext);
	BAIL_ON_NT_STATUS(ntStatus);

	*phRepository = (HANDLE)pDbContext;

cleanup:

	return ntStatus;

error:

	*phRepository = (HANDLE)NULL;

	goto cleanup;
}

NTSTATUS
SrvShareDbFindByName(
	HANDLE           hRepository,
    PWSTR            pwszShareName,
    PSRV_SHARE_INFO* ppShareInfo
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_INFO* ppShareInfoList = NULL;
    PSTR     pszShareName = NULL;
    BOOLEAN  bInLock = FALSE;
    ULONG    ulNumSharesFound = 0;
    PSRV_SHARE_DB_CONTEXT pDbContext = NULL;

    if (IsNullOrEmptyString(pwszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    pDbContext = (PSRV_SHARE_DB_CONTEXT)hRepository;

    ntStatus = SrvWc16sToMbs(pwszShareName, &pszShareName);
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

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gShareRepository_lwshare.dbMutex);

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
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gShareRepository_lwshare.dbMutex);

	if (ppShareInfoList)
	{
		SrvShareFreeInfoList(ppShareInfoList, ulNumSharesFound);
	}

	SRV_SAFE_FREE_MEMORY(pszShareName);

    return ntStatus;

error:

	*ppShareInfo = NULL;

    goto cleanup;
}

NTSTATUS
SrvShareDbAdd(
	IN HANDLE hRepository,
    IN PWSTR  pwszShareName,
    IN PWSTR  pwszPath,
    IN PWSTR  pwszComment,
    IN PBYTE  pSecDesc,
    IN ULONG  ulSecDescLen,
    IN PWSTR  pwszService
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_DB_CONTEXT pDbContext = NULL;
    PSTR pszShareName = NULL;
    PSTR pszPath = NULL;
    PSTR pszComment = NULL;
    PSTR pszService = NULL;
    BOOLEAN bInLock = FALSE;

    if (pwszShareName)
    {
		ntStatus = SrvWc16sToMbs(pwszShareName, &pszShareName);
		BAIL_ON_NT_STATUS(ntStatus);
    }
    if (pwszPath)
    {
		ntStatus = SrvWc16sToMbs(pwszPath, &pszPath);
		BAIL_ON_NT_STATUS(ntStatus);
    }
    if (pwszComment)
    {
		ntStatus = SrvWc16sToMbs(pwszComment, &pszComment);
		BAIL_ON_NT_STATUS(ntStatus);
    }
    if (pwszService)
    {
	ntStatus = SrvWc16sToMbs(pwszService, &pszService);
	BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gShareRepository_lwshare.dbMutex);

    pDbContext = (PSRV_SHARE_DB_CONTEXT)hRepository;

    ntStatus = SrvShareDbAdd_inlock(
					pDbContext,
                    pszShareName,
                    pszPath,
                    pszComment,
                    pSecDesc,
                    ulSecDescLen,
                    pszService);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &gShareRepository_lwshare.dbMutex);

    SRV_SAFE_FREE_MEMORY(pszShareName);
    SRV_SAFE_FREE_MEMORY(pszPath);
    SRV_SAFE_FREE_MEMORY(pszComment);
    SRV_SAFE_FREE_MEMORY(pszService);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SrvShareDbAdd_inlock(
    IN PSRV_SHARE_DB_CONTEXT pDbContext,
	IN PCSTR                 pszShareName,
	IN PCSTR                 pszPath,
	IN PCSTR                 pszComment,
	IN PBYTE                 pSecDesc,
	IN ULONG                 ulSecDescLen,
	IN PCSTR                 pszService
    )
{
    NTSTATUS ntStatus = 0;
    int  iCol = 1;

    if (IsNullOrEmptyString(pszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_2;
    }
    if (IsNullOrEmptyString(pszPath))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
    }

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
    }

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvShareDbBeginEnum(
	HANDLE  hRepository,
	ULONG   ulLimit,
	PHANDLE phResume
	)
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	PSRV_SHARE_DB_ENUM_CONTEXT pEnumContext = NULL;

	ntStatus = SrvAllocateMemory(
					sizeof(SRV_SHARE_DB_ENUM_CONTEXT),
					(PVOID*)&pEnumContext);
	BAIL_ON_NT_STATUS(ntStatus);

	pEnumContext->ulOffset = 0;
	pEnumContext->ulLimit = ulLimit;

	*phResume = (HANDLE)pEnumContext;

cleanup:

	return ntStatus;

error:

	*phResume = NULL;

	SRV_SAFE_FREE_MEMORY(pEnumContext);

	goto cleanup;
}

NTSTATUS
SrvShareDbEnum(
    HANDLE            hRepository,
    HANDLE            hResume,
    PSRV_SHARE_INFO** pppShareInfoList,
    PULONG            pulNumSharesFound
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_DB_CONTEXT pDbContext = (PSRV_SHARE_DB_CONTEXT)hRepository;
    PSRV_SHARE_DB_ENUM_CONTEXT pResume = (PSRV_SHARE_DB_ENUM_CONTEXT)hResume;
    PSRV_SHARE_INFO* ppShareInfoList = NULL;
    ULONG    ulNumSharesFound = 0;
    BOOLEAN  bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gShareRepository_lwshare.dbMutex);

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

	ntStatus = sqlite3_bind_int(pDbContext->pEnumStmt, 1, pResume->ulLimit);
	BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pEnumStmt);

	ntStatus = sqlite3_bind_int(pDbContext->pEnumStmt, 2, pResume->ulOffset);
	BAIL_ON_LWIO_SRV_SQLITE_ERROR_STMT(ntStatus, pDbContext->pEnumStmt);

	ntStatus = SrvShareDbWriteToShareInfo(
					pDbContext->pEnumStmt,
					&ppShareInfoList,
					&ulNumSharesFound);
	BAIL_ON_NT_STATUS(ntStatus);

	pResume->ulOffset += ulNumSharesFound;

	*pppShareInfoList =  ppShareInfoList;
	*pulNumSharesFound = ulNumSharesFound;

cleanup:

	if (pDbContext)
	{
		if (pDbContext->pEnumStmt)
		{
			sqlite3_reset(pDbContext->pEnumStmt);
		}
	}

    LWIO_UNLOCK_RWMUTEX(bInLock, &gShareRepository_lwshare.dbMutex);

	return ntStatus;

error:

	if (ppShareInfoList)
	{
		SrvShareFreeInfoList(ppShareInfoList, ulNumSharesFound);
	}

	goto cleanup;
}

NTSTATUS
SrvShareDbEndEnum(
	IN HANDLE hRepository,
	IN HANDLE hResume
	)
{
	PSRV_SHARE_DB_ENUM_CONTEXT pResume = (PSRV_SHARE_DB_ENUM_CONTEXT)hResume;

	SrvFreeMemory(pResume);

	return STATUS_SUCCESS;
}

static
NTSTATUS
SrvShareDbWriteToShareInfo(
    IN     sqlite3_stmt*     pSqlStatement,
    OUT    PSRV_SHARE_INFO** pppShareInfoList,
    IN OUT PULONG            pulNumSharesFound
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_INFO* ppShareInfoList = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;
    ULONG ulNumSharesAllocated = 0;
    ULONG ulNumSharesAvailable = 0;
    ULONG iShare = 0;

    while ((ntStatus = sqlite3_step(pSqlStatement)) == SQLITE_ROW)
    {
	ULONG iCol = 0;

	if (!ulNumSharesAvailable)
	{
		PSRV_SHARE_INFO* ppShareInfoListNew = NULL;
		ULONG ulNumSharesNew = 5;
		ULONG ulNumSharesAllocatedNew = ulNumSharesAllocated + ulNumSharesNew;

	    ntStatus = LW_RTL_ALLOCATE(
	                    &ppShareInfoListNew,
	                    PSRV_SHARE_INFO,
	                    sizeof(PSRV_SHARE_INFO) * ulNumSharesAllocatedNew);
	    BAIL_ON_NT_STATUS(ntStatus);

	    if (ppShareInfoList)
	    {
		memcpy((PBYTE)ppShareInfoListNew,
			   (PBYTE)ppShareInfoList,
			   sizeof(PSRV_SHARE_INFO) * ulNumSharesAllocated);

		LwRtlMemoryFree(ppShareInfoList);
	    }

	    ppShareInfoList = ppShareInfoListNew;
	    ulNumSharesAllocated = ulNumSharesAllocatedNew;
	    ulNumSharesAvailable = ulNumSharesNew;
	}

        ntStatus = SrvAllocateMemory(
						sizeof(SRV_SHARE_INFO),
						(PVOID*)&pShareInfo);
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

                    ntStatus = SrvShareMapServiceStringToId(
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
        SrvShareReleaseInfo(pShareInfo);
    }

    return ntStatus;

error:

    *pppShareInfoList = NULL;
    *pulNumSharesFound = 0;

    if (ppShareInfoList)
    {
        SrvShareFreeInfoList(ppShareInfoList, iShare);
    }

    goto cleanup;
}

NTSTATUS
SrvShareDbDelete(
	IN HANDLE hRepository,
    IN PWSTR  pwszShareName
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SHARE_DB_CONTEXT pDbContext = (PSRV_SHARE_DB_CONTEXT)hRepository;
    PSTR pszShareName = NULL;
    BOOLEAN bInLock = FALSE;

    if (IsNullOrEmptyString(pwszShareName))
    {
        ntStatus = STATUS_INVALID_PARAMETER_3;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBWc16sToMbs(pwszShareName, &pszShareName);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &gShareRepository_lwshare.dbMutex);

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
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &gShareRepository_lwshare.dbMutex);

    SRV_SAFE_FREE_MEMORY(pszShareName);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvShareDbGetCount(
	IN     HANDLE  hRepository,
    IN OUT PULONG  pulNumShares
    )
{
    NTSTATUS ntStatus = 0;
    INT   nShareCount = 0;
    PSRV_SHARE_DB_CONTEXT pDbContext = (PSRV_SHARE_DB_CONTEXT)hRepository;
    BOOLEAN bInLock = FALSE;

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

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &gShareRepository_lwshare.dbMutex);

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
	}

    LWIO_UNLOCK_RWMUTEX(bInLock, &gShareRepository_lwshare.dbMutex);

    return ntStatus;

error:

    *pulNumShares = 0;

    goto cleanup;
}

VOID
SrvShareDbClose(
	IN HANDLE hRepository
	)
{
	PSRV_SHARE_DB_CONTEXT pDbContext = (PSRV_SHARE_DB_CONTEXT)hRepository;

	SrvShareDbReleaseContext(pDbContext);
}

VOID
SrvShareDbShutdown(
    VOID
    )
{
	PSRV_SHARE_DB_GLOBALS pGlobals = &gShareRepository_lwshare;

	if (pGlobals->pDbMutex)
    {
        pthread_rwlock_destroy(&pGlobals->dbMutex);
        pGlobals->pDbMutex = NULL;
    }

    while (pGlobals->pDbContextList)
    {
	PSRV_SHARE_DB_CONTEXT pDbContext = pGlobals->pDbContextList;
	pGlobals->pDbContextList = pGlobals->pDbContextList->pNext;

	SrvShareDbReleaseContext(pDbContext);
    }

    pGlobals->ulNumDbContexts = pGlobals->ulMaxNumDbContexts;
}
