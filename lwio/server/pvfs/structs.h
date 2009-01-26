
typedef struct _PVFS_CCB{
    int fd;
} PVFS_CCB, *PPVFS_CCB;

typedef struct _PVFS_IRP_CONTEXT {
    PIRP pIrp;
} PVFS_IRP_CONTEXT, *PPVFS_IRP_CONTEXT;
