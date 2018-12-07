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
 *        This is the in-memory implementation of the VmDir Provider Local Cache
 *
 * Authors: Krishna Ganugapati (kstemen@likewisesoftware.com)
 *
 */

#include "includes.h"

static
DWORD
MemCacheCheckSizeInLock(
    IN PMEM_DB_CONNECTION pConn
    );

void
MemCacheFreeGuardian(
    IN const LW_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pKey)
    {
        LwFreeString(pEntry->pKey);
    }
    if (pEntry->pValue)
    {
        LwFreeMemory(pEntry->pValue);
    }
}

DWORD
MemCacheOpen(
    IN PCSTR pszDbPath,
    IN PLSA_VMDIR_PROVIDER_STATE pState,
    OUT PLSA_DB_HANDLE phDb
    )
{
    DWORD dwError = 0;
    PMEM_DB_CONNECTION pConn = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pConn),
                    (PVOID*)&pConn);
    BAIL_ON_LSA_ERROR(dwError);

    pConn->pProviderState = pState;

    dwError = LwMapErrnoToLwError(pthread_rwlock_init(&pConn->lock, NULL));
    BAIL_ON_LSA_ERROR(dwError);
    pConn->bLockCreated = TRUE;

    if (pszDbPath)
    {
        dwError = LwAllocateString(
                      pszDbPath,
                      &pConn->pszFilename);
        BAIL_ON_LSA_ERROR(dwError);
    }

    //indexes
    dwError = LwHashCreate(
                    100,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pDNToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    LwHashFreeStringKey,
                    NULL,
                    &pConn->pNT4ToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pSIDToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashPVoidCompare,
                    LwHashPVoidHash,
                    NULL,
                    NULL,
                    &pConn->pUIDToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pUserAliasToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pUPNToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashPVoidCompare,
                    LwHashPVoidHash,
                    NULL,
                    NULL,
                    &pConn->pGIDToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pGroupAliasToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    MemCacheFreeGuardian,
                    NULL,
                    &pConn->pParentSIDToMembershipList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashCreate(
                    100,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    MemCacheFreeGuardian,
                    NULL,
                    &pConn->pChildSIDToMembershipList);
    BAIL_ON_LSA_ERROR(dwError);

    pConn->bNeedShutdown = FALSE;
    dwError = LwMapErrnoToLwError(pthread_cond_init(
                    &pConn->signalShutdown,
                    NULL));
    BAIL_ON_LSA_ERROR(dwError);
    pConn->bSignalShutdownCreated = TRUE;

    *phDb = (LSA_DB_HANDLE)pConn;

cleanup:
    return dwError;

error:
    MemCacheSafeClose((PLSA_DB_HANDLE)&pConn);
    *phDb = NULL;

    goto cleanup;
}

VOID
MemCacheFreeObjects(
    IN PVOID pData,
    IN PVOID pUnused
    )
{
    PLSA_SECURITY_OBJECT pObject = (PLSA_SECURITY_OBJECT)pData;

    VMCacheSafeFreeObject(&pObject);
}

