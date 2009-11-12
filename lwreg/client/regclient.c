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
RegCreateKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszSubKey,
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
    PWSTR pwszSubKey = NULL;

    if (pszSubKey)
    {
        dwError = LwMbsToWc16s(pszSubKey, &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegTransactCreateKeyExW(
        hRegConnection,
        hKey,
        pwszSubKey,
        Reserved,
        pClass,
        dwOptions,
        samDesired,
        pSecurityAttributes,
        phkResult,
        pdwDisposition
        );
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszSubKey);

    return dwError;

error:
    goto cleanup;
}

REG_API
DWORD
RegCreateKeyExW(
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
    return RegTransactCreateKeyExW(
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
RegDeleteKeyA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszSubKey
    )
{
    DWORD dwError = 0;
    PWSTR pwszSubKey = NULL;

    BAIL_ON_INVALID_STRING(pszSubKey);

    dwError = LwMbsToWc16s(pszSubKey,
                           &pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegTransactDeleteKeyW(
        hRegConnection,
        hKey,
        pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszSubKey);

    return dwError;

error:
    goto cleanup;
}

REG_API
DWORD
RegDeleteKeyW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    )
{
    return RegTransactDeleteKeyW(
        hRegConnection,
        hKey,
        pSubKey
        );
}

REG_API
DWORD
RegDeleteKeyValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValueName
    )
{
    DWORD dwError = 0;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszValueName = NULL;

    if (pszSubKey)
    {
        dwError = LwMbsToWc16s(pszSubKey,
                               &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

    if (pszValueName)
    {
        dwError = LwMbsToWc16s(pszValueName,
                               &pwszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }

    dwError = RegTransactDeleteKeyValueW(
                           hRegConnection,
                           hKey,
                           pwszSubKey,
                           pwszValueName);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszSubKey);
    LW_SAFE_FREE_MEMORY(pwszValueName);

    return dwError;

error:
    goto cleanup;
}

REG_API
DWORD
RegDeleteKeyValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    )
{
    return RegTransactDeleteKeyValueW(
        hRegConnection,
        hKey,
        pSubKey,
        pValueName
        );
}

REG_API
DWORD
RegDeleteTreeA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey
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

    dwError = RegTransactDeleteTreeW(
                               hRegConnection,
                               hKey,
                               pwszSubKey);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszSubKey);
    return dwError;

error:
    goto cleanup;
}

REG_API
DWORD
RegDeleteTreeW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    )
{
    return RegTransactDeleteTreeW(
        hRegConnection,
        hKey,
        pSubKey
        );
}

REG_API
DWORD
RegDeleteValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCSTR pszValueName
    )
{
    DWORD dwError = 0;
    PWSTR pwszValueName = NULL;

    BAIL_ON_INVALID_STRING(pszValueName);

    dwError = LwMbsToWc16s(pszValueName,
                           &pwszValueName);
    BAIL_ON_REG_ERROR(dwError);

    dwError = RegTransactDeleteValueW(
        hRegConnection,
        hKey,
        pwszValueName
        );
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszValueName);

    return dwError;
error:
    goto cleanup;
}

REG_API
DWORD
RegDeleteValueW(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pValueName
    )
{
    return RegTransactDeleteValueW(
        hRegConnection,
        hKey,
        pValueName
        );
}

REG_API
DWORD
RegEnumKeyExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PSTR pszName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT PSTR pszClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    )
{
    DWORD dwError = 0;
    PWSTR pwszName = NULL;
    PSTR pszKeyName = NULL;
    PWSTR pwszClass = NULL;

    if (*pcName == 0)
    {
		dwError = LW_ERROR_INSUFFICIENT_BUFFER;
		BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwAllocateMemory(*pcName*sizeof(WCHAR), (LW_PVOID*)&pwszName);
    BAIL_ON_REG_ERROR(dwError);

    if (pcClass)
    {
        if (*pcClass == 0)
        {
		dwError = LW_ERROR_INSUFFICIENT_BUFFER;
		BAIL_ON_REG_ERROR(dwError);
        }

	dwError = LwAllocateMemory(*pcClass*sizeof(WCHAR), (LW_PVOID*)&pwszName);
        BAIL_ON_REG_ERROR(dwError);

        dwError = LwAllocateMemory(*pcClass*sizeof(WCHAR), (LW_PVOID*)&pwszClass);
        BAIL_ON_REG_ERROR(dwError);
    }

	dwError = RegTransactEnumKeyExW(
        hRegConnection,
        hKey,
        dwIndex,
        pwszName,
        pcName,
        pReserved,
        pwszClass,
        pcClass,
        pftLastWriteTime
        );
	BAIL_ON_REG_ERROR(dwError);

	dwError = LwWc16sToMbs((PCWSTR)pwszName, &pszKeyName);
	BAIL_ON_REG_ERROR(dwError);

	if (*pcName < strlen(pszKeyName))
	{
		dwError = LW_ERROR_INSUFFICIENT_BUFFER;
		BAIL_ON_REG_ERROR(dwError);
	}

	memcpy((PBYTE)pszName, (PBYTE)pszKeyName, strlen(pszKeyName));
	*pcName = strlen(pszKeyName);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszName);
    LW_SAFE_FREE_MEMORY(pwszClass);
    LW_SAFE_FREE_STRING(pszKeyName);

    return dwError;

error:
    goto cleanup;
}

