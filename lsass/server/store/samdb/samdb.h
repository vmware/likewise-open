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
SamDbConvertFiltertoTable(
    PWSTR             pwszFilter,
    SAMDB_ENTRY_TYPE* pdwTable
    );

VOID
SamDbClose(
    HANDLE hDirectory
    );

#endif /* __SAMDB_H__ */


