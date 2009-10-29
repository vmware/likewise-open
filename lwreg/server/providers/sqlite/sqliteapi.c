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
 *          Wei Fu (wfu@likewise.com)
 */
#include "includes.h"

DWORD
SqliteProvider_Initialize(
    PREGPROV_PROVIDER_FUNCTION_TABLE* ppFnTable,
    const PSTR* ppszRootKeyNames
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    int iCount = 0;

    dwError = RegDbOpen(REG_CACHE,
                        &ghCacheConnection);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegHashCreate(
                    2 * 1024,
                    RegHashCaselessStringCompare,
                    RegHashCaselessStringHash,
                    RegSrvFreeHashEntry,
                    NULL,
                    &gActiveKeyList.pKeyList);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < NUM_ROOTKEY; iCount++)
    {
        dwError = SqliteCreateKeyInternal(ppszRootKeyNames[iCount], NULL, NULL);
        BAIL_ON_REG_ERROR(dwError);
    }

    *ppFnTable = &gRegSqliteProviderAPITable;

cleanup:
    return dwError;

error:
    *ppFnTable = NULL;
    goto cleanup;
}

VOID
SqliteProvider_Shutdown(
    PREGPROV_PROVIDER_FUNCTION_TABLE pFnTable
    )
{
    REG_HASH_ITERATOR hashIterator = {0};
    REG_HASH_ENTRY*   pHashEntry = NULL;

    RegDbSafeClose(&ghCacheConnection);

    if (gActiveKeyList.pKeyList)
    {
        RegHashGetIterator(gActiveKeyList.pKeyList, &hashIterator);
        while ((pHashEntry = RegHashNext(&hashIterator)) != NULL)
        {
            RegSrvFreeHashEntry(pHashEntry);
        }

        RegHashSafeFree(&gActiveKeyList.pKeyList);
    }
}

DWORD
SqliteCreateKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN OPTIONAL PSECURITY_ATTRIBUTES pSecurityAttributes,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;

    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_INVALID_POINTER(pSubKey);

    dwError =  SqliteCreateKeyInternal(
               pKey->pszKeyName,
               pSubKey,
               phkResult);
    BAIL_ON_REG_ERROR(dwError);

error:
    return dwError;
}

