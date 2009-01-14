
NTSTATUS
RdrCommonCreate(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrAllocateIrpContext(
    PIRP pIrp,
    PRDR_IRP_CONTEXT * ppIrpContext
    );

NTSTATUS
RdrCommonCreateFile(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrBuildAbsolutePathName(
    UNICODE_STRING RootPathName,
    UNICODE_STRING RelativePathName,
    UNICODE_STRING AbsolutePathName
    );

NTSTATUS
RdrGetFilePathName(
    IO_FILE_HANDLE hFileHandle,
    UNICODE_STRING PathName
    );

NTSTATUS
RdrCommonCreateDirectory(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileSupersede(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileCreate(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileOpen(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileOpenIf(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileOverwrite(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateFileOverwriteIf(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS
RdrCommonCreateDirectory(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS
RdrCommonCreateDirectoryFileSupersede(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileCreate(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileOpen(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileOpenIf(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileOverwrite(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
RdrCommonCreateDirectoryFileOverwriteIf(
    PRDR_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


