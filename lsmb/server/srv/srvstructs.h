
typedef struct _SRV_CCB{
    //LIST_ENTRY NextCCB;
    IO_UNICODE_STRING AbsolutePathName;
    ACCESS_MASK DesiredAccess;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    FILE_SHARE_FLAGS ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions;
    PIO_EA_BUFFER pEaBuffer;

    int fd;
    char *path;
    int oflags;
    mode_t mode;
} SRV_CCB, *PSRV_CCB;

typedef struct _SRV_IRP_CONTEXT {
    PIRP pIrp;
    IO_DEVICE_HANDLE TargetDeviceHandle;
    IO_UNICODE_STRING RootPathName;
    IO_UNICODE_STRING RelativePathName;
    IO_UNICODE_STRING AbsolutePathName;
} SRV_IRP_CONTEXT, *PSRV_IRP_CONTEXT;