DWORD
SqliteOpenKeyExA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    DWORD dwError = 0;
    PWSTR pwszSubKey = NULL;

    if (pszSubKey)
    {
        dwError = LwMbsToWc16s(pszSubKey,
                               &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = SqliteOpenKeyExW(Handle,
                               hKey,
                               pwszSubKey,
                               ulOptions,
                               samDesired,
                               phkResult);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszSubKey);
    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteOpenKeyExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszRootKeyName = NULL;

    if (pKey)
    {
        if (LW_IS_NULL_OR_EMPTY_STR(pKey->pszKeyName))
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = SqliteOpenKeyInternal(pKey->pszKeyName,
                                        pwszSubKey,
                                        phkResult);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        if (!pwszSubKey)
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError =  LwWc16sToMbs(pwszSubKey, &pszRootKeyName);
        BAIL_ON_REG_ERROR(dwError);

        if (LW_IS_NULL_OR_EMPTY_STR(pszRootKeyName))
        {
            dwError = LW_ERROR_INVALID_PARAMETER;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = SqliteOpenKeyInternal(pszRootKeyName, NULL, phkResult);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    LW_SAFE_FREE_STRING(pszRootKeyName);

    return dwError;

error:
    goto cleanup;
}

VOID
SqliteCloseKey(
    IN HKEY hKey
    )
{
    RegSrvReleaseKey((PREG_KEY_CONTEXT)hKey);
}

DWORD
SqliteDeleteKey(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszSubKey = NULL;
    PSTR pszKeyName = NULL;
    PSTR pszParentKeyName = NULL;

    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_INVALID_POINTER(pSubKey);

    dwError = LwWc16sToMbs(pSubKey, &pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = LwAllocateStringPrintf(
                   &pszKeyName,
                   "%s\\%s",
                   pKey->pszKeyName,
                   pszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = SqliteDeleteActiveKey(pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = SqliteDbDeleteKeyInternal(pszKeyName);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_STRING(pszSubKey);
    LW_SAFE_FREE_STRING(pszKeyName);
    LW_SAFE_FREE_STRING(pszParentKeyName);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteQueryInfoKeyW(
    IN HANDLE Handle,
    IN HKEY hKey,
    OUT PWSTR pClass, /*A pointer to a buffer that receives the user-defined class of the key. This parameter can be NULL.*/
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pReserved,/*This parameter is reserved and must be NULL.*/
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,/*implement this later*/
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime/*implement this later*/
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    size_t sNumSubKeys = 0;
    size_t sNumValues = 0;
    DWORD dwOffset = 0;
    BOOLEAN bInLock = FALSE;


    BAIL_ON_INVALID_KEY(pKey);

    dwError = SqliteOpenKeyInternal(pKey->pszKeyName,
                                    NULL,
                                    (PHKEY) &pKeyResult);
    BAIL_ON_REG_ERROR(dwError);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    dwError = SqliteCacheSubKeysInfo_inlock(pKeyResult, FALSE);
    BAIL_ON_REG_ERROR(dwError);

    dwError = SqliteCacheKeyValuesInfo_inlock(pKeyResult, FALSE);
    BAIL_ON_REG_ERROR(dwError);

    if (pKeyResult->dwNumSubKeys > pKeyResult->dwNumCacheSubKeys)
    {
        dwOffset = pKeyResult->dwNumCacheSubKeys;
        do
        {
            dwError = SqliteUpdateSubKeysInfo_inlock(
                              dwOffset,
                              pKeyResult,
                              FALSE,
                              &sNumSubKeys);
            BAIL_ON_REG_ERROR(dwError);

            dwOffset+= (DWORD)sNumSubKeys;
        } while (sNumSubKeys);
    }

    if (pKeyResult->dwNumValues > pKeyResult->dwNumCacheValues)
    {
        dwOffset = pKeyResult->dwNumCacheValues;
        do
        {
            dwError = SqliteUpdateValuesInfo_inlock(
                              dwOffset,
                              pKeyResult,
                              FALSE,
                              &sNumValues);
            BAIL_ON_REG_ERROR(dwError);

            dwOffset+= (DWORD)sNumValues;
        } while (sNumValues);
    }

    if (pcSubKeys)
    {
        *pcSubKeys = pKeyResult->dwNumSubKeys;
    }
    if (pcMaxSubKeyLen)
    {
        *pcMaxSubKeyLen = pKeyResult->sMaxSubKeyLen;
    }
    if (pcValues)
    {
        *pcValues = pKeyResult->dwNumValues;
    }
    if (pcMaxValueNameLen)
    {
        *pcMaxValueNameLen = pKeyResult-> sMaxValueNameLen;
    }
    if (pcMaxValueLen)
    {
        *pcMaxValueLen = pKeyResult->sMaxValueLen;
    }

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);
    RegSrvReleaseKey(pKeyResult);

    return dwError;

error:
    if (pcSubKeys)
    {
        *pcSubKeys = 0;
    }
    if (pcMaxSubKeyLen)
    {
        *pcMaxSubKeyLen = 0;
    }
    if (pcValues)
    {
        *pcValues = 0;
    }
    if (pcMaxValueNameLen)
    {
        *pcMaxValueNameLen = 0;
    }
    if (pcMaxValueLen)
    {
        *pcMaxValueLen = 0;
    }

    goto cleanup;
}

DWORD
SqliteQueryInfoKeyA(
    IN HANDLE Handle,
    IN HKEY hKey,
    OUT PSTR pszClass, /*A pointer to a buffer that receives the user-defined class of the key. This parameter can be NULL.*/
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime/*implement this later*/
    )
{
    DWORD dwError = LW_ERROR_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    size_t sNumSubKeys = 0;
    size_t sNumValues = 0;
    DWORD dwOffset = 0;
    BOOLEAN bInLock = FALSE;


    BAIL_ON_INVALID_KEY(pKey);

    dwError = SqliteOpenKeyInternal(pKey->pszKeyName,
                                    NULL,
                                    (PHKEY) &pKeyResult);
    BAIL_ON_REG_ERROR(dwError);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    dwError = SqliteCacheSubKeysInfo_inlock(pKeyResult, TRUE);
    BAIL_ON_REG_ERROR(dwError);

    dwError = SqliteCacheKeyValuesInfo_inlock(pKeyResult, TRUE);
    BAIL_ON_REG_ERROR(dwError);

    if (pKeyResult->dwNumSubKeys > pKeyResult->dwNumCacheSubKeys)
    {
        dwOffset = pKeyResult->dwNumCacheSubKeys;
        do
        {
            dwError = SqliteUpdateSubKeysInfo_inlock(
                              dwOffset,
                              pKeyResult,
                              TRUE,
                              &sNumSubKeys);
            BAIL_ON_REG_ERROR(dwError);

            dwOffset+= (DWORD)sNumSubKeys;
        } while (sNumSubKeys);
    }

    if (pKeyResult->dwNumValues > pKeyResult->dwNumCacheValues)
    {
        dwOffset = pKeyResult->dwNumCacheValues;
        do
        {
            dwError = SqliteUpdateValuesInfo_inlock(
                              dwOffset,
                              pKeyResult,
                              TRUE,
                              &sNumValues);
            BAIL_ON_REG_ERROR(dwError);

            dwOffset+= (DWORD)sNumValues;
        } while (sNumValues);
    }

    if (pcSubKeys)
    {
        *pcSubKeys = pKeyResult->dwNumSubKeys;
    }
    if (pcMaxSubKeyLen)
    {
        *pcMaxSubKeyLen = pKeyResult->sMaxSubKeyALen;
    }
    if (pcValues)
    {
        *pcValues = pKeyResult->dwNumValues;
    }
    if (pcMaxValueNameLen)
    {
        *pcMaxValueNameLen = pKeyResult->sMaxValueNameALen;
    }
    if (pcMaxValueLen)
    {
        *pcMaxValueLen = pKeyResult->sMaxValueALen;
    }

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);
    RegSrvReleaseKey(pKeyResult);

    return dwError;

error:
    if (pcSubKeys)
    {
        *pcSubKeys = 0;
    }
    if (pcMaxSubKeyLen)
    {
        *pcMaxSubKeyLen = 0;
    }
    if (pcValues)
    {
        *pcValues = 0;
    }
    if (pcMaxValueNameLen)
    {
        *pcMaxValueNameLen = 0;
    }
    if (pcMaxValueLen)
    {
        *pcMaxValueLen = 0;
    }

    goto cleanup;
}

DWORD
SqliteEnumKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pName, /*buffer to hold keyName*/
    IN OUT PDWORD pcName,/*When the function returns, the variable receives the number of characters stored in the buffer,not including the terminating null character.*/
    IN PDWORD pReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT PFILETIME pftLastWriteTime
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    // Do not free if it is an active key
    PREG_KEY_CONTEXT pKeyResult = NULL;
    size_t sSubKeyLen = 0;
    PWSTR pSubKeyName = NULL;
    BOOLEAN bInLock = FALSE;
    size_t sNumSubKeys = 0;
    PREG_ENTRY* ppRegEntries = NULL;


    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_INVALID_POINTER(pName); // the size of pName is *pcName
    BAIL_ON_INVALID_POINTER(pcName);

    dwError = SqliteOpenKeyInternal(pKey->pszKeyName,
                                    NULL,
                                   (PHKEY) &pKeyResult);
    BAIL_ON_REG_ERROR(dwError);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    //Try to grab information from pKeyResults:
    //if subkey information is not yet available in pKeyResult, do it here
    //Otherwise, use this information
    dwError = SqliteCacheSubKeysInfo_inlock(pKeyResult, FALSE);
    BAIL_ON_REG_ERROR(dwError);

    if (!pKeyResult->dwNumSubKeys)
    {
        goto cleanup;
    }

    if (dwIndex >= pKeyResult->dwNumSubKeys)
    {
        dwError = LW_ERROR_NO_MORE_ITEMS;
        BAIL_ON_REG_ERROR(dwError);
    }

    if (dwIndex < pKeyResult->dwNumCacheSubKeys)
    {
        dwError = LwMbsToWc16s(pKeyResult->ppszSubKeyNames[dwIndex], &pSubKeyName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwWc16sLen((PCWSTR)pSubKeyName,&sSubKeyLen);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = RegDbQueryInfoKey(ghCacheConnection,
                                    pKeyResult->pszKeyName,
                                    QuerySubKeys,
                                    1,
                                    dwIndex,
                                    &sNumSubKeys,
                                    &ppRegEntries);
        BAIL_ON_REG_ERROR(dwError);

        if (sNumSubKeys != 1)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LwMbsToWc16s(ppRegEntries[0]->pszKeyName, &pSubKeyName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwWc16sLen((PCWSTR)pSubKeyName,&sSubKeyLen);
        BAIL_ON_REG_ERROR(dwError);
    }

    if (*pcName < sSubKeyLen+1)
    {
        dwError = LW_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_REG_ERROR(dwError);
    }

    memcpy(pName, pSubKeyName, sSubKeyLen*sizeof(*pName));
    pName[sSubKeyLen] = (LW_WCHAR)'\0';
    *pcName = (DWORD)sSubKeyLen;

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    LW_SAFE_FREE_MEMORY(pSubKeyName);
    RegCacheSafeFreeEntryList(sNumSubKeys,&ppRegEntries);
    RegSrvReleaseKey(pKeyResult);

    return dwError;

error:
    *pcName = 0;

    goto cleanup;
}

static
DWORD
SqliteSetValueExInternal(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN BOOLEAN bDoAnsi,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszValueName = NULL;
    PSTR pszValue = NULL;
    PWSTR pwcValue = NULL;
    DWORD dwValue = 0;
    BOOLEAN bIsWrongType = TRUE;
    PSTR* ppszOutMultiSz = NULL;
    PBYTE pOutData = NULL;
    SSIZE_T cOutDataLen = 0;

    BAIL_ON_INVALID_KEY(pKey);

    if (MAX_VALUE_LENGTH < cbData)
    {
        dwError = LW_ERROR_BEYOUND_MAX_VALUE_LEN;
        BAIL_ON_REG_ERROR(dwError);
    }

    if (!pValueName)
    {
        dwError = LwAllocateString("", &pszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = LwWc16sToMbs(pValueName,
                               &pszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }

    switch (dwType)
    {
        case REG_BINARY:
            dwError = LwByteArrayToHexStr((UCHAR*)pData,
                                          cbData,
                                          &pszValue);
            BAIL_ON_REG_ERROR(dwError);

            break;

        // Internally store reg_multi_sz as ansi
        case REG_MULTI_SZ:
            if (bDoAnsi)
            {
                dwError = LwByteArrayToHexStr((UCHAR*)pData,
                                              cbData,
                                              &pszValue);
                BAIL_ON_REG_ERROR(dwError);
            }
            else
            {
                dwError = RegByteArrayToMultiStrsW((UCHAR*)pData,
                                                   cbData,
                                                   &ppszOutMultiSz);
                BAIL_ON_REG_ERROR(dwError);

                dwError = RegMultiStrsToByteArrayA(
                                            ppszOutMultiSz,
                                            &pOutData,
                                            &cOutDataLen);
                BAIL_ON_REG_ERROR(dwError);

                dwError = LwByteArrayToHexStr((UCHAR*)pOutData,
                                              cOutDataLen,
                                              &pszValue);
                BAIL_ON_REG_ERROR(dwError);
            }

            break;


        case REG_SZ:

            if (bDoAnsi)
            {
                dwError = LwAllocateMemory(sizeof(*pszValue)*(cbData+1), (PVOID)&pszValue);
                BAIL_ON_REG_ERROR(dwError);

                memcpy(pszValue, pData, cbData*sizeof(*pData));
            }
            else
            {
                dwError = LwAllocateMemory(sizeof(*pwcValue)*(cbData+1), (PVOID)&pwcValue);
                BAIL_ON_REG_ERROR(dwError);

                dwError = LwWc16sToMbs(pwcValue, &pszValue);
                BAIL_ON_REG_ERROR(dwError);
            }
            break;

        case REG_DWORD:
            dwValue = *(DWORD*)&pData[0];

            /* Maximum size for a 4 byte integer converted to a string */
            dwError = LwAllocateMemory(sizeof(UCHAR) * 12, (PVOID)&pszValue);
            BAIL_ON_REG_ERROR(dwError);

            sprintf(pszValue, "%d", dwValue);

            break;

        default:
            dwError = LW_ERROR_UNKNOWN_DATA_TYPE;
            BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegDbGetKeyValue(ghCacheConnection,
                               pKey->pszKeyName,
                               pszValueName,
                               (REG_DATA_TYPE)dwType,
                               &bIsWrongType,
                               NULL);
    if (!dwError)
    {
        dwError = LW_ERROR_DUPLICATE_KEYVALUENAME;
        BAIL_ON_REG_ERROR(dwError);
    }
    else if (LW_ERROR_NO_SUCH_VALUENAME == dwError)
    {
        dwError = 0;
    }
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDbCreateKeyValue(ghCacheConnection,
                                  pKey->pszKeyName,
                                  pszValueName,
                                  pszValue,
                                  (REG_DATA_TYPE)dwType,
                                  NULL);
    BAIL_ON_REG_ERROR(dwError);

    RegSrvResetKeyValueInfo(pKey->pszKeyName);

cleanup:
    LW_SAFE_FREE_STRING(pszValueName);
    LW_SAFE_FREE_STRING(pszValue);
    LW_SAFE_FREE_MEMORY(pwcValue);
    LW_SAFE_FREE_MEMORY(pOutData);
    RegMultiStrsFree(ppszOutMultiSz);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteSetValueExA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    )
{
    DWORD dwError = 0;
    PWSTR pValueName = NULL;

    if (pszValueName)
    {
        dwError = LwMbsToWc16s(pszValueName, &pValueName);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = SqliteSetValueExInternal(Handle,
                                       hKey,
                                       TRUE,
                                       pValueName,
                                       Reserved,
                                       dwType,
                                       pData,
                                       cbData);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pValueName);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteSetValueExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    )
{
    return SqliteSetValueExInternal(Handle,
                                    hKey,
                                    FALSE,
                                    pValueName,
                                    Reserved,
                                    dwType,
                                    pData,
                                    cbData);
}

static
REG_DATA_TYPE
GetRegDataType(
    REG_DATA_TYPE_FLAGS Flags
    )
{
    REG_DATA_TYPE dataType = 0;

    switch (Flags)
    {
        case RRF_RT_REG_SZ:
            dataType = REG_SZ;
            break;
        case RRF_RT_REG_BINARY:
            dataType = REG_BINARY;
            break;
        case RRF_RT_REG_DWORD:
            dataType = REG_DWORD;
            break;
        case RRF_RT_REG_MULTI_SZ:
            dataType = REG_MULTI_SZ;
            break;

        default:
            dataType = REG_UNKNOWN;
    }

    return dataType;
}

static
DWORD
SqliteGetValueInternal(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN BOOLEAN bDoAnsi,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszKeyWithSubKeyName = NULL;
    //Do not free
    PSTR pszKeyName = NULL;
    PSTR pszSubKey = NULL;
    PSTR pszValueName = NULL;
    PREG_ENTRY pRegEntry = NULL;
    BOOLEAN bIsWrongType = FALSE;


    BAIL_ON_INVALID_KEY(pKey);

    if (pSubKey != NULL)
    {
        dwError = LwWc16sToMbs(pSubKey, &pszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwAllocateStringPrintf(
                       &pszKeyWithSubKeyName,
                       "%s\\%s",
                       pKey->pszKeyName,
                       pszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    pszKeyName = pSubKey ? pszKeyWithSubKeyName : pKey->pszKeyName;

    if (!pValue)
    {
        dwError = LwAllocateString("", &pszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = LwWc16sToMbs(pValue,
                               &pszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegDbGetKeyValue(ghCacheConnection,
                               pszKeyName,
                               pszValueName,
                               GetRegDataType(Flags),
                               &bIsWrongType,
                               &pRegEntry);
    BAIL_ON_REG_ERROR(dwError);

    dwError = GetValueAsBytes(pRegEntry->type,
                              (PCSTR)pRegEntry->pszValue,
                              bDoAnsi,
                              pData,
                              pcbData);
    BAIL_ON_REG_ERROR(dwError);

    if (pdwType)
    {
        *pdwType = (DWORD)pRegEntry->type;
    }

cleanup:
    LW_SAFE_FREE_STRING(pszKeyWithSubKeyName);
    LW_SAFE_FREE_STRING(pszSubKey);
    LW_SAFE_FREE_STRING(pszValueName);
    RegCacheSafeFreeEntry(&pRegEntry);

    return dwError;

error:
    if (pdwType)
    {
        *pdwType = 0;
    }

    goto cleanup;
}

DWORD
SqliteGetValueA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    PWSTR pSubKey = NULL;
    PWSTR pValue = NULL;

    if (pszSubKey)
    {
        dwError = LwMbsToWc16s(pszSubKey, &pSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    if (pszValue)
    {
        dwError = LwMbsToWc16s(pszValue, &pValue);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = SqliteGetValueInternal(Handle,
                                     hKey,
                                     TRUE,
                                     pSubKey,
                                     pValue,
                                     Flags,
                                     pdwType,
                                     pData,
                                     pcbData);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pSubKey);
    LW_SAFE_FREE_MEMORY(pValue);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteGetValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return SqliteGetValueInternal(Handle,
                                  hKey,
                                  FALSE,
                                  pSubKey,
                                  pValue,
                                  Flags,
                                  pdwType,
                                  pData,
                                  pcbData);
}

DWORD
SqliteDeleteKeyValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszSubKey = NULL;
    PSTR pszKeyNameWithSubKey = NULL;
    PSTR pszValueName = NULL;
    // Do not free
    PSTR pszKeyName = NULL;

    BAIL_ON_INVALID_KEY(pKey);

    if (pSubKey)
    {
        dwError = LwWc16sToMbs(pSubKey, &pszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwAllocateStringPrintf(
                       &pszKeyNameWithSubKey,
                       "%s\\%s",
                       pKey->pszKeyName,
                       pszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    pszKeyName = pSubKey ? pszKeyNameWithSubKey : pKey->pszKeyName;

    if (pValueName)
    {
        dwError = LwWc16sToMbs(pValueName, &pszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateString("", &pszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegDbGetKeyValue(ghCacheConnection,
                               pszKeyName,
                               pszValueName,
                               REG_UNKNOWN,
                               NULL,
                               NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDbDeleteKeyValue(ghCacheConnection,
                                  pszKeyName,
                                  pszValueName);
    BAIL_ON_REG_ERROR(dwError);

    RegSrvResetKeyValueInfo(pszKeyName);

cleanup:
    LW_SAFE_FREE_STRING(pszSubKey);
    LW_SAFE_FREE_STRING(pszKeyNameWithSubKey);
    LW_SAFE_FREE_STRING(pszValueName);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteDeleteValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszValueName = NULL;

    BAIL_ON_INVALID_KEY(pKey);

    if (pValueName)
    {
        dwError = LwWc16sToMbs(pValueName, &pszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = LwAllocateString("", &pszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegDbGetKeyValue(ghCacheConnection,
                               pKey->pszKeyName,
                               pszValueName,
                               REG_UNKNOWN,
                               NULL,
                               NULL);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegDbDeleteKeyValue(ghCacheConnection,
                                  pKey->pszKeyName,
                                  pszValueName);
    BAIL_ON_REG_ERROR(dwError);

    RegSrvResetKeyValueInfo(pKey->pszKeyName);

cleanup:
    LW_SAFE_FREE_STRING(pszValueName);

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteEnumValueA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PSTR pszValueName, /*buffer hold valueName*/
    IN OUT PDWORD pcchValueName, /*input - buffer pValueName length*/
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,/*buffer hold value content*/
    IN OUT OPTIONAL PDWORD pcbData /*input - buffer pData length*/
    )
{
    DWORD dwError = 0;

    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    // Do not free if it is an active key
    PREG_KEY_CONTEXT pKeyResult = NULL;
    REG_DATA_TYPE valueType = REG_UNKNOWN;
    BOOLEAN bInLock = FALSE;
    size_t sNumValues = 0;
    PREG_ENTRY* ppRegEntries = NULL;
    PSTR pszValName = NULL;
    PSTR pszValueContent = NULL;


    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_INVALID_POINTER(pszValueName);
    BAIL_ON_INVALID_POINTER(pcchValueName);

    dwError = SqliteOpenKeyInternal(pKey->pszKeyName,
                                    NULL,
                                   (PHKEY) &pKeyResult);
    BAIL_ON_REG_ERROR(dwError);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    //Try to grab information from pKeyResults:
    //if subkey information is not yet available in pKeyResult, do it here
    //Otherwise, use this information
    dwError = SqliteCacheKeyValuesInfo_inlock(pKeyResult, TRUE);
    BAIL_ON_REG_ERROR(dwError);

    if (!pKeyResult->dwNumValues)
    {
        goto cleanup;
    }

    if (dwIndex >= pKeyResult->dwNumValues)
    {
        dwError = LW_ERROR_NO_MORE_ITEMS;
        BAIL_ON_REG_ERROR(dwError);
    }

    if (dwIndex >= pKeyResult->dwNumCacheValues)
    {
        dwError = RegDbQueryInfoKey(ghCacheConnection,
                                    pKeyResult->pszKeyName,
                                    QueryValues,
                                    1,
                                    dwIndex,
                                    &sNumValues,
                                    &ppRegEntries);
        BAIL_ON_REG_ERROR(dwError);

        if (sNumValues != 1)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LwAllocateString(ppRegEntries[0]->pszValueName,
                                   &pszValName);
        BAIL_ON_REG_ERROR(dwError);

        valueType = ppRegEntries[0]->type;

        if (ppRegEntries[0]->pszValue)
        {
            dwError = LwAllocateString(ppRegEntries[0]->pszValue,
                                       &pszValueContent);
            BAIL_ON_REG_ERROR(dwError);
        }
    }
    else
    {
        dwError = LwAllocateString(pKeyResult->ppszValueNames[dwIndex],
                                   &pszValName);
        BAIL_ON_REG_ERROR(dwError);

        valueType = pKeyResult->pTypes[dwIndex];

        dwError = LwAllocateString(pKeyResult->ppszValues[dwIndex],
                                   &pszValueContent);
        BAIL_ON_REG_ERROR(dwError);
    }

    if (*pcchValueName < strlen(pszValName)+1)
    {
        dwError = LW_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_REG_ERROR(dwError);
    }

    memcpy(pszValueName, pszValName, strlen(pszValName)+1);

    *pcchValueName = strlen(pszValName);

    if (pcbData)
    {
        dwError = GetValueAsBytes(valueType,
                                  pszValueContent,
                                  TRUE,
                                  pData,
                                  pcbData);
        BAIL_ON_REG_ERROR(dwError);
    }

    if (pType)
    {
        *pType = valueType;
    }

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    LW_SAFE_FREE_STRING(pszValName);
    LW_SAFE_FREE_STRING(pszValueContent);

    RegCacheSafeFreeEntryList(sNumValues,&ppRegEntries);
    RegSrvReleaseKey(pKeyResult);

    return dwError;

error:
    *pcchValueName = 0;

    if (pType)
    {
        *pType = REG_UNKNOWN;
    }

    goto cleanup;

    return dwError;
}

DWORD
SqliteEnumValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName, /*buffer hold valueName*/
    IN OUT PDWORD pcchValueName, /*input - buffer pValueName length*/
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,/*buffer hold value content*/
    IN OUT OPTIONAL PDWORD pcbData /*input - buffer pData length*/
    )
{
    DWORD dwError = 0;

    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    // Do not free if it is an active key
    PREG_KEY_CONTEXT pKeyResult = NULL;
    size_t sValueNameLen = 0;
    PWSTR pValName = NULL;
    REG_DATA_TYPE valueType = REG_UNKNOWN;
    BOOLEAN bInLock = FALSE;
    size_t sNumValues = 0;
    PREG_ENTRY* ppRegEntries = NULL;
    PSTR pszValueContent = NULL;


    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_INVALID_POINTER(pValueName); // the size of pName is *pcName
    BAIL_ON_INVALID_POINTER(pcchValueName);

    dwError = SqliteOpenKeyInternal(pKey->pszKeyName,
                                    NULL,
                                   (PHKEY) &pKeyResult);
    BAIL_ON_REG_ERROR(dwError);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    //Try to grab information from pKeyResults:
    //if subkey information is not yet available in pKeyResult, do it here
    //Otherwise, use this information
    dwError = SqliteCacheKeyValuesInfo_inlock(pKeyResult, FALSE);
    BAIL_ON_REG_ERROR(dwError);

    if (!pKeyResult->dwNumValues)
    {
        goto cleanup;
    }

    if (dwIndex >= pKeyResult->dwNumValues)
    {
        dwError = LW_ERROR_NO_MORE_ITEMS;
        BAIL_ON_REG_ERROR(dwError);
    }

    if (dwIndex >= pKeyResult->dwNumCacheValues)
    {
        dwError = RegDbQueryInfoKey(ghCacheConnection,
                                    pKeyResult->pszKeyName,
                                    QueryValues,
                                    1,
                                    dwIndex,
                                    &sNumValues,
                                    &ppRegEntries);
        BAIL_ON_REG_ERROR(dwError);

        if (sNumValues != 1)
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LwMbsToWc16s(ppRegEntries[0]->pszValueName, &pValName);
        BAIL_ON_REG_ERROR(dwError);

        valueType = ppRegEntries[0]->type;

        if (ppRegEntries[0]->pszValue)
        {
            dwError = LwAllocateString(ppRegEntries[0]->pszValue,
                                       &pszValueContent);
            BAIL_ON_REG_ERROR(dwError);
        }
    }
    else
    {
        dwError = LwMbsToWc16s(pKeyResult->ppszValueNames[dwIndex], &pValName);
        BAIL_ON_REG_ERROR(dwError);

        valueType = pKeyResult->pTypes[dwIndex];

        dwError = LwAllocateString(pKeyResult->ppszValues[dwIndex],
                                   &pszValueContent);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwWc16sLen((PCWSTR)pValName,&sValueNameLen);
    BAIL_ON_REG_ERROR(dwError);

    if (*pcchValueName < sValueNameLen+1)
    {
        dwError = LW_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_REG_ERROR(dwError);
    }

    memcpy(pValueName, pValName, sValueNameLen*sizeof(*pValueName));
    pValueName[sValueNameLen] = (LW_WCHAR)'\0';
    *pcchValueName = (DWORD)sValueNameLen;

    if (pcbData)
    {
        dwError = GetValueAsBytes(valueType,
                                  pszValueContent,
                                  FALSE,
                                  pData,
                                  pcbData);
        BAIL_ON_REG_ERROR(dwError);
    }

    if (pType)
    {
        *pType = valueType;
    }

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    LW_SAFE_FREE_STRING(pszValueContent);

    RegCacheSafeFreeEntryList(sNumValues,&ppRegEntries);
    RegSrvReleaseKey(pKeyResult);

    return dwError;

error:
    *pcchValueName = 0;

    if (pType)
    {
        *pType = REG_UNKNOWN;
    }

    goto cleanup;

    return dwError;
}

DWORD
SqliteQueryValueExA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCSTR pszValueName,
    IN PDWORD pReserved,
    OUT PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return SqliteGetValueA(
             Handle,
             hKey,
             NULL,
             pszValueName,
             RRF_RT_REG_NONE,
             pType,
             pData,
             pcbData);
}

DWORD
SqliteQueryValueExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return SqliteGetValueW(
             Handle,
             hKey,
             NULL,
             pValueName,
             RRF_RT_REG_NONE,
             pType,
             pData,
             pcbData);
}

/*delete all subkeys and values of hKey*/
static
DWORD
SqliteDeleteTreeInternal(
    IN HANDLE Handle,
    IN HKEY hKey
    )
{
    DWORD dwError = 0;
    HKEY hCurrKey = NULL;
    int iCount = 0;
    DWORD dwSubKeyCount = 0;
    LW_WCHAR psubKeyName[MAX_KEY_LENGTH];
    DWORD dwSubKeyLen = 0;
    PSTR* ppszSubKey = NULL;
    //Do not free
    PSTR pszSubKeyName = NULL;
    PWSTR pwszSubKey = NULL;

    dwError = SqliteQueryInfoKeyW(Handle,
                                 hKey,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &dwSubKeyCount,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL);
    BAIL_ON_REG_ERROR(dwError);

    if (dwSubKeyCount)
    {
        dwError = LwAllocateMemory(sizeof(*ppszSubKey)*dwSubKeyCount, (PVOID)&ppszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    for (iCount = 0; iCount < dwSubKeyCount; iCount++)
    {
        dwSubKeyLen = MAX_KEY_LENGTH;
        memset(psubKeyName, 0, MAX_KEY_LENGTH);

        dwError = SqliteEnumKeyEx(Handle,
                                  hKey,
                                  iCount,
                                  psubKeyName,
                                  &dwSubKeyLen,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwWc16sToMbs(psubKeyName, &ppszSubKey[iCount]);
        BAIL_ON_REG_ERROR(dwError);
    }

    for (iCount = 0; iCount < dwSubKeyCount; iCount++)
    {
        pszSubKeyName = strrchr(ppszSubKey[iCount], '\\');

        if (LW_IS_NULL_OR_EMPTY_STR(pszSubKeyName))
        {
            dwError = LW_ERROR_INTERNAL;
            BAIL_ON_REG_ERROR(dwError);
        }

        dwError = LwMbsToWc16s(pszSubKeyName+1, &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = SqliteOpenKeyExW(Handle,
                                  hKey,
                                  pwszSubKey,
                                  0,
                                  0,
                                  &hCurrKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = SqliteDeleteTreeInternal(Handle,
                                           hCurrKey);
        BAIL_ON_REG_ERROR(dwError);

        if (hCurrKey)
        {
            SqliteCloseKey(hCurrKey);
        }

        dwError = SqliteDeleteKey(Handle, hKey, pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);

        LW_SAFE_FREE_MEMORY(pwszSubKey);
        hCurrKey = NULL;
    }

cleanup:
    if (hCurrKey)
    {
        SqliteCloseKey(hCurrKey);
    }
    LwFreeStringArray(ppszSubKey, dwSubKeyCount);
    LW_SAFE_FREE_MEMORY(pwszSubKey);

    return dwError;


error:
    goto cleanup;
}

DWORD
SqliteDeleteTree(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
    DWORD dwError = 0;
    HKEY hCurrKey = NULL;

    if (pSubKey)
    {
        dwError = SqliteOpenKeyExW(Handle,
                                  hKey,
                                  pSubKey,
                                  0,
                                  0,
                                  &hCurrKey);
        BAIL_ON_REG_ERROR(dwError);

        dwError = SqliteDeleteTreeInternal(Handle,
                                           hCurrKey);
        BAIL_ON_REG_ERROR(dwError);

        if (hCurrKey)
        {
            SqliteCloseKey(hCurrKey);
            hCurrKey = NULL;
        }

        dwError = SqliteDeleteKey(Handle, hKey, pSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }
    else
    {
        dwError = SqliteDeleteTreeInternal(Handle,
                                           hKey);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    if (hCurrKey)
    {
        SqliteCloseKey(hCurrKey);
    }

    return dwError;

error:
    goto cleanup;
}

DWORD
SqliteQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValue,
    OUT OPTIONAL PDWORD pdwTotalsize
    )
{
    DWORD dwError = 0;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    DWORD dwTotalSize = 0;
    PREG_ENTRY* ppRegEntries = NULL;
    PSTR pszValueName = NULL;
    PWSTR pSingleValue = NULL;
    size_t sValueLen = 0;
    int iCount  = 0;
    int iOffSet = 0;


    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_INVALID_POINTER(pVal_list);

    if (!num_vals)
    {
        goto cleanup;
    }

    dwError = LwAllocateMemory(sizeof(*ppRegEntries)*num_vals,(PVOID)&ppRegEntries);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < num_vals; iCount++)
    {
        BAIL_ON_INVALID_POINTER(pVal_list[iCount].ve_valuename);

        dwError = LwWc16sToMbs(pVal_list[iCount].ve_valuename, &pszValueName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = RegDbGetKeyValue(ghCacheConnection,
                                   pKey->pszKeyName,
                                   pszValueName,
                                   REG_UNKNOWN,
                                   NULL,
                                   &ppRegEntries[iCount]);
        BAIL_ON_REG_ERROR(dwError);
        LW_SAFE_FREE_STRING(pszValueName);

        dwError = LwMbsToWc16s(ppRegEntries[iCount]->pszValue, &pSingleValue);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwWc16sLen((PCWSTR)pSingleValue,&sValueLen);
        BAIL_ON_REG_ERROR(dwError);

        /* record val length */
        pVal_list[iCount].ve_valuelen = (DWORD)sValueLen;
        dwTotalSize += (DWORD)sValueLen;

        sValueLen = 0;
        LW_SAFE_FREE_MEMORY(pSingleValue);
    }

    if (!pVal_list)
        goto cleanup;

    if (pdwTotalsize && *pdwTotalsize < dwTotalSize)
    {
        dwError = LW_ERROR_INSUFFICIENT_BUFFER;
        BAIL_ON_REG_ERROR(dwError);
    }

    for (iCount = 0; iCount < num_vals; iCount++)
    {
        iOffSet = iCount == 0 ? 0 : (iOffSet + pVal_list[iCount-1].ve_valuelen);

        /* value type*/
        pVal_list[iCount].ve_type = ppRegEntries[iCount]->type;

        dwError = LwMbsToWc16s(ppRegEntries[iCount]->pszValue, &pSingleValue);
        BAIL_ON_REG_ERROR(dwError);

        memcpy(pValue+iOffSet, pSingleValue, pVal_list[iCount].ve_valuelen*sizeof(*pSingleValue));

        LW_SAFE_FREE_MEMORY(pSingleValue);
    }

cleanup:
    if (pdwTotalsize)
    {
        *pdwTotalsize = dwTotalSize;
    }

    RegCacheSafeFreeEntryList(num_vals, &ppRegEntries);
    LW_SAFE_FREE_STRING(pszValueName);
    LW_SAFE_FREE_MEMORY(pSingleValue);

    return dwError;

error:
    goto cleanup;
}
