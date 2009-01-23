#ifndef __SRV_SESSION_H__
#define __SRV_SESSION_H__

NTSTATUS
SrvSessionCreate(
    USHORT            uid,
    PSMB_SRV_SESSION* ppSession
    );

NTSTATUS
SrvSessionCreateTree(
    PSMB_SRV_SESSION pSession,
    USHORT           tid,
    PSMB_SRV_TREE*   ppTree
    );

VOID
SrvSessionRelease(
    PSMB_SRV_SESSION pSession
    );

#endif /* __SRV_SESSION_H__ */
