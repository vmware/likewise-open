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
    DWORD dwError = LW_ERROR_SUCCESS;

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
    DWORD dwError = LW_ERROR_SUCCESS;

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

    dwError =RegSqliteReadUInt32(
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

    dwError = LwAllocateMemory(
                    sizeof(REG_DB_CONNECTION),
                    (PVOID*)&pConn);
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
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".Type = 21",
            -1, //search for null termination in szQuery to get length
            &pConn->pstQuerySubKeys,
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
                    "AND " REG_DB_TABLE_NAME_ENTRIES ".Type != 21",
            -1, //search for null termination in szQuery to get length
            &pConn->pstQueryValues,
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
    LW_SAFE_FREE_STRING(pszDbDir);

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
        LW_SAFE_FREE_MEMORY(pConn);
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
    DWORD dwError = LW_ERROR_SUCCESS;
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
            dwError = LW_ERROR_OUT_OF_MEMORY;
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
    DWORD dwError = LW_ERROR_SUCCESS;
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
                dwError = LW_ERROR_OUT_OF_MEMORY;
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
            dwError = LW_ERROR_OUT_OF_MEMORY;
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
        LW_SAFE_FREE_MEMORY(*pppEntries);
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

        LW_SAFE_FREE_STRING(pEntry->pszKeyName);
        LW_SAFE_FREE_STRING(pEntry->pszValueName);
        LW_SAFE_FREE_STRING(pEntry->pszValue);

        LW_SAFE_FREE_MEMORY(pEntry);
        *ppEntry = NULL;
    }
}

DWORD
RegCacheSafeRecordSubKeysInfo(
    IN size_t sCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    int iCount = 0;
    size_t sSubKeyLen = 0;
    PWSTR pSubKey = NULL;

    BAIL_ON_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    //Remove previous subKey information if there is any
    LwFreeStringArray(pKeyResult->ppszSubKeyNames, pKeyResult->dwNumSubKeys);

    pKeyResult->dwNumSubKeys = (DWORD)sCount;

    dwError = LwAllocateMemory(sizeof(*(pKeyResult->ppszSubKeyNames)) * sCount,
                               (PVOID*)&pKeyResult->ppszSubKeyNames);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < (DWORD)sCount; iCount++)
    {
        dwError = LwAllocateString(ppRegEntries[iCount]->pszKeyName, &pKeyResult->ppszSubKeyNames[iCount]);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwMbsToWc16s(ppRegEntries[iCount]->pszKeyName, &pSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwWc16sLen((PCWSTR)pSubKey,&sSubKeyLen);
        BAIL_ON_REG_ERROR(dwError);

        if (pKeyResult->sMaxSubKeyLen < sSubKeyLen)
            pKeyResult->sMaxSubKeyLen = sSubKeyLen;

        LW_SAFE_FREE_MEMORY(pSubKey);
        sSubKeyLen = 0;
    }

    pKeyResult->bHasSubKeyInfo = TRUE;

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    LW_SAFE_FREE_MEMORY(pSubKey);

    return dwError;

error:
    goto cleanup;
}

