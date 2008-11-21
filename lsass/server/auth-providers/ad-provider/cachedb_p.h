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
    do { \
        if (dwError) \
        { \
           LSA_LOG_DEBUG("Sqlite3 error '%s' (code = %d)", \
                         LSA_SAFE_LOG_STRING(pszError), dwError); \
           goto error;                               \
        } \
    } while (0)

#define BAIL_ON_SQLITE3_ERROR_DB(dwError, pDb) \
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pDb))

#define BAIL_ON_SQLITE3_ERROR_STMT(dwError, pStatement) \
    BAIL_ON_SQLITE3_ERROR_DB(dwError, sqlite3_db_handle(pStatement))

#define ADCACHEDB_FREE_UNUSED_CACHEIDS   \
    "delete from " AD_CACHEDB_TABLE_NAME_CACHE_TAGS " where CacheId NOT IN " \
        "( select CacheId from " AD_CACHEDB_TABLE_NAME_MEMBERSHIP " ) AND " \
        "CacheId NOT IN ( select CacheId from " AD_CACHEDB_TABLE_NAME_OBJECTS " ) AND " \
        "CacheId NOT IN ( select CacheId from " AD_CACHEDB_TABLE_NAME_VERIFIERS " );\n"

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

    sqlite3_stmt *pstInsertCacheTag;
    sqlite3_stmt *pstGetLastInsertedRow;
    sqlite3_stmt *pstSetLdapMembership;
    sqlite3_stmt *pstSetPrimaryGroupMembership;
    sqlite3_stmt *pstAddMembership;
} CACHE_CONNECTION, *PCACHE_CONNECTION;

// This is the maximum number of characters necessary to store a guid in
// string form.
#define UUID_STR_SIZE 37

typedef DWORD (*PFN_AD_CACHEDB_EXEC_CALLBACK)(
    IN PCACHE_CONNECTION pConnection,
    IN PVOID pContext,
    OUT PSTR* ppszError
    );

typedef struct _AD_CACHEDB_CACHE_GROUP_MEMBERSHIP_CONTEXT
{
    IN PCSTR pszParentSid;
    IN size_t sMemberCount;
    IN PAD_GROUP_MEMBERSHIP* ppMembers;
} AD_CACHEDB_CACHE_GROUP_MEMBERSHIP_CONTEXT, *PAD_CACHEDB_CACHE_GROUP_MEMBERSHIP_CONTEXT;

typedef struct _AD_CACHEDB_CACHE_USER_MEMBERSHIP_CONTEXT
{
    IN PCSTR pszChildSid;
    IN size_t sMemberCount;
    IN PAD_GROUP_MEMBERSHIP* ppMembers;
    IN BOOLEAN bIsPacAuthoritative;
} AD_CACHEDB_CACHE_USER_MEMBERSHIP_CONTEXT, *PAD_CACHEDB_CACHE_USER_MEMBERSHIP_CONTEXT;

static
DWORD
ADCacheDB_ReadSqliteUInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    uint64_t *pqwResult);

static
DWORD
ADCacheDB_ReadSqliteInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    int64_t *pqwResult);

static
DWORD
ADCacheDB_ReadSqliteUInt32(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    DWORD *pdwResult);

static
DWORD
ADCacheDB_ReadSqliteBoolean(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    BOOLEAN *pbResult);

static
DWORD
ADCacheDB_ReadSqliteString(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PSTR *ppszResult);

static
DWORD
ADCacheDB_ReadSqliteStringInPlace(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSTR pszResult,
    //Includes NULL
    IN size_t sMaxSize);

static
DWORD
ADCacheDB_ReadSqliteSid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSID* ppSid);

static
DWORD
ADCacheDB_ReadSqliteGuid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT uuid_t** ppGuid);

static
DWORD
ADCacheDB_UnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    AD_CACHE_INFO *pResult);

static
DWORD
ADCacheDB_UnpackObjectInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PAD_SECURITY_OBJECT pResult);

