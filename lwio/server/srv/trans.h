#ifndef __SRV_TRANS_H__
#define __SRV_TRANS_H__

NTSTATUS
SrvProcessTransaction(
    PLWIO_SRV_CONTEXT pContext,
    PSMB_PACKET*      ppSmbResponse
    );

#endif /* __SRV_TRANS_H__ */
