#ifndef __SRV_FILE_H__
#define __SRV_FILE_H__

NTSTATUS
SrvFileCreate(
    USHORT                  fid,
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

VOID
SrvFileRelease(
    PSMB_SRV_FILE pFile
    );

#endif /* __SRV_FILE_H__ */
