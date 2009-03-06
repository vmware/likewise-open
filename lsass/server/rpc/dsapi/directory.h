typedef struct _ATTRIBUTE_VALUE {
    ULONG Type;
    union {
        ULONG uLongValue;
        PWSTR pszStringValue;
        BOOL  bBooleanValue;
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
