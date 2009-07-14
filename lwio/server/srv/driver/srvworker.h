#ifndef __WORKER_H__
#define __WORKER_H__

NTSTATUS
SrvWorkerInit(
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
