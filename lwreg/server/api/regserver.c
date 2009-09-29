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
 *        regserver.c
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
#include "api.h"

typedef enum __REG_ACCESS_RIGHT
{
    REG_WRITE = 0,
    REG_READ,
} REG_ACCESS_RIGHT, *PREG_ACCESS_RIGHT;

static
BOOLEAN
RegSrvCheckAccessRight(
    HANDLE handle,
    REG_ACCESS_RIGHT access
    )
{
    PREG_SRV_API_STATE pServerState = (PREG_SRV_API_STATE)handle;
    BOOLEAN bIsAccessible = TRUE;

    if (!(pServerState->peerGID == 0 || pServerState->peerUID == 0) && REG_WRITE == access)
    {
        bIsAccessible = FALSE;
    }

    return bIsAccessible;
}

BOOLEAN
RegSrvIsValidKeyName(
    PSTR pszKeyName
    )
{
    CHAR ch = '\\';
    PSTR pszStr = NULL;

    pszStr = strchr(pszKeyName,(int)ch);

    return pszStr == NULL ? TRUE : FALSE;
}

DWORD
RegSrvEnumRootKeys(
    IN HANDLE Handle,
    OUT PSTR** pppszRootKeys,
    OUT PDWORD pdwNumRootKeys
    )
{
    DWORD dwError = 0;
    PSTR* ppszRootKeys = NULL;
    int iCount = 0;

    dwError = LwAllocateMemory(sizeof(*ppszRootKeys)*NUM_ROOTKEY, (PVOID)&ppszRootKeys);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount< NUM_ROOTKEY; iCount++)
    {
        dwError = LwAllocateString(ROOT_KEYS[iCount], &ppszRootKeys[iCount]);
        BAIL_ON_REG_ERROR(dwError);
    }

    *pdwNumRootKeys = NUM_ROOTKEY;
    *pppszRootKeys = ppszRootKeys;

cleanup:
    return dwError;

error:
    if (ppszRootKeys)
    {
        LwFreeStringArray(ppszRootKeys, NUM_ROOTKEY);
    }
    *pdwNumRootKeys = 0;
    *pppszRootKeys = NULL;

    goto cleanup;
}

DWORD
RegSrvCreateKeyEx(
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

    if (!RegSrvCheckAccessRight(Handle, REG_WRITE))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvCreateKeyEx(
                                           Handle,
                                           hKey,
                                           pSubKey,
                                           Reserved,
                                           pClass,
                                           dwOptions,
                                           samDesired,
                                           pSecurityAttributes,
                                           phkResult,
                                           pdwDisposition);
    BAIL_ON_REG_ERROR(dwError);


error:

    return dwError;
}

DWORD
RegSrvOpenRootKey(
    IN HANDLE Handle,
    IN PSTR pszRootKeyName,
    OUT PHKEY phkResult
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvOpenRootKey(
                        Handle,
                        pszRootKeyName,
                        phkResult);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

DWORD
RegSrvOpenKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvOpenKeyEx(
                                 Handle,
                                 hKey,
                                 pSubKey,
                                 ulOptions,
                                 samDesired,
                                 phkResult);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

VOID
RegSrvCloseKey(
    HKEY hKey
    )
{
    return gpRegProvider->pfnRegSrvCloseKey(hKey);
}

DWORD
RegSrvDeleteKey(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_WRITE))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvDeleteKey(Handle,
                                                hKey,
                                                pSubKey);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

DWORD
RegSrvDeleteKeyValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValueName
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_WRITE))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvDeleteKeyValue(Handle,
                                                     hKey,
                                                     pSubKey,
                                                     pValueName);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

DWORD
RegSrvDeleteValue(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_WRITE))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvDeleteValue(Handle,
                                                  hKey,
                                                  pValueName);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

DWORD
RegSrvEnumKeyEx(
    HANDLE Handle,
    HKEY hKey,
    DWORD dwIndex,
    PWSTR pName,
    PDWORD pcName,
    PDWORD pReserved,
    PWSTR pClass,
    PDWORD pcClass,
    PFILETIME pftLastWriteTime
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvEnumKeyEx(
            Handle,
            hKey,
            dwIndex,
            pName,
            pcName,
            pReserved,
            pClass,
            pcClass,
            pftLastWriteTime);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

DWORD
RegSrvSetValueEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_WRITE))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvSetValueEx(
            Handle,
            hKey,
            pValueName,
            Reserved,
            dwType,
            pData,
            cbData);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

DWORD
RegSrvGetValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL DWORD dwFlags,
    OUT PDWORD pdwType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvGetValue(
            Handle,
            hKey,
            pSubKey,
            pValue,
            dwFlags,
            pdwType,
            pData,
            pcbData);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;

}

