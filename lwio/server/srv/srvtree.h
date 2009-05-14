#ifndef __SRV_TREE_H__
#define __SRV_TREE_H__

NTSTATUS
SrvTreeCreate(
    USHORT            tid,
    PSHARE_DB_INFO    pShareInfo,
    PLWIO_SRV_TREE*    ppTree
    );

NTSTATUS
SrvTreeFindFile(
    PLWIO_SRV_TREE  pTree,
    USHORT         fid,
    PLWIO_SRV_FILE* ppFile
    );

NTSTATUS
SrvTreeCreateFile(
    PLWIO_SRV_TREE           pTree,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_SRV_FILE*          ppFile
    );

NTSTATUS
SrvTreeRemoveFile(
    PLWIO_SRV_TREE pTree,
    USHORT        fid
    );

BOOLEAN
SrvTreeIsNamedPipe(
    PLWIO_SRV_TREE pTree
    );

VOID
SrvTreeRelease(
    PLWIO_SRV_TREE pTree
    );

#endif /* __SRV_TREE_H__ */
