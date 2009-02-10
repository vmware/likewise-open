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
 *        db.c
 *
 * Abstract:
 *
 *        Caching for AD Provider Database Interface
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *
 */
#include "includes.h"

static
DWORD
LsaDbSetup(
    IN sqlite3* pSqlHandle
    )
{
    DWORD dwError = 0;
    PSTR pszError = NULL;

    dwError = LsaSqliteExec(pSqlHandle,
                                LSA_DB_CREATE_TABLES,
                                &pszError);
    if (dwError)
    {
        LSA_LOG_DEBUG("SQL failed: code = %d, message = '%s'\nSQL =\n%s",
                      dwError, pszError, LSA_DB_CREATE_TABLES);
    }
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);

cleanup:
    SQLITE3_SAFE_FREE_STRING(pszError);
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDbOpen(
    IN PCSTR pszDbPath,
    OUT PLSA_DB_HANDLE phDb
    )
{
    DWORD dwError = 0;
    BOOLEAN bLockCreated = FALSE;
    PLSA_DB_CONNECTION pConn = NULL;
    PSTR pszError = NULL;
    BOOLEAN bExists = FALSE;
    PSTR pszQuery = NULL;
    PCSTR pszEitherQueryFormat =
        "select "
        "%s "
        "from " LSA_DB_TABLE_NAME_CACHE_TAGS ", " LSA_DB_TABLE_NAME_OBJECTS " left outer join " LSA_DB_TABLE_NAME_USERS " ON "
            LSA_DB_TABLE_NAME_OBJECTS ".ObjectSid = " LSA_DB_TABLE_NAME_USERS ".ObjectSid "
            "left outer join " LSA_DB_TABLE_NAME_GROUPS " ON "
            LSA_DB_TABLE_NAME_OBJECTS ".ObjectSid = " LSA_DB_TABLE_NAME_GROUPS ".ObjectSid "
        "where " LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " LSA_DB_TABLE_NAME_OBJECTS ".CacheId AND "
            "%s";
    PCSTR pszUserQueryFormat =
        "select "
        "%s "
        "from " LSA_DB_TABLE_NAME_CACHE_TAGS ", " LSA_DB_TABLE_NAME_USERS " join " LSA_DB_TABLE_NAME_OBJECTS " ON "
            LSA_DB_TABLE_NAME_USERS ".ObjectSid = " LSA_DB_TABLE_NAME_OBJECTS ".ObjectSid "
            "left outer join " LSA_DB_TABLE_NAME_GROUPS " ON "
            LSA_DB_TABLE_NAME_USERS ".ObjectSid = " LSA_DB_TABLE_NAME_GROUPS ".ObjectSid "
        "where " LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " LSA_DB_TABLE_NAME_OBJECTS ".CacheId AND "
            "%s";
    PCSTR pszGroupQueryFormat =
        "select "
        "%s "
        "from " LSA_DB_TABLE_NAME_CACHE_TAGS ", " LSA_DB_TABLE_NAME_GROUPS " join " LSA_DB_TABLE_NAME_OBJECTS " ON "
            LSA_DB_TABLE_NAME_GROUPS ".ObjectSid = " LSA_DB_TABLE_NAME_OBJECTS ".ObjectSid "
            "left outer join " LSA_DB_TABLE_NAME_USERS " ON "
            LSA_DB_TABLE_NAME_GROUPS ".ObjectSid = " LSA_DB_TABLE_NAME_USERS ".ObjectSid "
        "where " LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " LSA_DB_TABLE_NAME_OBJECTS ".CacheId AND "
            "%s";
    PCSTR pszRemoveBySidFormat =
        "delete from %s where ObjectSid = ?1;";
    PSTR pszDbDir = NULL;

    dwError = LsaGetDirectoryFromPath(
                    pszDbPath,
                    &pszDbDir);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(LSA_DB_CONNECTION),
                    (PVOID*)&pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_rwlock_init(&pConn->lock, NULL);
    BAIL_ON_LSA_ERROR(dwError);
    bLockCreated = TRUE;

    dwError = LsaCheckDirectoryExists(pszDbDir, &bExists);
    BAIL_ON_LSA_ERROR(dwError);

    if (!bExists)
    {
        mode_t cacheDirMode = S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;

        dwError = LsaCreateDirectory(pszDbDir, cacheDirMode);
        BAIL_ON_LSA_ERROR(dwError);
    }

    /* restrict access to u+rwx to the db folder */
    dwError = LsaChangeOwnerAndPermissions(pszDbDir, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_open(pszDbPath, &pConn->pDb);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaChangeOwnerAndPermissions(pszDbPath, 0, 0, S_IRWXU);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDbSetup(pConn->pDb);
    BAIL_ON_LSA_ERROR(dwError);

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszUserQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_USERS ".UPN = ?1 || '@' || ?2");
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
        pszEitherQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_OBJECTS ".NetbiosDomainName = ?1 AND "
        LSA_DB_TABLE_NAME_OBJECTS ".SamAccountName = ?2");
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
        pszUserQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_USERS ".AliasName = ?1");
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
        pszGroupQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_GROUPS ".AliasName = ?1");

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
        pszUserQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_USERS ".Uid = ?1");
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
        pszGroupQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_GROUPS ".Gid = ?1");
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
        pszEitherQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_OBJECTS ".DN = ?1");
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
        pszEitherQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_OBJECTS ".ObjectSid = ?1");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstFindObjectBySid,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszUserQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_OBJECTS ".Type = 2 and " LSA_DB_TABLE_NAME_OBJECTS
            ".SamAccountName > ?1 order by "
            LSA_DB_TABLE_NAME_OBJECTS
            ".SamAccountName limit ?2");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstEnumUsers,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszGroupQueryFormat,
        LsaDbGetObjectFieldList(),
        LSA_DB_TABLE_NAME_OBJECTS ".Type = 1 and " LSA_DB_TABLE_NAME_OBJECTS
             ".SamAccountName > ?1 order by " LSA_DB_TABLE_NAME_OBJECTS
             ".SamAccountName limit ?2");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstEnumGroups,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszRemoveBySidFormat,
        LSA_DB_TABLE_NAME_OBJECTS);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstRemoveObjectBySid,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszRemoveBySidFormat,
        LSA_DB_TABLE_NAME_USERS);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstRemoveUserBySid,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    LSA_SAFE_FREE_STRING(pszQuery);
    dwError = LsaAllocateStringPrintf(
        &pszQuery,
        pszRemoveBySidFormat,
        LSA_DB_TABLE_NAME_GROUPS);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            pszQuery,
            -1, //search for null termination in szQuery to get length
            &pConn->pstRemoveGroupBySid,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
            LSA_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".ParentSid, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".ChildSid, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".IsInPac, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".IsInPacOnly, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".IsInLdap, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".IsDomainPrimaryGroup "
            "from " LSA_DB_TABLE_NAME_CACHE_TAGS ", " LSA_DB_TABLE_NAME_MEMBERSHIP " "
            "where " LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " LSA_DB_TABLE_NAME_MEMBERSHIP ".CacheId "
                "AND " LSA_DB_TABLE_NAME_MEMBERSHIP ".ParentSid = ?1",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetGroupMembers,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
            LSA_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".ParentSid, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".ChildSid, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".IsInPac, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".IsInPacOnly, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".IsInLdap, "
            LSA_DB_TABLE_NAME_MEMBERSHIP ".IsDomainPrimaryGroup "
            "from " LSA_DB_TABLE_NAME_CACHE_TAGS ", " LSA_DB_TABLE_NAME_MEMBERSHIP " "
            "where " LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " LSA_DB_TABLE_NAME_MEMBERSHIP ".CacheId "
                "AND " LSA_DB_TABLE_NAME_MEMBERSHIP ".ChildSid = ?1",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetGroupsForUser,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select "
            LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
            LSA_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
            LSA_DB_TABLE_NAME_VERIFIERS ".ObjectSid, "
            LSA_DB_TABLE_NAME_VERIFIERS ".PasswordVerifier "
            "from " LSA_DB_TABLE_NAME_CACHE_TAGS ", " LSA_DB_TABLE_NAME_VERIFIERS " "
            "where " LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId = " LSA_DB_TABLE_NAME_VERIFIERS ".CacheId "
                "AND " LSA_DB_TABLE_NAME_VERIFIERS ".ObjectSid = ?1",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetPasswordVerifier,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "insert into " LSA_DB_TABLE_NAME_CACHE_TAGS " ("
                "LastUpdated"
                ") values ("
                "?1)",
            -1,
            &pConn->pstInsertCacheTag,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "select last_insert_rowid()",
            -1, //search for null termination in szQuery to get length
            &pConn->pstGetLastInsertedRow,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "update OR IGNORE " LSA_DB_TABLE_NAME_MEMBERSHIP " set "
                "CacheId = ?1,"
                "IsInPacOnly = 0,"
                "IsInLdap = 1"
            " where ParentSid = ?2 AND ChildSid = ?3",
            -1,
            &pConn->pstSetLdapMembership,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "update OR IGNORE " LSA_DB_TABLE_NAME_MEMBERSHIP " set "
                "CacheId = ?1,"
                "IsDomainPrimaryGroup = 1"
            " where ParentSid = ?2 AND ChildSid = ?3",
            -1,
            &pConn->pstSetPrimaryGroupMembership,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_prepare_v2(
            pConn->pDb,
            "insert OR IGNORE into " LSA_DB_TABLE_NAME_MEMBERSHIP " ("
                "CacheId, "
                "ParentSid, "
                "ChildSid, "
                "IsInPac, "
                "IsInPacOnly, "
                "IsInLdap, "
                "IsDomainPrimaryGroup"
            ") values ("
                "?1,"
                "?2,"
                "?3,"
                "?4,"
                "?5,"
                "?6,"
                "?7)",
            -1,
            &pConn->pstAddMembership,
            NULL);
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    *phDb = pConn;