REG_API
DWORD
RegEnumKeyExW(
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
    return RegTransactEnumKeyExW(
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
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    DWORD dwType = REG_UNKNOWN;
    PWSTR pwszValueName = NULL;
    PSTR pszTempValueName = NULL;
    PVOID pTempData = NULL;
    DWORD cTempData = 0;
    PBYTE pValue = NULL;

    if (*pcchValueName == 0)
    {
		dwError = LW_ERROR_INSUFFICIENT_BUFFER;
		BAIL_ON_REG_ERROR(dwError);
    }

    if (pData && !pcbData)
    {
	dwError = LW_ERROR_INVALID_PARAMETER;
	BAIL_ON_REG_ERROR(dwError);
    }

    dwError = LwAllocateMemory(*pcchValueName*sizeof(WCHAR), (LW_PVOID*)&pwszValueName);
    BAIL_ON_REG_ERROR(dwError);

    if (pData && pcbData)
    {
	cTempData = (*pcbData)*sizeof(WCHAR);

	if (cTempData > MAX_VALUE_LENGTH)
	{
		cTempData = MAX_VALUE_LENGTH;
	}

		dwError = LwAllocateMemory(cTempData*sizeof(WCHAR), &pTempData);
		BAIL_ON_REG_ERROR(dwError);
    }

	dwError = RegTransactEnumValueW(
	        hRegConnection,
	        hKey,
	        dwIndex,
	        pwszValueName,
	        pcchValueName,
	        pReserved,
	        &dwType,
	        pTempData,
	        &cTempData);
	BAIL_ON_REG_ERROR(dwError);

	if (!pTempData)
	{
		goto done;
	}

	if (REG_SZ == dwType)
	{
		dwError = LwWc16sToMbs((PCWSTR)pTempData, (PSTR*)&pValue);
		BAIL_ON_REG_ERROR(dwError);

		cTempData = strlen((PSTR)pValue) + 1;
	}
	else if (REG_MULTI_SZ == dwType)
	{
		dwError = RegConvertByteStreamW2A((PBYTE)pTempData,
				                           cTempData,
				                           &pValue,
				                           &cTempData);
		BAIL_ON_REG_ERROR(dwError);
	}
	else
	{
		dwError = LwAllocateMemory(cTempData, (LW_PVOID*)&pValue);
		BAIL_ON_REG_ERROR(dwError);

		memcpy(pValue, pTempData, cTempData);
	}

    if (pData)
    {
	memcpy(pData, pValue, cTempData);
    }

done:
    dwError = LwWc16sToMbs((PCWSTR)pwszValueName, &pszTempValueName);
    BAIL_ON_REG_ERROR(dwError);

    if (*pcchValueName < strlen(pszTempValueName))
    {
	    dwError = LW_ERROR_INSUFFICIENT_BUFFER;
	    BAIL_ON_REG_ERROR(dwError);
    }

    memcpy((PBYTE)pszValueName, (PBYTE)pszTempValueName, strlen(pszTempValueName));
    *pcchValueName = strlen(pszTempValueName);

    if (pdwType)
    {
        *pdwType = dwType;
    }

    if (pcbData)
    {
	*pcbData = cTempData;
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pwszValueName);
    LW_SAFE_FREE_MEMORY(pTempData);
    LW_SAFE_FREE_MEMORY(pValue);
    LW_SAFE_FREE_STRING(pszTempValueName);

    return dwError;

error:
    if (pdwType)
    {
        *pdwType = REG_UNKNOWN;
    }
    if (pcbData)
    {
	    *pcbData = 0;
    }
    *pcchValueName = 0;

    goto cleanup;
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
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    )
{
    DWORD dwError = 0;
    DWORD dwType = REG_UNKNOWN;
    PWSTR pwszSubKey = NULL;
    PWSTR pwszValueName = NULL;
    PVOID pTempData = NULL;
    DWORD cTempData = 0;
    PBYTE pValue = NULL;

    if (pvData && !pcbData)
    {
	dwError = LW_ERROR_INVALID_PARAMETER;
	BAIL_ON_REG_ERROR(dwError);
    }

    if (pszSubKey)
    {
	dwError = LwMbsToWc16s(pszSubKey, &pwszSubKey);
	BAIL_ON_REG_ERROR(dwError);
    }

    if (pszValueName)
    {
	dwError = LwMbsToWc16s(pszValueName, &pwszValueName);
	BAIL_ON_REG_ERROR(dwError);
    }

    if (pvData && pcbData)
    {
	cTempData = (*pcbData)*sizeof(WCHAR);

	if (cTempData > MAX_VALUE_LENGTH)
	{
		cTempData = MAX_VALUE_LENGTH;
	}

		dwError = LwAllocateMemory(cTempData*sizeof(WCHAR), &pTempData);
		BAIL_ON_REG_ERROR(dwError);
    }

	dwError = RegTransactGetValueW(
        hRegConnection,
        hKey,
        pwszSubKey,
        pwszValueName,
        Flags,
        &dwType,
        pTempData,
        &cTempData);
	BAIL_ON_REG_ERROR(dwError);

	if (!pTempData)
	{
		goto done;
	}

	if (REG_SZ == dwType)
	{
		dwError = LwWc16sToMbs((PCWSTR)pTempData, (PSTR*)&pValue);
		BAIL_ON_REG_ERROR(dwError);

		cTempData = strlen((PSTR)pValue) + 1;
	}
	else if (REG_MULTI_SZ == dwType)
	{
		dwError = RegConvertByteStreamW2A((PBYTE)pTempData,
				                           cTempData,
				                           &pValue,
				                           &cTempData);
		BAIL_ON_REG_ERROR(dwError);
	}
	else
	{
		dwError = LwAllocateMemory(cTempData, (LW_PVOID*)&pValue);
		BAIL_ON_REG_ERROR(dwError);

		memcpy(pValue, pTempData, cTempData);
	}

    if (pvData)
    {
	memcpy(pvData, pValue, cTempData);
    }

done:
    if (pdwType)
    {
        *pdwType = dwType;
    }

    if (pcbData)
    {
	*pcbData = cTempData;
    }

cleanup:
    LW_SAFE_FREE_MEMORY(pwszSubKey);
    LW_SAFE_FREE_MEMORY(pwszValueName);
    LW_SAFE_FREE_MEMORY(pTempData);
    LW_SAFE_FREE_MEMORY(pValue);

    return dwError;

error:
    if (pdwType)
    {
        *pdwType = REG_UNKNOWN;
    }

    if (pcbData)
    {
	    *pcbData = 0;
    }

    goto cleanup;
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
    DWORD dwError = 0;
    PWSTR pwszSubKey = NULL;

    if (pszSubKey)
    {
        dwError = LwMbsToWc16s(pszSubKey,
                               &pwszSubKey);
        BAIL_ON_REG_ERROR(dwError);
    }

	dwError = RegTransactOpenKeyExW(
        hRegConnection,
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
RegQueryInfoKeyA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    OUT PSTR pszClass,
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
    return RegTransactQueryInfoKeyA(
        hRegConnection,
        hKey,
        pszClass,
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
RegQueryInfoKeyW(
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
    return RegTransactQueryInfoKeyW(
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
	return RegGetValueA(hRegConnection,
			            hKey,
			            NULL,
			            pszValueName,
			            RRF_RT_REG_NONE,
			            pType,
			            pData,
			            pcbData);
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
	return RegGetValueW(hRegConnection,
			            hKey,
			            NULL,
			            pValueName,
			            RRF_RT_REG_NONE,
			            pType,
			            pData,
			            pcbData);
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
    DWORD dwError = 0;
    PWSTR pwszValueName = NULL;
    PBYTE   pOutData = NULL;
    DWORD   cbOutDataLen = 0;
    BOOLEAN bIsStrType = FALSE;

    if (pszValueName)
    {
        dwError = LwMbsToWc16s(pszValueName,
                               &pwszValueName);
        BAIL_ON_REG_ERROR(dwError);
    }

    if (pData)
    {
	if (REG_MULTI_SZ == dwType)
	{
		dwError = RegConvertByteStreamA2W((PBYTE)pData,
				                          cbData,
				                          &pOutData,
				                          &cbOutDataLen);
		BAIL_ON_REG_ERROR(dwError);

		bIsStrType = TRUE;
	}
	else if (REG_SZ == dwType)
	{
		dwError = LwMbsToWc16s((PCSTR)pData, (PWSTR*)&pOutData);
		BAIL_ON_REG_ERROR(dwError);

		cbOutDataLen = cbData*sizeof(WCHAR);

		bIsStrType = TRUE;
	}
    }

    dwError = RegTransactSetValueExW(
            hRegConnection,
            hKey,
            pwszValueName,
            Reserved,
            dwType,
            bIsStrType ? pOutData : pData,
            bIsStrType ? cbOutDataLen : cbData);
    BAIL_ON_REG_ERROR(dwError);

cleanup:
    LW_SAFE_FREE_MEMORY(pwszValueName);
    LW_SAFE_FREE_MEMORY(pOutData);

    return dwError;
error:
    goto cleanup;
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

