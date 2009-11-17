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
 *        sqliteapi.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Inter-process communication (Server) API for Users
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "includes.h"

DWORD
SqliteGetParentKeyName(
    PCSTR pszInputString,
    CHAR c,
    PSTR *ppszOutputString
    )
{
    DWORD dwError = 0;
    //Do not free
    PSTR pszFound = NULL;
    PSTR pszOutputString = NULL;

    BAIL_ON_INVALID_STRING(pszInputString);

    pszFound = strrchr(pszInputString, c);
    if (pszFound)
    {
        dwError = LwAllocateMemory(sizeof(*pszOutputString)*(pszFound - pszInputString +1),
                                   (PVOID)&pszOutputString);
        BAIL_ON_REG_ERROR(dwError);

        strncpy(pszOutputString,pszInputString,(pszFound - pszInputString));
    }

    *ppszOutputString = pszOutputString;

cleanup:

    return dwError;

error:
    LW_SAFE_FREE_STRING(pszOutputString);

    goto cleanup;
}

DWORD
SqliteCreateKeyHandle(
    IN PREG_ENTRY pRegEntry,
    OUT PHKEY phkResult
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PREG_KEY_CONTEXT pKeyResult = NULL;

    BAIL_ON_INVALID_REG_ENTRY(pRegEntry);

    dwError = LwAllocateMemory(sizeof(*pKeyResult), (PVOID*)&pKeyResult);
    BAIL_ON_REG_ERROR(dwError);

    pKeyResult->refCount = 1;

    pthread_rwlock_init(&pKeyResult->mutex, NULL);
    pKeyResult->pMutex = &pKeyResult->mutex;

    dwError = LwAllocateString(pRegEntry->pszKeyName, &pKeyResult->pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = SqliteGetParentKeyName(pKeyResult->pszKeyName, '\\',&pKeyResult->pszParentKeyName);
    BAIL_ON_REG_ERROR(dwError);

    *phkResult = (HKEY)pKeyResult;

cleanup:
    return dwError;

error:
    RegSrvSafeFreeKeyContext(pKeyResult);
    *phkResult = (HKEY)NULL;

    goto cleanup;
}

/* Create a new key, if the key exists already,
 * open the existing key
 */
DWORD
SqliteCreateKeyInternal(
    IN PSTR pszKeyName,
    IN OPTIONAL PCWSTR pSubKey, //pSubKey is null only when creating HKEY_LIKEWISE
    OUT OPTIONAL PHKEY ppKeyResult
    )
{
    DWORD dwError = 0;
    PREG_ENTRY pRegEntry = NULL;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    PSTR pszSubKey = NULL;
    PSTR pszKeyNameWithSubKey = NULL;
    PSTR pszParentKeyName = NULL;
    // Do not free
    PSTR pszFullKeyName = NULL;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_STRING(pszKeyName); //parent key name

    if (pSubKey)
    {
        dwError = LwWc16sToMbs(pSubKey,&pszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        if (!RegSrvIsValidKeyName(pszSubKey))
        {
            dwError = LW_ERROR_INVALID_KEYNAME;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LwAllocateStringPrintf(
                    &pszKeyNameWithSubKey,
                    "%s\\%s",
                    pszKeyName,
                    pszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    pszFullKeyName = pSubKey ? pszKeyNameWithSubKey : pszKeyName;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pKeyResult = RegSrvLocateActiveKey_inlock(pszFullKeyName);
    if (!pKeyResult)
    {
        dwError = RegDbOpenKey(ghCacheConnection,
                               pszFullKeyName,
                               &pRegEntry);
        if (LW_ERROR_NO_SUCH_KEY == dwError)
        {
            dwError = RegDbCreateKey(ghCacheConnection,
                                     pszFullKeyName,
                                     &pRegEntry);
            BAIL_ON_REG_ERROR(dwError);

            dwError = SqliteGetParentKeyName(pszFullKeyName, '\\',&pszParentKeyName);
            BAIL_ON_REG_ERROR(dwError);

            if (pszParentKeyName)
            {
                RegSrvResetParentKeySubKeyInfo_inlock(pszParentKeyName);
            }
        }
        BAIL_ON_REG_ERROR(dwError);

        dwError = SqliteCreateKeyHandle(pRegEntry, (PHKEY)&pKeyResult);
        BAIL_ON_REG_ERROR(dwError);

        // Cache this new key in gActiveKeyList
        dwError = RegSrvInsertActiveKey_inlock(pKeyResult);
        BAIL_ON_REG_ERROR(dwError);
    }

    if (ppKeyResult)
    {
        *ppKeyResult = pKeyResult;
        pKeyResult = NULL;
    }
    else
    {
        RegSrvReleaseKey_inlock(pKeyResult);
    }

cleanup:
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    LW_SAFE_FREE_STRING(pszSubKey);
    LW_SAFE_FREE_STRING(pszKeyNameWithSubKey);
    LW_SAFE_FREE_STRING(pszParentKeyName);
    RegCacheSafeFreeEntry(&pRegEntry);

    return dwError;

error:
    RegSrvReleaseKey_inlock(pKeyResult);

    if (ppKeyResult)
    {
        *ppKeyResult = NULL;
    }

    goto cleanup;
}

/* Open a key, if not found,
 * do not create a new key */
DWORD
SqliteOpenKeyInternal(
    IN PSTR pszKeyName,
    IN OPTIONAL PCWSTR pSubKey,
    OUT PHKEY phkResult
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PREG_ENTRY pRegEntry = NULL;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    PSTR pszSubKey = NULL;
    extern REG_SRV_API_KEYLOOKUP gActiveKeyList;
    PSTR pszKeyNameWithSubKey = NULL;
    // Do not free
    PSTR pszFullKeyName = NULL;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_STRING(pszKeyName);

    if (pSubKey)
    {
        dwError = LwWc16sToMbs(pSubKey,&pszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwAllocateStringPrintf(
                    &pszKeyNameWithSubKey,
                    "%s\\%s",
                    pszKeyName,
                    pszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    pszFullKeyName = pSubKey ? pszKeyNameWithSubKey : pszKeyName;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pKeyResult = RegSrvLocateActiveKey_inlock(pszFullKeyName);
    if (!pKeyResult)
    {
        dwError = RegDbOpenKey(ghCacheConnection, pszFullKeyName,&pRegEntry);
        BAIL_ON_REG_ERROR(dwError);

        dwError = SqliteCreateKeyHandle(pRegEntry,(PHKEY)&pKeyResult);
        BAIL_ON_REG_ERROR(dwError);

        // Cache this new key in gActiveKeyList
        dwError = RegSrvInsertActiveKey_inlock(pKeyResult);
        BAIL_ON_REG_ERROR(dwError);
    }

    *phkResult = (HKEY)pKeyResult;

cleanup:
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);
    RegCacheSafeFreeEntry(&pRegEntry);

    return dwError;

error:
    RegSrvReleaseKey_inlock(pKeyResult);
    pKeyResult = (HKEY)NULL;

    goto cleanup;
}

DWORD
SqliteDbDeleteKeyInternal(
    IN PSTR pszKeyName
    )
{
    DWORD dwError = 0;
    size_t sSubkeyCount = 0;
    PSTR pszParentKeyName = NULL;

    dwError = RegDbOpenKey(ghCacheConnection, pszKeyName, NULL);
    BAIL_ON_REG_ERROR(dwError);

    //Delete key from DB
    //Make sure this key does not have subkey before go ahead and delete it
    //Also need to delete the all of this subkey's values
    dwError = RegDbQueryInfoKeyCount(ghCacheConnection,
                                     pszKeyName,
                                     QuerySubKeys,
                                     &sSubkeyCount);
    BAIL_ON_REG_ERROR(dwError);

    if (sSubkeyCount == 0)
    {
        //Delete all the values of this key
        dwError = RegDbDeleteKey(ghCacheConnection, pszKeyName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = SqliteGetParentKeyName(pszKeyName, '\\',&pszParentKeyName);
        BAIL_ON_REG_ERROR(dwError);

        if (!LW_IS_NULL_OR_EMPTY_STR(pszParentKeyName))
        {
            RegSrvResetParentKeySubKeyInfo(pszParentKeyName);
        }
    }
    else
    {
        dwError = LW_ERROR_FAILED_DELETE_HAS_SUBKEY;
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszParentKeyName);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteDeleteActiveKey(
    IN PSTR pszKeyName
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pFoundKey = NULL;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pFoundKey = RegSrvLocateActiveKey_inlock(pszKeyName);
    if (pFoundKey)
    {
        dwError = LW_ERROR_KEY_IS_ACTIVE;
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    RegSrvReleaseKey_inlock(pFoundKey);
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return dwError;

error:
    goto cleanup;
}


/*Find whether this key has already been in active Key list,
 * If not, create key context and add the active key to the list
 * Otherwise, increment the existing active key reference count by 1
 */
PREG_KEY_CONTEXT
RegSrvLocateActiveKey(
    IN PCSTR pszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = NULL;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    pKeyResult = RegSrvLocateActiveKey_inlock(pszKeyName);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return pKeyResult;
}

PREG_KEY_CONTEXT
RegSrvLocateActiveKey_inlock(
    IN PCSTR pszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = NULL;
    DWORD dwError = 0;

    dwError = RegHashGetValue(gActiveKeyList.pKeyList,
                              pszKeyName,
                              (PVOID*)&pKeyResult);
    if (!dwError && pKeyResult)
    {
        LwInterlockedIncrement(&pKeyResult->refCount);
    }

    return pKeyResult;
}

DWORD
RegSrvInsertActiveKey(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    DWORD dwError = 0;
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    dwError = RegSrvInsertActiveKey_inlock(pKeyResult);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return dwError;

error:
    goto cleanup;
}

DWORD
RegSrvInsertActiveKey_inlock(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    DWORD dwError = 0;

    dwError = RegHashSetValue(gActiveKeyList.pKeyList,
                              (PVOID)pKeyResult->pszKeyName,
                              (PVOID)pKeyResult);
    BAIL_ON_REG_ERROR(dwError);

cleanup:

    return dwError;

error:
    goto cleanup;
}

VOID
RegSrvDeleteActiveKey(
    IN PSTR pszKeyName
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    RegSrvDeleteActiveKey_inlock(pszKeyName);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    return;
}

VOID
RegSrvDeleteActiveKey_inlock(
    IN PSTR pszKeyName
    )
{
    REG_HASH_ITERATOR hashIterator;
    REG_HASH_ENTRY* pHashEntry = NULL;
    int iCount = 0;
    DWORD dwError = 0;

    dwError = RegHashGetValue(gActiveKeyList.pKeyList,
                              pszKeyName,
                              NULL);
    if (ENOENT == dwError)
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
RegSrvResetParentKeySubKeyInfo(
    IN PSTR pszParentKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = RegSrvLocateActiveKey(pszParentKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        RegSrvResetSubKeyInfo(pKeyResult);
    }

    RegSrvReleaseKey(pKeyResult);

    return;
}

void
RegSrvResetParentKeySubKeyInfo_inlock(
    IN PSTR pszParentKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = RegSrvLocateActiveKey_inlock(pszParentKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        RegSrvResetSubKeyInfo(pKeyResult);
    }

    RegSrvReleaseKey_inlock(pKeyResult);

    return;
}

void
RegSrvResetKeyValueInfo(
    IN PSTR pszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = RegSrvLocateActiveKey(pszKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        RegSrvResetValueInfo(pKeyResult);
    }

    RegSrvReleaseKey(pKeyResult);

    return;
}

void
RegSrvResetKeyValueInfo_inlock(
    IN PSTR pszKeyName
    )
{
    PREG_KEY_CONTEXT pKeyResult = RegSrvLocateActiveKey_inlock(pszKeyName);

    if (pKeyResult)
    {
        // Todo: we can reflect activeKey to have the most current information,
        // For now, set the bHasSubKeyInfo to false to force it grab information from db
        RegSrvResetValueInfo(pKeyResult);
    }

    RegSrvReleaseKey_inlock(pKeyResult);

    return;
}

VOID
RegSrvReleaseKey(
    PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_MUTEX(bInLock, &gActiveKeyList.mutex);

    RegSrvReleaseKey_inlock(pKeyResult);

    LWREG_UNLOCK_MUTEX(bInLock, &gActiveKeyList.mutex);
}

VOID
RegSrvReleaseKey_inlock(
    PREG_KEY_CONTEXT pKeyResult
    )
{
    if (pKeyResult && InterlockedDecrement(&pKeyResult->refCount) == 0)
    {
        RegSrvDeleteActiveKey_inlock(pKeyResult->pszKeyName);
    }
}

void
RegSrvFreeHashEntry(
    IN const REG_HASH_ENTRY* pEntry
    )
{
    if (pEntry->pValue)
    {
        RegSrvSafeFreeKeyContext((PREG_KEY_CONTEXT)pEntry->pValue);
    }
}

DWORD
GetValueAsBytes(
    IN REG_DATA_TYPE type,
    IN PCSTR pszValue,
    IN BOOLEAN bDoAnsi,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD   dwError = 0;
    PBYTE   pTempData = NULL;
    PBYTE   pTempData1 = NULL;
    DWORD   dwValue = 0;
    DWORD   cbData = 0;
    DWORD   cbData1 = 0;
    PWSTR   pwcValue = NULL;
    PWSTR*  ppwszOutMultiSz = NULL;
    PBYTE   pOutData = NULL;

    if (pData && !pcbData)
    {
	dwError = LW_ERROR_INVALID_PARAMETER;
	BAIL_ON_REG_ERROR(dwError);
    }

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
	{
        goto done;
    }

    switch (type)
    {
        case REG_BINARY:
            dwError = LwHexStrToByteArray(
                           pszValue,
                           NULL,
                           &pTempData,
                           &cbData);
            BAIL_ON_REG_ERROR(dwError);

            if(pData && cbData > *pcbData)
            {
                dwError = LW_ERROR_INSUFFICIENT_BUFFER;
                BAIL_ON_REG_ERROR(dwError);
            }

            if (pData)
            {
                memcpy(pData, pTempData, cbData);
            }

            break;

        case REG_MULTI_SZ:
            dwError = LwHexStrToByteArray(
                           pszValue,
                           NULL,
                           &pTempData1,
                           &cbData1);
            BAIL_ON_REG_ERROR(dwError);

            if (bDoAnsi)
            {
                dwError = RegConvertByteStreamW2A(
                                pTempData1,
                                cbData1,
                                &pTempData,
                                &cbData);
                BAIL_ON_REG_ERROR(dwError);

                if (pData && cbData > *pcbData)
                {
                    dwError = LW_ERROR_INSUFFICIENT_BUFFER;
                    BAIL_ON_REG_ERROR(dwError);
                }

                if (pData)
                {
                    memcpy(pData, pTempData, cbData);
                }
            }
            else
            {
                if(pData && (DWORD)cbData1 > *pcbData)
                {
                    dwError = LW_ERROR_INSUFFICIENT_BUFFER;
                    BAIL_ON_REG_ERROR(dwError);
                }

                if (pData)
                {
                    memcpy(pData, pTempData1, cbData1);
                }

                cbData = cbData1;
            }

            break;

        case REG_SZ:

            dwError = LwHexStrToByteArray(
                           pszValue,
                           NULL,
                           &pTempData1,
                           &cbData1);
            BAIL_ON_REG_ERROR(dwError);

            if (bDoAnsi)
            {
		dwError = LwWc16sToMbs((PCWSTR)pTempData1, (PSTR*)&pTempData);
                BAIL_ON_REG_ERROR(dwError);

		if(pData && strlen((PCSTR)pTempData) > *pcbData)
                {
                    dwError = LW_ERROR_INSUFFICIENT_BUFFER;
                    BAIL_ON_REG_ERROR(dwError);
                }
                //value data length needs including NULL-terminator
                cbData = strlen((PCSTR)pTempData)+1;

                if (pData)
                {
                    memcpy(pData, pTempData, cbData);
                }
            }
            else
            {
                if(pData && (DWORD)cbData1 > *pcbData)
                {
                    dwError = LW_ERROR_INSUFFICIENT_BUFFER;
                    BAIL_ON_REG_ERROR(dwError);
                }

                if (pData)
                {
                    memcpy(pData, pTempData1, cbData1);
                }

                cbData = cbData1;
            }

            break;

        case REG_DWORD:

            if (pData &&  sizeof(dwValue) > *pcbData)
            {
                dwError = LW_ERROR_INSUFFICIENT_BUFFER;
                BAIL_ON_REG_ERROR(dwError);
            }

            dwValue = (DWORD)atoi(pszValue);

            cbData = sizeof(dwValue);

            if (pData)
            {
                memcpy(pData, &dwValue, cbData);
            }

            break;

        default:
            dwError = LW_ERROR_UNKNOWN_DATA_TYPE;
            BAIL_ON_REG_ERROR(dwError);
    }

done:

    if (pcbData)
    {
        *pcbData = cbData;
    }

cleanup:

    LW_SAFE_FREE_MEMORY(pTempData);
    LW_SAFE_FREE_MEMORY(pTempData1);
    LW_SAFE_FREE_MEMORY(pOutData);
    LW_SAFE_FREE_MEMORY(pwcValue);
    if (ppwszOutMultiSz)
    {
        RegFreeMultiStrsW(ppwszOutMultiSz);
    }

    return dwError;

error:

    if (pcbData)
    {
        *pcbData = 0;
    }

    goto cleanup;
}

// sqlite caching helper functions
DWORD
SqliteCacheSubKeysInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
    DWORD dwError = 0;
    size_t sNumSubKeys = 0;
    size_t sNumCacheSubKeys = 0;
    PREG_ENTRY* ppRegEntries = NULL;

    if (bDoAnsi ? pKeyResult->bHasSubKeyAInfo : pKeyResult->bHasSubKeyInfo)
    {
        goto cleanup;
    }

    dwError = RegDbQueryInfoKeyCount(ghCacheConnection,
                                     pKeyResult->pszKeyName,
                                     QuerySubKeys,
                                     &sNumSubKeys);
    BAIL_ON_REG_ERROR(dwError);

    sNumCacheSubKeys = (sNumSubKeys > (size_t)dwDefaultCacheSize)
                      ? dwDefaultCacheSize
                      : sNumSubKeys;

    dwError = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QuerySubKeys,
                                sNumCacheSubKeys,
                                0,
                                &sNumCacheSubKeys,
                                &ppRegEntries);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCacheSafeRecordSubKeysInfo_inlock(
                            sNumSubKeys,
                            sNumCacheSubKeys,
                            ppRegEntries,
                            pKeyResult,
                            bDoAnsi);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    RegCacheSafeFreeEntryList(sNumCacheSubKeys,&ppRegEntries);
    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteCacheSubKeysInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    dwError = SqliteCacheSubKeysInfo_inlock(pKeyResult, bDoAnsi);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteUpdateSubKeysInfo_inlock(
    IN DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi,
    OUT size_t* psNumSubKeys
    )
{
    DWORD dwError = 0;
    size_t sNumSubKeys = 0;
    PREG_ENTRY* ppRegEntries = NULL;
    int iCount = 0;
    PWSTR pSubKey = NULL;
    size_t sSubKeyLen = 0;

    dwError = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QuerySubKeys,
                                dwDefaultCacheSize,
                                dwOffSet,
                                &sNumSubKeys,
                                &ppRegEntries);
    BAIL_ON_REG_ERROR(dwError);

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
            dwError = LwMbsToWc16s(ppRegEntries[iCount]->pszKeyName, &pSubKey);
            BAIL_ON_REG_ERROR(dwError);

            dwError = LwWc16sLen((PCWSTR)pSubKey,&sSubKeyLen);
            BAIL_ON_REG_ERROR(dwError);

            if (pKeyResult->sMaxSubKeyLen < sSubKeyLen)
                pKeyResult->sMaxSubKeyLen = sSubKeyLen;
        }

        LW_SAFE_FREE_MEMORY(pSubKey);
        sSubKeyLen = 0;
    }

    pKeyResult->dwNumSubKeys += sNumSubKeys;

cleanup:
    *psNumSubKeys = sNumSubKeys;

    RegCacheSafeFreeEntryList(sNumSubKeys, &ppRegEntries);
    LW_SAFE_FREE_MEMORY(pSubKey);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteUpdateSubKeysInfo(
    IN DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi,
    OUT size_t* psNumSubKeys
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    dwError = SqliteUpdateSubKeysInfo_inlock(dwOffSet,
                                             pKeyResult,
                                             bDoAnsi,
                                             psNumSubKeys);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteCacheKeyValuesInfo_inlock(
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
    DWORD dwError = 0;
    size_t sNumValues = 0;
    size_t sNumCacheValues = 0;
    PREG_ENTRY* ppRegEntries = NULL;

    if (bDoAnsi ? pKeyResult->bHasValueAInfo : pKeyResult->bHasValueInfo)
    {
        goto cleanup;
    }

    dwError = RegDbQueryInfoKeyCount(ghCacheConnection,
                                     pKeyResult->pszKeyName,
                                     QueryValues,
                                     &sNumValues);
    BAIL_ON_REG_ERROR(dwError);

    sNumCacheValues = (sNumValues > (size_t)dwDefaultCacheSize)
                      ? dwDefaultCacheSize
                      : sNumValues;

    dwError = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QueryValues,
                                sNumCacheValues,
                                0,
                                &sNumCacheValues,
                                &ppRegEntries);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegCacheSafeRecordValuesInfo_inlock(
                            sNumValues,
                            sNumCacheValues,
                            ppRegEntries,
                            pKeyResult,
                            bDoAnsi);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    RegCacheSafeFreeEntryList(sNumCacheValues, &ppRegEntries);
    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteCacheKeyValuesInfo(
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    dwError = SqliteCacheKeyValuesInfo_inlock(pKeyResult, bDoAnsi);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteUpdateValuesInfo_inlock(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi,
    OUT size_t* psNumValues
    )
{
    DWORD dwError = 0;
    size_t sNumValues = 0;
    PREG_ENTRY* ppRegEntries = NULL;
    int iCount = 0;
    PWSTR pValueName = NULL;
    size_t sValueNameLen = 0;
    DWORD dwValueLen = 0;

    dwError = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QueryValues,
                                dwDefaultCacheSize,
                                dwOffSet,
                                &sNumValues,
                                &ppRegEntries);
    BAIL_ON_REG_ERROR(dwError);

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
            dwError = LwMbsToWc16s(ppRegEntries[iCount]->pszValueName, &pValueName);
            BAIL_ON_REG_ERROR(dwError);

            dwError = LwWc16sLen((PCWSTR)pValueName,&sValueNameLen);
            BAIL_ON_REG_ERROR(dwError);

            if (pKeyResult->sMaxValueNameLen < sValueNameLen)
                pKeyResult->sMaxValueNameLen = sValueNameLen;
        }

        dwError = GetValueAsBytes(ppRegEntries[iCount]->type,
                                  (PCSTR)ppRegEntries[iCount]->pszValue,
                                  bDoAnsi,
                                  NULL,
                                  &dwValueLen);
        BAIL_ON_REG_ERROR(dwError);

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

        LW_SAFE_FREE_MEMORY(pValueName);
        sValueNameLen = 0;
        dwValueLen = 0;
    }

    pKeyResult->dwNumValues += sNumValues;

cleanup:
    *psNumValues = sNumValues;
    RegCacheSafeFreeEntryList(sNumValues, &ppRegEntries);
    LW_SAFE_FREE_MEMORY(pValueName);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteUpdateValuesInfo(
    DWORD dwOffSet,
    IN OUT PREG_KEY_CONTEXT pKeyResult,
    IN BOOLEAN bDoAnsi,
    OUT size_t* psNumValues
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    BOOLEAN bInLock = FALSE;

    BAIL_ON_INVALID_POINTER(pKeyResult);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    dwError = SqliteUpdateValuesInfo_inlock(dwOffSet,
                                     pKeyResult,
                                     bDoAnsi,
                                     psNumValues);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwError;

error:
    goto cleanup;
}
