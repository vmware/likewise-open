#ifndef __SRV_CLOSE_H__
#define __SRV_CLOSE_H__

NTSTATUS
SrvProcessCloseAndX(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    );

#endif

