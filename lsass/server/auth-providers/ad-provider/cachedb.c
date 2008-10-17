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
 *        cachedb.c
 *
 * Abstract:
 *
 *        Caching for AD Provider Database Interface
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "adprovider.h"
#include "cachedb_p.h"

#define ENTER_CACHEDB_LOCK(pConn, bInLock)                 \
        if (!bInLock) {                                    \
           pthread_rwlock_wrlock(&pConn->lock);            \
           bInLock = TRUE;                                 \
        }

#define LEAVE_CACHEDB_LOCK(pConn, bInLock)                 \
        if (bInLock) {                                     \
           pthread_rwlock_unlock(&pConn->lock);            \
           bInLock = FALSE;                                \
        }

DWORD
ADCacheDB_Initialize(
    VOID
    )
{
    DWORD dwError = 0;
    BOOLEAN bLockCreated = FALSE;
    PCACHE_CONNECTION pConn = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;
    PSTR pszQuery = NULL;
    PCSTR pszQueryFormat =
        "select "
        "%s "
        "from lwicachetags, lwiobjects left outer join lwiusers ON "
            "lwiobjects.ObjectSid = lwiusers.ObjectSid "
            "left outer join lwigroups ON "
            "lwiobjects.ObjectSid = lwigroups.ObjectSid "
        "where lwicachetags.CacheId = lwiobjects.CacheId AND "
            "%s";

    if (gpCacheConnection != NULL)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(CACHE_CONNECTION),
                    (PVOID*)&pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_rwlock_init(&pConn->lock, NULL);
    BAIL_ON_LSA_ERROR(dwError);
    bLockCreated = TRUE;

    dwError = LsaCheckFileExists(LSASS_DB, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
        dwError = LsaCheckDirectoryExists(LSASS_DB_DIR, &bExists);
        BAIL_ON_LSA_ERROR(dwError);

        if (!bExists) {
            mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
            
            dwError = LsaCreateDirectory(LSASS_DB_DIR, cacheDirMode);
            BAIL_ON_LSA_ERROR(dwError);
        }

        /* restrict access to u+rwx to the db folder */
        dwError = LsaChangeOwnerAndPermissions(LSASS_DB_DIR, 0, 0, S_IRWXU);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = sqlite3_open(LSASS_DB, &pConn->pDb);
        BAIL_ON_LSA_ERROR(dwError);
        
        dwError = sqlite3_exec(pConn->pDb,
                               AD_CACHEDB_CREATE_TABLES,
                               NULL,
                               NULL,
                               &pszError);
        BAIL_ON_SQLITE3_ERROR(dwError, pszError);

        dwError = LsaChangeOwnerAndPermissions(LSASS_DB, 0, 0, S_IRWXU);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        dwError = sqlite3_open(LSASS_DB, &pConn->pDb);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszQueryFormat,
        ADCacheDB_GetObjectFieldList(),
        "lower(lwiusers.UPN) = lower(?1 || '@' || ?2)");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstFindUserByUPN,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszQueryFormat,
        ADCacheDB_GetObjectFieldList(),
        "lower(lwiobjects.NetbiosDomainName) = lower(?1) AND "
        "lower(lwiobjects.SamAccountName) = lower(?2)");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstFindObjectByNT4,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszQueryFormat,
        ADCacheDB_GetObjectFieldList(),
        "lower(lwiusers.AliasName) = lower(?1)");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstFindUserByAlias,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszQueryFormat,
        ADCacheDB_GetObjectFieldList(),
        "lower(lwigroups.AliasName) = lower(?1)");

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstFindGroupByAlias,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszQueryFormat,
        ADCacheDB_GetObjectFieldList(),
        "lwiusers.Uid = ?1");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstFindUserById,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszQueryFormat,
        ADCacheDB_GetObjectFieldList(),
        "lwigroups.Gid = ?1");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstFindGroupById,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszQueryFormat,
        ADCacheDB_GetObjectFieldList(),
        "lwiobjects.DN = ?1");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstFindObjectByDN,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszQueryFormat,
        ADCacheDB_GetObjectFieldList(),
        "lwiobjects.ObjectSid = ?1");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstFindObjectBySid,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            "lwicachetags.CacheId, "
            "lwicachetags.LastUpdated, "
            "lwigroupmembership.ParentSid, "
            "lwigroupmembership.ChildSid "
            "from lwicachetags, lwigroupmembership "
            "where lwicachetags.CacheId = lwigroupmembership.CacheId "
                "AND lwigroupmembership.ParentSid = ?1",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetGroupMembers,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            "lwicachetags.CacheId, "
            "lwicachetags.LastUpdated, "
            "lwigroupmembership.ParentSid, "
            "lwigroupmembership.ChildSid "
            "from lwicachetags, lwigroupmembership "
            "where lwicachetags.CacheId = lwigroupmembership.CacheId "
                "AND lwigroupmembership.ChildSid = ?1",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetGroupsForUser,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            "lwicachetags.CacheId, "
            "lwicachetags.LastUpdated, "
            "lwipasswordverifiers.ObjectSid, "
            "lwipasswordverifiers.PasswordVerifier "
            "from lwicachetags, lwipasswordverifiers "
            "where lwicachetags.CacheId = lwipasswordverifiers.CacheId "
                "AND lwipasswordverifiers.ObjectSid = ?1",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetPasswordVerifier,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            "lwiproviderdata.DirectoryMode, "
            "lwiproviderdata.ADConfigurationMode, "
            "lwiproviderdata.ADMaxPwdAge, "
            "lwiproviderdata.Domain, "
            "lwiproviderdata.ShortDomain, "
            "lwiproviderdata.ComputerDN, "
            "lwiproviderdata.CellDN "
            "from lwiproviderdata ",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetProviderData,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            "lwidomaintrusts.RowIndex, "
            "lwidomaintrusts.DnsDomainName, "
            "lwidomaintrusts.NetbiosDomainName, "
            "lwidomaintrusts.Sid, "
            "lwidomaintrusts.Guid, "
            "lwidomaintrusts.TrusteeDnsDomainName, "
            "lwidomaintrusts.TrustFlags, "
            "lwidomaintrusts.TrustType, "
            "lwidomaintrusts.TrustAttributes, "
            "lwidomaintrusts.TrustDirection, "
            "lwidomaintrusts.TrustMode, "
            "lwidomaintrusts.ForestName, "
            "lwidomaintrusts.Flags "
            "from lwidomaintrusts ORDER BY RowIndex ASC",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetDomainTrustList,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            "lwilinkedcells.RowIndex, "
            "lwilinkedcells.CellDN, "
            "lwilinkedcells.Domain, "
            "lwilinkedcells.IsForestCell "
            "from lwilinkedcells ORDER BY RowIndex ASC",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetCellList,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    gpCacheConnection = pConn;

cleanup:

    if (pszError != NULL)
    {
        sqlite3_free(pszError);
    }
    LSA_SAFE_FREE_STRING(pszQuery);

    return dwError;

error:
    if (pConn != NULL)
    {
        if (bLockCreated)
        {
            pthread_rwlock_destroy(&pConn->lock);
        }
        ADCacheDB_FreePreparedStatements(pConn);

        if (pConn->pDb != NULL)
        {
            sqlite3_close(pConn->pDb);
        }
        LSA_SAFE_FREE_MEMORY(pConn);
    }

    goto cleanup;
}

