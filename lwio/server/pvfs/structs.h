
typedef struct _PVFS_CCB{
    //LIST_ENTRY NextCCB;
    UNICODE_STRING AbsolutePathName;
    ACCESS_MASK DesiredAccess;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    FILE_SHARE_FLAGS ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions;
    
    int fd;
    char *path;
    int oflags;
    mode_t mode;
} PVFS_CCB, *PPVFS_CCB;

typedef struct _PVFS_IRP_CONTEXT {
    PIRP pIrp;
} PVFS_IRP_CONTEXT, *PPVFS_IRP_CONTEXT;
