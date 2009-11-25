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
 *        sqlitecache.c
 *
 * Abstract:
 *
 *        Registry Sqlite backend caching layer APIs
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewise.com)
 */
#include "includes.h"


/*Find whether this key has already been in active Key list,
 * If not, create key context and add the active key to the list
 * Otherwise, increment the existing active key reference count by 1
 */
PREG_KEY_CONTEXT
SqliteCacheLocateActiveKey(
    IN PCSTR pszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = NULL;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pKeyResult = SqliteCacheLocateActiveKey_inlock(pszKeyName);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return pKeyResult;
}

PREG_KEY_CONTEXT
SqliteCacheLocateActiveKey_inlock(
    IN PCSTR pszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = NULL;
    DWORD status = 0;

    status = RegHashGetValue(gActiveKeyList.pKeyList,
                              pszKeyName,
                              (PVOID*)&pKeyResult);
    if (!status && pKeyResult)
    {
        LwInterlockedIncrement(&pKeyResult->refCount);
    }

    return pKeyResult;
}

NTSTATUS
SqliteCacheInsertActiveKey(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    status = SqliteCacheInsertActiveKey_inlock(pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheInsertActiveKey_inlock(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;

    status = RegHashSetValue(gActiveKeyList.pKeyList,
                              (PVOID)pKeyResult->pszKeyName,
                              (PVOID)pKeyResult);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:
    goto cleanup;
}

VOID
SqliteCacheDeleteActiveKey(
    IN PSTR pszKeyName
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    SqliteCacheDeleteActiveKey_inlock(pszKeyName);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return;
}

VOID
SqliteCacheDeleteActiveKey_inlock(
    IN PSTR pszKeyName
    )
{
    REG_HASH_ITERATOR hashIterator;
    REG_HASH_ENTRY* pHashEntry = NULL;
    int iCount = 0;
    DWORD status = 0;

    status = RegHashGetValue(gActiveKeyList.pKeyList,
                              pszKeyName,
                              NULL);
    if (ENOENT == LwNtStatusToErrno(status))
    {
        return;
    }

    RegHashGetIterator(gActiveKeyList.pKeyList, &hashIterator);

    for (iCount = 0; (pHashEntry = RegHashNext(&hashIterator)) != NULL; iCount++)
    {
        if (!strcasecmp((PCSTR)pHashEntry->pKey, pszKeyName))
        {
            RegHashRemoveKey(gActiveKeyList.pKeyList, pHashEntry->pKey);

            break;
        }
    }

    return;
}

void
SqliteCacheResetParentKeySubKeyInfo(
    IN PSTR pszParentKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = SqliteCacheLocateActiveKey(pszParentKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        RegSrvResetSubKeyInfo(pKeyResult);
    }

    SqliteCacheReleaseKey(pKeyResult);

    return;
}

void
SqliteCacheResetParentKeySubKeyInfo_inlock(
    IN PSTR pszParentKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = SqliteCacheLocateActiveKey_inlock(pszParentKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        RegSrvResetSubKeyInfo(pKeyResult);
    }

    SqliteCacheReleaseKey_inlock(pKeyResult);

    return;
}

void
SqliteCacheResetKeyValueInfo(
    IN PSTR pszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = SqliteCacheLocateActiveKey(pszKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        RegSrvResetValueInfo(pKeyResult);
    }

    SqliteCacheReleaseKey(pKeyResult);

    return;
}

void
SqliteCacheResetKeyValueInfo_inlock(
    IN PSTR pszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = SqliteCacheLocateActiveKey_inlock(pszKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        RegSrvResetValueInfo(pKeyResult);
    }

    SqliteCacheReleaseKey_inlock(pKeyResult);

    return;
}

VOID
SqliteCacheReleaseKey(
    PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    SqliteCacheReleaseKey_inlock(pKeyResult);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);
}

VOID
SqliteCacheReleaseKey_inlock(
    PREG_KEY_CONTEXT pKeyResult
    )
{
    if (pKeyResult && InterlockedDecrement(&pKeyResult->refCount) == 0)
    {
        SqliteCacheDeleteActiveKey_inlock(pKeyResult->pszKeyName);
    }
}

void
SqliteCacheFreeHashEntry(
    IN const REG_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        RegSrvSafeFreeKeyContext((PREG_KEY_CONTEXT)pEntry->pValue);
    }
}

// sqlite caching helper functions supporting paging
NTSTATUS
SqliteCacheSubKeysInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sNumSubKeys = 0;
    size_t sNumCacheSubKeys = 0;
    PREG_ENTRY* ppRegEntries = NULL;

    if (bDoAnsi ? pKeyResult->bHasSubKeyAInfo : pKeyResult->bHasSubKeyInfo)
    {
        goto cleanup;
    }

    status = RegDbQueryInfoKeyCount(ghCacheConnection,
                                     pKeyResult->pszKeyName,
                                     QuerySubKeys,
                                     &sNumSubKeys);
    BAIL_ON_NT_STATUS(status);

    sNumCacheSubKeys = (sNumSubKeys > (size_t)dwDefaultCacheSize)
                      ? dwDefaultCacheSize
                      : sNumSubKeys;

    status = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QuerySubKeys,
                                sNumCacheSubKeys,
                                0,
                                &sNumCacheSubKeys,
                                &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    status = RegDbSafeRecordSubKeysInfo_inlock(
                            sNumSubKeys,
                            sNumCacheSubKeys,
                            ppRegEntries,
                            pKeyResult,
                            bDoAnsi);
    BAIL_ON_NT_STATUS(status);

