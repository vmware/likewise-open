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
 *        sqldb.c
 *
 * Abstract:
 *
 *        Sqlite3 backend for Registry Database Interface
 *
 * Authors: Wei Fu (wfu@likewise.com)
 *
 */
#include "includes.h"

static
NTSTATUS
RegDbUpdateKeyAclContent_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwAclDbId,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelToSet,
    IN ULONG ulSecDescToSetLen
    );


NTSTATUS
RegDbUnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PREG_ENTRY_VERSION_INFO pResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadInt64(
        pstQuery,
        piColumnPos,
        "CacheId",
        &pResult->qwDbId);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadTimeT(
        pstQuery,
        piColumnPos,
        "LastUpdated",
        &pResult->tLastUpdated);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbUnpackRegKeyInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_DB_KEY pResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadInt64(
        pstQuery,
        piColumnPos,
        "ParentId",
        &pResult->qwParentId);
    BAIL_ON_NT_STATUS(status);

	status = RegSqliteReadWC16String(
        pstQuery,
        piColumnPos,
        "KeyName",
        &pResult->pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadInt64(
        pstQuery,
        piColumnPos,
        "AclIndex",
        &pResult->qwAclIndex);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbUnpackRegValueInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_DB_VALUE pResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadInt64(
        pstQuery,
        piColumnPos,
        "ParentId",
        &pResult->qwParentId);
    BAIL_ON_NT_STATUS(status);

	status = RegSqliteReadWC16String(
        pstQuery,
        piColumnPos,
        "ValueName",
        &pResult->pwszValueName);
    BAIL_ON_NT_STATUS(status);

    if (!pResult->pwszValueName)
    {
	status = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(status);
    }

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Type",
        &pResult->type);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadBlob(
        pstQuery,
        piColumnPos,
        "Value",
        &pResult->pValue,
        &pResult->dwValueLen);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadTimeT(
        pstQuery,
        piColumnPos,
        "LastUpdated",
        &pResult->tLastUpdated);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbUnpackSubKeysCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "subkeyCount",
        pdwCount);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbUnpackAclrefCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "aclrefCount",
        pdwCount);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbUnpackTotalAclCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "totalAclCount",
        pdwCount);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbUnpackKeyValuesCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "valueCount",
        pdwCount);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbUnpackAclIndexInfoInAcls(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    int64_t* pqwAclIndex
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadInt64(
        pstQuery,
        piColumnPos,
        "CacheId",
        pqwAclIndex);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbUnpackAclIndexInfoInKeys(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    int64_t* pqwAclIndex
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadInt64(
        pstQuery,
        piColumnPos,
        "AclIndex",
        pqwAclIndex);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbUnpackAclInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    PULONG pSecDescLen
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadBlob(
        pstQuery,
        piColumnPos,
        "Acl",
        (PBYTE*)ppSecDescRel,
        pSecDescLen);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
RegDbSetup(
    IN sqlite3* pSqlHandle
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PSTR pszError = NULL;

    status = RegSqliteExec(pSqlHandle,
                           REG_DB_CREATE_TABLES,
                           &pszError);
    if (status)
    {
        REG_LOG_DEBUG("SQL failed: code = %d, message = '%s'\nSQL =\n%s",
                      status, pszError, REG_DB_CREATE_TABLES);
    }
    BAIL_ON_SQLITE3_ERROR(status, pszError);

cleanup:
    SQLITE3_SAFE_FREE_STRING(pszError);
    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbOpen(
    IN PCSTR pszDbPath,
    OUT PREG_DB_HANDLE phDb
    )
{
	NTSTATUS status = 0;
    BOOLEAN bLockCreated = FALSE;
    PREG_DB_CONNECTION pConn = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;
    PSTR pszDbDir = NULL;
    PWSTR pwszQueryStatement = NULL;

    status = RegGetDirectoryFromPath(
                    pszDbPath,
                    &pszDbDir);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pConn, REG_DB_CONNECTION, sizeof(*pConn));
    BAIL_ON_NT_STATUS(status);

    memset(pConn, 0, sizeof(*pConn));

    status = pthread_rwlock_init(&pConn->lock, NULL);
    BAIL_ON_NT_STATUS(status);
    bLockCreated = TRUE;

    status = RegCheckDirectoryExists(pszDbDir, &bExists);
    BAIL_ON_NT_STATUS(status);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        status = RegCreateDirectory(pszDbDir, cacheDirMode);
        BAIL_ON_NT_STATUS(status);
    }

    /* restrict access to u+rwx to the db folder */
    status = RegChangeOwnerAndPermissions(pszDbDir, 0, 0, S_IRWXU);
    BAIL_ON_NT_STATUS(status);

    status = sqlite3_open(pszDbPath, &pConn->pDb);
    BAIL_ON_NT_STATUS(status);

    status = RegChangeOwnerAndPermissions(pszDbPath, 0, 0, S_IRWXU);
    BAIL_ON_NT_STATUS(status);

    status = RegDbSetup(pConn->pDb);
    BAIL_ON_NT_STATUS(status);

	/*pstCreateRegKey*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_INSERT_REG_KEY);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
		    pConn->pDb,
		    pwszQueryStatement,
				-1,
				&pConn->pstCreateRegKey,
				NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);

	/*pstCreateRegValue*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_INSERT_REG_VALUE);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
		    pConn->pDb,
		    pwszQueryStatement,
				-1,
				&pConn->pstCreateRegValue,
				NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


	/*pstCreateRegAcl*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_INSERT_REG_ACL);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
		    pConn->pDb,
		    pwszQueryStatement,
				-1,
				&pConn->pstCreateRegAcl,
				NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);

    /*pstUpdateRegAclByCacheId*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_UPDATE_REG_ACL);
    BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
                pConn->pDb,
                pwszQueryStatement,
                -1,
                &pConn->pstUpdateRegAclByCacheId,
                NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstOpenKeyEx*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_OPEN_KEY_EX);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstOpenKeyEx,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQuerySubKeys*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_SUBKEYS);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, // search for null termination in szQuery to get length
			&pConn->pstQuerySubKeys,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQuerySubKeysCount*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_SUBKEY_COUNT);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, // search for null termination in szQuery to get length
			&pConn->pstQuerySubKeysCount,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryValuesCount*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_VALUE_COUNT);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, // search for null termination in szQuery to get length
			&pConn->pstQueryValuesCount,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryAclRefCount*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_ACL_REFCOUNT);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, // search for null termination in szQuery to get length
			&pConn->pstQueryAclRefCount,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);



    /*pstDeleteKey (delete the key)*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_DELETE_KEY);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstDeleteKey,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstDeleteAllKeyValues (delete all the key's values)*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_DELETE_KEY_VALUES);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstDeleteAllKeyValues,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


	/*pstQueryKeyValue*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_KEYVALUE);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, // search for null termination in szQuery to get length
			&pConn->pstQueryKeyValue,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryKeyValueWithType*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_KEYVALUE_WITHTYPE);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, // search for null termination in szQuery to get length
			&pConn->pstQueryKeyValueWithType,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryKeyValueWithWrongType*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_KEYVALUE_WITHWRONGTYPE);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, // search for null termination in szQuery to get length
			&pConn->pstQueryKeyValueWithWrongType,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryValues*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_VALUES);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, // search for null termination in szQuery to get length
			&pConn->pstQueryValues,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstDeleteKeyValue*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_DELETE_KEYVALUE);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstDeleteKeyValue,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstDeleteAcl*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_DELETE_ACL);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstDeleteAcl,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstUpdateRegValue*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_UPDATE_REG_VALUE);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstUpdateRegValue,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryKeyAclIndex*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_KEY_ACL_INDEX);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstQueryKeyAclIndex,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryKeyAclIndexByKeyId*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_KEY_ACL_INDEX_BY_KEYID);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstQueryKeyAclIndexByKeyId,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstUpdateKeyAclIndexByKeyId*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_UPDATE_KEY_ACL_INDEX_BY_KEYID);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstUpdateKeyAclIndexByKeyId,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryKeyAcl*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_KEY_ACL);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstQueryKeyAcl,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);



    /*pstQueryTotalAclCount*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_TOTAL_ACL_COUNT);
    BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstQueryTotalAclCount,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryAclByOffset*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_KEY_ACL_BY_OFFSET);
    BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, // search for null termination in szQuery to get length
            &pConn->pstQueryAclByOffset,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);

    *phDb = pConn;

cleanup:

    if (pszError != NULL)
    {
        sqlite3_free(pszError);
    }
    LWREG_SAFE_FREE_STRING(pszDbDir);
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);

    return status;

error:
    if (pConn != NULL)
    {
        if (bLockCreated)
        {
            pthread_rwlock_destroy(&pConn->lock);
        }

        if (pConn->pDb != NULL)
        {
            sqlite3_close(pConn->pDb);
        }
        LWREG_SAFE_FREE_MEMORY(pConn);
    }
    *phDb = (HANDLE)NULL;

    goto cleanup;
}

NTSTATUS
RegDbStoreRegKeys(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_KEY* ppKeys
    )
{
    NTSTATUS status = 0;
    sqlite3_stmt *pstCreateKey = NULL;
    sqlite3_stmt *pstCreateKeyAcl = NULL;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDB;
    int iColumnPos = 1;
    // Do not free pEntry
    PREG_DB_KEY pEntry = NULL;
    DWORD dwIndex = 0;
    PSTR pszError = NULL;
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
		        pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);


    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++)
    {
	pEntry = ppKeys[dwIndex];

	if (pEntry == NULL)
		{
			continue;
		}

        // Creating a new key qwDbId should be -1
        if (pEntry->version.qwDbId != -1)
        {
		REG_LOG_DEBUG("Registry::sqldb.c RegDbStoreRegKeys() qwDbId is -1\n");
		continue;
        }

        status = RegDbGetKeyAclIndexByKeyAcl_inlock(
			                            hDB,
		                                pEntry->pSecDescRel,
		                                pEntry->ulSecDescLength,
		                                &pEntry->qwAclIndex);
        BAIL_ON_NT_STATUS(status);

		if (pEntry->qwAclIndex == -1)
		{
			pstCreateKeyAcl = pConn->pstCreateRegAcl;

			status = RegSqliteBindBlob(
					   pstCreateKeyAcl,
					   1,
					   (BYTE*)pEntry->pSecDescRel,
					   pEntry->ulSecDescLength);
			BAIL_ON_SQLITE3_ERROR_STMT(status, pstCreateKeyAcl);

			status = (DWORD)sqlite3_step(pstCreateKeyAcl);
			if (status == SQLITE_DONE)
			{
				status = 0;
			}
			BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

			pEntry->qwAclIndex = sqlite3_last_insert_rowid(pConn->pDb);

			status = sqlite3_reset(pstCreateKeyAcl);
			BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
		}

		pstCreateKey = pConn->pstCreateRegKey;

		if (!bGotNow)
		{
			status = RegGetCurrentTimeSeconds(&now);
			BAIL_ON_NT_STATUS(status);

			bGotNow = TRUE;
		}

		iColumnPos = 1;

		status = RegSqliteBindInt64(
					pstCreateKey,
					iColumnPos,
					pEntry->qwParentId);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = RegSqliteBindStringW(
					pstCreateKey,
					iColumnPos,
					pEntry->pwszKeyName);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = RegSqliteBindInt64(
					pstCreateKey,
					iColumnPos,
					pEntry->qwAclIndex);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = RegSqliteBindInt64(
					pstCreateKey,
					iColumnPos,
					now);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = (DWORD)sqlite3_step(pstCreateKey);
		if (status == SQLITE_DONE)
		{
			status = 0;
		}
		BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

	    status = sqlite3_reset(pstCreateKey);
	    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    }

    status = sqlite3_exec(
		        pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbStoreRegKeys() finished\n");

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

 error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
				 "rollback",
				 NULL,
				 NULL,
				 NULL);

    goto cleanup;
}

NTSTATUS
RegDbStoreRegValues(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE* ppValues
    )
{
    NTSTATUS status = 0;
    sqlite3_stmt *pstQueryEntry = NULL;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDB;
    int iColumnPos = 1;
    PREG_DB_VALUE pEntry = NULL;
    DWORD dwIndex = 0;
    PSTR pszError = NULL;
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
		        pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);


    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++)
    {
	pEntry = ppValues[dwIndex];

	if (pEntry == NULL)
		{
			continue;
		}

        pstQueryEntry = pConn->pstCreateRegValue;

		if (!bGotNow)
		{
			status = RegGetCurrentTimeSeconds(&now);
			BAIL_ON_NT_STATUS(status);

			bGotNow = TRUE;
		}

		iColumnPos = 1;

		status = sqlite3_reset(pstQueryEntry);
		BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

		status = RegSqliteBindInt64(
					pstQueryEntry,
					iColumnPos,
					pEntry->qwParentId);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = RegSqliteBindStringW(
					pstQueryEntry,
					iColumnPos,
					pEntry->pwszValueName);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = RegSqliteBindInt32(
					pstQueryEntry,
					iColumnPos,
					pEntry->type);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = RegSqliteBindBlob(
				   pstQueryEntry,
				   iColumnPos,
				   pEntry->pValue,
				   pEntry->dwValueLen);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = RegSqliteBindInt64(
					pstQueryEntry,
					iColumnPos,
					now);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = (DWORD)sqlite3_step(pstQueryEntry);
		if (status == SQLITE_DONE)
		{
			status = 0;
		}
        BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);
    }

    status = sqlite3_exec(
		        pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbStoreERegValues() finished\n");

    status = sqlite3_reset(pstQueryEntry);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

 error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
				 "rollback",
				 NULL,
				 NULL,
				 NULL);

    goto cleanup;
}

NTSTATUS
RegDbUpdateRegValues(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_DB_VALUE* ppValues
    )
{
    NTSTATUS status = 0;
    sqlite3_stmt *pstQueryEntry = NULL;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDB;
    int iColumnPos = 1;
    PREG_DB_VALUE pEntry = NULL;
    DWORD dwIndex = 0;
    PSTR pszError = NULL;
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
		        pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);


    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++)
    {
	pEntry = ppValues[dwIndex];

	if (pEntry == NULL)
		{
			continue;
		}

	pstQueryEntry = pConn->pstUpdateRegValue;

		if (!bGotNow)
		{
			status = RegGetCurrentTimeSeconds(&now);
			BAIL_ON_NT_STATUS(status);

			bGotNow = TRUE;
		}

		iColumnPos = 1;

		status = sqlite3_reset(pstQueryEntry);
		BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

		status = RegSqliteBindInt64(
					pstQueryEntry,
					iColumnPos,
					now);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = RegSqliteBindBlob(
				   pstQueryEntry,
				   iColumnPos,
				   pEntry->pValue,
				   pEntry->dwValueLen);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

        status = RegSqliteBindInt64(
			pstQueryEntry,
                    iColumnPos,
                    pEntry->qwParentId);
        BAIL_ON_NT_STATUS(status);
        iColumnPos++;

		status = RegSqliteBindStringW(
					pstQueryEntry,
					iColumnPos,
					pEntry->pwszValueName);
		BAIL_ON_NT_STATUS(status);
		iColumnPos++;

		status = (DWORD)sqlite3_step(pstQueryEntry);
		if (status == SQLITE_DONE)
		{
			status = 0;
		}
        BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);
    }

    status = sqlite3_exec(
		        pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbStoreEntries() finished\n");

    status = sqlite3_reset(pstQueryEntry);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

 error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
				 "rollback",
				 NULL,
				 NULL,
				 NULL);

    goto cleanup;
}

NTSTATUS
RegDbOpenKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszFullKeyPath,
    OUT OPTIONAL PREG_DB_KEY* ppRegKey
    )
{
    NTSTATUS status = 0;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    PSTR pszError = NULL;
    BOOLEAN bInLock = FALSE;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
		        pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    status = RegDbOpenKey_inlock(hDb,
		                     pwszFullKeyPath,
		                     ppRegKey);
    BAIL_ON_NT_STATUS(status);


    status = sqlite3_exec(
		        pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbOpenKey() finished\n");

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
				 "rollback",
				 NULL,
				 NULL,
				 NULL);

    goto cleanup;
}

NTSTATUS
RegDbCreateKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszFullKeyName,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel,
    IN ULONG ulSecDescLength,
    OUT PREG_DB_KEY* ppRegKey
    )
{
	NTSTATUS status = STATUS_SUCCESS;
	PREG_DB_KEY pRegKey = NULL;
	PREG_DB_KEY pRegKeyFull = NULL;
	PREG_DB_KEY pRegParentKey = NULL;
    PWSTR pwszParentKeyName = NULL;
    PCWSTR pwszKeyName = RegStrrchr(pwszFullKeyName, '\\');
    BOOLEAN bInLock = FALSE;

    if (pwszKeyName)
    {
        status = SqliteGetParentKeyName(pwszFullKeyName, '\\', &pwszParentKeyName);
        BAIL_ON_NT_STATUS(status);

	status = RegDbOpenKey(hDb, pwszParentKeyName, &pRegParentKey);
	BAIL_ON_NT_STATUS(status);
    }

    /*Create key*/
    status = LW_RTL_ALLOCATE((PVOID*)&pRegKey, REG_DB_KEY, sizeof(*pRegKey));
    BAIL_ON_NT_STATUS(status);

    memset(pRegKey, 0, sizeof(*pRegKey));

    status = LwRtlWC16StringDuplicate(&pRegKey->pwszKeyName,
		                          pwszKeyName ? pwszKeyName+1 :  pwszFullKeyName);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringDuplicate(&pRegKey->pwszFullKeyName, pwszFullKeyName);
    BAIL_ON_NT_STATUS(status);

    pRegKey->qwParentId = pwszKeyName ? pRegParentKey->version.qwDbId : 0;
    pRegKey->version.qwDbId = -1;
    pRegKey->qwAclIndex = -1;

    status = LW_RTL_ALLOCATE((PVOID*)&pRegKey->pSecDescRel, VOID, ulSecDescLength);
    BAIL_ON_NT_STATUS(status);

    memcpy(pRegKey->pSecDescRel, pSecDescRel, ulSecDescLength);
    pRegKey->ulSecDescLength = ulSecDescLength;

    status = RegDbStoreRegKeys(
                 hDb,
                 1,
                 &pRegKey);
    BAIL_ON_NT_STATUS(status);

	status = RegDbOpenKey(hDb, pRegKey->pwszFullKeyName, &pRegKeyFull);
	BAIL_ON_NT_STATUS(status);

	pRegKey->qwAclIndex = pRegKeyFull->qwAclIndex;
	pRegKey->qwParentId = pRegKeyFull->qwParentId;
	pRegKey->version.qwDbId = pRegKeyFull->version.qwDbId;

    LWREG_LOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

	status = SqliteCacheInsertDbKeyInfo_inlock(pRegKey);
	BAIL_ON_NT_STATUS(status);
	pRegKey = NULL;

    *ppRegKey = pRegKeyFull;

cleanup:
    SqliteReleaseDbKeyInfo_inlock(pRegKey);
    LWREG_UNLOCK_MUTEX(bInLock, &gRegDbKeyList.mutex);

    LWREG_SAFE_FREE_MEMORY(pwszParentKeyName);
    RegDbSafeFreeEntryKey(&pRegParentKey);

    return status;

error:
    RegDbSafeFreeEntryKey(&pRegKeyFull);
    *ppRegKey = NULL;

    goto cleanup;
}

NTSTATUS
RegDbCreateKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    )
{
	NTSTATUS status = STATUS_SUCCESS;
	PREG_DB_VALUE pRegEntry = NULL;

    status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_DB_VALUE, sizeof(*pRegEntry));
    BAIL_ON_NT_STATUS(status);

    memset(pRegEntry, 0, sizeof(*pRegEntry));

    status = LwRtlWC16StringDuplicate(&pRegEntry->pwszValueName, pwszValueName);
    BAIL_ON_NT_STATUS(status);

    if (dwValueLen)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry->pValue, BYTE, sizeof(*pRegEntry->pValue)*dwValueLen);
        BAIL_ON_NT_STATUS(status);

        memset(pRegEntry->pValue, 0, sizeof(*pRegEntry->pValue)*dwValueLen);

        memcpy(pRegEntry->pValue, pValue, dwValueLen);
    }

    pRegEntry->dwValueLen = dwValueLen;
    pRegEntry->type = valueType;
    pRegEntry->qwParentId = qwParentKeyId;

    status = RegDbStoreRegValues(
                 hDb,
                 1,
                 &pRegEntry);
    BAIL_ON_NT_STATUS(status);

    if (ppRegEntry)
    {
        *ppRegEntry = pRegEntry;
    }

