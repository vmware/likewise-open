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
 *        sqlcache.c
 *
 * Abstract:
 *
 *        Sqlite3 Caching backend for AD Provider Database Interface
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
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

DWORD
RegDbUnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PREG_ENTRY_VERSION_INFO pResult
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;

    dwError = RegSqliteReadInt64(
        pstQuery,
        piColumnPos,
        "CacheId",
        &pResult->qwDbId);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSqliteReadTimeT(
        pstQuery,
        piColumnPos,
        "LastUpdated",
        &pResult->tLastUpdated);
    BAIL_ON_REG_ERROR(dwError);

error:
    return dwError;
}

DWORD
RegDbUnpackRegEntryInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_ENTRY pResult
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;

    dwError = RegSqliteReadString(
        pstQuery,
        piColumnPos,
        "KeyName",
        &pResult->pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSqliteReadString(
        pstQuery,
        piColumnPos,
        "ValueName",
        &pResult->pszValueName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Type",
        &pResult->type);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSqliteReadString(
        pstQuery,
        piColumnPos,
        "Value",
        &pResult->pszValue);
    BAIL_ON_REG_ERROR(dwError);

    if (pResult->type != REG_KEY && !pResult->pszValueName)
    {
        dwError = LWREG_ERROR_INVALID_VALUENAME;
        BAIL_ON_REG_ERROR(dwError);
    }

error:
    return dwError;
}

DWORD
RegDbUnpackSubKeysCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;

    dwError = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "subkeyCount",
        pdwCount);
    BAIL_ON_REG_ERROR(dwError);

error:
    return dwError;
}

DWORD
RegDbUnpackKeyValuesCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;

    dwError = RegSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "valueCount",
        pdwCount);
    BAIL_ON_REG_ERROR(dwError);

error:
    return dwError;
}

DWORD
RegDbSetup(
    IN sqlite3* pSqlHandle
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = RegSqliteExec(pSqlHandle,
                            REG_DB_CREATE_TABLES,
                            &pszError);
    if (dwError)
    {
        REG_LOG_DEBUG("SQL failed: code = %d, message = '%s'\nSQL =\n%s",
                      dwError, pszError, REG_DB_CREATE_TABLES);
    }
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);

cleanup:
    SQLITE3_SAFE_FREE_STRING(pszError);
    return dwError;

error:
    goto cleanup;
}

