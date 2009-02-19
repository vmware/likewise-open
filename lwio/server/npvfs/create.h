
NTSTATUS
NpfsCommonCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsValidateCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PUNICODE_STRING pPipeName
    );

NTSTATUS
NpfsAllocateIrpContext(
    PIRP pIrp,
    PNPFS_IRP_CONTEXT * ppIrpContext
    );
