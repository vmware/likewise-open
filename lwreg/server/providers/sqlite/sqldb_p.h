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
 *        sqlcache_p.h
 *
 * Abstract:
 *
 *        Likewise Security and Authentication Subsystem (LSASS)
 *
 *        Private functions in sqlite3 Caching backend
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#ifndef __SQLCACHE_P_H__
#define __SQLCACHE_P_H__

#define REG_DB_FREE_UNUSED_CACHEIDS   \
    "delete from " REG_DB_TABLE_NAME_CACHE_TAGS " where CacheId NOT IN " \
        "CacheId NOT IN ( select CacheId from " REG_DB_TABLE_NAME_ENTRIES " );\n"

typedef struct _REG_DB_CONNECTION
{
    sqlite3 *pDb;
    pthread_rwlock_t lock;

    sqlite3_stmt *pstOpenKeyEx;
    sqlite3_stmt *pstDeleteKey;
    sqlite3_stmt *pstDeleteKeyValue;
    sqlite3_stmt *pstQuerySubKeys;
    sqlite3_stmt *pstQuerySubKeysCount;
    sqlite3_stmt *pstQueryValues;
    sqlite3_stmt *pstQueryValuesCount;
    sqlite3_stmt *pstQueryKeyValue;
    sqlite3_stmt *pstQueryKeyValueWithType;
    sqlite3_stmt *pstQueryKeyValueWithWrongType;
    sqlite3_stmt *pstQueryMultiKeyValues;
    sqlite3_stmt *pstCreateCacheId;
    sqlite3_stmt *pstDeleteCacheIdEntry;
    sqlite3_stmt *pstUpdateCacheIdEntry;
    sqlite3_stmt *pstCreateRegEntry;
    sqlite3_stmt *pstReplaceRegEntry;
    sqlite3_stmt *pstUpdateRegEntry;


} REG_DB_CONNECTION, *PREG_DB_CONNECTION;


NTSTATUS
RegDbUnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PREG_ENTRY_VERSION_INFO pResult
    );

NTSTATUS
RegDbUnpackRegEntryInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_ENTRY pResult
    );

NTSTATUS
RegDbUnpackSubKeysCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    );

NTSTATUS
RegDbUnpackKeyValuesCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    );

void
RegDbSafeFreeEntry(
    PREG_ENTRY* ppEntry
    );

void
RegDbSafeFreeEntryList(
    size_t sCount,
    PREG_ENTRY** pppEntries
    );

NTSTATUS
RegDbSafeRecordSubKeysInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbSafeRecordSubKeysInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbSafeRecordValuesInfo_inlock(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbSafeRecordValuesInfo(
    IN size_t sCount,
    IN size_t sCacheCount,
    IN PREG_ENTRY* ppRegEntries,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    );

NTSTATUS
RegDbOpen(
    IN PCSTR pszDbPath,
    OUT PREG_DB_HANDLE phDb
    );

NTSTATUS
RegDbStoreEntries(
    IN HANDLE hDB,
    IN DWORD dwEntryCount,
    IN PREG_ENTRY* ppEntries,
    IN OPTIONAL PBOOLEAN pbIsUpdate
    );

NTSTATUS
RegDbStoreObjectEntries(
    REG_DB_HANDLE hDb,
    size_t  sEntryCount,
    PREG_ENTRY* ppEntries
    );

NTSTATUS
RegDbCreateKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    OUT PREG_ENTRY* ppRegEntry
    );

NTSTATUS
RegDbOpenKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    );

NTSTATUS
RegDbDeleteKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName
    );

NTSTATUS
RegDbQueryInfoKey(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN QueryKeyInfoOption queryType,
    IN DWORD dwLimit,
    IN DWORD dwOffset,
    OUT size_t* psCount,
    OUT OPTIONAL PREG_ENTRY** pppRegEntries
    );

NTSTATUS
RegDbQueryInfoKeyCount(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN QueryKeyInfoOption queryType,
    OUT size_t* psSubKeyCount
    );

NTSTATUS
RegDbCreateKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    );

NTSTATUS
RegDbSetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN const PBYTE pValue,
    IN DWORD dwValueLen,
    IN REG_DATA_TYPE valueType,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    );

NTSTATUS
RegDbGetKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName,
    IN REG_DATA_TYPE valueType,
    IN OPTIONAL PBOOLEAN pbIsWrongType,
    OUT OPTIONAL PREG_ENTRY* ppRegEntry
    );

NTSTATUS
RegDbDeleteKeyValue(
    IN REG_DB_HANDLE hDb,
    IN PCWSTR pwszKeyName,
    IN PCWSTR pwszValueName
    );


void
RegDbSafeClose(
    PREG_DB_HANDLE phDb
    );

NTSTATUS
RegDbEmptyCache(
    IN REG_DB_HANDLE hDb
    );

NTSTATUS
RegDbFlushNOP(
    REG_DB_HANDLE hDb
    );

#endif /* __SQLCACHE_P_H__ */
