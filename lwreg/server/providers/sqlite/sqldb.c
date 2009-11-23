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
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
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

    status = RegSqliteReadString(
        pstQuery,
        piColumnPos,
        "KeyName",
        &pResult->pszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadString(
        pstQuery,
        piColumnPos,
        "ValueName",
        &pResult->pszValueName);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Type",
        &pResult->type);
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteReadString(
        pstQuery,
        piColumnPos,
        "Value",
        &pResult->pszValue);
    BAIL_ON_NT_STATUS(status);

    if (pResult->type != REG_KEY && !pResult->pszValueName)
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

    /*pstOpenKeyEx*/
    status = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
            REG_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
            REG_DB_TABLE_NAME_ENTRIES ".KeyName, "
            REG_DB_TABLE_NAME_ENTRIES ".ValueName, "
            REG_DB_TABLE_NAME_ENTRIES ".Type, "
            REG_DB_TABLE_NAME_ENTRIES ".Value "
            "from " REG_DB_TABLE_NAME_CACHE_TAGS ", " REG_DB_TABLE_NAME_ENTRIES " "
            "where " REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " REG_DB_TABLE_NAME_ENTRIES ".CacheId "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1 "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".Type = 21",
            -1, //search for null termination in szQuery to get length
            &pConn->pstOpenKeyEx,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    /*pstDeleteKey (delete the key and all of its associated values)*/
    status = sqlite3_prepare_v2(
            pConn->pDb,
            "delete from "  REG_DB_TABLE_NAME_ENTRIES " "
            "where " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1",
            -1, //search for null termination in szQuery to get length
            &pConn->pstDeleteKey,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    /*pstDeleteKeyValue*/
    status = sqlite3_prepare_v2(
            pConn->pDb,
            "delete from "  REG_DB_TABLE_NAME_ENTRIES " "
            "where " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1"
            "AND " REG_DB_TABLE_NAME_ENTRIES ".ValueName = ?2",
            -1, //search for null termination in szQuery to get length
            &pConn->pstDeleteKeyValue,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    /*pstQuerySubKeys*/
    status = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
            REG_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
            REG_DB_TABLE_NAME_ENTRIES ".KeyName, "
            REG_DB_TABLE_NAME_ENTRIES ".ValueName, "
            REG_DB_TABLE_NAME_ENTRIES ".Type, "
            REG_DB_TABLE_NAME_ENTRIES ".Value "
            "from " REG_DB_TABLE_NAME_CACHE_TAGS ", " REG_DB_TABLE_NAME_ENTRIES " "
            "where " REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " REG_DB_TABLE_NAME_ENTRIES ".CacheId "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".KeyName LIKE ?1  || '\\%' "
                    "AND NOT " REG_DB_TABLE_NAME_ENTRIES ".KeyName LIKE ?1  || '\\%\\%' "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".Type = 21 "
                    "LIMIT ?2 OFFSET ?3",
            -1, //search for null termination in szQuery to get length
            &pConn->pstQuerySubKeys,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    /*pstQuerySubKeysCount*/
        status = sqlite3_prepare_v2(
                pConn->pDb,
                "select COUNT (*) as subkeyCount "
                "from " REG_DB_TABLE_NAME_CACHE_TAGS ", " REG_DB_TABLE_NAME_ENTRIES " "
                "where " REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " REG_DB_TABLE_NAME_ENTRIES ".CacheId "
                        "AND " REG_DB_TABLE_NAME_ENTRIES ".KeyName LIKE ?1  || '\\%' "
                        "AND NOT " REG_DB_TABLE_NAME_ENTRIES ".KeyName LIKE ?1  || '\\%\\%' "
                        "AND " REG_DB_TABLE_NAME_ENTRIES ".Type = 21",
                -1, //search for null termination in szQuery to get length
                &pConn->pstQuerySubKeysCount,
                NULL);
        BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    /*pstQueryValues*/
    status = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
            REG_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
            REG_DB_TABLE_NAME_ENTRIES ".KeyName, "
            REG_DB_TABLE_NAME_ENTRIES ".ValueName, "
            REG_DB_TABLE_NAME_ENTRIES ".Type, "
            REG_DB_TABLE_NAME_ENTRIES ".Value "
            "from " REG_DB_TABLE_NAME_CACHE_TAGS ", " REG_DB_TABLE_NAME_ENTRIES " "
            "where " REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " REG_DB_TABLE_NAME_ENTRIES ".CacheId "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1 "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".Type != 21 "
                    "LIMIT ?2 OFFSET ?3",
            -1, //search for null termination in szQuery to get length
            &pConn->pstQueryValues,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    /*pstQueryValuesCount*/
        status = sqlite3_prepare_v2(
                pConn->pDb,
                "select COUNT (*) as valueCount "
                "from " REG_DB_TABLE_NAME_CACHE_TAGS ", " REG_DB_TABLE_NAME_ENTRIES " "
                "where " REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " REG_DB_TABLE_NAME_ENTRIES ".CacheId "
                        "AND " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1 "
                        "AND " REG_DB_TABLE_NAME_ENTRIES ".Type != 21",
                -1, //search for null termination in szQuery to get length
                &pConn->pstQueryValuesCount,
                NULL);
        BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    /*pstQueryKeyValueWithType*/
    status = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
            REG_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
            REG_DB_TABLE_NAME_ENTRIES ".KeyName, "
            REG_DB_TABLE_NAME_ENTRIES ".ValueName, "
            REG_DB_TABLE_NAME_ENTRIES ".Type, "
            REG_DB_TABLE_NAME_ENTRIES ".Value "
            "from " REG_DB_TABLE_NAME_CACHE_TAGS ", " REG_DB_TABLE_NAME_ENTRIES " "
            "where " REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " REG_DB_TABLE_NAME_ENTRIES ".CacheId "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1 "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".ValueName = ?2 "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".Type = ?3",
            -1, //search for null termination in szQuery to get length
            &pConn->pstQueryKeyValueWithType,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    /*pstQueryKeyValueWithWrongType*/
        status = sqlite3_prepare_v2(
                pConn->pDb,
                "select "
                REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
                REG_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
                REG_DB_TABLE_NAME_ENTRIES ".KeyName, "
                REG_DB_TABLE_NAME_ENTRIES ".ValueName, "
                REG_DB_TABLE_NAME_ENTRIES ".Type, "
                REG_DB_TABLE_NAME_ENTRIES ".Value "
                "from " REG_DB_TABLE_NAME_CACHE_TAGS ", " REG_DB_TABLE_NAME_ENTRIES " "
                "where " REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " REG_DB_TABLE_NAME_ENTRIES ".CacheId "
                        "AND " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1 "
                        "AND " REG_DB_TABLE_NAME_ENTRIES ".ValueName = ?2 "
                        "AND " REG_DB_TABLE_NAME_ENTRIES ".Type != ?3",
                -1, //search for null termination in szQuery to get length
                &pConn->pstQueryKeyValueWithWrongType,
                NULL);
        BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    /*pstQueryKeyValue*/
    status = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
            REG_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
            REG_DB_TABLE_NAME_ENTRIES ".KeyName, "
            REG_DB_TABLE_NAME_ENTRIES ".ValueName, "
            REG_DB_TABLE_NAME_ENTRIES ".Type, "
            REG_DB_TABLE_NAME_ENTRIES ".Value "
            "from " REG_DB_TABLE_NAME_CACHE_TAGS ", " REG_DB_TABLE_NAME_ENTRIES " "
            "where " REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " REG_DB_TABLE_NAME_ENTRIES ".CacheId "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1 "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".ValueName = ?2 "
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".Type != 21",
            -1, //search for null termination in szQuery to get length
            &pConn->pstQueryKeyValue,
            NULL);
    BAIL_ON_SQLITE3_ERROR(status, sqlite3_errmsg(pConn->pDb));

    *phDb = pConn;

cleanup:

    if (pszError != NULL)
    {
        sqlite3_free(pszError);
    }
    LWREG_SAFE_FREE_STRING(pszDbDir);

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
RegDbStoreKeyObjectEntries(
    REG_DB_HANDLE hDb,
    size_t  sEntryCount,
    PREG_ENTRY* ppEntries
    )
{
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    NTSTATUS status = STATUS_SUCCESS;
    size_t sIndex = 0;
    //Free with sqlite3_free
    char *pszError = NULL;
    //Free with sqlite3_free
    char *pszNewStatement = NULL;
    REG_STRING_BUFFER buffer = {0};
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;


    status = RegInitializeStringBuffer(
            &buffer,
            sEntryCount * 200);
    BAIL_ON_NT_STATUS(status);

    status = RegAppendStringBuffer(
            &buffer,
            "begin");
    BAIL_ON_NT_STATUS(status);

    for (sIndex = 0; sIndex < sEntryCount; sIndex++)
    {
        if (ppEntries[sIndex] == NULL)
        {
            continue;
        }

        if (ppEntries[sIndex]->version.qwDbId == -1)
        {
            if (!bGotNow)
            {
                status = RegGetCurrentTimeSeconds(&now);
                BAIL_ON_NT_STATUS(status);

                bGotNow = TRUE;
            }

            // Before storing key objects, we already make sure no such key exists
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                // Make a new entry
                "insert into " REG_DB_TABLE_NAME_CACHE_TAGS " ("
                    "LastUpdated"
                    ") values ("
                    "%ld);\n"
                "insert into " REG_DB_TABLE_NAME_ENTRIES " ("
                    "CacheId,"
                    "KeyName,"
                    "ValueName,"
                    "Type,"
                    "Value"
                    ") values ("
                    // This is the CacheId column of the row that was just
                    // created.
                    "last_insert_rowid(),"
                    "%Q," //KeyName
                    "%Q," //ValueName
                    "%d," //Type
                    "%Q)" /*Value*/,
                now,
                ppEntries[sIndex]->pszKeyName,
                ppEntries[sIndex]->pszValueName,
                ppEntries[sIndex]->type,
                ppEntries[sIndex]->pszValue
                );
        }

        if (pszNewStatement == NULL)
        {
            status = STATUS_NO_MEMORY;
            BAIL_ON_NT_STATUS(status);
        }

        status = RegAppendStringBuffer(
                &buffer,
                pszNewStatement);
        BAIL_ON_NT_STATUS(status);
        SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    }

    status = RegAppendStringBuffer(
            &buffer,
            ";\nend");
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
        buffer.pszBuffer);
    BAIL_ON_NT_STATUS(status);

