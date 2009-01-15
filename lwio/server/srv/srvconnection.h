#ifndef __SRV_CONNECTION_H__
#define __SRV_CONNECTION_H__

NTSTATUS
SrvConnectionCreate(
    PSMB_SRV_SOCKET pSocket,
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

VOID
SrvConnectionRelease(
    PSMB_SRV_CONNECTION pConnection
    );

#endif /* __SRV_CONNECTION_H__ */
