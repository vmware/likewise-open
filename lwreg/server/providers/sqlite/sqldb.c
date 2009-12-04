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

PCSTR
RegDbGetObjectFieldList(
    VOID
    )
{
    return
        REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
        REG_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
        REG_DB_TABLE_NAME_ENTRIES ".KeyName, "
        REG_DB_TABLE_NAME_ENTRIES ".ValueName, "
        REG_DB_TABLE_NAME_ENTRIES ".Type, "
        REG_DB_TABLE_NAME_ENTRIES ".Value";
}

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
RegDbUnpackRegEntryInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_ENTRY pResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegSqliteReadWC16String(
        pstQuery,
        piColumnPos,
        "KeyName",
        &pResult->pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadWC16String(
        pstQuery,
        piColumnPos,
        "ValueName",
        &pResult->pwszValueName);
    BAIL_ON_NT_STATUS(status);

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

    if (pResult->type != REG_KEY && !pResult->pwszValueName)
    {
	status = STATUS_OBJECT_NAME_INVALID;
        BAIL_ON_NT_STATUS(status);
    }

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


    /*pstCreateCacheId*/
    status = sqlite3_prepare_v2(
		pConn->pDb,
		"insert into " REG_DB_TABLE_NAME_CACHE_TAGS " ("
			"LastUpdated"
			") values (?1)",
			-1,
			&pConn->pstCreateCacheId,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));


    /*pstUpdateCacheIdEntry*/
    status = sqlite3_prepare_v2(
		pConn->pDb,
		"update " REG_DB_TABLE_NAME_CACHE_TAGS " set "
		"LastUpdated = ?1 "
		"where CacheId = ?2",
			-1,
			&pConn->pstUpdateCacheIdEntry,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));


	/*pstCreateRegEntry*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_INSERT_REG_ENTRY);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
		    pConn->pDb,
		    pwszQueryStatement,
				-1,
				&pConn->pstCreateRegEntry,
				NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstDeleteCacheIdEntry*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_DELETE_CACHEID_ENTRY);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
		    pConn->pDb,
		    pwszQueryStatement,
				-1,
				&pConn->pstDeleteCacheIdEntry,
				NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);



    /*pstOpenKeyEx*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_OPEN_KEY_EX);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, //search for null termination in szQuery to get length
            &pConn->pstOpenKeyEx,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstReplaceRegEntry*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_REPLACE_REG_ENTRY);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, //search for null termination in szQuery to get length
            &pConn->pstReplaceRegEntry,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstUpdateRegEntry*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_UPDATE_REG_ENTRY);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, //search for null termination in szQuery to get length
            &pConn->pstUpdateRegEntry,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstDeleteKey (delete the key and all of its associated values)*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_DELETE_KEY);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, //search for null termination in szQuery to get length
            &pConn->pstDeleteKey,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstDeleteKeyValue*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_DELETE_KEYVALUE);
	BAIL_ON_NT_STATUS(status);

    status = sqlite3_prepare16_v2(
            pConn->pDb,
            pwszQueryStatement,
            -1, //search for null termination in szQuery to get length
            &pConn->pstDeleteKeyValue,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
    LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQuerySubKeys*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_SUBKEYS);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, //search for null termination in szQuery to get length
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
			-1, //search for null termination in szQuery to get length
			&pConn->pstQuerySubKeysCount,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryValues*/
    status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_VALUES);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, //search for null termination in szQuery to get length
			&pConn->pstQueryValues,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);

    /*pstQueryValuesCount*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_VALUE_COUNT);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, //search for null termination in szQuery to get length
			&pConn->pstQueryValuesCount,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


    /*pstQueryKeyValueWithType*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_KEYVALUE_WITHTYPE);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, //search for null termination in szQuery to get length
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
			-1, //search for null termination in szQuery to get length
			&pConn->pstQueryKeyValueWithWrongType,
			NULL);
	BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));
	LWREG_SAFE_FREE_MEMORY(pwszQueryStatement);


	/*pstQueryKeyValue*/
	status = LwRtlWC16StringAllocateFromCString(&pwszQueryStatement, REG_DB_QUERY_KEY_VALUE);
	BAIL_ON_NT_STATUS(status);

	status = sqlite3_prepare16_v2(
			pConn->pDb,
			pwszQueryStatement,
			-1, //search for null termination in szQuery to get length
			&pConn->pstQueryKeyValue,
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

NTSTATUS
RegDbCreateKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    OUT PREG_ENTRY* ppRegEntry
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_ENTRY pRegEntry = NULL;
    PREG_ENTRY pRegEntryDefaultValue = NULL;
    BOOLEAN bIsUpdate = FALSE;

    /*Create key*/
    status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringDuplicate(&pRegEntry->pwszKeyName, pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    pRegEntry->type = REG_KEY;
    pRegEntry->version.qwDbId = -1;

    status = RegDbStoreEntries(
                 hDb,
                 1,
                 &pRegEntry,
                 &bIsUpdate);
    BAIL_ON_NT_STATUS(status);

    *ppRegEntry = pRegEntry;

cleanup:
    RegDbSafeFreeEntry(&pRegEntryDefaultValue);

    return status;

error:
    RegDbSafeFreeEntry(&pRegEntry);
    *ppRegEntry = NULL;

    goto cleanup;
}

NTSTATUS
RegDbCreateKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_ENTRY pRegEntry = NULL;
    BOOLEAN bIsUpdate = TRUE;

    status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringDuplicate(&pRegEntry->pwszKeyName, pwszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlWC16StringDuplicate(&pRegEntry->pwszValueName, pwszValueName);
    BAIL_ON_NT_STATUS(status);

    if (dwValueLen)
    {
        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry->pValue, BYTE, sizeof(*pRegEntry->pValue)*dwValueLen);
        BAIL_ON_NT_STATUS(status);

        memcpy(pRegEntry->pValue, pValue, dwValueLen);
    }

    pRegEntry->dwValueLen = dwValueLen;
    pRegEntry->type = valueType;
    pRegEntry->version.qwDbId = -1;

    status = RegDbStoreEntries(
                 hDb,
                 1,
                 &pRegEntry,
                 &bIsUpdate);
    BAIL_ON_NT_STATUS(status);

    if (ppRegEntry)
    {
        *ppRegEntry = pRegEntry;
    }

cleanup:
    if (!ppRegEntry)
    {
        RegDbSafeFreeEntry(&pRegEntry);
    }

    return status;

error:
    RegDbSafeFreeEntry(&pRegEntry);
    *ppRegEntry = NULL;

    goto cleanup;
}

