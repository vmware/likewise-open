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

static
REG_DATA_TYPE
GetRegDataType(
    REG_DATA_TYPE_FLAGS Flags
    );

DWORD
SqliteProvider_Initialize(
    PREGPROV_PROVIDER_FUNCTION_TABLE* ppFnTable,
    const PSTR* ppszRootKeyNames
    )
{
    DWORD dwError = ERROR_SUCCESS;
    int iCount = 0;

    dwError = RegDbOpen(REG_CACHE,
                        &ghCacheConnection);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegHashCreate(
                    2 * 1024,
                    RegHashCaselessStringCompare,
                    RegHashCaselessStringHash,
                    SqliteCacheFreeHashEntry,
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
            SqliteCacheFreeHashEntry(pHashEntry);
        }

        RegHashSafeFree(&gActiveKeyList.pKeyList);
    }
}

NTSTATUS
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
	NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;

    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_NT_INVALID_POINTER(pSubKey);

    status =  SqliteCreateKeyInternal(
               pKey->pszKeyName,
               pSubKey,
               phkResult);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

NTSTATUS
SqliteOpenKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pwszSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszRootKeyName = NULL;

    if (pKey)
    {
        if (LW_IS_NULL_OR_EMPTY_STR(pKey->pszKeyName))
        {
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
        }

        status = SqliteOpenKeyInternal(pKey->pszKeyName,
                                        pwszSubKey,
                                        phkResult);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        if (!pwszSubKey)
        {
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
        }

	status = LwRtlCStringAllocateFromWC16String(&pszRootKeyName, pwszSubKey);
        BAIL_ON_NT_STATUS(status);

        if (LW_IS_NULL_OR_EMPTY_STR(pszRootKeyName))
        {
            status = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(status);
        }

        status = SqliteOpenKeyInternal(pszRootKeyName, NULL, phkResult);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    LWREG_SAFE_FREE_STRING(pszRootKeyName);

    return status;

error:
    goto cleanup;
}

VOID
SqliteCloseKey(
    IN HKEY hKey
    )
{
    SqliteCacheReleaseKey((PREG_KEY_CONTEXT)hKey);
}

