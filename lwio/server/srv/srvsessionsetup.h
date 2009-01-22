#ifndef _SRV_SESSION_SETUP_H__
#define _SRV_SESSION_SETUP_H__

NTSTATUS
SrvProcessSessionSetup(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest
    );

#endif /* _SRV_SESSION_SETUP_H__ */
