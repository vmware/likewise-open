#ifndef __SRV_FILE_H__
#define __SRV_FILE_H__

NTSTATUS
SrvFileCreate(
    PSRV_ID_ALLOCATOR pIdAllocator,
    PSMB_SRV_FILE*    ppFile
    );

VOID
SrvFileRelease(
    PSMB_SRV_FILE pFile
    );

#endif /* __SRV_FILE_H__ */
