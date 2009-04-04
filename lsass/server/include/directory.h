#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

typedef ULONG DIRECTORY_ATTR_TYPE;

#define DIRECTORY_ATTR_TYPE_BOOLEAN                 1
#define DIRECTORY_ATTR_TYPE_INTEGER                 2
#define DIRECTORY_ATTR_TYPE_LARGE_INTEGER           3
#define DIRECTORY_ATTR_TYPE_NT_SECURITY_DESCRIPTOR  4
#define DIRECTORY_ATTR_TYPE_OCTET_STREAM            5
#define DIRECTORY_ATTR_TYPE_UNICODE_STRING          6
#define DIRECTORY_ATTR_TYPE_ANSI_STRING             7

#define DIRECTORY_ATTR_TAG_DN                    "dn"
#define DIRECTORY_ATTR_TAG_USER_NAME             "user-name"
#define DIRECTORY_ATTR_TAG_USER_FULLNAME         "user-full-name"
#define DIRECTORY_ATTR_TAG_UID                   "uid"
#define DIRECTORY_ATTR_TAG_USER_SID              "user-sid"
#define DIRECTORY_ATTR_TAG_USER_PASSWORD         "user-password"
#define DIRECTORY_ATTR_TAG_GECOS                 "user-gecos"
#define DIRECTORY_ATTR_TAG_USER_PRIMARY_GROUP_DN "user-primary-group-dn"
#define DIRECTORY_ATTR_TAG_HOMEDIR               "user-home-directory"
#define DIRECTORY_ATTR_TAG_SHELL                 "user-shell"
#define DIRECTORY_ATTR_TAG_PASSWORD_CHANGE_TIME  "user-password-change-time"
#define DIRECTORY_ATTR_TAG_ACCOUNT_EXPIRY        "user-account-expiry"
#define DIRECTORY_ATTR_TAG_USER_INFO_FLAGS       "user-info-flags"
#define DIRECTORY_ATTR_TAG_GROUP_NAME            "group-name"
#define DIRECTORY_ATTR_TAG_GID                   "gid"
#define DIRECTORY_ATTR_TAG_GROUP_SID             "group-sid"
#define DIRECTORY_ATTR_TAG_GROUP_PASSWORD        "group-password"
#define DIRECTORY_ATTR_TAG_GROUP_MEMBERS         "group-members"
#define DIRECTORY_ATTR_TAG_DOMAIN_NAME           "domain-name"
#define DIRECTORY_ATTR_TAG_DOMAIN_SID            "domain-sid"
#define DIRECTORY_ATTR_TAG_DOMAIN_NETBIOS_NAME   "domain-netbios-name"

typedef struct _OCTET_STRING {
    ULONG ulNumBytes;
    PBYTE pBytes;
} OCTET_STRING, *POCTET_STRING;

typedef struct _ATTRIBUTE_VALUE
{
    DIRECTORY_ATTR_TYPE Type;

    union
    {
        ULONG  ulValue;
        LONG64 llValue;
        PWSTR  pwszStringValue;
        PSTR   pszStringValue;
        BOOL   bBooleanValue;
        POCTET_STRING pOctetString;
    };

} ATTRIBUTE_VALUE, *PATTRIBUTE_VALUE;

typedef struct _DIRECTORY_ATTRIBUTE
{
    PWSTR            pwszName;

    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pValues;

} DIRECTORY_ATTRIBUTE, *PDIRECTORY_ATTRIBUTE;

typedef ULONG DIR_MOD_FLAGS;

#define DIR_MOD_FLAGS_ADD     0x0
#define DIR_MOD_FLAGS_REPLACE 0x1
#define DIR_MOD_FLAGS_DELETE  0x2

typedef ULONG DIR_ACCOUNT_INFO_FLAG;

#define DIR_ACCOUNT_INFO_FLAG_DISABLED               0x00000001
#define DIR_ACCOUNT_INFO_FLAG_CANNOT_CHANGE_PASSWORD 0x00000002
#define DIR_ACCOUNT_INFO_FLAG_CANNOT_EXPIRE          0x00000004
#define DIR_ACCOUNT_INFO_FLAG_LOCKED_OUT             0x00000008

typedef struct _DIRECTORY_MOD
{
    DIR_MOD_FLAGS    ulOperationFlags;
    PWSTR            pwszAttrName;
    ULONG            ulNumValues;
    PATTRIBUTE_VALUE pAttrValues;

} DIRECTORY_MOD, *PDIRECTORY_MOD;

typedef struct _DIRECTORY_ENTRY
{
    ULONG                ulNumAttributes;
    PDIRECTORY_ATTRIBUTE pAttributes;

} DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

#define ENTRY(i, ppDirectoryEntry) = *(ppDirectoryEntry + i)
#define ATTRIBUTE(i, ppDirectoryAttributes) = *(ppDirectoryAttributes + i)

