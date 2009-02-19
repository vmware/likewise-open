#include "includes.h"

NTSTATUS
SrvTaskCreate(
    PSMB_SRV_CONNECTION pConnection,
    PSMB_PACKET         pRequest,
    PLWIO_SRV_TASK*     ppTask
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_TASK pTask = NULL;

    ntStatus = SMBAllocateMemory(
                    sizeof(LWIO_SRV_TASK),
                    (PVOID*)&pTask);
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

    SMBFreeMemory(pTask);
}
