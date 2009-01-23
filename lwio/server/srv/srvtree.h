#ifndef __SRV_TREE_H__
#define __SRV_TREE_H__

NTSTATUS
SrvTreeCreate(
    PSRV_ID_ALLOCATOR pIdAllocator,
    PSMB_SRV_TREE*    ppTree
    );

PSMB_SRV_FILE
SrvTreeFindFile(
    PSMB_SRV_TREE pTree,
    USHORT        fid
    );

NTSTATUS
SrvTreeCreateFile(
    PSMB_SRV_TREE  pTree,
    PSMB_SRV_FILE* ppFile
    );

VOID
SrvTreeRelease(
    PSMB_SRV_TREE pTree
    );

#endif /* __SRV_TREE_H__ */
