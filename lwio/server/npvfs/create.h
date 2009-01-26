
NTSTATUS
NpfsCommonCreate(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );

NTSTATUS
NpfsValidateCreateOptions(
    PNPFS_IRP_CONTEXT pIrpContext,
    PWSTR * ppszPipeName
    );

NTSTATUS
NpfsAllocateIrpContext(
    PIRP pIrp,
    PNPFS_IRP_CONTEXT * ppIrpContext
    );
