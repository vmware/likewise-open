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

void
MemCacheFreeGuardian(
    const LSA_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pKey)
    {
        LsaFreeString(pEntry->pKey);
    }
    if (pEntry->pValue)
    {
        LsaFreeMemory(pEntry->pValue);
    }
}

DWORD
MemCacheOpen(
    IN PCSTR pszDbPath,
    OUT PLSA_DB_HANDLE phDb
    )
{
    DWORD dwError = 0;
    PMEM_DB_CONNECTION pConn = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(*pConn),
                    (PVOID*)&pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_rwlock_init(&pConn->lock, NULL);
    BAIL_ON_LSA_ERROR(dwError);
    pConn->bLockCreated = TRUE;

    //indexes
    dwError = LsaHashCreate(
                    100,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pDNToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    LsaHashFreeStringKey,
                    NULL,
                    &pConn->pNT4ToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pSIDToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashPVoidCompare,
                    LsaHashPVoidHash,
                    NULL,
                    NULL,
                    &pConn->pUIDToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pUserAliasToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pUPNToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pSIDToPasswordVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashPVoidCompare,
                    LsaHashPVoidHash,
                    NULL,
                    NULL,
                    &pConn->pGIDToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    NULL,
                    NULL,
                    &pConn->pGroupAliasToSecurityObject);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    MemCacheFreeGuardian,
                    NULL,
                    &pConn->pParentSIDToMembershipList);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashCreate(
                    100,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
                    MemCacheFreeGuardian,
                    NULL,
                    &pConn->pChildSIDToMembershipList);
    BAIL_ON_LSA_ERROR(dwError);

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
    PVOID pData,
    PVOID pUnused
    )
{
    PLSA_SECURITY_OBJECT pObject = (PLSA_SECURITY_OBJECT)pData;

    ADCacheSafeFreeObject(&pObject);
}

VOID
MemCacheFreePasswordVerifiers(
    PVOID pData,
    PVOID pUnused
    )
{
    PLSA_PASSWORD_VERIFIER pPassword = (PLSA_PASSWORD_VERIFIER)pData;

    ADCacheFreePasswordVerifier(pPassword);
}

DWORD
MemCacheRemoveMembership(
    PMEM_DB_CONNECTION pConn,
    PMEM_GROUP_MEMBERSHIP pMembership
    )
{
    DWORD dwError = 0;

    pMembership->parentListNode.pPrev->pNext = pMembership->parentListNode.pNext;
    pMembership->parentListNode.pNext->pPrev = pMembership->parentListNode.pPrev;

    if (pMembership->parentListNode.pPrev->pPrev == pMembership->parentListNode.pPrev)
    {
        // Only the guardian is left, so remove the hash entry
        dwError = LsaHashRemoveKey(
                        pConn->pParentSIDToMembershipList,
                        pMembership->membership.pszParentSid);
        BAIL_ON_LSA_ERROR(dwError);
    }

    pMembership->childListNode.pPrev->pNext = pMembership->childListNode.pNext;
    pMembership->childListNode.pNext->pPrev = pMembership->childListNode.pPrev;

    if (pMembership->childListNode.pPrev->pPrev == pMembership->childListNode.pPrev)
    {
        // Only the guardian is left, so remove the hash entry
        dwError = LsaHashRemoveKey(
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
    PLSA_DB_HANDLE phDb
    )
{
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)*phDb;
    LSA_HASH_ITERATOR iterator = {0};
    LSA_HASH_ENTRY *pEntry = NULL;
    DWORD dwError = 0;

    if (pConn)
    {
        LsaHashSafeFree(&pConn->pDNToSecurityObject);
        LsaHashSafeFree(&pConn->pNT4ToSecurityObject);
        LsaHashSafeFree(&pConn->pSIDToSecurityObject);

        LsaHashSafeFree(&pConn->pUIDToSecurityObject);
        LsaHashSafeFree(&pConn->pUserAliasToSecurityObject);
        LsaHashSafeFree(&pConn->pUPNToSecurityObject);

        LsaHashSafeFree(&pConn->pSIDToPasswordVerifier);

        LsaHashSafeFree(&pConn->pGIDToSecurityObject);
        LsaHashSafeFree(&pConn->pGroupAliasToSecurityObject);

        // Remove all of the group memberships. Either table may be iterated,
        // so the parentsid list was chosen.
        dwError = LsaHashGetIterator(
                        pConn->pParentSIDToMembershipList,
                        &iterator);
        LSA_ASSERT(dwError == 0);

        while ((pEntry = LsaHashNext(&iterator)) != NULL)
        {
            PMEM_LIST_NODE pGuardian = (PMEM_LIST_NODE)pEntry->pValue;
            // Since the hash entry exists, the list must be non-empty
            BOOLEAN bListNonempty = TRUE;

            while (bListNonempty)
            {
                LSA_ASSERT(pGuardian->pNext != pGuardian);
                if (pGuardian->pNext->pNext == pGuardian)
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
                                PARENT_NODE_TO_MEMBERSHIP(pGuardian->pNext));
                LSA_ASSERT(dwError == 0);
            }
        }

        LSA_ASSERT(pConn->pParentSIDToMembershipList->sCount == 0);
        LsaHashSafeFree(&pConn->pParentSIDToMembershipList);
        LSA_ASSERT(pConn->pChildSIDToMembershipList->sCount == 0);
        LsaHashSafeFree(&pConn->pChildSIDToMembershipList);

        LsaDLinkedListForEach(
            pConn->pObjects,
            MemCacheFreeObjects,
            NULL);
        LsaDLinkedListFree(pConn->pObjects);
        pConn->pObjects = NULL;

        LsaDLinkedListForEach(
            pConn->pPasswordVerifiers,
            MemCacheFreePasswordVerifiers,
            NULL);
        LsaDLinkedListFree(pConn->pPasswordVerifiers);
        pConn->pPasswordVerifiers = NULL;


        if (pConn->bLockCreated)
        {
            pthread_rwlock_destroy(&pConn->lock);
        }
        LSA_SAFE_FREE_MEMORY(*phDb);
    }
}

