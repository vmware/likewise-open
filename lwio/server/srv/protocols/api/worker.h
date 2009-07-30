#ifndef __WORKER_H__
#define __WORKER_H__

NTSTATUS
SrvProtocolWorkerInit(
    PLWIO_SRV_PROTOCOL_WORKER pWorker,
    PSMB_PROD_CONS_QUEUE      pWorkQueue
    );

VOID
SrvProtocolWorkerIndicateStop(
    PLWIO_SRV_PROTOCOL_WORKER pWorker
    );

VOID
SrvProtocolWorkerFreeContents(
    PLWIO_SRV_PROTOCOL_WORKER pWorker
    );

#endif /* __WORKER_H__ */
