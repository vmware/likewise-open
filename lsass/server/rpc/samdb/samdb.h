
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
DirectoryAddObject();

NTSTATUS
DirectoryModifyObject();

NTSTATUS
DirectorySearchObject();


NTSTATUS

DirectoryDeleteObject();
