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
    PSMB_SRV_TREE           pTree,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PSMB_SRV_FILE*          ppFile
    );

NTSTATUS
SrvTreeRemoveFile(
    PSMB_SRV_TREE pTree,
    USHORT        fid
    );

VOID
SrvTreeRelease(
    PSMB_SRV_TREE pTree
    );

#endif /* __SRV_TREE_H__ */
