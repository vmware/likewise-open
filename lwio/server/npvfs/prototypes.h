/* memory.c */
NTSTATUS
NpfsAllocateMemory(
    );
VOID
NpfsFreeMemory(
    );

/* pipe.c */

NTSTATUS
NpfsFindAvailablePipe(
    PNPFS_FCB pFCB,
    PNPFS_PIPE * ppPipe
    );

NTSTATUS
NpfsFreePipeContext(
    PNPFS_PIPE pPipe
    );

/* ccb.c */


NTSTATUS
NpfsClientCreateCCB(
    PNPFS_IRP_CONTEXT pIrpContext,
    PNPFS_CCB * ppCCB
    );


/* fcb.c */



NTSTATUS
NpfsCreateFCB(
    PWSTR pUnicodeString,
    PNPFS_FCB * ppFcb
    );

NTSTATUS
NpfsFindFCB(
    PWSTR pUnicodeString,
    PNPFS_FCB * ppFcb
    );



/* mdl.c */


NTSTATUS
NpfsEnqueueBuffer(
    PNPFS_MDL pMdlList,
    PVOID pBuffer,
    ULONG Length,
    PNPFS_MDL * ppMdlList
    );

NTSTATUS
NpfsDequeueBuffer(
    PNPFS_MDL pMdlList,
    PVOID pBuffer,
    ULONG Length,
    PNPFS_MDL * ppMdlList
    );

NTSTATUS
NpfsCreateMdl(
    ULONG Length,
    PVOID pBuffer,
    PNPFS_MDL * ppMdl
    );

NTSTATUS
NpfsEnqueueMdl(
    PNPFS_MDL pMdlList,
    PNPFS_MDL pMdl,
    PNPFS_MDL *ppMdlList
    );

NTSTATUS
NpfsDequeueMdl(
    PNPFS_MDL pMdlList,
    PNPFS_MDL pMdl,
    PNPFS_MDL *ppMdlList
    );

NTSTATUS
NpfsCopyMdl(
    PNPFS_MDL pMdl,
    PVOID pBuffer,
    ULONG Length,
    ULONG *ppLengthCopied
    );

NTSTATUS
NpfsDeleteMdl(
    PNPFS_MDL pMdl
    );

NTSTATUS
NpfsAddMdltoInboundQueue(
    PNPFS_CCB pCCB,
    PNPFS_MDL pMdl
    );

NTSTATUS
NpfsFreeMdlList(
    PNPFS_MDL pNpfsMdlList
    );

VOID
NpfsFreeMdl(
    PNPFS_MDL pMdl
    );

/* read.c */

NTSTATUS
NpfsRead(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );


NTSTATUS
NpfsCommonRead(
    PNPFS_IRP_CONTEXT pIrpContext,
    PIRP pIrp
    );


NTSTATUS
NpfsReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );


NTSTATUS
NpfsServerReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );


NTSTATUS
NpfsClientReadFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsServerReadFile_Connected(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsClientReadFile_Connected(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

/* write.c */

NTSTATUS
NpfsWrite(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PIRP pIrp
    );


NTSTATUS
NpfsWriteFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsServerWriteFile(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsClientWriteFile(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsServerWriteFile_Connected(
    PNPFS_CCB pSCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
NpfsClientWriteFile_Connected(
    PNPFS_CCB pCCB,
    PNPFS_IRP_CONTEXT pIrpContext
    );