DWORD
ADCacheDB_FreePreparedStatements(
        PCACHE_CONNECTION pConn)
{
    int i;
    DWORD dwError = LSA_ERROR_SUCCESS;
    sqlite3_stmt * * const pppstFreeList[] = {
        &pConn->pstFindObjectByNT4,
        &pConn->pstFindObjectByDN,
        &pConn->pstFindObjectBySid,

        &pConn->pstFindUserByUPN,
        &pConn->pstFindUserByAlias,
        &pConn->pstFindUserById,

        &pConn->pstFindGroupByAlias,
        &pConn->pstFindGroupById,

        &pConn->pstGetGroupMembers,
        &pConn->pstGetGroupsForUser,

        &pConn->pstGetPasswordVerifier,

        &pConn->pstGetProviderData,
        &pConn->pstGetDomainTrustList,
        &pConn->pstGetCellList,
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

DWORD
ADCacheDB_Shutdown(
    VOID)
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    if (gpCacheConnection == NULL)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_FreePreparedStatements(gpCacheConnection);
    BAIL_ON_LSA_ERROR(dwError);

    if (gpCacheConnection->pDb != NULL)
    {
        sqlite3_close(gpCacheConnection->pDb);
        gpCacheConnection->pDb = NULL;
    }

    dwError = pthread_rwlock_destroy(&gpCacheConnection->lock);
    BAIL_ON_LSA_ERROR(dwError);
    LSA_SAFE_FREE_MEMORY(gpCacheConnection);

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
ADCacheDB_OpenDb(
    PHANDLE phDb
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    BAIL_ON_INVALID_POINTER(phDb);

    if (gpCacheConnection == NULL)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *phDb = (HANDLE)gpCacheConnection;
    
cleanup:

    return dwError;
    
error:

    goto cleanup;
}

void
ADCacheDB_SafeCloseDb(
    PHANDLE phDb
    )
{
    if (phDb != NULL)
    {
        // This points to our global instance. Don't free it
        *phDb = (HANDLE)0;
    }
}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the cache
DWORD
ADCacheDB_FindUserByName(
    HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PAD_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    PAD_SECURITY_OBJECT pObject = NULL;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    switch (pUserNameInfo->nameType)
    {
       case NameType_UPN:
            pstQuery = pConn->pstFindUserByUPN;
            dwError = sqlite3_bind_text(
                    pstQuery,
                    1, 
                    pUserNameInfo->pszName,
                    -1, // let sqlite calculate the length
                    SQLITE_TRANSIENT //let sqlite make its own copy
                    );
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

            dwError = sqlite3_bind_text(
                    pstQuery,
                    2, 
                    pUserNameInfo->pszFullDomainName,
                    -1, // let sqlite calculate the length
                    SQLITE_TRANSIENT //let sqlite make its own copy
                    );
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            break;
       case NameType_NT4:
            pstQuery = pConn->pstFindObjectByNT4;
            dwError = sqlite3_bind_text(
                    pstQuery,
                    1, 
                    pUserNameInfo->pszDomainNetBiosName,
                    -1, // let sqlite calculate the length
                    SQLITE_TRANSIENT //let sqlite make its own copy
                    );
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

            dwError = sqlite3_bind_text(
                    pstQuery,
                    2, 
                    pUserNameInfo->pszName,
                    -1, // let sqlite calculate the length
                    SQLITE_TRANSIENT //let sqlite make its own copy
                    );
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            break;
       case NameType_Alias:
            pstQuery = pConn->pstFindUserByAlias;
            dwError = sqlite3_bind_text(
                    pstQuery,
                    1, 
                    pUserNameInfo->pszName,
                    -1, // let sqlite calculate the length
                    SQLITE_TRANSIENT //let sqlite make its own copy
                    );
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            break;
       default:
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_QueryObject(pstQuery, &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_User)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;
    
cleanup:
    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheDB_SafeFreeObject(&pObject);
    goto cleanup;
}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the cache
DWORD
ADCacheDB_FindUserById(
    HANDLE hDb,
    uid_t uid,
    PAD_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    PAD_SECURITY_OBJECT pObject = NULL;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    pstQuery = pConn->pstFindUserById;
    dwError = sqlite3_bind_int64(
            pstQuery,
            1, 
            (uint64_t)uid
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = ADCacheDB_QueryObject(pstQuery, &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_User)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;
    
cleanup:
    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheDB_SafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
ADCacheDB_FindGroupByName(
    HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PAD_SECURITY_OBJECT* ppObject
    )
{    
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    PAD_SECURITY_OBJECT pObject = NULL;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    switch (pGroupNameInfo->nameType)
    {
       case NameType_NT4:
            pstQuery = pConn->pstFindObjectByNT4;
            dwError = sqlite3_bind_text(
                    pstQuery,
                    1, 
                    pGroupNameInfo->pszDomainNetBiosName,
                    -1, // let sqlite calculate the length
                    SQLITE_TRANSIENT //let sqlite make its own copy
                    );
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

            dwError = sqlite3_bind_text(
                    pstQuery,
                    2, 
                    pGroupNameInfo->pszName,
                    -1, // let sqlite calculate the length
                    SQLITE_TRANSIENT //let sqlite make its own copy
                    );
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            break;
       case NameType_Alias:
            pstQuery = pConn->pstFindGroupByAlias;
            dwError = sqlite3_bind_text(
                    pstQuery,
                    1, 
                    pGroupNameInfo->pszName,
                    -1, // let sqlite calculate the length
                    SQLITE_TRANSIENT //let sqlite make its own copy
                    );
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            break;
       default:
            dwError = LSA_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_QueryObject(pstQuery, &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_Group)
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;
    
cleanup:
    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheDB_SafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
ADCacheDB_FindGroupById(
    HANDLE hDb,
    gid_t gid,
    PAD_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    PAD_SECURITY_OBJECT pObject = NULL;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    pstQuery = pConn->pstFindGroupById;
    dwError = sqlite3_bind_int64(
            pstQuery,
            1, 
            (uint64_t)gid
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = ADCacheDB_QueryObject(pstQuery, &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_Group)
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;
    
cleanup:
    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheDB_SafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
ADCacheDB_ReadSqliteUInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    uint64_t *pqwResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    //Do not free
    PSTR pszEndPtr = NULL;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
#endif

    *pqwResult = strtoull(pszColumnValue, &pszEndPtr, 10);
    if (pszEndPtr == NULL || pszEndPtr == pszColumnValue || *pszEndPtr != '\0')
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    (*piColumnPos)++;

error:
    return dwError;
}

DWORD
ADCacheDB_ReadSqliteInt64(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    int64_t *pqwResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    //Do not free
    PSTR pszEndPtr = NULL;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
#endif

    *pqwResult = strtoll(pszColumnValue, &pszEndPtr, 10);
    if (pszEndPtr == NULL || pszEndPtr == pszColumnValue || *pszEndPtr != '\0')
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    (*piColumnPos)++;

error:
    return dwError;
}

DWORD
ADCacheDB_ReadSqliteTimeT(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    time_t *pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    uint64_t qwTemp;

    dwError = ADCacheDB_ReadSqliteUInt64(
        pstQuery,
        piColumnPos,
        name,
        &qwTemp);
    BAIL_ON_LSA_ERROR(dwError);

    *pResult = qwTemp;

error:
    return dwError;
}

DWORD
ADCacheDB_ReadSqliteUInt32(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    DWORD *pdwResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    uint64_t qwTemp;
    int iColumnPos = *piColumnPos;

    dwError = ADCacheDB_ReadSqliteUInt64(
        pstQuery,
        &iColumnPos,
        name,
        &qwTemp);
    BAIL_ON_LSA_ERROR(dwError);

    if (qwTemp > UINT_MAX)
    {
        dwError = ERANGE;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pdwResult = qwTemp;
    *piColumnPos = iColumnPos;

error:
    return dwError;
}

DWORD
ADCacheDB_ReadSqliteBoolean(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    BOOLEAN *pbResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD dwTemp;

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        name,
        &dwTemp);
    BAIL_ON_LSA_ERROR(dwError);

    *pbResult = (dwTemp != 0);

error:
    return dwError;
}

DWORD
ADCacheDB_ReadSqliteString(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PCSTR name,
    PSTR *ppszResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
#endif

    dwError = LsaStrDupOrNull(
            pszColumnValue,
            ppszResult);
    BAIL_ON_LSA_ERROR(dwError);

    (*piColumnPos)++;

cleanup:
    return dwError;

error:
    *ppszResult = NULL;

    goto cleanup;
}

DWORD
ADCacheDB_ReadSqliteStringInPlace(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSTR pszResult,
    //Includes NULL
    IN size_t sMaxSize)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    //Do not free
    PCSTR pszColumnValue = (PCSTR)sqlite3_column_text(pstQuery, *piColumnPos);
    size_t sRequiredSize = 0;

#ifdef DEBUG
    // Extra internal error checking
    if (strcmp(sqlite3_column_name(pstQuery, *piColumnPos), name))
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
#endif

    sRequiredSize = strlen(pszColumnValue) + 1;
    if (sRequiredSize > sMaxSize)
    {
        dwError = LSA_ERROR_OUT_OF_MEMORY;
        BAIL_ON_LSA_ERROR(dwError);
    }

    memcpy(pszResult, pszColumnValue, sRequiredSize);

    (*piColumnPos)++;

cleanup:
    return dwError;

error:
    pszResult[0] = '\0';

    goto cleanup;
}

DWORD
ADCacheDB_ReadSqliteSid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT PSID* ppSid)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszSid = NULL;
    PSID pSid = NULL;
    int iColumnPos = *piColumnPos;

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        &iColumnPos,
        name,
        &pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ParseSidString(
            &pSid,
            pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    *ppSid = pSid;
    *piColumnPos = iColumnPos;

cleanup:
    LSA_SAFE_FREE_STRING(pszSid);

    return dwError;

error:

    *ppSid = NULL;
    if (pSid != NULL)
    {
        SidFree(pSid);
    }
    goto cleanup;
}

DWORD
ADCacheDB_ReadSqliteGuid(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN PCSTR name,
    OUT uuid_t** ppGuid)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszGuid = NULL;
    uuid_t *pGuid = NULL;
    int iColumnPos = *piColumnPos;

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        &iColumnPos,
        name,
        &pszGuid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(*pGuid),
                    (PVOID*)&pGuid);
    BAIL_ON_LSA_ERROR(dwError);

    if (uuid_parse(
            pszGuid,
            *pGuid) < 0)
    {
        // uuid_parse returns -1 on error, but does not set errno
        dwError = LSA_ERROR_INVALID_OBJECTGUID;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppGuid = pGuid;
    *piColumnPos = iColumnPos;

cleanup:
    LSA_SAFE_FREE_STRING(pszGuid);

    return dwError;

error:

    *ppGuid = NULL;
    LSA_SAFE_FREE_MEMORY(pGuid);
    goto cleanup;
}

DWORD
ADCacheDB_UnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    AD_CACHE_INFO *pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    dwError = ADCacheDB_ReadSqliteInt64(
        pstQuery,
        piColumnPos,
        "CacheId",
        &pResult->qwCacheId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteTimeT(
        pstQuery,
        piColumnPos,
        "LastUpdated",
        &pResult->tLastUpdated);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

DWORD
ADCacheDB_UnpackObjectInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PAD_SECURITY_OBJECT pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    
    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "ObjectSid",
        &pResult->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "DN",
        &pResult->pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "Enabled",
        &pResult->enabled);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "NetbiosDomainName",
        &pResult->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "SamAccountName",
        &pResult->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "Type",
        (DWORD*)&pResult->type);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

DWORD
ADCacheDB_UnpackUserInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PAD_SECURITY_OBJECT pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "Uid",
        (DWORD*)&pResult->userInfo.uid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "Gid",
        (DWORD*)&pResult->userInfo.gid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "UPN",
        &pResult->userInfo.pszUPN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "AliasName",
        &pResult->userInfo.pszAliasName);
    BAIL_ON_LSA_ERROR(dwError);
    if ( !pResult->userInfo.pszAliasName)
    {
        dwError = LsaAllocateString(
                      "",
                      &pResult->userInfo.pszAliasName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "Passwd",
        &pResult->userInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "Gecos",
        &pResult->userInfo.pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "Shell",
        &pResult->userInfo.pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "Homedir",
        &pResult->userInfo.pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteUInt64(
        pstQuery,
        piColumnPos,
        "PwdLastSet",
        &pResult->userInfo.qwPwdLastSet);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteUInt64(
        pstQuery,
        piColumnPos,
        "AccountExpires",
        &pResult->userInfo.qwAccountExpires);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "GeneratedUPN",
        &pResult->userInfo.bIsGeneratedUPN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "PasswordExpired",
        &pResult->userInfo.bPasswordExpired);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "PasswordNeverExpires",
        &pResult->userInfo.bPasswordNeverExpires);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "PromptPasswordChange",
        &pResult->userInfo.bPromptPasswordChange);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "UserCanChangePassword",
        &pResult->userInfo.bUserCanChangePassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "AccountDisabled",
        &pResult->userInfo.bAccountDisabled);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "AccountExpired",
        &pResult->userInfo.bAccountExpired);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "AccountLocked",
        &pResult->userInfo.bAccountLocked);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

