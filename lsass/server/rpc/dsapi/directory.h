#define  TYPE_BOOLEAN           1
#define  TYPE_INTEGER           2
#define  TYPE_LARGE_INTEGER     3
#define  TYPE_NT_SECURITY_DESCRIPTOR
#define  TYPE_OCTET_STRING      5
#define  TYPE_PRINTABLE_STRING  6


typedef struct _OCTET_STRING {
    ULONG ulNumBytes;
    PBYTE pBytes
} OCTET_STRING, *POCTET_STRING;

typedef struct _ATTRIBUTE_VALUE {
    ULONG Type;
    union {
        ULONG uLongValue;
        PWSTR pszStringValue;
        BOOL  bBooleanValue;
        POCTET_STRING pOctetString;
    }
} ATTRIBUTE_VALUE, *PATTRIBUTE_VALUE;


typedef struct _DIRECTORY_ATTRIBUTE {
    PWSTR AttributeName;
    ULONG ulNumValues;
    PATTRIBUTE_VALUE * ppAttributeValues;
} DIRECTORY_ATTRIBUTE, *PDIRECTORY_ATTRIBUTE;

typedef struct _DIRECTORY_MOD {
    ULONG Operation;
    PWSTR AttributeName;
    ULONG ulType;
    ULONG ulNumValues;
    PDIRECTORY_VALUE *pAttribuValues;
} DIRECTORY_MOD, *PDIRECTORY_MOD;

typedef struct _DIRECTORY_ENTRY{
    ULONG ulNumAttributes;
    PDIRECTORY_ATTRIBUTE * ppDirectoryAttributes;
}DIRECTORY_ENTRY, *PDIRECTORY_ENTRY;

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

NTSTATUS
DirectoryAddObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    PDIRECTORY_MODS Attributes[]
    );


NTSTATUS
DirectoryModifyObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    PDIRECTORY_MODS Modifications[]
    );

NTSTATUS
DirectorySearch(
    HANDLE hDirectory,
    PWSTR Base,
    ULONG Scope,
    PWSTR Filter,
    PWSTR Attributes[],
    ULONG AttributesOnly,
    PDIRECTORY_ENTRY * ppDirectoryEntries
    PDWORD pdwNumValues
    );

NTSTATUS
DirectoryDeleteObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    );
