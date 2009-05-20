#ifndef __SRVECHO_H__
#define __SRVECHO_H__

NTSTATUS
SrvProcessEchoAndX(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    );

#endif /* __SRVECHO_H__ */