NTSTATUS
SqliteDeleteKey(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszSubKey = NULL;
    PSTR pszKeyName = NULL;
    PSTR pszParentKeyName = NULL;

    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_NT_INVALID_POINTER(pSubKey);

	status = LwRtlCStringAllocateFromWC16String(&pszSubKey, pSubKey);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCStringAllocatePrintf(
                   &pszKeyName,
                   "%s\\%s",
                   pKey->pszKeyName,
                   pszSubKey);
    BAIL_ON_NT_STATUS(status);

    status = SqliteDeleteActiveKey(pszKeyName);
    BAIL_ON_NT_STATUS(status);

    status = SqliteDeleteKeyInternal(pszKeyName);
    BAIL_ON_NT_STATUS(status);

cleanup:
    LWREG_SAFE_FREE_STRING(pszSubKey);
    LWREG_SAFE_FREE_STRING(pszKeyName);
    LWREG_SAFE_FREE_STRING(pszParentKeyName);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteQueryInfoKey(
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
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    size_t sNumSubKeys = 0;
    size_t sNumValues = 0;
    DWORD dwOffset = 0;
    BOOLEAN bInLock = FALSE;


    BAIL_ON_INVALID_KEY(pKey);

    status = SqliteOpenKeyInternal(pKey->pszKeyName,
                                    NULL,
                                    (PHKEY) &pKeyResult);
    BAIL_ON_NT_STATUS(status);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    status = SqliteCacheSubKeysInfo_inlock(pKeyResult, FALSE);
    BAIL_ON_NT_STATUS(status);

    status = SqliteCacheKeyValuesInfo_inlock(pKeyResult, FALSE);
    BAIL_ON_NT_STATUS(status);

    if (pKeyResult->dwNumSubKeys > pKeyResult->dwNumCacheSubKeys)
    {
        dwOffset = pKeyResult->dwNumCacheSubKeys;
        do
        {
            status = SqliteCacheUpdateSubKeysInfo_inlock(
                              dwOffset,
                              pKeyResult,
                              FALSE,
                              &sNumSubKeys);
            BAIL_ON_NT_STATUS(status);

            dwOffset+= (DWORD)sNumSubKeys;
        } while (sNumSubKeys);
    }

    if (pKeyResult->dwNumValues > pKeyResult->dwNumCacheValues)
    {
        dwOffset = pKeyResult->dwNumCacheValues;
        do
        {
            status = SqliteCacheUpdateValuesInfo_inlock(
                              dwOffset,
                              pKeyResult,
                              FALSE,
                              &sNumValues);
            BAIL_ON_NT_STATUS(status);

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
    SqliteCacheReleaseKey(pKeyResult);

    return status;

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

NTSTATUS
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
	NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    // Do not free if it is an active key
    PREG_KEY_CONTEXT pKeyResult = NULL;
    size_t sSubKeyLen = 0;
    PWSTR pSubKeyName = NULL;
    BOOLEAN bInLock = FALSE;
    size_t sNumSubKeys = 0;
    PREG_ENTRY* ppRegEntries = NULL;


    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_NT_INVALID_POINTER(pName); // the size of pName is *pcName
    BAIL_ON_NT_INVALID_POINTER(pcName);

    status = SqliteOpenKeyInternal(pKey->pszKeyName,
                                    NULL,
                                   (PHKEY) &pKeyResult);
    BAIL_ON_NT_STATUS(status);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    //Try to grab information from pKeyResults:
    //if subkey information is not yet available in pKeyResult, do it here
    //Otherwise, use this information
    status = SqliteCacheSubKeysInfo_inlock(pKeyResult, FALSE);
    BAIL_ON_NT_STATUS(status);

    if (!pKeyResult->dwNumSubKeys)
    {
        goto cleanup;
    }

    if (dwIndex >= pKeyResult->dwNumSubKeys)
    {
	status = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(status);
    }

    if (dwIndex < pKeyResult->dwNumCacheSubKeys)
    {
	status = LwRtlWC16StringAllocateFromCString(&pSubKeyName,
			                                     pKeyResult->ppszSubKeyNames[dwIndex]);
	BAIL_ON_NT_STATUS(status);

        if (pSubKeyName)
        {
		sSubKeyLen = RtlWC16StringNumChars(pSubKeyName);
        }
    }
    else
    {
        status = RegDbQueryInfoKey(ghCacheConnection,
                                    pKeyResult->pszKeyName,
                                    QuerySubKeys,
                                    1,
                                    dwIndex,
                                    &sNumSubKeys,
                                    &ppRegEntries);
        BAIL_ON_NT_STATUS(status);

        if (sNumSubKeys != 1)
        {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

	status = LwRtlWC16StringAllocateFromCString(&pSubKeyName,
			                                     ppRegEntries[0]->pszKeyName);
	BAIL_ON_NT_STATUS(status);

        if (pSubKeyName)
        {
		sSubKeyLen = RtlWC16StringNumChars(pSubKeyName);
        }
    }

    if (*pcName < sSubKeyLen+1)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pName, pSubKeyName, sSubKeyLen*sizeof(*pName));
    pName[sSubKeyLen] = (LW_WCHAR)'\0';
    *pcName = (DWORD)sSubKeyLen;

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    LWREG_SAFE_FREE_MEMORY(pSubKeyName);
    RegDbSafeFreeEntryList(sNumSubKeys,&ppRegEntries);
    SqliteCacheReleaseKey(pKeyResult);

    return status;

error:
    *pcName = 0;

    goto cleanup;
}

NTSTATUS
SqliteSetValueEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR    pszValueName = NULL;
    PSTR    pszValue = NULL;
    PWSTR   pwcValue = NULL;
    DWORD   dwValue = 0;
    BOOLEAN bIsWrongType = TRUE;
    PWSTR*  ppwszOutMultiSz = NULL;

    BAIL_ON_INVALID_KEY(pKey);

    if (MAX_VALUE_LENGTH < cbData)
    {
	status = STATUS_INVALID_BLOCK_LENGTH;
	BAIL_ON_NT_STATUS(status);
    }

    if (!pValueName)
    {
        status = LwRtlCStringDuplicate(&pszValueName, "");
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
	status = LwRtlCStringAllocateFromWC16String(&pszValueName, pValueName);
        BAIL_ON_NT_STATUS(status);
    }

    status = RegDbGetKeyValue(ghCacheConnection,
                               pKey->pszKeyName,
                               pszValueName,
                               (REG_DATA_TYPE)dwType,
                               &bIsWrongType,
                               NULL);
    if (!status)
    {
        status = STATUS_DUPLICATE_NAME;
        BAIL_ON_NT_STATUS(status);
    }
    else if (STATUS_OBJECT_NAME_NOT_FOUND == status)
    {
        status = 0;
    }
    BAIL_ON_NT_STATUS(status);

    if (!cbData)
    {
	goto done;
    }

    switch (dwType)
    {
        case REG_BINARY:
            status = RegByteArrayToHexStr((UCHAR*)pData,
                                          cbData,
                                          &pszValue);
            BAIL_ON_NT_STATUS(status);

            break;

        case REG_MULTI_SZ:
        case REG_SZ:
		if (cbData == 1)
		{
                status = STATUS_INTERNAL_ERROR;
                BAIL_ON_NT_STATUS(status);
		}

            if (pData[cbData-1] != '\0' || pData[cbData-2] != '\0' )
            {
                status = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(status);
            }

			status = RegByteArrayToHexStr((PBYTE)pData,
										  cbData,
										  &pszValue);
			BAIL_ON_NT_STATUS(status);

            break;

        case REG_DWORD:
            dwValue = *(DWORD*)&pData[0];

            /* Maximum size for a 4 byte integer converted to a string */
            status = LW_RTL_ALLOCATE((PVOID*)&pszValue, UCHAR, 32+1);
            BAIL_ON_NT_STATUS(status);

            sprintf(pszValue, "%d", dwValue);

            break;

        default:
		status = STATUS_NOT_SUPPORTED;
            BAIL_ON_NT_STATUS(status);
    }

done:
    status = RegDbCreateKeyValue(ghCacheConnection,
                                  pKey->pszKeyName,
                                  pszValueName,
                                  pszValue,
                                  (REG_DATA_TYPE)dwType,
                                  NULL);
    BAIL_ON_NT_STATUS(status);

    SqliteCacheResetKeyValueInfo(pKey->pszKeyName);

cleanup:
    LWREG_SAFE_FREE_STRING(pszValueName);
    LWREG_SAFE_FREE_STRING(pszValue);
    LWREG_SAFE_FREE_MEMORY(pwcValue);
    if (ppwszOutMultiSz)
    {
        RegFreeMultiStrsW(ppwszOutMultiSz);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteGetValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
	NTSTATUS status = STATUS_SUCCESS;
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
	status = LwRtlCStringAllocateFromWC16String(&pszSubKey, pSubKey);
        BAIL_ON_NT_STATUS(status);

        status = LwRtlCStringAllocatePrintf(
                       &pszKeyWithSubKeyName,
                       "%s\\%s",
                       pKey->pszKeyName,
                       pszSubKey);
        BAIL_ON_NT_STATUS(status);
    }
    pszKeyName = pSubKey ? pszKeyWithSubKeyName : pKey->pszKeyName;

    if (!pValue)
    {
        status = LwRtlCStringDuplicate(&pszValueName, "");
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
	status = LwRtlCStringAllocateFromWC16String(&pszValueName, pValue);
        BAIL_ON_NT_STATUS(status);
    }

    status = RegDbGetKeyValue(ghCacheConnection,
                               pszKeyName,
                               pszValueName,
                               GetRegDataType(Flags),
                               &bIsWrongType,
                               &pRegEntry);
    BAIL_ON_NT_STATUS(status);

    status = RegGetValueAsBytes(pRegEntry->type,
                              (PCSTR)pRegEntry->pszValue,
                              FALSE,
                              pData,
                              pcbData);
    BAIL_ON_NT_STATUS(status);

    *pdwType = (DWORD)pRegEntry->type;

cleanup:
    LWREG_SAFE_FREE_STRING(pszKeyWithSubKeyName);
    LWREG_SAFE_FREE_STRING(pszSubKey);
    LWREG_SAFE_FREE_STRING(pszValueName);
    RegDbSafeFreeEntry(&pRegEntry);

    return status;

error:
   *pdwType = 0;

    goto cleanup;
}

NTSTATUS
SqliteDeleteKeyValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszSubKey = NULL;
    PSTR pszKeyNameWithSubKey = NULL;
    PSTR pszValueName = NULL;
    // Do not free
    PSTR pszKeyName = NULL;

    BAIL_ON_INVALID_KEY(pKey);

    if (pSubKey)
    {
	status = LwRtlCStringAllocateFromWC16String(&pszSubKey, pSubKey);
        BAIL_ON_NT_STATUS(status);

        status = LwRtlCStringAllocatePrintf(
                       &pszKeyNameWithSubKey,
                       "%s\\%s",
                       pKey->pszKeyName,
                       pszSubKey);
        BAIL_ON_NT_STATUS(status);
    }

    pszKeyName = pSubKey ? pszKeyNameWithSubKey : pKey->pszKeyName;

    if (pValueName)
    {
	status = LwRtlCStringAllocateFromWC16String(&pszValueName, pValueName);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        status = LwRtlCStringDuplicate(&pszValueName, "");
        BAIL_ON_NT_STATUS(status);
    }

    status = RegDbGetKeyValue(ghCacheConnection,
                               pszKeyName,
                               pszValueName,
                               REG_UNKNOWN,
                               NULL,
                               NULL);
    BAIL_ON_NT_STATUS(status);

    status = RegDbDeleteKeyValue(ghCacheConnection,
                                  pszKeyName,
                                  pszValueName);
    BAIL_ON_NT_STATUS(status);

    SqliteCacheResetKeyValueInfo(pszKeyName);

cleanup:
    LWREG_SAFE_FREE_STRING(pszSubKey);
    LWREG_SAFE_FREE_STRING(pszKeyNameWithSubKey);
    LWREG_SAFE_FREE_STRING(pszValueName);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteDeleteValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PSTR pszValueName = NULL;

    BAIL_ON_INVALID_KEY(pKey);

    if (pValueName)
    {
	status = LwRtlCStringAllocateFromWC16String(&pszValueName, pValueName);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
	status = LwRtlCStringDuplicate(&pszValueName, "");
        BAIL_ON_NT_STATUS(status);
    }

    status = RegDbGetKeyValue(ghCacheConnection,
                               pKey->pszKeyName,
                               pszValueName,
                               REG_UNKNOWN,
                               NULL,
                               NULL);
    BAIL_ON_NT_STATUS(status);

    status = RegDbDeleteKeyValue(ghCacheConnection,
                                  pKey->pszKeyName,
                                  pszValueName);
    BAIL_ON_NT_STATUS(status);

    SqliteCacheResetKeyValueInfo(pKey->pszKeyName);

cleanup:
    LWREG_SAFE_FREE_STRING(pszValueName);

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteEnumValue(
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
	NTSTATUS status = STATUS_SUCCESS;

    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    PREG_KEY_CONTEXT pKeyResult = NULL;
    size_t sValueNameLen = 0;
    PWSTR pValName = NULL;
    REG_DATA_TYPE valueType = REG_UNKNOWN;
    BOOLEAN bInLock = FALSE;
    size_t sNumValues = 0;
    PREG_ENTRY* ppRegEntries = NULL;
    PSTR pszValueContent = NULL;


    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_NT_INVALID_POINTER(pValueName); // the size of pName is *pcName
    BAIL_ON_NT_INVALID_POINTER(pcchValueName);

    status = SqliteOpenKeyInternal(pKey->pszKeyName,
                                    NULL,
                                   (PHKEY) &pKeyResult);
    BAIL_ON_NT_STATUS(status);

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    //Try to grab information from pKeyResults:
    //if subkey information is not yet available in pKeyResult, do it here
    //Otherwise, use this information
    status = SqliteCacheKeyValuesInfo_inlock(pKeyResult, FALSE);
    BAIL_ON_NT_STATUS(status);

    if (!pKeyResult->dwNumValues)
    {
        goto cleanup;
    }

    if (dwIndex >= pKeyResult->dwNumValues)
    {
	status = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(status);
    }

    if (dwIndex >= pKeyResult->dwNumCacheValues)
    {
        status = RegDbQueryInfoKey(ghCacheConnection,
                                    pKeyResult->pszKeyName,
                                    QueryValues,
                                    1,
                                    dwIndex,
                                    &sNumValues,
                                    &ppRegEntries);
        BAIL_ON_NT_STATUS(status);

        if (sNumValues != 1)
        {
            status = STATUS_INTERNAL_ERROR;
            BAIL_ON_NT_STATUS(status);
        }

	status = LwRtlWC16StringAllocateFromCString(&pValName,
			                                     ppRegEntries[0]->pszValueName);
	BAIL_ON_NT_STATUS(status);

        valueType = ppRegEntries[0]->type;

        if (ppRegEntries[0]->pszValue)
        {
            status = LwRtlCStringDuplicate(&pszValueContent,
			                        ppRegEntries[0]->pszValue);
            BAIL_ON_NT_STATUS(status);
        }
    }
    else
    {
	status = LwRtlWC16StringAllocateFromCString(&pValName,
			                                     pKeyResult->ppszValueNames[dwIndex]);
	BAIL_ON_NT_STATUS(status);

        valueType = pKeyResult->pTypes[dwIndex];

        status = LwRtlCStringDuplicate(&pszValueContent,
			                        pKeyResult->ppszValues[dwIndex]);
        BAIL_ON_NT_STATUS(status);
    }

    if (pValName)
    {
	sValueNameLen = RtlWC16StringNumChars(pValName);
    }

    if (*pcchValueName < sValueNameLen+1)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    memcpy(pValueName, pValName, sValueNameLen*sizeof(*pValueName));
    pValueName[sValueNameLen] = (LW_WCHAR)'\0';
    *pcchValueName = (DWORD)sValueNameLen;

    if (pcbData)
    {
        status = RegGetValueAsBytes(valueType,
                                  pszValueContent,
                                  FALSE,
                                  pData,
                                  pcbData);
        BAIL_ON_NT_STATUS(status);
    }

    if (pType)
    {
        *pType = valueType;
    }

cleanup:
    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    LWREG_SAFE_FREE_STRING(pszValueContent);
    LWREG_SAFE_FREE_MEMORY(pValName);

    RegDbSafeFreeEntryList(sNumValues,&ppRegEntries);
    SqliteCacheReleaseKey(pKeyResult);

    return status;

error:
    *pcchValueName = 0;

    if (pType)
    {
        *pType = REG_UNKNOWN;
    }

    goto cleanup;

    return status;
}

NTSTATUS
SqliteDeleteTree(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    HKEY hCurrKey = NULL;

    if (pSubKey)
    {
        status = SqliteOpenKeyEx(Handle,
                                  hKey,
                                  pSubKey,
                                  0,
                                  0,
                                  &hCurrKey);
        BAIL_ON_NT_STATUS(status);

        status = SqliteDeleteTreeInternal(Handle,
                                           hCurrKey);
        BAIL_ON_NT_STATUS(status);

        if (hCurrKey)
        {
            SqliteCloseKey(hCurrKey);
            hCurrKey = NULL;
        }

        status = SqliteDeleteKey(Handle, hKey, pSubKey);
        BAIL_ON_NT_STATUS(status);
    }
    else
    {
        status = SqliteDeleteTreeInternal(Handle,
                                           hKey);
        BAIL_ON_NT_STATUS(status);
    }

cleanup:
    if (hCurrKey)
    {
        SqliteCloseKey(hCurrKey);
    }

    return status;

error:
    goto cleanup;
}

NTSTATUS
SqliteQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValue,
    OUT OPTIONAL PDWORD pdwTotalsize
    )
{
	NTSTATUS status = STATUS_SUCCESS;
    PREG_KEY_CONTEXT pKey = (PREG_KEY_CONTEXT)hKey;
    DWORD dwTotalSize = 0;
    PREG_ENTRY* ppRegEntries = NULL;
    PSTR pszValueName = NULL;
    PWSTR pSingleValue = NULL;
    size_t sValueLen = 0;
    int iCount  = 0;
    int iOffSet = 0;


    BAIL_ON_INVALID_KEY(pKey);
    BAIL_ON_NT_INVALID_POINTER(pVal_list);

    if (!num_vals)
    {
        goto cleanup;
    }

    status = LW_RTL_ALLOCATE((PVOID*)&ppRegEntries, REG_ENTRY, sizeof(*ppRegEntries) * num_vals);
    BAIL_ON_NT_STATUS(status);

    for (iCount = 0; iCount < num_vals; iCount++)
    {
	BAIL_ON_NT_INVALID_POINTER(pVal_list[iCount].ve_valuename);

	status = LwRtlCStringAllocateFromWC16String(&pszValueName, pVal_list[iCount].ve_valuename);
        BAIL_ON_NT_STATUS(status);

        status = RegDbGetKeyValue(ghCacheConnection,
                                   pKey->pszKeyName,
                                   pszValueName,
                                   REG_UNKNOWN,
                                   NULL,
                                   &ppRegEntries[iCount]);
        BAIL_ON_NT_STATUS(status);
        LWREG_SAFE_FREE_STRING(pszValueName);

	status = LwRtlWC16StringAllocateFromCString(&pSingleValue,
			                                     ppRegEntries[iCount]->pszValue);
	BAIL_ON_NT_STATUS(status);

        if (pSingleValue)
        {
		sValueLen = RtlWC16StringNumChars(pSingleValue);
        }

        /* record val length */
        pVal_list[iCount].ve_valuelen = (DWORD)sValueLen;
        dwTotalSize += (DWORD)sValueLen;

        sValueLen = 0;
        LWREG_SAFE_FREE_MEMORY(pSingleValue);
    }

    if (!pVal_list)
        goto cleanup;

    if (pdwTotalsize && *pdwTotalsize < dwTotalSize)
    {
        status = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(status);
    }

    for (iCount = 0; iCount < num_vals; iCount++)
    {
        iOffSet = iCount == 0 ? 0 : (iOffSet + pVal_list[iCount-1].ve_valuelen);

        /* value type*/
        pVal_list[iCount].ve_type = ppRegEntries[iCount]->type;

	status = LwRtlWC16StringAllocateFromCString(&pSingleValue,
			                                     ppRegEntries[iCount]->pszValue);
	BAIL_ON_NT_STATUS(status);

        memcpy(pValue+iOffSet, pSingleValue, pVal_list[iCount].ve_valuelen*sizeof(*pSingleValue));

        LWREG_SAFE_FREE_MEMORY(pSingleValue);
    }

cleanup:
    if (pdwTotalsize)
    {
        *pdwTotalsize = dwTotalSize;
    }

    RegDbSafeFreeEntryList(num_vals, &ppRegEntries);
    LWREG_SAFE_FREE_STRING(pszValueName);
    LWREG_SAFE_FREE_MEMORY(pSingleValue);

    return status;

error:
    goto cleanup;
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
