#ifndef __SRV_CONNECTION_H__
#define __SRV_CONNECTION_H__

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
SrvConnectionGetNamedPipeClientAddress(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

#endif /* __SRV_CONNECTION_H__ */
