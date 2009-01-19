
typedef struct _NPFS_MDL{
    ULONG Length;
    PVOID Buffer;
    struct _NPFS_MDL *pNext;
} NPFS_MDL, *PNPFS_MDL;

typedef struct _NPFS_CCB{

    ULONG CcbType;

    //LIST_ENTRY NextCCB;
    UNICODE_STRING AbsolutePathName;
    ACCESS_MASK DesiredAccess;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    FILE_SHARE_FLAGS ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions;
    PIO_EA_BUFFER pEaBuffer;

} NPFS_CCB, *PNPFS_CCB;


typedef struct _NPFS_PIPE {
    //CRITICAL_SECTION  PipeMutex;
    PNPFS_CCB pScb;
    PNPFS_CCB pCcb;
} NPFS_PIPE, *PNPFS_PIPE;

typedef struct _NPFS_IRP_CONTEXT {
    PIRP pIrp;
    IO_DEVICE_HANDLE TargetDeviceHandle;
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;
} NPFS_IRP_CONTEXT, *PNPFS_IRP_CONTEXT;
