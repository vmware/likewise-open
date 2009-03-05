
typedef struct _DIRECTORY_MOD {
    ULONG Operation;
    PWSTR pszAttributeName;
    ULONG ulType;
    union {
        PWSTR pszString;
        ULONG ulLong;
    }
} DIRECTORY_MOD, *PDIRECTORY_MOD;


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
    PDIRECTORY_VALUES * ppDirectoryValues
    PDWORD pdwNumValues
    );

NTSTATUS
DirectoryDeleteObject(
    HANDLE hBindHandle,
    PWSTR ObjectDN,
    );