cleanup:

    if (pszError != NULL)
    {
        sqlite3_free(pszError);
    }
    LSA_SAFE_FREE_STRING(pszQuery);
    LSA_SAFE_FREE_STRING(pszDbDir);

    return dwError;

error:
    if (pConn != NULL)
    {
        if (bLockCreated)
        {
            pthread_rwlock_destroy(&pConn->lock);
        }
        LsaDbFreePreparedStatements(pConn);

        if (pConn->pDb != NULL)
        {
            sqlite3_close(pConn->pDb);
        }
        LSA_SAFE_FREE_MEMORY(pConn);
    }
    *phDb = (HANDLE)NULL;

    goto cleanup;
}

static
DWORD
LsaDbFreePreparedStatements(
    IN OUT PLSA_DB_CONNECTION pConn
    )
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

        &pConn->pstRemoveObjectBySid,
        &pConn->pstRemoveUserBySid,
        &pConn->pstRemoveGroupBySid,

        &pConn->pstEnumUsers,
        &pConn->pstEnumGroups,

        &pConn->pstGetGroupMembers,
        &pConn->pstGetGroupsForUser,

        &pConn->pstGetPasswordVerifier,

        &pConn->pstInsertCacheTag,
        &pConn->pstGetLastInsertedRow,
        &pConn->pstSetLdapMembership,
        &pConn->pstSetPrimaryGroupMembership,
        &pConn->pstAddMembership,
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
LsaDbSafeClose(
    PLSA_DB_HANDLE phDb
    )
{
    // This function cannot return an error, only log errors that occur
    // along the way
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_DB_CONNECTION pConn = NULL;

    if (phDb == NULL)
    {
        goto cleanup;
    }

    pConn = (PLSA_DB_CONNECTION)*phDb;

    if (pConn == NULL)
    {
        goto cleanup;
    }

    dwError = LsaDbFreePreparedStatements(pConn);
    if (dwError != LSA_ERROR_SUCCESS)
    {
        LSA_LOG_ERROR("Error freeing prepared statements [%d]", dwError);
        dwError = LSA_ERROR_SUCCESS;
    }

    if (pConn->pDb != NULL)
    {
        sqlite3_close(pConn->pDb);
        pConn->pDb = NULL;
    }

    dwError = pthread_rwlock_destroy(&pConn->lock);
    if (dwError != LSA_ERROR_SUCCESS)
    {
        LSA_LOG_ERROR("Error destroying lock [%d]", dwError);
        dwError = LSA_ERROR_SUCCESS;
    }
    LSA_SAFE_FREE_MEMORY(pConn);

    *phDb = (HANDLE)0;

cleanup:
    return;
}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
LsaDbFindUserByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

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

    dwError = LsaDbQueryObject(pstQuery, &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_User)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    LsaDbSafeFreeObject(&pObject);
    goto cleanup;
}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
LsaDbFindUserById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pstQuery = pConn->pstFindUserById;
    dwError = sqlite3_bind_int64(
            pstQuery,
            1,
            (uint64_t)uid
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = LsaDbQueryObject(pstQuery, &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_User)
    {
        dwError = LSA_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    LsaDbSafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
LsaDbFindGroupByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

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

    dwError = LsaDbQueryObject(pstQuery, &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_Group)
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    LsaDbSafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
LsaDbFindGroupById(
    LSA_DB_HANDLE hDb,
    gid_t gid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pstQuery = pConn->pstFindGroupById;
    dwError = sqlite3_bind_int64(
            pstQuery,
            1,
            (uint64_t)gid
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = LsaDbQueryObject(pstQuery, &pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_Group)
    {
        dwError = LSA_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    LsaDbSafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
LsaDbRemoveUserBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstRemoveObjectBySid;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    dwError = LsaSqliteBindString(pstQuery, 1, pszSid);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    pstQuery = pConn->pstRemoveUserBySid;

    dwError = LsaSqliteBindString(pstQuery, 1, pszSid);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
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
LsaDbRemoveGroupBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstRemoveObjectBySid;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    dwError = LsaSqliteBindString(pstQuery, 1, pszSid);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    pstQuery = pConn->pstRemoveGroupBySid;

    dwError = LsaSqliteBindString(pstQuery, 1, pszSid);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
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
LsaDbEmptyCache(
    IN LSA_DB_HANDLE hDb
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    PCSTR pszEmptyCache =
        "begin;\n"
        "delete from " LSA_DB_TABLE_NAME_CACHE_TAGS ";\n"
        "delete from " LSA_DB_TABLE_NAME_OBJECTS ";\n"
        "delete from " LSA_DB_TABLE_NAME_USERS ";\n"
        "delete from " LSA_DB_TABLE_NAME_VERIFIERS ";\n"
        "delete from " LSA_DB_TABLE_NAME_GROUPS ";\n"
        "delete from " LSA_DB_TABLE_NAME_MEMBERSHIP ";\n"
        "end";

    dwError = LsaSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
        pszEmptyCache);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    return dwError;

error:

    goto cleanup;
}

static
DWORD
LsaDbUnpackCacheInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT_VERSION_INFO pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    dwError = LsaSqliteReadInt64(
        pstQuery,
        piColumnPos,
        "CacheId",
        &pResult->qwDbId);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadTimeT(
        pstQuery,
        piColumnPos,
        "LastUpdated",
        &pResult->tLastUpdated);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

static
DWORD
LsaDbUnpackObjectInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    DWORD dwType = 0;

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "ObjectSid",
        &pResult->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "DN",
        &pResult->pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "Enabled",
        &pResult->enabled);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "NetbiosDomainName",
        &pResult->pszNetbiosDomainName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "SamAccountName",
        &pResult->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Type",
        &dwType);
    BAIL_ON_LSA_ERROR(dwError);

    if (dwType > (UINT8)-1)
    {
        dwError = LSA_ERROR_INTERNAL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    pResult->type = (UINT8) dwType;

error:
    return dwError;
}

static
DWORD
LsaDbUnpackUserInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Uid",
        (DWORD*)&pResult->userInfo.uid);
    if (dwError == LSA_ERROR_INVALID_PARAMETER)
    {
        dwError = LSA_ERROR_DATA_ERROR;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Gid",
        (DWORD*)&pResult->userInfo.gid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "UPN",
        &pResult->userInfo.pszUPN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
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

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "Passwd",
        &pResult->userInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "Gecos",
        &pResult->userInfo.pszGecos);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "Shell",
        &pResult->userInfo.pszShell);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "Homedir",
        &pResult->userInfo.pszHomedir);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadUInt64(
        pstQuery,
        piColumnPos,
        "PwdLastSet",
        &pResult->userInfo.qwPwdLastSet);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadUInt64(
        pstQuery,
        piColumnPos,
        "AccountExpires",
        &pResult->userInfo.qwAccountExpires);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "GeneratedUPN",
        &pResult->userInfo.bIsGeneratedUPN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "PasswordExpired",
        &pResult->userInfo.bPasswordExpired);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "PasswordNeverExpires",
        &pResult->userInfo.bPasswordNeverExpires);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "PromptPasswordChange",
        &pResult->userInfo.bPromptPasswordChange);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "UserCanChangePassword",
        &pResult->userInfo.bUserCanChangePassword);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "AccountDisabled",
        &pResult->userInfo.bAccountDisabled);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "AccountExpired",
        &pResult->userInfo.bAccountExpired);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "AccountLocked",
        &pResult->userInfo.bAccountLocked);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

