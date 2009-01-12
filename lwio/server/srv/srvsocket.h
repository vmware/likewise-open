#ifndef __SRVSOCK_H__
#define __SRVSOCK_H__

NTSTATUS
SrvSocketCreate(
    int fd,
    struct sockaddr_in* pClientAddr,
    PSMB_SRV_SOCKET* ppSocket
    );

VOID
SrvSocketFree(
    PSMB_SRV_SOCKET pSocket
    );

#endif /* __SRVSOCK_H__ */