DWORD
ADCacheDB_UnpackGroupInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PAD_SECURITY_OBJECT pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "Gid",
        (DWORD*)&pResult->groupInfo.gid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "AliasName",
        &pResult->groupInfo.pszAliasName);
    BAIL_ON_LSA_ERROR(dwError);
    if ( !pResult->groupInfo.pszAliasName)
    {
        dwError = LsaAllocateString(
                      "",
                      &pResult->groupInfo.pszAliasName);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "Passwd",
        &pResult->groupInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

DWORD
ADCacheDB_UnpackDomainTrust(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pResult
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    //Skip the index
    (*piColumnPos)++;

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "DnsDomainName",
        (PSTR*)&pResult->pszDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "NetbiosDomainName",
        (PSTR*)&pResult->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteSid(
        pstQuery,
        piColumnPos,
        "Sid",
        &pResult->pSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteGuid(
        pstQuery,
        piColumnPos,
        "Guid",
        &pResult->pGuid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "TrusteeDnsDomainName",
        (PSTR*)&pResult->pszTrusteeDnsDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "TrustFlags",
        &pResult->dwTrustFlags);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "TrustType",
        &pResult->dwTrustType);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "TrustAttributes",
        &pResult->dwTrustAttributes);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "TrustDirection",
        &pResult->dwTrustDirection);
    BAIL_ON_LSA_ERROR(dwError);
    
    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "TrustMode",
        &pResult->dwTrustMode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "ForestName",
        (PSTR*)&pResult->pszForestName);
    BAIL_ON_LSA_ERROR(dwError);

    pResult->pszClientSiteName = NULL;

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        piColumnPos,
        "Flags",
        &pResult->Flags);
    BAIL_ON_LSA_ERROR(dwError);

    pResult->DcInfo = NULL;
    pResult->GcInfo = NULL;

error:
    return dwError;
}

DWORD
ADCacheDB_UnpackLinkedCellInfo(
    IN sqlite3_stmt *pstQuery,
    IN OUT int *piColumnPos,
    IN OUT PAD_LINKED_CELL_INFO pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    //Skip the index
    (*piColumnPos)++;

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "CellDN",
        (PSTR*)&pResult->pszCellDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        piColumnPos,
        "Domain",
        (PSTR*)&pResult->pszDomain);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteBoolean(
        pstQuery,
        piColumnPos,
        "IsForestCell",
        &pResult->bIsForestCell);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

DWORD
ADCacheDB_ExecWithRetry(
    PCACHE_CONNECTION pConn,
    PCSTR pszTransaction)
{
    PSTR pszError = NULL;
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD dwRetry;
    BOOLEAN bInLock = FALSE;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    for(dwRetry = 0; dwRetry < 20; dwRetry++)
    {
        dwError = sqlite3_exec(pConn->pDb,
                pszTransaction,
                NULL,
                NULL,
                &pszError);
        if (dwError == SQLITE_BUSY)
        {
            SQLITE3_SAFE_FREE_STRING(pszError);
            dwError = 0;
            // sqlite3_exec runs pszSetFullEntry statement by statement. If
            // it fails, it leaves the sqlite VM with half of the transaction
            // finished. This rollsback the transaction so it can be retried
            // in entirety.
            sqlite3_exec(pConn->pDb,
                    "ROLLBACK",
                    NULL,
                    NULL,
                    NULL);

            LSA_LOG_ERROR("There is a conflict trying to access the cache database. This would happen if another process is trying to access it. Retrying...");
        }
        else
        {
            BAIL_ON_SQLITE3_ERROR(dwError, pszError);
            break;
        }
    }

error:
    LEAVE_CACHEDB_LOCK(pConn, bInLock);
    
    SQLITE3_SAFE_FREE_STRING(pszError);
    return dwError;
}

DWORD
ADCacheDB_CacheObjectEntry(
    HANDLE hDb,
    PAD_SECURITY_OBJECT pObject
    )
{
    return ADCacheDB_CacheObjectEntries(
            hDb,
            1,
            &pObject);
}

