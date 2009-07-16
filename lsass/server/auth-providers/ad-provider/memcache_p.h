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
#ifndef __MEMCACHE_P_H__
#define __MEMCACHE_P_H__

typedef struct _MEM_LIST_NODE MEM_LIST_NODE, *PMEM_LIST_NODE;

struct _MEM_LIST_NODE
{
    PMEM_LIST_NODE pNext;
    PMEM_LIST_NODE pPrev;
};

typedef struct _MEM_GROUP_MEMBERSHIP
{
    LSA_GROUP_MEMBERSHIP membership;
    MEM_LIST_NODE parentListNode;
    MEM_LIST_NODE childListNode;
} MEM_GROUP_MEMBERSHIP, *PMEM_GROUP_MEMBERSHIP;

#define PARENT_NODE_TO_MEMBERSHIP(x) (PMEM_GROUP_MEMBERSHIP) \
    ((char *)x - (char *)&((PMEM_GROUP_MEMBERSHIP)0)->parentListNode)

#define CHILD_NODE_TO_MEMBERSHIP(x) (PMEM_GROUP_MEMBERSHIP) \
    ((char *)x - (char *)&((PMEM_GROUP_MEMBERSHIP)0)->childListNode)

typedef struct _MEM_DB_CONNECTION
{
    BOOLEAN bLockCreated;
    pthread_rwlock_t lock;

    //linked lists
    // pItem is of type PLSA_SECURITY_OBJECT
    PDLINKEDLIST pObjects;
    // pItem is of type PLSA_PASSWORD_VERIFIER
    PDLINKEDLIST pPasswordVerifiers;

    //indexes
    PLSA_HASH_TABLE pDNToSecurityObject;
    PLSA_HASH_TABLE pNT4ToSecurityObject;
    PLSA_HASH_TABLE pSIDToSecurityObject;

    PLSA_HASH_TABLE pUIDToSecurityObject;
    PLSA_HASH_TABLE pUserAliasToSecurityObject;
    PLSA_HASH_TABLE pUPNToSecurityObject;

    PLSA_HASH_TABLE pSIDToPasswordVerifier;

    PLSA_HASH_TABLE pGIDToSecurityObject;
    PLSA_HASH_TABLE pGroupAliasToSecurityObject;

    // Points to a guardian MEM_LIST_NODE. The rest of the linked list points
    // to MEM_LIST_NODEs from the parentListNode field in MEM_GROUP_MEMBERSHIP
    // objects.
    PLSA_HASH_TABLE pParentSIDToMembershipList;
    // Points to a guardian MEM_LIST_NODE. The rest of the linked list points
    // to MEM_LIST_NODEs from the childListNode field in MEM_GROUP_MEMBERSHIP
    // objects.
    PLSA_HASH_TABLE pChildSIDToMembershipList;
} MEM_DB_CONNECTION, *PMEM_DB_CONNECTION;

void
InitializeMemCacheProvider(
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheTable
    );

void
MemCacheFreeGuardian(
    const LSA_HASH_ENTRY* pEntry
    );

DWORD
MemCacheOpen(
    IN PCSTR pszDbPath,
    OUT PLSA_DB_HANDLE phDb
    );

VOID
MemCacheFreeObjects(
    PVOID pData,
    PVOID pUnused
    );

VOID
MemCacheFreePasswordVerifiers(
    PVOID pData,
    PVOID pUnused
    );

void
MemCacheSafeClose(
    PLSA_DB_HANDLE phDb
    );

DWORD
MemCacheFindUserByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
MemCacheFindUserById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
MemCacheFindGroupByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
MemCacheFindGroupById(
    LSA_DB_HANDLE hDb,
    gid_t gid,
    PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
MemCacheRemoveUserBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    );

DWORD
MemCacheRemoveGroupBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    );

DWORD
MemCacheEmptyCache(
    IN LSA_DB_HANDLE hDb
    );

DWORD
MemCacheStoreObjectEntry(
    LSA_DB_HANDLE hDb,
    PLSA_SECURITY_OBJECT pObject
    );

DWORD
MemCacheStoreObjectEntries(
    LSA_DB_HANDLE hDb,
    size_t  sObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects
    );

void
MemCacheSafeFreeGroupMembership(
    PMEM_GROUP_MEMBERSHIP* ppMembership
    );

void
MemCacheFreeMembershipValue(
    const LSA_HASH_ENTRY* pEntry
    );

DWORD
MemCacheStoreGroupMembership(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszParentSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers
    );

DWORD
MemCacheStoreGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszChildSid,
    IN size_t sMemberCount,
    IN PLSA_GROUP_MEMBERSHIP* ppMembers,
    IN BOOLEAN bIsPacAuthoritative
    );

DWORD
MemCacheGetMemberships(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bIsGroupMembers,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    );

DWORD
MemCacheGetGroupMembers(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    );

DWORD
MemCacheGetGroupsForUser(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    IN BOOLEAN bFilterNotInPacNorLdap,
    OUT size_t* psCount,
    OUT PLSA_GROUP_MEMBERSHIP** pppResults
    );

DWORD
MemCacheEnumUsersCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumUsers,
    IN PCSTR                   pszResume,
    OUT DWORD*                 dwNumUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
MemCacheEnumGroupsCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumGroups,
    IN PCSTR                   pszResume,
    OUT DWORD*                 dwNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    );

DWORD
MemCacheFindObjectByDN(
    LSA_DB_HANDLE hDb,
    PCSTR pszDN,
    PLSA_SECURITY_OBJECT *ppObject);

DWORD
MemCacheFindObjectsByDNList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszDnList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
MemCacheFindObjectBySid(
    LSA_DB_HANDLE hDb,
    PCSTR pszSid,
    PLSA_SECURITY_OBJECT *ppObject
    );

DWORD
MemCacheFindObjectsBySidList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszSidList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
MemCacheGetPasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszUserSid,
    OUT PLSA_PASSWORD_VERIFIER *ppResult
    );

DWORD
MemCacheStorePasswordVerifier(
    LSA_DB_HANDLE hDb,
    PLSA_PASSWORD_VERIFIER pVerifier
    );

void
InitializeMemCacheProvider(
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheTable
    );

#endif /* __MEMCACHE_P_H__ */
