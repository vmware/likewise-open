#ifndef __SRV_CONNECTION_H__
#define __SRV_CONNECTION_H__

NTSTATUS
SrvConnectionCreate(
    PSMB_SRV_SOCKET      pSocket,
    HANDLE               hPacketAllocator,
    HANDLE               hGssContext,
    PSRV_PROPERTIES      pServerProperties,
    PSMB_SRV_CONNECTION* ppConnection
    );

int
SrvConnectionGetFd(
    PSMB_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionGetNextSequence(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PULONG              pulRequestSequence,
    PULONG              pulResponseSequence
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
    PSMB_PACKET         pPacket
    );

PSMB_SRV_SESSION
SrvConnectionFindSession(
    PSMB_SRV_CONNECTION pConnection,
    USHORT uid
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
