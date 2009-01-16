#ifndef __SRV_CONNECTION_H__
#define __SRV_CONNECTION_H__

NTSTATUS
SrvConnectionCreate(
    PSMB_SRV_SOCKET pSocket,
    HANDLE          hPacketAllocator,
    PSMB_SRV_CONNECTION* ppConnection
    );

int
SrvConnectionGetFd(
    PSMB_SRV_CONNECTION pConnection
    );

BOOLEAN
SrvConnectionIsInvalid(
    PSMB_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionReadPacket(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET* ppPacket
    );

VOID
SrvConnectionRelease(
    PSMB_SRV_CONNECTION pConnection
    );

#endif /* __SRV_CONNECTION_H__ */
