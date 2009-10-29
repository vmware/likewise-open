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
 *        reg.h
 *
 * Abstract:
 *
 *        Registry
 *
 *        Public Client API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 *          Wei Fu (wfu@likewisesoftware.com)
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#ifndef __REG_H__
#define __REG_H__

#include <lw/types.h>
#include <lw/attrs.h>
/*
 * Logging
 */
typedef enum
{
    REG_LOG_LEVEL_ALWAYS = 0,
    REG_LOG_LEVEL_ERROR,
    REG_LOG_LEVEL_WARNING,
    REG_LOG_LEVEL_INFO,
    REG_LOG_LEVEL_VERBOSE,
    REG_LOG_LEVEL_DEBUG,
    REG_LOG_LEVEL_TRACE
} RegLogLevel;

typedef enum
{
    REG_LOG_TARGET_DISABLED = 0,
    REG_LOG_TARGET_CONSOLE,
    REG_LOG_TARGET_FILE,
    REG_LOG_TARGET_SYSLOG
} RegLogTarget;

typedef VOID (*PFN_REG_LOG_MESSAGE)(
                    HANDLE      hLog,
                    RegLogLevel logLevel,
                    PCSTR       pszFormat,
                    va_list     msgList
                    );

typedef struct __REG_LOG_INFO {
    RegLogLevel  maxAllowedLogLevel;
    RegLogTarget logTarget;
    PSTR         pszPath;
} REG_LOG_INFO, *PREG_LOG_INFO;

typedef struct __REG_KEY_CONTEXT *HKEY, **PHKEY;

typedef DWORD REG_ACCESS_MASK;
typedef REG_ACCESS_MASK REGSAM;


typedef DWORD REG_DATA_TYPE;
typedef DWORD *PREG_DATA_TYPE;

#define REG_NONE                           0 // No value type
#define REG_SZ                             1 // Unicode null terminated string
#define REG_EXPAND_SZ                      2 // hex(2): (Not supported)
#define REG_BINARY                         3 // hex:
#define REG_DWORD                          4 // dword
#define REG_DWORD_LITTLE_ENDIAN            4 // 32-bit number (same as REG_DWORD)
#define REG_DWORD_BIG_ENDIAN               5 // 32-bit number (Not supported)
#define REG_LINK                           6 // hex(7): (Not supported)
#define REG_MULTI_SZ                       7 // Multiple Unicode strings
#define REG_RESOURCE_LIST                  8 // hex(8): (Not supported)
#define REG_FULL_RESOURCE_DESCRIPTOR       9 // hex(9): (Not supported)
#define REG_RESOURCE_REQUIREMENTS_LIST     10// hex(a): (Not supported)
#define REG_QWORD                          11// hex(b): (Not supported)
#define REG_QWORD_LITTLE_ENDIAN            11// hex(b):


#define REG_KEY                            21// represent the reg entry is a Key
#define REG_KEY_DEFAULT                    22// Default "@" entry
#define REG_PLAIN_TEXT                     23// A string without "" around it
#define REG_UNKNOWN                        24// Unknown data type


typedef DWORD REG_DATA_TYPE_FLAGS;

#define RRF_RT_REG_NONE       0x00000001
#define RRF_RT_REG_SZ         0x00000002 //Restrict type to REG_SZ.
#define RRF_RT_REG_EXPAND_SZ  0x00000004 //Restrict type to REG_EXPAND_SZ.
#define RRF_RT_REG_BINARY     0x00000008 //Restrict type to REG_BINARY.
#define RRF_RT_REG_DWORD      0x00000010 //Restrict type to REG_DWORD.
#define RRF_RT_REG_MULTI_SZ   0x00000020 //Restrict type to REG_MULTI_SZ.
#define RRF_RT_REG_QWORD      0x00000040 //Restrict type to REG_QWORD.
#define RRF_RT_DWORD          RRF_RT_REG_BINARY | RRF_RT_REG_DWORD
#define RRF_RT_QWORD          RRF_RT_REG_BINARY | RRF_RT_REG_QWORD
#define RRF_RT_ANY            0x0000FFFF //No type restriction.
#define RRF_NOEXPAND          0x10000000
#define RRF_ZEROONFAILURE     0x20000000

#define LIKEWISE_ROOT_KEY "HKEY_THIS_MACHINE"
#define HKEY_THIS_MACHINE "HKEY_THIS_MACHINE"

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_LENGTH 1024

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
}FILETIME, *PFILETIME;

typedef struct value_ent {
    PWSTR     ve_valuename;
    PDWORD    ve_valueptr;
    DWORD     ve_valuelen;
    DWORD     ve_type;
}VALENT, *PVALENT;

