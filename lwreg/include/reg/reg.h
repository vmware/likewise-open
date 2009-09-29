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
 *          Marc Guy (mguy@likewisesoftware.com)
 */
#ifndef __REG_H__
#define __REG_H__

#include <lw/types.h>
#include <lw/attrs.h>

#ifndef REG_ERRORS_DEFINED
#define REG_ERRORS_DEFINED 1
#endif

#define REG_EVENT_INFO_SERVICE_STARTED                             1000
#define REG_EVENT_ERROR_SERVICE_START_FAILURE                      1001
#define REG_EVENT_INFO_SERVICE_STOPPED                             1002
#define REG_EVENT_ERROR_SERVICE_STOPPED                            1003
#define REG_EVENT_INFO_SERVICE_CONFIGURATION_CHANGED               1004

/*
 * Tracing support
 */
#define REG_TRACE_FLAG_USER_GROUP_QUERIES        1
#define REG_TRACE_FLAG_AUTHENTICATION            2
#define REG_TRACE_FLAG_USER_GROUP_ADMINISTRATION 3
#define REG_TRACE_FLAG_SENTINEL                  4

typedef struct __REG_TRACE_INFO
{
    DWORD   dwTraceFlag;
    BOOLEAN bStatus;
} REG_TRACE_INFO, *PREG_TRACE_INFO;

typedef struct __REG_TRACE_INFO_LIST
{
    DWORD dwNumFlags;
    PREG_TRACE_INFO pTraceInfoArray;
} REG_TRACE_INFO_LIST, *PREG_TRACE_INFO_LIST;

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

typedef HANDLE HKEY, *PHKEY;
typedef CHAR *LPCSTR;
typedef TCHAR *LPCTSTR;
typedef DWORD REGSAM;

typedef enum __REG_DATA_TYPE
{
    /* Registry data types */
    REG_UNKNOWN = 0,
    REG_DWORD = 1,                  /* dword:    */
    REG_SZ,                         /* ="REG_SZ" */
    REG_BINARY,                     /* hex:      */
    REG_NONE,                       /* hex(0):   */
    REG_EXPAND_SZ,                  /* hex(2):   */
    REG_MULTI_SZ,                   /* hex(7):   */
    REG_RESOURCE_LIST,              /* hex(8):   */
    REG_FULL_RESOURCE_DESCRIPTOR,   /* hex(9):   */
    REG_RESOURCE_REQUIREMENTS_LIST, /* hex(a):   */
    REG_QUADWORD,                   /* hex(b):   */
    REG_KEY,                        /* represent the reg entry is a Key*/
    REG_KEY_DEFAULT,                /* Default "@" entry */
    REG_PLAIN_TEXT,                 /* A string without "" around it */
} REG_DATA_TYPE, *PREG_DATA_TYPE;

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

typedef struct _REG_PARSE_ITEM
{
    REG_DATA_TYPE type;
    REG_DATA_TYPE valueType;
    PSTR keyName;
    PSTR valueName;
    DWORD lineNumber;
    void *value;
    DWORD valueLen;
} REG_PARSE_ITEM, *PREG_PARSE_ITEM;

#define LIKEWISE_ROOT_KEY "HKEY_LIKEWISE"
#define LIKEWISE_FOREIGN_ROOT_KEY "HKEY_LIKEWISE_IMPORT"

#define NUM_ROOTKEY  2

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

DWORD
RegSetTraceFlags(
    HANDLE          hRegConnection,
    PREG_TRACE_INFO pTraceFlagArray,
    DWORD           dwNumFlags
    );

DWORD
RegEnumTraceFlags(
    HANDLE           hRegConnection,
    PREG_TRACE_INFO* ppTraceFlagArray,
    PDWORD           pdwNumFlags
    );

DWORD
RegGetTraceFlag(
    HANDLE           hRegConnection,
    DWORD            dwTraceFlag,
    PREG_TRACE_INFO* ppTraceFlag
    );


LW_DWORD
RegGetErrorMessageForLoggingEvent(
    LW_DWORD dwError,
    LW_PSTR* ppszErrorMsg
    );

VOID
PrintError(
    IN OPTIONAL PCSTR pszErrorPrefix,
    IN DWORD dwError
    );

DWORD
RegEnumRootKeys(
    IN HANDLE hRegConnection,
    OUT PSTR** pppszRootKeyNames,
    OUT PDWORD pdwNumRootKeys
    );

DWORD
RegOpenRootKey(
    IN HANDLE hRegConnection,
    IN PSTR pszRootKeyName,
    OUT PHKEY phkResult
    );

DWORD
RegCreateKeyEx(
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
RegDeleteKey(
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
RegEnumValue(
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
RegEnumValueA(
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
RegGetValue(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pSubKey,
    PCWSTR pValue,
    DWORD dwFlags,
    PDWORD pdwType,
    PVOID pvData,
    PDWORD pcbData
    );

DWORD
RegGetValueA(
    IN HANDLE hRegConnection,
    IN HKEY hKey,
    IN OPTIONAL PCWSTR pSubKey,
    IN OPTIONAL PCWSTR pValue,
    IN OPTIONAL DWORD dwFlags,
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
    IN OPTIONAL DWORD dwFlags,
    OUT OPTIONAL PDWORD pdwType,
    OUT OPTIONAL PVOID pvData,
    IN OUT OPTIONAL PDWORD pcbData
    );

DWORD
RegOpenKeyEx(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pSubKey,
    DWORD ulOptions,
    REGSAM samDesired,
    PHKEY phkResult
    );

DWORD
RegQueryInfoKey(
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
RegQueryValueEx(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pValueName,
    PDWORD pReserved,
    PDWORD pType,
    PBYTE pData,
    PDWORD pcbData
    );

DWORD
RegQueryValueExA(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pValueName,
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
RegSetValueEx(
    HANDLE hRegConnection,
    HKEY hKey,
    PCWSTR pValueName,
    DWORD Reserved,
    DWORD dwType,
    const BYTE *pData,
    DWORD cbData
    );

#endif /* __REG_H__ */


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