DWORD
ADCacheDB_CacheObjectEntries(
    HANDLE hDb,
    size_t  sObjectCount,
    PAD_SECURITY_OBJECT* ppObjects
    )
{
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t sIndex = 0;
    //Free with sqlite3_free
    char *pszError = NULL;

    //Free with sqlite3_free
    char *pszNewStatement = NULL;

    LSA_STRING_BUFFER buffer = {0};

    /* This function generates a SQL transaction to update multiple cache
     * entries at a time. The SQL command is in this format:
     * 1. Delete cache tag entries which are no longer referenced.
     * 2. Create/update the new cache tag entries, and create/update the
     *    lwiobjects.
     * 3. Create/update the lwiuser and lwigroup objects.
     */

    dwError = LsaInitializeStringBuffer(
            &buffer,
            sObjectCount * 200);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAppendStringBuffer(
            &buffer,
            "begin");
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sObjectCount; sIndex++)
    {
        if (ppObjects[sIndex] == NULL)
        {
            continue;
        }

        if (ppObjects[sIndex]->cache.qwCacheId == -1)
        {
            /* We don't know whether this object already has a cache tag in
             * the database. If there is one, it needs to be deleted.
             *
             * I tried to create one delete statement for every object, with
             * a very long expression, but sqlite didn't like that.
             */
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                "delete from lwicachetags where CacheId IN "
                    "( select CacheId from lwiobjects where ObjectSid = %Q)",
                ppObjects[sIndex]->pszObjectSid);

            if (pszNewStatement == NULL)
            {
                dwError = (DWORD)sqlite3_errcode(pConn->pDb);
                BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            }

            dwError = LsaAppendStringBuffer(
                    &buffer,
                    pszNewStatement);
            BAIL_ON_LSA_ERROR(dwError);
            SQLITE3_SAFE_FREE_STRING(pszNewStatement);
        }
    }

    for (sIndex = 0; sIndex < sObjectCount; sIndex++)
    {
        if (ppObjects[sIndex] == NULL)
        {
            continue;
        }

        if (ppObjects[sIndex]->cache.qwCacheId == -1)
        {
            // The object is either not cached yet, or the existing cache entry
            // needs to be replaced.
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                // Make a new cache entry
                "insert into lwicachetags ("
                    "LastUpdated"
                    ") values ("
                    "%ld);\n"
                "replace into lwiobjects ("
                    "CacheId,"	        
                    "ObjectSid,"
                    "DN,"
                    "Enabled,"
                    "NetbiosDomainName,"
                    "SamAccountName,"
                    "Type"
                    ") values ("
                    // This is the CacheId column of the row that was just
                    // created.
                    "last_insert_rowid(),"
                    "%Q," //sid
                    "%Q," //DN
                    "%d," //enabled
                    "%Q," //domain name
                    "%Q," //sam account
                    "%d)" /*type*/,
                ppObjects[sIndex]->cache.tLastUpdated,
                ppObjects[sIndex]->pszObjectSid,
                ppObjects[sIndex]->pszDN,
                ppObjects[sIndex]->enabled,
                ppObjects[sIndex]->pszNetbiosDomainName,
                ppObjects[sIndex]->pszSamAccountName,
                ppObjects[sIndex]->type
                );
        }
        else
        {
            // The object is already cached. Just update the existing info.
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                    // Update the existing cache entry
                    "update lwicachetags set "
                        "LastUpdated = %ld "
                        "where CacheId = %llu;\n"
                    "update lwiobjects set "
                        "CacheId = %llu, "
                        "Enabled = %d, "
                        "NetbiosDomainName = %Q, "
                        "SamAccountName = %Q, "
                        "Type = %d "
                        "where ObjectSid = %Q",
                ppObjects[sIndex]->cache.tLastUpdated,
                ppObjects[sIndex]->cache.qwCacheId,
                ppObjects[sIndex]->cache.qwCacheId,
                ppObjects[sIndex]->enabled,
                ppObjects[sIndex]->pszNetbiosDomainName,
                ppObjects[sIndex]->pszSamAccountName,
                ppObjects[sIndex]->type,
                ppObjects[sIndex]->pszObjectSid
                );
        }

        if (pszNewStatement == NULL)
        {
            dwError = (DWORD)sqlite3_errcode(pConn->pDb);
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
        }

        dwError = LsaAppendStringBuffer(
                &buffer,
                pszNewStatement);
        BAIL_ON_LSA_ERROR(dwError);
        SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    }
    
    for (sIndex = 0; sIndex < sObjectCount; sIndex++)
    {
        if (ppObjects[sIndex] == NULL)
        {
            continue;
        }

        if (ppObjects[sIndex]->enabled)
        {
            switch (ppObjects[sIndex]->type)
            {
                case AccountType_User:
                    pszNewStatement = sqlite3_mprintf(
                        ";\n"
                        "replace into lwiusers ("
                            "ObjectSid,"
                            "Uid,"
                            "Gid,"
                            "UPN,"
                            "AliasName,"
                            "Passwd,"
                            "Gecos,"
                            "Shell,"
                            "Homedir,"
                            "PwdLastSet,"
                            "AccountExpires,"
                            "GeneratedUPN,"
                            "PasswordExpired,"
                            "PasswordNeverExpires,"
                            "PromptPasswordChange,"
                            "UserCanChangePassword,"
                            "AccountDisabled,"
                            "AccountExpired,"
                            "AccountLocked"
                        ") values ("
                            "%Q," //sid
                            "%u," //uid
                            "%u," //gid
                            "%Q," //upn
                            "%Q," //alias
                            "%Q," //passwd
                            "%Q," //gecos
                            "%Q," //shell
                            "%Q," //homedir
                            "%llu," //pwdlastset
                            "%llu," //account expires
                            "%d," //generatedUPN
                            "%d," //passwordExpired
                            "%d," //passwordNeverExpires
                            "%d," //promptPasswordChange
                            "%d," //user can change password
                            "%d," //account disabled
                            "%d," //account expired
                            "%d" //account locked
                        ")",
                        ppObjects[sIndex]->pszObjectSid,
                        ppObjects[sIndex]->userInfo.uid,
                        ppObjects[sIndex]->userInfo.gid,
                        ppObjects[sIndex]->userInfo.pszUPN,
                        IsNullOrEmptyString(ppObjects[sIndex]->userInfo.pszAliasName) ?
                            NULL :
                            ppObjects[sIndex]->userInfo.pszAliasName,
                        ppObjects[sIndex]->userInfo.pszPasswd,
                        ppObjects[sIndex]->userInfo.pszGecos,
                        ppObjects[sIndex]->userInfo.pszShell,
                        ppObjects[sIndex]->userInfo.pszHomedir,
                        ppObjects[sIndex]->userInfo.qwPwdLastSet,
                        ppObjects[sIndex]->userInfo.qwAccountExpires,
                        ppObjects[sIndex]->userInfo.bIsGeneratedUPN,
                        ppObjects[sIndex]->userInfo.bPasswordExpired,
                        ppObjects[sIndex]->userInfo.bPasswordNeverExpires,
                        ppObjects[sIndex]->userInfo.bPromptPasswordChange,
                        ppObjects[sIndex]->userInfo.bUserCanChangePassword,
                        ppObjects[sIndex]->userInfo.bAccountDisabled,
                        ppObjects[sIndex]->userInfo.bAccountExpired,
                        ppObjects[sIndex]->userInfo.bAccountLocked
                        );
                    break;
                case AccountType_Group:
                    pszNewStatement = sqlite3_mprintf(
                        ";\n"
                        "replace into lwigroups ("
                            "ObjectSid,"
                            "Gid,"
                            "AliasName,"
                            "Passwd"
                        ") values ("
                            "%Q," //sid
                            "%u," //gid
                            "%Q,"
                            "%Q" //alias
                        ")",
                        ppObjects[sIndex]->pszObjectSid,
                        ppObjects[sIndex]->groupInfo.gid,
                        IsNullOrEmptyString(ppObjects[sIndex]->groupInfo.pszAliasName) ?
                            NULL :
                            ppObjects[sIndex]->groupInfo.pszAliasName,
                        ppObjects[sIndex]->groupInfo.pszPasswd
                        );
                    break;
                default:
                    dwError = LSA_ERROR_INVALID_PARAMETER;
                    BAIL_ON_LSA_ERROR(dwError);
            }

            if (pszNewStatement == NULL)
            {
                dwError = (DWORD)sqlite3_errcode(pConn->pDb);
                BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            }

            dwError = LsaAppendStringBuffer(
                    &buffer,
                    pszNewStatement);
            BAIL_ON_LSA_ERROR(dwError);
            SQLITE3_SAFE_FREE_STRING(pszNewStatement);
        }
    }

    dwError = LsaAppendStringBuffer(
            &buffer,
            ";\nend");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ExecWithRetry(
        pConn,
        buffer.pszBuffer);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    SQLITE3_SAFE_FREE_STRING(pszNewStatement);
    SQLITE3_SAFE_FREE_STRING(pszError);
    LsaFreeStringBufferContents(&buffer);

    return dwError;

error:

    goto cleanup;
}

void
ADCacheDB_SafeFreeObject(
    PAD_SECURITY_OBJECT* ppObject
    )
{
    PAD_SECURITY_OBJECT pObject = NULL;
    if (ppObject != NULL && *ppObject != NULL)
    {
        pObject = *ppObject;

        LSA_SAFE_FREE_STRING(pObject->pszObjectSid);

        LSA_SAFE_FREE_STRING(pObject->pszNetbiosDomainName);
        LSA_SAFE_FREE_STRING(pObject->pszSamAccountName);
        LSA_SAFE_FREE_STRING(pObject->pszDN);

        if (pObject->type == AccountType_User)
        {
            LSA_SAFE_FREE_STRING(pObject->userInfo.pszUPN);
            LSA_SAFE_FREE_STRING(pObject->userInfo.pszAliasName);
            LSA_SAFE_FREE_STRING(pObject->userInfo.pszPasswd);
            LSA_SAFE_FREE_STRING(pObject->userInfo.pszGecos);
            LSA_SAFE_FREE_STRING(pObject->userInfo.pszShell);
            LSA_SAFE_FREE_STRING(pObject->userInfo.pszHomedir);
        }
        else if (pObject->type == AccountType_Group)
        {
            LSA_SAFE_FREE_STRING(pObject->groupInfo.pszAliasName);
            LSA_SAFE_FREE_STRING(pObject->groupInfo.pszPasswd);
        }

        LSA_SAFE_FREE_MEMORY(pObject);
        *ppObject = NULL;
    }
}