#define VALUE(i, pAttribute) = *(pAttribute->ppAttributeValues + i)
#define NUM_VALUES(pAttribute) = pAttribute->ulNumValues
#define INTEGER(pValue) =  pValue->ulValue;
#define PRINTABLE_STRING(pValue) = pValue->pPrintableString;
#define IA5_STRING(pValue) = pValue->pIA5String;
#define OCTET_STRING_LENGTH(pValue) = pValue->pOctetString->ulNumBytes;
#define OCTET_STRING_DATA(pValue) = pValue->pOctetString->pBytes;
#define NT_SECURITY_DESCRIPTOR_LENGTH(pValue) = pValue->pNTSecurityDescriptor->ulNumBytes;
#define NT_SECURITY_DESCRIPTOR_DATA(pValue) = pValue->pNTSecurityDescriptor->pBytes;

#define DIRECTORY_FREE_STRING(pszStr) \
    if (pszStr) { \
        DirectoryFreeString(pszStr); \
    }

#define DIRECTORY_FREE_STRING_AND_RESET(pszStr) \
    if (pszStr) { \
        DirectoryFreeString(pszStr); \
        *(pszStr) = NULL; \
    }

#define DIRECTORY_FREE_MEMORY(pMem) \
    if (pMem) { \
        DirectoryFreeMemory(pMem); \
    }

#define DIRECTORY_FREE_MEMORY_AND_RESET(pMem) \
    if (pMem) { \
        DirectoryFreeMemory(pMem); \
        *(pMem) = NULL; \
    }

DWORD
DirectoryOpen(
    PHANDLE phDirectory
    );

DWORD
DirectoryBind(
    HANDLE hDirectory,
    PWSTR  pwszDistinguishedName,
    PWSTR  pwszCredentials,
    ULONG  ulMethod
    );

DWORD
DirectoryAddObject(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD attributes[]
    );

DWORD
DirectoryModifyObject(
    HANDLE        hDirectory,
    PWSTR         pwszObjectDN,
    DIRECTORY_MOD modifications[]
    );

DWORD
DirectorySearch(
    HANDLE            hDirectory,
    PWSTR             pwszBase,
    ULONG             ulScope,
    PWSTR             pwszFilter,
    PWSTR             wszAttributes[],
    ULONG             ulAttributesOnly,
    PDIRECTORY_ENTRY* ppDirectoryEntries,
    PDWORD            pdwNumEntries
    );

DWORD
DirectoryDeleteObject(
    HANDLE hBindHandle,
    PWSTR  pwszObjectDN
    );

DWORD
DirectorySetPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

DWORD
DirectoryVerifyPassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszPassword
    );

DWORD
DirectoryGetUserCount(
    HANDLE hBindHandle,
    PDWORD pdwNumUsers
    );

DWORD
DirectoryGetGroupCount(
    HANDLE hBindHandle,
    PDWORD pdwNumGroups
    );

DWORD
DirectoryChangePassword(
    HANDLE hBindHandle,
    PWSTR  pwszUserDN,
    PWSTR  pwszOldPassword,
    PWSTR  pwszNewPassword
    );

VOID
DirectoryClose(
    HANDLE hDirectory
    );

VOID
DirectoryFreeEntries(
    PDIRECTORY_ENTRY pEntries,
    DWORD            dwNumEntries
    );

VOID
DirectoryFreeAttributeValues(
    PATTRIBUTE_VALUE pAttrValues,
    DWORD            dwNumValues
    );

DWORD
DirectoryGetEntryAttributeSingle(
    PDIRECTORY_ENTRY pEntry,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    );

DWORD
DirectoryGetEntryAttributeByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    );

DWORD
DirectoryGetEntryAttributeByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttributeName,
    PDIRECTORY_ATTRIBUTE *ppAttribute
    );

DWORD
DirectoryGetAttributeValue(
    PDIRECTORY_ATTRIBUTE pAttribute,
    PATTRIBUTE_VALUE *ppAttrValue
    );

DWORD
DirectoryGetEntryAttrValueByName(
    PDIRECTORY_ENTRY pEntry,
    PCWSTR pwszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    );

DWORD
DirectoryGetEntryAttrValueByNameA(
    PDIRECTORY_ENTRY pEntry,
    PCSTR pszAttrName,
    DIRECTORY_ATTR_TYPE AttrType,
    void *pValue
    );

DWORD
DirectoryAllocateMemory(
    size_t sSize,
    PVOID* ppMemory
    );

DWORD
DirectoryReallocMemory(
    PVOID  pMemory,
    PVOID* ppNewMemory,
    size_t sSize
    );

VOID
DirectoryFreeMemory(
    PVOID pMemory
    );

DWORD
DirectoryAllocateStringW(
    PWSTR  pwszInputString,
    PWSTR* ppwszOutputString
    );

DWORD
DirectoryAllocateString(
    PCSTR pszInputString,
    PSTR* ppszOutputString
    );

VOID
DirectoryFreeStringW(
    PWSTR pwszString
    );

VOID
DirectoryFreeString(
    PSTR pszString
    );

VOID
DirectoryFreeStringArray(
    PWSTR* ppStringArray,
    DWORD  dwCount
    );

#endif /* __DIRECTORY_H__ */

