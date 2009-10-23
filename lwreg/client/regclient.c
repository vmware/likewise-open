/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

/*
 * Copyright Likewise Software    2004-2008
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the license, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.  You should have received a copy
 * of the GNU Lesser General Public License along with this program.  If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * LESSER GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        registry.c
 *
 * Abstract:
 *
 *        Registry
 *
 *        Client wrapper API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Kyle Stemen (kstemen@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#include "client.h"

REG_API
DWORD
RegEnumRootKeysA(
    IN HANDLE hRegConnection,
    OUT PSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    )
{
    DWORD dwError = 0;
    PWSTR* ppwszRootKeyNames = NULL;
    PSTR* ppszRootKeyNames = NULL;
    DWORD dwNumRootKeys = 0;
    int iCount = 0;

    dwError = RegTransactEnumRootKeysW(hRegConnection,
                                       &ppwszRootKeyNames,
                                       &dwNumRootKeys);
    BAIL_ON_REG_ERROR(dwError);

    if (!dwNumRootKeys)
        goto cleanup;

    dwError = LwAllocateMemory(sizeof(*ppszRootKeyNames)*dwNumRootKeys,
                               (PVOID)&ppszRootKeyNames);
    BAIL_ON_REG_ERROR(dwError);

    for (iCount = 0; iCount < dwNumRootKeys; iCount++)
    {
        dwError = LwWc16sToMbs(ppwszRootKeyNames[iCount],
                               &ppszRootKeyNames[iCount]);
        BAIL_ON_REG_ERROR(dwError);
    }

cleanup:
    *pppszRootKeyNames = ppszRootKeyNames;
    *pdwNumRootKeys = dwNumRootKeys;

    if (ppwszRootKeyNames)
    {
        for (iCount=0; iCount<dwNumRootKeys; iCount++)
        {
            LW_SAFE_FREE_MEMORY(ppwszRootKeyNames[iCount]);
        }
        ppwszRootKeyNames = NULL;
    }

    return dwError;

error:
    if (ppszRootKeyNames)
    {
        LwFreeStringArray(ppszRootKeyNames, dwNumRootKeys);
    }

    goto cleanup;
}

REG_API
DWORD
RegEnumRootKeysW(
    IN HANDLE hRegConnection,
    OUT PWSTR** pppwszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    )
{
    return RegTransactEnumRootKeysW(hRegConnection,
                                    pppwszRootKeyNames,
                                   pdwNumRootKeys);
}

REG_API
DWORD
RegCreateKeyEx(
    IN HANDLE hRegConnection,
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
    return RegTransactCreateKeyEx(
        hRegConnection,
        hKey,
        pSubKey,
        Reserved,
        pClass,
        dwOptions,
        samDesired,
        pSecurityAttributes,
        phkResult,
        pdwDisposition
        );
}

REG_API
DWORD
RegCloseKey(
    IN HANDLE hRegConnection,
    IN HKEY hKey
    )
{
    return RegTransactCloseKey(
        hRegConnection,
        hKey
        );
}

REG_API
DWORD
RegDeleteKey(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    return RegTransactDeleteKey(
        hRegConnection,
        hKey,
        pSubKey
        );
}


REG_API
DWORD
RegDeleteKeyValue(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    return RegTransactDeleteKeyValue(
        hRegConnection,
        hKey,
        pSubKey,
        pValueName
        );
}

REG_API
DWORD
RegDeleteTree(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
    return RegTransactDeleteTree(
        hRegConnection,
        hKey,
        pSubKey
        );
}

REG_API
DWORD
RegDeleteValue(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pValueName
    )
{
    return RegTransactDeleteValue(
        hRegConnection,
        hKey,
        pValueName
        );
}

REG_API
DWORD
RegEnumKeyEx(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    return RegTransactEnumKeyEx(
        hRegConnection,
        hKey,
        dwIndex,
        pName,
        pcName,
        pReserved,
        pClass,
        pcClass,
        pftLastWriteTime
        );
}


REG_API
DWORD
RegEnumValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PSTR pszValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegTransactEnumValueA(
        hRegConnection,
        hKey,
        dwIndex,
        pszValueName,
        pcchValueName,
        pReserved,
        pType,
        pData,
        pcbData
        );
}

REG_API
DWORD
RegEnumValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegTransactEnumValueW(
        hRegConnection,
        hKey,
        dwIndex,
        pValueName,
        pcchValueName,
        pReserved,
        pType,
        pData,
        pcbData
        );
}

REG_API
DWORD
RegGetValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pSubKey,
    IN OPTIONAL PCSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegTransactGetValueA(
        hRegConnection,
        hKey,
        pSubKey,
        pValue,
        Flags,
        pdwType,
        pvData,
        pcbData
        );
}

REG_API
DWORD
RegGetValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegTransactGetValueW(
        hRegConnection,
        hKey,
        pSubKey,
        pValue,
        Flags,
        pdwType,
        pvData,
        pcbData
        );
}

REG_API
DWORD
RegOpenKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    return RegTransactOpenKeyExA(
        hRegConnection,
        hKey,
        pszSubKey,
        ulOptions,
        samDesired,
        phkResult
        );
}

REG_API
DWORD
RegOpenKeyExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    )
{
    return RegTransactOpenKeyExW(
        hRegConnection,
        hKey,
        pSubKey,
        ulOptions,
        samDesired,
        phkResult
        );
}

REG_API
DWORD
RegQueryInfoKey(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pcSubKeys,
    OUT OPTIONAL PDWORD pcMaxSubKeyLen,
    OUT OPTIONAL PDWORD pcMaxClassLen,
    OUT OPTIONAL PDWORD pcValues,
    OUT OPTIONAL PDWORD pcMaxValueNameLen,
    OUT OPTIONAL PDWORD pcMaxValueLen,
    OUT OPTIONAL PDWORD pcbSecurityDescriptor,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    return RegTransactQueryInfoKey(
        hRegConnection,
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
        pftLastWriteTime
        );
}

REG_API
DWORD
RegQueryMultipleValues(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT PVALENT val_list,
    IN DWORD num_vals,
    OUT OPTIONAL PWSTR pValueBuf,
    IN OUT OPTIONAL PDWORD dwTotsize
    )
{
    return RegTransactQueryMultipleValues(
        hRegConnection,
        hKey,
        val_list,
        num_vals,
        pValueBuf,
        dwTotsize
        );
}

REG_API
DWORD
RegQueryValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegTransactQueryValueExA(
        hRegConnection,
        hKey,
        pszValueName,
        pReserved,
        pType,
        pData,
        pcbData
        );
}

REG_API
DWORD
RegQueryValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    return RegTransactQueryValueExW(
        hRegConnection,
        hKey,
        pValueName,
        pReserved,
        pType,
        pData,
        pcbData
        );
}

REG_API
DWORD
RegSetValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    )
{
    return RegTransactSetValueExA(
        hRegConnection,
        hKey,
        pszValueName,
        Reserved,
        dwType,
        pData,
        cbData
        );
}

REG_API
DWORD
RegSetValueExW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    )
{
    return RegTransactSetValueExW(
        hRegConnection,
        hKey,
        pValueName,
        Reserved,
        dwType,
        pData,
        cbData
        );
}

REG_API
DWORD
RegSetKeyValue(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR lpSubKey,
    IN OPTIONAL PCWSTR lpValueName,
    IN DWORD dwType,
    IN OPTIONAL PCVOID lpData,
    IN DWORD cbData
    )
{
    return RegTransactSetKeyValue(
        hRegConnection,
        hKey,
        lpSubKey,
        lpValueName,
        dwType,
        lpData,
        cbData
        );
}

