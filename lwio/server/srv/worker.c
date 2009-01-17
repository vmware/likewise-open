#include "includes.h"

static
BOOLEAN
SrvWorkerMustStop(
    PSMB_SRV_WORKER_CONTEXT pContext
    );

static
NTSTATUS
SrvWorkerStop(
    PSMB_SRV_WORKER_CONTEXT pContext
    );

static
NTSTATUS
SrvWorkerExecute(
    PSMB_SRV_TASK pTask
    );

NTSTATUS
SrvWorkerInit(
    PSMB_PROD_CONS_QUEUE pWorkQueue,
    PSMB_SRV_WORKER pWorker
    )
{
    NTSTATUS ntStatus = 0;

    memset(&pWorker->context, 0, sizeof(pWorker->context));

    pthread_mutex_init(&pWorker->context.mutex);
    pWorker->context.pMutex = &pWorker->context.mutex;

    pWorker->context.bStop = FALSE;
    pWorker->context.pWorkQueue = pWorkQueue;

    ntStatus = pthread_create(
                    &pWorker->worker,
                    NULL,
                    &SrvWorkerMain,
                    &pWorker->context);
    BAIL_ON_NT_STATUS(ntStatus);

    pWorker->pWorker = &pWorker->worker;

error:

    return ntStatus;
}

static
PVOID
SrvWorkerMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_WORKER_CONTEXT pContext = (PSMB_SRV_WORKER_CONTEXT)pData;
    PLWIO_SRV_TASK pTask = NULL;

    while (!SrvWorkerMustStop(pContext))
    {
        struct timespec ts = {0, 0};

        ts.tv_sec = time(NULL) + 30;

        if (pTask)
        {
            SrvTaskFree(pTask);
            pTask = NULL;
        }

        ntStatus = SrvProdConsTimedDequeue(
                        pContext->pWorkQueue,
                        &ts,
                        &pTask);
        if (ntStatus == STATUS_IO_TIMEOUT)
        {
            ntStatus = 0;
        }
        BAIL_ON_NT_STATUS(ntStatus);

        if (SrvWorkerMustStop(pContext))
        {
            break;
        }

        if (pTask)
        {
            ntStatus = SrvWorkerExecute(pTask);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

cleanup:

    if (pTask)
    {
        SrvTaskFree(pTask);
    }

    return NULL;

error:

    goto cleanup;
}

NTSTATUS
SrvWorkerFreeContents(
    PSMB_SRV_WORKER pWorker
    )
{
    NTSTATUS ntStatus = 0;

    if (pWorker->pWorker)
    {
        SrvWorkerStop();

        pthread_join(pWorker->pWorker);
    }

    if (pWorker->context.pMutex)
    {
        pthread_mutex_destroy(pWorker->context.pMutex);
        pWorker->context.pMutex = NULL;
    }

    // Don't free items in the work queue which will be
    // deleted by the owner of the queue

    return ntStatus;
}

static
BOOLEAN
SrvWorkerMustStop(
    PSMB_SRV_WORKER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pContext->mutex);

    bStop = pContext->bStop;

    pthread_mutex_unlock(&pContext->mutex);

    return bStop;
}

static
NTSTATUS
SrvWorkerStop(
    PSMB_SRV_WORKER_CONTEXT pContext
    )
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_mutex_unlock(&pContext->mutex);
}

static
NTSTATUS
SrvWorkerExecute(
    PSMB_SRV_TASK pTask
    )
{
    NTSTATUS ntStatus = 0;



    return ntStatus;
}
