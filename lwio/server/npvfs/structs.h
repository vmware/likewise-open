
struct _NPFS_PIPE;

typedef struct _NPFS_MDL{
    ULONG Length;
    PVOID Buffer;
    struct _NPFS_MDL *pNext;
} NPFS_MDL, *PNPFS_MDL;

typedef struct _NPFS_CCB{

    ULONG CcbType;
    //CRITICAL_SECTION CcbMutex;
    //EVENT  ReadEvent;
    //PNPFS_PIPE pPipe;
    struct _NPFS_PIPE * pPipe;
    //CRITICAL_SECTION InBoundMutex;
    PNPFS_MDL pMdlList;

    //LIST_ENTRY NextCCB;
    UNICODE_STRING AbsolutePathName;
    ACCESS_MASK DesiredAccess;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    FILE_SHARE_FLAGS ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions;

} NPFS_CCB, *PNPFS_CCB;


typedef struct _NPFS_PIPE {
   // CRITICAL_SECTION  PipeMutex;
    ULONG PipeClientState;
    ULONG PipeServerState;
    PNPFS_CCB pSCB;
    PNPFS_CCB pCCB;

    struct _NPFS_PIPE *pNext;
} NPFS_PIPE, *PNPFS_PIPE;

typedef struct _NPFS_FCB {
    PNPFS_PIPE pPipeList;
    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    UNICODE_STRING PipeName;
    PNPFS_PIPE pPipes;

    struct _NPFS_FCB *pNext;
}NPFS_FCB, *PNPFS_FCB;

typedef struct _NPFS_IRP_CONTEXT {
    PIRP pIrp;
    IO_DEVICE_HANDLE TargetDeviceHandle;
    UNICODE_STRING RootPathName;
    UNICODE_STRING RelativePathName;
    UNICODE_STRING AbsolutePathName;
} NPFS_IRP_CONTEXT, *PNPFS_IRP_CONTEXT;
