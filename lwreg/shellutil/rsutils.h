#ifndef _SHELLUTIL_H
#define _SHELLUTIL_H

#include <config.h>
#include <regsystem.h>

#include <reg/reg.h>
#include <regutils.h>
#include <regdef.h>
#include <regclient.h>
#include <regparse_r.h>

#include <lw/base.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lwstr.h>
#include <lwmem.h>
#include <lwerror.h>

#define REGEXPORT_LINE_WIDTH 80

typedef struct _REGSHELL_UTIL_VALUE
{
    REG_DATA_TYPE type;
    PWSTR pValueName;
    LW_PVOID pData;
    DWORD dwDataLen;
} REGSHELL_UTIL_VALUE, *PREGSHELL_UTIL_VALUE;


DWORD
RegShellCanonicalizePath(
    PSTR pszInDefaultKey,
    PSTR pszInKeyName,
    PSTR *ppszFullPath,
    PSTR *ppszParentPath,
    PSTR *ppszSubKey);

DWORD
RegShellIsValidKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszKey);

DWORD
RegShellUtilAddKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR pszKeyName);

DWORD
RegShellUtilDeleteKey(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName);

DWORD
RegShellUtilDeleteTree(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName);

DWORD
RegShellUtilGetKeys(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    LW_WCHAR ***pppRetSubKeys,
    PDWORD pdwRetSubKeyCount);

DWORD
RegShellUtilSetValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID data,
    DWORD dataLen);

DWORD
RegShellUtilGetValues(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PREGSHELL_UTIL_VALUE *valueArray,
    PDWORD pdwValueArrayLen
    );

DWORD
RegShellUtilDeleteValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PSTR valueName);

DWORD
RegShellUtilGetValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    OPTIONAL PSTR pszDefaultKey,
    PSTR pszKeyName,
    PSTR pszValueName,
    REG_DATA_TYPE regType,
    PVOID *ppValue,
    PDWORD pdwValueLen);

DWORD RegShellUtilImportCallback(
    PREG_PARSE_ITEM pItem,
    HANDLE userContext);

DWORD RegShellUtilImportDebugCallback(
    PREG_PARSE_ITEM pItem,
    HANDLE userContext);

DWORD
RegShellUtilExport(
    HANDLE hReg,
    FILE* fp,
    HKEY hKey,
    PSTR pszKeyName,
    DWORD dwNumSubKeys
    );

DWORD RegExportBinaryTypeToString(
    REG_DATA_TYPE token,
    PSTR tokenStr,
    BOOLEAN dumpFormat);


DWORD
RegExportEntry(
    PSTR keyName,
    REG_DATA_TYPE valueType,
    PSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportDword(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    DWORD value,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportRegKey(
    PSTR keyName,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportString(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    PCHAR value,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportBinaryData(
    REG_DATA_TYPE valueType,
    PSTR valueName,
    REG_DATA_TYPE type,
    UCHAR *value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportPlainText(
    PCHAR value,
    PSTR *dumpString,
    PDWORD dumpStringLen);

#endif
