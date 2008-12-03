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
 *        cachedb.h
 *
 * Abstract:
 *
 *        Likewise Netlogon (LWNET)
 * 
 *        Caching for Likewise Netlogon
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 *
 */
#ifndef __CACHEDB_H__
#define __CACHEDB_H__

#include <lwnet-utils.h>

//
// Default DB location
//

#define NETLOGON_DB_DIR LWNET_CACHE_DIR "/db"
#define NETLOGON_DB     NETLOGON_DB_DIR "/netlogon-cache.db"

//
// Types for low-level API
//

// ISSUE-2008/07/01-dalmeida -- These should only be here if we do export
typedef DWORD LWNET_CACHE_DB_QUERY_TYPE;

#define LWNET_CACHE_DB_QUERY_TYPE_DC  0
#define LWNET_CACHE_DB_QUERY_TYPE_GC  1
#define LWNET_CACHE_DB_QUERY_TYPE_PDC 2

typedef struct _LWNET_CACHE_DB_ENTRY {
    LWNET_CACHE_DB_QUERY_TYPE QueryType;
    LWNET_UNIX_TIME_T LastDiscovered;
    LWNET_UNIX_TIME_T LastPinged;
    LWNET_DC_INFO DcInfo;
} LWNET_CACHE_DB_ENTRY, *PLWNET_CACHE_DB_ENTRY;

typedef struct _LWNET_CACHE_DB_KRB5_ENTRY {
    LWNET_UNIX_TIME_T LastUpdated;
    PSTR Realm;
    PSTR* ServerAddressArray;
    DWORD ServerAddressCount;
} LWNET_CACHE_DB_KRB5_ENTRY, *PLWNET_CACHE_DB_KRB5_ENTRY;

// ISSUE-2008/07/01-dalmeida -- simplify declaraion of handle type
typedef struct _LWNET_CACHE_DB_HANDLE_DATA  LWNET_CACHE_DB_HANDLE_DATA, *PLWNET_CACHE_DB_HANDLE_DATA;
typedef PLWNET_CACHE_DB_HANDLE_DATA LWNET_CACHE_DB_HANDLE, *PLWNET_CACHE_DB_HANDLE;


//
// Low-level API (for tools)
//
DWORD
LWNetCacheDbOpen(
    IN PCSTR Path,
    IN BOOLEAN bIsWrite,
    OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    );

VOID
LWNetCacheDbClose(
    IN OUT PLWNET_CACHE_DB_HANDLE pDbHandle
    );

DWORD
LWNetCacheDbSetup(
    IN LWNET_CACHE_DB_HANDLE DbHandle
    );

DWORD
LWNetCacheDbQuery(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PLWNET_UNIX_TIME_T LastDiscovered,
    OUT PLWNET_UNIX_TIME_T LastPinged
    );

DWORD
LWNetCacheDbUpdate(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    IN OPTIONAL PLWNET_UNIX_TIME_T LastDiscovered,
    IN PLWNET_DC_INFO pDcInfo
    );

DWORD
LWNetCacheDbScavenge(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    );

DWORD
LWNetCacheDbExport(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    OUT PLWNET_CACHE_DB_ENTRY* ppEntries,
    OUT PDWORD pdwCount
    );

DWORD
LWNetCacheDbUpdateKrb5(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszDnsDomainName,
    IN PCSTR* pServerAddressArray,
    IN DWORD dwServerAddressCount
    );

DWORD
LWNetCacheDbScavengeKrb5(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN LWNET_UNIX_TIME_T Age
    );

DWORD
LWNetCacheDbExportKrb5(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    OUT PLWNET_CACHE_DB_KRB5_ENTRY* ppEntries,
    OUT PDWORD pdwCount
    );

//
// High-level API for server
//
DWORD
LWNetCacheInitialize(
    );

VOID
LWNetCacheCleanup(
    );

DWORD
LWNetCacheQuery(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    OUT PLWNET_DC_INFO* ppDcInfo,
    OUT PLWNET_UNIX_TIME_T LastDiscovered,
    OUT PLWNET_UNIX_TIME_T LastPinged
    );

DWORD
LWNetCacheUpdatePing(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    IN LWNET_UNIX_TIME_T LastDiscovered,
    IN PLWNET_DC_INFO pDcInfo
    );

DWORD
LWNetCacheUpdateDiscover(
    IN PCSTR pszDnsDomainName,
    IN OPTIONAL PCSTR pszSiteName,
    IN DWORD dwDsFlags,
    IN PLWNET_DC_INFO pDcInfo
    );

DWORD
LWNetCacheScavenge(
    IN LWNET_UNIX_TIME_T PositiveCacheAge,
    IN LWNET_UNIX_TIME_T NegativeCacheAge
    );

DWORD
LWNetCacheUpdateKrb5(
    IN PCSTR pszDnsDomainName,
    IN PCSTR* pServerAddressArray,
    IN DWORD dwServerAddressCount
    );

DWORD
LWNetCacheSavengeKrb5(
    IN LWNET_UNIX_TIME_T Age
    );

DWORD
LWNetCacheExportKrb5(
    OUT PLWNET_CACHE_DB_KRB5_ENTRY* ppEntries,
    OUT PDWORD pdwCount
    );

#endif /* __CACHEDB_H__ */
