#ifndef __SRV_SESSION_H__
#define __SRV_SESSION_H__

NTSTATUS
SrvSessionCreate(
    USHORT            uid,
    PLWIO_SRV_SESSION* ppSession
    );

NTSTATUS
SrvSessionFindTree(
    PLWIO_SRV_SESSION pSession,
    USHORT           tid,
    PLWIO_SRV_TREE*   ppTree
    );

NTSTATUS
SrvSessionRemoveTree(
    PLWIO_SRV_SESSION pSession,
    USHORT           tid
    );

NTSTATUS
SrvSessionCreateTree(
    PLWIO_SRV_SESSION pSession,
    PSHARE_DB_INFO   pShareInfo,
    PLWIO_SRV_TREE*   ppTree
    );

NTSTATUS
SrvSessionGetNamedPipeClientPrincipal(
    PLWIO_SRV_SESSION pSession,
    PIO_ECP_LIST     pEcpList
    );

VOID
SrvSessionRelease(
    PLWIO_SRV_SESSION pSession
    );

#endif /* __SRV_SESSION_H__ */
