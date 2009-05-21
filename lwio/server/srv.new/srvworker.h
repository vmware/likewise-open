#ifndef __WORKER_H__
#define __WORKER_H__

NTSTATUS
SrvWorkerInit(
    PSMB_PROD_CONS_QUEUE pWorkQueue,
    PLWIO_SRV_WORKER      pWorker
    );

VOID
SrvWorkerIndicateStop(
    PLWIO_SRV_WORKER pWorker
    );

VOID
SrvWorkerFreeContents(
    PLWIO_SRV_WORKER pWorker
    );

#endif /* __WORKER_H__ */
