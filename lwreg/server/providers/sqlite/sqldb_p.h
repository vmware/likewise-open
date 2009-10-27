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

} REG_DB_CONNECTION, *PREG_DB_CONNECTION;

DWORD
RegDbUnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PREG_ENTRY_VERSION_INFO pResult
    );

DWORD
RegDbUnpackRegEntryInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PREG_ENTRY pResult
    );

DWORD
RegDbUnpackSubKeysCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    );

DWORD
RegDbUnpackKeyValuesCountInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PDWORD pdwCount
    );

#endif /* __SQLCACHE_P_H__ */
