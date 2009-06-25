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
 *        memcache.c
 *
 * Abstract:
 *
 *        This is the in-memory implementation of the AD Provider Local Cache
 *
 * Authors: Krishna Ganugapati (kstemen@likewisesoftware.com)
 *
 */
#include "adprovider.h"


#ifdef AD_CACHE_IN_MEMORY


DWORD
MemCacheOpen(
    IN PCSTR pszDbPath,
    OUT PLSA_DB_HANDLE phDb
    )
{
    DWORD dwError = 0;

    return dwError;
}


void
MemCacheSafeClose(
    PLSA_DB_HANDLE phDb
    )
{
    return;
}

DWORD
MemCacheFindUserByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->FindUserByName(
                        hDb,
                        pUserNameInfo,
                        ppObject
                        );
    return dwError;
}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
MemCacheFindUserById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheFindGroupByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheFindGroupById(
    LSA_DB_HANDLE hDb,
    gid_t gid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheRemoveUserBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheRemoveGroupBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheEmptyCache(
    IN LSA_DB_HANDLE hDb
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheStoreObjectEntry(
    LSA_DB_HANDLE hDb,
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->StoreObjectEntry(
                        hDb,
                        pObject
                        );
    return dwError;

}

DWORD
MemCacheStoreObjectEntries(
    LSA_DB_HANDLE hDb,
    size_t  sObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->StoreObjectEntries(
                        hDb,
                        sObjectCount,
                        ppObjects
                        );
    return dwError;
}


DWORD
MemCacheUpdateMembership(
    IN sqlite3_stmt* pstQuery,
    IN int64_t CacheId,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid
    )
{
}



DWORD
MemCacheStoreGroupMembership(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszParentSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
MemCacheStoreGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszChildSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers,
    IN BOOLEAN bIsPacAuthoritative
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
MemCacheGetMemberships(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bIsGroupMembers,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    DWORD dwError = 0;

    dwError = pCacheProvider->GetMemberships(
                    hDb,
                    pszSid,
                    bIsGroupMembers,
                    bFilterNotInPacNorLdap,
                    psCount,
                    pppResults
                    );

    return dwError;
}

DWORD
MemCacheGetGroupMembers(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheGetGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    )
{
    DWORD dwError = 0;

    return dwError;
}


DWORD
MemCacheEnumUsersCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumUsers,
    IN PCSTR                   pszResume,
    OUT DWORD*                 dwNumUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheEnumGroupsCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumGroups,
    IN PCSTR                   pszResume,
    OUT DWORD*                 dwNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;


    return dwError;
}


DWORD
MemCacheFindObjectByDN(
    LSA_DB_HANDLE hDb,
    PCSTR pszDN,
    PLSA_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;

    dwError = pCacheProvider->FindObjectByDN(
                        hDb,
                        pszDN,
                        ppObject
                        );
    return dwError;
}

DWORD
MemCacheFindObjectsByDNList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszDnList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheFindObjectBySid(
    LSA_DB_HANDLE hDb,
    PCSTR pszSid,
    PLSA_SECURITY_OBJECT *ppObject)
{
    DWORD dwError = 0;


    return dwError;
}

// Leaves NULLs in pppResults for the objects which can't be found in the
// version.
DWORD
MemCacheFindObjectsBySidList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    )
{
    DWORD dwError = 0;

    return dwError;
}

// returns LSA_ERROR_NOT_HANDLED if the user is not in the database
DWORD
MemCacheGetPasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszUserSid,
    OUT PLSA_PASSWORD_VERIFIER *ppResult
    )
{
    DWORD dwError = 0;

    return dwError;
}

void
MemCacheFreePasswordVerifier(
    IN OUT PLSA_PASSWORD_VERIFIER pVerifier
    )
{
    DWORD dwError = 0;

    return dwError;
}

DWORD
MemCacheStorePasswordVerifier(
    LSA_DB_HANDLE hDb,
    PLSA_PASSWORD_VERIFIER pVerifier
    )
{
    DWORD dwError = 0;

    return dwError;
}


void
MemCacheInitProvider()
{
    MemCacheTable.pfnOPENHANDLE               = MemCacheOpenHandle;
    MemCacheTable.pfnSafeClose                = MemCacheSafeClose;
    MemCacheTable.pfnFindUserByName           = MemCacheFindUserByName;
    MemCacheTable.pfnFindUserById             = MemCacheFindUserById;
    MemCacheTable.pfnFindGroupByName          = MemCacheFindGroupByName;
    MemCacheTable.pfnFindGroupById            = MemCacheFindGroupById;
    MemCacheTable.pfnRemoveUserBySid          = MemCacheRemoveUserBySid;
    MemCacheTable.pfnRemoveGroupBySid         = MemCacheRemoveGroupBySid;
    MemCacheTable.pfnEmptyCache               = MemCacheEmptyCache;
    MemCacheTable.pfnStoreObjectEntry         = MemCacheStoreObjectEntry;
    MemCacheTable.pfnStoreObjectEntries       = MemCacheStoreObjectEntries;
    MemCacheTable.pfnStoreGroupMembership     = MemCacheStoreGroupMembership;
    MemCacheTable.pfnStoreGroupsForUser       = MemCacheStoreGroupsForUser;
    MemCacheTable.pfnGetMemberships           = MemCacheGetMemberships;
    MemCacheTable.pfnGetGroupMembers          = MemCacheGetGroupMembers;
    MemCacheTable.pfnGetGroupsForUser         = MemCacheGetGroupsForUser;
    MemCacheTable.pfnEnumUsersCache           = MemCacheEnumUsersCache;
    MemCacheTable.pfnEnumGroupsCache          = MemCacheEnumGroupsCache;
    MemCacheTable.pfnFindObjectByDN           = MemCacheFindObjectByDN;
    MemCacheTable.pfnFindObjectsByDNList      = MemCacheFindObjectsByDNList;
    MemCacheTable.pfnFindObjectBySid          = MemCacheFindObjectBySid;
    MemCacheTable.pfnFindObjectBySidList      = MemCacheFindObjectsBySidList;
    MemCacheTable.pfnGetPasswordVerifier      = MemCacheGetPasswordVerifier;
    MemCacheTable.pfnStorePasswordVerifier    = MemCacheStorePasswordVerifier;
}

#endif
