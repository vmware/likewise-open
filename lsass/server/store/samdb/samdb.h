#ifndef __SAMDB_H__
#define __SAMDB_H__

typedef enum
{
    SAMDB_DOMAIN_TABLE_COLUMN_DOMAIN_NAME = 1,
    SAMDB_DOMAIN_TABLE_COLUMN_NETBIOS_NAME,
    SAMDB_DOMAIN_TABLE_COLUMN_MACHINE_SID

} SAMDB_DOMAIN_TABLE_COLUMN;

typedef enum
{
    SAMDB_GROUP_TABLE_COLUMN_NAME = 1,
    SAMDB_GROUP_TABLE_COLUMN_SID,
    SAMDB_GROUP_TABLE_COLUMN_GID,
    SAMDB_GROUP_TABLE_COLUMN_PASSWORD,
    SAMDB_GROUP_TABLE_COLUMN_MEMBERS

} SAMDB_GROUP_TABLE_COLUMN;

typedef enum
{
    SAMDB_USER_TABLE_COLUMN_NAME = 1,
    SAMDB_USER_TABLE_COLUMN_UID,
    SAMDB_USER_TABLE_COLUMN_SID,
    SAMDB_USER_TABLE_COLUMN_PRIMARY_GROUP,
    SAMDB_USER_TABLE_COLUMN_PASSWORD,
    SAMDB_USER_TABLE_COLUMN_GECOS,
    SAMDB_USER_TABLE_COLUMN_HOMEDIR,
    SAMDB_USER_TABLE_COLUMN_SHELL,
    SAMDB_USER_TABLE_COLUMN_USER_INFO_FLAGS,
    SAMDB_USER_TABLE_COLUMN_PASSWORD_CHANGE_TIME,
    SAMDB_USER_TABLE_COLUMN_FULL_NAME,
    SAMDB_USER_TABLE_COLUMN_ACCOUNT_EXPIRY,
    SAMDB_USER_TABLE_COLUMN_LM_HASH,
    SAMDB_USER_TABLE_COLUMN_NT_HASH

} SAMDB_USER_TABLE_COLUMN;

typedef enum
{
    SAMDB_OBJECT_CLASS_UNKNOWN   = 0,
    SAMDB_OBJECT_CLASS_DOMAIN    = 1,
    SAMDB_OBJECT_CLASS_CONTAINER = 2,
    SAMDB_OBJECT_CLASS_GROUP     = 3,
    SAMDB_OBJECT_CLASS_USER      = 4
} SAMDB_OBJECT_CLASS;

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
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbModifyObject(
    HANDLE        hBindHandle,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD Modifications[]
    );

DWORD
SamDbSearchObject(
    HANDLE            hDirectory,
    PWSTR             pwszBaseDN,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
SamDbDeleteObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN
    );

DWORD
SamDbAddUser(
    HANDLE        hDirectory,
    PWSTR         pwszUserDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbModifyUser(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbSearchUsers(
    HANDLE            hDirectory,
    PWSTR             pwszBaseDN,
    ULONG             ulScope,
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
SamDbDeleteUser(
    HANDLE hDirectory,
    PWSTR  pwszUserDN
    );

DWORD
SamDbAddGroup(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbNumMembersInGroup_inlock(
    HANDLE hDirectory,
    PCSTR  pszGroupName,
    PCSTR  pszDomain,
    PDWORD pdwNumGroupMembers
    );

DWORD
SamDbModifyGroup(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbSearchGroups(
    HANDLE            hDirectory,
    PWSTR             pwszBaseDN,
    ULONG             ulScope,
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
SamDbFindGID_inlock(
    HANDLE hDirectory,
    PWSTR  pwszGroupDN,
    PDWORD pdwGID
    );

DWORD
SamDbDeleteGroup(
    HANDLE hDirectory,
    PWSTR  pwszGroupDN
    );

DWORD
SamDbAddDomain(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbFindDomains(
    HANDLE                hDirectory,
    PWSTR                 pwszDomainName,
    PSAM_DB_DOMAIN_INFO** pppDomainInfoList,
    PDWORD                pdwNumDomains
    );

DWORD
SamDbBuildDomainInfo(
    PSTR*                 ppszResult,
    int                   nRows,
    int                   nCols,
    int                   nHeaderColsToSkip,
    PSAM_DB_DOMAIN_INFO** pppDomainInfo,
    PDWORD                pdwNumDomainsFound
    );

DWORD
SamDbModifyDomain(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
SamDbSearchDomains(
    HANDLE            hDirectory,
    PWSTR             pwszBaseDN,
    ULONG             ulScope,
    PWSTR             pwszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
SamDbDeleteDomain(
    HANDLE hDirectory,
    PWSTR  pswzObjectDN
    );

DWORD
SamDbNumObjectsInDomain_inlock(
    HANDLE hDirectory,
    PSTR   pszDomainName,
    PDWORD pdwNumObjects
    );

VOID
SamDbFreeDomainInfoList(
    PSAM_DB_DOMAIN_INFO* ppDomainInfoList,
    DWORD dwNumDomains
    );

VOID
SamDbFreeDomainInfo(
    PSAM_DB_DOMAIN_INFO pDomainInfo
    );

DWORD
SamDbBuildDomainDirectoryEntries(
    PSAM_DIRECTORY_CONTEXT pDirContext,
    PWSTR                  pwszAttributes[],
    ULONG                  ulAttributesOnly,
    PSAM_DB_DOMAIN_INFO*   ppDomainInfoList,
    DWORD                  dwNumDomains,
    PDIRECTORY_ENTRY*      ppDirectoryEntries
    );

DWORD
SamDbBuildAttributeLookup(
    PSAMDB_ATTRIBUTE_LOOKUP* ppAttrLookup
    );

DWORD
SamDbAcquireAttributeLookup(
    PSAMDB_ATTRIBUTE_LOOKUP  pAttrLookup,
    PSAMDB_ATTRIBUTE_LOOKUP* ppAttrLookup
    );

VOID
SamDbReleaseAttributeLookup(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    );

VOID
SamDbFreeAttributeLookupEntry(
    PSAMDB_ATTRIBUTE_LOOKUP_ENTRY pLookupEntry
    );

DWORD
SamDbConvertFiltertoTable(
    PWSTR             pwszFilter,
    SAMDB_ENTRY_TYPE* pdwTable
    );

DWORD
SamDbAddDomainAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    );

DWORD
SamDbAddUserAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    );

DWORD
SamDbAddGroupAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    );

VOID
SamDbClose(
    HANDLE hDirectory
    );

#endif /* __SAMDB_H__ */


