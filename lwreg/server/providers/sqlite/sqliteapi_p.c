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
    RegSafeFreeKeyContext(&pKeyResult);
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
        *ppKeyResult = (PHKEY)pKeyResult;
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
SqliteFillInSubKeysInfo(
    PREG_KEY_CONTEXT pKeyResult
    )
{
    DWORD dwError = 0;
    size_t sNumSubKeys = 0;
    PREG_ENTRY* ppRegEntries = NULL;

    dwError = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QuerySubKeys,
                                &sNumSubKeys,
                                &ppRegEntries);
    BAIL_ON_REG_ERROR(dwError);

    if (sNumSubKeys && ppRegEntries)
    {
        dwError = RegCacheSafeRecordSubKeysInfo(
                                sNumSubKeys,
                                ppRegEntries,
                                pKeyResult);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    RegCacheSafeFreeEntryList(sNumSubKeys,&ppRegEntries);
    return dwError;

error:
    goto cleanup;

}

DWORD
SqliteFillinKeyValuesInfo(
    PREG_KEY_CONTEXT pKeyResult
    )
{
    DWORD dwError = 0;
    size_t sNumValues = 0;
    PREG_ENTRY* ppRegEntries = NULL;

    dwError = RegDbQueryInfoKey(ghCacheConnection,
                                pKeyResult->pszKeyName,
                                QueryValues,
                                &sNumValues,
                                &ppRegEntries);
    BAIL_ON_REG_ERROR(dwError);

    if (sNumValues && ppRegEntries)
    {
        dwError = RegCacheSafeRecordValuesInfo(
                                sNumValues,
                                ppRegEntries,
                                pKeyResult);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    RegCacheSafeFreeEntryList(sNumValues,&ppRegEntries);
    return dwError;

error:
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
    dwError = RegDbQueryInfoKey(ghCacheConnection,
                                pszKeyName,
                                QuerySubKeys,
                                &sSubkeyCount,
                                NULL);
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
        /*if (RegSrvGetKeyRefCount(pFoundKey) == 1)
        {
            RegSrvDeleteActiveKey_inlock(pszKeyName);
        }*/
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
        RegSrvSetHasSubKeyInfo(FALSE, pKeyResult);
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
        RegSrvSetHasSubKeyInfo(FALSE, pKeyResult);
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
        RegSrvSetHasValueInfo(FALSE, pKeyResult);
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
        RegSrvSetHasValueInfo(FALSE, pKeyResult);
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
        RegSafeFreeKeyContext((PREG_KEY_CONTEXT*)&pEntry->pValue);
    }
}

DWORD
GetValueAsBytes(
    IN REG_DATA_TYPE type,
    IN PCSTR pszValue,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    PBYTE pTempData = NULL;
    DWORD dwValue = 0;
    DWORD cbData = 0;

    if (LW_IS_NULL_OR_EMPTY_STR(pszValue))
        goto cleanup;

    switch (type)
    {
        case REG_BINARY:
        case REG_MULTI_SZ:
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

        case REG_SZ:

            if(pData && strlen(pszValue)+1 > *pcbData)
            {
                dwError = LW_ERROR_INSUFFICIENT_BUFFER;
                BAIL_ON_REG_ERROR(dwError);
            }

            cbData = strlen(pszValue)*sizeof(*pszValue);

            if (pData)
            {
                memcpy(pData, pszValue, (strlen(pszValue)+1)*sizeof(*pszValue));
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

    if (pcbData)
    {
        *pcbData = cbData;
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pTempData);
    return dwError;

error:
    if (pcbData)
    {
        *pcbData = 0;
    }

    goto cleanup;
}