DWORD
ADCacheDB_CacheGroupMembership(
    HANDLE hDb,
    PCSTR pszParentSid,
    size_t sMemberCount,
    PAD_GROUP_MEMBERSHIP* ppMembers)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t iMember;
    BOOLEAN newTagCreated = FALSE;
    PSTR pszOldExpression = NULL;
    PSTR pszSqlCommand = NULL;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;

    /* Make sure all of the group membership structures use pszParentSid */
    for (iMember = 0; iMember < sMemberCount; iMember++)
    {
        if (ppMembers[iMember]->pszParentSid == NULL)
        {
            dwError = LsaAllocateString(
                            pszParentSid, 
                            &ppMembers[iMember]->pszParentSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (strcmp(ppMembers[iMember]->pszParentSid, pszParentSid))
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    pszSqlCommand = sqlite3_mprintf(
        "begin;\n"
             /* clear all expirable cached group members for pszParentSid
              * (the entries in lwigroupmembership whose tLastUpdated time is
              * not 0xFFFFFFFF).*/
            "delete from lwigroupmembership where EXISTS "
                "(select LastUpdated from lwicachetags where "
                    "lwicachetags.CacheId = lwigroupmembership.CacheId AND "
                    "lwicachetags.LastUpdated NOT IN "
                        "(-1, -2)"
                ") AND "
                "lwigroupmembership.ParentSid = %Q;\n",
        pszParentSid);
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

     /* put ppMembers into the cache. Replace any of the unexpirable entries
      * if there are any already in the db. */
    for (iMember = 0; iMember < sMemberCount; iMember++)
    {
        if (ppMembers[iMember]->cache.qwCacheId == -1)
        {
            if (!newTagCreated)
            {
                SQLITE3_SAFE_FREE_STRING(pszOldExpression);
                pszOldExpression = pszSqlCommand;
                pszSqlCommand = sqlite3_mprintf(
                    "%s"
                    "insert into lwicachetags ("
                        "LastUpdated"
                        ") values ("
                        "%ld);\n",
                    pszOldExpression,
                    ppMembers[iMember]->cache.tLastUpdated);
                if (pszSqlCommand == NULL)
                {
                    dwError = (DWORD)sqlite3_errcode(pConn->pDb);
                    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
                }
                newTagCreated = TRUE;
            }
            SQLITE3_SAFE_FREE_STRING(pszOldExpression);
            pszOldExpression = pszSqlCommand;
            pszSqlCommand = sqlite3_mprintf(
                "%s"
                "replace into lwigroupmembership ("
                    "CacheId, "
                    "ParentSid, "
                    "ChildSid"
                ") values ("
                    "(select max(lwicachetags.CacheId) from lwicachetags),"
                    "%Q,"
                    "%Q);\n",
                pszOldExpression,
                ppMembers[iMember]->pszParentSid,
                ppMembers[iMember]->pszChildSid);
        }
        else
        {
            SQLITE3_SAFE_FREE_STRING(pszOldExpression);
            pszOldExpression = pszSqlCommand;
            pszSqlCommand = sqlite3_mprintf(
                "%s"
                "replace into lwigroupmembership ("
                    "CacheId, "
                    "ParentSid, "
                    "ChildSid"
                ") values ("
                    "%lld,"
                    "%Q,"
                    "%Q);\n",
                pszOldExpression,
                ppMembers[iMember]->cache.qwCacheId,
                ppMembers[iMember]->pszParentSid,
                ppMembers[iMember]->pszChildSid);
        }

        if (pszSqlCommand == NULL)
        {
            dwError = (DWORD)sqlite3_errcode(pConn->pDb);
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
        }
    }

    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    pszOldExpression = pszSqlCommand;
    pszSqlCommand = sqlite3_mprintf(
            "%s"
            /* delete any lwicachetags which are no longer being used. */
            "%s"
        "end;",
        pszOldExpression,
        ADCACHEDB_FREE_UNUSED_CACHEIDS
        );
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    dwError = ADCacheDB_ExecWithRetry(
        pConn,
        pszSqlCommand);
    BAIL_ON_LSA_ERROR(dwError);

error:
    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    return dwError;
}

DWORD
ADCacheDB_CacheGroupsForUser(
    HANDLE hDb,
    PCSTR pszChildSid,
    size_t sMemberCount,
    PAD_GROUP_MEMBERSHIP* ppMembers,
    BOOLEAN bOverwritePacEntries)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t iMember;
    PSTR pszOldExpression = NULL;
    PSTR pszSqlCommand = NULL;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;

    /* Make sure all of the group membership structures use pszChildSid */
    for (iMember = 0; iMember < sMemberCount; iMember++)
    {
        if (ppMembers[iMember]->pszChildSid == NULL)
        {
            dwError = LsaAllocateString(
                            pszChildSid, 
                            &ppMembers[iMember]->pszChildSid);
            BAIL_ON_LSA_ERROR(dwError);
        }
        else if (strcmp(ppMembers[iMember]->pszChildSid, pszChildSid))
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    pszSqlCommand = sqlite3_mprintf(
        "begin;\n"
             /* clear all expirable cached group members for pszChildSid
              * (the entries in lwigroupmembership whose tLastUpdated time is
              * not 0xFFFFFFFF).*/
            "delete from lwigroupmembership where %s"
                "lwigroupmembership.ChildSid = %Q;\n",
        bOverwritePacEntries ? "" : " -1 NOT IN "
                "(select LastUpdated from lwicachetags where "
                    "lwicachetags.CacheId = lwigroupmembership.CacheId) AND ",
        pszChildSid);
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

     /* put ppMembers into the cache. Replace any of the unexpirable entries
      * if there are any already in the db. */
    for (iMember = 0; iMember < sMemberCount; iMember++)
    {
        if (ppMembers[iMember]->cache.qwCacheId == -1)
        {
            SQLITE3_SAFE_FREE_STRING(pszOldExpression);
            pszOldExpression = pszSqlCommand;
            pszSqlCommand = sqlite3_mprintf(
                "%s"
                "insert into lwicachetags ("
                    "LastUpdated"
                    ") values ("
                    "%ld);\n",
                pszOldExpression,
                ppMembers[iMember]->cache.tLastUpdated);
            if (pszSqlCommand == NULL)
            {
                dwError = (DWORD)sqlite3_errcode(pConn->pDb);
                BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
            }

            SQLITE3_SAFE_FREE_STRING(pszOldExpression);
            pszOldExpression = pszSqlCommand;
            pszSqlCommand = sqlite3_mprintf(
                "%s"
                "replace into lwigroupmembership ("
                    "CacheId, "
                    "ParentSid, "
                    "ChildSid"
                ") values ("
                    "last_insert_rowid(),"
                    "%Q,"
                    "%Q);\n",
                pszOldExpression,
                ppMembers[iMember]->pszParentSid,
                ppMembers[iMember]->pszChildSid);
        }
        else
        {
            SQLITE3_SAFE_FREE_STRING(pszOldExpression);
            pszOldExpression = pszSqlCommand;
            pszSqlCommand = sqlite3_mprintf(
                "%s"
                "replace into lwigroupmembership ("
                    "CacheId, "
                    "ParentSid, "
                    "ChildSid"
                ") values ("
                    "%lld,"
                    "%Q,"
                    "%Q);\n",
                pszOldExpression,
                ppMembers[iMember]->cache.qwCacheId,
                ppMembers[iMember]->pszParentSid,
                ppMembers[iMember]->pszChildSid);
        }

        if (pszSqlCommand == NULL)
        {
            dwError = (DWORD)sqlite3_errcode(pConn->pDb);
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
        }
    }

    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    pszOldExpression = pszSqlCommand;
    pszSqlCommand = sqlite3_mprintf(
            "%s"
            /* delete any lwicachetags which are no longer being used. */
            "%s"
        "end;",
        pszOldExpression,
        ADCACHEDB_FREE_UNUSED_CACHEIDS
        );
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    dwError = ADCacheDB_ExecWithRetry(
        pConn,
        pszSqlCommand);
    BAIL_ON_LSA_ERROR(dwError);

error:
    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    return dwError;
}

DWORD
ADCacheDB_GetGroupMembers(
    HANDLE hDb,
    PCSTR pszSid,
    size_t* psCount,
    PAD_GROUP_MEMBERSHIP **pppResults)
{
    /* 
     * Search the cache for all members of pszSid.
     */
    DWORD dwError = LSA_ERROR_SUCCESS;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCapacity = 0;
    size_t sResultCount = 0;
    PAD_GROUP_MEMBERSHIP *ppResults = NULL;
    const int nExpectedCols = 4;
    int iColumnPos = 0;
    int nGotColumns = 0;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    pstQuery = pConn->pstGetGroupMembers;
    dwError = sqlite3_bind_text(
            pstQuery,
            1, 
            pszSid,
            -1, // let sqlite calculate the length
            SQLITE_TRANSIENT //let sqlite make its own copy
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    while((dwError = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (sResultCount >= sResultCapacity)
        {
            sResultCapacity *= 2;
            sResultCapacity += 10;
            dwError = LsaReallocMemory(
                            ppResults,
                            (PVOID*)&ppResults,
                            sizeof(PAD_GROUP_MEMBERSHIP) * sResultCapacity);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                        sizeof(AD_GROUP_MEMBERSHIP),
                        (PVOID*)&ppResults[sResultCount]);
        BAIL_ON_LSA_ERROR(dwError);
        /* Increment the result count here since this result is now partially
         * filled. If an error occurs while filling in the fields for this
         * result, the initialized fields will still get freed.
         */
        sResultCount++;
        iColumnPos = 0;

        dwError = ADCacheDB_UnpackCacheInfo(pstQuery,
                &iColumnPos,
                &ppResults[sResultCount - 1]->cache);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheDB_ReadSqliteString(
            pstQuery,
            &iColumnPos,
            "ParentSid",
            &ppResults[sResultCount - 1]->pszParentSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheDB_ReadSqliteString(
            pstQuery,
            &iColumnPos,
            "ChildSid",
            &ppResults[sResultCount - 1]->pszChildSid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_DONE)
    {
        // No more results found
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    *pppResults = ppResults;
    *psCount = sResultCount;
    
cleanup:
    LEAVE_CACHEDB_LOCK(pConn, bInLock);
    
    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;
    ADCacheDB_SafeFreeGroupMembershipList(sResultCount, &ppResults);
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

DWORD
ADCacheDB_GetGroupsForUser(
    HANDLE hDb,
    PCSTR pszSid,
    size_t* psCount,
    PAD_GROUP_MEMBERSHIP **pppResults)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCapacity = 0;
    size_t sResultCount = 0;
    PAD_GROUP_MEMBERSHIP *ppResults = NULL;
    const int nExpectedCols = 4;
    int iColumnPos = 0;
    int nGotColumns = 0;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    pstQuery = pConn->pstGetGroupsForUser;
    dwError = sqlite3_bind_text(
            pstQuery,
            1, 
            pszSid,
            -1, // let sqlite calculate the length
            SQLITE_TRANSIENT //let sqlite make its own copy
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    while((dwError = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (sResultCount >= sResultCapacity)
        {
            sResultCapacity *= 2;
            sResultCapacity += 10;
            dwError = LsaReallocMemory(
                            ppResults,
                            (PVOID*)&ppResults,
                            sizeof(PAD_GROUP_MEMBERSHIP) * sResultCapacity);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                        sizeof(AD_GROUP_MEMBERSHIP),
                        (PVOID*)&ppResults[sResultCount]);
        BAIL_ON_LSA_ERROR(dwError);
        /* Increment the result count here since this result is now partially
         * filled. If an error occurs while filling in the fields for this
         * result, the initialized fields will still get freed.
         */
        sResultCount++;
        iColumnPos = 0;

        dwError = ADCacheDB_UnpackCacheInfo(pstQuery,
                &iColumnPos,
                &ppResults[sResultCount - 1]->cache);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheDB_ReadSqliteString(
            pstQuery,
            &iColumnPos,
            "ParentSid",
            &ppResults[sResultCount - 1]->pszParentSid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheDB_ReadSqliteString(
            pstQuery,
            &iColumnPos,
            "ChildSid",
            &ppResults[sResultCount - 1]->pszChildSid);
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_DONE)
    {
        // No more results found
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    *pppResults = ppResults;
    *psCount = sResultCount;
    
cleanup:
    LEAVE_CACHEDB_LOCK(pConn, bInLock);
    
    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;
    ADCacheDB_SafeFreeGroupMembershipList(sResultCount, &ppResults);
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

void
ADCacheDB_SafeFreeGroupMembership(
        PAD_GROUP_MEMBERSHIP* ppMembership)
{
    if (*ppMembership != NULL)
    {
        LSA_SAFE_FREE_STRING((*ppMembership)->pszParentSid);
        LSA_SAFE_FREE_STRING((*ppMembership)->pszChildSid);
    }
    LSA_SAFE_FREE_MEMORY(*ppMembership);
}

void
ADCacheDB_SafeFreeGroupMembershipList(
        size_t sCount,
        PAD_GROUP_MEMBERSHIP** pppMembershipList)
{
    if (*pppMembershipList != NULL)
    {
        size_t iMember;
        for (iMember = 0; iMember < sCount; iMember++)
        {
            ADCacheDB_SafeFreeGroupMembership(&(*pppMembershipList)[iMember]);
        }
        LSA_SAFE_FREE_MEMORY(*pppMembershipList);
    }
}

void
ADCacheDB_SafeFreeObjectList(
        size_t sCount,
        PAD_SECURITY_OBJECT** pppObjectList)
{
    if (*pppObjectList != NULL)
    {
        size_t sIndex;
        for (sIndex = 0; sIndex < sCount; sIndex++)
        {
            ADCacheDB_SafeFreeObject(&(*pppObjectList)[sIndex]);
        }
        LSA_SAFE_FREE_MEMORY(*pppObjectList);
    }
}

DWORD
ADCacheDB_QueryObject(
        sqlite3_stmt *pstQuery,
        PAD_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;
    const int nExpectedCols = 29; // This is the number of fields defined in cachedb.h (AD_SECURITY_OBJECT)
    int iColumnPos = 0;
    PAD_SECURITY_OBJECT pObject = NULL;
    int nGotColumns = 0;

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        // No results found
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_ROW)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError,
                sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    }

    nGotColumns = sqlite3_column_count(pstQuery);
    if (nGotColumns != nExpectedCols)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(AD_SECURITY_OBJECT),
                    (PVOID*)&pObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_UnpackCacheInfo(pstQuery,
            &iColumnPos,
            &pObject->cache);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_UnpackObjectInfo(pstQuery,
            &iColumnPos,
            pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type == AccountType_User && pObject->enabled)
    {
        dwError = ADCacheDB_UnpackUserInfo(pstQuery,
                &iColumnPos,
                pObject);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        iColumnPos += 18; // This is the number of fields in the userInfo section of cachedb.h (AD_SECURITY_OBJECT)
    }

    if (pObject->type == AccountType_Group && pObject->enabled)
    {
        dwError = ADCacheDB_UnpackGroupInfo(pstQuery,
                &iColumnPos,
                pObject);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_ROW)
    {
        if (pObject->type == AccountType_Group)
        {
            dwError = LSA_ERROR_DUPLICATE_GROUPNAME;
        }
        else
        {
            dwError = LSA_ERROR_DUPLICATE_USERNAME;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError,
                sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    }
    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));

    *ppObject = pObject;
    
cleanup:
    
    return dwError;

error:
    *ppObject = NULL;
    ADCacheDB_SafeFreeObject(&pObject);
    sqlite3_reset(pstQuery);

    goto cleanup;
}

PCSTR
ADCacheDB_GetObjectFieldList(
    VOID
    )
{
    return
        "lwicachetags.CacheId, "
        "lwicachetags.LastUpdated, "
        "lwiobjects.ObjectSid, "
        "lwiobjects.DN, "
        "lwiobjects.Enabled, "
        "lwiobjects.NetbiosDomainName, "
        "lwiobjects.SamAccountName, "
        "lwiobjects.Type, "
        "lwiusers.Uid, "
        "lwiusers.Gid, "
        "lwiusers.UPN, "
        "lwiusers.AliasName, "
        "lwiusers.Passwd, "
        "lwiusers.Gecos, "
        "lwiusers.Shell, "
        "lwiusers.Homedir, "
        "lwiusers.PwdLastSet, "
        "lwiusers.AccountExpires, "
        "lwiusers.GeneratedUPN, "
        "lwiusers.PasswordExpired, "
        "lwiusers.PasswordNeverExpires, "
        "lwiusers.PromptPasswordChange, "
        "lwiusers.UserCanChangePassword, "
        "lwiusers.AccountDisabled, "
        "lwiusers.AccountExpired, "
        "lwiusers.AccountLocked, "
        "lwigroups.Gid, "
        "lwigroups.AliasName, "
        "lwigroups.Passwd";
}

DWORD
ADCacheDB_FindObjectByDN(
    HANDLE hDb,
    PCSTR pszDN,
    PAD_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    pstQuery = pConn->pstFindObjectByDN;
    dwError = sqlite3_bind_text(
            pstQuery,
            1, 
            pszDN,
            -1, // let sqlite calculate the length
            SQLITE_TRANSIENT //let sqlite make its own copy
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = ADCacheDB_QueryObject(pstQuery, ppObject);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    goto cleanup;
}

// Leaves NULLs in pppResults for the objects which can't be found in the
// cache.
DWORD
ADCacheDB_FindObjectsByDNList(
    HANDLE hDb,
    size_t sCount,
    PSTR* ppszDnList,
    PAD_SECURITY_OBJECT **pppResults)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t sIndex;
    PAD_SECURITY_OBJECT* ppResults = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(PAD_SECURITY_OBJECT) * sCount,
                    (PVOID*)&ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    for(sIndex = 0; sIndex < sCount; sIndex++)
    {
        dwError = ADCacheDB_FindObjectByDN(
            hDb,
            ppszDnList[sIndex],
            &ppResults[sIndex]);
        if (dwError == LSA_ERROR_NOT_HANDLED)
            dwError = LSA_ERROR_SUCCESS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppResults = ppResults;

cleanup:
    return dwError;

error:
    ADCacheDB_SafeFreeObjectList(sCount, &ppResults);
    goto cleanup;
}

DWORD
ADCacheDB_FindObjectBySid(
    HANDLE hDb,
    PCSTR pszSid,
    PAD_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    pstQuery = pConn->pstFindObjectBySid;
    dwError = sqlite3_bind_text(
            pstQuery,
            1, 
            pszSid,
            -1, // let sqlite calculate the length
            SQLITE_TRANSIENT //let sqlite make its own copy
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = ADCacheDB_QueryObject(pstQuery, ppObject);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    goto cleanup;
}

// Leaves NULLs in pppResults for the objects which can't be found in the
// cache.
DWORD
ADCacheDB_FindObjectsBySidList(
    HANDLE hDb,
    size_t sCount,
    PSTR* ppszSidList,
    PAD_SECURITY_OBJECT **pppResults)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t sIndex;
    PAD_SECURITY_OBJECT* ppResults = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(PAD_SECURITY_OBJECT) * sCount,
                    (PVOID*)&ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    for(sIndex = 0; sIndex < sCount; sIndex++)
    {
        dwError = ADCacheDB_FindObjectBySid(
            hDb,
            ppszSidList[sIndex],
            &ppResults[sIndex]);
        if (dwError == LSA_ERROR_NOT_HANDLED)
            dwError = LSA_ERROR_SUCCESS;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppResults = ppResults;

cleanup:
    return dwError;

error:
    ADCacheDB_SafeFreeObjectList(sCount, &ppResults);
    goto cleanup;
}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the cache
DWORD
ADCacheDB_GetPasswordVerifier(
    IN HANDLE hDb,
    IN PCSTR pszUserSid,
    OUT PAD_PASSWORD_VERIFIER *ppResult
    )
{
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    const int nExpectedCols = 4;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PAD_PASSWORD_VERIFIER pResult = NULL;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    pstQuery = pConn->pstGetPasswordVerifier;
    dwError = sqlite3_bind_text(
            pstQuery,
            1, 
            pszUserSid,
            -1, // let sqlite calculate the length
            SQLITE_TRANSIENT //let sqlite make its own copy
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        // No results found
        dwError = LSA_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_ROW)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError,
                sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    }

    nGotColumns = sqlite3_column_count(pstQuery);
    if (nGotColumns != nExpectedCols)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(AD_PASSWORD_VERIFIER),
                    (PVOID*)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_UnpackCacheInfo(
        pstQuery,
        &iColumnPos,
        &pResult->cache);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        &iColumnPos,
        "ObjectSid",
        &pResult->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteString(
        pstQuery,
        &iColumnPos,
        "PasswordVerifier",
        &pResult->pszPasswordVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_ROW)
    {
        dwError = LSA_ERROR_DUPLICATE_USERNAME;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError,
                sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    }
    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));

    *ppResult = pResult;
    
cleanup:
    
    LEAVE_CACHEDB_LOCK(pConn, bInLock);
    return dwError;

error:
    *ppResult = NULL;
    sqlite3_reset(pstQuery);
    ADCACHEDB_SAFE_FREE_PASSWORD_VERIFIER(pResult);

    goto cleanup;
}

void
ADCacheDB_FreePasswordVerifier(
    IN OUT PAD_PASSWORD_VERIFIER pVerifier
    )
{
    LSA_SAFE_FREE_STRING(pVerifier->pszObjectSid);
    LSA_SAFE_FREE_STRING(pVerifier->pszPasswordVerifier);
    LSA_SAFE_FREE_MEMORY(pVerifier);
}

DWORD
ADCacheDB_CachePasswordVerifier(
    HANDLE hDb,
    PAD_PASSWORD_VERIFIER pVerifier
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszSqlCommand = NULL;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;

    if (pVerifier->cache.qwCacheId == -1)
    {
        pszSqlCommand = sqlite3_mprintf(
            "begin;"
                "insert into lwicachetags ("
                    "LastUpdated"
                    ") values ("
                    "%ld);\n"
                "replace into lwipasswordverifiers ("
                    "CacheId, "
                    "ObjectSid, "
                    "PasswordVerifier"
                ") values ("
                    "last_insert_rowid(),"
                    "%Q,"
                    "%Q);\n"
                "%s"
            "end;",
            pVerifier->cache.tLastUpdated,
            pVerifier->pszObjectSid,
            pVerifier->pszPasswordVerifier,
            ADCACHEDB_FREE_UNUSED_CACHEIDS);
    }
    else
    {
        pszSqlCommand = sqlite3_mprintf(
            "begin;"
                "replace into lwipasswordverifiers ("
                    "CacheId, "
                    "ObjectSid, "
                    "PasswordVerifier"
                ") values ("
                    "%lld,"
                    "%Q,"
                    "%Q);\n"
                "%s"
            "end;",
            pVerifier->cache.qwCacheId,
            pVerifier->pszObjectSid,
            pVerifier->pszPasswordVerifier,
            ADCACHEDB_FREE_UNUSED_CACHEIDS);
    }

    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    dwError = ADCacheDB_ExecWithRetry(
        pConn,
        pszSqlCommand);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    return dwError;

error:

    goto cleanup;
}

DWORD
ADCacheDB_GetProviderData(
    IN HANDLE hDb,
    OUT PAD_PROVIDER_DATA* ppResult
    )
{
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    const int nExpectedCols = 7;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PAD_PROVIDER_DATA pResult = NULL;
    DWORD dwTemp = 0;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    pstQuery = pConn->pstGetProviderData;

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        // No results found
        dwError = ENOENT;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_ROW)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError,
                sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    }

    nGotColumns = sqlite3_column_count(pstQuery);
    if (nGotColumns != nExpectedCols)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaAllocateMemory(
                    sizeof(AD_PROVIDER_DATA),
                    (PVOID*)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        &iColumnPos,
        "DirectoryMode",
        &pResult->dwDirectoryMode);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteUInt32(
        pstQuery,
        &iColumnPos,
        "ADConfigurationMode",
        &dwTemp);
    BAIL_ON_LSA_ERROR(dwError);
    pResult->adConfigurationMode = dwTemp;

    dwError = ADCacheDB_ReadSqliteUInt64(
        pstQuery,
        &iColumnPos,
        "ADMaxPwdAge",
        &pResult->adMaxPwdAge);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteStringInPlace(
        pstQuery,
        &iColumnPos,
        "Domain",
        pResult->szDomain,
        sizeof(pResult->szDomain));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteStringInPlace(
        pstQuery,
        &iColumnPos,
        "ShortDomain",
        pResult->szShortDomain,
        sizeof(pResult->szShortDomain));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteStringInPlace(
        pstQuery,
        &iColumnPos,
        "ComputerDN",
        pResult->szComputerDN,
        sizeof(pResult->szComputerDN));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDB_ReadSqliteStringInPlace(
        pstQuery,
        &iColumnPos,
        "CellDN",
        pResult->cell.szCellDN,
        sizeof(pResult->cell.szCellDN));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_ROW)
    {
        dwError = LSA_ERROR_DUPLICATE_DOMAINNAME;
        BAIL_ON_LSA_ERROR(dwError);
    }
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    else
    {
        BAIL_ON_SQLITE3_ERROR(dwError,
                sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    }
    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));

    dwError = ADCacheDB_GetCellListNoLock(
        hDb,
        &pResult->pCellList
        );
    BAIL_ON_LSA_ERROR(dwError);

    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    *ppResult = pResult;
    
cleanup:
    
    return dwError;

error:
    *ppResult = NULL;
    sqlite3_reset(pstQuery);
    if (pResult)
    {
        ADProviderFreeProviderData(pResult);
    }
    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    goto cleanup;
}

DWORD
ADCacheDB_CacheProviderData(
    IN HANDLE hDb,
    IN PAD_PROVIDER_DATA pProvider
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszSqlCommand = NULL;
    PSTR pszCellCommand = NULL;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;

    dwError = ADCacheDB_GetCacheCellListCommand(
                hDb,
                pProvider->pCellList,
                &pszCellCommand);
    BAIL_ON_LSA_ERROR(dwError);

    pszSqlCommand = sqlite3_mprintf(
        "begin;"
            "replace into lwiproviderdata ("
                "DirectoryMode, "
                "ADConfigurationMode, "
                "ADMaxPwdAge, "
                "Domain, "
                "ShortDomain, "
                "ComputerDN, "
                "CellDN "
            ") values ("
                "%d,"
                "%d,"
                "%lld,"
                "%Q,"
                "%Q,"
                "%Q,"
                "%Q"
            ");\n"
            "%s"
        "end;",
        pProvider->dwDirectoryMode,
        pProvider->adConfigurationMode,
        pProvider->adMaxPwdAge,
        pProvider->szDomain,
        pProvider->szShortDomain,
        pProvider->szComputerDN,
        pProvider->cell.szCellDN,
        pszCellCommand
    );

    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    dwError = ADCacheDB_ExecWithRetry(
        pConn,
        pszSqlCommand);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    SQLITE3_SAFE_FREE_STRING(pszCellCommand);
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    return dwError;

error:

    goto cleanup;
}

DWORD
ADCacheDB_GetDomainTrustList(
    IN HANDLE hDb,
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    OUT PDLINKEDLIST* ppList
    )
{
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    const int nExpectedCols = 13;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PDLINKEDLIST pList = NULL;
    PLSA_DM_ENUM_DOMAIN_INFO pEntry = NULL;

    ENTER_CACHEDB_LOCK(pConn, bInLock);

    pstQuery = pConn->pstGetDomainTrustList;

    while (TRUE)
    {
        dwError = (DWORD)sqlite3_step(pstQuery);
        if (dwError == SQLITE_DONE)
        {
            // No more results
            dwError = LSA_ERROR_SUCCESS;
            break;
        }
        if (dwError == SQLITE_ROW)
        {
            dwError = LSA_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_SQLITE3_ERROR(dwError,
                    sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
        }

        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                        sizeof(*pEntry),
                        (PVOID*)&pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        iColumnPos = 0;
        dwError = ADCacheDB_UnpackDomainTrust(
                    pstQuery,
                    &iColumnPos,
                    pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDLinkedListAppend(
                    &pList,
                    pEntry);
        BAIL_ON_LSA_ERROR(dwError);
        pEntry = NULL;
    }

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    *ppList = pList;
    
cleanup:
    
    return dwError;

error:
    *ppList = NULL;
    sqlite3_reset(pstQuery);
    if (pList)
    {
        ADCacheDB_FreeEnumDomainInfoList(pList);
    }
    if (pEntry)
    {
        ADCacheDB_FreeEnumDomainInfo(pEntry);
    }
    LEAVE_CACHEDB_LOCK(pConn, bInLock);

    goto cleanup;
}

DWORD
ADCacheDB_CacheDomainTrustList(
    IN HANDLE hDb,
    IN PLSA_DM_ENUM_DOMAIN_INFO* ppDomainInfo,
    IN DWORD dwDomainInfoCount
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszOldExpression = NULL;
    PSTR pszSqlCommand = NULL;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    DWORD dwIndex = 0;
    const LSA_DM_ENUM_DOMAIN_INFO* pDomain = NULL;
    char szGuid[UUID_STR_SIZE];
    PSTR pszSid = NULL;
    PWSTR pwszSid = NULL;

    pszSqlCommand = sqlite3_mprintf(
        "begin;\n"
            "delete from lwidomaintrusts;\n");
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    for (dwIndex = 0; dwIndex < dwDomainInfoCount; dwIndex++)
    {
        pDomain = ppDomainInfo[dwIndex];
        
        LSA_SAFE_FREE_MEMORY(pwszSid);
        LSA_SAFE_FREE_STRING(pszSid);

        if (pDomain->pSid != NULL)
        {
            dwError = SidToString(
                    pDomain->pSid,
                    &pwszSid);
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LsaWc16sToMbs(
                    pwszSid,
                    &pszSid);
            BAIL_ON_LSA_ERROR(dwError);
        }

        // Writes into a 37-byte caller allocated string
        uuid_unparse(*pDomain->pGuid, szGuid);

        SQLITE3_SAFE_FREE_STRING(pszOldExpression);
        pszOldExpression = pszSqlCommand;
        pszSqlCommand = sqlite3_mprintf(
            "%s"
            "replace into lwidomaintrusts ("
                "RowIndex, "
                "DnsDomainName, "
                "NetbiosDomainName, "
                "Sid, "
                "Guid, "
                "TrusteeDnsDomainName, "
                "TrustFlags, "
                "TrustType, "
                "TrustAttributes, "
                "TrustDirection, "
                "TrustMode, "
                "ForestName, "
                "Flags "
            ") values ("
                "%lu, "
                "%Q, "
                "%Q, "
                "%Q, "
                "%Q, "
                "%Q, "
                "%d, "
                "%d, "
                "%d, "
                "%d, "
                "%d, "
                "%Q, "
                "%d "
            ");\n",
            pszOldExpression,
            dwIndex,
            pDomain->pszDnsDomainName,
            pDomain->pszNetbiosDomainName,
            pszSid,
            szGuid,
            pDomain->pszTrusteeDnsDomainName,
            pDomain->dwTrustFlags,
            pDomain->dwTrustType,
            pDomain->dwTrustAttributes,
            pDomain->dwTrustDirection,
            pDomain->dwTrustMode,
            pDomain->pszForestName,
            pDomain->pszClientSiteName,
            pDomain->Flags);

        if (pszSqlCommand == NULL)
        {
            dwError = (DWORD)sqlite3_errcode(pConn->pDb);
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
        }
    }

    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    pszOldExpression = pszSqlCommand;
    pszSqlCommand = sqlite3_mprintf(
            "%s"
        "end;",
        pszOldExpression
        );
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    dwError = ADCacheDB_ExecWithRetry(
        pConn,
        pszSqlCommand);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    LSA_SAFE_FREE_MEMORY(pwszSid);
    LSA_SAFE_FREE_STRING(pszSid);
    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    return dwError;

error:

    goto cleanup;
}

DWORD
ADCacheDB_GetCellListNoLock(
    IN HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN OUT PDLINKEDLIST* ppList
    )
{
    DWORD dwError = 0;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    const int nExpectedCols = 4;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PDLINKEDLIST pList = NULL;
    PAD_LINKED_CELL_INFO pEntry = NULL;

    pstQuery = pConn->pstGetCellList;

    while (TRUE)
    {
        dwError = (DWORD)sqlite3_step(pstQuery);
        if (dwError == SQLITE_DONE)
        {
            // No more results
            dwError = LSA_ERROR_SUCCESS;
            break;
        }
        if (dwError == SQLITE_ROW)
        {
            dwError = LSA_ERROR_SUCCESS;
        }
        else
        {
            BAIL_ON_SQLITE3_ERROR(dwError,
                    sqlite3_errmsg(sqlite3_db_handle(pstQuery)));
        }

        nGotColumns = sqlite3_column_count(pstQuery);
        if (nGotColumns != nExpectedCols)
        {
            dwError = LSA_ERROR_DATA_ERROR;
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                        sizeof(*pEntry),
                        (PVOID*)&pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheDB_UnpackLinkedCellInfo(
                    pstQuery,
                    &iColumnPos,
                    pEntry);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDLinkedListAppend(
                    &pList,
                    pEntry);
        BAIL_ON_LSA_ERROR(dwError);
        pEntry = NULL;
    }

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));

    *ppList = pList;
    
cleanup:
    
    return dwError;

error:
    *ppList = NULL;
    sqlite3_reset(pstQuery);
    if (pList)
    {
        ADProviderFreeCellList(pList);
    }
    if (pEntry)
    {
        ADProviderFreeCellInfo(pEntry);
    }
    goto cleanup;
}

DWORD
ADCacheDB_GetCacheCellListCommand(
    IN HANDLE hDb,
    // Contains type PAD_LINKED_CELL_INFO
    IN const DLINKEDLIST* pCellList,
    OUT PSTR* ppszCommand
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszOldExpression = NULL;
    PSTR pszSqlCommand = NULL;
    PCACHE_CONNECTION pConn = (PCACHE_CONNECTION)hDb;
    const DLINKEDLIST* pPos = pCellList;
    size_t sIndex = 0;
    const AD_LINKED_CELL_INFO* pCell = NULL;

    pszSqlCommand = sqlite3_mprintf(
        "delete from lwilinkedcells;\n");
    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    while (pPos != NULL)
    {
        pCell = (const PAD_LINKED_CELL_INFO)pPos->pItem;

        SQLITE3_SAFE_FREE_STRING(pszOldExpression);
        pszOldExpression = pszSqlCommand;
        pszSqlCommand = sqlite3_mprintf(
            "%s"
            "replace into lwilinkedcells ("
                "RowIndex, "
                "CellDN, "
                "Domain, "
                "IsForestCell "
            ") values ("
                "%lu, "
                "%Q, "
                "%Q, "
                "%d "
            ");\n",
            pszOldExpression,
            sIndex++,
            pCell->pszCellDN,
            pCell->pszDomain,
            pCell->bIsForestCell
            );

        if (pszSqlCommand == NULL)
        {
            dwError = (DWORD)sqlite3_errcode(pConn->pDb);
            BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
        }

        pPos = pPos->pNext;
    }

    *ppszCommand = pszSqlCommand;

cleanup:

    SQLITE3_SAFE_FREE_STRING(pszOldExpression);
    return dwError;

error:

    *ppszCommand = NULL;
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    goto cleanup;
}

VOID
ADCacheDB_FreeEnumDomainInfoList(
    // Contains type PLSA_DM_ENUM_DOMAIN_INFO
    IN OUT PDLINKEDLIST pList
    )
{
    LsaDLinkedListForEach(
        pList,
        ADCacheDB_FreeEnumDomainInfoCallback,
        NULL);

    LsaDLinkedListFree(pList);
}

static
VOID
ADCacheDB_FreeEnumDomainInfoCallback(
    IN OUT PVOID pData,
    IN PVOID pUserData
    )
{
    PLSA_DM_ENUM_DOMAIN_INFO pInfo =
        (PLSA_DM_ENUM_DOMAIN_INFO)pData;

    ADCacheDB_FreeEnumDomainInfo(pInfo);
}

static
VOID
ADCacheDB_FreeEnumDomainInfo(
    IN OUT PLSA_DM_ENUM_DOMAIN_INFO pDomainInfo
    )
{
    if (pDomainInfo)
    {
        LSA_SAFE_FREE_STRING(pDomainInfo->pszDnsDomainName);
        LSA_SAFE_FREE_STRING(pDomainInfo->pszNetbiosDomainName);
        LSA_SAFE_FREE_MEMORY(pDomainInfo->pSid);
        LSA_SAFE_FREE_MEMORY(pDomainInfo->pGuid);
        LSA_SAFE_FREE_STRING(pDomainInfo->pszTrusteeDnsDomainName);
        LSA_SAFE_FREE_STRING(pDomainInfo->pszForestName);
        LSA_SAFE_FREE_STRING(pDomainInfo->pszClientSiteName);
        if (pDomainInfo->DcInfo)
        {
            // ISSUE-2008/09/10-dalmeida -- need ASSERT macro
            LSA_LOG_ALWAYS("ASSERT!!! - DcInfo should never be set by DB code!");
        }
        if (pDomainInfo->GcInfo)
        {
            // ISSUE-2008/09/10-dalmeida -- need ASSERT macro
            LSA_LOG_ALWAYS("ASSERT!!! - GcInfo should never be set by DB code!");
        }
        LsaFreeMemory(pDomainInfo);
    }
}

