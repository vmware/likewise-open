#ifndef __SRV_TASK_H__
#define __SRV_TASK_H__

NTSTATUS
SrvContextCreate(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pRequest,
    PLWIO_SRV_CONTEXT*  ppContext
    );

VOID
SrvContextFree(
    PVOID pContext
    );

#endif /* __SRV_TASK_H__ */