DWORD
RegSrvGetValueA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL DWORD dwFlags,
    OUT PDWORD pdwType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvGetValueA(
            Handle,
            hKey,
            pSubKey,
            pValue,
            dwFlags,
            pdwType,
            pData,
            pcbData);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;

}

DWORD
RegSrvGetValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL DWORD dwFlags,
    OUT PDWORD pdwType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvGetValueW(
            Handle,
            hKey,
            pSubKey,
            pValue,
            dwFlags,
            pdwType,
            pData,
            pcbData);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

DWORD
RegSrvEnumValue(
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

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvEnumValue(
            Handle,
            hKey,
            dwIndex,
            pValueName,
            pcchValueName,
            pReserved,
            pType,
            pData,
            pcbData);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;

}

DWORD
RegSrvEnumValueA(
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

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvEnumValueA(
            Handle,
            hKey,
            dwIndex,
            pValueName,
            pcchValueName,
            pReserved,
            pType,
            pData,
            pcbData);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;

}

DWORD
RegSrvEnumValueW(
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

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvEnumValueW(
            Handle,
            hKey,
            dwIndex,
            pValueName,
            pcchValueName,
            pReserved,
            pType,
            pData,
            pcbData);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;

}

DWORD
RegSrvQueryInfoKey(
    HANDLE Handle,
    HKEY hKey,
    PWSTR pClass,
    PDWORD pcClass,
    PDWORD pReserved,
    PDWORD pcSubKeys,
    PDWORD pcMaxSubKeyLen,
    PDWORD pcMaxClassLen,
    PDWORD pcValues,
    PDWORD pcMaxValueNameLen,
    PDWORD pcMaxValueLen,
    PDWORD pcbSecurityDescriptor,
    PFILETIME pftLastWriteTime
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvQueryInfoKey(
            Handle,
            hKey,
            pClass,
            pcClass,
            pReserved,
            pcSubKeys,
            pcMaxSubKeyLen,
            pcMaxClassLen,
            pcValues,
            pcMaxValueNameLen,
            pcMaxValueLen,
            pcbSecurityDescriptor,
            pftLastWriteTime);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

DWORD
RegSrvQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValue,
    OUT OPTIONAL PDWORD pdwTotalsize
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvQueryMultipleValues(
            Handle,
            hKey,
            pVal_list,
            num_vals,
            pValue,
            pdwTotalsize);
    BAIL_ON_REG_ERROR(dwError);

error:
    return dwError;
}

DWORD
RegSrvQueryValueEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT PDWORD pType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvQueryValueEx(
            Handle,
            hKey,
            pValueName,
            pReserved,
            pType,
            pData,
            pcbData);
    BAIL_ON_REG_ERROR(dwError);

error:
    return dwError;
}

DWORD
RegSrvQueryValueExA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT PDWORD pType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvQueryValueExA(
            Handle,
            hKey,
            pValueName,
            pReserved,
            pType,
            pData,
            pcbData);
    BAIL_ON_REG_ERROR(dwError);

error:
    return dwError;
}

DWORD
RegSrvQueryValueExW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT PDWORD pType,
    OUT PBYTE pData,
    IN OUT PDWORD pcbData
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_READ))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvQueryValueExW(
            Handle,
            hKey,
            pValueName,
            pReserved,
            pType,
            pData,
            pcbData);
    BAIL_ON_REG_ERROR(dwError);


error:
    return dwError;
}

DWORD
RegSrvDeleteTree(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    )
{
    DWORD dwError = 0;

    if (!RegSrvCheckAccessRight(Handle, REG_WRITE))
    {
        dwError = LW_ERROR_ACCESS_DENIED;
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = gpRegProvider->pfnRegSrvDeleteTree(
            Handle,
            hKey,
            pSubKey);
    BAIL_ON_REG_ERROR(dwError);

error:
    return dwError;
}

/*Helper functions*/
void
RegSafeFreeKeyContext(
    IN OUT PREG_KEY_CONTEXT* ppKeyResult
    )
{
    PREG_KEY_CONTEXT pKeyResult = NULL;

    if (ppKeyResult != NULL && *ppKeyResult != NULL)
    {
        pKeyResult = *ppKeyResult;

        LW_SAFE_FREE_STRING(pKeyResult->pszKeyName);
        LW_SAFE_FREE_STRING(pKeyResult->pszParentKeyName);

        pKeyResult->bHasSubKeyInfo = FALSE;

        LwFreeStringArray(pKeyResult->ppszSubKeyNames, pKeyResult->dwNumSubKeys);
        LwFreeStringArray(pKeyResult->ppszValueNames, pKeyResult->dwNumValues);
        LwFreeStringArray(pKeyResult->ppszValues, pKeyResult->dwNumValues);

        LW_SAFE_FREE_MEMORY(pKeyResult->pTypes);

        if (pKeyResult->pMutex)
        {
            pthread_rwlock_destroy(&pKeyResult->mutex);
        }

        LW_SAFE_FREE_MEMORY(pKeyResult);
        *ppKeyResult = NULL;
    }
}


DWORD
RegSrvGetKeyRefCount(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD refCount = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    refCount = pKeyResult->refCount;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return refCount;
}

void
RegSrvSetHasSubKeyInfo(
    IN BOOLEAN bHasSubKeyInfo,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    pKeyResult->bHasSubKeyInfo = bHasSubKeyInfo;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return;
}

BOOLEAN
RegSrvHasSubKeyInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bHasSubKeyInfo = FALSE;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    bHasSubKeyInfo = pKeyResult->bHasSubKeyInfo;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return bHasSubKeyInfo;
}

