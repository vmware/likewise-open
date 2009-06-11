#ifndef __SRVSOCK_H__
#define __SRVSOCK_H__

NTSTATUS
SrvSocketCreate(
    int fd,
    struct sockaddr_in* pClientAddr,
    PLWIO_SRV_SOCKET* ppSocket
    );

VOID
SrvSocketFree(
    HANDLE hSocket
    );

#endif /* __SRVSOCK_H__ */
