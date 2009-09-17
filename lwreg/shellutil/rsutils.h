#ifndef _SHELLUTIL_H
#define _SHELLUTIL_H

#include <config.h>
#include <regsystem.h>

#include <reg/reg.h>
#include <regutils.h>
#include <regdef.h>
#include <regclient.h>

#include <lw/base.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lwstr.h>
#include <lwmem.h>
#include <lwerror.h>

typedef struct _REGSHELL_UTIL_VALUE
{
    REG_DATA_TYPE type;
    PWSTR pValueName;
    LW_PVOID pData;
    DWORD dwDataLen;
} REGSHELL_UTIL_VALUE, *PREGSHELL_UTIL_VALUE;
#if 1

typedef struct _IMPORT_CONTEXT
{
    HANDLE hReg;
} IMPORT_CONTEXT;
#endif


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
    PDWORD dwRetSubKeyLen);

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
    PDWORD pdwValueArrayLen);

DWORD
RegShellUtilDeleteValue(
    HANDLE hReg,
    PSTR pszRootKeyName,
    PSTR pszDefaultKey,
    PSTR keyName,
    PSTR valueName);

DWORD RegShellUtilImportCallback(
    PREG_PARSE_ITEM pItem,
    HANDLE userContext);

DWORD RegShellUtilImportDebugCallback(
    PREG_PARSE_ITEM pItem,
    HANDLE userContext);

#endif
