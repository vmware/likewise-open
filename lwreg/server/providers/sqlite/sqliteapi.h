/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        sqliteapi.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        LSA Server API (Private Header)
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */

#ifndef SQLITEAPI_H_
#define SQLITEAPI_H_

DWORD
SqliteProvider_Initialize(
    PREGPROV_PROVIDER_FUNCTION_TABLE* ppFnTable
    );

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
    );

DWORD
SqliteOpenRootKey(
    IN HANDLE Handle,
    IN PSTR pszRootKeyName,
    OUT PHKEY phkResult
    );

DWORD
SqliteOpenKeyEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN DWORD ulOptions,
    IN REGSAM samDesired,
    OUT PHKEY phkResult
    );

VOID
SqliteCloseKey(
    IN HKEY hKey
    );

DWORD
SqliteDeleteKey(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pSubKey
    );

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
    );

DWORD
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
    );

DWORD
SqliteSetValueEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN const BYTE *pData,
    DWORD cbData
    );

DWORD
SqliteGetValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL DWORD dwFlags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
SqliteGetValueA(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL DWORD dwFlags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
SqliteGetValueW(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL DWORD dwFlags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
SqliteDeleteKeyValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    );

DWORD
SqliteQueryMultipleValues(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OUT PVALENT pVal_list,
    IN DWORD num_vals,
    OUT PWSTR pValue,
    OUT PDWORD pdwTotalsize
    );

DWORD
SqliteDeleteValue(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    );









DWORD
SqliteEnumValue(
    HANDLE Handle,
    HKEY hKey,
    DWORD dwIndex,
    PWSTR pValueName,
    PDWORD pcchValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );


DWORD
SqliteQueryValueEx(
    IN HANDLE Handle,
    IN HKEY hKey,
    IN PCWSTR pValueName,
    IN PDWORD pReserved,
    OUT PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
SqliteDeleteTree(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    );

#endif /* SQLITEAPI_H_ */
