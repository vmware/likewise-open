
typedef struct _PVFS_CCB{
    //LIST_ENTRY NextCCB;
} PVFS_CCB, *PPVFS_CCB;

typedef struct _PVFS_IRP_CONTEXT {
    PIRP pIrp;
    IO_DEVICE_HANDLE TargetDeviceHandle;
} PVFS_IRP_CONTEXT, *PPVFS_IRP_CONTEXT;