NTSTATUS
RegDbSetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN const PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    )
{
	NTSTATUS status = STATUS_SUCCESS;
	BOOLEAN bIsWrongType = FALSE;
    PREG_ENTRY pRegEntry = NULL;

    status = RegDbGetKeyValue(hDb,
		                  pwszKeyName,
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

	status = RegDbStoreEntries(
                     hDb,
                     1,
                     &pRegEntry,
                     NULL);
        BAIL_ON_NT_STATUS(status);
    }
    if (STATUS_OBJECT_NAME_NOT_FOUND == status && !pRegEntry)
    {
	status = RegDbCreateKeyValue(
			    hDb,
			    pwszKeyName,
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
        RegDbSafeFreeEntry(&pRegEntry);
    }

    return status;

error:
    RegDbSafeFreeEntry(&pRegEntry);
    if (ppRegEntry)
    {
	*ppRegEntry = NULL;
    }

    goto cleanup;
}

NTSTATUS
RegDbGetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    const int nExpectedCols = 6;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_ENTRY pRegEntry = NULL;

    BAIL_ON_NT_INVALID_STRING(pwszKeyName);
    BAIL_ON_NT_INVALID_STRING(pwszValueName);


    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    if (valueType == REG_UNKNOWN)
    {
        pstQuery = pConn->pstQueryKeyValue;

        status = (NTSTATUS)RegSqliteBindStringW(pstQuery, 1, pwszKeyName);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = (NTSTATUS)RegSqliteBindStringW(pstQuery, 2, pwszValueName);
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

        status = RegSqliteBindStringW(pstQuery, 1, pwszKeyName);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = RegSqliteBindStringW(pstQuery, 2, pwszValueName);
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
            //Duplicate key value records are found
		status = STATUS_DUPLICATE_NAME;
            BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackCacheInfo(pstQuery,
                        &iColumnPos,
                        &pRegEntry->version);
        BAIL_ON_NT_STATUS(status);

        status = RegDbUnpackRegEntryInfo(pstQuery,
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
        RegDbSafeFreeEntry(&pRegEntry);
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
RegDbOpenKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    const int nExpectedCols = 6;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_ENTRY pRegEntry = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pstQuery = pConn->pstOpenKeyEx;
    status = RegSqliteBindStringW(pstQuery, 1, pwszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

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
            //Duplicate keys are found
		status = STATUS_DUPLICATE_NAME;
            BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackCacheInfo(pstQuery,
                        &iColumnPos,
                        &pRegEntry->version);
        BAIL_ON_NT_STATUS(status);

        status = RegDbUnpackRegEntryInfo(pstQuery,
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

    status = (NTSTATUS)sqlite3_reset(pstQuery);
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
        RegDbSafeFreeEntry(&pRegEntry);
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
    IN PCWSTR pwszKeyName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteKey;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = RegSqliteBindStringW(pstQuery, 1, pwszKeyName);
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
RegDbQueryInfoKeyCount(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN QueryKeyInfoOption queryType,
    OUT size_t* psCount
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    const int nExpectedCols = 1;
    int iColumnPos = 0;
    int nGotColumns = 0;
    DWORD dwCount = 0;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    switch (queryType)
    {
        case QuerySubKeys:
            pstQuery = pConn->pstQuerySubKeysCount;
            break;

        case QueryValues:
            pstQuery = pConn->pstQueryValuesCount;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

    status = RegSqliteBindStringW(pstQuery, 1, pwszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

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
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

        iColumnPos = 0;

        switch (queryType)
        {
            case QuerySubKeys:
                status = RegDbUnpackSubKeysCountInfo(pstQuery,
                                                      &iColumnPos,
                                                      &dwCount);
                BAIL_ON_NT_STATUS(status);

                break;

            case QueryValues:
                status = RegDbUnpackKeyValuesCountInfo(pstQuery,
                                                      &iColumnPos,
                                                      &dwCount);
                BAIL_ON_NT_STATUS(status);

                break;

            default:
                status = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(status);
        }

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
        status = STATUS_DATA_ERROR;
        BAIL_ON_NT_STATUS(status);
    }

    *psCount = (size_t)dwCount;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return status;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    *psCount = 0;

    goto cleanup;
}

NTSTATUS
RegDbQueryInfoKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN QueryKeyInfoOption queryType,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_ENTRY** pppRegEntries
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    size_t sResultCapacity = 0;
    const int nExpectedCols = 6;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_ENTRY pRegEntry = NULL;
    PREG_ENTRY* ppRegEntries = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    switch (queryType)
    {
        case QuerySubKeys:
            pstQuery = pConn->pstQuerySubKeys;
            break;

        case QueryValues:
            pstQuery = pConn->pstQueryValues;
            break;

        default:
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
    }

    status = RegSqliteBindStringW(pstQuery, 1, pwszKeyName);
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
            status = RegReallocMemory(
                            ppRegEntries,
                            (PVOID*)&ppRegEntries,
                            sizeof(PREG_ENTRY) * sResultCapacity);
            BAIL_ON_NT_STATUS(status);
        }

        status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
        BAIL_ON_NT_STATUS(status);

        iColumnPos = 0;

        status = RegDbUnpackCacheInfo(pstQuery,
                        &iColumnPos,
                        &pRegEntry->version);
        BAIL_ON_NT_STATUS(status);

        status = RegDbUnpackRegEntryInfo(pstQuery,
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
        RegDbSafeFreeEntry(&pRegEntry);
        RegDbSafeFreeEntryList(sResultCount, &ppRegEntries);
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
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteKeyValue;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = RegSqliteBindStringW(pstQuery, 1, pwszKeyName);
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

static
NTSTATUS
RegDbFreePreparedStatements(
    IN OUT PREG_DB_CONNECTION pConn
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    int i;
    sqlite3_stmt * * const pppstFreeList[] = {
        &pConn->pstCreateCacheId,
        &pConn->pstCreateRegEntry,
        &pConn->pstDeleteCacheIdEntry,
        &pConn->pstDeleteKey,
        &pConn->pstDeleteKeyValue,
        &pConn->pstOpenKeyEx,
        &pConn->pstQueryKeyValue,
        &pConn->pstQueryKeyValueWithType,
        &pConn->pstQueryKeyValueWithWrongType,
        &pConn->pstQueryMultiKeyValues,
        &pConn->pstQuerySubKeys,
        &pConn->pstQuerySubKeysCount,
        &pConn->pstQueryValues,
        &pConn->pstQueryValuesCount,
        &pConn->pstReplaceRegEntry,
        &pConn->pstUpdateCacheIdEntry,
        &pConn->pstUpdateRegEntry
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
        "delete from " REG_DB_TABLE_NAME_CACHE_TAGS ";\n"
        "delete from " REG_DB_TABLE_NAME_ENTRIES ";\n"
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

void
RegDbSafeFreeEntryList(
    size_t sCount,
    PREG_ENTRY** pppEntries
    )
{
    if (*pppEntries != NULL)
    {
        size_t iEntry;
        for (iEntry = 0; iEntry < sCount; iEntry++)
        {
            RegDbSafeFreeEntry(&(*pppEntries)[iEntry]);
        }
        LWREG_SAFE_FREE_MEMORY(*pppEntries);
    }
}

void
RegDbSafeFreeEntry(
    PREG_ENTRY* ppEntry
    )
{
    PREG_ENTRY pEntry = NULL;
    if (ppEntry != NULL && *ppEntry != NULL)
    {
        pEntry = *ppEntry;

        LWREG_SAFE_FREE_MEMORY(pEntry->pwszKeyName);
        LWREG_SAFE_FREE_MEMORY(pEntry->pwszValueName);
        LWREG_SAFE_FREE_MEMORY(pEntry->pValue);
        pEntry->dwValueLen = 0;

        LWREG_SAFE_FREE_MEMORY(pEntry);
        *ppEntry = NULL;
    }
}

NTSTATUS
RegDbSafeRecordSubKeysInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    int iCount = 0;
    size_t sSubKeyLen = 0;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    //Remove previous subKey information if there is any
    RegFreeWC16StringArray(pKeyResult->ppwszSubKeyNames, pKeyResult->dwNumCacheSubKeys);

    if (!sCacheCount)
    {
        goto cleanup;
    }

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppwszSubKeyNames, PWSTR, sizeof(*(pKeyResult->ppwszSubKeyNames)) * sCacheCount);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (DWORD)sCacheCount; iCount++)
    {
        status = LwRtlWC16StringDuplicate(&pKeyResult->ppwszSubKeyNames[iCount], ppRegEntries[iCount]->pwszKeyName);
        BAIL_ON_NT_STATUS(status);

		if (ppRegEntries[iCount]->pwszKeyName)
		{
			sSubKeyLen = RtlWC16StringNumChars(ppRegEntries[iCount]->pwszKeyName);
		}

		if (pKeyResult->sMaxSubKeyLen < sSubKeyLen)
			pKeyResult->sMaxSubKeyLen = sSubKeyLen;

        sSubKeyLen = 0;
    }

cleanup:
    pKeyResult->dwNumSubKeys = (DWORD)sCount;
    pKeyResult->dwNumCacheSubKeys = sCacheCount;
    pKeyResult->bHasSubKeyInfo = TRUE;

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbSafeRecordSubKeysInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = RegDbSafeRecordSubKeysInfo_inlock(sCount,
                                                   sCacheCount,
                                                   ppRegEntries,
                                                   pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbSafeRecordValuesInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    int iCount = 0;
    size_t sValueNameLen = 0;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    //Remove previous subKey information if there is any
    RegFreeWC16StringArray(pKeyResult->ppwszValueNames, pKeyResult->dwNumCacheValues);
    RegFreeValueByteArray(pKeyResult->ppValues, pKeyResult->dwNumCacheValues);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pTypes);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pdwValueLen);

    if (!sCacheCount)
    {
        goto cleanup;
    }

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppwszValueNames, PWSTR,
		                  sizeof(*(pKeyResult->ppwszValueNames))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppValues, PBYTE,
		                  sizeof(*(pKeyResult->ppValues))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pTypes, REG_DATA_TYPE,
		                  sizeof(*(pKeyResult->pTypes))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pdwValueLen, DWORD,
		                  sizeof(*(pKeyResult->pdwValueLen))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (DWORD)sCacheCount; iCount++)
    {
        status = LwRtlWC16StringDuplicate(&pKeyResult->ppwszValueNames[iCount],
			                          ppRegEntries[iCount]->pwszValueName);
        BAIL_ON_NT_STATUS(status);

        if (ppRegEntries[iCount]->dwValueLen)
        {
            status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppValues[iCount], BYTE,
			                sizeof(*(pKeyResult->ppValues[iCount]))* ppRegEntries[iCount]->dwValueLen);
            BAIL_ON_NT_STATUS(status);

            memcpy(pKeyResult->ppValues[iCount], ppRegEntries[iCount]->pValue, ppRegEntries[iCount]->dwValueLen);
        }

        pKeyResult->pdwValueLen[iCount] = ppRegEntries[iCount]->dwValueLen;
        pKeyResult->pTypes[iCount] = ppRegEntries[iCount]->type;

		if (pKeyResult->sMaxValueLen < (size_t)ppRegEntries[iCount]->dwValueLen)
		{
			pKeyResult->sMaxValueLen = (size_t)ppRegEntries[iCount]->dwValueLen;
		}

		if (pKeyResult->ppwszValueNames[iCount])
		{
			sValueNameLen = RtlWC16StringNumChars(pKeyResult->ppwszValueNames[iCount]);
		}

		if (pKeyResult->sMaxValueNameLen < sValueNameLen)
			pKeyResult->sMaxValueNameLen = sValueNameLen;

        sValueNameLen = 0;
    }

cleanup:
    pKeyResult->dwNumValues = (DWORD)sCount;
    pKeyResult->dwNumCacheValues = sCacheCount;

    pKeyResult->bHasValueInfo = TRUE;

    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbSafeRecordValuesInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = RegDbSafeRecordValuesInfo_inlock(sCount,
                                           sCacheCount,
                                           ppRegEntries,
                                           pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}
