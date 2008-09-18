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
 *        Likewise Security and Authentication Subsystem (LSASS)
 * 
 *        Private functions for AD Caching
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#ifndef __CACHEDB_P_H__
#define __CACHEDB_P_H__

#define SQLITE3_SAFE_FREE_STRING(x) \
    if ((x) != NULL) \
    { \
       sqlite3_free(x); \
       (x) = NULL; \
    }

#define BAIL_ON_SQLITE3_ERROR(dwError, pszError) \
    if (dwError) {                               \
       LSA_LOG_DEBUG("Sqlite3 error '%s' at %s:%d [code: %d]", \
               LsaEmptyStrForNull(pszError), __FILE__, __LINE__, dwError); \
       goto error;                               \
    }

#define ADCACHEDB_FREE_UNUSED_CACHEIDS   \
    "delete from lwicachetags where CacheId NOT IN " \
        "( select CacheId from lwigroupmembership ) AND " \
        "CacheId NOT IN ( select CacheId from lwiobjects ) AND " \
        "CacheId NOT IN ( select CacheId from lwipasswordverifiers );\n"

typedef struct _CACHE_CONNECTION
{
    sqlite3 *pDb;
    pthread_rwlock_t lock;
    sqlite3_stmt *pstFindObjectByNT4;
    sqlite3_stmt *pstFindObjectByDN;
    sqlite3_stmt *pstFindObjectBySid;

    sqlite3_stmt *pstFindUserByUPN;
    sqlite3_stmt *pstFindUserByAlias;
    sqlite3_stmt *pstFindUserById;

    sqlite3_stmt *pstFindGroupByAlias;
    sqlite3_stmt *pstFindGroupById;

    sqlite3_stmt *pstGetGroupMembers;
    sqlite3_stmt *pstGetGroupsForUser;
 
    sqlite3_stmt *pstGetPasswordVerifier;

    sqlite3_stmt *pstGetProviderData;
    sqlite3_stmt *pstGetDomainTrustList;
    sqlite3_stmt *pstGetCellList;
} CACHE_CONNECTION, *PCACHE_CONNECTION;

// This is the maximum number of characters necessary to store a guid in
// string form.
#define UUID_STR_SIZE 37

DWORD
ADCacheDB_ReadSqliteUInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    uint64_t *pqwResult);

DWORD
ADCacheDB_ReadSqliteInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    int64_t *pqwResult);

DWORD
ADCacheDB_ReadSqliteUInt32(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    DWORD *pdwResult);

DWORD
ADCacheDB_ReadSqliteBoolean(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    BOOLEAN *pbResult);

DWORD
ADCacheDB_ReadSqliteString(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PSTR *ppszResult);

DWORD
ADCacheDB_ReadSqliteStringInPlace(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSTR pszResult,
    //Includes NULL
    IN size_t sMaxSize);

DWORD
ADCacheDB_ReadSqliteSid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSID* ppSid);

DWORD
ADCacheDB_ReadSqliteGuid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT uuid_t** ppGuid);

DWORD
ADCacheDB_UnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    AD_CACHE_INFO *pResult);

DWORD
ADCacheDB_UnpackObjectInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PAD_SECURITY_OBJECT pResult);

DWORD
ADCacheDB_UnpackUserInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PAD_SECURITY_OBJECT pResult);

DWORD
ADCacheDB_UnpackGroupInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PAD_SECURITY_OBJECT pResult);

DWORD
ADCacheDB_UnpackDomainTrust(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PLSA_DM_ENUM_DOMAIN_CALLBACK_INFO pResult);

DWORD
ADCacheDB_UnpackLinkedCellInfo(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PAD_LINKED_CELL_INFO pResult);

DWORD
ADCacheDB_ExecWithRetry(
    PCACHE_CONNECTION pConn,
    PCSTR pszTransaction);

DWORD
ADCacheDB_QueryObject(
        sqlite3_stmt *pstQuery,
        PAD_SECURITY_OBJECT *ppObject);

PCSTR
ADCacheDB_GetObjectFieldList(
    VOID
    );

DWORD
ADCacheDB_FreePreparedStatements(
    PCACHE_CONNECTION pConn);

DWORD
ADCacheDB_GetCellListNoLock(
    IN HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN OUT PDLINKEDLIST* ppCellList
    );

DWORD
ADCacheDB_GetCacheCellListCommand(
    IN HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN const DLINKEDLIST* pCellList,
    OUT PSTR* ppszCommand
    );

VOID
ADCacheDB_FreeCallbackInfoNode(
    IN PVOID pData,
    IN PVOID pUserData
    );

#endif /* __CACHEDB_P_H__ */