cleanup:
    SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    SQLITE3_SAFE_FREE_STRING(pszError);
    RegFreeStringBufferContents(&buffer);

    return status;

error:

    goto cleanup;
}

NTSTATUS
RegDbStoreObjectEntries(
    REG_DB_HANDLE hDb,
    size_t  sEntryCount,
    PREG_ENTRY* ppEntries
    )
{
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    NTSTATUS status = STATUS_SUCCESS;
    size_t sIndex = 0;
    //Free with sqlite3_free
    char *pszError = NULL;
    //Free with sqlite3_free
    char *pszNewStatement = NULL;
    REG_STRING_BUFFER buffer = {0};
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;

    /* This function generates a SQL transaction to update multiple
     * entries at a time. The SQL command is in this format:
     * 1. Delete database tag entries which are no longer referenced.
     * 2. Create/update the new database tag entries, and create/update the
     *    " REG_DB_TABLE_NAME_ENTRIES ".
     */

    status = RegInitializeStringBuffer(
            &buffer,
            sEntryCount * 200);
    BAIL_ON_NT_STATUS(status);

    status = RegAppendStringBuffer(
            &buffer,
            "begin");
    BAIL_ON_NT_STATUS(status);

    for (sIndex = 0; sIndex < sEntryCount; sIndex++)
    {
        if (ppEntries[sIndex] == NULL)
        {
            continue;
        }

        if (ppEntries[sIndex]->version.qwDbId == -1)
        {
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                "delete from " REG_DB_TABLE_NAME_CACHE_TAGS " where CacheId IN "
                    "( select CacheId from " REG_DB_TABLE_NAME_ENTRIES " where KeyName = %Q and ValueName = %Q)",
                    ppEntries[sIndex]->pszKeyName, ppEntries[sIndex]->pszValueName);

            if (pszNewStatement == NULL)
            {
                status = STATUS_NO_MEMORY;
                BAIL_ON_NT_STATUS(status);
            }

            status = RegAppendStringBuffer(
                    &buffer,
                    pszNewStatement);
            BAIL_ON_NT_STATUS(status);
            SQLITE3_SAFE_FREE_STRING(pszNewStatement);
        }
    }

    for (sIndex = 0; sIndex < sEntryCount; sIndex++)
    {
        if (ppEntries[sIndex] == NULL)
        {
            continue;
        }

        if (ppEntries[sIndex]->version.qwDbId == -1)
        {
            if (!bGotNow)
            {
                status = RegGetCurrentTimeSeconds(&now);
                BAIL_ON_NT_STATUS(status);

                bGotNow = TRUE;
            }

            // The object is either not stored yet, or the existing entry
            // needs to be replaced.
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                // Make a new entry
                "insert into " REG_DB_TABLE_NAME_CACHE_TAGS " ("
                    "LastUpdated"
                    ") values ("
                    "%ld);\n"
                "replace into " REG_DB_TABLE_NAME_ENTRIES " ("
                    "CacheId,"
                    "KeyName,"
                    "ValueName,"
                    "Type,"
                    "Value"
                    ") values ("
                    // This is the CacheId column of the row that was just
                    // created.
                    "last_insert_rowid(),"
                    "%Q," /*KeyName*/
                    "%Q," /*ValueName*/
                    "%d," /*Type*/
                    "%Q)" /*Value*/,
                now,
                ppEntries[sIndex]->pszKeyName,
                ppEntries[sIndex]->pszValueName,
                ppEntries[sIndex]->type,
                ppEntries[sIndex]->pszValue
                );
        }
        else
        {
            // The object is already stored. Just update the existing info.
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                    // Update the existing entry
                    "update " REG_DB_TABLE_NAME_CACHE_TAGS " set "
                        "LastUpdated = %ld "
                        "where CacheId = %llu;\n"
                    "update " REG_DB_TABLE_NAME_ENTRIES " set "
                        "CacheId = %llu, "
                        "Type = %d, "
                        "Value = %Q "
                        "where KeyName = %Q and ValueName = %Q",
                ppEntries[sIndex]->version.tLastUpdated,
                ppEntries[sIndex]->version.qwDbId,
                ppEntries[sIndex]->version.qwDbId,
                ppEntries[sIndex]->type,
                ppEntries[sIndex]->pszValue,
                ppEntries[sIndex]->pszKeyName,
                ppEntries[sIndex]->pszValueName
                );
        }

        if (pszNewStatement == NULL)
        {
            status = STATUS_NO_MEMORY;
            BAIL_ON_NT_STATUS(status);
        }

        status = RegAppendStringBuffer(
                &buffer,
                pszNewStatement);
        BAIL_ON_NT_STATUS(status);
        SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    }

    status = RegAppendStringBuffer(
            &buffer,
            ";\nend");
    BAIL_ON_NT_STATUS(status);

    status = RegSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
        buffer.pszBuffer);
    BAIL_ON_NT_STATUS(status);

