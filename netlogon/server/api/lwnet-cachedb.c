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
 *        lwnet-cachedb.c
 *
 * Abstract:
 *
 *        Caching for Likewise Netlogon
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#include "includes.h"

#define LWNET_CACHE_DB_RETRY_WRITE_ATTEMPTS 20
#define LWNET_CACHE_DB_RETRY_WRITE_WAIT_MILLISECONDS 500

struct _LWNET_CACHE_DB_HANDLE_DATA {
    sqlite3* SqlHandle;
    // This RW lock helps us to ensure that we don't stomp
    // ourselves while giving up good parallel access.
    // Note, however, that SQLite might still return busy errors
    // if some other process is trying to poke at the database
    // (which might happen with database debugging or maintenance tools).
    pthread_rwlock_t Lock;
    pthread_rwlock_t* pLock;
    BOOLEAN CanWrite;
};

static LWNET_CACHE_DB_HANDLE gDbHandle;

// ISSUE-2008/07/01-dalmeida -- For now, use exlusive locking as we need to
// verify actual thread safety wrt things like error strings and such.
#define RW_LOCK_ACQUIRE_READ(Lock) \
    pthread_rwlock_wrlock(Lock)

#define RW_LOCK_RELEASE_READ(Lock) \
    pthread_rwlock_unlock(Lock)

#define RW_LOCK_ACQUIRE_WRITE(Lock) \
    pthread_rwlock_wrlock(Lock)

#define RW_LOCK_RELEASE_WRITE(Lock) \
    pthread_rwlock_unlock(Lock)

#ifdef DEBUG
#define LWNET_CACHE_DB_VERIFY_COLUMN_HEADER(Text, Table, ColumnCount, ColumnIndex) \
    ((strcmp(Text, LWNetCacheDbGetTableElement(Table, ColumnCount, 0, ColumnIndex))) ? LWNET_ERROR_DATA_ERROR : 0)
#else
#define LWNET_CACHE_DB_VERIFY_COLUMN_HEADER(Text, Table, ColumnCount, ColumnIndex) 0
#endif