static
DWORD
LsaDbUnpackGroupInfo(
    sqlite3_stmt *pstQuery,
    int *piColumnPos,
    PLSA_SECURITY_OBJECT pResult)
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    dwError = LsaSqliteReadUInt32(
        pstQuery,
        piColumnPos,
        "Gid",
        (DWORD*)&pResult->groupInfo.gid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
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

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "Passwd",
        &pResult->groupInfo.pszPasswd);
    BAIL_ON_LSA_ERROR(dwError);

error:
    return dwError;
}

static
DWORD
LsaDbUnpackGroupMembershipInfo(
    IN sqlite3_stmt* pstQuery,
    IN OUT int* piColumnPos,
    IN OUT PLSA_GROUP_MEMBERSHIP pResult
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "ParentSid",
        &pResult->pszParentSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        piColumnPos,
        "ChildSid",
        &pResult->pszChildSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "IsInPac",
        &pResult->bIsInPac);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "IsInPacOnly",
        &pResult->bIsInPacOnly);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "IsInLdap",
        &pResult->bIsInLdap);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadBoolean(
        pstQuery,
        piColumnPos,
        "IsDomainPrimaryGroup",
        &pResult->bIsDomainPrimaryGroup);
    BAIL_ON_LSA_ERROR(dwError);

    // Except for NULL entries, memberships must come from the PAC or LDAP.
    if (pResult->pszParentSid != NULL &&
        pResult->pszChildSid != NULL &&
        !pResult->bIsInPac && !pResult->bIsInLdap)
    {
        dwError = LSA_ERROR_UNEXPECTED_DB_RESULT;
        BAIL_ON_LSA_ERROR(dwError);
    }
    // See the definition of LSA_GROUP_MEMBERSHIP
    if (pResult->bIsInPacOnly && (!pResult->bIsInPac || pResult->bIsInLdap))
    {
        dwError = LSA_ERROR_UNEXPECTED_DB_RESULT;
        BAIL_ON_LSA_ERROR(dwError);
    }

error:
    return dwError;
}

DWORD
LsaDbStoreObjectEntry(
    LSA_DB_HANDLE hDb,
    PLSA_SECURITY_OBJECT pObject
    )
{
    return LsaDbStoreObjectEntries(
            hDb,
            1,
            &pObject);
}

