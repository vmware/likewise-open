#ifndef _SRV_SESSION_SETUP_H__
#define _SRV_SESSION_SETUP_H__

NTSTATUS
SrvProcessSessionSetup(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    );

#endif /* _SRV_SESSION_SETUP_H__ */