static
DWORD
ADCacheDB_UnpackUserInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PAD_SECURITY_OBJECT pResult);

static
DWORD
ADCacheDB_UnpackGroupInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PAD_SECURITY_OBJECT pResult);

static
DWORD
ADCacheDB_UnpackDomainTrust(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pResult
    );

static
DWORD
ADCacheDB_UnpackLinkedCellInfo(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PAD_LINKED_CELL_INFO pResult);

static
DWORD
ADCacheDB_SqlBindInt64(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN int64_t Value
    );

static
DWORD
ADCacheDB_SqlBindString(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN PCSTR pszValue
    );

static
DWORD
ADCacheDB_SqlBindBoolean(
    IN OUT sqlite3_stmt* pstQuery,
    IN int Index,
    IN BOOLEAN bValue
    );

static
DWORD
ADCacheDB_SqlAllocPrintf(
    OUT PSTR* ppszSqlCommand,
    IN PCSTR pszSqlFormat,
    IN ...
    );

static
DWORD
ADCacheDB_SqlExec(
    IN sqlite3* pSqlDatabase,
    IN PCSTR pszSqlCommand,
    OUT PSTR* ppszSqlError
    );

static
DWORD
ADCacheDB_ExecCallbackWithRetry(
    IN PCACHE_CONNECTION pConn,
    IN PFN_AD_CACHEDB_EXEC_CALLBACK pfnCallback,
    IN PVOID pContext
    );

static
DWORD
ADCacheDB_ExecWithRetry(
    IN PCACHE_CONNECTION pConn,
    IN PCSTR pszTransaction
    );

static
DWORD
ADCacheDB_BasicCallback(
    IN PCACHE_CONNECTION pConn,
    IN PVOID pContext,
    OUT PSTR* ppszError
    );

static
DWORD
ADCacheDB_QueryObject(
    IN sqlite3_stmt* pstQuery,
    OUT PAD_SECURITY_OBJECT* ppObject
    );

static
PCSTR
ADCacheDB_GetObjectFieldList(
    VOID
    );

static
DWORD
ADCacheDB_FreePreparedStatements(
    IN OUT PCACHE_CONNECTION pConn
    );

static
DWORD
ADCacheDB_GetCellListNoLock(
    IN HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN OUT PDLINKEDLIST* ppCellList
    );

static
DWORD
ADCacheDB_GetCacheCellListCommand(
    IN HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN const DLINKEDLIST* pCellList,
    OUT PSTR* ppszCommand
    );

static
VOID
ADCacheDB_FreeEnumDomainInfoCallback(
    IN OUT PVOID pData,
    IN PVOID pUserData
    );

static
VOID
ADCacheDB_FreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    );

static
DWORD
ADCacheDB_CreateCacheTag(
    IN PCACHE_CONNECTION pConn,
    IN time_t tLastUpdated,
    OUT int64_t *pqwCacheId
    );

static
DWORD
ADCacheDB_UpdateMembership(
    IN sqlite3_stmt* pstQuery,
    IN int64_t CacheId,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid
    );

static
DWORD
ADCacheDB_AddMembership(
    IN PCACHE_CONNECTION pConn,
    IN time_t tLastUpdated,
    IN int64_t CacheId,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid,
    IN BOOLEAN bIsInPac,
    IN BOOLEAN bIsInPacOnly,
    IN BOOLEAN bIsInLdap,
    IN BOOLEAN bIsDomainPrimaryGroup
    );

static
DWORD
ADCacheDB_CacheGroupMembershipCallback(
    IN PCACHE_CONNECTION pConnection,
    IN PVOID pContext,
    OUT PSTR* ppszError
    );

static
DWORD
ADCacheDB_CacheUserMembershipCallback(
    IN PCACHE_CONNECTION pConn,
    IN PVOID pContext,
    OUT PSTR* ppszError
    );

#endif /* __CACHEDB_P_H__ */