DWORD
LsaDbStoreObjectEntries(
    LSA_DB_HANDLE hDb,
    size_t  sObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects
    )
{
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t sIndex = 0;
    //Free with sqlite3_free
    char *pszError = NULL;
    //Free with sqlite3_free
    char *pszNewStatement = NULL;
    LSA_STRING_BUFFER buffer = {0};
    BOOLEAN bGotNow = FALSE;
    time_t now = 0;

    /* This function generates a SQL transaction to update multiple
     * entries at a time. The SQL command is in this format:
     * 1. Delete database tag entries which are no longer referenced.
     * 2. Create/update the new database tag entries, and create/update the
     *    " LSA_DB_TABLE_NAME_OBJECTS ".
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

        if (ppObjects[sIndex]->version.qwDbId == -1)
        {
            /* We don't know whether this object already has a database tag in
             * the database. If there is one, it needs to be deleted.
             *
             * I tried to create one delete statement for every object, with
             * a very long expression, but sqlite didn't like that.
             */
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                "delete from " LSA_DB_TABLE_NAME_CACHE_TAGS " where CacheId IN "
                    "( select CacheId from " LSA_DB_TABLE_NAME_OBJECTS " where ObjectSid = %Q)",
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

        if (ppObjects[sIndex]->version.qwDbId == -1)
        {
            if (!bGotNow)
            {
                dwError = LsaGetCurrentTimeSeconds(&now);
                BAIL_ON_LSA_ERROR(dwError);

                bGotNow = TRUE;
            }

            // The object is either not stored yet, or the existing entry
            // needs to be replaced.
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                // Make a new entry
                "insert into " LSA_DB_TABLE_NAME_CACHE_TAGS " ("
                    "LastUpdated"
                    ") values ("
                    "%ld);\n"
                "replace into " LSA_DB_TABLE_NAME_OBJECTS " ("
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
                now,
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
            // The object is already stored. Just update the existing info.
            pszNewStatement = sqlite3_mprintf(
                ";\n"
                    // Update the existing entry
                    "update " LSA_DB_TABLE_NAME_CACHE_TAGS " set "
                        "LastUpdated = %ld "
                        "where CacheId = %llu;\n"
                    "update " LSA_DB_TABLE_NAME_OBJECTS " set "
                        "CacheId = %llu, "
                        "Enabled = %d, "
                        "NetbiosDomainName = %Q, "
                        "SamAccountName = %Q, "
                        "Type = %d "
                        "where ObjectSid = %Q",
                ppObjects[sIndex]->version.tLastUpdated,
                ppObjects[sIndex]->version.qwDbId,
                ppObjects[sIndex]->version.qwDbId,
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
                        "replace into " LSA_DB_TABLE_NAME_USERS " ("
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
                        "replace into " LSA_DB_TABLE_NAME_GROUPS " ("
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

    dwError = LsaSqliteExecWithRetry(
        pConn->pDb,
        &pConn->lock,
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
LsaDbSafeFreeObject(
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    PLSA_SECURITY_OBJECT pObject = NULL;
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

static
DWORD
LsaDbCreateCacheTag(
    IN PLSA_DB_CONNECTION pConn,
    IN time_t tLastUpdated,
    OUT int64_t *pqwCacheId
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstInsertCacheTag;
    int64_t qwDbId;

    dwError = LsaSqliteBindInt64(pstQuery, 1, tLastUpdated);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    pstQuery = pConn->pstGetLastInsertedRow;

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        // The value is missing
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwError == SQLITE_ROW)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    if (sqlite3_column_count(pstQuery) != 1)
    {
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }

    qwDbId = sqlite3_column_int64(pstQuery, 0);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_ROW)
    {
        // Duplicate value
        dwError = LSA_ERROR_DATA_ERROR;
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
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

static
DWORD
LsaDbUpdateMembership(
    IN sqlite3_stmt* pstQuery,
    IN int64_t CacheId,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;

    dwError = LsaSqliteBindInt64(pstQuery, 1, CacheId);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = LsaSqliteBindString(pstQuery, 2, pszParentSid);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = LsaSqliteBindString(pstQuery, 3, pszChildSid);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

cleanup:
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
LsaDbAddMembership(
    IN PLSA_DB_CONNECTION pConn,
    IN time_t tLastUpdated,
    IN int64_t CacheId,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid,
    IN BOOLEAN bIsInPac,
    IN BOOLEAN bIsInPacOnly,
    IN BOOLEAN bIsInLdap,
    IN BOOLEAN bIsDomainPrimaryGroup
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    // Do not free
    sqlite3_stmt *pstQuery = pConn->pstAddMembership;

    dwError = LsaSqliteBindInt64(pstQuery, 1, CacheId);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = LsaSqliteBindString(pstQuery, 2, pszParentSid);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = LsaSqliteBindString(pstQuery, 3, pszChildSid);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = LsaSqliteBindBoolean(pstQuery, 4, bIsInPac);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = LsaSqliteBindBoolean(pstQuery, 5, bIsInPacOnly);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = LsaSqliteBindBoolean(pstQuery, 6, bIsInLdap);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = LsaSqliteBindBoolean(pstQuery, 7, bIsDomainPrimaryGroup);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_step(pstQuery);
    if (dwError == SQLITE_DONE)
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

cleanup:
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
LsaDbStoreGroupMembershipCallback(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_DB_STORE_GROUP_MEMBERSHIP_CONTEXT pArgs = (PLSA_DB_STORE_GROUP_MEMBERSHIP_CONTEXT)pContext;
    PCSTR pszParentSid = pArgs->pszParentSid;
    PLSA_GROUP_MEMBERSHIP* ppMembers = pArgs->ppMembers;
    size_t sMemberCount = pArgs->sMemberCount;
    size_t iMember;
    BOOLEAN bCreatedTag = FALSE;
    int64_t qwNewCacheId = -1;
    PSTR pszSqlCommand = NULL;
    PSTR pszError = NULL;
    time_t now = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Start the transaction
    //
    // 1) Clear all group members for child SID.  However, keep
    //    the PAC and primary group ones.
    //
    // 2) Update any remaining PAC items to clear IsInLdap so that we
    //    can set it later in the transaction depending on what membership
    //    info got passed in.
    //
    dwError = LsaSqliteAllocPrintf(&pszSqlCommand,
        "begin;\n"
        "    delete from " LSA_DB_TABLE_NAME_MEMBERSHIP " where\n"
        "        ParentSid = %Q AND\n"
        "        IsInPac = 0 AND\n"
        "        IsDomainPrimaryGroup = 0;\n"
        // ISSUE-2008/11/03-dalmeida -- Do we want to set update time here?
        "    update OR IGNORE " LSA_DB_TABLE_NAME_MEMBERSHIP " set\n"
        "        IsInLdap = 0\n"
        "        where ParentSid = %Q;\n"
        "",
        pszParentSid,
        pszParentSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteExec(pDb, pszSqlCommand, &pszError);
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);

    //
    // Put memberships into the version.
    //
    // 1) We update any remaining entries (which must be from PAC
    //    or primary group).
    //
    // 2) Insert new entries (which cannot be from PAC).
    //
    for (iMember = 0; iMember < sMemberCount; iMember++)
    {
        if (!bCreatedTag)
        {
            dwError = LsaDbCreateCacheTag(
                    pArgs->pConn,
                    now,
                    &qwNewCacheId);
            BAIL_ON_LSA_ERROR(dwError);

            bCreatedTag = TRUE;
        }

        if (ppMembers[iMember]->bIsInLdap)
        {
            dwError = LsaDbUpdateMembership(
                            pArgs->pConn->pstSetLdapMembership,
                            qwNewCacheId,
                            pszParentSid,
                            ppMembers[iMember]->pszChildSid);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaDbAddMembership(
                        pArgs->pConn,
                        now,
                        qwNewCacheId,
                        pszParentSid,
                        ppMembers[iMember]->pszChildSid,
                        FALSE,
                        FALSE,
                        TRUE,
                        FALSE);
        BAIL_ON_LSA_ERROR(dwError);
    }

    //
    // End the transaction
    //
    // 1) Delete any database tags which are no longer used.
    //
    dwError = LsaSqliteExec(
                    pDb,
                    LSA_DB_FREE_UNUSED_CACHEIDS "end;",
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);

cleanup:
    *ppszError = NULL;
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    SQLITE3_SAFE_FREE_STRING(pszError);
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDbStoreGroupMembership(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszParentSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    size_t iMember;
    LSA_DB_STORE_GROUP_MEMBERSHIP_CONTEXT context = { 0 };

    LSA_LOG_VERBOSE("ENTER: Caching %ld group memberships.",
            (unsigned long)sMemberCount);

    //
    // Check each membership for consistency.
    //
    // 1) pszParentSid field must match the group or be NULL.
    //
    // 2) The should be no database ID as this should be fresh data.
    //
    // 3) There must not be any PAC entries.
    //
    for (iMember = 0; iMember < sMemberCount; iMember++)
    {
        assert(ppMembers[iMember]->version.qwDbId == -1);
        if (ppMembers[iMember]->pszParentSid &&
            strcasecmp(ppMembers[iMember]->pszParentSid, pszParentSid))
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
        if (ppMembers[iMember]->bIsInPac)
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    context.pszParentSid = pszParentSid;
    context.sMemberCount = sMemberCount;
    context.ppMembers = ppMembers;
    context.pConn = pConn;

    dwError = LsaSqliteExecCallbackWithRetry(
                    pConn->pDb,
                    &pConn->lock,
                    LsaDbStoreGroupMembershipCallback,
                    &context);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_LOG_VERBOSE("LEAVE: caching group memberships.");
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaDbStoreUserMembershipCallback(
    IN sqlite3 *pDb,
    IN PVOID pContext,
    OUT PSTR* ppszError
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_DB_STORE_USER_MEMBERSHIP_CONTEXT pArgs = (PLSA_DB_STORE_USER_MEMBERSHIP_CONTEXT)pContext;
    PCSTR pszChildSid = pArgs->pszChildSid;
    size_t sMemberCount = pArgs->sMemberCount;
    PLSA_GROUP_MEMBERSHIP* ppMembers = pArgs->ppMembers;
    BOOLEAN bIsPacAuthoritative = pArgs->bIsPacAuthoritative;
    size_t iMember;
    BOOLEAN bCreatedTag = FALSE;
    int64_t qwNewCacheId = -1;
    PSTR pszSqlCommand = NULL;
    PSTR pszError = NULL;
    time_t now = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    //
    // Start the transaction
    //
    // 1) Clear all group members for child SID.  However, we keep the
    //    PAC ones unless we have authoritative PAC info.
    //
    // 2) Update any remaining items to clear IsInLdap and
    //    IsDomainPrimaryGroup so that we can set them later in the
    //    transaction depending on what membership info got passed in.
    //
    dwError = LsaSqliteAllocPrintf(&pszSqlCommand,
        "begin;\n"
        "    delete from " LSA_DB_TABLE_NAME_MEMBERSHIP " where\n"
        "        ChildSid = %Q\n"
        "        %s;\n"
        // ISSUE-2008/11/03-dalmeida -- Do we want to set update time here?
        "    update OR IGNORE " LSA_DB_TABLE_NAME_MEMBERSHIP " set\n"
        "        IsInLdap = 0,\n"
        "        IsDomainPrimaryGroup = 0\n"
        "        where ChildSid = %Q;\n"
        "",
        pszChildSid,
        bIsPacAuthoritative ? "" : "AND IsInPac = 0",
        pszChildSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteExec(pDb, pszSqlCommand, &pszError);
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);

    //
    // Put memberships into the version.
    //
    // 1) We update any remaining entries (which must be from PAC).
    //
    // 2) If inserting a new entry, we need to compute the
    //    "is in PAC only" bit.
    //
    for (iMember = 0; iMember < sMemberCount; iMember++)
    {
        BOOLEAN bIsNewEntryInPacOnly = FALSE;

        if (!bCreatedTag)
        {
            dwError = LsaDbCreateCacheTag(pArgs->pConn, now, &qwNewCacheId);
            BAIL_ON_LSA_ERROR(dwError);

            bCreatedTag = TRUE;
        }

        if (!bIsPacAuthoritative && ppMembers[iMember]->bIsInLdap)
        {
            dwError = LsaDbUpdateMembership(
                            pArgs->pConn->pstSetLdapMembership,
                            qwNewCacheId,
                            ppMembers[iMember]->pszParentSid,
                            pszChildSid);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (!bIsPacAuthoritative && ppMembers[iMember]->bIsDomainPrimaryGroup)
        {
            dwError = LsaDbUpdateMembership(
                            pArgs->pConn->pstSetPrimaryGroupMembership,
                            qwNewCacheId,
                            ppMembers[iMember]->pszParentSid,
                            pszChildSid);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (ppMembers[iMember]->bIsInPac && !ppMembers[iMember]->bIsInLdap)
        {
            bIsNewEntryInPacOnly = TRUE;
        }

        dwError = LsaDbAddMembership(
                        pArgs->pConn,
                        now,
                        qwNewCacheId,
                        ppMembers[iMember]->pszParentSid,
                        pszChildSid,
                        ppMembers[iMember]->bIsInPac,
                        bIsNewEntryInPacOnly,
                        ppMembers[iMember]->bIsInLdap,
                        ppMembers[iMember]->bIsDomainPrimaryGroup);
        BAIL_ON_LSA_ERROR(dwError);
    }

    //
    // End the transaction
    //
    // 1) Delete any database tags which are no longer used.
    //
    dwError = LsaSqliteExec(
                    pDb,
                    LSA_DB_FREE_UNUSED_CACHEIDS "end;",
                    &pszError);
    BAIL_ON_SQLITE3_ERROR(dwError, pszError);

cleanup:
    *ppszError = NULL;
    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    SQLITE3_SAFE_FREE_STRING(pszError);
    return dwError;

error:
    goto cleanup;
}

DWORD
LsaDbStoreGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszChildSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers,
    IN BOOLEAN bIsPacAuthoritative
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    size_t iMember;
    LSA_DB_STORE_USER_MEMBERSHIP_CONTEXT context = { 0 };

    //
    // Check each membership for consistency.
    //
    // 1) pszChildSid field must match the user or be NULL.
    //
    // 2) The should be no database ID as this should be fresh data.
    //
    for (iMember = 0; iMember < sMemberCount; iMember++)
    {
        assert(ppMembers[iMember]->version.qwDbId == -1);
        if (ppMembers[iMember]->pszChildSid &&
            strcasecmp(ppMembers[iMember]->pszChildSid, pszChildSid))
        {
            dwError = LSA_ERROR_INVALID_PARAMETER;
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    context.pszChildSid = pszChildSid;
    context.sMemberCount = sMemberCount;
    context.ppMembers = ppMembers;
    context.bIsPacAuthoritative = bIsPacAuthoritative;
    context.pConn = pConn;

    dwError = LsaSqliteExecCallbackWithRetry(
                    pConn->pDb,
                    &pConn->lock,
                    LsaDbStoreUserMembershipCallback,
                    &context);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    return dwError;

error:
    goto cleanup;
}

static
DWORD
LsaDbGetMemberships(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bIsGroupMembers,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    size_t sResultCapacity = 0;
    size_t sResultCount = 0;
    PLSA_GROUP_MEMBERSHIP *ppResults = NULL;
    const int nExpectedCols = 8;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PLSA_GROUP_MEMBERSHIP pMembership = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    if (bIsGroupMembers)
    {
        pstQuery = pConn->pstGetGroupMembers;
    }
    else
    {
        pstQuery = pConn->pstGetGroupsForUser;
    }

    dwError = LsaSqliteBindString(pstQuery, 1, pszSid);
    BAIL_ON_SQLITE3_ERROR_STMT(dwError, pstQuery);

    while ((dwError = (DWORD)sqlite3_step(pstQuery)) == SQLITE_ROW)
    {
        BOOLEAN bSkip = FALSE;

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
                            sizeof(PLSA_GROUP_MEMBERSHIP) * sResultCapacity);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateMemory(
                        sizeof(*pMembership),
                        (PVOID*)&pMembership);
        BAIL_ON_LSA_ERROR(dwError);

        iColumnPos = 0;

        dwError = LsaDbUnpackCacheInfo(pstQuery,
                        &iColumnPos,
                        &pMembership->version);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaDbUnpackGroupMembershipInfo(pstQuery,
                        &iColumnPos,
                        pMembership);
        BAIL_ON_LSA_ERROR(dwError);

        if (bFilterNotInPacNorLdap)
        {
            // Filter out stuff from PAC that was in LDAP
            // but is no longer in LDAP.
            if (pMembership->bIsInPac &&
                !pMembership->bIsInPacOnly &&
                !pMembership->bIsInLdap)
            {
                bSkip = TRUE;
            }
        }

        if (bSkip)
        {
            LsaDbSafeFreeGroupMembership(&pMembership);
        }
        else
        {
            ppResults[sResultCount] = pMembership;
            pMembership = NULL;
            sResultCount++;
        }
    }
    if (dwError == SQLITE_DONE)
    {
        // No more results found
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR_DB(dwError, pConn->pDb);

    *pppResults = ppResults;
    *psCount = sResultCount;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *psCount = 0;
    *pppResults = NULL;
    LsaDbSafeFreeGroupMembership(&pMembership);
    LsaDbSafeFreeGroupMembershipList(sResultCount, &ppResults);
    if (pstQuery != NULL)
    {
        sqlite3_reset(pstQuery);
    }

    goto cleanup;
}

DWORD
LsaDbGetGroupMembers(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    return LsaDbGetMemberships(hDb, pszSid, TRUE,
                                    bFilterNotInPacNorLdap,
                                    psCount, pppResults);
}

DWORD
LsaDbGetGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    return LsaDbGetMemberships(hDb, pszSid, FALSE,
                                    bFilterNotInPacNorLdap,
                                    psCount, pppResults);
}

void
LsaDbSafeFreeGroupMembership(
        PLSA_GROUP_MEMBERSHIP* ppMembership)
{
    if (*ppMembership != NULL)
    {
        LSA_SAFE_FREE_STRING((*ppMembership)->pszParentSid);
        LSA_SAFE_FREE_STRING((*ppMembership)->pszChildSid);
    }
    LSA_SAFE_FREE_MEMORY(*ppMembership);
}

void
LsaDbSafeFreeGroupMembershipList(
        size_t sCount,
        PLSA_GROUP_MEMBERSHIP** pppMembershipList)
{
    if (*pppMembershipList != NULL)
    {
        size_t iMember;
        for (iMember = 0; iMember < sCount; iMember++)
        {
            LsaDbSafeFreeGroupMembership(&(*pppMembershipList)[iMember]);
        }
        LSA_SAFE_FREE_MEMORY(*pppMembershipList);
    }
}

void
LsaDbSafeFreeObjectList(
        size_t sCount,
        PLSA_SECURITY_OBJECT** pppObjectList)
{
    if (*pppObjectList != NULL)
    {
        size_t sIndex;
        for (sIndex = 0; sIndex < sCount; sIndex++)
        {
            LsaDbSafeFreeObject(&(*pppObjectList)[sIndex]);
        }
        LSA_SAFE_FREE_MEMORY(*pppObjectList);
    }
}

DWORD
LsaDbQueryObjectMulti(
    IN sqlite3_stmt* pstQuery,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    const int nExpectedCols = 29; // This is the number of fields defined in lsadb.h (LSA_SECURITY_OBJECT)
    int iColumnPos = 0;
    PLSA_SECURITY_OBJECT pObject = NULL;
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
                    sizeof(LSA_SECURITY_OBJECT),
                    (PVOID*)&pObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDbUnpackCacheInfo(pstQuery,
                  &iColumnPos,
                  &pObject->version);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDbUnpackObjectInfo(pstQuery,
                  &iColumnPos,
                  pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type == AccountType_User && pObject->enabled)
    {
        dwError = LsaDbUnpackUserInfo(pstQuery,
                      &iColumnPos,
                      pObject);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        iColumnPos += 18; // This is the number of fields in the userInfo section of lsadb.h (LSA_SECURITY_OBJECT)
    }

    if (pObject->type == AccountType_Group && pObject->enabled)
    {
        dwError = LsaDbUnpackGroupInfo(pstQuery,
                      &iColumnPos,
                      pObject);
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:

    return dwError;

error:

    *ppObject = NULL;

    LsaDbSafeFreeObject(&pObject);
    sqlite3_reset(pstQuery);

    goto cleanup;
}

DWORD
LsaDbEnumUsersCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumUsers,
    IN PCSTR                   pszResume,
    OUT DWORD*                 dwNumUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD                 dwError = 0;
    PLSA_DB_CONNECTION    pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN               bInLock = FALSE;
    sqlite3_stmt *        pstQuery = pConn->pstEnumUsers;
    DWORD                 dwUserCount = 0;
    PLSA_SECURITY_OBJECT* ppObjectsLocal = NULL;

    dwError = LsaAllocateMemory(
                  sizeof(PLSA_SECURITY_OBJECT) * dwMaxNumUsers,
                  (PVOID*)&ppObjectsLocal);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    dwError = sqlite3_bind_text(
                  pstQuery,
                  1,
                  pszResume ? pszResume : "",
                  -1, // let sqlite calculate the length
                  SQLITE_TRANSIENT //let sqlite make its own copy
                  );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_bind_int64(
                  pstQuery,
                  2,
                  (uint64_t)dwMaxNumUsers
                  );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    for ( dwUserCount = 0 ;
          dwUserCount < dwMaxNumUsers ;
          dwUserCount++ )
    {
        dwError = LsaDbQueryObjectMulti(
                      pstQuery,
                      &ppObjectsLocal[dwUserCount]);
        if ( dwError )
        {
            break;
        }
    }
    if ( dwError == LSA_ERROR_NOT_HANDLED && dwUserCount > 0 )
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));

    *dwNumUsersFound = dwUserCount;
    *pppObjects = ppObjectsLocal;

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:

    *dwNumUsersFound = 0;
    *pppObjects = NULL;

    LsaDbSafeFreeObjectList(dwUserCount, &ppObjectsLocal);

    sqlite3_reset(pstQuery);

    goto cleanup;
}

DWORD
LsaDbEnumGroupsCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumGroups,
    IN PCSTR                   pszResume,
    OUT DWORD*                 dwNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD                 dwError = 0;
    PLSA_DB_CONNECTION    pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN               bInLock = FALSE;
    sqlite3_stmt *        pstQuery = pConn->pstEnumGroups;
    DWORD                 dwGroupCount = 0;
    PLSA_SECURITY_OBJECT* ppObjectsLocal = NULL;

    dwError = LsaAllocateMemory(
                  sizeof(PLSA_SECURITY_OBJECT) * dwMaxNumGroups,
                  (PVOID*)&ppObjectsLocal);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    dwError = sqlite3_bind_text(
                  pstQuery,
                  1,
                  pszResume ? pszResume : "",
                  -1, // let sqlite calculate the length
                  SQLITE_TRANSIENT //let sqlite make its own copy
                  );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = sqlite3_bind_int64(
                  pstQuery,
                  2,
                  (uint64_t)dwMaxNumGroups
                  );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    for ( dwGroupCount = 0 ;
          dwGroupCount < dwMaxNumGroups ;
          dwGroupCount++ )
    {
        dwError = LsaDbQueryObjectMulti(
                      pstQuery,
                      &ppObjectsLocal[dwGroupCount]);
        if ( dwError )
        {
            break;
        }
    }
    if ( dwError == LSA_ERROR_NOT_HANDLED && dwGroupCount > 0 )
    {
        dwError = LSA_ERROR_SUCCESS;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = (DWORD)sqlite3_reset(pstQuery);
    BAIL_ON_SQLITE3_ERROR(dwError,
            sqlite3_errmsg(sqlite3_db_handle(pstQuery)));

    *dwNumGroupsFound = dwGroupCount;
    *pppObjects = ppObjectsLocal;

cleanup:

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:

    *dwNumGroupsFound = 0;
    *pppObjects = NULL;

    LsaDbSafeFreeObjectList(dwGroupCount, &ppObjectsLocal);
    sqlite3_reset(pstQuery);

    goto cleanup;
}

static
DWORD
LsaDbQueryObject(
    IN sqlite3_stmt* pstQuery,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    const int nExpectedCols = 29; // This is the number of fields defined in lsadb.h (LSA_SECURITY_OBJECT)
    int iColumnPos = 0;
    PLSA_SECURITY_OBJECT pObject = NULL;
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
                    sizeof(LSA_SECURITY_OBJECT),
                    (PVOID*)&pObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDbUnpackCacheInfo(pstQuery,
            &iColumnPos,
            &pObject->version);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDbUnpackObjectInfo(pstQuery,
            &iColumnPos,
            pObject);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type == AccountType_User && pObject->enabled)
    {
        dwError = LsaDbUnpackUserInfo(pstQuery,
                &iColumnPos,
                pObject);
        if (dwError == LSA_ERROR_DATA_ERROR)
        {
            LSA_LOG_ERROR("The user attributes in the cache data for '%s\\%s' are invalid. The cache database or user data in Active Directory could be corrupt.",
                LSA_SAFE_LOG_STRING(pObject->pszNetbiosDomainName),
                LSA_SAFE_LOG_STRING(pObject->pszSamAccountName));
            // Pretend like the whole object is not in the database.
            dwError = LSA_ERROR_NOT_HANDLED;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }
    else
    {
        iColumnPos += 18; // This is the number of fields in the userInfo section of lsadb.h (LSA_SECURITY_OBJECT)
    }

    if (pObject->type == AccountType_Group && pObject->enabled)
    {
        dwError = LsaDbUnpackGroupInfo(pstQuery,
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
    LsaDbSafeFreeObject(&pObject);
    sqlite3_reset(pstQuery);

    goto cleanup;
}

static
PCSTR
LsaDbGetObjectFieldList(
    VOID
    )
{
    return
        LSA_DB_TABLE_NAME_CACHE_TAGS ".CacheId, "
        LSA_DB_TABLE_NAME_CACHE_TAGS ".LastUpdated, "
        LSA_DB_TABLE_NAME_OBJECTS ".ObjectSid, "
        LSA_DB_TABLE_NAME_OBJECTS ".DN, "
        LSA_DB_TABLE_NAME_OBJECTS ".Enabled, "
        LSA_DB_TABLE_NAME_OBJECTS ".NetbiosDomainName, "
        LSA_DB_TABLE_NAME_OBJECTS ".SamAccountName, "
        LSA_DB_TABLE_NAME_OBJECTS ".Type, "
        LSA_DB_TABLE_NAME_USERS ".Uid, "
        LSA_DB_TABLE_NAME_USERS ".Gid, "
        LSA_DB_TABLE_NAME_USERS ".UPN, "
        LSA_DB_TABLE_NAME_USERS ".AliasName, "
        LSA_DB_TABLE_NAME_USERS ".Passwd, "
        LSA_DB_TABLE_NAME_USERS ".Gecos, "
        LSA_DB_TABLE_NAME_USERS ".Shell, "
        LSA_DB_TABLE_NAME_USERS ".Homedir, "
        LSA_DB_TABLE_NAME_USERS ".PwdLastSet, "
        LSA_DB_TABLE_NAME_USERS ".AccountExpires, "
        LSA_DB_TABLE_NAME_USERS ".GeneratedUPN, "
        LSA_DB_TABLE_NAME_USERS ".PasswordExpired, "
        LSA_DB_TABLE_NAME_USERS ".PasswordNeverExpires, "
        LSA_DB_TABLE_NAME_USERS ".PromptPasswordChange, "
        LSA_DB_TABLE_NAME_USERS ".UserCanChangePassword, "
        LSA_DB_TABLE_NAME_USERS ".AccountDisabled, "
        LSA_DB_TABLE_NAME_USERS ".AccountExpired, "
        LSA_DB_TABLE_NAME_USERS ".AccountLocked, "
        LSA_DB_TABLE_NAME_GROUPS ".Gid, "
        LSA_DB_TABLE_NAME_GROUPS ".AliasName, "
        LSA_DB_TABLE_NAME_GROUPS ".Passwd";
}

DWORD
LsaDbFindObjectByDN(
    LSA_DB_HANDLE hDb,
    PCSTR pszDN,
    PLSA_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pstQuery = pConn->pstFindObjectByDN;
    dwError = sqlite3_bind_text(
            pstQuery,
            1,
            pszDN,
            -1, // let sqlite calculate the length
            SQLITE_TRANSIENT //let sqlite make its own copy
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = LsaDbQueryObject(pstQuery, ppObject);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    goto cleanup;
}

// Leaves NULLs in pppResults for the objects which can't be found in the
// version.
DWORD
LsaDbFindObjectsByDNList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszDnList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t sIndex;
    PLSA_SECURITY_OBJECT* ppResults = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(PLSA_SECURITY_OBJECT) * sCount,
                    (PVOID*)&ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    for(sIndex = 0; sIndex < sCount; sIndex++)
    {
        dwError = LsaDbFindObjectByDN(
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
    LsaDbSafeFreeObjectList(sCount, &ppResults);
    goto cleanup;
}

DWORD
LsaDbFindObjectBySid(
    LSA_DB_HANDLE hDb,
    PCSTR pszSid,
    PLSA_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pstQuery = pConn->pstFindObjectBySid;
    dwError = sqlite3_bind_text(
            pstQuery,
            1,
            pszSid,
            -1, // let sqlite calculate the length
            SQLITE_TRANSIENT //let sqlite make its own copy
            );
    BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));

    dwError = LsaDbQueryObject(pstQuery, ppObject);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    goto cleanup;
}

// Leaves NULLs in pppResults for the objects which can't be found in the
// version.
DWORD
LsaDbFindObjectsBySidList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    size_t sIndex;
    PLSA_SECURITY_OBJECT* ppResults = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(PLSA_SECURITY_OBJECT) * sCount,
                    (PVOID*)&ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    for(sIndex = 0; sIndex < sCount; sIndex++)
    {
        dwError = LsaDbFindObjectBySid(
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
    LsaDbSafeFreeObjectList(sCount, &ppResults);
    goto cleanup;
}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
LsaDbGetPasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszUserSid,
    OUT PLSA_PASSWORD_VERIFIER *ppResult
    )
{
    DWORD dwError = 0;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // do not free
    sqlite3_stmt *pstQuery = NULL;
    const int nExpectedCols = 4;
    int iColumnPos = 0;
    int nGotColumns = 0;
    PLSA_PASSWORD_VERIFIER pResult = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

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
                    sizeof(LSA_PASSWORD_VERIFIER),
                    (PVOID*)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaDbUnpackCacheInfo(
        pstQuery,
        &iColumnPos,
        &pResult->version);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
        pstQuery,
        &iColumnPos,
        "ObjectSid",
        &pResult->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaSqliteReadString(
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

    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);
    return dwError;

error:
    *ppResult = NULL;
    sqlite3_reset(pstQuery);
    LSA_DB_SAFE_FREE_PASSWORD_VERIFIER(pResult);

    goto cleanup;
}

void
LsaDbFreePasswordVerifier(
    IN OUT PLSA_PASSWORD_VERIFIER pVerifier
    )
{
    LSA_SAFE_FREE_STRING(pVerifier->pszObjectSid);
    LSA_SAFE_FREE_STRING(pVerifier->pszPasswordVerifier);
    LSA_SAFE_FREE_MEMORY(pVerifier);
}

DWORD
LsaDbStorePasswordVerifier(
    LSA_DB_HANDLE hDb,
    PLSA_PASSWORD_VERIFIER pVerifier
    )
{
    DWORD dwError = LSA_ERROR_SUCCESS;
    PSTR pszSqlCommand = NULL;
    PLSA_DB_CONNECTION pConn = (PLSA_DB_CONNECTION)hDb;

    if (pVerifier->version.qwDbId == -1)
    {
        time_t now = 0;

        dwError = LsaGetCurrentTimeSeconds(&now);
        BAIL_ON_LSA_ERROR(dwError);

        pszSqlCommand = sqlite3_mprintf(
            "begin;"
                "insert into " LSA_DB_TABLE_NAME_CACHE_TAGS " ("
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
            now,
            pVerifier->pszObjectSid,
            pVerifier->pszPasswordVerifier,
            LSA_DB_FREE_UNUSED_CACHEIDS);
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
            pVerifier->version.qwDbId,
            pVerifier->pszObjectSid,
            pVerifier->pszPasswordVerifier,
            LSA_DB_FREE_UNUSED_CACHEIDS);
    }

    if (pszSqlCommand == NULL)
    {
        dwError = (DWORD)sqlite3_errcode(pConn->pDb);
        BAIL_ON_SQLITE3_ERROR(dwError, sqlite3_errmsg(pConn->pDb));
    }

    dwError = LsaSqliteExecWithRetry(
                pConn->pDb,
                &pConn->lock,
                pszSqlCommand);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:

    SQLITE3_SAFE_FREE_STRING(pszSqlCommand);
    return dwError;

error:

    goto cleanup;
}