DWORD
RegSrvSubKeyNum(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD dwSubKeyCount = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    dwSubKeyCount = pKeyResult->dwNumSubKeys;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwSubKeyCount;
}

size_t
RegSrvSubKeyNameMaxLen(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    size_t sSubKeyNameMaxLen = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    sSubKeyNameMaxLen = pKeyResult->sMaxSubKeyLen;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return sSubKeyNameMaxLen;
}

PCSTR
RegSrvSubKeyName(
    IN PREG_KEY_CONTEXT pKeyResult,
    IN DWORD dwIndex
    )
{
    BOOLEAN bInLock = FALSE;
    PCSTR pszSubKeyName = NULL;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    pszSubKeyName = pKeyResult->ppszSubKeyNames[dwIndex];

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return pszSubKeyName;
}

void
RegSrvSetHasValueInfo(
    IN BOOLEAN bHasValueInfo,
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;

    LWREG_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pKeyResult->mutex);

    pKeyResult->bHasValueInfo = bHasValueInfo;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return;
}

BOOLEAN
RegSrvHasValueInfo(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bHasValueInfo = FALSE;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    bHasValueInfo = pKeyResult->bHasValueInfo;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return bHasValueInfo;
}

DWORD
RegSrvValueNum(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    DWORD dwValueCount = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    dwValueCount = pKeyResult->dwNumValues;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return dwValueCount;
}

size_t
RegSrvMaxValueNameLen(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    size_t sMaxValueNameLen = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    sMaxValueNameLen = pKeyResult->sMaxValueNameLen;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return sMaxValueNameLen;
}

size_t
RegSrvMaxValueLen(
    IN PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;
    size_t sMaxValueLen = 0;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    sMaxValueLen = pKeyResult->sMaxValueLen;

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return sMaxValueLen;
}

PCSTR
RegSrvValueName(
    IN PREG_KEY_CONTEXT pKeyResult,
    DWORD dwIndex
    )
{
    BOOLEAN bInLock = FALSE;
    PCSTR pszValueName = NULL;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    pszValueName = pKeyResult->ppszValueNames[dwIndex];

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return pszValueName;
}

PCSTR
RegSrvValueContent(
    IN PREG_KEY_CONTEXT pKeyResult,
    DWORD dwIndex
    )
{
    BOOLEAN bInLock = FALSE;
    PCSTR pszValue = NULL;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    pszValue = pKeyResult->ppszValues[dwIndex];

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return pszValue;
}

REG_DATA_TYPE
RegSrvValueType(
    IN PREG_KEY_CONTEXT pKeyResult,
    DWORD dwIndex
    )
{
    BOOLEAN bInLock = FALSE;
    REG_DATA_TYPE type = REG_UNKNOWN;

    LWREG_LOCK_RWMUTEX_SHARED(bInLock, &pKeyResult->mutex);

    type = pKeyResult->pTypes[dwIndex];

    LWREG_UNLOCK_RWMUTEX(bInLock, &pKeyResult->mutex);

    return type;
}


#if 0
void
RegSrvSafeInsertSubKey(
    IN OUT PREG_KEY_CONTEXT pKeyResult
    )
{
    BOOLEAN bInLock = FALSE;

    ENTER_ACTIVE_KEY_CONTEXT_WRITER_LOCK(bInLock, pKeyResult->pActiveRegKeyContext_rwlock);

    if (!pKeyResult)
    {
        goto cleanup;
    }

    pKeyResult->refCount++;

cleanup:
    LEAVE_ACTIVE_KEY_CONTEXT_WRITER_LOCK(bInLock, pKeyResult->pActiveRegKeyContext_rwlock);

    return;
}
#endif
