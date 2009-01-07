
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
