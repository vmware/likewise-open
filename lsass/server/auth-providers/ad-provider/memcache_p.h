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

#define ENTER_READER_RW_LOCK(pLock, bInLock)\
        if (!bInLock) {                                    \
           pthread_rwlock_rdlock(pLock);            \
           bInLock = TRUE;                                 \
        }

#define LEAVE_RW_LOCK(pLock, bInLock) \
        if (bInLock) {                                     \
           pthread_rwlock_unlock(pLock);            \
           bInLock = FALSE;                                \
        }

#define ENTER_WRITER_RW_LOCK(pLock, bInLock) \
        if (!bInLock) {                                    \
           pthread_rwlock_wrlock(pLock);            \
           bInLock = TRUE;                                 \
        }

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

    PSTR pszFilename;

    //linked lists
    // pItem is of type PLSA_SECURITY_OBJECT
    PDLINKEDLIST pObjects;

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
    OUT PADCACHE_PROVIDER_FUNCTION_TABLE pCacheTable
    );

void
MemCacheFreeGuardian(
    const LSA_HASH_ENTRY* pEntry
    );

void
MemCacheFreePasswordVerifier(
    IN const LSA_HASH_ENTRY* pEntry
    );

DWORD
MemCacheOpen(
    IN PCSTR pszDbPath,
    OUT PLSA_DB_HANDLE phDb
    );

DWORD
MemCacheLoadFile(
    IN LSA_DB_HANDLE hDb
    );

DWORD
MemCacheStoreFile(
    IN LSA_DB_HANDLE hDb
    );

VOID
MemCacheFreeObjects(
    IN PVOID pData,
    IN PVOID pUnused
    );

DWORD
MemCacheRemoveMembership(
    IN PMEM_DB_CONNECTION pConn,
    IN PMEM_GROUP_MEMBERSHIP pMembership
    );

void
MemCacheSafeClose(
    IN OUT PLSA_DB_HANDLE phDb
    );

DWORD
MemCacheFindUserByName(
    IN LSA_DB_HANDLE hDb,
    IN PLSA_LOGIN_NAME_INFO pUserNameInfo,
    OUT PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
MemCacheFindUserById(
    IN LSA_DB_HANDLE hDb,
    IN uid_t uid,
    OUT PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
MemCacheFindGroupByName(
    IN LSA_DB_HANDLE hDb,
    IN PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    OUT PLSA_SECURITY_OBJECT* ppObject
    );

DWORD
MemCacheFindGroupById(
    IN LSA_DB_HANDLE hDb,
    IN gid_t gid,
    OUT PLSA_SECURITY_OBJECT* ppObject
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
    IN LSA_DB_HANDLE hDb,
    IN PLSA_SECURITY_OBJECT pObject
    );

DWORD
MemCacheRemoveObjectByHashKey(
    IN PMEM_DB_CONNECTION pConn,
    IN OUT PLSA_HASH_TABLE pTable,
    IN PVOID pvKey
    );

DWORD
MemCacheClearExistingObjectKeys(
    IN PMEM_DB_CONNECTION pConn,
    IN PLSA_SECURITY_OBJECT pObject
    );

DWORD
MemCacheEnsureHashSpace(
    IN OUT PLSA_HASH_TABLE pTable,
    IN size_t sNewEntries
    );

DWORD
MemCacheStoreObjectEntries(
    IN LSA_DB_HANDLE hDb,
    IN size_t  sObjectCount,
    IN PLSA_SECURITY_OBJECT* ppObjects
    );

DWORD
MemCacheStoreObjectEntryInLock(
    IN PMEM_DB_CONNECTION pConn,
    IN PLSA_SECURITY_OBJECT pObject
    );

void
MemCacheSafeFreeGroupMembership(
    IN OUT PMEM_GROUP_MEMBERSHIP* ppMembership
    );

void
MemCacheFreeMembershipValue(
    IN const LSA_HASH_ENTRY* pEntry
    );

DWORD
MemCacheDuplicateMembership(
    OUT PMEM_GROUP_MEMBERSHIP* ppDest,
    IN PLSA_GROUP_MEMBERSHIP pSrc
    );

DWORD
MemCacheAddMembership(
    IN PMEM_DB_CONNECTION pConn,
    IN PMEM_GROUP_MEMBERSHIP pMembership
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
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszDN,
    OUT PLSA_SECURITY_OBJECT *ppObject
    );

DWORD
MemCacheFindObjectsByDNList(
    IN LSA_DB_HANDLE hDb,
    IN size_t sCount,
    IN PSTR* ppszDnList,
    OUT PLSA_SECURITY_OBJECT** pppResults
    );

DWORD
MemCacheFindObjectBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    OUT PLSA_SECURITY_OBJECT *ppObject
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
    IN LSA_DB_HANDLE hDb,
    IN PLSA_PASSWORD_VERIFIER pVerifier
    );

void
InitializeMemCacheProvider(
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheTable
    );

#endif /* __MEMCACHE_P_H__ */