DWORD
MemCacheFindUserByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pUserNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLSA_HASH_TABLE pIndex = NULL;
    PSTR pszKey = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    switch (pUserNameInfo->nameType)
    {
        case NameType_UPN:
            pIndex = pConn->pUPNToSecurityObject;

            dwError = LsaAllocateStringPrintf(
                            &pszKey,
                            "%s@%s",
                            pUserNameInfo->pszName,
                            pUserNameInfo->pszFullDomainName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
       case NameType_NT4:
            pIndex = pConn->pNT4ToSecurityObject;

            dwError = LsaAllocateStringPrintf(
                            &pszKey,
                            "%s\\%s",
                            pUserNameInfo->pszDomainNetBiosName,
                            pUserNameInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
       case NameType_Alias:
            pIndex = pConn->pUserAliasToSecurityObject;

            dwError = LsaAllocateStringPrintf(
                            &pszKey,
                            "%s",
                            pUserNameInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
       default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaHashGetValue(
                    pIndex,
                    pszKey,
                    (PVOID*)&pListEntry);
    if (dwError == ENOENT)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_User)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);
    LSA_SAFE_FREE_STRING(pszKey);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheSafeFreeObject(&pObject);
    goto cleanup;
}

// returns LW_ERROR_NOT_HANDLED if the user is not in the database
DWORD
MemCacheFindUserById(
    LSA_DB_HANDLE hDb,
    uid_t uid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pUIDToSecurityObject;

    dwError = LsaHashGetValue(
                    pIndex,
                    (PVOID)uid,
                    (PVOID*)&pListEntry);
    if (dwError == ENOENT)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_User)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheSafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
MemCacheFindGroupByName(
    LSA_DB_HANDLE hDb,
    PLSA_LOGIN_NAME_INFO pGroupNameInfo,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLSA_HASH_TABLE pIndex = NULL;
    PSTR pszKey = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    switch (pGroupNameInfo->nameType)
    {
       case NameType_NT4:
            pIndex = pConn->pNT4ToSecurityObject;

            dwError = LsaAllocateStringPrintf(
                            &pszKey,
                            "%s\\%s",
                            pGroupNameInfo->pszDomainNetBiosName,
                            pGroupNameInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
       case NameType_Alias:
            pIndex = pConn->pGroupAliasToSecurityObject;

            dwError = LsaAllocateStringPrintf(
                            &pszKey,
                            "%s",
                            pGroupNameInfo->pszName);
            BAIL_ON_LSA_ERROR(dwError);
            break;
       default:
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = LsaHashGetValue(
                    pIndex,
                    pszKey,
                    (PVOID *)&pListEntry);
    if (dwError == ENOENT)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_Group)
    {
        dwError = LW_ERROR_NO_SUCH_GROUP;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);
    LSA_SAFE_FREE_STRING(pszKey);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheSafeFreeObject(&pObject);
    goto cleanup;
}

DWORD
MemCacheFindGroupById(
    LSA_DB_HANDLE hDb,
    gid_t gid,
    PLSA_SECURITY_OBJECT* ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pGIDToSecurityObject;

    dwError = LsaHashGetValue(
                    pIndex,
                    (PVOID)gid,
                    (PVOID *)&pListEntry);
    if (dwError == ENOENT)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->type != AccountType_Group)
    {
        dwError = LW_ERROR_NO_SUCH_USER;
        BAIL_ON_LSA_ERROR(dwError);
    }

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheSafeFreeObject(&pObject);
    goto cleanup;
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
    return MemCacheStoreObjectEntries(
            hDb,
            1,
            &pObject);
}

DWORD
MemCacheRemoveObjectByHashKey(
    PMEM_DB_CONNECTION pConn,
    PLSA_HASH_TABLE pTable,
    PVOID pvKey
    )
{
    DWORD dwError = 0;
    PDLINKEDLIST pListEntry = NULL;
    PLSA_SECURITY_OBJECT pObject = NULL;
    PSTR pszKey = NULL;

    dwError = LsaHashGetValue(
                    pTable,
                    pvKey,
                    (PVOID*)&pListEntry);
    if (dwError == ENOENT)
    {
        // The key does not exist
        dwError = 0;
        goto cleanup;
    }
    BAIL_ON_LSA_ERROR(dwError);

    pObject = (PLSA_SECURITY_OBJECT)pListEntry->pItem;

    //Remove it from all indexes
    dwError = LsaHashRemoveKey(
                    pConn->pDNToSecurityObject,
                    pObject->pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszKey,
                    "%s\\%s",
                    pObject->pszNetbiosDomainName,
                    pObject->pszSamAccountName);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashRemoveKey(
                    pConn->pNT4ToSecurityObject,
                    pszKey);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashRemoveKey(
                    pConn->pSIDToSecurityObject,
                    pObject->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    if (pObject->enabled && pObject->type == AccountType_User)
    {
        dwError = LsaHashRemoveKey(
                        pConn->pUIDToSecurityObject,
                        (PVOID)pObject->userInfo.uid);
        BAIL_ON_LSA_ERROR(dwError);

        if (pObject->userInfo.pszAliasName != NULL)
        {
            dwError = LsaHashRemoveKey(
                            pConn->pUserAliasToSecurityObject,
                            pObject->userInfo.pszAliasName);
            BAIL_ON_LSA_ERROR(dwError);
        }

        if (pObject->userInfo.pszUPN != NULL)
        {
            dwError = LsaHashRemoveKey(
                            pConn->pUPNToSecurityObject,
                            pObject->userInfo.pszUPN);
            BAIL_ON_LSA_ERROR(dwError);
        }
    }
    else if (pObject->enabled && pObject->type == AccountType_Group)
    {
        dwError = LsaHashRemoveKey(
                        pConn->pGIDToSecurityObject,
                        (PVOID)pObject->groupInfo.gid);
        BAIL_ON_LSA_ERROR(dwError);

        if (pObject->groupInfo.pszAliasName != NULL)
        {
            dwError = LsaHashRemoveKey(
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
    LSA_SAFE_FREE_MEMORY(pListEntry);
    ADCacheSafeFreeObject(&pObject);

cleanup:
    LSA_SAFE_FREE_STRING(pszKey);
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheClearExistingObjectKeys(
    PMEM_DB_CONNECTION pConn,
    PLSA_SECURITY_OBJECT pObject
    )
{
    DWORD dwError = 0;
    PSTR pszKey = NULL;

    dwError = MemCacheRemoveObjectByHashKey(
                    pConn,
                    pConn->pDNToSecurityObject,
                    pObject->pszDN);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszKey,
                    "%s\\%s",
                    pObject->pszNetbiosDomainName,
                    pObject->pszSamAccountName);
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

    if (pObject->enabled && pObject->type == AccountType_User)
    {
        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pUIDToSecurityObject,
                        (PVOID)pObject->userInfo.uid);
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
    else if (pObject->enabled && pObject->type == AccountType_Group)
    {
        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pGIDToSecurityObject,
                        (PVOID)pObject->groupInfo.gid);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = MemCacheRemoveObjectByHashKey(
                        pConn,
                        pConn->pGroupAliasToSecurityObject,
                        pObject->groupInfo.pszAliasName);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LSA_SAFE_FREE_STRING(pszKey);
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheStoreObjectEntries(
    LSA_DB_HANDLE hDb,
    size_t  sObjectCount,
    PLSA_SECURITY_OBJECT* ppObjects
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    size_t sIndex = 0;
    // Do not free
    PLSA_SECURITY_OBJECT pObject = NULL;
    PLSA_SECURITY_OBJECT pObjectTemp = NULL;
    PSTR pszKey = NULL;
    time_t now = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    for (sIndex = 0; sIndex < sObjectCount; sIndex++)
    {
        dwError = MemCacheClearExistingObjectKeys(
                        pConn,
                        ppObjects[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = ADCacheDuplicateObject(
                        &pObjectTemp,
                        ppObjects[sIndex]);
        BAIL_ON_LSA_ERROR(dwError);

        pObjectTemp->version.tLastUpdated = now;

        // Afterwards pConn->pObjects points to the new node with pObject
        // inside. This node pointer will stored in the hash tables.
        dwError = LsaDLinkedListPrepend(
                        &pConn->pObjects,
                        pObjectTemp);
        BAIL_ON_LSA_ERROR(dwError);

        // pObjectTemp is now owned by the linked list
        pObject = pObjectTemp;
        pObjectTemp = NULL;

        if (pObject->pszDN != NULL)
        {
            dwError = LsaHashSetValue(
                            pConn->pDNToSecurityObject,
                            pObject->pszDN,
                            pConn->pObjects);
            BAIL_ON_LSA_ERROR(dwError);
        }

        dwError = LsaAllocateStringPrintf(
                        &pszKey,
                        "%s\\%s",
                        pObject->pszNetbiosDomainName,
                        pObject->pszSamAccountName);
        BAIL_ON_LSA_ERROR(dwError);

        LSA_ASSERT(pszKey != NULL);
        dwError = LsaHashSetValue(
                        pConn->pNT4ToSecurityObject,
                        pszKey,
                        pConn->pObjects);
        BAIL_ON_LSA_ERROR(dwError);
        // The key is now owned by the hash table
        pszKey = NULL;

        LSA_ASSERT(pObject->pszObjectSid);
        dwError = LsaHashSetValue(
                        pConn->pSIDToSecurityObject,
                        pObject->pszObjectSid,
                        pConn->pObjects);
        BAIL_ON_LSA_ERROR(dwError);

        switch(pObject->type)
        {
            case AccountType_Group:
                dwError = LsaHashSetValue(
                                pConn->pGIDToSecurityObject,
                                (PVOID)pObject->groupInfo.gid,
                                pConn->pObjects);
                BAIL_ON_LSA_ERROR(dwError);

                if (pObject->groupInfo.pszAliasName)
                {
                    dwError = LsaHashSetValue(
                                    pConn->pGroupAliasToSecurityObject,
                                    pObject->groupInfo.pszAliasName,
                                    pConn->pObjects);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;
            case AccountType_User:
                dwError = LsaHashSetValue(
                                pConn->pUIDToSecurityObject,
                                (PVOID)pObject->userInfo.uid,
                                pConn->pObjects);
                BAIL_ON_LSA_ERROR(dwError);

                if (pObject->userInfo.pszAliasName)
                {
                    dwError = LsaHashSetValue(
                                    pConn->pUserAliasToSecurityObject,
                                    pObject->userInfo.pszAliasName,
                                    pConn->pObjects);
                    BAIL_ON_LSA_ERROR(dwError);
                }

                if (pObject->userInfo.pszUPN)
                {
                    dwError = LsaHashSetValue(
                                    pConn->pUPNToSecurityObject,
                                    pObject->userInfo.pszUPN,
                                    pConn->pObjects);
                    BAIL_ON_LSA_ERROR(dwError);
                }
                break;
        }
    }

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);
    LSA_SAFE_FREE_STRING(pszKey);

    return dwError;

error:
    ADCacheSafeFreeObject(&pObjectTemp);
    goto cleanup;
}

void
MemCacheSafeFreeGroupMembership(
    PMEM_GROUP_MEMBERSHIP* ppMembership
    )
{
    if (*ppMembership != NULL)
    {
        ADCacheFreeGroupMembershipContents(&(*ppMembership)->membership);
        LSA_SAFE_FREE_MEMORY(*ppMembership);
    }
}

void
MemCacheFreeMembershipValue(
    const LSA_HASH_ENTRY* pEntry
    )
{
    PMEM_GROUP_MEMBERSHIP pMembership = (PMEM_GROUP_MEMBERSHIP)pEntry->pValue;

    MemCacheSafeFreeGroupMembership(&pMembership);
}

DWORD
MemCacheDuplicateMembership(
    PMEM_GROUP_MEMBERSHIP* ppDest,
    PLSA_GROUP_MEMBERSHIP pSrc
    )
{
    DWORD dwError = 0;
    PMEM_GROUP_MEMBERSHIP pDest = NULL;

    dwError = LsaAllocateMemory(
                    sizeof(*pDest),
                    (PVOID*)&pDest);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDuplicateMembershipContents(
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
    PMEM_LIST_NODE pGuardian = NULL;
    DWORD dwError = 0;
    PMEM_LIST_NODE pGuardianTemp = NULL;

    dwError = LsaHashGetValue(
                    pConn->pParentSIDToMembershipList,
                    pMembership->membership.pszParentSid,
                    (PVOID*)&pGuardian);
    if (dwError == ENOENT)
    {
        // Gotta add the guardian node
        dwError = 0;

        dwError = LsaAllocateMemory(
                        sizeof(*pGuardianTemp),
                        (PVOID*)&pGuardianTemp);
        BAIL_ON_LSA_ERROR(dwError);

        pGuardianTemp->pNext = pGuardianTemp;
        pGuardianTemp->pPrev = pGuardianTemp;

        dwError = LsaHashSetValue(
                        pConn->pParentSIDToMembershipList,
                        pMembership->membership.pszParentSid,
                        pGuardianTemp);
        BAIL_ON_LSA_ERROR(dwError);

        // The guardian is now owned by the hash table, so don't free it if
        // something goes wrong after this point.
        pGuardian = pGuardianTemp;
        pGuardianTemp = NULL;
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    pMembership->parentListNode.pNext = pGuardian->pNext;
    pMembership->parentListNode.pPrev = pGuardian;

    pGuardian->pNext->pPrev = &pMembership->parentListNode;
    pGuardian->pNext = &pMembership->parentListNode;

    dwError = LsaHashGetValue(
                    pConn->pChildSIDToMembershipList,
                    pMembership->membership.pszChildSid,
                    (PVOID*)&pGuardian);
    if (dwError == ENOENT)
    {
        // Gotta add the guardian node
        dwError = 0;

        dwError = LsaAllocateMemory(
                        sizeof(*pGuardianTemp),
                        (PVOID*)&pGuardianTemp);
        BAIL_ON_LSA_ERROR(dwError);

        pGuardianTemp->pNext = pGuardianTemp;
        pGuardianTemp->pPrev = pGuardianTemp;

        dwError = LsaHashSetValue(
                        pConn->pChildSIDToMembershipList,
                        pMembership->membership.pszChildSid,
                        pGuardianTemp);
        BAIL_ON_LSA_ERROR(dwError);

        // The guardian is now owned by the hash table, so don't free it if
        // something goes wrong after this point.
        pGuardian = pGuardianTemp;
        pGuardianTemp = NULL;
    }
    else
    {
        BAIL_ON_LSA_ERROR(dwError);
    }

    pMembership->childListNode.pNext = pGuardian->pNext;
    pMembership->childListNode.pPrev = pGuardian;

    pGuardian->pNext->pPrev = &pMembership->childListNode;
    pGuardian->pNext = &pMembership->childListNode;

cleanup:
    LSA_SAFE_FREE_MEMORY(pGuardianTemp);
    return dwError;

error:
    goto cleanup;
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
    PLSA_HASH_TABLE pCombined = NULL;
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pExistingMembership = NULL;
    // Do not free
    PMEM_LIST_NODE pGuardian = NULL;
    // Do not free
    PMEM_LIST_NODE pPos = NULL;
    BOOLEAN bListNonempty = FALSE;
    LSA_HASH_ITERATOR iterator = {0};
    size_t sIndex = 0;
    BOOLEAN bInLock = FALSE;
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    // Do not free
    LSA_HASH_ENTRY *pEntry = NULL;
    time_t now = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    // Make a hash table to hold the new membership list along with any
    // existing memberships that need to be merged in.
    dwError = LsaHashCreate(
                    sMemberCount * 2,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
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

        dwError = LsaHashSetValue(
                        pCombined,
                        pMembership->membership.pszChildSid,
                        pMembership);
        BAIL_ON_LSA_ERROR(dwError);

        // It is now owned by the hash table
        pMembership = NULL;
    }

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    // Copy the existing pac and primary domain memberships to the temporary
    // hash table
    dwError = LsaHashGetValue(
                    pConn->pParentSIDToMembershipList,
                    pszParentSid,
                    (PVOID*)&pGuardian);
    if (dwError == ENOENT)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pGuardian)
    {
        pPos = pGuardian->pNext;
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
            dwError = LsaHashGetValue(
                            pCombined,
                            pExistingMembership->membership.pszChildSid,
                            (PVOID*)&pMembership);
            if (dwError == ENOENT)
            {
                // This entry from the existing cache data is not in the list
                // passed to this function
                dwError = MemCacheDuplicateMembership(
                                &pMembership,
                                &pExistingMembership->membership);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = LsaHashSetValue(
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
        pPos = pPos->pNext;
    }

    // Remove all of the existing memberships in the child hash and parent hash
    if (pGuardian)
    {
        // Since the hash entry exists, the list must be non-empty
        bListNonempty = TRUE;
    }

    while (bListNonempty)
    {
        LSA_ASSERT(pGuardian->pNext != pGuardian);
        if (pGuardian->pNext->pNext == pGuardian)
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
                        PARENT_NODE_TO_MEMBERSHIP(pGuardian->pNext));
        LSA_ASSERT(dwError == 0);
    }

    // Copy the combined list into the parent and child hashes
    dwError = LsaHashGetIterator(
                    pCombined,
                    &iterator);
    BAIL_ON_LSA_ERROR(dwError);

    while ((pEntry = LsaHashNext(&iterator)) != NULL)
    {
        dwError = MemCacheAddMembership(
                    pConn,
                    (PMEM_GROUP_MEMBERSHIP)pEntry->pValue);
        BAIL_ON_LSA_ERROR(dwError);

        pEntry->pValue = NULL;
    }

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);
    LsaHashSafeFree(&pCombined);

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
    PLSA_HASH_TABLE pCombined = NULL;
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pExistingMembership = NULL;
    // Do not free
    PMEM_LIST_NODE pGuardian = NULL;
    // Do not free
    PMEM_LIST_NODE pPos = NULL;
    BOOLEAN bListNonempty = FALSE;
    LSA_HASH_ITERATOR iterator = {0};
    size_t sIndex = 0;
    BOOLEAN bInLock = FALSE;
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    // Do not free
    LSA_HASH_ENTRY *pEntry = NULL;
    time_t now = 0;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    // Make a hash table to hold the new membership list along with any
    // existing memberships that need to be merged in.
    dwError = LsaHashCreate(
                    sMemberCount * 2,
                    LsaHashCaselessStringCompare,
                    LsaHashCaselessStringHash,
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

        dwError = LsaHashSetValue(
                        pCombined,
                        pMembership->membership.pszParentSid,
                        pMembership);
        BAIL_ON_LSA_ERROR(dwError);

        // It is now owned by the hash table
        pMembership = NULL;
    }

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    dwError = LsaHashGetValue(
                    pConn->pParentSIDToMembershipList,
                    pszChildSid,
                    (PVOID*)&pGuardian);
    if (dwError == ENOENT)
    {
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (!bIsPacAuthoritative && pGuardian != NULL)
    {
        // Copy the existing pac and primary domain memberships to the
        // temporary hash table
        pPos = pGuardian->pNext;

        while(pPos != pGuardian)
        {
            pExistingMembership = CHILD_NODE_TO_MEMBERSHIP(pPos);
            if (pExistingMembership->membership.bIsInPac ||
                pExistingMembership->membership.bIsDomainPrimaryGroup)
            {
                dwError = LsaHashGetValue(
                                pCombined,
                                pExistingMembership->membership.pszParentSid,
                                (PVOID*)&pMembership);
                if (dwError == ENOENT)
                {
                    // This entry from the existing cache data is not in the list
                    // passed to this function
                    dwError = MemCacheDuplicateMembership(
                                    &pMembership,
                                    &pExistingMembership->membership);
                    BAIL_ON_LSA_ERROR(dwError);

                    dwError = LsaHashSetValue(
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
            pPos = pPos->pNext;
        }
    }

    // Remove all of the existing memberships in the child hash and parent hash
    if (pGuardian)
    {
        // Since the hash entry exists, the list must be non-empty
        bListNonempty = TRUE;
    }

    while (bListNonempty)
    {
        LSA_ASSERT(pGuardian->pNext != pGuardian);
        if (pGuardian->pNext->pNext == pGuardian)
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
                        CHILD_NODE_TO_MEMBERSHIP(pGuardian->pNext));
        LSA_ASSERT(dwError == 0);
    }

    // Copy the combined list into the parent and child hashes
    dwError = LsaHashGetIterator(
                    pCombined,
                    &iterator);
    BAIL_ON_LSA_ERROR(dwError);

    while ((pEntry = LsaHashNext(&iterator)) != NULL)
    {
        dwError = MemCacheAddMembership(
                    pConn,
                    (PMEM_GROUP_MEMBERSHIP)pEntry->pValue);
        BAIL_ON_LSA_ERROR(dwError);

        pEntry->pValue = NULL;
    }

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);
    LsaHashSafeFree(&pCombined);

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
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PMEM_LIST_NODE pGuardian = NULL;
    // Do not free
    PMEM_LIST_NODE pPos = NULL;
    size_t sCount = 0;
    // Do not free
    PMEM_GROUP_MEMBERSHIP pMembership = NULL;
    PLSA_GROUP_MEMBERSHIP* ppResults = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    if (bIsGroupMembers)
    {
        pIndex = pConn->pParentSIDToMembershipList;
    }
    else
    {
        pIndex = pConn->pChildSIDToMembershipList;
    }

    dwError = LsaHashGetValue(
                    pIndex,
                    pszSid,
                    (PVOID*)&pGuardian);
    if (dwError == ENOENT)
    {
        // This function returns success with 0 results when the group is not
        // found in the cache
        dwError = 0;
    }
    BAIL_ON_LSA_ERROR(dwError);

    if (pGuardian)
    {
        pPos = pGuardian->pNext;
    }
    else
    {
        pPos = NULL;
    }
    while (pPos != pGuardian)
    {
        sCount++;
        pPos = pPos->pNext;
    }

    dwError = LsaAllocateMemory(
                    sizeof(*ppResults) * sCount,
                    (PVOID*)&ppResults);
    BAIL_ON_LSA_ERROR(dwError);

    if (pGuardian)
    {
        pPos = pGuardian->pNext;
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
            dwError = ADCacheDuplicateMembership(
                            &ppResults[sCount],
                            &pMembership->membership);
            BAIL_ON_LSA_ERROR(dwError);
            sCount++;
        }

        pPos = pPos->pNext;
    }

    *pppResults = ppResults;
    *psCount = sCount;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    ADCacheSafeFreeGroupMembershipList(sCount, &ppResults);
    *pppResults = NULL;
    *psCount = 0;
    goto cleanup;
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
    return MemCacheGetMemberships(hDb, pszSid, TRUE,
                                    bFilterNotInPacNorLdap,
                                    psCount, pppResults);
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
    return MemCacheGetMemberships(hDb, pszSid, FALSE,
                                    bFilterNotInPacNorLdap,
                                    psCount, pppResults);
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
    return 0;
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
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pDNToSecurityObject;

    dwError = LsaHashGetValue(
                    pIndex,
                    pszDN,
                    (PVOID*)&pListEntry);
    if (dwError == ENOENT)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheSafeFreeObject(&pObject);
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

    dwError = LsaAllocateMemory(
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
    LSA_SAFE_FREE_MEMORY(ppResults);
    *pppResults = NULL;
    goto cleanup;
}

DWORD
MemCacheFindObjectBySid(
    LSA_DB_HANDLE hDb,
    PCSTR pszSid,
    PLSA_SECURITY_OBJECT *ppObject
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_SECURITY_OBJECT pObject = NULL;
    // Do not free
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pSIDToSecurityObject;

    dwError = LsaHashGetValue(
                    pIndex,
                    pszSid,
                    (PVOID*)&pListEntry);
    if (dwError == ENOENT)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDuplicateObject(
                    &pObject,
                    (PLSA_SECURITY_OBJECT)pListEntry->pItem);
    BAIL_ON_LSA_ERROR(dwError);

    *ppObject = pObject;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheSafeFreeObject(&pObject);
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

    dwError = LsaAllocateMemory(
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
    LSA_SAFE_FREE_MEMORY(ppResults);
    *pppResults = NULL;
    goto cleanup;
}

// returns LW_ERROR_NOT_HANDLED if the user is not in the database
DWORD
MemCacheGetPasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PCSTR pszUserSid,
    OUT PLSA_PASSWORD_VERIFIER *ppResult
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_PASSWORD_VERIFIER pResult = NULL;
    // Do not free
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_SQLITE_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pSIDToPasswordVerifier;

    dwError = LsaHashGetValue(
                    pIndex,
                    pszUserSid,
                    (PVOID*)&pListEntry);
    if (dwError == ENOENT)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateMemory(
                    sizeof(*pResult),
                    (PVOID*)&pResult);
    BAIL_ON_LSA_ERROR(dwError);

    pResult->version = ((PLSA_PASSWORD_VERIFIER)pListEntry->pItem)->version;

    dwError = LsaAllocateString(
                    ((PLSA_PASSWORD_VERIFIER)pListEntry->pItem)->pszObjectSid,
                    &pResult->pszObjectSid);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateString(
                    ((PLSA_PASSWORD_VERIFIER)pListEntry->pItem)->
                        pszPasswordVerifier,
                    &pResult->pszPasswordVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    *ppResult = pResult;

cleanup:
    LEAVE_SQLITE_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppResult = NULL;
    LSA_DB_SAFE_FREE_PASSWORD_VERIFIER(pResult);
    goto cleanup;
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
InitializeMemCacheProvider(
    PADCACHE_PROVIDER_FUNCTION_TABLE pCacheTable
    )
{
    pCacheTable->pfnOpenHandle               = MemCacheOpen;
    pCacheTable->pfnSafeClose                = MemCacheSafeClose;
    pCacheTable->pfnFindUserByName           = MemCacheFindUserByName;
    pCacheTable->pfnFindUserById             = MemCacheFindUserById;
    pCacheTable->pfnFindGroupByName          = MemCacheFindGroupByName;
    pCacheTable->pfnFindGroupById            = MemCacheFindGroupById;
    pCacheTable->pfnRemoveUserBySid          = MemCacheRemoveUserBySid;
    pCacheTable->pfnRemoveGroupBySid         = MemCacheRemoveGroupBySid;
    pCacheTable->pfnEmptyCache               = MemCacheEmptyCache;
    pCacheTable->pfnStoreObjectEntry         = MemCacheStoreObjectEntry;
    pCacheTable->pfnStoreObjectEntries       = MemCacheStoreObjectEntries;
    pCacheTable->pfnStoreGroupMembership     = MemCacheStoreGroupMembership;
    pCacheTable->pfnStoreGroupsForUser       = MemCacheStoreGroupsForUser;
    pCacheTable->pfnGetMemberships           = MemCacheGetMemberships;
    pCacheTable->pfnGetGroupMembers          = MemCacheGetGroupMembers;
    pCacheTable->pfnGetGroupsForUser         = MemCacheGetGroupsForUser;
    pCacheTable->pfnEnumUsersCache           = MemCacheEnumUsersCache;
    pCacheTable->pfnEnumGroupsCache          = MemCacheEnumGroupsCache;
    pCacheTable->pfnFindObjectByDN           = MemCacheFindObjectByDN;
    pCacheTable->pfnFindObjectsByDNList      = MemCacheFindObjectsByDNList;
    pCacheTable->pfnFindObjectBySid          = MemCacheFindObjectBySid;
    pCacheTable->pfnFindObjectsBySidList      = MemCacheFindObjectsBySidList;
    pCacheTable->pfnGetPasswordVerifier      = MemCacheGetPasswordVerifier;
    pCacheTable->pfnStorePasswordVerifier    = MemCacheStorePasswordVerifier;
}

#endif