cleanup:
    SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    SQLITE3_SAFE_FREE_STRING(pszError);
    RegFreeStringBufferContents(&buffer);

    return status;

error:

    goto cleanup;
}

NTSTATUS
RegDbCreateKey(
    IN REG_DB_HANDLE hDb,
    IN PSTR pszKeyName,
    OUT PREG_ENTRY* ppRegEntry
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_ENTRY pRegEntry = NULL;
    PREG_ENTRY pRegEntryDefaultValue = NULL;

    /*Create key*/
    status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCStringDuplicate(&pRegEntry->pszKeyName, pszKeyName);
    BAIL_ON_NT_STATUS(status);

    pRegEntry->type = REG_KEY;
    pRegEntry->version.qwDbId = -1;

    status = RegDbStoreKeyObjectEntries(
                 hDb,
                 1,
                 &pRegEntry);
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
    IN PSTR pszKeyName,
    IN PSTR pszValueName,
    IN PSTR pszValue,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_ENTRY pRegEntry = NULL;

    status = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCStringDuplicate(&pRegEntry->pszKeyName, pszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCStringDuplicate(&pRegEntry->pszValueName, pszValueName);
    BAIL_ON_NT_STATUS(status);

    if (pszValue)
    {
        status = LwRtlCStringDuplicate(&pRegEntry->pszValue, pszValue);
        BAIL_ON_NT_STATUS(status);
    }

    pRegEntry->type = valueType;
    pRegEntry->version.qwDbId = -1;

    status = RegDbStoreObjectEntries(
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
        RegDbSafeFreeEntry(&pRegEntry);
    }

    return status;

error:
    RegDbSafeFreeEntry(&pRegEntry);
    *ppRegEntry = NULL;

    goto cleanup;
}

NTSTATUS
RegDbGetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PSTR pszKeyName,
    IN PSTR pszValueName,
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

    BAIL_ON_NT_INVALID_STRING(pszKeyName);
    BAIL_ON_NT_INVALID_STRING(pszValueName);


    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    if (valueType == REG_UNKNOWN)
    {
        pstQuery = pConn->pstQueryKeyValue;

        status = (NTSTATUS)RegSqliteBindString(pstQuery, 1, pszKeyName);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = (NTSTATUS)RegSqliteBindString(pstQuery, 2, pszValueName);
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

        status = RegSqliteBindString(pstQuery, 1, pszKeyName);
        BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

        status = RegSqliteBindString(pstQuery, 2, pszValueName);
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
    IN PCSTR pszKeyName,
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
    status = RegSqliteBindString(pstQuery, 1, pszKeyName);
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
    IN PCSTR pszKeyName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteKey;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = RegSqliteBindString(pstQuery, 1, pszKeyName);
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
    IN PCSTR pszKeyName,
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

    status = RegSqliteBindString(pstQuery, 1, pszKeyName);
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
    IN PCSTR pszKeyName,
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

    status = RegSqliteBindString(pstQuery, 1, pszKeyName);
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
    IN PCSTR pszKeyName,
    IN PCSTR pszValueName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteKeyValue;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    status = RegSqliteBindString(pstQuery, 1, pszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(status, pstQuery);

    status = RegSqliteBindString(pstQuery, 2, pszValueName);
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
        &pConn->pstDeleteKey,
        &pConn->pstDeleteKeyValue,
        &pConn->pstOpenKeyEx,
        &pConn->pstQueryKeyValue,
        &pConn->pstQueryKeyValueWithType,
        &pConn->pstQuerySubKeys,
        &pConn->pstQuerySubKeysCount,
        &pConn->pstQueryValues,
        &pConn->pstQueryValuesCount,
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

        LWREG_SAFE_FREE_STRING(pEntry->pszKeyName);
        LWREG_SAFE_FREE_STRING(pEntry->pszValueName);
        LWREG_SAFE_FREE_STRING(pEntry->pszValue);

        LWREG_SAFE_FREE_MEMORY(pEntry);
        *ppEntry = NULL;
    }
}

NTSTATUS
RegDbSafeRecordSubKeysInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    int iCount = 0;
    size_t sSubKeyLen = 0;
    PWSTR pSubKey = NULL;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    //Remove previous subKey information if there is any
    RegFreeStringArray(pKeyResult->ppszSubKeyNames, pKeyResult->dwNumCacheSubKeys);

    if (!sCacheCount)
    {
        goto cleanup;
    }

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppszSubKeyNames, PSTR, sizeof(*(pKeyResult->ppszSubKeyNames)) * sCacheCount);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (DWORD)sCacheCount; iCount++)
    {
        status = LwRtlCStringDuplicate(&pKeyResult->ppszSubKeyNames[iCount], ppRegEntries[iCount]->pszKeyName);
        BAIL_ON_NT_STATUS(status);

        if (bDoAnsi)
        {
            sSubKeyLen = strlen(ppRegEntries[iCount]->pszKeyName);

            if (pKeyResult->sMaxSubKeyALen < sSubKeyLen)
                pKeyResult->sMaxSubKeyALen = sSubKeyLen;
        }
        else
        {
		status = LwRtlWC16StringAllocateFromCString(&pSubKey, ppRegEntries[iCount]->pszKeyName);
		BAIL_ON_NT_STATUS(status);

            if (pSubKey)
            {
		sSubKeyLen = RtlWC16StringNumChars(pSubKey);
            }

            if (pKeyResult->sMaxSubKeyLen < sSubKeyLen)
                pKeyResult->sMaxSubKeyLen = sSubKeyLen;
        }

        LWREG_SAFE_FREE_MEMORY(pSubKey);
        sSubKeyLen = 0;
    }