cleanup:
    RegDbSafeFreeEntryList(sNumCacheSubKeys,&ppRegEntries);
    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheSubKeysInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheSubKeysInfo_inlock(pKeyResult, bDoAnsi);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateSubKeysInfo_inlock(
    IN DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi,
    OUT size_t* psNumSubKeys
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sNumSubKeys = 0;
    PREG_ENTRY* ppRegEntries = NULL;
    int iCount = 0;
    PWSTR pSubKey = NULL;
    size_t sSubKeyLen = 0;

    status = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QuerySubKeys,
                                dwDefaultCacheSize,
                                dwOffSet,
                                &sNumSubKeys,
                                &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (int)sNumSubKeys; iCount++)
    {
        if (bDoAnsi)
        {
            sSubKeyLen = strlen(ppRegEntries[iCount]->pszKeyName);

            if (pKeyResult->sMaxSubKeyALen < sSubKeyLen)
                pKeyResult->sMaxSubKeyALen = sSubKeyLen;
        }
        else
        {
		status = LwRtlWC16StringAllocateFromCString(&pSubKey,
				                                     ppRegEntries[iCount]->pszKeyName);
		BAIL_ON_NT_STATUS(status);

            if (pSubKey)
            {
		sSubKeyLen = RtlWC16StringNumChars(pSubKey);
            }

            if (pKeyResult->sMaxSubKeyLen < sSubKeyLen)
                pKeyResult->sMaxSubKeyLen = sSubKeyLen;
        }

        LWREG_SAFE_FREE_MEMORY(pSubKey);
        sSubKeyLen = 0;
    }

    pKeyResult->dwNumSubKeys += sNumSubKeys;

cleanup:
    *psNumSubKeys = sNumSubKeys;

    RegDbSafeFreeEntryList(sNumSubKeys, &ppRegEntries);
    LWREG_SAFE_FREE_MEMORY(pSubKey);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateSubKeysInfo(
    IN DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi,
    OUT size_t* psNumSubKeys
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheUpdateSubKeysInfo_inlock(dwOffSet,
                                             pKeyResult,
                                             bDoAnsi,
                                             psNumSubKeys);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheKeyValuesInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sNumValues = 0;
    size_t sNumCacheValues = 0;
    PREG_ENTRY* ppRegEntries = NULL;

    if (bDoAnsi ? pKeyResult->bHasValueAInfo : pKeyResult->bHasValueInfo)
    {
        goto cleanup;
    }

    status = RegDbQueryInfoKeyCount(ghCacheConnection,
                                     pKeyResult->pszKeyName,
                                     QueryValues,
                                     &sNumValues);
    BAIL_ON_NT_STATUS(status);

    sNumCacheValues = (sNumValues > (size_t)dwDefaultCacheSize)
                      ? dwDefaultCacheSize
                      : sNumValues;

    status = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QueryValues,
                                sNumCacheValues,
                                0,
                                &sNumCacheValues,
                                &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    status = RegDbSafeRecordValuesInfo_inlock(
                            sNumValues,
                            sNumCacheValues,
                            ppRegEntries,
                            pKeyResult,
                            bDoAnsi);
    BAIL_ON_NT_STATUS(status);

cleanup:
    RegDbSafeFreeEntryList(sNumCacheValues, &ppRegEntries);
    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheKeyValuesInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheKeyValuesInfo_inlock(pKeyResult, bDoAnsi);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateValuesInfo_inlock(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi,
    OUT size_t* psNumValues
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    size_t sNumValues = 0;
    PREG_ENTRY* ppRegEntries = NULL;
    int iCount = 0;
    PWSTR pValueName = NULL;
    size_t sValueNameLen = 0;
    DWORD dwValueLen = 0;

    status = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QueryValues,
                                dwDefaultCacheSize,
                                dwOffSet,
                                &sNumValues,
                                &ppRegEntries);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < (int)sNumValues; iCount++)
    {
        if (bDoAnsi)
        {
            sValueNameLen = strlen(ppRegEntries[iCount]->pszValueName);

            if (pKeyResult->sMaxValueNameALen < sValueNameLen)
                pKeyResult->sMaxValueNameALen = sValueNameLen;
        }
        else
        {
		status = LwRtlWC16StringAllocateFromCString(&pValueName,
				                                     ppRegEntries[iCount]->pszValueName);
		BAIL_ON_NT_STATUS(status);

            if (pValueName)
            {
		sValueNameLen = RtlWC16StringNumChars(pValueName);
            }

            if (pKeyResult->sMaxValueNameLen < sValueNameLen)
                pKeyResult->sMaxValueNameLen = sValueNameLen;
        }

        status = RegGetValueAsBytes(ppRegEntries[iCount]->type,
                                  (PCSTR)ppRegEntries[iCount]->pszValue,
                                  bDoAnsi,
                                  NULL,
                                  &dwValueLen);
        BAIL_ON_NT_STATUS(status);

        if (bDoAnsi)
        {
            if (pKeyResult->sMaxValueALen < (size_t)dwValueLen)
                pKeyResult->sMaxValueALen = (size_t)dwValueLen;
        }
        else
        {
            if (pKeyResult->sMaxValueLen < (size_t)dwValueLen)
                pKeyResult->sMaxValueLen = (size_t)dwValueLen;
        }

        LWREG_SAFE_FREE_MEMORY(pValueName);
        sValueNameLen = 0;
        dwValueLen = 0;
    }

    pKeyResult->dwNumValues += sNumValues;

cleanup:
    *psNumValues = sNumValues;
    RegDbSafeFreeEntryList(sNumValues, &ppRegEntries);
    LWREG_SAFE_FREE_MEMORY(pValueName);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteCacheUpdateValuesInfo(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi,
    OUT size_t* psNumValues
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_NT_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheUpdateValuesInfo_inlock(dwOffSet,
                                     pKeyResult,
                                     bDoAnsi,
                                     psNumValues);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return status;

error:
    goto cleanup;
}
