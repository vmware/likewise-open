#ifndef __DIRECTORY_H__
#define __DIRECTORY_H__

#define  TYPE_BOOLEAN           1
#define  TYPE_INTEGER           2
#define  TYPE_LARGE_INTEGER     3
#define  TYPE_NT_SECURITY_DESCRIPTOR
#define  TYPE_OCTET_STRING      5
#define  TYPE_PRINTABLE_STRING  6

typedef enum
{
    LOCAL_SAM = 0
} DirectoryType;

typedef struct _DIRECTORY_CONTEXT {
    DirectoryType directoryType;
    HANDLE        hBindHandle;
} DIRECTORY_CONTEXT, *PDIRECTORY_CONTEXT;

#define ENTRY(i, ppDirectoryEntry) = *(ppDirectoryEntry + i)
#define ATTRIBUTE(i, ppDirectoryAttributes) = *(ppDirectoryAttributes + i)

#define VALUE(i, pAttribute) = *(pAttribute->ppAttributeValues + i)
#define NUM_VALUES(pAttribute) = pAttribute->ulNumValues
#define INTEGER(pValue) =  pValue->uLongValue;
#define PRINTABLE_STRING(pValue) = pValue->pPrintableString;
#define IA5_STRING(pValue) = pValue->pIA5String;
#define OCTET_STRING_LENGTH(pValue) = pValue->pOctetString->ulNumBytes;
#define OCTET_STRING_DATA(pValue) = pValue->pOctetString->pBytes;
#define NT_SECURITY_DESCRIPTOR_LENGTH(pValue) = pValue->pNTSecurityDescriptor->ulNumBytes;
#define NT_SECURITY_DESCRIPTOR_DATA(pValue) = pValue->pNTSecurityDescriptor->pBytes;

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
DirectoryAllocateString(
    PWSTR  pwszInputString,
    PWSTR* ppwszOutputString
    );

VOID
DirectoryFreeString(
    PWSTR pwszString
    );

VOID
DirectoryFreeStringArray(
    PWSTR* ppStringArray,
    DWORD  dwCount
    );

DWORD
DirectoryAddObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    DIRECTORY_MOD Attributes[]
    );


DWORD
DirectoryModifyObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    DIRECTORY_MOD Modifications[]
    );

DWORD
DirectorySearch(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Filter,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PATTRIBUTE_VALUE * ppDirectoryEntries,
    PDWORD pdwNumValues
    );

DWORD
DirectoryDeleteObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN
    );

#endif /* __DIRECTORY_H__ */

