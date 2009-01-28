#ifndef __SRV_SESSION_H__
#define __SRV_SESSION_H__

NTSTATUS
SrvSessionCreate(
    USHORT            uid,
    PSMB_SRV_SESSION* ppSession
    );

NTSTATUS
SrvSessionFindTree(
    PSMB_SRV_SESSION pSession,
    USHORT           tid,
    PSMB_SRV_TREE*   ppTree
    );

NTSTATUS
SrvSessionRemoveTree(
    PSMB_SRV_SESSION pSession,
    USHORT           tid
    );

NTSTATUS
SrvSessionCreateTree(
    PSMB_SRV_SESSION pSession,
    PSHARE_DB_INFO   pShareInfo,
    PSMB_SRV_TREE*   ppTree
    );

VOID
SrvSessionRelease(
    PSMB_SRV_SESSION pSession
    );

#endif /* __SRV_SESSION_H__ */