DWORD
RegDbOpen(
    IN PCSTR pszDbPath,
    OUT PREG_DB_HANDLE phDb
    )
{
    DWORD dwError = 0;
    BOOLEAN bLockCreated = FALSE;
    PREG_DB_CONNECTION pConn = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;
    PSTR pszDbDir = NULL;

    dwError = RegGetDirectoryFromPath(
                    pszDbPath,
                    &pszDbDir);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LW_RTL_ALLOCATE((PVOID*)&pConn, REG_DB_CONNECTION, sizeof(*pConn));
    BAIL_ON_REG_ERROR(dwError);

    dwError = pthread_rwlock_init(&pConn->lock, NULL);
    BAIL_ON_REG_ERROR(dwError);
    bLockCreated = TRUE;

    dwError = RegCheckDirectoryExists(pszDbDir, &bExists);
    BAIL_ON_REG_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = RegCreateDirectory(pszDbDir, cacheDirMode);
        BAIL_ON_REG_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = RegChangeOwnerAndPermissions(pszDbDir, 0, 0, S_IRWXU);
    BAIL_ON_REG_ERROR(dwError);

    dwError = sqlite3_open(pszDbPath, &pConn->pDb);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegChangeOwnerAndPermissions(pszDbPath, 0, 0, S_IRWXU);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDbSetup(pConn->pDb);
    BAIL_ON_REG_ERROR(dwError);

    /*pstOpenKeyEx*/
    dwError = sqlite3_prepare_v2(
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
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    /*pstDeleteKey (delete the key and all of its associated values)*/
    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "delete from "  REG_DB_TABLE_NAME_ENTRIES " "
            "where " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1",
            -1, //search for null termination in szQuery to get length
            &pConn->pstDeleteKey,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    /*pstDeleteKeyValue*/
    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "delete from "  REG_DB_TABLE_NAME_ENTRIES " "
            "where " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1"
            "AND " REG_DB_TABLE_NAME_ENTRIES ".ValueName = ?2",
            -1, //search for null termination in szQuery to get length
            &pConn->pstDeleteKeyValue,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    /*pstQuerySubKeys*/
    dwError = sqlite3_prepare_v2(
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
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    /*pstQuerySubKeysCount*/
        dwError = sqlite3_prepare_v2(
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
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    /*pstQueryValues*/
    dwError = sqlite3_prepare_v2(
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
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    /*pstQueryValuesCount*/
        dwError = sqlite3_prepare_v2(
                pConn->pDb,
                "select COUNT (*) as valueCount "
                "from " REG_DB_TABLE_NAME_CACHE_TAGS ", " REG_DB_TABLE_NAME_ENTRIES " "
                "where " REG_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " REG_DB_TABLE_NAME_ENTRIES ".CacheId "
                        "AND " REG_DB_TABLE_NAME_ENTRIES ".KeyName = ?1 "
                        "AND " REG_DB_TABLE_NAME_ENTRIES ".Type != 21",
                -1, //search for null termination in szQuery to get length
                &pConn->pstQueryValuesCount,
                NULL);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    /*pstQueryKeyValueWithType*/
    dwError = sqlite3_prepare_v2(
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
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    /*pstQueryKeyValueWithWrongType*/
        dwError = sqlite3_prepare_v2(
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
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    /*pstQueryKeyValue*/
    dwError = sqlite3_prepare_v2(
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
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    *phDb = pConn;

cleanup:

    if (pszError != NULL)
    {
        sqlite3_free(pszError);
    }
    LWREG_SAFE_FREE_STRING(pszDbDir);

    return dwError;

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

DWORD
RegDbStoreKeyObjectEntries(
    REG_DB_HANDLE hDb,
    size_t  sEntryCount,
    PREG_ENTRY* ppEntries
    )
{
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    DWORD dwError = LWREG_ERROR_SUCCESS;
    size_t sIndex = 0;
    //Free with sqlite3_free
    char *pszError = NULL;
    //Free with sqlite3_free
    char *pszNewStatement = NULL;
    REG_STRING_BUFFER buffer = {0};
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;


    dwError = RegInitializeStringBuffer(
            &buffer,
            sEntryCount * 200);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegAppendStringBuffer(
            &buffer,
            "begin");
    BAIL_ON_REG_ERROR(dwError);

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
                dwError = RegGetCurrentTimeSeconds(&now);
                BAIL_ON_REG_ERROR(dwError);

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
            dwError = LWREG_ERROR_OUT_OF_MEMORY;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = RegAppendStringBuffer(
                &buffer,
                pszNewStatement);
        BAIL_ON_REG_ERROR(dwError);
        SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    }

    dwError = RegAppendStringBuffer(
            &buffer,
            ";\nend");
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
        buffer.pszBuffer);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    SQLITE3_SAFE_FREE_STRING(pszError);
    RegFreeStringBufferContents(&buffer);

    return dwError;

error:

    goto cleanup;
}

DWORD
RegDbStoreObjectEntries(
    REG_DB_HANDLE hDb,
    size_t  sEntryCount,
    PREG_ENTRY* ppEntries
    )
{
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    DWORD dwError = LWREG_ERROR_SUCCESS;
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

    dwError = RegInitializeStringBuffer(
            &buffer,
            sEntryCount * 200);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegAppendStringBuffer(
            &buffer,
            "begin");
    BAIL_ON_REG_ERROR(dwError);

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
                dwError = LWREG_ERROR_OUT_OF_MEMORY;
                BAIL_ON_REG_ERROR(dwError);
            }

            dwError = RegAppendStringBuffer(
                    &buffer,
                    pszNewStatement);
            BAIL_ON_REG_ERROR(dwError);
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
                dwError = RegGetCurrentTimeSeconds(&now);
                BAIL_ON_REG_ERROR(dwError);

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
            dwError = LWREG_ERROR_OUT_OF_MEMORY;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = RegAppendStringBuffer(
                &buffer,
                pszNewStatement);
        BAIL_ON_REG_ERROR(dwError);
        SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    }

    dwError = RegAppendStringBuffer(
            &buffer,
            ";\nend");
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
        buffer.pszBuffer);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    SQLITE3_SAFE_FREE_STRING(pszError);
    RegFreeStringBufferContents(&buffer);

    return dwError;

error:

    goto cleanup;
}

DWORD
RegDbCreateKey(
    IN REG_DB_HANDLE hDb,
    IN PSTR pszKeyName,
    OUT PREG_ENTRY* ppRegEntry
    )
{
    DWORD dwError = 0;
    PREG_ENTRY pRegEntry = NULL;
    PREG_ENTRY pRegEntryDefaultValue = NULL;

    /*Create key*/
    dwError = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwRtlCStringDuplicate(&pRegEntry->pszKeyName, pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    pRegEntry->type = REG_KEY;
    pRegEntry->version.qwDbId = -1;

    dwError = RegDbStoreKeyObjectEntries(
                 hDb,
                 1,
                 &pRegEntry);
    BAIL_ON_REG_ERROR(dwError);

    *ppRegEntry = pRegEntry;

cleanup:
    RegCacheSafeFreeEntry(&pRegEntryDefaultValue);

    return dwError;

error:
    RegCacheSafeFreeEntry(&pRegEntry);
    *ppRegEntry = NULL;

    goto cleanup;
}

DWORD
RegDbCreateKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PSTR pszKeyName,
    IN PSTR pszValueName,
    IN PSTR pszValue,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    )
{
    DWORD dwError = 0;
    PREG_ENTRY pRegEntry = NULL;

    dwError = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwRtlCStringDuplicate(&pRegEntry->pszKeyName, pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwRtlCStringDuplicate(&pRegEntry->pszValueName, pszValueName);
    BAIL_ON_REG_ERROR(dwError);

    if (pszValue)
    {
        dwError = LwRtlCStringDuplicate(&pRegEntry->pszValue, pszValue);
        BAIL_ON_REG_ERROR(dwError);
    }

    pRegEntry->type = valueType;
    pRegEntry->version.qwDbId = -1;

    dwError = RegDbStoreObjectEntries(
                 hDb,
                 1,
                 &pRegEntry);
    BAIL_ON_REG_ERROR(dwError);

    if (ppRegEntry)
    {
        *ppRegEntry = pRegEntry;
    }

cleanup:
    if (!ppRegEntry)
    {
        RegCacheSafeFreeEntry(&pRegEntry);
    }

    return dwError;

error:
    RegCacheSafeFreeEntry(&pRegEntry);
    *ppRegEntry = NULL;

    goto cleanup;
}

DWORD
RegDbGetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PSTR pszKeyName,
    IN PSTR pszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCount = 0;
    const int nExpectedCols = 6;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PREG_ENTRY pRegEntry = NULL;

    BAIL_ON_INVALID_STRING(pszKeyName);
    BAIL_ON_INVALID_STRING(pszValueName);


    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    if (valueType == REG_UNKNOWN)
    {
        pstQuery = pConn->pstQueryKeyValue;

        dwError = RegSqliteBindString(pstQuery, 1, pszKeyName);
        BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

        dwError = RegSqliteBindString(pstQuery, 2, pszValueName);
        BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);
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

        dwError = RegSqliteBindString(pstQuery, 1, pszKeyName);
        BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

        dwError = RegSqliteBindString(pstQuery, 2, pszValueName);
        BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

        dwError = RegSqliteBindInt32(pstQuery, 3, (int)valueType);
        BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);
    }

    while ((dwError = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LWREG_ERROR_DATA_ERROR;
            BAIL_ON_REG_ERROR(dwError);
        }

        if (sResultCount >= 1)
        {
            //Duplicate key value records are found
            dwError = LWREG_ERROR_DUPLICATE_KEYVALUENAME;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
        BAIL_ON_REG_ERROR(dwError);

        iColumnPos = 0;

        dwError = RegDbUnpackCacheInfo(pstQuery,
                        &iColumnPos,
                        &pRegEntry->version);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegDbUnpackRegEntryInfo(pstQuery,
                                          &iColumnPos,
                                          pRegEntry);
        BAIL_ON_REG_ERROR(dwError);

        sResultCount++;
    }

    if (dwError == SQLITE_DONE)
    {
        // No more results found
        dwError = LWREG_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    if (!sResultCount)
    {
        dwError = LWREG_ERROR_NO_SUCH_VALUENAME;
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (!dwError && ppRegEntry)
    {
        *ppRegEntry = pRegEntry;
    }
    else
    {
        RegCacheSafeFreeEntry(&pRegEntry);
        if (ppRegEntry)
            *ppRegEntry = NULL;
    }

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);


    return dwError;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

DWORD
RegDbOpenKey(
    IN REG_DB_HANDLE hDb,
    IN PCSTR pszKeyName,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
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
    dwError = RegSqliteBindString(pstQuery, 1, pszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    while ((dwError = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LWREG_ERROR_DATA_ERROR;
            BAIL_ON_REG_ERROR(dwError);
        }

        if (sResultCount >= 1)
        {
            //Duplicate keys are found
            dwError = LWREG_ERROR_DUPLICATE_KEYNAME;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
        BAIL_ON_REG_ERROR(dwError);

        iColumnPos = 0;

        dwError = RegDbUnpackCacheInfo(pstQuery,
                        &iColumnPos,
                        &pRegEntry->version);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegDbUnpackRegEntryInfo(pstQuery,
                                          &iColumnPos,
                                          pRegEntry);
        BAIL_ON_REG_ERROR(dwError);

        sResultCount++;
    }

    if (dwError == SQLITE_DONE)
    {
        // No more results found
        dwError = LWREG_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    if (!sResultCount)
    {
        dwError = LWREG_ERROR_NO_SUCH_KEY;
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (!dwError && ppRegEntry)
    {
        *ppRegEntry = pRegEntry;
    }
    else
    {
        RegCacheSafeFreeEntry(&pRegEntry);
        if (ppRegEntry)
            *ppRegEntry = NULL;
    }

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);


    return dwError;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

DWORD
RegDbDeleteKey(
    IN REG_DB_HANDLE hDb,
    IN PCSTR pszKeyName
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteKey;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    dwError = RegSqliteBindString(pstQuery, 1, pszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LWREG_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:

    if (pstQuery)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

DWORD
RegDbQueryInfoKeyCount(
    IN REG_DB_HANDLE hDb,
    IN PCSTR pszKeyName,
    IN QueryKeyInfoOption queryType,
    OUT size_t* psCount
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
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
            dwError = LWREG_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegSqliteBindString(pstQuery, 1, pszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    while ((dwError = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LWREG_ERROR_DATA_ERROR;
            BAIL_ON_REG_ERROR(dwError);
        }

        if (sResultCount >= 1)
        {
            dwError = LWREG_ERROR_INTERNAL;
            BAIL_ON_REG_ERROR(dwError);
        }

        iColumnPos = 0;

        switch (queryType)
        {
            case QuerySubKeys:
                dwError = RegDbUnpackSubKeysCountInfo(pstQuery,
                                                      &iColumnPos,
                                                      &dwCount);
                BAIL_ON_REG_ERROR(dwError);

                break;

            case QueryValues:
                dwError = RegDbUnpackKeyValuesCountInfo(pstQuery,
                                                      &iColumnPos,
                                                      &dwCount);
                BAIL_ON_REG_ERROR(dwError);

                break;

            default:
                dwError = LWREG_ERROR_INVALID_PARAMETER;
                BAIL_ON_REG_ERROR(dwError);
        }

        sResultCount++;
    }

    if (dwError == SQLITE_DONE)
    {
        // No more results found
        dwError = LWREG_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    if (!sResultCount)
    {
        dwError = LWREG_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }

    *psCount = (size_t)dwCount;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    *psCount = 0;

    goto cleanup;
}

DWORD
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
    DWORD dwError = LWREG_ERROR_SUCCESS;
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
            dwError = LWREG_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegSqliteBindString(pstQuery, 1, pszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = RegSqliteBindInt64(pstQuery, 2, dwLimit);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = RegSqliteBindInt64(pstQuery, 3, dwOffset);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    while ((dwError = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LWREG_ERROR_DATA_ERROR;
            BAIL_ON_REG_ERROR(dwError);
        }

        if (sResultCount >= sResultCapacity)
        {
            sResultCapacity *= 2;
            sResultCapacity += 10;
            dwError = RegReallocMemory(
                            ppRegEntries,
                            (PVOID*)&ppRegEntries,
                            sizeof(PREG_ENTRY) * sResultCapacity);
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LW_RTL_ALLOCATE((PVOID*)&pRegEntry, REG_ENTRY, sizeof(*pRegEntry));
        BAIL_ON_REG_ERROR(dwError);

        iColumnPos = 0;

        dwError = RegDbUnpackCacheInfo(pstQuery,
                        &iColumnPos,
                        &pRegEntry->version);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegDbUnpackRegEntryInfo(pstQuery,
                                          &iColumnPos,
                                          pRegEntry);
        BAIL_ON_REG_ERROR(dwError);

        ppRegEntries[sResultCount] = pRegEntry;
        pRegEntry = NULL;
        sResultCount++;
    }

    if (dwError == SQLITE_DONE)
    {
        // No more results found
        dwError = LWREG_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

cleanup:
    if (!dwError)
    {
        if (pppRegEntries)
        {
            *pppRegEntries = ppRegEntries;
        }
        *psCount = sResultCount;
    }
    else
    {
        RegCacheSafeFreeEntry(&pRegEntry);
        RegCacheSafeFreeEntryList(sResultCount, &ppRegEntries);
        if (pppRegEntries)
        {
            *pppRegEntries = NULL;
        }
        *psCount = 0;
    }

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);


    return dwError;

error:
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

DWORD
RegDbDeleteKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PCSTR pszKeyName,
    IN PCSTR pszValueName
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstDeleteKeyValue;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    dwError = RegSqliteBindString(pstQuery, 1, pszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = RegSqliteBindString(pstQuery, 2, pszValueName);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LWREG_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:

    if (pstQuery)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

static
DWORD
RegDbFreePreparedStatements(
    IN OUT PREG_DB_CONNECTION pConn
    )
{
    int i;
    DWORD dwError = LWREG_ERROR_SUCCESS;
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
            dwError = sqlite3_finalize(*pppstFreeList[i]);
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            *pppstFreeList[i] = NULL;
        }
    }

cleanup:
    return dwError;

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
    DWORD dwError = LWREG_ERROR_SUCCESS;
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

    dwError = RegDbFreePreparedStatements(pConn);
    if (dwError != LWREG_ERROR_SUCCESS)
    {
        REG_LOG_ERROR("Error freeing prepared statements [%d]", dwError);
        dwError = LWREG_ERROR_SUCCESS;
    }

    if (pConn->pDb != NULL)
    {
        sqlite3_close(pConn->pDb);
        pConn->pDb = NULL;
    }

    dwError = pthread_rwlock_destroy(&pConn->lock);
    if (dwError != LWREG_ERROR_SUCCESS)
    {
        REG_LOG_ERROR("Error destroying lock [%d]", dwError);
        dwError = LWREG_ERROR_SUCCESS;
    }
    LWREG_SAFE_FREE_MEMORY(pConn);

    *phDb = (HANDLE)0;

cleanup:
    return;
}

DWORD
RegDbEmptyCache(
    IN REG_DB_HANDLE hDb
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    PREG_DB_CONNECTION pConn = (PREG_DB_CONNECTION)hDb;
    PCSTR pszEmptyCache =
        "begin;\n"
        "delete from " REG_DB_TABLE_NAME_CACHE_TAGS ";\n"
        "delete from " REG_DB_TABLE_NAME_ENTRIES ";\n"
        "end";

    dwError = RegSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
        pszEmptyCache);
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
RegDbFlushNOP(
    REG_DB_HANDLE hDb
    )
{
    return 0;
}

void
RegCacheSafeFreeEntryList(
    size_t sCount,
    PREG_ENTRY** pppEntries
    )
{
    if (*pppEntries != NULL)
    {
        size_t iEntry;
        for (iEntry = 0; iEntry < sCount; iEntry++)
        {
            RegCacheSafeFreeEntry(&(*pppEntries)[iEntry]);
        }
        LWREG_SAFE_FREE_MEMORY(*pppEntries);
    }
}

void
RegCacheSafeFreeEntry(
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

DWORD
RegCacheSafeRecordSubKeysInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    int iCount = 0;
    size_t sSubKeyLen = 0;
    PWSTR pSubKey = NULL;

    BAIL_ON_INVALID_POINTER(pKeyResult);

    //Remove previous subKey information if there is any
    RegFreeStringArray(pKeyResult->ppszSubKeyNames, pKeyResult->dwNumCacheSubKeys);

    if (!sCacheCount)
    {
        goto cleanup;
    }

    dwError = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppszSubKeyNames, PSTR, sizeof(*(pKeyResult->ppszSubKeyNames)) * sCacheCount);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < (DWORD)sCacheCount; iCount++)
    {
        dwError = LwRtlCStringDuplicate(&pKeyResult->ppszSubKeyNames[iCount], ppRegEntries[iCount]->pszKeyName);
        BAIL_ON_REG_ERROR(dwError);

        if (bDoAnsi)
        {
            sSubKeyLen = strlen(ppRegEntries[iCount]->pszKeyName);

            if (pKeyResult->sMaxSubKeyALen < sSubKeyLen)
                pKeyResult->sMaxSubKeyALen = sSubKeyLen;
        }
        else
        {
		dwError = LwRtlWC16StringAllocateFromCString(&pSubKey, ppRegEntries[iCount]->pszKeyName);
		BAIL_ON_REG_ERROR(dwError);

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
    return dwError;

error:
    goto cleanup;
}

DWORD
RegCacheSafeRecordSubKeysInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    dwError = RegCacheSafeRecordSubKeysInfo_inlock(sCount,
                                                   sCacheCount,
                                                   ppRegEntries,
                                                   pKeyResult,
                                                   bDoAnsi);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwError;

error:
    goto cleanup;
}

DWORD
RegCacheSafeRecordValuesInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    int iCount = 0;
    size_t sValueNameLen = 0;
    PWSTR pValueName = NULL;
    DWORD dwValueLen = 0;

    BAIL_ON_INVALID_POINTER(pKeyResult);

    //Remove previous subKey information if there is any
    RegFreeStringArray(pKeyResult->ppszValueNames, pKeyResult->dwNumCacheValues);
    RegFreeStringArray(pKeyResult->ppszValues, pKeyResult->dwNumCacheValues);
    LWREG_SAFE_FREE_MEMORY(pKeyResult->pTypes);

    if (!sCacheCount)
    {
        goto cleanup;
    }

    dwError = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppszValueNames, PSTR,
		                  sizeof(*(pKeyResult->ppszValueNames))* sCacheCount);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->ppszValues, PSTR,
		                  sizeof(*(pKeyResult->ppszValues))* sCacheCount);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LW_RTL_ALLOCATE((PVOID*)&pKeyResult->pTypes, REG_DATA_TYPE,
		                  sizeof(*(pKeyResult->pTypes))* sCacheCount);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < (DWORD)sCacheCount; iCount++)
    {
        dwError = LwRtlCStringDuplicate(&pKeyResult->ppszValueNames[iCount],
			                        ppRegEntries[iCount]->pszValueName);
        BAIL_ON_REG_ERROR(dwError);

        if (ppRegEntries[iCount]->pszValue)
        {
            dwError = LwRtlCStringDuplicate(&pKeyResult->ppszValues[iCount],
			                        ppRegEntries[iCount]->pszValue);
            BAIL_ON_REG_ERROR(dwError);
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
		dwError = LwRtlWC16StringAllocateFromCString(&pValueName,
				                                     pKeyResult->ppszValueNames[iCount]);
		BAIL_ON_REG_ERROR(dwError);

            if (pValueName)
            {
		sValueNameLen = RtlWC16StringNumChars(pValueName);
            }

            if (pKeyResult->sMaxValueNameLen < sValueNameLen)
                pKeyResult->sMaxValueNameLen = sValueNameLen;
        }

        dwError = GetValueAsBytes(ppRegEntries[iCount]->type,
                                  (PCSTR)ppRegEntries[iCount]->pszValue,
                                  bDoAnsi,
                                  NULL,
                                  &dwValueLen);
        BAIL_ON_REG_ERROR(dwError);

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
    return dwError;

error:
    goto cleanup;
}

DWORD
RegCacheSafeRecordValuesInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
    DWORD dwError = LWREG_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    dwError = RegCacheSafeRecordValuesInfo_inlock(sCount,
                                           sCacheCount,
                                           ppRegEntries,
                                           pKeyResult,
                                           bDoAnsi);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwError;

error:
    goto cleanup;
}
