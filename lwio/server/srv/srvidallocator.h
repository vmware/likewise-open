#ifndef __SRV_ID_ALLOCATOR_H__
#define __SRV_ID_ALLOCATOR_H__

NTSTATUS
SrvIdAllocatorCreate(
    USHORT usMaxId,
    PSRV_ID_ALLOCATOR* ppIdAllocator
    );

NTSTATUS
SrvIdAllocatorAcquireId(
    PSRV_ID_ALLOCATOR pIdAllocator,
    PUSHORT pusId
    );

VOID
SrvIdAllocatorReleaseId(
    PSRV_ID_ALLOCATOR pIdAllocator,
    USHORT usId
    );

VOID
SrvIdAllocatorRelease(
    PSRV_ID_ALLOCATOR pIdAllocator
    );

#endif /* __SRV_ID_ALLOCATOR_H__ */
