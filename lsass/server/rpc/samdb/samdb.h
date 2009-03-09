#ifndef __SAMDB_H__
#define __SAMDB_H__

typedef enum
{
    SAMDB_USER  = 0,
    SAMDB_GROUP,
    SAMDB_DOMAIN
} SamDbEntryType;

DWORD
SamDBAddObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN
    );

DWORD
SamDBModifyObject(
    HANDLE hBindHandle,
    PWSTR  ObjectDN,
    DIRECTORY_MODS Modifications[]
    );

DWORD
SamDbModifyUser(
    HANDLE hDirectory,
    PWSTR  pwszObjectName,
    DIRECTORY_MODS Modifications[]
    );

DWORD
SamDbModifyGroup(
    HANDLE hDirectory,
    PWSTR  pwszObjectName,
    DIRECTORY_MODS Modifications[]
    );

DWORD
SamDBDeleteObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN
    );

DWORD
SamDbDeleteUser(
    HANDLE hDirectory,
    PWSTR  pwszUserName
    );

DWORD
SamDbDeleteGroup(
    HANDLE hDirectory,
    PWSTR  pwszGroupName
    );


DWORD
SamDbSearchUsers(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PDIRECTORY_VALUES * ppDirectoryValues,
    PDWORD pdwNumValues
    );

DWORD
SamDbSearchGroups(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PDIRECTORY_VALUES * ppDirectoryValues,
    PDWORD pdwNumValues
    );

DWORD
SamDbSearchDomains(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PDIRECTORY_VALUES * ppDirectoryValues,
    PDWORD pdwNumValues
    );

DWORD
SamDbConvertFiltertoTable(
    PWSTR pwszFilter,
    PDWORD pdwTable
    );

#endif /* __SAMDB_H__ */


