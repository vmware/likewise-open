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
 *        cachedb_p.h
 *
 * Abstract:
 *
 *        Likewise Netlogon (LWNET)
 * 
 *        Private functions for AD Caching
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Brian Dunstan (bdunstan@likewisesoftware.com)
 * 
 */
#ifndef __CACHEDB_P_H__
#define __CACHEDB_P_H__

#include "lwnet.h"
#include "lwnet-cachedb.h"
#include "sqlite3.h"

#define SQLITE3_SAFE_FREE_STRING(x) \
    do { \
        if (x) \
        { \
           sqlite3_free(x); \
           (x) = 0; \
        } \
    } while (0)

#define BAIL_ON_SQLITE3_ERROR(dwError, pszError) \
    if (dwError) {                               \
       LWNET_LOG_DEBUG("Sqlite3 error '%s' at %s:%d [code: %d]", \
               LWNetEmptyStrForNull(pszError), __FILE__, __LINE__, dwError); \
       goto error;                               \
    }


DWORD
LWNetCacheDbExecQueryCountExpression(
    IN sqlite3* SqlHandle,
    IN PCSTR pszSql,
    OUT PDWORD pdwCount
    );

DWORD
LWNetCacheDbExecWithRetry(
    IN LWNET_CACHE_DB_HANDLE DbHandle,
    IN PCSTR pszSql
    );

// ISSUE-2008/07/01-dalmeida -- Should inline
PCSTR
LWNetCacheDbGetTableElement(
    IN PSTR* Table,
    IN int Width,
    IN int Row,
    IN int Column
    );

DWORD
LWNetCacheDbReadSqliteBoolean(
    IN PCSTR pszValue,
    OUT PBOOLEAN pResult
    );

DWORD
LWNetCacheDbReadSqliteUInt16(
    IN PCSTR pszValue,
    OUT PWORD pResult
    );

DWORD
LWNetCacheDbReadSqliteUInt32(
    IN PCSTR pszValue,
    OUT PDWORD pResult
    );

DWORD
LWNetCacheDbReadSqliteUInt64(
    IN PCSTR pszValue,
    OUT uint64_t* pResult
    );

DWORD
LWNetCacheDbReadSqliteInt32(
    IN PCSTR pszValue,
    OUT int32_t* pResult
    );

DWORD
LWNetCacheDbReadSqliteInt64(
    IN PCSTR pszValue,
    OUT int64_t* pResult
    );

DWORD
LWNetCacheDbReadSqliteString(
    IN PCSTR pszValue,
    OUT PSTR* pResult
    );

DWORD
LWNetCacheDbReadSqliteStringIntoBuffer(
    IN PCSTR pszValue,
    IN DWORD dwResultSize,
    OUT PSTR pResultBuffer,
    OUT PSTR* pResultValue,
    OUT OPTIONAL PSTR* pResultEnd
    );

#endif /* __CACHEDB_P_H__ */
