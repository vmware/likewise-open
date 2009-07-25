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


typedef enum __MemCachePersistTag
{
    MEM_CACHE_OBJECT,
    MEM_CACHE_MEMBERSHIP,
    MEM_CACHE_PASSWORD,
} MemCachePersistTag;

static LWMsgTypeSpec gLsaSecurityObjectVersionSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_SECURITY_OBJECT_VERSION_INFO),
    LWMSG_MEMBER_INT64(LSA_SECURITY_OBJECT_VERSION_INFO, qwDbId),
    LWMSG_MEMBER_INT64(LSA_SECURITY_OBJECT_VERSION_INFO, tLastUpdated),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaGroupMembershipSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_GROUP_MEMBERSHIP),
    LWMSG_MEMBER_TYPESPEC(LSA_GROUP_MEMBERSHIP, version, gLsaSecurityObjectVersionSpec),
    LWMSG_MEMBER_PSTR(LSA_GROUP_MEMBERSHIP, pszParentSid),
    LWMSG_MEMBER_PSTR(LSA_GROUP_MEMBERSHIP, pszChildSid),
    LWMSG_MEMBER_UINT8(LSA_GROUP_MEMBERSHIP, bIsInPac),
    LWMSG_MEMBER_UINT8(LSA_GROUP_MEMBERSHIP, bIsInPacOnly),
    LWMSG_MEMBER_UINT8(LSA_GROUP_MEMBERSHIP, bIsInLdap),
    LWMSG_MEMBER_UINT8(LSA_GROUP_MEMBERSHIP, bIsDomainPrimaryGroup),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaPasswordVerifierSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_PASSWORD_VERIFIER),
    LWMSG_MEMBER_TYPESPEC(LSA_PASSWORD_VERIFIER, version, gLsaSecurityObjectVersionSpec),
    LWMSG_MEMBER_PSTR(LSA_PASSWORD_VERIFIER, pszObjectSid),
    LWMSG_MEMBER_PSTR(LSA_PASSWORD_VERIFIER, pszPasswordVerifier),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaSecurityObjectUserInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_SECURITY_OBJECT_USER_INFO),
    LWMSG_MEMBER_UINT32(LSA_SECURITY_OBJECT_USER_INFO, uid),
    LWMSG_MEMBER_UINT32(LSA_SECURITY_OBJECT_USER_INFO, gid),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT_USER_INFO, pszUPN),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT_USER_INFO, pszAliasName),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT_USER_INFO, pszPasswd),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT_USER_INFO, pszGecos),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT_USER_INFO, pszShell),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT_USER_INFO, pszHomedir),
    LWMSG_MEMBER_UINT64(LSA_SECURITY_OBJECT_USER_INFO, qwPwdLastSet),
    LWMSG_MEMBER_UINT64(LSA_SECURITY_OBJECT_USER_INFO, qwAccountExpires),

    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT_USER_INFO, bIsGeneratedUPN),
    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT_USER_INFO, bIsAccountInfoKnown),
    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT_USER_INFO, bPasswordExpired),
    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT_USER_INFO, bPasswordNeverExpires),
    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT_USER_INFO, bPromptPasswordChange),
    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT_USER_INFO, bUserCanChangePassword),
    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT_USER_INFO, bAccountDisabled),
    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT_USER_INFO, bAccountExpired),
    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT_USER_INFO, bAccountLocked),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaSecurityObjectGroupInfoSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_SECURITY_OBJECT_GROUP_INFO),
    LWMSG_MEMBER_UINT32(LSA_SECURITY_OBJECT_GROUP_INFO, gid),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT_GROUP_INFO, pszAliasName),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT_GROUP_INFO, pszPasswd),
    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgTypeSpec gLsaSecurityObjectSpec[] =
{
    LWMSG_STRUCT_BEGIN(LSA_SECURITY_OBJECT),

    LWMSG_MEMBER_TYPESPEC(LSA_SECURITY_OBJECT, version, gLsaSecurityObjectVersionSpec),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT, pszDN),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT, pszObjectSid),
    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT, enabled),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT, pszNetbiosDomainName),
    LWMSG_MEMBER_PSTR(LSA_SECURITY_OBJECT, pszSamAccountName),

    LWMSG_MEMBER_UINT8(LSA_SECURITY_OBJECT, type),
    LWMSG_MEMBER_UNION_BEGIN(LSA_SECURITY_OBJECT, typeInfo),
    LWMSG_MEMBER_TYPESPEC(LSA_SECURITY_OBJECT, userInfo, gLsaSecurityObjectUserInfoSpec),
    LWMSG_ATTR_TAG(AccountType_User),
    LWMSG_MEMBER_TYPESPEC(LSA_SECURITY_OBJECT, groupInfo, gLsaSecurityObjectGroupInfoSpec),
    LWMSG_ATTR_TAG(AccountType_Group),
    LWMSG_UNION_END,
    LWMSG_ATTR_DISCRIM(LSA_SECURITY_OBJECT, type),

    LWMSG_STRUCT_END,
    LWMSG_TYPE_END
};

