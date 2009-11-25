/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        lwntreg.h
 *
 * Abstract:
 *
 *        NtRegistry
 *
 *        Public NT Client API
 *
 * Authors: Wei Fu (wfu@likewisesoftware.com)
 */
#ifndef __LWNTREG_H__
#define __LWNTREG_H__

#include <lw/types.h>
#include <lw/attrs.h>
#include <reg/reg.h>


NTSTATUS
LwNtRegOpenServer(
    PHANDLE phConnection
    );

VOID
LwNtRegCloseServer(
    HANDLE hConnection
    );

NTSTATUS
LwNtRegEnumRootKeysA(
    IN HANDLE hNtRegConnection,
    OUT PSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

NTSTATUS
LwNtRegEnumRootKeysW(
    IN HANDLE hNtRegConnection,
    OUT PWSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

NTSTATUS
LwNtRegCreateKeyExA(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PCSTR pszSubKey,
    DWORD Reserved,
    PWSTR pClass,
    DWORD dwOptions,
    REGSAM samDesired,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHKEY phkResult,
    PDWORD pdwDisposition
    );

NTSTATUS
LwNtRegCreateKeyExW(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PCWSTR pSubKey,
    DWORD Reserved,
    PWSTR pClass,
    DWORD dwOptions,
    REGSAM samDesired,
    PSECURITY_ATTRIBUTES pSecurityAttributes,
    PHKEY phkResult,
    PDWORD pdwDisposition
    );

NTSTATUS
LwNtRegCloseKey(
    HANDLE hNtRegConnection,
    HKEY hKey
    );

NTSTATUS
LwNtRegDeleteKeyA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN PCSTR pszSubKey
    );

NTSTATUS
LwNtRegDeleteKeyW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    );

NTSTATUS
LwNtRegDeleteKeyValueA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValueName
    );

NTSTATUS
LwNtRegDeleteKeyValueW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    );

NTSTATUS
LwNtRegDeleteTreeA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey
    );

NTSTATUS
LwNtRegDeleteTreeW(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PCWSTR pSubKey
    );

NTSTATUS
LwNtRegDeleteValueA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN PCSTR pszValueName
    );

NTSTATUS
LwNtRegDeleteValueW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN PCWSTR pValueName
    );

NTSTATUS
LwNtRegEnumKeyExA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PSTR pszName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT OPTIONAL PSTR pszClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    );

NTSTATUS
LwNtRegEnumKeyExW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    IN OUT PWSTR pName,
    IN OUT PDWORD pcName,
    IN PDWORD pReserved,
    IN OUT PWSTR pClass,
    IN OUT OPTIONAL PDWORD pcClass,
    OUT OPTIONAL PFILETIME pftLastWriteTime
    );

NTSTATUS
LwNtRegEnumValueA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PSTR pszValueName,
    IN OUT PDWORD pcchValueName,
    IN OPTIONAL PDWORD pReserved,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
LwNtRegEnumValueW(
    HANDLE hNtRegConnection,
    HKEY hKey,
    DWORD dwIndex,
    PWSTR pValueName,
    PDWORD pcchValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );

NTSTATUS
LwNtRegGetValueA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pSubKey,
    IN OPTIONAL PCSTR pszValueName,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