typedef struct _SECURITY_ATTRIBUTES {
    DWORD  nLength;
    PVOID  pSecurityDescriptor;
    BOOL   bInheritHandle;
}SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES;

DWORD
RegOpenServer(
    PHANDLE phConnection
    );

VOID
RegCloseServer(
    HANDLE hConnection
    );

DWORD
RegBuildLogInfo(
    RegLogLevel    maxAllowedLogLevel,
    RegLogTarget   logTarget,
    PCSTR          pszPath,
    PREG_LOG_INFO* ppLogInfo
    );

DWORD
RegSetLogLevel(
    HANDLE      hRegConnection,
    RegLogLevel logLevel
    );

DWORD
RegGetLogInfo(
    HANDLE         hRegConnection,
    PREG_LOG_INFO* ppLogInfo
    );

DWORD
RegSetLogInfo(
    HANDLE        hRegConnection,
    PREG_LOG_INFO pLogInfo
    );

VOID
RegFreeLogInfo(
    PREG_LOG_INFO pLogInfo
    );

LW_DWORD
RegGetErrorMessageForLoggingEvent(
    LW_DWORD dwError,
    LW_PSTR* ppszErrorMsg
    );


// Registry Client Side APIs
DWORD
RegEnumRootKeysA(
    IN HANDLE hRegConnection,
    OUT PSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

DWORD
RegEnumRootKeysW(
    IN HANDLE hRegConnection,
    OUT PWSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

DWORD
RegCreateKeyExA(
    HANDLE hRegConnection,
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

DWORD
RegCreateKeyExW(
    HANDLE hRegConnection,
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

DWORD
RegCloseKey(
    HANDLE hRegConnection,
    HKEY hKey
    );

DWORD
RegDeleteKeyA(
    HANDLE hRegConnection,
    HKEY hKey,
    PCSTR pszSubKey
    );

DWORD
RegDeleteKeyW(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pSubKey
    );

DWORD
RegDeleteKeyValue(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValueName
    );

DWORD
RegDeleteTree(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pSubKey
    );

DWORD
RegDeleteValue(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN PCWSTR pValueName
    );

DWORD
RegEnumKeyEx(
    HANDLE hRegConnection,
    HKEY hKey,
    DWORD dwIndex,
    PWSTR pName,
    PDWORD pcName,
    PDWORD pReserved,
    PWSTR pClass,
    PDWORD pcClass,
    PFILETIME pftLastWriteTime
    );

DWORD
RegEnumValueA(
    HANDLE hRegConnection,
    HKEY hKey,
    DWORD dwIndex,
    PSTR pszValueName,
    PDWORD pcchValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );

DWORD
RegEnumValueW(
    HANDLE hRegConnection,
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
RegGetValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszSubKey,
    IN OPTIONAL PCSTR pszValue,
    IN OPTIONAL REG_DATA_TYPE_FLAGS Flags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

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
    );

DWORD
RegOpenKeyExA(
    HANDLE hRegConnection,
    HKEY hKey,
    PCSTR pszSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

DWORD
RegOpenKeyExW(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pwszSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

DWORD
RegQueryInfoKeyA(
    HANDLE hRegConnection,
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

DWORD
RegQueryInfoKeyW(
    HANDLE hRegConnection,
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

DWORD
RegQueryMultipleValues(
    HANDLE hRegConnection,
    HKEY hKey,
    PVALENT val_list,
    DWORD num_vals,
    PWSTR pValueBuf,
    PDWORD dwTotsize
    );

DWORD
RegQueryValueExA(
    HANDLE hRegConnection,
    HKEY hKey,
    PCSTR pszValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );

DWORD
RegQueryValueExW(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );

DWORD
RegSetValueExA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCSTR pszValueName,
    IN DWORD Reserved,
    IN DWORD dwType,
    IN OPTIONAL const BYTE *pData,
    IN DWORD cbData
    );

DWORD
RegSetValueExW(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *pData,
    DWORD cbData
    );

/* registry multi-str data type conversion functions */
DWORD
RegMultiStrsToByteArrayW(
    PSTR* ppszInMultiSz,
    PBYTE *outBuf,
    SSIZE_T *outBufLen
    );

DWORD
RegMultiStrsToByteArrayA(
    PSTR* ppszInMultiSz,
    PBYTE *outBuf,
    SSIZE_T *outBufLen
    );

DWORD
RegByteArrayToMultiStrsW(
    PBYTE pInBuf,
    SSIZE_T bufLen,
    PSTR **pppszOutMultiSz
    );

DWORD
RegByteArrayToMultiStrsA(
    PBYTE pInBuf,
    SSIZE_T bufLen,
    PSTR **pppszOutMultiSz
    );

void
RegMultiStrsFree(
    PCHAR *pszMultiSz
    );

DWORD
RegEnumRootKeysA(
    IN HANDLE hRegConnection,
    OUT PSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

DWORD
RegCreateKeyExA(
    HANDLE hRegConnection,
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

DWORD
RegQueryInfoKeyW(
    HANDLE hRegConnection,
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

#ifdef UNICODE
#define RegEnumValue(hRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData) \
    RegEnumValueW(hRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData)
#define RegGetValue(hRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData) \
    RegGetValueW(hRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData)
#define RegQueryValueEx(hRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData) \
    RegQueryValueExW(hRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData)
#define RegSetValueEx(hRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData) \
    RegSetValueExW(hRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData)
#define RegOpenKeyEx(hRegConnection, hKey, pSubKey, ulOptions, samDesired, phkResult) \
    RegOpenKeyExW(hRegConnection, hKey, pwszSubKey, ulOptions, samDesired, phkResult)
#define RegMultiStrsToByteArray(ppszInMultiSz, outBuf, outBufLen) \
    RegMultiStrsToByteArrayW(ppszInMultiSz, outBuf, outBufLen)
#define RegByteArrayToMultiStrs(pInBuf, bufLen, pppszOutMultiSz) \
    RegByteArrayToMultiStrsW(pInBuf, bufLen, pppszOutMultiSz)
#define RegEnumRootKeys(hRegConnection, pppRootKeyNames, pdwNumRootKeys) \
    RegEnumRootKeysW(hRegConnection, pppwszRootKeyNames, pdwNumRootKeys)
#define RegCreateKeyEx(hRegConnection, hKey, pszSubKey, Reserved, pClass, dwOptions, samDesired, pSecurityAttributes, phkResult, pdwDisposition) \
    RegCreateKeyExW(hRegConnection, hKey, pwszSubKey, Reserved, pClass, dwOptions, samDesired, pSecurityAttributes, phkResult, pdwDisposition)
#define RegQueryInfoKey(hRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime) \
    RegQueryInfoKeyW(hRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime)
#define RegDeleteKey(hRegConnection, hKey, pSubKey) \
    RegDeleteKeyW(hRegConnection, hKey, pwszSubKey)
#else
#define RegEnumValue(hRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData) \
    RegEnumValueA(hRegConnection, hKey, dwIndex, pValueName, pcchValueName, pReserved, pType, pData, pcbData)
#define RegGetValue(hRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData) \
    RegGetValueA(hRegConnection, hKey, pSubKey, pValue, Flags, pdwType, pvData, pcbData)
#define RegQueryValueEx(hRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData) \
    RegQueryValueExA(hRegConnection, hKey, pValueName, pReserved, pType, pData, pcbData)
#define RegSetValueEx(hRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData) \
    RegSetValueExA(hRegConnection, hKey, pValueName, Reserved, dwType, pData, cbData)
#define RegOpenKeyEx(hRegConnection, hKey, pSubKey, ulOptions, samDesired, phkResult) \
    RegOpenKeyExA(hRegConnection, hKey, pszSubKey, ulOptions, samDesired, phkResult)
#define RegMultiStrsToByteArray(ppszInMultiSz, outBuf, outBufLen) \
    RegMultiStrsToByteArrayA(ppszInMultiSz, outBuf, outBufLen)
#define RegByteArrayToMultiStrs(pInBuf, bufLen, pppszOutMultiSz) \
    RegByteArrayToMultiStrsA(pInBuf, bufLen, pppszOutMultiSz)
#define RegEnumRootKeys(hRegConnection, pppRootKeyNames, pdwNumRootKeys) \
    RegEnumRootKeysA(hRegConnection, pppszRootKeyNames, pdwNumRootKeys)
#define RegCreateKeyEx(hRegConnection, hKey, pszSubKey, Reserved, pClass, dwOptions, samDesired, pSecurityAttributes, phkResult, pdwDisposition) \
    RegCreateKeyExA(hRegConnection, hKey, pszSubKey, Reserved, pClass, dwOptions, samDesired, pSecurityAttributes, phkResult, pdwDisposition)
#define RegQueryInfoKey(hRegConnection, hKey, pClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime) \
    RegQueryInfoKeyA(hRegConnection, hKey, pszClass, pcClass, pReserved, pcSubKeys, pcMaxSubKeyLen, pcMaxClassLen, pcValues, pcMaxValueNameLen, pcMaxValueLen, pcbSecurityDescriptor, pftLastWriteTime)
#define RegDeleteKey(hRegConnection, hKey, pSubKey) \
    RegDeleteKeyA(hRegConnection, hKey, pszSubKey)
#endif

#endif /* __REG_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
