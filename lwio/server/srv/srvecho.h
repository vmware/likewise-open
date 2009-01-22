#ifndef __SRVECHO_H__
#define __SRVECHO_H__

NTSTATUS
SrvProcessEchoAndX(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest
    );

#endif /* __SRVECHO_H__ */
