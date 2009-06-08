#ifndef __SRV_CONNECTION_H__
#define __SRV_CONNECTION_H__

NTSTATUS
SrvConnectionCreate(
    PLWIO_SRV_SOCKET           pSocket,
    HANDLE                    hPacketAllocator,
    HANDLE                    hGssContext,
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    PSRV_PROPERTIES           pServerProperties,
    PSRV_HOST_INFO            pHostinfo,
    PLWIO_SRV_CONNECTION*      ppConnection
    );

int
SrvConnectionGetFd(
    PLWIO_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionGetNextSequence(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PULONG              pulRequestSequence
    );

BOOLEAN
SrvConnectionIsInvalid(
    PLWIO_SRV_CONNECTION pConnection
    );

VOID
SrvConnectionSetInvalid(
    PLWIO_SRV_CONNECTION pConnection
    );

LWIO_SRV_CONN_STATE
SrvConnectionGetState(
    PLWIO_SRV_CONNECTION pConnection
    );

VOID
SrvConnectionSetState(
    PLWIO_SRV_CONNECTION pConnection,
    LWIO_SRV_CONN_STATE  connState
    );

NTSTATUS
SrvConnectionReadPacket(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET* ppPacket
    );

NTSTATUS
SrvConnectionWriteMessage(
    PLWIO_SRV_CONNECTION pConnection,
    ULONG               ulSequence,
    PSMB_PACKET         pPacket
    );

NTSTATUS
SrvConnectionFindSession(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT uid,
    PLWIO_SRV_SESSION* ppSession
    );

NTSTATUS
SrvConnectionRemoveSession(
    PLWIO_SRV_CONNECTION pConnection,
    USHORT              uid
    );

NTSTATUS
SrvConnectionCreateSession(
    PLWIO_SRV_CONNECTION pConnection,
    PLWIO_SRV_SESSION* ppSession
    );

NTSTATUS
SrvConnectionGetNamedPipeSessionKey(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

NTSTATUS
SrvConnectionGetNamedPipeClientAddress(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

VOID
SrvConnectionRelease(
    PLWIO_SRV_CONNECTION pConnection
    );

#endif /* __SRV_CONNECTION_H__ */
