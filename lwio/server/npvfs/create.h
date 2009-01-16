
NTSTATUS
NpfsCommonCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsAllocateIrpContext(
    PIRP pIrp,
    PNPFS_IRP_CONTEXT * ppIrpContext
    );

NTSTATUS
NpfsCommonCreateFile(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsBuildAbsolutePathName(
    UNICODE_STRING RootPathName,
    UNICODE_STRING RelativePathName,
    UNICODE_STRING AbsolutePathName
    );

NTSTATUS
NpfsGetFilePathName(
    IO_FILE_HANDLE hFileHandle,
    UNICODE_STRING PathName
    );

NTSTATUS
NpfsCommonCreateDirectory(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateFileSupersede(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateFileCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateFileOpen(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateFileOpenIf(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateFileOverwrite(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateFileOverwriteIf(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS
NpfsCommonCreateDirectory(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS
NpfsCommonCreateDirectoryFileSupersede(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateDirectoryFileCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateDirectoryFileOpen(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateDirectoryFileOpenIf(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateDirectoryFileOverwrite(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsCommonCreateDirectoryFileOverwriteIf(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


