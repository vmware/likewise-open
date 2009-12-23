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
 *        clientipc_p.h
 *
 * Abstract:
 *
 *        Registry Subsystem
 *
 *        Private Header (Library)
 *
 *        Inter-process Communication (Client) API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#ifndef __CLIENTIPC_P_H__
#define __CLIENTIPC_P_H__

#include <lwmsg/lwmsg.h>

typedef struct __REG_CLIENT_CONNECTION_CONTEXT
{
    LWMsgProtocol* pProtocol;
    LWMsgAssoc* pAssoc;
} REG_CLIENT_CONNECTION_CONTEXT, *PREG_CLIENT_CONNECTION_CONTEXT;

NTSTATUS
RegTransactEnumRootKeysW(
    IN HANDLE hConnection,
    OUT PWSTR** pppwszRootKeyNames,
    OUT PDWORD pdwNumRootKey
    );

NTSTATUS
RegTransactOpenKeyExW(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pwszSubKey,
    DWORD ulOptions,
    ACCESS_MASK AccessDesired,
    PHKEY phkResult
    );

NTSTATUS
RegTransactCreateKeyExW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN PCWSTR pSubKey,
    IN DWORD Reserved,
    IN OPTIONAL PWSTR pClass,
    IN DWORD dwOptions,
    IN ACCESS_MASK AccessDesired,
    IN OPTIONAL PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor,
    OUT PHKEY phkResult,
    OUT OPTIONAL PDWORD pdwDisposition
    );

NTSTATUS
RegTransactCloseKey(
    HANDLE Handle,
    HKEY hKey
    );

NTSTATUS
RegTransactDeleteKeyW(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pSubKey
    );

NTSTATUS
RegTransactQueryInfoKeyW(
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

NTSTATUS
RegTransactEnumKeyExW(
    IN HANDLE hConnection,
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
RegTransactDeleteKeyValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValueName
    );

NTSTATUS
RegTransactDeleteTreeW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey
    );

NTSTATUS
RegTransactDeleteValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pValueName
    );

NTSTATUS
RegTransactEnumValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN DWORD dwIndex,
    OUT PWSTR pValueName,
    IN OUT PDWORD pcchValueName,
    IN PDWORD pReserved,
    OUT OPTIONAL PDWORD pType,
    OUT OPTIONAL PBYTE pData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
RegTransactGetValueW(
    IN HANDLE hConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

NTSTATUS
RegTransactQueryMultipleValues(
    HANDLE Handle,
    HKEY hKey,
    PVALENT val_list,
    DWORD num_vals,
    PWSTR pValueBuf,
    PDWORD dwTotsize
    );

NTSTATUS
RegTransactSetValueExW(
    HANDLE Handle,
    HKEY hKey,
    PCWSTR pValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *pData,
    DWORD cbData
    );

NTSTATUS
RegTransactSetKeySecurity(
	IN HANDLE hNtRegConnection,
	IN HKEY hKey,
	IN SECURITY_INFORMATION SecurityInformation,
	IN PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
	IN ULONG Length
	);

NTSTATUS
RegTransactGetKeySecurity(
	IN HANDLE hNtRegConnection,
	IN HKEY hKey,
	IN SECURITY_INFORMATION SecurityInformation,
	OUT PSECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor,
	IN OUT PULONG lpcbSecurityDescriptor
	);

#endif /* __CLIENTIPC_P_H__ */

