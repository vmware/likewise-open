#ifndef __WORKER_H__
#define __WORKER_H__

NTSTATUS
SrvWorkerInit(
    PSMB_SRV_WORKER pWorker
    );

NTSTATUS
SrvWorkerFreeContents(
    PSMB_SRV_WORKER pWorker
    );

#endif /* __WORKER_H__ */
