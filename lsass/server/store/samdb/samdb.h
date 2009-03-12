#ifndef __SAMDB_H__
#define __SAMDB_H__

DWORD
SamDbInit(
    VOID
    );

DWORD
SamDbOpen(
    PHANDLE phDirectory
    );

DWORD
SamDbBind(
    HANDLE hDirectory,
    PWSTR  pwszDistinguishedName,
    PWSTR  pwszCredential,
    ULONG  ulMethod
    );

DWORD
SamDbAddObject(
    HANDLE hDirectory,
    PWSTR  pwszObjectDN,
    DIRECTORY_MOD Modifications[]
    );

DWORD
SamDbAddUser(
    HANDLE hDirectory,
    PWSTR  pwszObjectName,
    DIRECTORY_MOD Modifications[]
    );

DWORD
SamDbAddGroup(
    HANDLE hDirectory,
    PWSTR  pwszObjectName,
    DIRECTORY_MOD Modifications[]
    );

DWORD
SamDbModifyObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN,
    DIRECTORY_MOD Modifications[]
    );

DWORD
SamDbModifyUser(
    HANDLE hDirectory,
    PWSTR  pwszObjectName,
    DIRECTORY_MOD Modifications[]
    );

DWORD
SamDbModifyGroup(
    HANDLE hDirectory,
    PWSTR  pwszObjectName,
    DIRECTORY_MOD Modifications[]
    );

DWORD
SamDbDeleteObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN
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
SamDbSearchObject(
    HANDLE hDirectory,
    PWSTR  pwszBase,
    ULONG  ulScope,
    PWSTR  pwszFilter,
    PWSTR  pwszAttributes[],
    ULONG  ulAttributesOnly,
    PATTRIBUTE_VALUE* ppDirectoryValues,
    PDWORD pdwNumValues
    );


DWORD
SamDbSearchUsers(
    HANDLE hDirectory,
    PWSTR  pwszBase,
    ULONG  ulScope,
    PWSTR  pwszAttributes[],
    ULONG  ulAttributesOnly,
    PATTRIBUTE_VALUE * ppDirectoryValues,
    PDWORD pdwNumValues
    );

DWORD
SamDbSearchGroups(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PATTRIBUTE_VALUE * ppDirectoryValues,
    PDWORD pdwNumValues
    );

DWORD
SamDbSearchDomains(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PATTRIBUTE_VALUE * ppDirectoryValues,
    PDWORD pdwNumValues
    );

DWORD
SamDbConvertFiltertoTable(
    PWSTR pwszFilter,
    PDWORD pdwTable
    );

DWORD
SamDbInitUserTable(
    PSAM_DB_CONTEXT pDbContext
    );

DWORD
SamDbInitGroupTable(
    PSAM_DB_CONTEXT pDbContext
    );

DWORD
SamDbClose(
    HANDLE hDirectory
    );

#endif /* __SAMDB_H__ */