static LWMsgProtocolSpec gMemCachePersistence[] =
{
    LWMSG_MESSAGE(MEM_CACHE_OBJECT, gLsaSecurityObjectSpec),
    LWMSG_MESSAGE(MEM_CACHE_MEMBERSHIP, gLsaGroupMembershipSpec),
    LWMSG_MESSAGE(MEM_CACHE_PASSWORD, gLsaPasswordVerifierSpec),
    LWMSG_PROTOCOL_END
};

void
MemCacheFreeGuardian(
    IN const LSA_HASH_ENTRY* pEntry
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

void
MemCacheFreePasswordVerifier(
    IN const LSA_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        ADCacheFreePasswordVerifier((PLSA_PASSWORD_VERIFIER)pEntry->pValue);
    }
}

void *
MemCacheBackupRoutine(
    void* pDb
    )
{
    DWORD dwError = 0;
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)pDb;
    struct timespec timeout = {0, 0};
    BOOLEAN bMutexLocked = FALSE;

    ENTER_MUTEX(&pConn->backupMutex, bMutexLocked);

    while (!pConn->bNeedShutdown)
    {
        while (!pConn->bNeedBackup && !pConn->bNeedShutdown)
        {
            dwError = pthread_cond_wait(
                            &pConn->signalBackup,
                            &pConn->backupMutex);
            BAIL_ON_LSA_ERROR(dwError);
        }
        if (!pConn->bNeedBackup)
        {
            break;
        }
        LSA_LOG_INFO("Delayed backup scheduled");

        timeout.tv_sec = time(NULL) + pConn->dwBackupDelay;
        timeout.tv_nsec = 0;
        while (!pConn->bNeedShutdown && time(NULL) < timeout.tv_sec)
        {
            dwError = pthread_cond_timedwait(
                            &pConn->signalShutdown,
                            &pConn->backupMutex,
                            &timeout);
            if (dwError == ETIMEDOUT)
            {
                dwError = 0;
            }
            BAIL_ON_LSA_ERROR(dwError);
        }

        LSA_LOG_INFO("Performing backup");
        dwError = MemCacheStoreFile((LSA_DB_HANDLE)pConn);
        BAIL_ON_LSA_ERROR(dwError);

        pConn->bNeedBackup = FALSE;
    }

cleanup:
    LEAVE_MUTEX(&pConn->backupMutex, bMutexLocked);
    return (void *)(size_t)dwError;

error:
    LSA_LOG_INFO("The in-memory backup thread is exiting with error code %d\n", dwError);
    goto cleanup;
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

    dwError = LsaAllocateString(
                    pszDbPath,
                    &pConn->pszFilename);
    BAIL_ON_LSA_ERROR(dwError);

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
                    MemCacheFreePasswordVerifier,
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

    dwError = MemCacheLoadFile((LSA_DB_HANDLE)pConn);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = pthread_mutex_init(
            &pConn->backupMutex,
            NULL);
    BAIL_ON_LSA_ERROR(dwError);
    pConn->bBackupMutexCreated = TRUE;

    pConn->dwBackupDelay = 5 * 60;

    pConn->bNeedBackup = FALSE;
    dwError = pthread_cond_init(
                    &pConn->signalBackup,
                    NULL);
    pConn->bSignalBackupCreated = TRUE;

    pConn->bNeedShutdown = FALSE;
    dwError = pthread_cond_init(
                    &pConn->signalShutdown,
                    NULL);
    pConn->bSignalShutdownCreated = TRUE;

    dwError = pthread_create(
                    &pConn->backupThread,
                    NULL,
                    MemCacheBackupRoutine,
                    pConn);
    BAIL_ON_LSA_ERROR(dwError);
    pConn->bBackupThreadCreated = TRUE;

    *phDb = (LSA_DB_HANDLE)pConn;

