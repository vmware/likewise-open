
NTSTATUS
PvfsCommonCreate(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsAllocateIrpContext(
    PIRP pIrp,
    PPVFS_IRP_CONTEXT * ppIrpContext
    );

NTSTATUS
PvfsCommonCreateFile(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsBuildAbsolutePathName(
    IO_UNICODE_STRING RootPathName,
    IO_UNICODE_STRING RelativePathName,
    IO_UNICODE_STRING AbsolutePathName
    );

NTSTATUS
PvfsGetFilePathName(
    IO_FILE_HANDLE hFileHandle,
    IO_UNICODE_STRING PathName
    );

NTSTATUS
PvfsCommonCreateDirectory(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

