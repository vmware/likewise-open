#ifndef _RSUTILS_H
#define _RSUTILS_H

#define REGEXPORT_LINE_WIDTH 80

typedef struct _REGSHELL_UTIL_VALUE
{
    REG_DATA_TYPE type;
    PWSTR pValueName;
    LW_PVOID pData;
    DWORD dwDataLen;
} REGSHELL_UTIL_VALUE, *PREGSHELL_UTIL_VALUE;


typedef enum _REGSHELL_UTIL_IMPORT_MODE
{
    REGSHELL_UTIL_IMPORT_OVERWRITE = 0,
    REGSHELL_UTIL_IMPORT_UPGRADE,
} REGSHELL_UTIL_IMPORT_MODE;

typedef struct _REGSHELL_UTIL_IMPORT_CONTEXT
{
    HANDLE hReg;
    REGSHELL_UTIL_IMPORT_MODE eImportMode;
} REGSHELL_UTIL_IMPORT_CONTEXT, *PREGSHELL_UTIL_IMPORT_CONTEXT;


DWORD
RegShellCanonicalizePath(
    PCSTR pszInDefaultKey,
    PCSTR pszInKeyName,
    PSTR *ppszFullPath,
    PSTR *ppszParentPath,
    PSTR *ppszSubKey
    );

DWORD
RegShellIsValidKey(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszKey
    );

DWORD
RegShellUtilAddKey(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR pszKeyName,
    BOOLEAN bDoBail
    );

DWORD
RegShellUtilAddKeySecDesc(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR pszKeyName,
    BOOLEAN bDoBail,
    ACCESS_MASK AccessDesired,
    PSECURITY_DESCRIPTOR_ABSOLUTE pSecurityDescriptor
    );

DWORD
RegShellUtilDeleteKey(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName
    );

DWORD
RegShellUtilDeleteTree(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName
    );

DWORD
RegShellUtilGetKeys(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName,
    LW_WCHAR ***pppRetSubKeys,
    PDWORD pdwRetSubKeyCount
    );

DWORD
RegShellUtilSetValue(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName,
    PCSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID data,
    DWORD dataLen
    );

DWORD
RegShellUtilGetValues(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName,
    PREGSHELL_UTIL_VALUE *valueArray,
    PDWORD pdwValueArrayLen
    );

DWORD
RegShellUtilDeleteValue(
    HANDLE hReg,
    PCSTR pszRootKeyName,
    PCSTR pszDefaultKey,
    PCSTR keyName,
    PCSTR valueName
    );

DWORD
RegShellUtilGetValue(
    IN OPTIONAL HANDLE hReg,
    IN OPTIONAL PCSTR pszRootKeyName,
    IN OPTIONAL PCSTR pszDefaultKey,
    IN OPTIONAL PCSTR pszKeyName,
    IN PCSTR pszValueName,
    OUT OPTIONAL PREG_DATA_TYPE pRegType,
    OUT OPTIONAL PVOID *ppValue,
    OUT OPTIONAL PDWORD pdwValueLen
    );

DWORD
RegShellUtilImportCallback(
    PREG_PARSE_ITEM pItem,
    HANDLE hUserCtx
    );

DWORD
RegShellUtilImportDebugCallback(
    PREG_PARSE_ITEM pItem,
    HANDLE userContext
    );

DWORD
RegShellUtilExport(
    HANDLE hReg,
    FILE* fp,
    HKEY hKey,
    PCSTR pszKeyName,
    DWORD dwNumSubKeys,
    DWORD dwMaxSubKeyLen
    );

DWORD
RegShellUtilEscapeString(
    PCSTR pszValue,
    PSTR *ppszRetValue,
    PDWORD pdwEscapeValueLen
    );

DWORD
RegExportBinaryTypeToString(
    REG_DATA_TYPE token,
    PSTR tokenStr,
    BOOLEAN dumpFormat
    );

DWORD
RegExportEntry(
    PCSTR keyName,
    REG_DATA_TYPE valueType,
    PCSTR valueName,
    REG_DATA_TYPE type,
    LW_PVOID value,
    DWORD valueLen,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportDword(
    REG_DATA_TYPE valueType,
    PCSTR valueName,
    DWORD value,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportRegKey(
    PCSTR keyName,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportString(
    REG_DATA_TYPE valueType,
    PCSTR valueName,
    PCSTR value,
    PSTR *dumpString,
    PDWORD dumpStringLen);

DWORD
RegExportBinaryData(
    REG_DATA_TYPE valueType,
    PCSTR valueName,
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

DWORD
RegShellUtilValueArrayFree(
    PREGSHELL_UTIL_VALUE pValueArray,
    DWORD dwValueArrayLen);

#endif