cleanup:
    pKeyResult->dwNumSubKeys = (DWORD)sCount;
    pKeyResult->dwNumCacheSubKeys = sCacheCount;
    if (bDoAnsi)
    {
        pKeyResult->bHasSubKeyAInfo = TRUE;
    }
    else
    {
        pKeyResult->bHasSubKeyInfo = TRUE;
    }

    LWREG_SAFE_FREE_MEMORY(pSubKey);
    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbSafeRecordSubKeysInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = RegDbSafeRecordSubKeysInfo_inlock(sCount,
                                                   sCacheCount,
                                                   ppRegEntries,
                                                   pKeyResult,
                                                   bDoAnsi);
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
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    int iCount = 0;
    size_t sValueNameLen = 0;
    PWSTR pValueName = NULL;
    DWORD dwValueLen = 0;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    //Remove previous subKey information if there is any
    RegFreeStringArray(pKeyResult->ppszValueNames, pKeyResult->dwNumCacheValues);
    RegFreeStringArray(pKeyResult->ppszValues, pKeyResult->dwNumCacheValues);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pTypes);

    if (!sCacheCount)
    {
        goto cleanup;
    }

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppszValueNames, PSTR,
		                  sizeof(*(pKeyResult->ppszValueNames))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppszValues, PSTR,
		                  sizeof(*(pKeyResult->ppszValues))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    status = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pTypes, REG_DATA_TYPE,
		                  sizeof(*(pKeyResult->pTypes))* sCacheCount);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (DWORD)sCacheCount; iCount++)
    {
        status = LwRtlCStringDuplicate(&pKeyResult->ppszValueNames[iCount],
			                        ppRegEntries[iCount]->pszValueName);
        BAIL_ON_NT_STATUS(status);

        if (ppRegEntries[iCount]->pszValue)
        {
            status = LwRtlCStringDuplicate(&pKeyResult->ppszValues[iCount],
			                        ppRegEntries[iCount]->pszValue);
            BAIL_ON_NT_STATUS(status);
        }

        pKeyResult->pTypes[iCount] = ppRegEntries[iCount]->type;

        if (bDoAnsi)
        {
            sValueNameLen = strlen(pKeyResult->ppszValueNames[iCount]);

            if (pKeyResult->sMaxValueNameALen < sValueNameLen)
                pKeyResult->sMaxValueNameALen = sValueNameLen;
        }
        else
        {
		status = LwRtlWC16StringAllocateFromCString(&pValueName,
				                                     pKeyResult->ppszValueNames[iCount]);
		BAIL_ON_NT_STATUS(status);

            if (pValueName)
            {
		sValueNameLen = RtlWC16StringNumChars(pValueName);
            }

            if (pKeyResult->sMaxValueNameLen < sValueNameLen)
                pKeyResult->sMaxValueNameLen = sValueNameLen;
        }

        status = RegGetValueAsBytes(ppRegEntries[iCount]->type,
                                  (PCSTR)ppRegEntries[iCount]->pszValue,
                                  bDoAnsi,
                                  NULL,
                                  &dwValueLen);
        BAIL_ON_NT_STATUS(status);

        if (bDoAnsi)
        {
            if (pKeyResult->sMaxValueALen < (size_t)dwValueLen)
                pKeyResult->sMaxValueALen = (size_t)dwValueLen;
        }
        else
        {
            if (pKeyResult->sMaxValueLen < (size_t)dwValueLen)
                pKeyResult->sMaxValueLen = (size_t)dwValueLen;
        }

        LWREG_SAFE_FREE_MEMORY(pValueName);
        sValueNameLen = 0;
        dwValueLen = 0;
    }

cleanup:
    pKeyResult->dwNumValues = (DWORD)sCount;
    pKeyResult->dwNumCacheValues = sCacheCount;

    if (bDoAnsi)
    {
        pKeyResult->bHasValueAInfo = TRUE;
    }
    else
    {
        pKeyResult->bHasValueInfo = TRUE;
    }

    LWREG_SAFE_FREE_MEMORY(pValueName);
    return status;

error:
    goto cleanup;
}

NTSTATUS
RegDbSafeRecordValuesInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = RegDbSafeRecordValuesInfo_inlock(sCount,
                                           sCacheCount,
                                           ppRegEntries,
                                           pKeyResult,
                                           bDoAnsi);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}
