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
 *        adcache.c
 *
 * Abstract:
 *
 *        This is the public interface for the AD Provider Local Cache
 *
 * Authors: Kyle Stemen (kstemen@likewisesoftware.com)
 *          Krishna Ganugapati (krishnag@likewisesoftware.com)
 *
 */

#ifndef __REGPROVSPI_H__
#define __REGPROVSPI_H__

typedef
DWORD
(*PFNRegSrvOpenRootKey)(
    IN HANDLE Handle,
    IN PSTR pszRootKeyName,
    OUT PHKEY phkResult
    );

typedef
DWORD
(*PFNRegSrvCreateKeyEx)(
    HANDLE Handle,
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

typedef
VOID
(*PFNRegSrvCloseKey)(
    HKEY hKey
    );

typedef
DWORD
(*PFNRegSrvDeleteKey)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    );

typedef
DWORD
(*PFNRegSrvDeleteKeyValue)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValueName
    );

typedef
DWORD
(*PFNRegSrvDeleteValue)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpValueName
    );

typedef
DWORD
(*PFNRegSrvDeleteTree)(
    HANDLE Handle,
    HKEY hKey,
    OPTIONAL PCWSTR pSubKey
    );

typedef
DWORD
(*PFNRegSrvEnumKeyEx)(
    HANDLE Handle,
    HKEY hKey,
    DWORD dwIndex,
    PWSTR pName,
    PDWORD pcName,
    PDWORD pReserved,
    PWSTR pClass,
    PDWORD pcClass,
    PFILETIME pftLastWriteTime
    );

typedef
DWORD
(*PFNRegSrvEnumValueA)(
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

typedef
DWORD
(*PFNRegSrvEnumValueW)(
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

typedef
DWORD
(*PFNRegSrvGetValueA)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpSubKey,
    PCWSTR lpValue,
    REG_DATA_TYPE_FLAGS Flags,
    PDWORD pdwType,
    PBYTE pvData,
    PDWORD pcbData
    );

typedef
DWORD
(*PFNRegSrvGetValueW)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpSubKey,
    PCWSTR lpValue,
    REG_DATA_TYPE_FLAGS Flags,
    PDWORD pdwType,
    PBYTE pvData,
    PDWORD pcbData
    );

typedef
DWORD
(*PFNRegSrvOpenKeyEx)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR lpSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

typedef
DWORD
(*PFNRegSrvQueryInfoKey)(
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
    );
typedef
DWORD
(*PFNRegSrvQueryMultipleValues)(
    HANDLE Handle,
    HKEY hKey,
    PVALENT val_list,
    DWORD num_vals,
    PWSTR pValueBuf,
    PDWORD dwTotsize
    );

typedef
DWORD
(*PFNRegSrvQueryValueExA)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );

typedef
DWORD
(*PFNRegSrvQueryValueExW)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );

typedef
DWORD
(*PFNRegSrvSetValueExA)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *pData,
    DWORD cbData
    );

typedef
DWORD
(*PFNRegSrvSetValueExW)(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *pData,
    DWORD cbData
    );

typedef struct __REGPROV_PROVIDER_FUNCTION_TABLE
{
    PFNRegSrvOpenRootKey         pfnRegSrvOpenRootKey;
    PFNRegSrvCreateKeyEx         pfnRegSrvCreateKeyEx;
    PFNRegSrvCloseKey            pfnRegSrvCloseKey;
    PFNRegSrvDeleteKey           pfnRegSrvDeleteKey;
    PFNRegSrvDeleteKeyValue      pfnRegSrvDeleteKeyValue;
    PFNRegSrvDeleteValue         pfnRegSrvDeleteValue;
    PFNRegSrvDeleteTree          pfnRegSrvDeleteTree;
    PFNRegSrvEnumKeyEx           pfnRegSrvEnumKeyEx;
    PFNRegSrvEnumValueA          pfnRegSrvEnumValueA;
    PFNRegSrvEnumValueW          pfnRegSrvEnumValueW;
    PFNRegSrvGetValueA           pfnRegSrvGetValueA;
    PFNRegSrvGetValueW           pfnRegSrvGetValueW;
    PFNRegSrvOpenKeyEx           pfnRegSrvOpenKeyEx;
    PFNRegSrvQueryInfoKey        pfnRegSrvQueryInfoKey;
    PFNRegSrvQueryMultipleValues pfnRegSrvQueryMultipleValues;
    PFNRegSrvQueryValueExA       pfnRegSrvQueryValueExA;
    PFNRegSrvQueryValueExW       pfnRegSrvQueryValueExW;
    PFNRegSrvSetValueExA         pfnRegSrvSetValueExA;
    PFNRegSrvSetValueExW         pfnRegSrvSetValueExW;
} REGPROV_PROVIDER_FUNCTION_TABLE, *PREGPROV_PROVIDER_FUNCTION_TABLE;

typedef
DWORD
(*PFNRegSrvInit)(
    PREGPROV_PROVIDER_FUNCTION_TABLE* ppFnTable
    );

typedef
VOID
(*PFNRegSrvShutdown)(
    PREGPROV_PROVIDER_FUNCTION_TABLE pFnTable
    );


#define REGPROV_SYMBOL_NAME_INITIALIZE_PROVIDER "RegProvInitializeProvider"

typedef DWORD (*PFNRegSrvInitializeRegProvider)(
                    PCSTR pszConfigFilePath,
                    PSTR* ppszProviderName,
                    PREGPROV_PROVIDER_FUNCTION_TABLE* ppFnTable
                    );

#define REGPROV_SYMBOL_NAME_SHUTDOWN_PROVIDER "RegProvShutdownProvider"

typedef DWORD (*PFNRegSrvShutdownRegProvider)(
                    PSTR pszProviderName,
                    PREGPROV_PROVIDER_FUNCTION_TABLE pFnTable
                    );

#endif /* __REGPROVSPI_H__ */