cleanup:
    return dwError;

error:
    MemCacheSafeClose((PLSA_DB_HANDLE)&pConn);
    *phDb = NULL;

    goto cleanup;
}

DWORD
MemCacheLoadFile(
    IN LSA_DB_HANDLE hDb
    )
{
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    LWMsgArchive* pArchive = NULL;
    LWMsgProtocol* pArchiveProtocol = NULL;
    LWMsgStatus status = 0;
    BOOLEAN bInLock = FALSE;
    DWORD dwError = 0;
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;
    PMEM_GROUP_MEMBERSHIP pMemCacheMembership = NULL;
    BOOLEAN bMutexLocked = FALSE;

    ENTER_MUTEX(&pConn->backupMutex, bMutexLocked);
    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(
                    NULL,
                    &pArchiveProtocol));
    BAIL_ON_LSA_ERROR(dwError);
    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(
                    pArchiveProtocol,
                    gMemCachePersistence));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MAP_LWMSG_ERROR(lwmsg_archive_new(
                    NULL,
                    pArchiveProtocol,
                    &pArchive));
    BAIL_ON_LSA_ERROR(dwError);
    dwError = MAP_LWMSG_ERROR(lwmsg_archive_set_file(
                    pArchive,
                    pConn->pszFilename,
                    LWMSG_ARCHIVE_READ,
                    0));
    BAIL_ON_LSA_ERROR(dwError);

    status = lwmsg_archive_open(pArchive);
    if (status == LWMSG_STATUS_FILE_NOT_FOUND)
    {
        LSA_LOG_INFO("The in-memory cache file does not exist yet");
        status = 0;
        goto cleanup;
    }
    dwError = MAP_LWMSG_ERROR(status);
    BAIL_ON_LSA_ERROR(dwError);

    while (1)
    {
        lwmsg_archive_destroy_message(pArchive, &message);
        status = lwmsg_archive_read_message(pArchive, &message);
        if (status == LWMSG_STATUS_EOF)
        {
            // There are no more messages in the file
            status = 0;
            break;
        }
        dwError = MAP_LWMSG_ERROR(status);
        BAIL_ON_LSA_ERROR(dwError);

        switch(message.tag)
        {
            case MEM_CACHE_OBJECT:
                dwError = MemCacheStoreObjectEntryInLock(
                                pConn,
                                (PLSA_SECURITY_OBJECT)message.data);
                // It is now owned by the global datastructures
                message.data = NULL;
                message.tag = -1;
                BAIL_ON_LSA_ERROR(dwError);
                break;
            case MEM_CACHE_MEMBERSHIP:
                dwError = MemCacheDuplicateMembership(
                                &pMemCacheMembership,
                                (PLSA_GROUP_MEMBERSHIP)message.data);
                BAIL_ON_LSA_ERROR(dwError);

                dwError = MemCacheAddMembership(
                                pConn,
                                pMemCacheMembership);
                BAIL_ON_LSA_ERROR(dwError);
                pMemCacheMembership = NULL;
                break;
            case MEM_CACHE_PASSWORD:
                dwError = LsaHashSetValue(
                                pConn->pSIDToPasswordVerifier,
                                ((PLSA_PASSWORD_VERIFIER)message.data)->pszObjectSid,
                                message.data);
                BAIL_ON_LSA_ERROR(dwError);
                // It is now owned by the global datastructures
                message.data = NULL;
                message.tag = -1;
                break;
        }
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_archive_close(pArchive));
    BAIL_ON_LSA_ERROR(dwError);

    pConn->bNeedBackup = TRUE;
    dwError = pthread_cond_signal(&pConn->signalBackup);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LEAVE_MUTEX(&pConn->backupMutex, bMutexLocked);
    if (pArchive)
    {
        lwmsg_archive_destroy_message(pArchive, &message);
        lwmsg_archive_delete(pArchive);
    }

    if (pArchiveProtocol)
    {
        lwmsg_protocol_delete(pArchiveProtocol);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheStoreFile(
    IN LSA_DB_HANDLE hDb
    )
{
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    DWORD dwError = 0;
    LWMsgArchive* pArchive = NULL;
    LWMsgProtocol* pArchiveProtocol = NULL;
    BOOLEAN bInLock = FALSE;
    LWMsgMessage message = LWMSG_MESSAGE_INITIALIZER;
    LSA_HASH_ITERATOR iterator = {0};
    // do not free
    LSA_HASH_ENTRY *pEntry = NULL;
    // do not free
    PMEM_LIST_NODE pGuardian = NULL;
    // do not free
    PMEM_LIST_NODE pMemPos = NULL;
    // do not free
    PDLINKEDLIST pPos = NULL;
    PSTR pszTempFile = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_new(
                    NULL,
                    &pArchiveProtocol));
    BAIL_ON_LSA_ERROR(dwError);
    dwError = MAP_LWMSG_ERROR(lwmsg_protocol_add_protocol_spec(
                    pArchiveProtocol,
                    gMemCachePersistence));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaAllocateStringPrintf(
                    &pszTempFile,
                    "%s.new",
                    pConn->pszFilename);
    BAIL_ON_LSA_ERROR(dwError);
    dwError = MAP_LWMSG_ERROR(lwmsg_archive_new(
                    NULL,
                    pArchiveProtocol,
                    &pArchive));
    BAIL_ON_LSA_ERROR(dwError);
    dwError = MAP_LWMSG_ERROR(lwmsg_archive_set_file(
                    pArchive,
                    pszTempFile,
                    LWMSG_ARCHIVE_WRITE,
                    0600));
    BAIL_ON_LSA_ERROR(dwError);
    dwError = MAP_LWMSG_ERROR(lwmsg_archive_open(
                    pArchive));
    BAIL_ON_LSA_ERROR(dwError);

    message.tag = MEM_CACHE_OBJECT;
    pPos = pConn->pObjects;
    while (pPos)
    {
        message.data = pPos->pItem;
        dwError = MAP_LWMSG_ERROR(lwmsg_archive_write_message(
                        pArchive,
                        &message));
        BAIL_ON_LSA_ERROR(dwError);

        pPos = pPos->pNext;
    }

    message.tag = MEM_CACHE_MEMBERSHIP;
    dwError = LsaHashGetIterator(
                    pConn->pParentSIDToMembershipList,
                    &iterator);
    BAIL_ON_LSA_ERROR(dwError);
    while ((pEntry = LsaHashNext(&iterator)) != NULL)
    {
        pGuardian = (PMEM_LIST_NODE) pEntry->pValue;
        pMemPos = pGuardian->pNext;
        while (pMemPos != pGuardian)
        {
            message.data = PARENT_NODE_TO_MEMBERSHIP(pMemPos);
            dwError = MAP_LWMSG_ERROR(lwmsg_archive_write_message(
                            pArchive,
                            &message));
            BAIL_ON_LSA_ERROR(dwError);

            pMemPos = pMemPos->pNext;
        }
    }

    message.tag = MEM_CACHE_PASSWORD;
    dwError = LsaHashGetIterator(
                    pConn->pSIDToPasswordVerifier,
                    &iterator);
    BAIL_ON_LSA_ERROR(dwError);
    while ((pEntry = LsaHashNext(&iterator)) != NULL)
    {
        message.data = pEntry->pValue;
        dwError = MAP_LWMSG_ERROR(lwmsg_archive_write_message(
                        pArchive,
                        &message));
        BAIL_ON_LSA_ERROR(dwError);
    }

    dwError = MAP_LWMSG_ERROR(lwmsg_archive_close(pArchive));
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaMoveFile(pszTempFile, pConn->pszFilename);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    if (pArchive)
    {
        lwmsg_archive_delete(pArchive);
    }

    if (pArchiveProtocol)
    {
        lwmsg_protocol_delete(pArchiveProtocol);
    }

    LSA_SAFE_FREE_STRING(pszTempFile);

    return dwError;

error:
    goto cleanup;
}