DWORD
MemCacheRemoveMembership(
    IN PMEM_DB_CONNECTION pConn,
    IN PMEM_GROUP_MEMBERSHIP pMembership
    )
{
    DWORD dwError = 0;
    BOOLEAN bLastItem = FALSE;

    pConn->sCacheSize -= pMembership->membership.version.dwObjectSize;

    // See if only this membership plus the guardian is in the list
    bLastItem = (pMembership->parentListNode.Next->Next ==
            &pMembership->parentListNode);
    LsaListRemove(&pMembership->parentListNode);

    if (bLastItem)
    {
        // Only the guardian is left, so remove the hash entry
        dwError = LwHashRemoveKey(
                        pConn->pParentSIDToMembershipList,
                        pMembership->membership.pszParentSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    // See if only this membership plus the guardian is in the list
    bLastItem = (pMembership->childListNode.Next->Next ==
            &pMembership->childListNode);
    LsaListRemove(&pMembership->childListNode);

    if (bLastItem)
    {
        // Only the guardian is left, so remove the hash entry
        dwError = LwHashRemoveKey(
                        pConn->pChildSIDToMembershipList,
                        pMembership->membership.pszChildSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    MemCacheSafeFreeGroupMembership(&pMembership);

cleanup:
    return dwError;

error:
    goto cleanup;
}

void
MemCacheSafeClose(
    IN OUT PLSA_DB_HANDLE phDb
    )
{
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)*phDb;
    DWORD dwError = 0;

    if (pConn)
    {
        dwError = MemCacheEmptyCache(*phDb);
        LSA_ASSERT(dwError == 0);

        LwHashSafeFree(&pConn->pDNToSecurityObject);
        LwHashSafeFree(&pConn->pNT4ToSecurityObject);
        LwHashSafeFree(&pConn->pSIDToSecurityObject);

        LwHashSafeFree(&pConn->pUIDToSecurityObject);
        LwHashSafeFree(&pConn->pUserAliasToSecurityObject);
        LwHashSafeFree(&pConn->pUPNToSecurityObject);

        LwHashSafeFree(&pConn->pGIDToSecurityObject);
        LwHashSafeFree(&pConn->pGroupAliasToSecurityObject);
        LW_SAFE_FREE_STRING(pConn->pszFilename);

        LwHashSafeFree(&pConn->pParentSIDToMembershipList);
        LwHashSafeFree(&pConn->pChildSIDToMembershipList);

        if (pConn->bLockCreated)
        {
            dwError = LwMapErrnoToLwError(pthread_rwlock_destroy(&pConn->lock));
            LSA_ASSERT(dwError == 0);
        }

        if (pConn->bSignalShutdownCreated)
        {
            dwError = LwMapErrnoToLwError(pthread_cond_destroy(&pConn->signalShutdown));
            LSA_ASSERT(dwError == 0);
        }

        LW_SAFE_FREE_MEMORY(*phDb);
    }
}

DWORD
MemCacheFindUserByName(
    IN LSA_DB_HANDLE hDb,
    IN PLSA_LOGIN_NAME_INFO pUserNameInfo,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    PSTR pszKey = NULL;
    PSTR pszDnsDomain = NULL;
    PSTR pszShortDomain = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;
    PVMDIR_BIND_INFO pBindInfo = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    switch (pUserNameInfo->nameType)
    {
        case NameType_UPN:
            dwError = VmDirGetBindInfo(&pBindInfo);
            if (dwError)
            {
                dwError = LW_ERROR_NO_SUCH_USER;
            }
            BAIL_ON_LSA_ERROR(dwError);

            dwError = LwAllocateString(
                          pBindInfo->pszDomainFqdn,
                          &pszDnsDomain);
            BAIL_ON_LSA_ERROR(dwError);

            pIndex = pConn->pUPNToSecurityObject;

            BAIL_ON_INVALID_STRING(pUserNameInfo->pszName);
            BAIL_ON_INVALID_STRING(pszDnsDomain);

            dwError = LwAllocateStringPrintf(
                            &pszKey,
                            "%s@%s",
                            pUserNameInfo->pszName,
                            pszDnsDomain);
            BAIL_ON_LSA_ERROR(dwError);
            break;
       case NameType_NT4:
            dwError = LW_ERROR_NO_SUCH_USER;
            BAIL_ON_LSA_ERROR(dwError);
            break;
       case NameType_Alias:
            pIndex = pConn->pUserAliasToSecurityObject;

            BAIL_ON_INVALID_STRING(pUserNameInfo->pszName);

            dwError = LwAllocateStringPrintf(
                            &pszKey,
                            "%s",
                            pUserNameInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
       default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwHashGetValue(
                    pIndex,
                    pszKey,
                    (PVOID*)&pListEntry);
    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = VMCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != LSA_OBJECT_TYPE_USER)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LW_SAFE_FREE_STRING(pszKey);
    LW_SAFE_FREE_STRING(pszDnsDomain);
    LW_SAFE_FREE_STRING(pszShortDomain);
    if (pBindInfo)
    {
        VmDirReleaseBindInfo(pBindInfo);
    }

    return dwError;

error:
    *ppObject = NULL;
    VMCacheSafeFreeObject(&pObject);
    goto cleanup;
}

// returns LW_ERROR_NOT_HANDLED if the user is not in the database
DWORD
MemCacheFindUserById(
    IN LSA_DB_HANDLE hDb,
    IN uid_t uid,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pUIDToSecurityObject;

    dwError = LwHashGetValue(
                    pIndex,
                    (PVOID)(size_t)uid,
                    (PVOID*)&pListEntry);
    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = VMCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != LSA_OBJECT_TYPE_USER)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    VMCacheSafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
MemCacheFindGroupByName(
    IN LSA_DB_HANDLE hDb,
    IN PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    PSTR pszKey = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    switch (pGroupNameInfo->nameType)
    {
       case NameType_NT4:
            pIndex = pConn->pNT4ToSecurityObject;

            BAIL_ON_INVALID_STRING(pGroupNameInfo->pszDomain);
            BAIL_ON_INVALID_STRING(pGroupNameInfo->pszName);

            dwError = LwAllocateStringPrintf(
                            &pszKey,
                            "%s\\%s",
                            pGroupNameInfo->pszDomain,
                            pGroupNameInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
       case NameType_Alias:
            pIndex = pConn->pGroupAliasToSecurityObject;

            BAIL_ON_INVALID_STRING(pGroupNameInfo->pszName);

            dwError = LwAllocateStringPrintf(
                            &pszKey,
                            "%s",
                            pGroupNameInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
       default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwHashGetValue(
                    pIndex,
                    pszKey,
                    (PVOID *)&pListEntry);
    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = VMCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != LSA_OBJECT_TYPE_GROUP)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LW_SAFE_FREE_STRING(pszKey);

    return dwError;

error:
    *ppObject = NULL;
    VMCacheSafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
MemCacheFindGroupById(
    IN LSA_DB_HANDLE hDb,
    IN gid_t gid,
    OUT PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pGIDToSecurityObject;

    dwError = LwHashGetValue(
                    pIndex,
                    (PVOID)(size_t)gid,
                    (PVOID *)&pListEntry);
    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = VMCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != LSA_OBJECT_TYPE_GROUP)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    VMCacheSafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
MemCacheRemoveUserBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;

    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    dwError = MemCacheRemoveObjectByHashKey(
                    pConn,
                    pConn->pSIDToSecurityObject,
                    pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    MemCacheRemoveMembershipsBySid(pConn, pszSid, FALSE, TRUE);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheRemoveGroupBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid
    )
{
    DWORD dwError = 0;
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;

    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    dwError = MemCacheRemoveObjectByHashKey(
                    pConn,
                    pConn->pSIDToSecurityObject,
                    pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    MemCacheRemoveMembershipsBySid(pConn, pszSid, TRUE, TRUE);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheEmptyCache(
    IN LSA_DB_HANDLE hDb
    )
{
    BOOLEAN bInLock = FALSE;
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    DWORD dwError = 0;
    LW_HASH_ITERATOR iterator = {0};
    // Do not free
    LW_HASH_ENTRY *pEntry = NULL;

    if (pConn->bLockCreated)
    {
        ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);
    }

    MemCacheCheckSizeInLock(pConn);

    if (pConn->pDNToSecurityObject)
    {
        LwHashRemoveAll(pConn->pDNToSecurityObject);
    }
    if (pConn->pNT4ToSecurityObject)
    {
        LwHashRemoveAll(pConn->pNT4ToSecurityObject);
    }
    if (pConn->pSIDToSecurityObject)
    {
        LwHashRemoveAll(pConn->pSIDToSecurityObject);
    }

    if (pConn->pUIDToSecurityObject)
    {
        LwHashRemoveAll(pConn->pUIDToSecurityObject);
    }
    if (pConn->pUserAliasToSecurityObject)
    {
        LwHashRemoveAll(pConn->pUserAliasToSecurityObject);
    }
    if (pConn->pUPNToSecurityObject)
    {
        LwHashRemoveAll(pConn->pUPNToSecurityObject);
    }

    if (pConn->pGIDToSecurityObject)
    {
        LwHashRemoveAll(pConn->pGIDToSecurityObject);
    }
    if (pConn->pGroupAliasToSecurityObject)
    {
        LwHashRemoveAll(pConn->pGroupAliasToSecurityObject);
    }

    if (pConn->pParentSIDToMembershipList)
    {
        // Remove all of the group memberships. Either table may be iterated,
        // so the parentsid list was chosen.
        dwError = LwHashGetIterator(
                      pConn->pParentSIDToMembershipList,
                      &iterator);
        BAIL_ON_LSA_ERROR(dwError);

        while ((pEntry = LwHashNext(&iterator)) != NULL)
        {
            PLSA_LIST_LINKS pGuardian = (PLSA_LIST_LINKS)pEntry->pValue;
            // Since the hash entry exists, the list must be non-empty
            BOOLEAN bListNonempty = TRUE;

            while (bListNonempty)
            {
                LSA_ASSERT(!LsaListIsEmpty(pGuardian));
                if (pGuardian->Next->Next == pGuardian)
                {
                    // At this point, there is a guardian node plus one other
                    // entry. MemCacheRemoveMembership will remove the last
                    // entry and the guardian node in the next call. Since the
                    // entry hash entry will have been deleted, the loop can
                    // then exit. The pGuardian pointer will be invalid, so
                    // this condition has to be checked before the last
                    // membership is removed.
                    bListNonempty = FALSE;
                }
                dwError = MemCacheRemoveMembership(
                              pConn,
                              PARENT_NODE_TO_MEMBERSHIP(pGuardian->Next));
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

    LsaDLinkedListForEach(
        pConn->pObjects,
        MemCacheFreeObjects,
        NULL);
    LsaDLinkedListFree(pConn->pObjects);
    pConn->pObjects = NULL;

    pConn->sCacheSize = 0;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    return dwError;

error:
    goto cleanup;
}

static
DWORD
MemCacheCheckSizeInLock(
    IN PMEM_DB_CONNECTION pConn
    )
{
    DWORD dwError = 0;
    size_t sCacheSize = 0;
    // Do not free
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;
    // DWORD dwOut = 0;
    LW_HASH_ITERATOR iterator = {0};
    // Do not free
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    // Do not free
    LW_HASH_ENTRY *pEntry = NULL;
    // Do not free
    PLSA_LIST_LINKS pGuardian = NULL;
    // Do not free
    PLSA_LIST_LINKS pMemPos = NULL;

    // Start at the beginning of the list
    pListEntry = pConn->pObjects;

    for (pListEntry = pConn->pObjects;
        pListEntry != NULL;
        pListEntry = pListEntry->pNext)
    {
        pObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;
        sCacheSize += pObject->version.dwObjectSize;
    }

    if (pConn->pParentSIDToMembershipList)
    {
        dwError = LwHashGetIterator(
                      pConn->pParentSIDToMembershipList,
                      &iterator);
        BAIL_ON_LSA_ERROR(dwError);
        while ((pEntry = LwHashNext(&iterator)) != NULL)
        {
            pGuardian = (PLSA_LIST_LINKS) pEntry->pValue;
            pMemPos = pGuardian->Next;
            while (pMemPos != pGuardian)
            {
                pMembership = PARENT_NODE_TO_MEMBERSHIP(pMemPos);
                sCacheSize += pMembership->membership.version.dwObjectSize;

                pMemPos = pMemPos->Next;
            }
        }
    }

    if (pConn->sCacheSize != sCacheSize)
    {
        LSA_LOG_ERROR("Recorded cache size not equal calculated size: %zu, %zu", pConn->sCacheSize, sCacheSize);
    }

error:

    return dwError;
}

DWORD
MemCacheRemoveObjectByHashKey(
    IN PMEM_DB_CONNECTION pConn,
    IN OUT PLW_HASH_TABLE pTable,
    IN const void* pvKey
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pListEntry = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSTR pszKey = NULL;

    dwError = LwHashGetValue(
                    pTable,
                    pvKey,
                    (PVOID*)&pListEntry);
    if (dwError == ERROR_NOT_FOUND)
    {
        // The key does not exist
        dwError = 0;
        goto cleanup;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;

    //Remove it from all indexes
    if (!LW_IS_NULL_OR_EMPTY_STR(pObject->pszDN))
    {
        dwError = LwHashRemoveKey(
                        pConn->pDNToSecurityObject,
                        pObject->pszDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                    &pszKey,
                    "%s\\%s",
                    pObject->pszNetbiosDomainName ?
                        pObject->pszNetbiosDomainName : "",
                    pObject->pszSamAccountName ?
                        pObject->pszSamAccountName : "");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashRemoveKey(
                    pConn->pSIDToSecurityObject,
                    pObject->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->enabled && pObject->type == LSA_OBJECT_TYPE_USER)
    {
        dwError = LwHashRemoveKey(
                        pConn->pUIDToSecurityObject,
                        (PVOID)(size_t)pObject->userInfo.uid);
        BAIL_ON_LSA_ERROR(dwError);

        if (pObject->userInfo.pszAliasName != NULL &&
                pObject->userInfo.pszAliasName[0])
        {
            dwError = LwHashRemoveKey(
                            pConn->pUserAliasToSecurityObject,
                            pObject->userInfo.pszAliasName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pObject->userInfo.pszUPN != NULL && pObject->userInfo.pszUPN[0])
        {
            dwError = LwHashRemoveKey(
                            pConn->pUPNToSecurityObject,
                            pObject->userInfo.pszUPN);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else if (pObject->enabled && pObject->type == LSA_OBJECT_TYPE_GROUP)
    {
        dwError = LwHashRemoveKey(
                        pConn->pGIDToSecurityObject,
                        (PVOID)(size_t)pObject->groupInfo.gid);
        BAIL_ON_LSA_ERROR(dwError);

        if (pObject->groupInfo.pszAliasName != NULL &&
                pObject->groupInfo.pszAliasName[0])
        {
            dwError = LwHashRemoveKey(
                            pConn->pGroupAliasToSecurityObject,
                            pObject->groupInfo.pszAliasName);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    //Remove it from the global linked list
    if (pListEntry->pPrev != NULL)
    {
        pListEntry->pPrev->pNext = pListEntry->pNext;
    }
    else
    {
        pConn->pObjects = pListEntry->pNext;
    }

    if (pListEntry->pNext != NULL)
    {
        pListEntry->pNext->pPrev = pListEntry->pPrev;
    }
    LW_SAFE_FREE_MEMORY(pListEntry);
    pConn->sCacheSize -= pObject->version.dwObjectSize;

    VMCacheSafeFreeObject(&pObject);

cleanup:
    LW_SAFE_FREE_STRING(pszKey);
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheClearExistingObjectKeys(
    IN PMEM_DB_CONNECTION pConn,
    IN PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PSTR pszKey = NULL;
    PDLINKEDLIST pListEntry = NULL;

    if (!LW_IS_NULL_OR_EMPTY_STR(pObject->pszDN))
    {
        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pDNToSecurityObject,
                        pObject->pszDN);
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LwAllocateStringPrintf(
                    &pszKey,
                    "%s\\%s",
                    pObject->pszNetbiosDomainName ?
                        pObject->pszNetbiosDomainName : "",
                    pObject->pszSamAccountName ?
                        pObject->pszSamAccountName : "");
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheRemoveObjectByHashKey(
                    pConn,
                    pConn->pNT4ToSecurityObject,
                    pszKey);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheRemoveObjectByHashKey(
                    pConn,
                    pConn->pSIDToSecurityObject,
                    pObject->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->enabled && pObject->type == LSA_OBJECT_TYPE_USER)
    {
        dwError = LwHashGetValue(
                        pConn->pUIDToSecurityObject,
                        (PVOID)(size_t)pObject->userInfo.uid,
                        (PVOID*)&pListEntry);
        if (dwError == ERROR_NOT_FOUND)
        {
            // The key does not exist
            dwError = 0;
        }
        else
        {
            char oldTimeBuf[128] = { 0 };
            char newTimeBuf[128] = { 0 };
            struct tm oldTmBuf = { 0 };
            struct tm newTmBuf = { 0 };
            PLSA_SECURITY_OBJECT pDuplicateObject = NULL;

            BAIL_ON_LSA_ERROR(dwError);

            pDuplicateObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;
            localtime_r(&pDuplicateObject->version.tLastUpdated, &oldTmBuf);
            localtime_r(&pObject->version.tLastUpdated, &newTmBuf);
            strftime(
                    oldTimeBuf,
                    sizeof(oldTimeBuf),
                    "%Y/%m/%d %H:%M:%S",
                    &oldTmBuf);
            strftime(
                    newTimeBuf,
                    sizeof(newTimeBuf),
                    "%Y/%m/%d %H:%M:%S",
                    &newTmBuf);

            LSA_LOG_ERROR("Conflict discovered for UID %d. User %s\\%s had this UID at time %s, but now (%s) user %s\\%s has the UID. Please check that these users are not currently conflicting in Active Directory. This could also happen (safely) if the UIDs were swapped between these users.",
                        (int)pObject->userInfo.uid,
                        LSA_SAFE_LOG_STRING(
                            pDuplicateObject->pszNetbiosDomainName),
                        LSA_SAFE_LOG_STRING(
                            pDuplicateObject->pszSamAccountName),
                        oldTimeBuf,
                        newTimeBuf,
                        LSA_SAFE_LOG_STRING(pObject->pszNetbiosDomainName),
                        LSA_SAFE_LOG_STRING(pObject->pszSamAccountName));
        }

        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pUIDToSecurityObject,
                        (PVOID)(size_t)pObject->userInfo.uid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pUserAliasToSecurityObject,
                        pObject->userInfo.pszAliasName);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pUPNToSecurityObject,
                        pObject->userInfo.pszUPN);
        BAIL_ON_LSA_ERROR(dwError);
    }
    else if (pObject->enabled && pObject->type == LSA_OBJECT_TYPE_GROUP)
    {
        dwError = LwHashGetValue(
                        pConn->pGIDToSecurityObject,
                        (PVOID)(size_t)pObject->groupInfo.gid,
                        (PVOID*)&pListEntry);
        if (dwError == ERROR_NOT_FOUND)
        {
            // The key does not exist
            dwError = 0;
        }
        else
        {
            char oldTimeBuf[128] = { 0 };
            char newTimeBuf[128] = { 0 };
            struct tm oldTmBuf = { 0 };
            struct tm newTmBuf = { 0 };
            PLSA_SECURITY_OBJECT pDuplicateObject = NULL;

            BAIL_ON_LSA_ERROR(dwError);

            pDuplicateObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;
            localtime_r(&pDuplicateObject->version.tLastUpdated, &oldTmBuf);
            localtime_r(&pObject->version.tLastUpdated, &newTmBuf);
            strftime(
                    oldTimeBuf,
                    sizeof(oldTimeBuf),
                    "%Y/%m/%d %H:%M:%S",
                    &oldTmBuf);
            strftime(
                    newTimeBuf,
                    sizeof(newTimeBuf),
                    "%Y/%m/%d %H:%M:%S",
                    &newTmBuf);

            LSA_LOG_ERROR("Conflict discovered for GID %d. Group %s\\%s had this GID at time %s, but now (%s) group %s\\%s has the GID. Please check that these groups are not currently conflicting in Active Directory. This could also happen (safely) if the GIDs were swapped between these groups.",
                        (int)pObject->groupInfo.gid,
                        LSA_SAFE_LOG_STRING(
                            pDuplicateObject->pszNetbiosDomainName),
                        LSA_SAFE_LOG_STRING(
                            pDuplicateObject->pszSamAccountName),
                        oldTimeBuf,
                        newTimeBuf,
                        LSA_SAFE_LOG_STRING(pObject->pszNetbiosDomainName),
                        LSA_SAFE_LOG_STRING(pObject->pszSamAccountName));
        }
        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pGIDToSecurityObject,
                        (PVOID)(size_t)pObject->groupInfo.gid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pGroupAliasToSecurityObject,
                        pObject->groupInfo.pszAliasName);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszKey);
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheEnsureHashSpace(
    IN OUT PLW_HASH_TABLE pTable,
    IN size_t sNewEntries
    )
{
    DWORD dwError = 0;

    if ((pTable->sCount + sNewEntries) * 2 > pTable->sTableSize)
    {
        dwError = LwHashResize(
            pTable,
            (pTable->sCount + sNewEntries + 10) * 3);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheStoreObjectEntries(
    IN LSA_DB_HANDLE hDb,
    IN size_t sObjectCount,
    IN PLSA_SECURITY_OBJECT* ppObjects
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // Do not free
    size_t sIndex = 0;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSTR pszKey = NULL;
    time_t now = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    // For simplicity, don't check whether keys exist for sure or whether the
    // objects are users or groups. Just make sure there is enough space in all
    // cases.
    dwError = MemCacheEnsureHashSpace(
                    pConn->pDNToSecurityObject,
                    sObjectCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pSIDToSecurityObject,
                    sObjectCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pUIDToSecurityObject,
                    sObjectCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pUserAliasToSecurityObject,
                    sObjectCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pUPNToSecurityObject,
                    sObjectCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pGIDToSecurityObject,
                    sObjectCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pGroupAliasToSecurityObject,
                    sObjectCount);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sObjectCount; sIndex++)
    {
        dwError = VMCacheDuplicateObject(
                        &pObject,
                        ppObjects[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        pObject->version.tLastUpdated = now;

        dwError = MemCacheStoreObjectEntryInLock(
                        pConn,
                        pObject);
        // It is now owned by the hash table
        pObject = NULL;
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MemCacheMaintainSizeCap(pConn);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszKey);
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    VMCacheSafeFreeObject(&pObject);
    goto cleanup;
}

size_t
MemCacheGetStringSpace(
    IN PCSTR pszStr
    )
{
    if (pszStr)
    {
        return strlen(pszStr) + HEAP_HEADER_SIZE;
    }
    else
    {
        return 0;
    }
}

PMEM_GROUP_MEMBERSHIP
MemCacheFindMembership(
    IN PMEM_DB_CONNECTION pConn,
    IN PCSTR pszParentSid,
    IN PCSTR pszChildSid
    )
{
    DWORD dwError = 0;
    // Do not free
    PLSA_LIST_LINKS pGuardian = NULL;
    // Do not free
    PLSA_LIST_LINKS pPos = NULL;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;

    dwError = LwHashGetValue(
                    pConn->pParentSIDToMembershipList,
                    pszParentSid,
                    (PVOID*)&pGuardian);
    if (dwError == ERROR_NOT_FOUND)
    {
        return NULL;
    }
    LSA_ASSERT(dwError == 0);

    LSA_ASSERT(pGuardian != NULL);
    pPos = pGuardian->Next;
    while (pPos != pGuardian)
    {
        pMembership = PARENT_NODE_TO_MEMBERSHIP(pPos);
        // The == comparison takes care of the case where both strings are null
        if (!strcmp(LwEmptyStrForNull(pMembership->membership.pszParentSid),
                    LwEmptyStrForNull(pszParentSid)) &&
            !strcmp(LwEmptyStrForNull(pMembership->membership.pszChildSid),
                    LwEmptyStrForNull(pszChildSid)))
        {
            return pMembership;
        }

        pPos = pPos->Next;
    }

    return NULL;
}

VOID
MemCacheResetWeight(
    IN PVOID pData,
    IN PVOID pNow
    )
{
    PLSA_SECURITY_OBJECT pObject = (PLSA_SECURITY_OBJECT)pData;
    time_t age = *(time_t*)pNow - pObject->version.tLastUpdated;

    if (age < 0)
    {
        age = 0;
    }

    pObject->version.fWeight = 1.0 / (age + LOGGED_IN_VS_NSEC);
}

VOID
MemCacheMergeLists(
    IN OUT PDLINKEDLIST pList1,
    IN OUT PDLINKEDLIST pList2,
    IN OUT PDLINKEDLIST pList2End
    )
{
    PLSA_SECURITY_OBJECT pObject1 = NULL;
    PLSA_SECURITY_OBJECT pObject2 = NULL;
    PDLINKEDLIST pInsertAfter = pList1->pPrev;
    PDLINKEDLIST pList1End = pList2;

    while (pList1 != pList1End && pList2 != pList2End)
    {
        pObject1 = (PLSA_SECURITY_OBJECT)pList1->pItem;
        pObject2 = (PLSA_SECURITY_OBJECT)pList2->pItem;

        if (pObject1->version.fWeight < pObject2->version.fWeight)
        {
            pInsertAfter->pNext = pList1;
            pList1->pPrev = pInsertAfter;

            pInsertAfter = pList1;
            pList1 = pList1->pNext;
        }
        else
        {
            pInsertAfter->pNext = pList2;
            pList2->pPrev = pInsertAfter;

            pInsertAfter = pList2;
            pList2 = pList2->pNext;
        }
    }

    if (pList1 != pList1End)
    {
        pInsertAfter->pNext = pList1;
        pList1->pPrev = pInsertAfter;

        while (pList1->pNext != pList1End)
        {
            pList1 = pList1->pNext;
        }
        pList1->pNext = pList2End;
        if (pList2End != NULL)
        {
            pList2End->pPrev = pList1;
        }
    }
    else
    {
        pInsertAfter->pNext = pList2;
        pList2->pPrev = pInsertAfter;
    }
}

// Returns the first item in the object list that is out of order, or NULL if
// the entire list is ordered.
PDLINKEDLIST
MemCacheFindOutOfOrderNode(
    IN PDLINKEDLIST pList
    )
{
    PLSA_SECURITY_OBJECT pCurObject = NULL;
    PLSA_SECURITY_OBJECT pNextObject = NULL;

    while (pList->pNext != NULL)
    {
        pCurObject = (PLSA_SECURITY_OBJECT)pList->pItem;
        pNextObject = (PLSA_SECURITY_OBJECT)pList->pNext->pItem;

        if (pCurObject->version.fWeight > pNextObject->version.fWeight)
        {
            // The next object is out of order
            break;
        }

        pList = pList->pNext;
    }
    return pList->pNext;
}

VOID
MemCacheSortObjectList(
    IN OUT PDLINKEDLIST* ppObjects
    )
{
    DLINKEDLIST guardian = { 0 };
    PDLINKEDLIST pObjects = *ppObjects;
    PDLINKEDLIST pList1 = NULL;
    PDLINKEDLIST pList2 = NULL;
    PDLINKEDLIST pList2End = NULL;

    if (pObjects)
    {
        // Add a fake first node to simplify the calculations. This node will be
        // removed after the list is sorted.
        pObjects->pPrev = &guardian;
        guardian.pNext = pObjects;

        do
        {
            pList2End = guardian.pNext;
            while (pList2End != NULL)
            {
                // Find two consecutive lists which are ordered
                pList1 = pList2End;
                pList2 = MemCacheFindOutOfOrderNode(pList1);
                if (pList2 != NULL)
                {
                    pList2End = MemCacheFindOutOfOrderNode(pList2);
                    // Merge them together to make one ordered list
                    MemCacheMergeLists(pList1, pList2, pList2End);
                }
                else
                {
                    break;
                }
            }
        // while the entire list is not ordered
        } while (pList1 != guardian.pNext);

        // Double check the sort worked
        pList1 = guardian.pNext;
        while (pList1)
        {
            if (pList1->pNext)
            {
                PLSA_SECURITY_OBJECT pCurObject = NULL;
                PLSA_SECURITY_OBJECT pNextObject = NULL;

                pCurObject = (PLSA_SECURITY_OBJECT)pList1->pItem;
                pNextObject = (PLSA_SECURITY_OBJECT)pList1->pNext->pItem;

                LSA_ASSERT(pCurObject->version.fWeight <=
                        pNextObject->version.fWeight);
            }
            pList1 = pList1->pNext;
        }

        *ppObjects = guardian.pNext;
        guardian.pNext->pPrev = NULL;
    }
}

DWORD
MemCacheRemoveOrphanedMemberships(
    IN PMEM_DB_CONNECTION pConn
    )
{
    DWORD dwError = 0;
    LW_HASH_ITERATOR iterator = {0};
    LW_HASH_ENTRY *pEntry = NULL;
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    BOOLEAN bOrphaned = FALSE;
    PDLINKEDLIST pListEntry = NULL;
    PMEM_GROUP_MEMBERSHIP pCompleteness = NULL;

    // Only one table needs to be enumerated to see all memberships
    dwError = LwHashGetIterator(
                    pConn->pParentSIDToMembershipList,
                    &iterator);
    BAIL_ON_LSA_ERROR(dwError);

    while ((pEntry = LwHashNext(&iterator)) != NULL)
    {
        PLSA_LIST_LINKS pGuardian = (PLSA_LIST_LINKS)pEntry->pValue;
        PLSA_LIST_LINKS pPos = pGuardian->Next;

        while (pPos != pGuardian)
        {
            pMembership = PARENT_NODE_TO_MEMBERSHIP(pPos);
            bOrphaned = FALSE;

            if (pMembership->membership.pszParentSid != NULL)
            {
                dwError = LwHashGetValue(
                                pConn->pSIDToSecurityObject,
                                pMembership->membership.pszParentSid,
                                (PVOID*)&pListEntry);
                if (dwError == ERROR_NOT_FOUND)
                {
                    bOrphaned = TRUE;
                    dwError = 0;
                }
                BAIL_ON_LSA_ERROR(dwError);
            }

            if (pMembership->membership.pszChildSid != NULL)
            {
                dwError = LwHashGetValue(
                                pConn->pSIDToSecurityObject,
                                pMembership->membership.pszChildSid,
                                (PVOID*)&pListEntry);
                if (dwError == ERROR_NOT_FOUND)
                {
                    bOrphaned = TRUE;
                    dwError = 0;
                }
                BAIL_ON_LSA_ERROR(dwError);
            }

            pPos = pPos->Next;

            if (bOrphaned)
            {
                LSA_LOG_INFO("Removing orphaned membership between %s and %s",
                        LSA_SAFE_LOG_STRING(pMembership->
                            membership.pszParentSid),
                        LSA_SAFE_LOG_STRING(pMembership->
                            membership.pszChildSid));

                // The parent group's member list will no longer be complete.
                // Remove the completeness node.
                pCompleteness = MemCacheFindMembership(
                                        pConn,
                                        pMembership->membership.pszParentSid,
                                        NULL);
                if (pCompleteness && pCompleteness != pMembership)
                {
                    if (pPos == &pCompleteness->parentListNode)
                    {
                        pPos = pPos->Next;
                    }
                    dwError = MemCacheRemoveMembership(
                                    pConn,
                                    pCompleteness);
                    BAIL_ON_LSA_ERROR(dwError);
                }

                // The list of groups the object is a member of will no longer
                // be complete.  Remove the completeness node.
                pCompleteness = MemCacheFindMembership(
                                        pConn,
                                        NULL,
                                        pMembership->membership.pszChildSid);
                if (pCompleteness && pCompleteness != pMembership)
                {
                    if (pPos == &pCompleteness->parentListNode)
                    {
                        pPos = pPos->Next;
                    }
                    dwError = MemCacheRemoveMembership(
                                    pConn,
                                    pCompleteness);
                    BAIL_ON_LSA_ERROR(dwError);

                    // The hash iterator always points to the next entry to
                    // return. It is possible that entry was just deleted.
                    // Resetting this pointer to NULL may cause the iterator to
                    // repeat items, but it guarentees that the iterator will
                    // not access a deleted entry.
                    //
                    // This was not an issue for deleting the parent
                    // completeness node, since it was guarenteed to be in the
                    // same linked list (and thus the same hash entry).
                    iterator.pEntryPos = NULL;
                }

                // It is safe to remove this membership since pPos points to
                // the entry after it.
                dwError = MemCacheRemoveMembership(
                                pConn,
                                pMembership);
                BAIL_ON_LSA_ERROR(dwError);
            }
        }
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

VOID
MemCacheAddPinnedObject(
    IN OUT PLSA_SECURITY_OBJECT pPinnedObjects[PINNED_USER_COUNT],
    IN PLSA_SECURITY_OBJECT pObject
    )
{
    ssize_t ssIndex = 0;

    for (ssIndex = PINNED_USER_COUNT - 1;
        ssIndex >= 0;
        ssIndex--)
    {
        if (!pPinnedObjects[ssIndex] ||
                pObject->version.tLastUpdated >
                pPinnedObjects[ssIndex]->version.tLastUpdated)
        {
            memmove(pPinnedObjects, pPinnedObjects + 1, ssIndex);
            pPinnedObjects[ssIndex] = pObject;
            return;
        }
    }
}

static
DWORD
MemCacheSetPinnedObjectWeights(
    IN PMEM_DB_CONNECTION pConn,
    IN OUT PLSA_SECURITY_OBJECT pPinnedObjects[PINNED_USER_COUNT]
    )
{
    DWORD dwError = 0;
    size_t ssIndex = 0;
    time_t age = 0;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLSA_LIST_LINKS pGuardian = NULL;
    // Do not free
    PLSA_LIST_LINKS pPos = NULL;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    for (ssIndex = 0; ssIndex < PINNED_USER_COUNT; ssIndex++)
    {
        pObject = pPinnedObjects[ssIndex];
        if (pObject)
        {
            LSA_LOG_VERBOSE("User object %s\\%s (sid %s) is pinned (cannot be evicted before unpinned objects)",
                    pObject->pszNetbiosDomainName,
                    pObject->pszSamAccountName,
                    pObject->pszObjectSid);

            pObject->version.fWeight = 1.0 / (age + LOGGED_IN_VS_NSEC) *
                ((LOGGED_IN_VS_ZERO_SEC + LOGGED_IN_VS_NSEC)/
                 LOGGED_IN_VS_NSEC);

            // Increase the weight of all the groups the user is a member of
            dwError = LwHashGetValue(
                            pConn->pChildSIDToMembershipList,
                            pObject->pszObjectSid,
                            (PVOID*)&pGuardian);
            if (dwError == ERROR_NOT_FOUND)
            {
                dwError = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);

            if (pGuardian)
            {
                pPos = pGuardian->Next;
            }
            else
            {
                pPos = pGuardian;
            }
            while (pPos != pGuardian)
            {
                pMembership = CHILD_NODE_TO_MEMBERSHIP(pPos);

                dwError = LwHashGetValue(
                                pConn->pSIDToSecurityObject,
                                pMembership->membership.pszParentSid,
                                (PVOID*)&pListEntry);
                if (dwError == ERROR_NOT_FOUND)
                {
                    dwError = 0;
                }
                BAIL_ON_LSA_ERROR(dwError);

                if (pListEntry)
                {
                    pObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;

                    pObject->version.fWeight = 1.0 / (age + LOGGED_IN_VS_NSEC) *
                        ((LOGGED_IN_VS_ZERO_SEC + LOGGED_IN_VS_NSEC)/
                             LOGGED_IN_VS_NSEC);
                }

                pPos = pPos->Next;
            }
        }
    }

cleanup:
    return dwError;
error:
    goto cleanup;
}

DWORD
MemCacheMaintainSizeCap(
    IN PMEM_DB_CONNECTION pConn
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;
    // Do not free
    LW_HASH_ENTRY *pEntry = NULL;
    LW_HASH_ITERATOR iterator = {0};
    time_t now = 0;
    PLSA_SECURITY_OBJECT pObject = NULL;
    time_t age = 0;
    // Do not free
    PLSA_LIST_LINKS pGuardian = NULL;
    // Do not free
    PLSA_LIST_LINKS pPos = NULL;
    // Do not free
    PSTR pszSid = NULL;
    PLSA_SECURITY_OBJECT pPinnedObjects[PINNED_USER_COUNT] = { 0 };

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    // The connection already has a write lock

    if (!pConn->sSizeCap)
    {
        goto cleanup;
    }

    if (pConn->sCacheSize <= pConn->sSizeCap)
    {
        goto cleanup;
    }

    LSA_LOG_WARNING("The current cache size (%zu) is larger than the cap (%zu) - evicting old objects", pConn->sCacheSize, pConn->sSizeCap);

    // Remove any orphaned memberships (memberships where the parent or child
    // security object is not cached)

    dwError = MemCacheRemoveOrphanedMemberships(pConn);
    BAIL_ON_LSA_ERROR(dwError);

    LsaDLinkedListForEach(
        pConn->pObjects,
        MemCacheResetWeight,
        &now);

    while ((pEntry = LwHashNext(&iterator)) != NULL)
    {
        if (pListEntry != NULL)
        {
            pObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;

            if (!pPinnedObjects[0] || pObject->version.tLastUpdated > pPinnedObjects[0]->version.tLastUpdated)
            {
                MemCacheAddPinnedObject(pPinnedObjects, pObject);
            }

            age = now - pObject->version.tLastUpdated;
            if (age < 0)
            {
                age = 0;
            }
            pObject->version.fWeight = 1.0 / (age + LOGGED_IN_VS_NSEC) *
                ((LOGGED_IN_VS_ZERO_SEC + LOGGED_IN_VS_NSEC)/ LOGGED_IN_VS_NSEC);

            if (pGuardian)
            {
                pPos = pGuardian->Next;
            }
            else
            {
                pPos = pGuardian;
            }
            while (pPos != pGuardian)
            {
                pMembership = CHILD_NODE_TO_MEMBERSHIP(pPos);

                dwError = LwHashGetValue(
                                pConn->pSIDToSecurityObject,
                                pMembership->membership.pszParentSid,
                                (PVOID*)&pListEntry);
                if (dwError == ERROR_NOT_FOUND)
                {
                    dwError = 0;
                }
                BAIL_ON_LSA_ERROR(dwError);

                if (pListEntry)
                {
                    pObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;

                    age = now - pObject->version.tLastUpdated;
                    if (age < 0)
                    {
                        age = 0;
                    }
                    pObject->version.fWeight = 1.0 / (age + LOGGED_IN_VS_NSEC) *
                        ((LOGGED_IN_VS_ZERO_SEC + LOGGED_IN_VS_NSEC)/
                             LOGGED_IN_VS_NSEC);
                }

                pPos = pPos->Next;
            }
        }
    }

    dwError = MemCacheSetPinnedObjectWeights(
                    pConn,
                    pPinnedObjects);
    BAIL_ON_LSA_ERROR(dwError);

    MemCacheSortObjectList(&pConn->pObjects);

    while (pConn->sCacheSize > pConn->sSizeCap * 3/4)
    {
        pObject = (PLSA_SECURITY_OBJECT)pConn->pObjects->pItem;
        pszSid = pObject->pszObjectSid;

        if (pObject->type == LSA_OBJECT_TYPE_USER)
        {
            LSA_LOG_VERBOSE("Evicting user %s\\%s (sid %s)",
                    pObject->pszNetbiosDomainName,
                    pObject->pszSamAccountName,
                    pszSid);
        }
        else if (pObject->type == LSA_OBJECT_TYPE_GROUP)
        {
            LSA_LOG_VERBOSE("Evicting group %s\\%s (sid %s)",
                    pObject->pszNetbiosDomainName,
                    pObject->pszSamAccountName,
                    pszSid);
        }
        else
        {
            LSA_LOG_VERBOSE("Evicting object with sid %s", pszSid);
        }

        // Remove all membership information for what this group contains (and
        // remove the completeness entry in the children's member-of list)
        MemCacheRemoveMembershipsBySid(
            pConn,
            pszSid,
            TRUE,
            TRUE);

        // Remove all membership information for what groups this user is a
        // member of (and remove the completeness entry in the parent group's
        // member-of list)
        MemCacheRemoveMembershipsBySid(
            pConn,
            pszSid,
            FALSE,
            TRUE);

        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pSIDToSecurityObject,
                        pszSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_LOG_VERBOSE("The cache size reduced to (%zu)", pConn->sCacheSize);

    // The caller will notify the backup thread

cleanup:
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheStoreObjectEntryInLock(
    IN PMEM_DB_CONNECTION pConn,
    IN PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PSTR pszKey = NULL;
    // Keep track of how much space is used to store the object (including hash
    // space)
    size_t sObjectSize = sizeof(*pObject) + HEAP_HEADER_SIZE;

    dwError = MemCacheClearExistingObjectKeys(
                    pConn,
                    pObject);
    BAIL_ON_LSA_ERROR(dwError);

    // Afterwards pConn->pObjects points to the new node with pObject
    // inside. This node pointer will stored in the hash tables.
    dwError = LsaDLinkedListPrepend(
                    &pConn->pObjects,
                    pObject);
    BAIL_ON_LSA_ERROR(dwError);

    sObjectSize += sizeof(*pConn->pObjects) + HEAP_HEADER_SIZE;

    if (pObject->pszDN != NULL)
    {
        sObjectSize += MemCacheGetStringSpace(pObject->pszDN);

        sObjectSize += HASH_ENTRY_SPACE;
        dwError = LwHashSetValue(
                        pConn->pDNToSecurityObject,
                        pObject->pszDN,
                        pConn->pObjects);
        BAIL_ON_LSA_ERROR(dwError);
    }

    LSA_ASSERT(pObject->pszObjectSid);
    sObjectSize += MemCacheGetStringSpace(pObject->pszObjectSid);
    sObjectSize += HASH_ENTRY_SPACE;
    dwError = LwHashSetValue(
                    pConn->pSIDToSecurityObject,
                    pObject->pszObjectSid,
                    pConn->pObjects);
    BAIL_ON_LSA_ERROR(dwError);

    sObjectSize += MemCacheGetStringSpace(pObject->pszSamAccountName);

    switch(pObject->type)
    {
        case LSA_OBJECT_TYPE_GROUP:
            sObjectSize += HASH_ENTRY_SPACE;
            dwError = LwHashSetValue(
                            pConn->pGIDToSecurityObject,
                            (PVOID)(size_t)pObject->groupInfo.gid,
                            pConn->pObjects);
            BAIL_ON_LSA_ERROR(dwError);

            if (pObject->groupInfo.pszAliasName &&
                    pObject->groupInfo.pszAliasName[0])
            {
                sObjectSize += HASH_ENTRY_SPACE;
                dwError = LwHashSetValue(
                                pConn->pGroupAliasToSecurityObject,
                                pObject->groupInfo.pszAliasName,
                                pConn->pObjects);
                BAIL_ON_LSA_ERROR(dwError);

                sObjectSize += MemCacheGetStringSpace(pObject->groupInfo.pszAliasName);
            }

            sObjectSize += MemCacheGetStringSpace(pObject->groupInfo.pszPasswd);
            break;
        case LSA_OBJECT_TYPE_USER:
            sObjectSize += HASH_ENTRY_SPACE;
            dwError = LwHashSetValue(
                            pConn->pUIDToSecurityObject,
                            (PVOID)(size_t)pObject->userInfo.uid,
                            pConn->pObjects);
            BAIL_ON_LSA_ERROR(dwError);

            if (pObject->userInfo.pszUPN)
            {
                sObjectSize += MemCacheGetStringSpace(pObject->userInfo.pszUPN);
                sObjectSize += HASH_ENTRY_SPACE;
                dwError = LwHashSetValue(
                                pConn->pUPNToSecurityObject,
                                pObject->userInfo.pszUPN,
                                pConn->pObjects);
                BAIL_ON_LSA_ERROR(dwError);
            }

            if (pObject->userInfo.pszAliasName &&
                    pObject->userInfo.pszAliasName[0])
            {
                sObjectSize += MemCacheGetStringSpace(pObject->userInfo.pszAliasName);
                sObjectSize += HASH_ENTRY_SPACE;
                dwError = LwHashSetValue(
                                pConn->pUserAliasToSecurityObject,
                                pObject->userInfo.pszAliasName,
                                pConn->pObjects);
                BAIL_ON_LSA_ERROR(dwError);
            }

            sObjectSize += MemCacheGetStringSpace(pObject->userInfo.pszPasswd);
            sObjectSize += MemCacheGetStringSpace(pObject->userInfo.pszGecos);
            sObjectSize += MemCacheGetStringSpace(pObject->userInfo.pszShell);
            sObjectSize += MemCacheGetStringSpace(pObject->userInfo.pszHomedir);
            break;
    }

    pObject = (PLSA_SECURITY_OBJECT)pConn->pObjects->pItem;
    pObject->version.dwObjectSize = sObjectSize;

    pConn->sCacheSize += sObjectSize;

cleanup:
    LW_SAFE_FREE_STRING(pszKey);

    return dwError;

error:
    goto cleanup;
}

void
MemCacheSafeFreeGroupMembership(
    IN OUT PMEM_GROUP_MEMBERSHIP* ppMembership
    )
{
    if (*ppMembership != NULL)
    {
        VMCacheFreeGroupMembershipContents(&(*ppMembership)->membership);
        LW_SAFE_FREE_MEMORY(*ppMembership);
    }
}

void
MemCacheFreeMembershipValue(
    IN const LW_HASH_ENTRY* pEntry
    )
{
    PMEM_GROUP_MEMBERSHIP pMembership = (PMEM_GROUP_MEMBERSHIP)pEntry->pValue;

    MemCacheSafeFreeGroupMembership(&pMembership);
}

DWORD
MemCacheDuplicateMembership(
    OUT PMEM_GROUP_MEMBERSHIP* ppDest,
    IN PLSA_GROUP_MEMBERSHIP pSrc
    )
{
    DWORD dwError = 0;
    PMEM_GROUP_MEMBERSHIP pDest = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*pDest),
                    (PVOID*)&pDest);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = VMCacheDuplicateMembershipContents(
                    &pDest->membership,
                    pSrc);
    BAIL_ON_LSA_ERROR(dwError);

    *ppDest = pDest;

cleanup:
    return dwError;

error:
    MemCacheSafeFreeGroupMembership(&pDest);
    *ppDest = NULL;
    goto cleanup;
}

DWORD
MemCacheAddMembership(
    IN PMEM_DB_CONNECTION pConn,
    IN PMEM_GROUP_MEMBERSHIP pMembership
    )
{
    // Do not free
    PLSA_LIST_LINKS pGuardian = NULL;
    DWORD dwError = 0;
    PLSA_LIST_LINKS pGuardianTemp = NULL;
    PSTR pszSidCopy = NULL;
    size_t sObjectSize = sizeof(*pMembership) + HEAP_HEADER_SIZE;

    sObjectSize += MemCacheGetStringSpace(pMembership->membership.pszParentSid);
    sObjectSize += MemCacheGetStringSpace(pMembership->membership.pszChildSid);

    // For simplicity assume we always add a guardian node with regards to
    // space calculations
    sObjectSize += HASH_ENTRY_SPACE;
    sObjectSize += MemCacheGetStringSpace(pMembership->membership.pszParentSid);
    sObjectSize += HASH_ENTRY_SPACE;
    sObjectSize += MemCacheGetStringSpace(pMembership->membership.pszChildSid);

    pMembership->membership.version.dwObjectSize = sObjectSize;

    dwError = LwHashGetValue(
                    pConn->pParentSIDToMembershipList,
                    pMembership->membership.pszParentSid,
                    (PVOID*)&pGuardian);
    if (dwError == ERROR_NOT_FOUND)
    {
        // Gotta add the guardian node
        dwError = 0;

        dwError = LwAllocateMemory(
                        sizeof(*pGuardianTemp),
                        (PVOID*)&pGuardianTemp);
        BAIL_ON_LSA_ERROR(dwError);

        LsaListInit(pGuardianTemp);

        dwError = LwStrDupOrNull(
                        pMembership->membership.pszParentSid,
                        &pszSidCopy);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwHashSetValue(
                        pConn->pParentSIDToMembershipList,
                        pszSidCopy,
                        pGuardianTemp);
        BAIL_ON_LSA_ERROR(dwError);

        // The guardian is now owned by the hash table, so don't free it if
        // something goes wrong after this point.
        pGuardian = pGuardianTemp;
        pGuardianTemp = NULL;
        pszSidCopy = NULL;
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaListInsertAfter(
        pGuardian,
        &pMembership->parentListNode);

    dwError = LwHashGetValue(
                    pConn->pChildSIDToMembershipList,
                    pMembership->membership.pszChildSid,
                    (PVOID*)&pGuardian);
    if (dwError == ERROR_NOT_FOUND)
    {
        // Gotta add the guardian node
        dwError = 0;

        dwError = LwAllocateMemory(
                        sizeof(*pGuardianTemp),
                        (PVOID*)&pGuardianTemp);
        BAIL_ON_LSA_ERROR(dwError);

        LsaListInit(pGuardianTemp);

        dwError = LwStrDupOrNull(
                        pMembership->membership.pszChildSid,
                        &pszSidCopy);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LwHashSetValue(
                        pConn->pChildSIDToMembershipList,
                        pszSidCopy,
                        pGuardianTemp);
        BAIL_ON_LSA_ERROR(dwError);

        // The guardian is now owned by the hash table, so don't free it if
        // something goes wrong after this point.
        pGuardian = pGuardianTemp;
        pGuardianTemp = NULL;
        pszSidCopy = NULL;
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    LsaListInsertAfter(
            pGuardian,
            &pMembership->childListNode);

    pConn->sCacheSize += sObjectSize;

cleanup:
    LW_SAFE_FREE_MEMORY(pGuardianTemp);
    LW_SAFE_FREE_STRING(pszSidCopy);
    return dwError;

error:
    goto cleanup;
}

VOID
MemCacheRemoveMembershipsBySid(
    IN PMEM_DB_CONNECTION pConn,
    IN PCSTR pszSid,
    IN BOOLEAN bIsParentSid,
    IN BOOLEAN bRemoveCompleteness
    )
{
    // Do not free
    PLSA_LIST_LINKS pGuardian = NULL;
    BOOLEAN bListNonempty = FALSE;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pCompleteness = NULL;
    DWORD dwError = 0;

    if (bIsParentSid)
    {
        pIndex = pConn->pParentSIDToMembershipList;
    }
    else
    {
        pIndex = pConn->pChildSIDToMembershipList;
    }

    // Copy the existing pac and primary domain memberships to the temporary
    // hash table
    dwError = LwHashGetValue(
                    pIndex,
                    pszSid,
                    (PVOID*)&pGuardian);
    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = 0;
    }
    LSA_ASSERT(dwError == 0);

    // Remove all of the existing memberships in the child hash and parent hash
    if (pGuardian)
    {
        // Since the hash entry exists, the list must be non-empty
        bListNonempty = TRUE;
    }

    while (bListNonempty)
    {
        LSA_ASSERT(!LsaListIsEmpty(pGuardian));
        if (pGuardian->Next->Next == pGuardian)
        {
            // At this point, there is a guardian node plus one other
            // entry. MemCacheRemoveMembership will remove the last
            // entry and the guardian node in the next call. Since the
            // entry hash entry will have been deleted, the loop can
            // then exit. The pGuardian pointer will be invalid, so
            // this condition has to be checked before the last
            // membership is removed.
            bListNonempty = FALSE;
        }
        if (bIsParentSid)
        {
            pMembership = PARENT_NODE_TO_MEMBERSHIP(pGuardian->Next);
        }
        else
        {
            pMembership = CHILD_NODE_TO_MEMBERSHIP(pGuardian->Next);
        }
        if (bRemoveCompleteness)
        {
            if (bIsParentSid)
            {
                pCompleteness = MemCacheFindMembership(
                                    pConn,
                                    NULL,
                                    pMembership->membership.pszChildSid);
            }
            else
            {
                pCompleteness = MemCacheFindMembership(
                                    pConn,
                                    pMembership->membership.pszParentSid,
                                    NULL);
            }
            if (pCompleteness && pCompleteness != pMembership)
            {
                dwError = MemCacheRemoveMembership(
                                pConn,
                                pCompleteness);
                LSA_ASSERT(dwError == 0);
            }

        }
        dwError = MemCacheRemoveMembership(
                        pConn,
                        pMembership);
        LSA_ASSERT(dwError == 0);
    }
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
    // Indexed by child sid (since parent sid is identical for all entries)
    PLW_HASH_TABLE pCombined = NULL;
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pExistingMembership = NULL;
    // Do not free
    PLSA_LIST_LINKS pGuardian = NULL;
    // Do not free
    PLSA_LIST_LINKS pPos = NULL;
    LW_HASH_ITERATOR iterator = {0};
    size_t sIndex = 0;
    BOOLEAN bInLock = FALSE;
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    // Do not free
    LW_HASH_ENTRY *pEntry = NULL;
    time_t now = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    // Make a hash table to hold the new membership list along with any
    // existing memberships that need to be merged in.
    dwError = LwHashCreate(
                    sMemberCount * 2,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    MemCacheFreeMembershipValue,
                    NULL,
                    &pCombined);
    BAIL_ON_LSA_ERROR(dwError);

    // Copy the new items to the temporary hash table
    for (sIndex = 0; sIndex < sMemberCount; sIndex++)
    {
        dwError = MemCacheDuplicateMembership(
                        &pMembership,
                        ppMembers[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        pMembership->membership.version.tLastUpdated = now;

        dwError = LwHashSetValue(
                        pCombined,
                        pMembership->membership.pszChildSid,
                        pMembership);
        BAIL_ON_LSA_ERROR(dwError);

        // It is now owned by the hash table
        pMembership = NULL;
    }

    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pParentSIDToMembershipList,
                    sMemberCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pChildSIDToMembershipList,
                    sMemberCount);
    BAIL_ON_LSA_ERROR(dwError);

    // Copy the existing pac and primary domain memberships to the temporary
    // hash table
    dwError = LwHashGetValue(
                    pConn->pParentSIDToMembershipList,
                    pszParentSid,
                    (PVOID*)&pGuardian);
    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pGuardian)
    {
        pPos = pGuardian->Next;
    }
    else
    {
        pPos = NULL;
    }
    while(pPos != pGuardian)
    {
        pExistingMembership = PARENT_NODE_TO_MEMBERSHIP(pPos);
        if (pExistingMembership->membership.bIsInPac ||
            pExistingMembership->membership.bIsDomainPrimaryGroup)
        {
            dwError = LwHashGetValue(
                            pCombined,
                            pExistingMembership->membership.pszChildSid,
                            (PVOID*)&pMembership);
            if (dwError == ERROR_NOT_FOUND)
            {
                // This entry from the existing cache data is not in the list
                // passed to this function
                dwError = MemCacheDuplicateMembership(
                                &pMembership,
                                &pExistingMembership->membership);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LwHashSetValue(
                                pCombined,
                                pMembership->membership.pszChildSid,
                                pMembership);
            }
            BAIL_ON_LSA_ERROR(dwError);

            pMembership->membership.bIsInPac |=
                pExistingMembership->membership.bIsInPac;
            pMembership->membership.bIsDomainPrimaryGroup |=
                pExistingMembership->membership.bIsDomainPrimaryGroup;
            pMembership->membership.bIsInPacOnly =
                pMembership->membership.bIsInPac && !pMembership->membership.bIsInLdap;

            // The membership is owned by the combined hash table. This stops
            // it from getting double freed.
            pMembership = NULL;
        }
        pPos = pPos->Next;
    }

    // Remove all of the existing memberships in the child hash and parent hash
    MemCacheRemoveMembershipsBySid(
        pConn,
        pszParentSid,
        TRUE,
        FALSE);

    // Copy the combined list into the parent and child hashes
    dwError = LwHashGetIterator(
                    pCombined,
                    &iterator);
    BAIL_ON_LSA_ERROR(dwError);

    while ((pEntry = LwHashNext(&iterator)) != NULL)
    {
        dwError = MemCacheAddMembership(
                    pConn,
                    (PMEM_GROUP_MEMBERSHIP)pEntry->pValue);
        BAIL_ON_LSA_ERROR(dwError);

        pEntry->pValue = NULL;
    }

    dwError = MemCacheMaintainSizeCap(pConn);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LwHashSafeFree(&pCombined);

    return dwError;

error:
    MemCacheSafeFreeGroupMembership(&pMembership);
    goto cleanup;
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
    // Indexed by parent sid (since child sid is identical for all entries)
    PLW_HASH_TABLE pCombined = NULL;
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pExistingMembership = NULL;
    // Do not free
    PLSA_LIST_LINKS pGuardian = NULL;
    // Do not free
    PLSA_LIST_LINKS pPos = NULL;
    LW_HASH_ITERATOR iterator = {0};
    size_t sIndex = 0;
    BOOLEAN bInLock = FALSE;
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    // Do not free
    LW_HASH_ENTRY *pEntry = NULL;
    time_t now = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    // Make a hash table to hold the new membership list along with any
    // existing memberships that need to be merged in.
    dwError = LwHashCreate(
                    sMemberCount * 2,
                    LwHashCaselessStringCompare,
                    LwHashCaselessStringHash,
                    MemCacheFreeMembershipValue,
                    NULL,
                    &pCombined);
    BAIL_ON_LSA_ERROR(dwError);

    // Copy the new items to the temporary hash table
    for (sIndex = 0; sIndex < sMemberCount; sIndex++)
    {
        dwError = MemCacheDuplicateMembership(
                        &pMembership,
                        ppMembers[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        pMembership->membership.version.tLastUpdated = now;

        dwError = LwHashSetValue(
                        pCombined,
                        pMembership->membership.pszParentSid,
                        pMembership);
        BAIL_ON_LSA_ERROR(dwError);

        // It is now owned by the hash table
        pMembership = NULL;
    }

    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pParentSIDToMembershipList,
                    sMemberCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pChildSIDToMembershipList,
                    sMemberCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LwHashGetValue(
                    pConn->pParentSIDToMembershipList,
                    pszChildSid,
                    (PVOID*)&pGuardian);
    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsPacAuthoritative && pGuardian != NULL)
    {
        // Copy the existing pac and primary domain memberships to the
        // temporary hash table
        pPos = pGuardian->Next;

        while(pPos != pGuardian)
        {
            pExistingMembership = PARENT_NODE_TO_MEMBERSHIP(pPos);
            if (pExistingMembership->membership.bIsInPac ||
                pExistingMembership->membership.bIsDomainPrimaryGroup)
            {
                dwError = LwHashGetValue(
                                pCombined,
                                pExistingMembership->membership.pszParentSid,
                                (PVOID*)&pMembership);
                if (dwError == ERROR_NOT_FOUND)
                {
                    // This entry from the existing cache data is not in the list
                    // passed to this function
                    dwError = MemCacheDuplicateMembership(
                                    &pMembership,
                                    &pExistingMembership->membership);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = LwHashSetValue(
                                    pCombined,
                                    pMembership->membership.pszParentSid,
                                    pMembership);
                }
                BAIL_ON_LSA_ERROR(dwError);

                pMembership->membership.bIsInPac |=
                    pExistingMembership->membership.bIsInPac;
                pMembership->membership.bIsDomainPrimaryGroup |=
                    pExistingMembership->membership.bIsDomainPrimaryGroup;
                pMembership->membership.bIsInPacOnly =
                    pMembership->membership.bIsInPac && !pMembership->membership.bIsInLdap;

                // The membership is owned by the combined hash table. This stops
                // it from getting double freed.
                pMembership = NULL;
            }
            pPos = pPos->Next;
        }
    }

    // Remove all of the existing memberships in the child hash and parent hash
    MemCacheRemoveMembershipsBySid(
        pConn,
        pszChildSid,
        FALSE,
        FALSE);

    // Copy the combined list into the parent and child hashes
    dwError = LwHashGetIterator(
                    pCombined,
                    &iterator);
    BAIL_ON_LSA_ERROR(dwError);

    while ((pEntry = LwHashNext(&iterator)) != NULL)
    {
        PMEM_GROUP_MEMBERSHIP pMember = (PMEM_GROUP_MEMBERSHIP)pEntry->pValue;

        if (pMember->membership.bIsInPac && !pMember->membership.bIsInLdap)
        {
            pMember->membership.bIsInPacOnly = TRUE;
        }

        dwError = MemCacheAddMembership(
                    pConn,
                    pMember);
        BAIL_ON_LSA_ERROR(dwError);

        pEntry->pValue = NULL;
    }

    dwError = MemCacheMaintainSizeCap(pConn);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LwHashSafeFree(&pCombined);

    return dwError;

error:
    MemCacheSafeFreeGroupMembership(&pMembership);
    goto cleanup;
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
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    // Do not free
    PLSA_LIST_LINKS pGuardian = NULL;
    // Do not free
    PLSA_LIST_LINKS pPos = NULL;
    size_t sCount = 0;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    PLSA_GROUP_MEMBERSHIP* ppResults = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    if (bIsGroupMembers)
    {
        pIndex = pConn->pParentSIDToMembershipList;
    }
    else
    {
        pIndex = pConn->pChildSIDToMembershipList;
    }

    dwError = LwHashGetValue(
                    pIndex,
                    pszSid,
                    (PVOID*)&pGuardian);
    if (dwError == ERROR_NOT_FOUND)
    {
        // This function returns success with 0 results when the group is not
        // found in the cache
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pGuardian)
    {
        pPos = pGuardian->Next;
    }
    else
    {
        pPos = NULL;
    }
    while (pPos != pGuardian)
    {
        sCount++;
        pPos = pPos->Next;
    }

    dwError = LwAllocateMemory(
                    sizeof(*ppResults) * sCount,
                    (PVOID*)&ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    if (pGuardian)
    {
        pPos = pGuardian->Next;
    }
    else
    {
        pPos = NULL;
    }
    sCount = 0;
    while (pPos != pGuardian)
    {
        if (bIsGroupMembers)
        {
            pMembership = PARENT_NODE_TO_MEMBERSHIP(pPos);
        }
        else
        {
            pMembership = CHILD_NODE_TO_MEMBERSHIP(pPos);
        }
        // Filter out stuff from PAC that was in LDAP
        // but is no longer in LDAP.
        if (bFilterNotInPacNorLdap &&
            pMembership->membership.bIsInPac &&
            !pMembership->membership.bIsInPacOnly &&
            !pMembership->membership.bIsInLdap)
        {
            LSA_LOG_DEBUG("Skipping membership because it is no longer in LDAP");
        }
        else
        {
            dwError = VMCacheDuplicateMembership(
                            &ppResults[sCount],
                            &pMembership->membership);
            BAIL_ON_LSA_ERROR(dwError);
            sCount++;
        }

        pPos = pPos->Next;
    }

    *pppResults = ppResults;
    *psCount = sCount;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    VMCacheSafeFreeGroupMembershipList(sCount, &ppResults);
    *pppResults = NULL;
    *psCount = 0;
    goto cleanup;
}

DWORD
MemCacheEnumUsersCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumUsers,
    IN PCSTR                   pszResume,
    OUT DWORD*                 pdwNumUsersFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    // Do not free
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;
    DWORD dwOut = 0;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pSIDToSecurityObject;

    dwMaxNumUsers = LW_MIN(dwMaxNumUsers, pIndex->sCount);

    dwError = LwAllocateMemory(
                    sizeof(*ppObjects) * dwMaxNumUsers,
                    (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszResume)
    {
        // Start at one after the resume SID
        dwError = LwHashGetValue(
                        pIndex,
                        pszResume,
                        (PVOID*)&pListEntry);
        if (dwError == ERROR_NOT_FOUND)
        {
            dwError = LW_ERROR_NOT_HANDLED;
        }
        BAIL_ON_LSA_ERROR(dwError);
        pListEntry = pListEntry->pNext;
    }
    else
    {
        // Start at the beginning of the list
        pListEntry = pConn->pObjects;
    }

    for (dwOut = 0;
        pListEntry != NULL && dwOut < dwMaxNumUsers;
        pListEntry = pListEntry->pNext)
    {
        pObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;
        if (pObject->type == LSA_OBJECT_TYPE_USER)
        {
            dwError = VMCacheDuplicateObject(
                            &ppObjects[dwOut],
                            pObject);
            BAIL_ON_LSA_ERROR(dwError);
            dwOut++;
        }
    }
    if (dwOut == 0)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppObjects = ppObjects;
    *pdwNumUsersFound = dwOut;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *pppObjects = NULL;
    *pdwNumUsersFound = 0;
    VMCacheSafeFreeObjectList(
            dwOut,
            &ppObjects);
    goto cleanup;
}

DWORD
MemCacheEnumGroupsCache(
    IN LSA_DB_HANDLE           hDb,
    IN DWORD                   dwMaxNumGroups,
    IN PCSTR                   pszResume,
    OUT DWORD*                 pdwNumGroupsFound,
    OUT PLSA_SECURITY_OBJECT** pppObjects
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT* ppObjects = NULL;
    // Do not free
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;
    DWORD dwOut = 0;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pSIDToSecurityObject;

    dwMaxNumGroups = LW_MIN(dwMaxNumGroups, pIndex->sCount);

    dwError = LwAllocateMemory(
                    sizeof(*ppObjects) * dwMaxNumGroups,
                    (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszResume)
    {
        // Start at one after the resume SID
        dwError = LwHashGetValue(
                        pIndex,
                        pszResume,
                        (PVOID*)&pListEntry);
        if (dwError == ERROR_NOT_FOUND)
        {
            dwError = LW_ERROR_NOT_HANDLED;
        }
        BAIL_ON_LSA_ERROR(dwError);
        pListEntry = pListEntry->pNext;
    }
    else
    {
        // Start at the beginning of the list
        pListEntry = pConn->pObjects;
    }

    for (dwOut = 0;
        pListEntry != NULL && dwOut < dwMaxNumGroups;
        pListEntry = pListEntry->pNext)
    {
        pObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;
        if (pObject->type == LSA_OBJECT_TYPE_GROUP)
        {
            dwError = VMCacheDuplicateObject(
                            &ppObjects[dwOut],
                            pObject);
            BAIL_ON_LSA_ERROR(dwError);
            dwOut++;
        }
    }
    if (dwOut == 0)
    {
        dwError = LW_ERROR_NOT_HANDLED;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppObjects = ppObjects;
    *pdwNumGroupsFound = dwOut;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *pppObjects = NULL;
    *pdwNumGroupsFound = 0;
    VMCacheSafeFreeObjectList(
            dwOut,
            &ppObjects);
    goto cleanup;
}


DWORD
MemCacheFindObjectByDN(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszDN,
    OUT PLSA_SECURITY_OBJECT *ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pDNToSecurityObject;

    dwError = LwHashGetValue(
                    pIndex,
                    pszDN,
                    (PVOID*)&pListEntry);
    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = VMCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    *ppObject = pObject;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    VMCacheSafeFreeObject(&pObject);
    goto cleanup;
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
    size_t sIndex = 0;
    PLSA_SECURITY_OBJECT* ppResults = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*ppResults) * sCount,
                    (PVOID*)&ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        dwError = MemCacheFindObjectByDN(
                        hDb,
                        ppszDnList[sIndex],
                        &ppResults[sIndex]);
        if (dwError == LW_ERROR_NOT_HANDLED)
        {
            // Leave this result as NULL to indicate it was not found
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppResults = ppResults;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(ppResults);
    *pppResults = NULL;
    goto cleanup;
}

DWORD
MemCacheFindObjectBySid(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszSid,
    OUT PLSA_SECURITY_OBJECT *ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLW_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pSIDToSecurityObject;

    dwError = LwHashGetValue(
                    pIndex,
                    pszSid,
                    (PVOID*)&pListEntry);
    if (dwError == ERROR_NOT_FOUND)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = VMCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    *ppObject = pObject;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    VMCacheSafeFreeObject(&pObject);
    goto cleanup;
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
    size_t sIndex = 0;
    PLSA_SECURITY_OBJECT* ppResults = NULL;

    dwError = LwAllocateMemory(
                    sizeof(*ppResults) * sCount,
                    (PVOID*)&ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    for (sIndex = 0; sIndex < sCount; sIndex++)
    {
        dwError = MemCacheFindObjectBySid(
                        hDb,
                        ppszSidList[sIndex],
                        &ppResults[sIndex]);
        if (dwError == LW_ERROR_NOT_HANDLED)
        {
            // Leave this result as NULL to indicate it was not found
            dwError = 0;
        }
        BAIL_ON_LSA_ERROR(dwError);
    }

    *pppResults = ppResults;

cleanup:
    return dwError;

error:
    LW_SAFE_FREE_MEMORY(ppResults);
    *pppResults = NULL;
    goto cleanup;
}

DWORD
MemCacheSetSizeCap(
    IN LSA_DB_HANDLE hDb,
    IN size_t sMemoryCap
    )
{
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;

    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    pConn->sSizeCap = sMemoryCap;

    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return 0;
}

void
InitializeMemCacheProvider(
    OUT PVMCACHE_PROVIDER_FUNCTION_TABLE pCacheTable
    )
{
    pCacheTable->pfnOpenHandle               = MemCacheOpen;
    pCacheTable->pfnSafeClose                = MemCacheSafeClose;
    pCacheTable->pfnFindUserByName           = MemCacheFindUserByName;
    pCacheTable->pfnFindUserById             = MemCacheFindUserById;
    pCacheTable->pfnFindGroupByName          = MemCacheFindGroupByName;
    pCacheTable->pfnFindGroupById            = MemCacheFindGroupById;
    pCacheTable->pfnEmptyCache               = MemCacheEmptyCache;
    pCacheTable->pfnStoreObjectEntries       = MemCacheStoreObjectEntries;
    pCacheTable->pfnStoreGroupMembership     = MemCacheStoreGroupMembership;
    pCacheTable->pfnStoreGroupsForUser       = MemCacheStoreGroupsForUser;
    pCacheTable->pfnGetMemberships           = MemCacheGetMemberships;
    pCacheTable->pfnEnumUsersCache           = MemCacheEnumUsersCache;
    pCacheTable->pfnEnumGroupsCache          = MemCacheEnumGroupsCache;
    pCacheTable->pfnFindObjectByDN           = MemCacheFindObjectByDN;
    pCacheTable->pfnFindObjectsByDNList      = MemCacheFindObjectsByDNList;
    pCacheTable->pfnFindObjectBySid          = MemCacheFindObjectBySid;
    pCacheTable->pfnFindObjectsBySidList     = MemCacheFindObjectsBySidList;
}