LwNtRegGetValueW(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
LwNtRegOpenKeyExA(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PCSTR pszSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

NTSTATUS
LwNtRegOpenKeyExW(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PCWSTR pwszSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

NTSTATUS
LwNtRegQueryInfoKeyA(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PSTR pszClass,
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
    );

NTSTATUS
LwNtRegQueryInfoKeyW(
    HANDLE hNtRegConnection,
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
    );

NTSTATUS
LwNtRegQueryValueExA(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PCSTR pszValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );

NTSTATUS
LwNtRegQueryValueExW(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PCWSTR pValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );

NTSTATUS
LwNtRegSetValueExA(
    IN HANDLE hNtRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    );

NTSTATUS
LwNtRegSetValueExW(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PCWSTR pValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *pData,
    DWORD cbData
    );

NTSTATUS
LwNtRegQueryMultipleValues(
    HANDLE hNtRegConnection,
    HKEY hKey,
    PVALENT val_list,
    DWORD num_vals,
    PWSTR pValueBuf,
    PDWORD dwTotsize
    );

/* NtRegistry multi-str data type conversion functions */
NTSTATUS
LwNtRegMultiStrsToByteArrayW(
    PWSTR*   ppwszInMultiSz,
    PBYTE*   ppOutBuf,
    SSIZE_T* pOutBufLen
    );

NTSTATUS
LwNtRegMultiStrsToByteArrayA(
    PSTR*    ppszInMultiSz,
    PBYTE*   ppOutBuf,
    SSIZE_T* pOutBufLen
    );

NTSTATUS
LwNtRegByteArrayToMultiStrsW(
    PBYTE   pInBuf,
    SSIZE_T bufLen,
    PWSTR** pppwszStrings
    );

NTSTATUS
LwNtRegByteArrayToMultiStrsA(
    PBYTE   pInBuf,
    SSIZE_T bufLen,
    PSTR**  pppszStrings
    );

NTSTATUS
LwNtRegConvertByteStreamA2W(
    const PBYTE pData,
    DWORD       cbData,
    PBYTE*      ppOutData,
    PDWORD      pcbOutDataLen
    );

NTSTATUS
LwNtRegConvertByteStreamW2A(
    const PBYTE pData,
    DWORD       cbData,
    PBYTE*      ppOutData,
    PDWORD      pcbOutDataLen
    );



#ifndef LW_STRICT_NAMESPACE
#define NtRegOpenServer LwNtRegOpenServer
#define NtRegCloseServer LwNtRegCloseServer
#define NtRegEnumRootKeysA LwNtRegEnumRootKeysA
#define NtRegEnumRootKeysW LwNtRegEnumRootKeysW
#define NtRegEnumRootKeys LwNtRegEnumRootKeys
#define NtRegCreateKeyExA LwNtRegCreateKeyExA
#define NtRegCreateKeyExW LwNtRegCreateKeyExW
#define NtRegCreateKeyEx LwNtRegCreateKeyEx
#define NtRegCloseKey LwNtRegCloseKey
#define NtRegDeleteKeyA LwNtRegDeleteKeyA
#define NtRegDeleteKeyW LwNtRegDeleteKeyW
#define NtRegDeleteKey LwNtRegDeleteKey
#define NtRegDeleteKeyValueA LwNtRegDeleteKeyValueA
#define NtRegDeleteKeyValueW LwNtRegDeleteKeyValueW
#define NtRegDeleteKeyValue LwNtRegDeleteKeyValue
#define NtRegDeleteTreeA LwNtRegDeleteTreeA
#define NtRegDeleteTreeW LwNtRegDeleteTreeW
#define NtRegDeleteTree LwNtRegDeleteTree
#define NtRegDeleteValueA LwNtRegDeleteValueA
#define NtRegDeleteValueW LwNtRegDeleteValueW
#define NtRegDeleteValue LwNtRegDeleteValue
#define NtRegQueryValueExA LwNtRegQueryValueExA
#define NtRegQueryValueExW LwNtRegQueryValueExW
#define NtRegQueryValueEx LwNtRegQueryValueEx
#define NtRegEnumKeyExA LwNtRegEnumKeyExA
#define NtRegEnumKeyExW LwNtRegEnumKeyExW
#define NtRegEnumKeyEx LwNtRegEnumKeyEx
#define NtRegEnumValueA LwNtRegEnumValueA
#define NtRegEnumValueW LwNtRegEnumValueW
#define NtRegEnumValue LwNtRegEnumValue
#define NtRegGetValueA LwNtRegGetValueA
#define NtRegGetValueW LwNtRegGetValueW
#define NtRegGetValue LwNtRegGetValue
#define NtRegOpenKeyExA LwNtRegOpenKeyExA
#define NtRegOpenKeyExW LwNtRegOpenKeyExW
#define NtRegOpenKeyEx LwNtRegOpenKeyEx
#define NtRegQueryInfoKeyA LwNtRegQueryInfoKeyA
#define NtRegQueryInfoKeyW LwNtRegQueryInfoKeyW
#define NtRegQueryInfoKey LwNtRegQueryInfoKey
#define NtRegSetValueExA LwNtRegSetValueExA
#define NtRegSetValueExW LwNtRegSetValueExW
#define NtRegSetValueEx LwNtRegSetValueEx
#define NtRegMultiStrsToByteArrayA LwNtRegMultiStrsToByteArrayA
#define NtRegMultiStrsToByteArrayW LwNtRegMultiStrsToByteArrayW
#define NtRegMultiStrsToByteArray LwNtRegMultiStrsToByteArray
#define NtRegByteArrayToMultiStrsA LwNtRegByteArrayToMultiStrsA
#define NtRegByteArrayToMultiStrsW LwNtRegByteArrayToMultiStrsW
#define NtRegByteArrayToMultiStrs LwNtRegByteArrayToMultiStrs
#define NtRegConvertByteStreamA2W LwNtRegConvertByteStreamA2W
#define NtRegConvertByteStreamW2A LwNtRegConvertByteStreamW2A
#define NtRegQueryMultipleValues LwNtRegQueryMultipleValues

#endif /* ! LW_STRICT_NAMESPACE */


#ifdef UNICODE

#define LwNtRegEnumRootKeys(hNtRegConnection, pppwszRootKeyNames, pdwNumRootKeys) \
    LwNtRegEnumRootKeysW(hNtRegConnection, pppwszRootKeyNames, pdwNumRootKeys)

#define LwNtRegCreateKeyEx(hNtRegConnection, hKey, pwszSubKey, Reserved, pClass, dwOptions, samDesired, pSecurityAttributes, phkResult, pdwDisposition) \
    LwNtRegCreateKeyExW(hNtRegConnection, hKey, pwszSubKey, Reserved, pClass, dwOptions, samDesired, pSecurityAttributes, phkResult, pdwDisposition)

#define LwNtRegEnumKeyEx(hNtRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \
    LwNtRegEnumKeyExW(hNtRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \

#define LwNtRegEnumValue(hNtRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData) \
    LwNtRegEnumValueW(hNtRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData)

#define LwNtRegGetValue(hNtRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData) \
    LwNtRegGetValueW(hNtRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData)

#define LwNtRegQueryValueEx(hNtRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData) \
    LwNtRegQueryValueExW(hNtRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData)

#define LwNtRegSetValueEx(hNtRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData) \
    LwNtRegSetValueExW(hNtRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData)

#define LwNtRegOpenKeyEx(hNtRegConnection, hKey, pwszSubKey, ulOptions, samDesired, phkResult) \
    LwNtRegOpenKeyExW(hNtRegConnection, hKey, pwszSubKey, ulOptions, samDesired, phkResult)

#define LwNtRegMultiStrsToByteArray(ppszInMultiSz, outBuf, outBufLen) \
    LwNtRegMultiStrsToByteArrayW(ppszInMultiSz, outBuf, outBufLen)

#define LwNtRegByteArrayToMultiStrs(pInBuf, bufLen, pppszOutMultiSz) \
    LwNtRegByteArrayToMultiStrsW(pInBuf, bufLen, pppszOutMultiSz)

#define LwNtRegQueryInfoKey(hNtRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime) \
    LwNtRegQueryInfoKeyW(hNtRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime)

#define LwNtRegDeleteKey(hNtRegConnection, hKey, pwszSubKey) \
    LwNtRegDeleteKeyW(hNtRegConnection, hKey, pwszSubKey)

#define LwNtRegDeleteKeyValue(hNtRegConnection, hKey, pwszSubKey, pwszValueName) \
    LwNtRegDeleteKeyValueW(hNtRegConnection, hKNtRegCloseKeyey, pwszSubKey, pwszValueName)

#define LwNtRegDeleteTree(hNtRegConnection, hKey, pwszSubKey) \
    LwNtRegDeleteTreeW(hNtRegConnection, hKey, pwszSubKey)

#define LwNtRegDeleteValue(hNtRegConnection, hKey, pwszValueName) \
    LwNtRegDeleteValueW(hNtRegConnection, hKey, pwszValueName)

#else

#define LwNtRegEnumRootKeys(hNtRegConnection, pppszRootKeyNames, pdwNumRootKeys) \
	LwNtRegEnumRootKeysA(hNtRegConnection, pppszRootKeyNames, pdwNumRootKeys)

#define LwNtRegCreateKeyEx(hNtRegConnection, hKey, pszSubKey, Reserved, pClass, dwOptions, samDesired, pSecurityAttributes, phkResult, pdwDisposition) \
    LwNtRegCreateKeyExA(hNtRegConnection, hKey, pszSubKey, Reserved, pClass, dwOptions, samDesired, pSecurityAttributes, phkResult, pdwDisposition)

#define LwNtRegEnumKeyEx(hNtRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \
    LwNtRegEnumKeyExA(hNtRegConnection, hKey, dwIndex, pName, pcName, pReserved, pClass, pcClass,pftLastWriteTime) \

#define LwNtRegEnumValue(hNtRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData) \
    LwNtRegEnumValueA(hNtRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData)

#define LwNtRegGetValue(hNtRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData) \
    LwNtRegGetValueA(hNtRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData)

#define LwNtRegQueryValueEx(hNtRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData) \
    LwNtRegQueryValueExA(hNtRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData)

#define LwNtRegSetValueEx(hNtRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData) \
    LwNtRegSetValueExA(hNtRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData)

#define LwNtRegOpenKeyEx(hNtRegConnection, hKey, pszSubKey, ulOptions, samDesired, phkResult) \
    LwNtRegOpenKeyExA(hNtRegConnection, hKey, pszSubKey, ulOptions, samDesired, phkResult)

#define LwNtRegMultiStrsToByteArray(ppszInMultiSz, outBuf, outBufLen) \
    LwNtRegMultiStrsToByteArrayA(ppszInMultiSz, outBuf, outBufLen)

#define LwNtRegByteArrayToMultiStrs(pInBuf, bufLen, pppszOutMultiSz) \
    LwNtRegByteArrayToMultiStrsA(pInBuf, bufLen, pppszOutMultiSz)

#define LwNtRegQueryInfoKey(hNtRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime) \
    LwNtRegQueryInfoKeyA(hNtRegConnection, hKey, pszClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime)

#define LwNtRegDeleteKey(hNtRegConnection, hKey, pszSubKey) \
    LwNtRegDeleteKeyA(hNtRegConnection, hKey, pszSubKey)

#define LwNtRegDeleteKeyValue(hNtRegConnection, hKey, pszSubKey, pszValueName) \
    LwNtRegDeleteKeyValueA(hNtRegConnection, hKey, pszSubKey, pszValueName)

#define LwNtRegDeleteTree(hNtRegConnection, hKey, pszSubKey) \
    LwNtRegDeleteTreeA(hNtRegConnection, hKey, pszSubKey)

#define LwNtRegDeleteValue(hNtRegConnection, hKey, pszValueName) \
    LwNtRegDeleteValueA(hNtRegConnection, hKey, pszValueName)
#endif


#endif /* __LWNTREG_H__ */

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
