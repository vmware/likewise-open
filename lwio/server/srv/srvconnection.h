#ifndef __SRV_CONNECTION_H__
#define __SRV_CONNECTION_H__

NTSTATUS
SrvConnectionCreate(
    PSMB_SRV_SOCKET           pSocket,
    HANDLE                    hPacketAllocator,
    HANDLE                    hGssContext,
    PSMB_SRV_SHARE_DB_CONTEXT pShareDbContext,
    PSRV_PROPERTIES           pServerProperties,
    PSRV_HOST_INFO            pHostinfo,
    PSMB_SRV_CONNECTION*      ppConnection
    );

int
SrvConnectionGetFd(
    PSMB_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionGetNextSequence(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PULONG              pulRequestSequence
    );

BOOLEAN
SrvConnectionIsInvalid(
    PSMB_SRV_CONNECTION pConnection
    );

VOID
SrvConnectionSetInvalid(
    PSMB_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionReadPacket(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET* ppPacket
    );

NTSTATUS
SrvConnectionWriteMessage(
    PSMB_SRV_CONNECTION pConnection,
    ULONG               ulSequence,
    PSMB_PACKET         pPacket
    );

NTSTATUS
SrvConnectionFindSession(
    PSMB_SRV_CONNECTION pConnection,
    USHORT uid,
    PSMB_SRV_SESSION* ppSession
    );

NTSTATUS
SrvConnectionRemoveSession(
    PSMB_SRV_CONNECTION pConnection,
    USHORT              uid
    );

NTSTATUS
SrvConnectionCreateSession(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_SRV_SESSION* ppSession
    );

VOID
SrvConnectionRelease(
    PSMB_SRV_CONNECTION pConnection
    );

#endif /* __SRV_CONNECTION_H__ */
