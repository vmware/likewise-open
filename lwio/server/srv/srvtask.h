#ifndef __SRV_TASK_H__
#define __SRV_TASK_H__

NTSTATUS
SrvTaskCreate(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pRequest,
    PLWIO_SRV_TASK*     ppTask
    );

VOID
SrvTaskFree(
    PVOID pTask
    );

#endif /* __SRV_TASK_H__ */
