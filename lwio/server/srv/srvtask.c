#include "includes.h"

NTSTATUS
SrvTaskCreate(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pRequest,
    PLWIO_SRV_TASK*     ppTask
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_TASK pTask = NULL;

    ntStatus = LW_RTL_ALLOCATE(
                    &pTask,
                    LWIO_SRV_TASK,
                    sizeof(LWIO_SRV_TASK));
    BAIL_ON_NT_STATUS(ntStatus);

    pTask->pConnection = pConnection;

    InterlockedIncrement(&pConnection->refCount);

    pTask->pRequest = pRequest;

    *ppTask = pTask;

cleanup:

    return ntStatus;

error:

    *ppTask = NULL;

    goto cleanup;
}

VOID
SrvTaskFree(
    PVOID pTask
    )
{
    PLWIO_SRV_TASK pIOTask = (PLWIO_SRV_TASK)pTask;

    if (pIOTask->pConnection)
    {
        SrvConnectionRelease(pIOTask->pConnection);
    }

    LwRtlMemoryFree(pTask);
}
