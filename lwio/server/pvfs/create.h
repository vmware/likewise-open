
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

NTSTATUS
PvfsCommonCreateFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsCommonCreateFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsCommonCreateFileOpen(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS 
PvfsCommonCreateFileOpenIf(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsCommonCreateFileOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsCommonCreateFileOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS 
PvfsCommonCreateDirectory(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS
PvfsCommonCreateDirectoryFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsCommonCreateDirectoryFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsCommonCreateDirectoryFileOpen(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS 
PvfsCommonCreateDirectoryFileOpenIf(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsCommonCreateDirectoryFileOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
PvfsCommonCreateDirectoryFileOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