cleanup:
    if (!ppRegEntry)
    {
        RegDbSafeFreeEntryValue(&pRegEntry);
    }

    return status;

error:
    RegDbSafeFreeEntryValue(&pRegEntry);
    *ppRegEntry = NULL;

    goto cleanup;
}

NTSTATUS
RegDbSetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN const PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    )
{
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN bIsWrongType = FALSE;
    PREG_DB_VALUE pRegEntry = NULL;

    status = RegDbGetKeyValue(hDb,
		                  qwParentKeyId,
		                  pwszValueName,
		                  valueType,
		                  &bIsWrongType,
		                  &pRegEntry);
    if (!status)
    {
	LWREG_SAFE_FREE_MEMORY(pRegEntry->pValue);
	pRegEntry->dwValueLen = 0;

        if (dwValueLen)
        {
            status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry->pValue, BYTE, sizeof(*pRegEntry->pValue)*dwValueLen);
            BAIL_ON_NT_STATUS(status);

            memcpy(pRegEntry->pValue, pValue, dwValueLen);
        }

        pRegEntry->dwValueLen = dwValueLen;

	status = RegDbUpdateRegValues(
                     hDb,
                     1,
                     &pRegEntry);
        BAIL_ON_NT_STATUS(status);
    }
    if (STATUS_OBJECT_NAME_NOT_FOUND == status && !pRegEntry)
    {
	status = RegDbCreateKeyValue(
			    hDb,
			    qwParentKeyId,
			    pwszValueName,
			    pValue,
			    dwValueLen,
			    valueType,
			    &pRegEntry);
	BAIL_ON_NT_STATUS(status);
    }
    BAIL_ON_NT_STATUS(status);

    if (ppRegEntry)
    {
        *ppRegEntry = pRegEntry;
    }