DWORD
RegCacheSafeRecordValuesInfo(
    IN size_t sCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    int iCount = 0;
    size_t sValueNameLen = 0;
    PWSTR pValueName = NULL;
    DWORD dwValueLen = 0;


    BAIL_ON_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    //Remove previous subKey information if there is any
    LwFreeStringArray(pKeyResult->ppszValueNames, pKeyResult->dwNumValues);

    pKeyResult->dwNumValues = (DWORD)sCount;

    dwError = LwAllocateMemory(sizeof(*(pKeyResult->ppszValueNames)) * sCount,
                               (PVOID*)&pKeyResult->ppszValueNames);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*(pKeyResult->ppszValues)) * sCount,
                               (PVOID*)&pKeyResult->ppszValues);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateMemory(sizeof(*(pKeyResult->pTypes)) * sCount,
                               (PVOID*)&pKeyResult->pTypes);
    BAIL_ON_REG_ERROR(dwError);


    for (iCount = 0; iCount < (DWORD)sCount; iCount++)
    {
        dwError = LwAllocateStringPrintf(&pKeyResult->ppszValueNames[iCount],
                                         "%s\\%s",
                                         ppRegEntries[iCount]->pszKeyName,
                                         ppRegEntries[iCount]->pszValueName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwAllocateString(ppRegEntries[iCount]->pszValue, &pKeyResult->ppszValues[iCount]);
        BAIL_ON_REG_ERROR(dwError);

        pKeyResult->pTypes[iCount] = ppRegEntries[iCount]->type;

        dwError = LwMbsToWc16s(pKeyResult->ppszValueNames[iCount], &pValueName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwWc16sLen((PCWSTR)pValueName,&sValueNameLen);
        BAIL_ON_REG_ERROR(dwError);

        dwError = GetValueAsBytes(ppRegEntries[iCount]->type,
                                  (PCSTR)ppRegEntries[iCount]->pszValue,
                                  FALSE,
                                  NULL,
                                  &dwValueLen);
        BAIL_ON_REG_ERROR(dwError);

        if (pKeyResult->sMaxValueNameLen < sValueNameLen)
            pKeyResult->sMaxValueNameLen = sValueNameLen;

        if (pKeyResult->sMaxValueLen < (size_t)dwValueLen)
            pKeyResult->sMaxValueLen = (size_t)dwValueLen;

        LW_SAFE_FREE_MEMORY(pValueName);
        sValueNameLen = 0;
        dwValueLen = 0;
    }

    pKeyResult->bHasValueInfo = TRUE;

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

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
    dwError = LwAllocateMemory(sizeof(*pRegEntry), (PVOID*)&pRegEntry);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateString(pszKeyName, &pRegEntry->pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateString("", &pRegEntry->pszValueName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateString("", &pRegEntry->pszValue);
    BAIL_ON_REG_ERROR(dwError);

    pRegEntry->type = REG_KEY;
    pRegEntry->version.qwDbId = -1;

    dwError = RegDbStoreKeyObjectEntries(
                 hDb,
                 1,
                 &pRegEntry);
    BAIL_ON_REG_ERROR(dwError);

    /*Create default key value*/
    dwError = RegDbCreateKeyValue(hDb,
                                  pszKeyName,
                                  "@",
                                  "",
                                  REG_SZ,
                                  NULL);
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

    dwError = LwAllocateMemory(sizeof(REG_ENTRY), (PVOID*)&pRegEntry);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateString(pszKeyName, &pRegEntry->pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateString(pszValueName, &pRegEntry->pszValueName);
    BAIL_ON_REG_ERROR(dwError);

    if (pszValue)
    {
        dwError = LwAllocateString(pszValue, &pRegEntry->pszValue);
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
    DWORD dwError = LW_ERROR_SUCCESS;
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
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_REG_ERROR(dwError);
        }

        if (sResultCount >= 1)
        {
            //Duplicate key value records are found
            dwError = LW_ERROR_DUPLICATE_KEYVALUENAME;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LwAllocateMemory(
                        sizeof(*pRegEntry),
                        (PVOID*)&pRegEntry);
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
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    if (!sResultCount)
    {
        dwError = LW_ERROR_NO_SUCH_VALUENAME;
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
    DWORD dwError = LW_ERROR_SUCCESS;
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
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_REG_ERROR(dwError);
        }

        if (sResultCount >= 1)
        {
            //Duplicate keys are found
            dwError = LW_ERROR_DUPLICATE_KEYNAME;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LwAllocateMemory(
                        sizeof(*pRegEntry),
                        (PVOID*)&pRegEntry);
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
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    if (!sResultCount)
    {
        dwError = LW_ERROR_NO_SUCH_KEY;
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
    DWORD dwError = LW_ERROR_SUCCESS;
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
        dwError = LW_ERROR_SUCCESS;
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
RegDbQueryInfoKey(
    IN REG_DB_HANDLE hDb,
    IN PCSTR pszKeyName,
    IN QueryKeyInfoOption queryType,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_ENTRY** pppRegEntries
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
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
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegSqliteBindString(pstQuery, 1, pszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    while ((dwError = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_REG_ERROR(dwError);
        }

        if (sResultCount >= sResultCapacity)
        {
            sResultCapacity *= 2;
            sResultCapacity += 10;
            dwError = LwReallocMemory(
                            ppRegEntries,
                            (PVOID*)&ppRegEntries,
                            sizeof(PREG_ENTRY) * sResultCapacity);
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LwAllocateMemory(
                        sizeof(*pRegEntry),
                        (PVOID*)&pRegEntry);
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
        dwError = LW_ERROR_SUCCESS;
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
    DWORD dwError = LW_ERROR_SUCCESS;
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
        dwError = LW_ERROR_SUCCESS;
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
    DWORD dwError = LW_ERROR_SUCCESS;
    sqlite3_stmt * * const pppstFreeList[] = {
        &pConn->pstDeleteKey,
        &pConn->pstDeleteKeyValue,
        &pConn->pstOpenKeyEx,
        &pConn->pstQueryKeyValue,
        &pConn->pstQueryKeyValueWithType,
        &pConn->pstQuerySubKeys,
        &pConn->pstQueryValues,
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
    DWORD dwError = LW_ERROR_SUCCESS;
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
    if (dwError != LW_ERROR_SUCCESS)
    {
        REG_LOG_ERROR("Error freeing prepared statements [%d]", dwError);
        dwError = LW_ERROR_SUCCESS;
    }

    if (pConn->pDb != NULL)
    {
        sqlite3_close(pConn->pDb);
        pConn->pDb = NULL;
    }

    dwError = pthread_rwlock_destroy(&pConn->lock);
    if (dwError != LW_ERROR_SUCCESS)
    {
        REG_LOG_ERROR("Error destroying lock [%d]", dwError);
        dwError = LW_ERROR_SUCCESS;
    }
    LW_SAFE_FREE_MEMORY(pConn);

    *phDb = (HANDLE)0;

cleanup:
    return;
}

DWORD
RegDbEmptyCache(
    IN REG_DB_HANDLE hDb
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
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

#if 0
DWORD
RegDbGetMultiKeyValues(
    IN REG_DB_HANDLE hDb,
    IN PSTR pszKeyName,
    IN PVALENT pVal_list,
    IN DWORD dwNumValuesRequest,
    OUT size_t* psCount,
    OUT PREG_ENTRY** pppRegEntry
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
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

    BAIL_ON_INVALID_STRING(pszKeyName);

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pstQuery = pConn->pstQueryMultiKeyValues;
    dwError = RegSqliteBindString(pstQuery, 1, pszKeyName);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    while ((dwError = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LW_ERROR_DATA_ERROR;
            BAIL_ON_REG_ERROR(dwError);
        }

        // stop processing when the number of values retrieved is no more than client's need
        if (sResultCount >= sResultCapacity && sResultCount <= dwNumValuesRequest)
        {
            sResultCapacity *= 2;
            sResultCapacity += 10;
            dwError = LwReallocMemory(
                            ppRegEntries,
                            (PVOID*)&ppRegEntries,
                            sizeof(PREG_ENTRY) * sResultCapacity);
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LwAllocateMemory(
                        sizeof(*pRegEntry),
                        (PVOID*)&pRegEntry);
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
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

cleanup:
    if (!dwError)
    {
        *pppRegEntries = ppRegEntries;
        *psCount = sResultCount;
    }
    else
    {
        RegCacheSafeFreeEntry(&pRegEntry);
        RegCacheSafeFreeEntryList(sResultCount, &ppRegEntries);
        *pppRegEntries = NULL;
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
RegDbCreateCacheTag(
    IN PREG_DB_CONNECTION pConn,
    IN time_t tLastUpdated,
    OUT int64_t *pqwCacheId
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstInsertCacheTag;
    int64_t qwDbId;

    dwError = RegSqliteBindInt64(pstQuery, 1, tLastUpdated);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    pstQuery = pConn->pstGetLastInsertedRow;

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        // The value is missing
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (dwError == SQLITE_ROW)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    if (sqlite3_column_count(pstQuery) != 1)
    {
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }

    qwDbId = sqlite3_column_int64(pstQuery, 0);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_ROW)
    {
        // Duplicate value
        dwError = LW_ERROR_DATA_ERROR;
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (dwError == SQLITE_DONE)
    {
        dwError = LW_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    *pqwCacheId = qwDbId;

cleanup:
    return dwError;

error:
    if (pstQuery)
    {
        sqlite3_reset(pstQuery);
    }
    *pqwCacheId = -1;
    goto cleanup;
}
#endif