VOID
MemCacheFreeObjects(
    IN PVOID pData,
    IN PVOID pUnused
    )
{
    PLSA_SECURITY_OBJECT pObject = (PLSA_SECURITY_OBJECT)pData;

    ADCacheSafeFreeObject(&pObject);
}

DWORD
MemCacheRemoveMembership(
    IN PMEM_DB_CONNECTION pConn,
    IN PMEM_GROUP_MEMBERSHIP pMembership
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
    IN OUT PLSA_DB_HANDLE phDb
    )
{
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)*phDb;
    DWORD dwError = 0;
    void* pError = NULL;

    if (pConn)
    {
        if (pConn->bBackupThreadCreated)
        {
            // Notify the backup thread that it needs to shutdown. It will
            // backup one last time if necessary.
            dwError = pthread_mutex_lock(&pConn->backupMutex);
            LSA_ASSERT(dwError == 0);
            pConn->bNeedShutdown = TRUE;
            dwError = pthread_cond_signal(&pConn->signalBackup);
            LSA_ASSERT(dwError == 0);
            dwError = pthread_cond_signal(&pConn->signalShutdown);
            LSA_ASSERT(dwError == 0);
            dwError = pthread_mutex_unlock(&pConn->backupMutex);
            LSA_ASSERT(dwError == 0);

            // Wait for the thread to exit
            dwError = pthread_join(pConn->backupThread, &pError);
            LSA_ASSERT(dwError == 0);
            LSA_ASSERT(pError == NULL);
        }

        dwError = MemCacheEmptyCache(*phDb);
        LSA_ASSERT(dwError == 0);

        LsaHashSafeFree(&pConn->pDNToSecurityObject);
        LsaHashSafeFree(&pConn->pNT4ToSecurityObject);
        LsaHashSafeFree(&pConn->pSIDToSecurityObject);

        LsaHashSafeFree(&pConn->pUIDToSecurityObject);
        LsaHashSafeFree(&pConn->pUserAliasToSecurityObject);
        LsaHashSafeFree(&pConn->pUPNToSecurityObject);

        LsaHashSafeFree(&pConn->pSIDToPasswordVerifier);

        LsaHashSafeFree(&pConn->pGIDToSecurityObject);
        LsaHashSafeFree(&pConn->pGroupAliasToSecurityObject);
        LSA_SAFE_FREE_STRING(pConn->pszFilename);

        LsaHashSafeFree(&pConn->pParentSIDToMembershipList);
        LsaHashSafeFree(&pConn->pChildSIDToMembershipList);

        if (pConn->bLockCreated)
        {
            pthread_rwlock_destroy(&pConn->lock);
        }
        if (pConn->bBackupMutexCreated)
        {
            dwError = pthread_mutex_destroy(&pConn->backupMutex);
            LSA_ASSERT(dwError == 0);
        }
        if (pConn->bSignalBackupCreated)
        {
            dwError = pthread_cond_destroy(&pConn->signalBackup);
            LSA_ASSERT(dwError == 0);
        }
        if (pConn->bSignalShutdownCreated)
        {
            dwError = pthread_cond_destroy(&pConn->signalShutdown);
            LSA_ASSERT(dwError == 0);
        }

        LSA_SAFE_FREE_MEMORY(*phDb);
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
    PLSA_HASH_TABLE pIndex = NULL;
    PSTR pszKey = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

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
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
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
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pUIDToSecurityObject;

    dwError = LsaHashGetValue(
                    pIndex,
                    (PVOID)(size_t)uid,
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
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheSafeFreeObject(&pObject);
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
    PLSA_HASH_TABLE pIndex = NULL;
    PSTR pszKey = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

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
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LSA_SAFE_FREE_STRING(pszKey);

    return dwError;

error:
    *ppObject = NULL;
    ADCacheSafeFreeObject(&pObject);
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
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pGIDToSecurityObject;

    dwError = LsaHashGetValue(
                    pIndex,
                    (PVOID)(size_t)gid,
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
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

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
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    BOOLEAN bMutexLocked = FALSE;

    ENTER_MUTEX(&pConn->backupMutex, bMutexLocked);
    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    dwError = MemCacheRemoveObjectByHashKey(
                    pConn,
                    pConn->pSIDToSecurityObject,
                    pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    pConn->bNeedBackup = TRUE;
    dwError = pthread_cond_signal(&pConn->signalBackup);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LEAVE_MUTEX(&pConn->backupMutex, bMutexLocked);
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
    BOOLEAN bMutexLocked = FALSE;

    ENTER_MUTEX(&pConn->backupMutex, bMutexLocked);
    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    dwError = MemCacheRemoveObjectByHashKey(
                    pConn,
                    pConn->pSIDToSecurityObject,
                    pszSid);
    BAIL_ON_LSA_ERROR(dwError);

    pConn->bNeedBackup = TRUE;
    dwError = pthread_cond_signal(&pConn->signalBackup);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LEAVE_MUTEX(&pConn->backupMutex, bMutexLocked);
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
    LSA_HASH_ITERATOR iterator = {0};
    // Do not free
    LSA_HASH_ENTRY *pEntry = NULL;
    BOOLEAN bMutexLocked = FALSE;

    if (pConn->bBackupMutexCreated)
    {
        ENTER_MUTEX(&pConn->backupMutex, bMutexLocked);
    }

    if (pConn->bLockCreated)
    {
        ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);
    }

    if (pConn->pDNToSecurityObject)
    {
        LsaHashRemoveAll(pConn->pDNToSecurityObject);
    }
    if (pConn->pNT4ToSecurityObject)
    {
        LsaHashRemoveAll(pConn->pNT4ToSecurityObject);
    }
    if (pConn->pSIDToSecurityObject)
    {
        LsaHashRemoveAll(pConn->pSIDToSecurityObject);
    }

    if (pConn->pUIDToSecurityObject)
    {
        LsaHashRemoveAll(pConn->pUIDToSecurityObject);
    }
    if (pConn->pUserAliasToSecurityObject)
    {
        LsaHashRemoveAll(pConn->pUserAliasToSecurityObject);
    }
    if (pConn->pUPNToSecurityObject)
    {
        LsaHashRemoveAll(pConn->pUPNToSecurityObject);
    }

    if (pConn->pSIDToPasswordVerifier)
    {
        LsaHashRemoveAll(pConn->pSIDToPasswordVerifier);
    }

    if (pConn->pGIDToSecurityObject)
    {
        LsaHashRemoveAll(pConn->pGIDToSecurityObject);
    }
    if (pConn->pGroupAliasToSecurityObject)
    {
        LsaHashRemoveAll(pConn->pGroupAliasToSecurityObject);
    }

    // Remove all of the group memberships. Either table may be iterated,
    // so the parentsid list was chosen.
    dwError = LsaHashGetIterator(
                    pConn->pParentSIDToMembershipList,
                    &iterator);
    BAIL_ON_LSA_ERROR(dwError);

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
            BAIL_ON_LSA_ERROR(dwError);
        }
    }

    LSA_ASSERT(pConn->pParentSIDToMembershipList->sCount == 0);
    LSA_ASSERT(pConn->pChildSIDToMembershipList->sCount == 0);

    LsaDLinkedListForEach(
        pConn->pObjects,
        MemCacheFreeObjects,
        NULL);
    LsaDLinkedListFree(pConn->pObjects);
    pConn->pObjects = NULL;

    if (bMutexLocked)
    {
        pConn->bNeedBackup = TRUE;
        dwError = pthread_cond_signal(&pConn->signalBackup);
        BAIL_ON_LSA_ERROR(dwError);
    }

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LEAVE_MUTEX(&pConn->backupMutex, bMutexLocked);
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheRemoveObjectByHashKey(
    IN PMEM_DB_CONNECTION pConn,
    IN OUT PLSA_HASH_TABLE pTable,
    IN const void* pvKey
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
                        (PVOID)(size_t)pObject->userInfo.uid);
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
                        (PVOID)(size_t)pObject->groupInfo.gid);
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
    IN PMEM_DB_CONNECTION pConn,
    IN PLSA_SECURITY_OBJECT pObject
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
    else if (pObject->enabled && pObject->type == AccountType_Group)
    {
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
    LSA_SAFE_FREE_STRING(pszKey);
    return dwError;

error:
    goto cleanup;
}