cleanup:
    if (!ppRegEntry)
    {
        RegDbSafeFreeEntryValue(&pRegEntry);
    }

    return status;

error:
    RegDbSafeFreeEntryValue(&pRegEntry);
    if (ppRegEntry)
    {
	*ppRegEntry = NULL;
    }

    goto cleanup;
}

NTSTATUS
RegDbGetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_DB_VALUE* ppRegEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    const int nExpectedCols = 5;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_DB_VALUE pRegEntry = NULL;

    BAIL_ON_NT_INVALID_STRING(pwszValueName);

    if (qwParentKeyId <= 0)
    {
	status = STATUS_INTERNAL_ERROR;
	BAIL_ON_NT_STATUS(status);
    }


    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    if (valueType == REG_UNKNOWN)
    {
        pstQuery = pConn->pstQueryKeyValue;

        status = (NTSTATUS)RegSqliteBindStringW(pstQuery, 1, pwszValueName);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = (NTSTATUS)RegSqliteBindInt64(pstQuery, 2, qwParentKeyId);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);
    }
    else
    {
        if (pbIsWrongType && !*pbIsWrongType)
        {
            pstQuery = pConn->pstQueryKeyValueWithType;
        }
        else
        {
            pstQuery = pConn->pstQueryKeyValueWithWrongType;
        }

        status = RegSqliteBindStringW(pstQuery, 1, pwszValueName);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = RegSqliteBindInt64(pstQuery, 2, qwParentKeyId);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = RegSqliteBindInt32(pstQuery, 3, (int)valueType);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);
    }

    while ((status = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
		status = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        if (sResultCount >= 1)
        {
            // Duplicate key value records are found
		status = STATUS_DUPLICATE_NAME;
            BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_DB_VALUE, sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackRegValueInfo(pstQuery,
                                         &iColumnPos,
                                         pRegEntry);
        BAIL_ON_NT_STATUS(status);

        sResultCount++;
    }

    if (status == SQLITE_DONE)
    {
        // No more results found
	status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    if (!sResultCount)
    {
	status = STATUS_OBJECT_NAME_NOT_FOUND;
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (!status && ppRegEntry)
    {
        *ppRegEntry = pRegEntry;
    }
    else
    {
        RegDbSafeFreeEntryValue(&pRegEntry);
        if (ppRegEntry)
            *ppRegEntry = NULL;
    }

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);


    return status;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

NTSTATUS
RegDbDeleteKey(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN int64_t qwAclId,
    IN PCWSTR pwszFullKeyName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
	PSTR pszError = NULL;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;


    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
		        pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    status = RegDbDeleteKey_inlock(hDb,
		                       qwId,
		                       qwAclId,
		                       pwszFullKeyName);
    BAIL_ON_NT_STATUS(status);

    status = sqlite3_exec(
		        pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbDeleteKey() finished\n");

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

 error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
				 "rollback",
				 NULL,
				 NULL,
				 NULL);

    goto cleanup;
}

NTSTATUS
RegDbQueryInfoKeyCount(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN QueryKeyInfoOption queryType,
    OUT size_t* psCount
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;

    if (qwId <= 0)
    {
	status = STATUS_INTERNAL_ERROR;
	BAIL_ON_NT_STATUS(status);
    }

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = RegDbQueryInfoKeyCount_inlock(hDb,
		                               qwId,
		                               queryType,
		                               psCount);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

error:

    *psCount = 0;

    goto cleanup;
}

NTSTATUS
RegDbQueryInfoKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_KEY** pppRegEntries
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;

    if (qwId <= 0)
    {
	status = STATUS_INTERNAL_ERROR;
	BAIL_ON_NT_STATUS(status);
    }

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = RegDbQueryInfoKey_inlock(hDb,
		                          pwszKeyName,
		                          qwId,
		                          dwLimit,
		                          dwOffset,
		                          psCount,
		                          pppRegEntries);
    BAIL_ON_NT_STATUS(status);

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

error:

    goto cleanup;
}

NTSTATUS
RegDbQueryInfoKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwId,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_DB_VALUE** pppRegEntries
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    size_t sResultCapacity = 0;
    const int nExpectedCols = 5;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_DB_VALUE pRegEntry = NULL;
    PREG_DB_VALUE* ppRegEntries = NULL;

    if (qwId <= 0)
    {
	status = STATUS_INTERNAL_ERROR;
	BAIL_ON_NT_STATUS(status);
    }

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pstQuery = pConn->pstQueryValues;

    status = RegSqliteBindInt64(pstQuery, 1, qwId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindInt64(pstQuery, 2, dwLimit);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindInt64(pstQuery, 3, dwOffset);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    while ((status = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            status = STATUS_DATA_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        if (sResultCount >= sResultCapacity)
        {
            sResultCapacity *= 2;
            sResultCapacity += 10;
            status = NtRegReallocMemory(
                            ppRegEntries,
                            (PVOID*)&ppRegEntries,
                            sizeof(*ppRegEntries) * sResultCapacity);
            BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_DB_KEY, sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackRegValueInfo(pstQuery,
                                         &iColumnPos,
                                         pRegEntry);
        BAIL_ON_NT_STATUS(status);

        ppRegEntries[sResultCount] = pRegEntry;
        pRegEntry = NULL;
        sResultCount++;
    }

    if (status == SQLITE_DONE)
    {
        // No more results found
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

cleanup:
    if (!status)
    {
        if (pppRegEntries)
        {
            *pppRegEntries = ppRegEntries;
        }
        *psCount = sResultCount;
    }
    else
    {
        RegDbSafeFreeEntryValue(&pRegEntry);
        RegDbSafeFreeEntryValueList(sResultCount, &ppRegEntries);
        if (pppRegEntries)
        {
            *pppRegEntries = NULL;
        }
        *psCount = 0;
    }

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);


    return status;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

NTSTATUS
RegDbDeleteKeyValue(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwParentKeyId,
    IN PCWSTR pwszValueName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteKeyValue;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = RegSqliteBindInt64(pstQuery, 1, qwParentKeyId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindStringW(pstQuery, 2, pwszValueName);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_step(pstQuery);
    if (status == SQLITE_DONE)
    {
        status = STATUS_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

error:

    if (pstQuery)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

NTSTATUS
RegDbUpdateKeyAcl(
	IN REG_DB_HANDLE hDb,
	IN PCWSTR pwszFullKeyPath,
	IN int64_t qwKeyDbId,
	IN int64_t qwKeyCurrSdId,
	IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelToSet,
	IN ULONG ulSecDescToSetLen
	)
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    PSTR pszError = NULL;
    int64_t qwAclIndex = -1;
    // Do not free
    sqlite3_stmt *pstCreateKeyAcl = pConn->pstCreateRegAcl;
    size_t sCount = 0;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
		        pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

	// Check whether the current ACL still has reference to it in the DB
    // If not, delete this ACL

    status = RegDbQueryAclRefCountWOCurrKey_inlock(hDb, qwKeyCurrSdId,
		                                       qwKeyDbId, &sCount);
    BAIL_ON_NT_STATUS(status);

    if (!sCount)
    {
	// Remove this ACL
	status = RegDbDeleteAcl_inlock(hDb, qwKeyCurrSdId);
	BAIL_ON_NT_STATUS(status);
    }

    status = RegDbGetKeyAclIndexByKeyAcl_inlock(
		                            hDb,
		                            pSecDescRelToSet,
		                            ulSecDescToSetLen,
	                                &qwAclIndex);
    BAIL_ON_NT_STATUS(status);

	if (qwAclIndex == -1)
	{
		status = RegSqliteBindBlob(
				   pstCreateKeyAcl,
				   1,
				   (BYTE*)pSecDescRelToSet,
				   ulSecDescToSetLen);
		BAIL_ON_SQLITE3_ERROR_STMT(status, pstCreateKeyAcl);

		status = (DWORD)sqlite3_step(pstCreateKeyAcl);
		if (status == SQLITE_DONE)
		{
			status = 0;
		}
		BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

		status = sqlite3_reset(pstCreateKeyAcl);
		BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

		qwAclIndex = sqlite3_last_insert_rowid(pConn->pDb);
	}

	// Update key's AclIndex with qwAclIndex
	if (qwKeyCurrSdId != qwAclIndex)
	{
	    status = RegDbUpdateKeyAclIndex_inlock(hDb, qwKeyDbId, qwAclIndex);
	    BAIL_ON_NT_STATUS(status);
	}

    status = sqlite3_exec(
		        pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbUpdateKeyAcl() finished\n");

    SqliteCacheDeleteDbKeyInfo(pwszFullKeyPath);

cleanup:

	LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

	return status;

 error:

	if (pszError)
	{
		sqlite3_free(pszError);
	}
	sqlite3_exec(pConn->pDb,
				 "rollback",
				 NULL,
				 NULL,
				 NULL);

	goto cleanup;
}

NTSTATUS
RegDbGetKeyAclByKeyId(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwKeyDbId,
    OUT int64_t *pqwKeyAclId,
    OUT PSECURITY_DESCRIPTOR_RELATIVE* ppSecDescRel,
    OUT PULONG pSecDescLen
    )
{
	NTSTATUS status = 0;
	PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
	PSTR pszError = NULL;
	BOOLEAN bInLock = FALSE;
	int64_t qwKeyAclId = -1;

	ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

	status = sqlite3_exec(
					pConn->pDb,
					"begin;",
					NULL,
					NULL,
					&pszError);
	BAIL_ON_SQLITE3_ERROR(status, pszError);

	status = RegDbGetKeyAclIndexByKeyId_inlock(hDb, qwKeyDbId, &qwKeyAclId);
	BAIL_ON_NT_STATUS(status);

	if (qwKeyAclId != -1)
	{
        status = RegDbGetKeyAclByAclIndex_inlock(hDb,
	                                         qwKeyAclId,
		                                     ppSecDescRel,
		                                     pSecDescLen);
        BAIL_ON_NT_STATUS(status);
	}

    status = sqlite3_exec(
		        pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbOpenKey() finished\n");

    *pqwKeyAclId = qwKeyAclId;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
				 "rollback",
				 NULL,
				 NULL,
				 NULL);

    goto cleanup;
}


NTSTATUS
RegDbFixAcls(
    IN REG_DB_HANDLE hDb
    )
{
    NTSTATUS status = 0;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    PSTR pszError = NULL;
    BOOLEAN bInLock = FALSE;
    size_t sAclCount = 0;
    int iAclIndex = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRel = NULL;
    ULONG ulSecDescRelLen = 0;
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecDescAbs = NULL;
    ULONG ulSecDescAbsLen = 0;
    PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelNew = NULL;
    ULONG ulSecDescRelNewLen = 1024;
    PACL pDacl = NULL;
    ULONG ulDaclLen = 0;
    PACL pSacl = NULL;
    ULONG ulSaclLen = 0;
    PSID pOwner = NULL;
    ULONG ulOwnerLen = 0;
    PSID pGroup = NULL;
    ULONG ulGroupLen = 0;
    PSID pRegGroup = NULL;
    BOOLEAN bIsGroupDefaulted = FALSE;
    PSID pGroupSid = NULL;


    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
                    pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    status = RegDbQueryTotalAclCount_inlock(hDb, &sAclCount);
    BAIL_ON_NT_STATUS(status);

    for (iAclIndex = 0; iAclIndex < sAclCount; iAclIndex++)
    {
        int64_t qwCacheId = -1;

        status = RegDbGetKeyAclByAclOffset_inlock(hDb,
                                                 iAclIndex,
                                                 &qwCacheId,
                                                 &pSecDescRel,
                                                 &ulSecDescRelLen);
        BAIL_ON_NT_STATUS(status);

        // Get sizes
        status = RtlSelfRelativeToAbsoluteSD(pSecDescRel,
                                             pSecDescAbs, &ulSecDescAbsLen,
                                             pDacl, &ulDaclLen,
                                             pSacl, &ulSaclLen,
                                             pOwner, &ulOwnerLen,
                                             pGroup, &ulGroupLen);
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            status = STATUS_SUCCESS;
        }
        BAIL_ON_NT_STATUS(status);

        status = LW_RTL_ALLOCATE(&pSecDescAbs, VOID, ulSecDescAbsLen);
        BAIL_ON_NT_STATUS(status);

        if (ulOwnerLen)
        {
            status = LW_RTL_ALLOCATE(&pOwner, SID, ulOwnerLen);
            BAIL_ON_NT_STATUS(status);
        }

        if (ulGroupLen)
        {
            status = LW_RTL_ALLOCATE(&pGroup, SID, ulGroupLen);
            BAIL_ON_NT_STATUS(status);
        }

        if (ulDaclLen)
        {
            status = LW_RTL_ALLOCATE(&pDacl, VOID, ulDaclLen);
            BAIL_ON_NT_STATUS(status);
        }

        if (ulSaclLen)
        {
            status = LW_RTL_ALLOCATE(&pSacl, VOID, ulSaclLen);
            BAIL_ON_NT_STATUS(status);
        }

        status = RtlSelfRelativeToAbsoluteSD(pSecDescRel,
                                             pSecDescAbs, &ulSecDescAbsLen,
                                             pDacl, &ulDaclLen,
                                             pSacl, &ulSaclLen,
                                             pOwner, &ulOwnerLen,
                                             pGroup, &ulGroupLen);
        BAIL_ON_NT_STATUS(status);

        // Check whether group part is set, if NOT set, set it
        status = RtlGetGroupSecurityDescriptor(pSecDescAbs,
                                               &pRegGroup,
                                               &bIsGroupDefaulted);
        BAIL_ON_NT_STATUS(status);

        if (!pRegGroup)
        {

            status = RtlAllocateSidFromCString(&pGroupSid, "S-1-5-32-544");
            BAIL_ON_NT_STATUS(status);

            status = RtlSetGroupSecurityDescriptor(
                         pSecDescAbs,
                         pGroupSid,
                         FALSE);
            BAIL_ON_NT_STATUS(status);

            // convert absolute back to relative before write to registry
            do
            {
                status = NtRegReallocMemory(pSecDescRelNew,
                                            (PVOID*)&pSecDescRelNew,
                                            ulSecDescRelNewLen);
                BAIL_ON_NT_STATUS(status);

                memset(pSecDescRelNew, 0, ulSecDescRelNewLen);

                status = RtlAbsoluteToSelfRelativeSD(pSecDescAbs,
                                                     pSecDescRelNew,
                                                     &ulSecDescRelNewLen);
                if (STATUS_BUFFER_TOO_SMALL  == status)
                {
                    ulSecDescRelNewLen *= 2;
                }
                else
                {
                    BAIL_ON_NT_STATUS(status);
                }
            }
            while((status != STATUS_SUCCESS) &&
                  (ulSecDescRelNewLen <= SECURITY_DESCRIPTOR_RELATIVE_MAX_SIZE));

            // Write the fixed ACL back to registry
            status = RegDbUpdateKeyAclContent_inlock(hDb,
                                                     qwCacheId,
                                                     pSecDescRelNew,
                                                     ulSecDescRelNewLen);
            BAIL_ON_NT_STATUS(status);
        }
        BAIL_ON_NT_STATUS(status);

        LWREG_SAFE_FREE_MEMORY(pSecDescRel);
        ulSecDescRelLen = 0;
        LWREG_SAFE_FREE_MEMORY(pSecDescRelNew);
        ulSecDescRelNewLen = 1024;
        RegSrvFreeAbsoluteSecurityDescriptor(&pSecDescAbs);
        ulSecDescAbsLen = 0;
    }

    status = sqlite3_exec(
                    pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbFixAcls() finished\n");


cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    LWREG_SAFE_FREE_MEMORY(pSecDescRel);
    LWREG_SAFE_FREE_MEMORY(pSecDescRelNew);
    RegSrvFreeAbsoluteSecurityDescriptor(&pSecDescAbs);

    return status;

error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
                 "rollback",
                 NULL,
                 NULL,
                 NULL);

    goto cleanup;
}


static
NTSTATUS
RegDbFreePreparedStatements(
    IN OUT PREG_DB_CONNECTION pConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int i;
    sqlite3_stmt * * const pppstFreeList[] = {

        &pConn->pstCreateRegKey,
        &pConn->pstCreateRegValue,
        &pConn->pstCreateRegAcl,
        &pConn->pstUpdateRegValue,
        &pConn->pstQueryKeyAclIndex,
        &pConn->pstQueryKeyAcl,
        &pConn->pstQueryKeyAclIndexByKeyId,
        &pConn->pstUpdateKeyAclIndexByKeyId,
        &pConn->pstOpenKeyEx,
        &pConn->pstDeleteKey,
        &pConn->pstDeleteAllKeyValues,
        &pConn->pstDeleteKeyValue,
        &pConn->pstDeleteAcl,
        &pConn->pstQuerySubKeys,
        &pConn->pstQuerySubKeysCount,
        &pConn->pstQueryValues,
        &pConn->pstQueryValuesCount,
        &pConn->pstQueryKeyValue,
        &pConn->pstQueryKeyValueWithType,
        &pConn->pstQueryKeyValueWithWrongType,
        &pConn->pstQueryMultiKeyValues,
        &pConn->pstQueryAclRefCount,
        &pConn->pstQueryTotalAclCount,
        &pConn->pstQueryAclByOffset,
        &pConn->pstUpdateRegAclByCacheId
    };

    for (i = 0; i < sizeof(pppstFreeList)/sizeof(pppstFreeList[0]); i++)
    {
        if (*pppstFreeList[i] != NULL)
        {
            status = sqlite3_finalize(*pppstFreeList[i]);
            BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
            *pppstFreeList[i] = NULL;
        }
    }

cleanup:
    return status;

error:
    goto cleanup;
}

void
RegDbSafeClose(
    PREG_DB_HANDLE phDb
    )
{
    // This function cannot return an error, only log errors that occur
    // along the way
    DWORD status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = NULL;

    if (phDb == NULL)
    {
        goto cleanup;
    }

    pConn = (PREG_DB_CONNECTION)*phDb;

    if (pConn == NULL)
    {
        goto cleanup;
    }

    status = RegDbFreePreparedStatements(pConn);
    if (status != STATUS_SUCCESS)
    {
        REG_LOG_ERROR("Error freeing prepared statements [%d]", status);
        status = STATUS_SUCCESS;
    }

    if (pConn->pDb != NULL)
    {
        sqlite3_close(pConn->pDb);
        pConn->pDb = NULL;
    }

    status = pthread_rwlock_destroy(&pConn->lock);
    if (status != STATUS_SUCCESS)
    {
        REG_LOG_ERROR("Error destroying lock [%d]", status);
        status = STATUS_SUCCESS;
    }
    LWREG_SAFE_FREE_MEMORY(pConn);

    *phDb = (HANDLE)0;

cleanup:
    return;
}

NTSTATUS
RegDbEmptyCache(
    IN REG_DB_HANDLE hDb
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    PCSTR pszEmptyCache =
        "begin;\n"
        "delete from " REG_DB_TABLE_NAME_KEYS ";\n"
        "delete from " REG_DB_TABLE_NAME_VALUES ";\n"
        "delete from " REG_DB_TABLE_NAME_ACLS ";\n"
        "end";

    status = RegSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
        pszEmptyCache);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RegDbFlushNOP(
    REG_DB_HANDLE hDb
    )
{
    return 0;
}

static
NTSTATUS
RegDbUpdateKeyAclContent_inlock(
    IN REG_DB_HANDLE hDb,
    IN int64_t qwAclDbId,
    IN PSECURITY_DESCRIPTOR_RELATIVE pSecDescRelToSet,
    IN ULONG ulSecDescToSetLen
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    PSTR pszError = NULL;
    // Do not free
    sqlite3_stmt *pstUpdateKeyAcl = pConn->pstUpdateRegAclByCacheId;


    if (qwAclDbId < 0)
    {
        status = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(status);
    }

    status = RegSqliteBindBlob(
               pstUpdateKeyAcl,
               1,
               (BYTE*)pSecDescRelToSet,
               ulSecDescToSetLen);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstUpdateKeyAcl);

    status = RegSqliteBindInt64(pstUpdateKeyAcl, 2, qwAclDbId);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstUpdateKeyAcl);

    status = (DWORD)sqlite3_step(pstUpdateKeyAcl);
    if (status == SQLITE_DONE)
    {
        status = 0;
    }
    BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

    status = sqlite3_reset(pstUpdateKeyAcl);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

cleanup:

    return status;

 error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }

    goto cleanup;
}



#if 0
// Obsolete code, leave for now as future reference
NTSTATUS
RegDbStoreEntries(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_ENTRY* ppEntries,
    IN OPTIONAL PBOOLEAN pbIsUpdate
    )
{
    NTSTATUS status = 0;
    sqlite3_stmt *pstQueryCacheId = NULL;
    sqlite3_stmt *pstQueryEntry = NULL;
    sqlite3_stmt *pstQueryCacheEntry = NULL;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDB;
    int iColumnPos = 1;
    PREG_ENTRY pEntry = NULL;
    DWORD dwIndex = 0;
    PSTR pszError = NULL;
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;
    BOOLEAN bInLock = FALSE;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = sqlite3_exec(
		        pConn->pDb,
                    "begin;",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);


    for (dwIndex = 0; dwIndex < dwEntryCount; dwIndex++)
    {
	pEntry = ppEntries[dwIndex];

	if (pEntry == NULL)
		{
			continue;
		}

		if (ppEntries[dwIndex]->version.qwDbId == -1)
		{
		    if (pbIsUpdate && *pbIsUpdate)
		    {
			pstQueryEntry = pConn->pstReplaceRegEntry;
			pstQueryCacheEntry = pConn->pstDeleteCacheIdEntry;
		    }
		    else
		    {
			pstQueryEntry = pConn->pstCreateRegEntry;
		    }
			pstQueryCacheId = pConn->pstCreateCacheId;

			if (!bGotNow)
			{
				status = RegGetCurrentTimeSeconds(&now);
				BAIL_ON_NT_STATUS(status);

				bGotNow = TRUE;
			}

			if (pbIsUpdate && *pbIsUpdate)
			{
			    status = sqlite3_reset(pstQueryCacheEntry);
			    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

			    iColumnPos = 1;
	            status = RegSqliteBindStringW(
				    pstQueryCacheEntry,
	                        iColumnPos,
	                        pEntry->pwszKeyName);
	            BAIL_ON_NT_STATUS(status);
	            iColumnPos++;

	            status = RegSqliteBindBlob(
				   pstQueryCacheEntry,
						   iColumnPos,
						   pEntry->pValue,
						   pEntry->dwValueLen);
			    BAIL_ON_NT_STATUS(status);
	            iColumnPos++;

				status = (DWORD)sqlite3_step(pstQueryCacheEntry);
				if (status == SQLITE_DONE)
				{
					status = 0;
				}
				BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);
			}

		    status = sqlite3_reset(pstQueryCacheId);
		    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

			iColumnPos = 1;
			status = RegSqliteBindInt64(
						pstQueryCacheId,
						iColumnPos,
						now);
			BAIL_ON_NT_STATUS(status);
			iColumnPos++;

			status = (DWORD)sqlite3_step(pstQueryCacheId);
			if (status == SQLITE_DONE)
			{
				status = 0;
			}
			BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);

			iColumnPos = 1;

            status = sqlite3_reset(pstQueryEntry);
            BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

            status = RegSqliteBindInt64(
				pstQueryEntry,
                        iColumnPos,
                        sqlite3_last_insert_rowid(pConn->pDb));
            BAIL_ON_NT_STATUS(status);
            iColumnPos++;

            status = RegSqliteBindStringW(
				pstQueryEntry,
                        iColumnPos,
                        pEntry->pwszKeyName);
            BAIL_ON_NT_STATUS(status);
            iColumnPos++;

            status = RegSqliteBindStringW(
				pstQueryEntry,
                        iColumnPos,
                        pEntry->pwszValueName);
            BAIL_ON_NT_STATUS(status);
            iColumnPos++;

            status = RegSqliteBindInt32(
				pstQueryEntry,
                        iColumnPos,
                        pEntry->type);
            BAIL_ON_NT_STATUS(status);
            iColumnPos++;

            status = RegSqliteBindBlob(
			   pstQueryEntry,
					   iColumnPos,
					   pEntry->pValue,
					   pEntry->dwValueLen);
		    BAIL_ON_NT_STATUS(status);
            iColumnPos++;

            status = (DWORD)sqlite3_step(pstQueryEntry);
            if (status == SQLITE_DONE)
            {
                status = 0;
            }
            BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);
		} // end of if ppEntries[dwIndex]->version.qwDbId == -1
		else
		{
			pstQueryEntry = pConn->pstUpdateRegEntry;
			pstQueryCacheEntry = pConn->pstUpdateCacheIdEntry;

			iColumnPos = 1;

			status = sqlite3_reset(pstQueryCacheEntry);
			BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

			status = RegSqliteBindInt64(
					    pstQueryCacheEntry,
						iColumnPos,
						pEntry->version.tLastUpdated);
			BAIL_ON_NT_STATUS(status);
			iColumnPos++;

            status = RegSqliteBindInt64(
			    pstQueryCacheEntry,
                        iColumnPos,
                        pEntry->version.qwDbId);
            BAIL_ON_NT_STATUS(status);
            iColumnPos++;

			status = (DWORD)sqlite3_step(pstQueryCacheEntry);
			if (status == SQLITE_DONE)
			{
				status = 0;
			}
			BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);


			iColumnPos = 1;

			status = sqlite3_reset(pstQueryEntry);
			BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

            status = RegSqliteBindInt64(
				pstQueryEntry,
                        iColumnPos,
                        pEntry->version.qwDbId);
            BAIL_ON_NT_STATUS(status);
            iColumnPos++;

            status = RegSqliteBindBlob(
			   pstQueryEntry,
					   iColumnPos,
					   pEntry->pValue,
					   pEntry->dwValueLen);
		    BAIL_ON_NT_STATUS(status);
            iColumnPos++;

			status = RegSqliteBindStringW(
						pstQueryEntry,
						iColumnPos,
						pEntry->pwszKeyName);
			BAIL_ON_NT_STATUS(status);
			iColumnPos++;

			status = RegSqliteBindStringW(
						pstQueryEntry,
						iColumnPos,
						pEntry->pwszValueName);
			BAIL_ON_NT_STATUS(status);
			iColumnPos++;

			status = RegSqliteBindInt32(
						pstQueryEntry,
						iColumnPos,
						pEntry->type);
			BAIL_ON_NT_STATUS(status);
			iColumnPos++;

			status = (DWORD)sqlite3_step(pstQueryEntry);
			if (status == SQLITE_DONE)
			{
				status = 0;
			}
			BAIL_ON_SQLITE3_ERROR_DB(status, pConn->pDb);
		}
    }

    status = sqlite3_exec(
		        pConn->pDb,
                    "end",
                    NULL,
                    NULL,
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(status, pszError);

    REG_LOG_VERBOSE("Registry::sqldb.c RegDbStoreEntries() finished\n");

    status = sqlite3_reset(pstQueryEntry);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    status = sqlite3_reset(pstQueryCacheId);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

 error:

    if (pszError)
    {
        sqlite3_free(pszError);
    }
    sqlite3_exec(pConn->pDb,
				 "rollback",
				 NULL,
				 NULL,
				 NULL);

    goto cleanup;
}
#endif
