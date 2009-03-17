#ifndef __SAMDB_H__
#define __SAMDB_H__

#define SAMDB_ATTR_TAG_USER_NAME            "user-name"
#define SAMDB_ATTR_TAG_USER_FULLNAME        "user-full-name"
#define SAMDB_ATTR_TAG_UID                  "uid"
#define SAMDB_ATTR_TAG_USER_SID             "user-sid"
#define SAMDB_ATTR_TAG_USER_PASSWORD        "user-password"
#define SAMDB_ATTR_TAG_GECOS                "user-gecos"
#define SAMDB_ATTR_TAG_USER_PRIMARY_GROUP   "user-primary-group"
#define SAMDB_ATTR_TAG_HOMEDIR              "user-home-directory"
#define SAMDB_ATTR_TAG_PASSWORD_CHANGE_TIME "user-password-change-time"
#define SAMDB_ATTR_TAG_ACCOUNT_EXPIRY       "user-account-expiry"
#define SAMDB_ATTR_TAG_USER_INFO_FLAGS      "user-info-flags"
#define SAMDB_ATTR_TAG_USER_LM_HASH         "user-lm-hash"
#define SAMDB_ATTR_TAG_USER_NT_HASH         "user-nt-hash"
#define SAMDB_ATTR_TAG_GROUP_NAME           "group-name"
#define SAMDB_ATTR_TAG_GID                  "gid"
#define SAMDB_ATTR_TAG_GROUP_SID            "group-sid"
#define SAMDB_ATTR_TAG_GROUP_PASSWORD       "group-password"
#define SAMDB_ATTR_TAG_GROUP_MEMBERS        "group-members"
#define SAMDB_ATTR_TAG_DOMAIN_NAME          "domain-name"
#define SAMDB_ATTR_TAG_DOMAIN_SID           "domain-sid"
#define SAMDB_ATTR_TAG_DOMAIN_NETBIOS_NAME  "domain-netbios-name"

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
SamDbAddDomain(
    HANDLE hDirectory,
    PWSTR pszObjectName,
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
SamDbFindDomain(
    HANDLE hDirectory,
    PWSTR  pwszDomainName,
    PSAM_DB_DOMAIN_INFO* ppDomainInfo
    );

DWORD
SamDbBuildDomainInfo(
    PSTR*  ppszResult,
    int    nRows,
    int    nCols,
    int    nHeaderColsToSkip,
    PSAM_DB_DOMAIN_INFO** pppDomainInfo,
    PDWORD pdwNumDomainsFound
    );

DWORD
SamDbModifyDomain(
    HANDLE hDirectory,
    PWSTR pszObjectName,
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
SamDbDeleteDomain(
    HANDLE hDirectory,
    PWSTR pszObjectName
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
SamDbInitDomainTable(
    PSAM_DB_CONTEXT pDbContext
    );

DWORD
SamDbAddDomainAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    );

DWORD
SamDbInitUserTable(
    PSAM_DB_CONTEXT pDbContext
    );

DWORD
SamDbAddUserAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    );

DWORD
SamDbInitGroupTable(
    PSAM_DB_CONTEXT pDbContext
    );

DWORD
SamDbAddGroupAttrLookups(
    PSAMDB_ATTRIBUTE_LOOKUP pAttrLookup
    );

DWORD
SamDbClose(
    HANDLE hDirectory
    );

#endif /* __SAMDB_H__ */


