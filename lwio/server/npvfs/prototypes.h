/* memory.c */
NTSTATUS
NpfsAllocateMemory(
    );
VOID
NpfsFreeMemory(
    );

/* pipe.c */

NTSTATUS
NpfsFreePipeContext(
    PNPFS_PIPE pPipe
    );

/* mdl.c */

NTSTATUS
NpfsFreeMdlList(
    PNPFS_MDL pMdlList
    );


NTSTATUS
NpfsEnqueueBuffer(
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