DWORD
LWNetCacheDbOpen(
    IN PCSTR Path,
    IN BOOLEAN bIsWrite,
    OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    )
{
    DWORD dwError = 0;
    LWNET_CACHE_DB_HANDLE dbHandle = NULL;

    dwError = LWNetAllocateMemory(sizeof(*dbHandle), (PVOID *)&dbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

    // TODO-dalmeida-2008/06/30 -- Convert error code
    dwError = pthread_rwlock_init(&dbHandle->Lock, NULL);
    BAIL_ON_LWNET_ERROR(dwError);

    dbHandle->pLock = &dbHandle->Lock;

    // Note that this will create the database if it does not already
    // exist.

    // TODO-dalmeida-2008/06/30 -- Convert error code
    if (bIsWrite)
    {
        dwError = sqlite3_open(Path, &dbHandle->SqlHandle);
        BAIL_ON_LWNET_ERROR(dwError);
        dbHandle->CanWrite = TRUE;
    }
    else
    {
        dwError = sqlite3_open_v2(Path, &dbHandle->SqlHandle,
                                  SQLITE_OPEN_READONLY, NULL);
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    if (dwError)
    {
        LWNetCacheDbClose(&dbHandle);
    }    
    *pDbHandle = dbHandle;
    return dwError;
}

VOID
LWNetCacheDbClose(
    IN OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    )
{
    LWNET_CACHE_DB_HANDLE dbHandle = *pDbHandle;

    if (dbHandle)
    {
        if (dbHandle->pLock)
        {
            pthread_rwlock_destroy(dbHandle->pLock);
        }
        if (dbHandle->SqlHandle)
        {
            sqlite3_close(dbHandle->SqlHandle);
        }
        *pDbHandle = NULL;
    }
}

DWORD
LWNetCacheDbSetup(
    IN LWNET_CACHE_DB_HANDLE DbHandle
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = sqlite3_exec(DbHandle->SqlHandle,
                           LWNET_CACHEDB_SQL_SETUP,
                           NULL,
                           NULL,
                           &pszError);
    if (dwError)
    {
        LWNET_LOG_DEBUG("SQL failed: code = %d, message = '%s'\nSQL =\n%s",
                        dwError, pszError, LWNET_CACHEDB_SQL_SETUP);
    }
    BAIL_ON_LWNET_ERROR(dwError);

error:
    SQLITE3_SAFE_FREE_STRING(pszError);
    return dwError;
}

static
LWNET_CACHE_DB_QUERY_TYPE
LWNetCacheDbQueryToQueryType(
    IN DWORD dwDsFlags
    )
{
    LWNET_CACHE_DB_QUERY_TYPE result = LWNET_CACHE_DB_QUERY_TYPE_DC;
    if (dwDsFlags & DS_PDC_REQUIRED)
    {
        result = LWNET_CACHE_DB_QUERY_TYPE_PDC;
    }
    if (dwDsFlags & DS_GC_SERVER_REQUIRED)
    {
        result = LWNET_CACHE_DB_QUERY_TYPE_GC;
    }
    return result;
}

DWORD
LWNetCacheDbQuery(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PLWNET_UNIX_TIME_T LastDiscovered,
    OUT PLWNET_UNIX_TIME_T LastPinged
    )
{
    DWORD dwError = 0;
    LWNET_CACHE_DB_QUERY_TYPE queryType = 0;
    PSTR pszSql = NULL;
    PSTR* pszResultTable = NULL;
    int rowCount = 0;
    int columnCount = 0;
    PSTR errorMessage = NULL;
    BOOLEAN isAcquired = FALSE;
    int columnIndex = 0;
    const int expectedColumns = 18;
    PLWNET_DC_INFO pDcInfo = NULL;
    LWNET_UNIX_TIME_T lastDiscovered = 0;
    LWNET_UNIX_TIME_T lastPinged = 0;
    PCSTR valueString = NULL;
    PSTR pszGuid = NULL;
    PBYTE pGuidBytes = NULL;
    DWORD dwGuidByteCount = 0;

    queryType = LWNetCacheDbQueryToQueryType(dwDsFlags);

    pszSql = sqlite3_mprintf("SELECT "
                             "LastPinged, "
                             "LastDiscovered, "
                             "PingTime, "
                             "DomainControllerAddressType, "
                             "Flags, "
                             "Version, "
                             "LMToken, "
                             "NTToken, "
                             "DomainControllerName, "
                             "DomainControllerAddress, "
                             "DomainGUID, "
                             "NetBIOSDomainName, "
                             "FullyQualifiedDomainName, "
                             "DnsForestName, "
                             "DCSiteName, "
                             "ClientSiteName, "
                             "NetBIOSHostName, "
                             "UserName "
                             "FROM " NETLOGON_DB_TABLE_NAME " "
                             "WHERE "
                             "QueryDnsDomainName = trim(lower(%Q)) AND "
                             "QuerySiteName = trim(lower(%Q)) AND "
                             "QueryType = %d ",
                             pszDnsDomainName,
                             pszSiteName ? pszSiteName : "",
                             queryType);
    if (!pszSql)
    {
        // This must have been a memory issue -- note that we cannot query
        // a handle since no handle is passed in sqlite3_mprintf.
        dwError = LWNET_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    RW_LOCK_ACQUIRE_READ(DbHandle->pLock);
    isAcquired = TRUE;

    // TODO-dalmeida-2008/07/01 -- Error code conversion
    dwError = (DWORD)sqlite3_get_table(DbHandle->SqlHandle,
                                       pszSql,
                                       &pszResultTable,
                                       &rowCount,
                                       &columnCount,
                                       &errorMessage);
    BAIL_ON_SQLITE3_ERROR(dwError, errorMessage);

    RW_LOCK_RELEASE_READ(DbHandle->pLock);
    isAcquired = FALSE;

    if (rowCount <= 0)
    {
        // Nothing found in the cache, so we return success, but no data.
        dwError = 0;
        goto error;
    }

    if (rowCount != 1)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    if (columnCount != expectedColumns)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    columnIndex = 0;

    dwError = LWNetAllocateMemory(sizeof(*pDcInfo), (PVOID*)&pDcInfo);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("LastPinged",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteInt64(valueString, &lastPinged);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("LastDiscovered",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteInt64(valueString, &lastDiscovered);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("PingTime",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    // TODO -- handle negative caching -- also make sure callers handle it...
    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteUInt32(valueString, &pDcInfo->dwPingTime);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("DomainControllerAddressType",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteUInt32(valueString, &pDcInfo->dwDomainControllerAddressType);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("Flags",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteUInt32(valueString, &pDcInfo->dwFlags);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("Version",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteUInt32(valueString, &pDcInfo->dwVersion);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("LMToken",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteUInt16(valueString, &pDcInfo->wLMToken);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("NTToken",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteUInt16(valueString, &pDcInfo->wNTToken);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("DomainControllerName",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pDcInfo->pszDomainControllerName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("DomainControllerAddress",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pDcInfo->pszDomainControllerAddress);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("DomainGUID",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pszGuid);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetHexStrToByteArray(pszGuid, &pGuidBytes, &dwGuidByteCount);
    BAIL_ON_LWNET_ERROR(dwError);

    if (sizeof(pDcInfo->pucDomainGUID) != dwGuidByteCount)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    memcpy(pDcInfo->pucDomainGUID, pGuidBytes, sizeof(pDcInfo->pucDomainGUID));

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("NetBIOSDomainName",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pDcInfo->pszNetBIOSDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("FullyQualifiedDomainName",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pDcInfo->pszFullyQualifiedDomainName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("DnsForestName",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pDcInfo->pszDnsForestName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("DCSiteName",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pDcInfo->pszDCSiteName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("ClientSiteName",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pDcInfo->pszClientSiteName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("NetBIOSHostName",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pDcInfo->pszNetBIOSHostName);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER("UserName",
                                                  pszResultTable,
                                                  columnCount,
                                                  columnIndex);
    BAIL_ON_LWNET_ERROR(dwError);

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, columnIndex++);
    dwError = LWNetCacheDbReadSqliteString(valueString, &pDcInfo->pszUserName);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (isAcquired)
    {
        RW_LOCK_RELEASE_READ(DbHandle->pLock);
    }
    if (dwError)
    {
        LWNET_SAFE_FREE_DC_INFO(pDcInfo);
        lastPinged = 0;
        lastDiscovered = 0;
    }
    SQLITE3_SAFE_FREE_STRING(pszSql);
    SQLITE3_SAFE_FREE_STRING(errorMessage);
    if (pszResultTable)
    {
        sqlite3_free_table(pszResultTable);
    }
    LWNET_SAFE_FREE_STRING(pszGuid);
    LWNET_SAFE_FREE_MEMORY(pGuidBytes);

    *ppDcInfo = pDcInfo;
    *LastDiscovered = lastDiscovered;
    *LastPinged = lastPinged;

    return dwError;
}

DWORD
LWNetCacheDbUpdate(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    IN OPTIONAL PLWNET_UNIX_TIME_T LastDiscovered,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    DWORD dwError = 0;
    PSTR pszSql = NULL;
    PSTR pszDomainGUIDOctetString = NULL;
    LWNET_UNIX_TIME_T now = 0;
    LWNET_CACHE_DB_QUERY_TYPE queryType = 0;

    dwError = LWNetByteArrayToHexStr(pDcInfo->pucDomainGUID,
                                     sizeof(pDcInfo->pucDomainGUID),
                                     &pszDomainGUIDOctetString);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetGetSystemTime(&now);
    BAIL_ON_LWNET_ERROR(dwError);

    queryType = LWNetCacheDbQueryToQueryType(dwDsFlags);

    pszSql = sqlite3_mprintf("REPLACE INTO " NETLOGON_DB_TABLE_NAME " ("
                             "QueryDnsDomainName, "
                             "QuerySiteName, "
                             "QueryType, "
                             "LastDiscovered, "
                             "LastPinged, "
                             "PingTime, "
                             "DomainControllerAddressType, "
                             "Flags, "
                             "Version, "
                             "LMToken, "
                             "NTToken, "
                             "DomainControllerName, "
                             "DomainControllerAddress, "
                             "DomainGUID, "
                             "NetBIOSDomainName, "
                             "FullyQualifiedDomainName, "
                             "DnsForestName, "
                             "DCSiteName, "
                             "ClientSiteName, "
                             "NetBIOSHostName, "
                             "UserName"
                             ") VALUES ("
                             "trim(lower(%Q)), " // QueryDnsDomainName
                             "trim(lower(%Q)), " // QuerySiteName
                             "%u, " // QueryType
                             "%lld, " // LastDiscovered
                             "%lld, " // LastPinged
                             "%u, " // PingTime
                             "%u, " // DomainControllerAddressType
                             "%u, " // Flags
                             "%u, " // Version
                             "%u, " // LMToken
                             "%u, " // NTToken
                             "%Q, " // DomainControllerName
                             "%Q, " // DomainControllerAddress
                             "%Q, " // DomainGUID
                             "%Q, " // NetBIOSDomainName
                             "%Q, " // FullyQualifiedDomainName
                             "%Q, " // DnsForestName
                             "%Q, " // DCSiteName
                             "%Q, " // ClientSiteName
                             "%Q, " // NetBIOSHostName
                             "%Q" // UserName
                             ");\n",
                             pszDnsDomainName,
                             pszSiteName ? pszSiteName : "",
                             queryType,
                             LastDiscovered ? *LastDiscovered : now,
                             now,
                             pDcInfo->dwPingTime,
                             pDcInfo->dwDomainControllerAddressType,
                             pDcInfo->dwFlags,
                             pDcInfo->dwVersion,
                             pDcInfo->wLMToken,
                             pDcInfo->wNTToken,
                             pDcInfo->pszDomainControllerName,
                             pDcInfo->pszDomainControllerAddress,
                             pszDomainGUIDOctetString,
                             pDcInfo->pszNetBIOSDomainName,
                             pDcInfo->pszFullyQualifiedDomainName,
                             pDcInfo->pszDnsForestName,
                             pDcInfo->pszDCSiteName,
                             pDcInfo->pszClientSiteName,
                             pDcInfo->pszNetBIOSHostName,
                             pDcInfo->pszUserName);
    if (!pszSql)
    {
        // This must have been a memory issue -- note that we cannot query
        // a handle since no handle is passed in sqlite3_mprintf.
        dwError = LWNET_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetCacheDbExecWithRetry(DbHandle, pszSql);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    SQLITE3_SAFE_FREE_STRING(pszSql);
    LWNET_SAFE_FREE_STRING(pszDomainGUIDOctetString);

    return dwError;
}

DWORD
LWNetCacheDbScavenge(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    )
{
    DWORD dwError = 0;
    PSTR pszSql = NULL;
    LWNET_UNIX_TIME_T now = 0;
    LWNET_UNIX_TIME_T positiveTimeLimit = 0;

    dwError = LWNetGetSystemTime(&now);
    positiveTimeLimit = now + PositiveCacheAge;

    // ISSUE-2008/07/01-dalmeida -- Add negative cache login when
    // adding negative caching.

    pszSql = sqlite3_mprintf("DELETE FROM " NETLOGON_DB_TABLE_NAME " "
                             "WHERE "
                             "LastPinged < %llu \n",
                             positiveTimeLimit);
    if (!pszSql)
    {
        // This must have been a memory issue -- note that we cannot query
        // a handle since no handle is passed in sqlite3_mprintf.
        dwError = LWNET_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetCacheDbExecWithRetry(DbHandle, pszSql);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    SQLITE3_SAFE_FREE_STRING(pszSql);
    return dwError;
}

DWORD
LWNetCacheDbExport(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    OUT PLWNET_CACHE_DB_ENTRY* ppEntries,
    OUT PDWORD pdwCount
    )
{
    return LWNET_ERROR_NOT_IMPLEMENTED;
}

DWORD
LWNetCacheDbUpdateKrb5(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN PCSTR* pServerAddressArray,
    IN DWORD dwServerAddressCount
    )
{
    DWORD dwError = 0;
    PSTR pszSql = NULL;
    LWNET_UNIX_TIME_T now = 0;
    DWORD i = 0;
    DWORD dwSize = 0;
    PSTR pszServerList = NULL;
    PCSTR pszInput = NULL;
    PSTR pszOutput = NULL;

    dwError = LWNetGetSystemTime(&now);
    BAIL_ON_LWNET_ERROR(dwError);

    dwSize = 0;
    for (i = 0; i < dwServerAddressCount; i++)
    {
        dwSize += strlen(pServerAddressArray[i]) + 1;
    }

    dwError = LWNetAllocateMemory(dwSize, (PVOID*)&pszServerList);
    BAIL_ON_LWNET_ERROR(dwError);

    pszOutput = pszServerList;
    for (i = 0; i < dwServerAddressCount; i++)
    {
        pszInput = pServerAddressArray[i];
        while (*pszInput)
        {
            if (' ' == *pszInput)
            {
                LWNET_LOG_ERROR("Unexpected space in krb5 server '%s' "
                                "for domain '%s'", pServerAddressArray[i],
                                pszDnsDomainName);
                dwError = LWNET_ERROR_DATA_ERROR;
                BAIL_ON_LWNET_ERROR(dwError);
            }
            pszOutput[0] = pszInput[0];
            pszOutput++;
            pszInput++;
        }
        pszOutput[0] = ' ';
        pszOutput++;
    }
    pszOutput--;
    pszOutput[0] = 0;

    pszSql = sqlite3_mprintf("REPLACE INTO " NETLOGON_KRB5_DB_TABLE_NAME " ("
                             "Realm, "
                             "LastUpdated, "
                             "ServerCount, "
                             "ServerList"
                             ") VALUES ("
                             "trim(upper(%Q)), " // Realm
                             "%lld, " // LastUpdated
                             "%u, " // ServerCount
                             "trim(%Q)" // ServerList
                             ");\n",
                             pszDnsDomainName,
                             now,
                             dwServerAddressCount,
                             pszServerList);
    if (!pszSql)
    {
        // This must have been a memory issue -- note that we cannot query
        // a handle since no handle is passed in sqlite3_mprintf.
        dwError = LWNET_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetCacheDbExecWithRetry(DbHandle, pszSql);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    SQLITE3_SAFE_FREE_STRING(pszSql);
    LWNET_SAFE_FREE_STRING(pszServerList);

    return dwError;
}

DWORD
LWNetCacheDbScavengeKrb5(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN LWNET_UNIX_TIME_T Age
    )
{
    DWORD dwError = 0;
    PSTR pszSql = NULL;
    LWNET_UNIX_TIME_T now = 0;
    LWNET_UNIX_TIME_T timeLimit = 0;

    dwError = LWNetGetSystemTime(&now);
    timeLimit = now + Age;

    pszSql = sqlite3_mprintf("DELETE FROM " NETLOGON_KRB5_DB_TABLE_NAME " "
                             "WHERE "
                             "LastUpdated < %llu \n",
                             timeLimit);
    if (!pszSql)
    {
        // This must have been a memory issue -- note that we cannot query
        // a handle since no handle is passed in sqlite3_mprintf.
        dwError = LWNET_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwError = LWNetCacheDbExecWithRetry(DbHandle, pszSql);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    SQLITE3_SAFE_FREE_STRING(pszSql);
    return dwError;
}

DWORD
LWNetCacheDbExportKrb5(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    OUT PLWNET_CACHE_DB_KRB5_ENTRY* ppEntries,
    OUT PDWORD pdwCount
    )
{
    DWORD dwError = 0;
    PSTR* pszResultTable = NULL;
    int rowCount = 0;
    int columnCount = 0;
    PSTR errorMessage = NULL;
    BOOLEAN isAcquired = FALSE;
    int rowIndex = 0;
    int columnIndex = 0;
    const int expectedColumns = 4;
    PCSTR valueString = NULL;
#ifdef DEBUG
    PCSTR columnHeaders[] = { "Realm", "LastUpdated", "ServerCount", "ServerList" };
#endif
    DWORD dwEntryCount = 0;
    DWORD dwServerCountTotal = 0;
    DWORD dwMaxTotalStringSize = 0;
    DWORD dwEntriesSize = 0;
    DWORD dwServersSize = 0;
    DWORD dwTotalSize = 0;
    PSTR* ppStringPointers = NULL;
    PSTR pszStrings = NULL;
    PLWNET_CACHE_DB_KRB5_ENTRY pEntries = NULL;

    RW_LOCK_ACQUIRE_READ(DbHandle->pLock);
    isAcquired = TRUE;

    dwError = LWNetCacheDbExecQueryCountExpression(DbHandle->SqlHandle,
                                                   "SELECT count(*) FROM "
                                                   NETLOGON_KRB5_DB_TABLE_NAME,
                                                   &dwEntryCount);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetCacheDbExecQueryCountExpression(DbHandle->SqlHandle,
                                                   "SELECT sum(ServerCount) FROM "
                                                   NETLOGON_KRB5_DB_TABLE_NAME,
                                                   &dwServerCountTotal);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetCacheDbExecQueryCountExpression(DbHandle->SqlHandle,
                                                   "SELECT "
                                                   "sum(length(Realm)) + count(Realm) "
                                                   "+ "
                                                   "sum(length(ServerList)) + count(ServerList) "
                                                   "FROM "
                                                   NETLOGON_KRB5_DB_TABLE_NAME,
                                                   &dwMaxTotalStringSize);
    BAIL_ON_LWNET_ERROR(dwError);

    // TODO-dalmeida-2008/07/01 -- Error code conversion
    dwError = (DWORD)sqlite3_get_table(DbHandle->SqlHandle,
                                       LWNET_CACHEDB_SQL_DUMP_NETLOGON_KRB5_DB_TABLE,
                                       &pszResultTable,
                                       &rowCount,
                                       &columnCount,
                                       &errorMessage);
    BAIL_ON_SQLITE3_ERROR(dwError, errorMessage);

    RW_LOCK_RELEASE_READ(DbHandle->pLock);
    isAcquired = FALSE;

    if (rowCount <= 0)
    {
        // Nothing found in the cache, so we return success, but no data.
        dwError = 0;
        goto error;
    }

    if (columnCount != expectedColumns)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    for (columnIndex = 0; columnIndex < columnCount; columnIndex++)
    {
        dwError = LWNET_CACHE_DB_VERIFY_COLUMN_HEADER(columnHeaders[columnIndex],
                                                      pszResultTable,
                                                      columnCount,
                                                      columnIndex++);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    dwEntriesSize = dwEntryCount * sizeof(pEntries[0]);
    dwServersSize = dwServerCountTotal * sizeof(pEntries[0].ServerAddressArray[0]);
    dwTotalSize = dwMaxTotalStringSize + dwEntriesSize + dwServersSize;

    dwError = LWNetAllocateMemory(dwTotalSize, (PVOID*)&pEntries);
    BAIL_ON_LWNET_ERROR(dwError);

    ppStringPointers = (PSTR*) CT_PTR_ADD(pEntries, dwEntriesSize);
    pszStrings = CT_PTR_ADD(pEntries, dwEntriesSize + dwServersSize);

    for (rowIndex = 1; rowIndex <= rowCount; rowIndex++)
    {
        PLWNET_CACHE_DB_KRB5_ENTRY pEntry = &pEntries[rowIndex-1];

        columnIndex = 0;

        valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, rowIndex, columnIndex++);
        dwError = LWNetCacheDbReadSqliteStringIntoBuffer(valueString,
                                                         CT_PTR_OFFSET(pszStrings, CT_PTR_ADD(pEntries, dwTotalSize)),
                                                         pszStrings,
                                                         &pEntry->Realm,
                                                         &pszStrings);
        BAIL_ON_LWNET_ERROR(dwError);

        valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, rowIndex, columnIndex++);
        dwError = LWNetCacheDbReadSqliteInt64(valueString, &pEntry->LastUpdated);
        BAIL_ON_LWNET_ERROR(dwError);

        valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, rowIndex, columnIndex++);
        dwError = LWNetCacheDbReadSqliteUInt32(valueString, &pEntry->ServerAddressCount);
        BAIL_ON_LWNET_ERROR(dwError);

        if (pEntry->ServerAddressCount > 0)
        {
            pEntry->ServerAddressArray = ppStringPointers;
            ppStringPointers += pEntry->ServerAddressCount;

            // First, read the server address into the first location.
            valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, rowIndex, columnIndex++);
            dwError = LWNetCacheDbReadSqliteStringIntoBuffer(valueString,
                                                             CT_PTR_OFFSET(pszStrings, CT_PTR_ADD(pEntries, dwTotalSize)),
                                                             pszStrings,
                                                             &pEntry->ServerAddressArray[0],
                                                             &pszStrings);
            BAIL_ON_LWNET_ERROR(dwError);

            if (pEntry->ServerAddressArray[0])
            {
                PSTR pszServerAddress = pEntry->ServerAddressArray[0];
                DWORD dwServerIndex = 0;

                while (pszServerAddress[0])
                {
                    if (' ' == pszServerAddress[0])
                    {
                        pszServerAddress[0] = 0;
                        pEntry->ServerAddressArray[++dwServerIndex] = pszServerAddress + 1;
                    }
                    pszServerAddress++;
                }
            }
        }
    }

    // ISSUE-2008/07/07-dalmeida -- Add proper ASSERT macros
    if (ppStringPointers > (PSTR*)CT_PTR_ADD(pEntries, dwEntriesSize + dwServersSize))
    {
        LWNET_LOG_ALWAYS("Failed ASSERT at %s:%d", __FILE__, __LINE__);
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    if (pszStrings > CT_PTR_ADD(pEntries, dwTotalSize))
    {
        LWNET_LOG_ALWAYS("Failed ASSERT at %s:%d", __FILE__, __LINE__);
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    if (isAcquired)
    {
        RW_LOCK_RELEASE_READ(DbHandle->pLock);
    }
    SQLITE3_SAFE_FREE_STRING(errorMessage);
    if (pszResultTable)
    {
        sqlite3_free_table(pszResultTable);
    }

    if (dwError)
    {
        LWNET_SAFE_FREE_MEMORY(pEntries);
        dwEntryCount = 0;
    }

    *ppEntries = pEntries;
    *pdwCount = dwEntryCount;

    return dwError;
}

DWORD
LWNetCacheInitialize(
    )
{
    DWORD dwError = 0;
    BOOLEAN bExists = FALSE;

    dwError = LWNetCheckDirectoryExists(NETLOGON_DB_DIR, &bExists);
    BAIL_ON_LWNET_ERROR(dwError);

    // We are securing the cache dir and file to just root
    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        /* Allow go+rx to the base cache folder */
        dwError = LWNetCreateDirectory(NETLOGON_DB_DIR, cacheDirMode);
        BAIL_ON_LWNET_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LWNetChangeOwnerAndPermissions(NETLOGON_DB_DIR, 0, 0, S_IRWXU);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetCacheDbOpen(NETLOGON_DB, TRUE, &gDbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

    // Make sure that we are secured
    dwError = LWNetChangePermissions(NETLOGON_DB, S_IRWXU);
    BAIL_ON_LWNET_ERROR(dwError);

    dwError = LWNetCacheDbSetup(gDbHandle);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        LWNetCacheCleanup();
    }
    return dwError;
}

VOID
LWNetCacheCleanup(
    )
{
    LWNetCacheDbClose(&gDbHandle);
}

DWORD
LWNetCacheQuery(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PLWNET_UNIX_TIME_T LastDiscovered,
    OUT PLWNET_UNIX_TIME_T LastPinged
    )
{
    return LWNetCacheDbQuery(gDbHandle,
                             pszDnsDomainName, pszSiteName, dwDsFlags,
                             ppDcInfo, LastDiscovered, LastPinged);
}


DWORD
LWNetCacheUpdatePing(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    IN LWNET_UNIX_TIME_T LastDiscovered,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LWNetCacheDbUpdate(gDbHandle,
                              pszDnsDomainName, pszSiteName, dwDsFlags,
                              &LastDiscovered, pDcInfo);
}

DWORD
LWNetCacheUpdateDiscover(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    IN PLWNET_DC_INFO pDcInfo
    )
{
    return LWNetCacheDbUpdate(gDbHandle,
                              pszDnsDomainName, pszSiteName, dwDsFlags,
                              NULL, pDcInfo);
}

DWORD
LWNetCacheScavenge(
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    )
{
    return LWNetCacheDbScavenge(gDbHandle, PositiveCacheAge, NegativeCacheAge);
}

DWORD
LWNetCacheUpdateKrb5(
    IN PCSTR pszDnsDomainName,
    IN PCSTR* pServerAddressArray,
    IN DWORD dwServerAddressCount
    )
{
    return LWNetCacheDbUpdateKrb5(gDbHandle, pszDnsDomainName,
                                  pServerAddressArray,
                                  dwServerAddressCount);
}

DWORD
LWNetCacheSavengeKrb5(
    IN LWNET_UNIX_TIME_T Age
    )
{
    return LWNetCacheDbScavengeKrb5(gDbHandle, Age);
}

DWORD
LWNetCacheExportKrb5(
    OUT PLWNET_CACHE_DB_KRB5_ENTRY* ppEntries,
    OUT PDWORD pdwCount
    )
{
    return LWNetCacheDbExportKrb5(gDbHandle, ppEntries, pdwCount);
}

DWORD
LWNetCacheDbExecQueryCountExpression(
    IN sqlite3* SqlHandle,
    IN PCSTR pszSql,
    OUT PDWORD pdwCount
    )
{
    DWORD dwError = 0;
    DWORD dwCount = 0;
    PSTR* pszResultTable = NULL;
    int rowCount = 0;
    int columnCount = 0;
    PSTR errorMessage = NULL;
    PCSTR valueString = NULL;

    // NOTE: Caller must have already locked the handle.

    // TODO-dalmeida-2008/07/01 -- Error code conversion
    dwError = (DWORD)sqlite3_get_table(SqlHandle,
                                       pszSql,
                                       &pszResultTable,
                                       &rowCount,
                                       &columnCount,
                                       &errorMessage);
    BAIL_ON_SQLITE3_ERROR(dwError, errorMessage);

    if (rowCount <= 0)
    {
        // Nothing found in the cache, so we return success, but no data.
        dwError = 0;
        goto error;
    }

    if (columnCount != 1)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    valueString = LWNetCacheDbGetTableElement(pszResultTable, columnCount, 1, 0);
    dwError = LWNetCacheDbReadSqliteUInt32(valueString, &dwCount);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        dwCount = 0;
    }

    SQLITE3_SAFE_FREE_STRING(errorMessage);
    if (pszResultTable)
    {
        sqlite3_free_table(pszResultTable);
    }

    *pdwCount = dwCount;

    return dwError;
}

DWORD
LWNetCacheDbExecWithRetry(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszSql
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PSTR pszError = NULL;
    DWORD dwRetry = 0;
    BOOLEAN isAcquired = FALSE;

    for (dwRetry = 0; dwRetry < LWNET_CACHE_DB_RETRY_WRITE_ATTEMPTS; dwRetry++)
    {
        RW_LOCK_ACQUIRE_WRITE(DbHandle->pLock);
        isAcquired = TRUE;

        dwError = sqlite3_exec(DbHandle->SqlHandle,
                               pszSql,
                               NULL,
                               NULL,
                               &pszError);
        if (dwError == SQLITE_BUSY)
        {
            SQLITE3_SAFE_FREE_STRING(pszError);

            LWNET_LOG_ERROR("There is a conflict trying to access the "
                            "cache database.  This would happen if another "
                            "process is trying to access it.  Retrying...");

            dwError = 0;
            // sqlite3_exec runs pszSetFullEntry statement by statement. If
            // it fails, it leaves the sqlite VM with half of the transaction
            // finished. This rollsback the transaction so it can be retried
            // in entirety.
            sqlite3_exec(DbHandle->SqlHandle,
                         "ROLLBACK",
                         NULL,
                         NULL,
                         NULL);
            // ISSUE-2008/07/01-dalmeida -- Do we need to check for rollback error?

            RW_LOCK_RELEASE_WRITE(DbHandle->pLock);
            isAcquired = FALSE;

            dwError = LWNetSleepInMs(LWNET_CACHE_DB_RETRY_WRITE_WAIT_MILLISECONDS);
            BAIL_ON_LWNET_ERROR(dwError);

            continue;
        }
        BAIL_ON_SQLITE3_ERROR(dwError, pszError);
        break;
    }

error:
    if (isAcquired)
    {
        RW_LOCK_RELEASE_WRITE(DbHandle->pLock);
    }
    SQLITE3_SAFE_FREE_STRING(pszError);
    return dwError;
}


PCSTR
LWNetCacheDbGetTableElement(
    IN PSTR* Table,
    IN int Width,
    IN int Row,
    IN int Column
    )
{
    return Column < Width ? Table[Row * Width + Column] : NULL;
}

DWORD
LWNetCacheDbReadSqliteBoolean(
    IN PCSTR pszValue,
    OUT PBOOLEAN pResult
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    DWORD dwValue = 0;

    dwError = LWNetCacheDbReadSqliteUInt32(pszValue, &dwValue);
    BAIL_ON_LWNET_ERROR(dwError);

error:
    if (dwError)
    {
        dwValue = 0;
    }
    *pResult = (dwValue != 0) ? TRUE : FALSE;
    return dwError;
}

DWORD
LWNetCacheDbReadSqliteUInt16(
    IN PCSTR pszValue,
    OUT PWORD pResult
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    DWORD dwValue = 0;

    dwError = LWNetCacheDbReadSqliteUInt32(pszValue, &dwValue);
    BAIL_ON_LWNET_ERROR(dwError);

    if (dwValue != (dwValue & 0xFFFF))
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }

error:
    if (dwError)
    {
        dwValue = 0;
    }
    *pResult = (WORD) dwValue;
    return dwError;
}

DWORD
LWNetCacheDbReadSqliteUInt32(
    IN PCSTR pszValue,
    OUT PDWORD pResult
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PSTR pszEndPtr = NULL;
    unsigned long int value = 0;
    DWORD result = 0;

    errno = 0;
    value = strtoul(pszValue, &pszEndPtr, 10);
    dwError = errno;
    if (!pszEndPtr || pszEndPtr == pszValue || *pszEndPtr || dwError)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    if (value < 0 || value > DWORD_MAX)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    result = (DWORD)value;

error:
    if (dwError)
    {
        result = 0;
    }
    *pResult = result;
    return dwError;
}

DWORD
LWNetCacheDbReadSqliteUInt64(
    IN PCSTR pszValue,
    OUT uint64_t* pResult
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PSTR pszEndPtr = NULL;
    unsigned long long int value = 0;
    uint64_t result = 0;

    errno = 0;
    value = strtoull(pszValue, &pszEndPtr, 10);
    dwError = errno;
    if (!pszEndPtr || pszEndPtr == pszValue || *pszEndPtr || dwError)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    if (value < 0 || value > (uint64_t) -1)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    result = (uint64_t)value;

error:
    if (dwError)
    {
        result = 0;
    }
    *pResult = result;
    return dwError;
}

DWORD
LWNetCacheDbReadSqliteInt32(
    IN PCSTR pszValue,
    OUT int32_t* pResult
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PSTR pszEndPtr = NULL;
    long int value = 0;
    int32_t result = 0;

    errno = 0;
    value = strtol(pszValue, &pszEndPtr, 10);
    dwError = errno;
    if (!pszEndPtr || pszEndPtr == pszValue || *pszEndPtr || dwError)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }
    // ISSUE-2008/07/01-dalmeida -- Add range checking
    result = (int32_t)value;

error:
    if (dwError)
    {
        result = 0;
    }
    *pResult = result;
    return dwError;
}

DWORD
LWNetCacheDbReadSqliteInt64(
    IN PCSTR pszValue,
    OUT int64_t* pResult
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    PSTR pszEndPtr = NULL;
    long long int value = 0;
    int64_t result = 0;

    errno = 0;
    value = strtoll(pszValue, &pszEndPtr, 10);
    dwError = errno;
    if (!pszEndPtr || pszEndPtr == pszValue || *pszEndPtr || dwError)
    {
        dwError = LWNET_ERROR_DATA_ERROR;
        BAIL_ON_LWNET_ERROR(dwError);
    }

    // ISSUE-2008/07/01-dalmeida -- Add range checking
    result = (int64_t)value;

error:
    if (dwError)
    {
        result = 0;
    }
    *pResult = result;
    return dwError;
}

DWORD
LWNetCacheDbReadSqliteString(
    IN PCSTR pszValue,
    OUT PSTR* pResult
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;

    dwError = LWNetStrDupOrNull(pszValue, pResult);

    return dwError;
}

DWORD
LWNetCacheDbReadSqliteStringIntoBuffer(
    IN PCSTR pszValue,
    IN DWORD dwResultSize,
    OUT PSTR pResultBuffer,
    OUT PSTR* pResultValue,
    OUT OPTIONAL PSTR* pResultEnd
    )
{
    DWORD dwError = LWNET_ERROR_SUCCESS;
    size_t length = 0;
    PSTR pStart = NULL;
    PSTR pEnd = pResultBuffer;

    if (!pszValue)
    {
        dwError = 0;
        goto error;
    }

    length = strlen(pszValue);

    // Need to make sure we have space for terminating NULL too.
    if (dwResultSize < (length + 1))
    {
        dwError = LWNET_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_LWNET_ERROR(dwError)
    }

    memcpy(pResultBuffer, pszValue, length + 1);

    pStart = pResultBuffer;
    pEnd = pResultBuffer + length + 1;

error:
    if (dwError)
    {
        pStart = NULL;
        pEnd = pResultBuffer;
    }

    *pResultValue = pStart;
    if (pResultEnd)
    {
        *pResultEnd = pEnd;
    }

    return dwError;
}

