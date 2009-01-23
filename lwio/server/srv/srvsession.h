#ifndef __SRV_SESSION_H__
#define __SRV_SESSION_H__

NTSTATUS
SrvSessionCreate(
    PSRV_ID_ALLOCATOR pIdAllocator,
    PSMB_SRV_SESSION* ppSession
    );

NTSTATUS
SrvSessionFindTree(
    PSMB_SRV_SESSION pSession,
    USHORT           tid,
    PSMB_SRV_TREE*   ppTree
    );

NTSTATUS
SrvSessionCreateTree(
    PSMB_SRV_SESSION pSession,
    PSMB_SRV_TREE*   ppTree
    );

VOID
SrvSessionRelease(
    PSMB_SRV_SESSION pSession
    );

#endif /* __SRV_SESSION_H__ */