DWORD
MemCacheEnsureHashSpace(
    IN OUT PLSA_HASH_TABLE pTable,
    IN size_t sNewEntries
    )
{
    DWORD dwError = 0;

    if ((pTable->sCount + sNewEntries) * 2 > pTable->sTableSize)
    {
        dwError = LsaHashResize(
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
    BOOLEAN bMutexLocked = FALSE;

    dwError = LsaGetCurrentTimeSeconds(&now);
    BAIL_ON_LSA_ERROR(dwError);

    ENTER_MUTEX(&pConn->backupMutex, bMutexLocked);
    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    // For simplicity, don't check whether keys exist for sure or whether the
    // objects are users or groups. Just make sure there is enough space in all
    // cases.
    dwError = MemCacheEnsureHashSpace(
                    pConn->pDNToSecurityObject,
                    sObjectCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pNT4ToSecurityObject,
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
                    pConn->pSIDToPasswordVerifier,
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
        dwError = ADCacheDuplicateObject(
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

    pConn->bNeedBackup = TRUE;
    dwError = pthread_cond_signal(&pConn->signalBackup);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_SAFE_FREE_STRING(pszKey);
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LEAVE_MUTEX(&pConn->backupMutex, bMutexLocked);

    return dwError;

error:
    ADCacheSafeFreeObject(&pObject);
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
                            (PVOID)(size_t)pObject->groupInfo.gid,
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
                            (PVOID)(size_t)pObject->userInfo.uid,
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

cleanup:
    LSA_SAFE_FREE_STRING(pszKey);

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
        ADCacheFreeGroupMembershipContents(&(*ppMembership)->membership);
        LSA_SAFE_FREE_MEMORY(*ppMembership);
    }
}

void
MemCacheFreeMembershipValue(
    IN const LSA_HASH_ENTRY* pEntry
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
    PSTR pszSidCopy = NULL;

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

        dwError = LsaStrDupOrNull(
                        pMembership->membership.pszParentSid,
                        &pszSidCopy);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaHashSetValue(
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

        dwError = LsaStrDupOrNull(
                        pMembership->membership.pszChildSid,
                        &pszSidCopy);
        BAIL_ON_LSA_ERROR(dwError);

        dwError = LsaHashSetValue(
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

    pMembership->childListNode.pNext = pGuardian->pNext;
    pMembership->childListNode.pPrev = pGuardian;

    pGuardian->pNext->pPrev = &pMembership->childListNode;
    pGuardian->pNext = &pMembership->childListNode;

cleanup:
    LSA_SAFE_FREE_MEMORY(pGuardianTemp);
    LSA_SAFE_FREE_STRING(pszSidCopy);
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
    BOOLEAN bMutexLocked = FALSE;

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

    ENTER_MUTEX(&pConn->backupMutex, bMutexLocked);
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

    pConn->bNeedBackup = TRUE;
    dwError = pthread_cond_signal(&pConn->signalBackup);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LEAVE_MUTEX(&pConn->backupMutex, bMutexLocked);
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
    BOOLEAN bMutexLocked = FALSE;

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

    ENTER_MUTEX(&pConn->backupMutex, bMutexLocked);
    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pParentSIDToMembershipList,
                    sMemberCount);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = MemCacheEnsureHashSpace(
                    pConn->pChildSIDToMembershipList,
                    sMemberCount);
    BAIL_ON_LSA_ERROR(dwError);

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

    pConn->bNeedBackup = TRUE;
    dwError = pthread_cond_signal(&pConn->signalBackup);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LEAVE_MUTEX(&pConn->backupMutex, bMutexLocked);
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

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

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
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    ADCacheSafeFreeGroupMembershipList(sCount, &ppResults);
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
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;
    DWORD dwOut = 0;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pSIDToSecurityObject;

    dwMaxNumUsers = LW_MIN(dwMaxNumUsers, pIndex->sCount);

    dwError = LsaAllocateMemory(
                    sizeof(*ppObjects) * dwMaxNumUsers,
                    (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszResume)
    {
        // Start at one after the resume SID
        dwError = LsaHashGetValue(
                        pIndex,
                        pszResume,
                        (PVOID*)&pListEntry);
        if (dwError == ENOENT)
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
        if (pObject->type == AccountType_User)
        {
            dwError = ADCacheDuplicateObject(
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
    ADCacheSafeFreeObjectList(
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
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;
    DWORD dwOut = 0;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pSIDToSecurityObject;

    dwMaxNumGroups = LW_MIN(dwMaxNumGroups, pIndex->sCount);

    dwError = LsaAllocateMemory(
                    sizeof(*ppObjects) * dwMaxNumGroups,
                    (PVOID*)&ppObjects);
    BAIL_ON_LSA_ERROR(dwError);

    if (pszResume)
    {
        // Start at one after the resume SID
        dwError = LsaHashGetValue(
                        pIndex,
                        pszResume,
                        (PVOID*)&pListEntry);
        if (dwError == ENOENT)
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
        if (pObject->type == AccountType_Group)
        {
            dwError = ADCacheDuplicateObject(
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
    ADCacheSafeFreeObjectList(
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
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

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
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

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
    PLSA_HASH_TABLE pIndex = NULL;
    // Do not free
    PDLINKEDLIST pListEntry = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

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
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

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
    PLSA_PASSWORD_VERIFIER pFromHash = NULL;
    // Do not free
    PLSA_HASH_TABLE pIndex = NULL;

    ENTER_READER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pSIDToPasswordVerifier;

    dwError = LsaHashGetValue(
                    pIndex,
                    pszUserSid,
                    (PVOID*)&pFromHash);
    if (dwError == ENOENT)
    {
        dwError = LW_ERROR_NOT_HANDLED;
    }
    BAIL_ON_LSA_ERROR(dwError);

    dwError = ADCacheDuplicatePasswordVerifier(
                    &pResult,
                    pFromHash);
    BAIL_ON_LSA_ERROR(dwError);

    *ppResult = pResult;

cleanup:
    LEAVE_RW_LOCK(&pConn->lock, bInLock);

    return dwError;

error:
    *ppResult = NULL;
    LSA_DB_SAFE_FREE_PASSWORD_VERIFIER(pResult);
    goto cleanup;
}

DWORD
MemCacheStorePasswordVerifier(
    IN LSA_DB_HANDLE hDb,
    IN PLSA_PASSWORD_VERIFIER pVerifier
    )
{
    DWORD dwError = 0;
    // Do not free
    PMEM_DB_CONNECTION pConn = (PMEM_DB_CONNECTION)hDb;
    BOOLEAN bInLock = FALSE;
    PLSA_PASSWORD_VERIFIER pCopy = NULL;
    // Do not free
    PLSA_HASH_TABLE pIndex = NULL;
    BOOLEAN bMutexLocked = FALSE;

    ENTER_MUTEX(&pConn->backupMutex, bMutexLocked);
    ENTER_WRITER_RW_LOCK(&pConn->lock, bInLock);

    pIndex = pConn->pSIDToPasswordVerifier;

    dwError = ADCacheDuplicatePasswordVerifier(
                    &pCopy,
                    pVerifier);
    BAIL_ON_LSA_ERROR(dwError);

    dwError = LsaHashSetValue(
                    pIndex,
                    pCopy->pszObjectSid,
                    pCopy);
    BAIL_ON_LSA_ERROR(dwError);
    // This is now owned by the hash
    pCopy = NULL;

    pConn->bNeedBackup = TRUE;
    dwError = pthread_cond_signal(&pConn->signalBackup);
    BAIL_ON_LSA_ERROR(dwError);

cleanup:
    LSA_DB_SAFE_FREE_PASSWORD_VERIFIER(pCopy);
    LEAVE_RW_LOCK(&pConn->lock, bInLock);
    LEAVE_MUTEX(&pConn->backupMutex, bMutexLocked);

    return dwError;

error:
    goto cleanup;
}

void
InitializeMemCacheProvider(
    OUT PADCACHE_PROVIDER_FUNCTION_TABLE pCacheTable
    )
{
    pCacheTable->pfnOpenHandle               = MemCacheOpen;
    pCacheTable->pfnSafeClose                = MemCacheSafeClose;
    pCacheTable->pfnFlushToDisk              = MemCacheStoreFile;
    pCacheTable->pfnFindUserByName           = MemCacheFindUserByName;
    pCacheTable->pfnFindUserById             = MemCacheFindUserById;
    pCacheTable->pfnFindGroupByName          = MemCacheFindGroupByName;
    pCacheTable->pfnFindGroupById            = MemCacheFindGroupById;
    pCacheTable->pfnRemoveUserBySid          = MemCacheRemoveUserBySid;
    pCacheTable->pfnRemoveGroupBySid         = MemCacheRemoveGroupBySid;
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
    pCacheTable->pfnGetPasswordVerifier      = MemCacheGetPasswordVerifier;
    pCacheTable->pfnStorePasswordVerifier    = MemCacheStorePasswordVerifier;
}
