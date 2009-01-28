#ifndef __SRV_TREE_H__
#define __SRV_TREE_H__

NTSTATUS
SrvTreeCreate(
    USHORT            tid,
    PSHARE_DB_INFO    pShareInfo,
    PSMB_SRV_TREE*    ppTree
    );

NTSTATUS
SrvTreeFindFile(
    PSMB_SRV_TREE  pTree,
    USHORT         fid,
    PSMB_SRV_FILE* ppFile
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
