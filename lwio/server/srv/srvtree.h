#ifndef __SRV_TREE_H__
#define __SRV_TREE_H__

NTSTATUS
SrvTreeCreate(
    USHORT         usTreeId,
    PSMB_SRV_TREE* ppTree
    );

NTSTATUS
SrvTreeCreateFile(
    PSMB_SRV_TREE  pTree,
    USHORT         fid,
    PSMB_SRV_FILE* ppFile
    );

VOID
SrvTreeRelease(
    PSMB_SRV_TREE pTree
    );

#endif /* __SRV_TREE_H__ */
