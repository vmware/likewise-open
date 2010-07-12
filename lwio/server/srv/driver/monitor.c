 /* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * */

/*
 * Copyright Likewise Software
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.  You should have received a copy of the GNU General
 * Public License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * LIKEWISE SOFTWARE MAKES THIS SOFTWARE AVAILABLE UNDER OTHER LICENSING
 * TERMS AS WELL.  IF YOU HAVE ENTERED INTO A SEPARATE LICENSE AGREEMENT
 * WITH LIKEWISE SOFTWARE, THEN YOU MAY ELECT TO USE THE SOFTWARE UNDER THE
 * TERMS OF THAT SOFTWARE LICENSE AGREEMENT INSTEAD OF THE TERMS OF THE GNU
 * GENERAL PUBLIC LICENSE, NOTWITHSTANDING THE ABOVE NOTICE.  IF YOU
 * HAVE QUESTIONS, OR WISH TO REQUEST A COPY OF THE ALTERNATE LICENSING
 * TERMS OFFERED BY LIKEWISE SOFTWARE, PLEASE CONTACT LIKEWISE SOFTWARE AT
 * license@likewisesoftware.com
 */



/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        monitor.c
 *
 * Abstract:
 *
 *        Likewise File System Driver (Srv)
 *
 *        DeviceIo Dispatch Routine
 *
 *        Monitor
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
PVOID
SrvMonitorExecute(
    PVOID pContext
    );

static
VOID
SrvLogResourceStats(
    VOID
    );

static
VOID
SrvMonitorIndicateStopContext(
    PLWIO_SRV_MONITOR_CONTEXT pContext
    );

NTSTATUS
SrvMonitorCreate(
    ULONG              ulMonitorInterval,
    PLWIO_SRV_MONITOR* ppMonitor
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PLWIO_SRV_MONITOR pMonitor = NULL;

    if (!ulMonitorInterval || (ulMonitorInterval == UINT32_MAX))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(sizeof(LWIO_SRV_MONITOR), (PVOID*)&pMonitor);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pMonitor->context.mutex, NULL);
    pMonitor->context.pMutex = &pMonitor->context.mutex;

    pthread_cond_init(&pMonitor->context.cond, NULL);
    pMonitor->context.pCond = &pMonitor->context.cond;

    pMonitor->context.ulMonitorInterval = ulMonitorInterval;

    *ppMonitor = pMonitor;

cleanup:

    return ntStatus;

error:

    *ppMonitor = NULL;

    if (pMonitor)
    {
        SrvMonitorFree(pMonitor);
    }

    goto cleanup;
}

NTSTATUS
SrvMonitorStart(
    PLWIO_SRV_MONITOR pMonitor
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = pthread_create(
                        &pMonitor->worker,
                        NULL,
                        &SrvMonitorExecute,
                        &pMonitor->context);
    BAIL_ON_NT_STATUS(ntStatus);

    pMonitor->pWorker = &pMonitor->worker;

error:

    return ntStatus;
}

static
PVOID
SrvMonitorExecute(
    PVOID pData
    )
{
    NTSTATUS status = 0;
    PLWIO_SRV_MONITOR_CONTEXT pContext = (PLWIO_SRV_MONITOR_CONTEXT)pData;
    BOOLEAN bInLock = FALSE;

    LWIO_LOG_DEBUG("Srv Monitor starting");

    LWIO_LOCK_MUTEX(bInLock, &pContext->mutex);

    while (!pContext->bStop)
    {
        BOOLEAN bRetryWait = FALSE;
        struct timespec tsLong =
        {
            .tv_sec = time(NULL) + pContext->ulMonitorInterval * SMB_SECONDS_IN_MINUTE,
            .tv_nsec = 0
        };

        SrvLogResourceStats();

        do
        {
            int errCode = 0;

            bRetryWait = FALSE;

            errCode = pthread_cond_timedwait(
                            &pContext->cond,
                            &pContext->mutex,
                            &tsLong);

            if (errCode == ETIMEDOUT)
            {
                if (time(NULL) < tsLong.tv_sec)
                {
                    bRetryWait = TRUE;
                    continue;
                }

                break;
            }

            status = LwErrnoToNtStatus(errCode);
            BAIL_ON_NT_STATUS(status);

        } while (bRetryWait && !pContext->bStop);
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pContext->mutex);

    LWIO_LOG_DEBUG("Srv Monitor stopping");

    return NULL;

error:

    LWIO_LOG_ERROR("Srv Monitor stopping due to error [%d]", status);

    goto cleanup;
}

static
VOID
SrvLogResourceStats(
    VOID
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    IO_STATISTICS_INFO_INPUT_BUFFER ioStatBuf =
    {
            .ulAction    = IO_STATISTICS_ACTION_TYPE_GET,
            .ulInfoLevel = 0
    };
    IO_STATISTICS_INFO_0 ioStatInfo0 = {0};
    ULONG ulBytesTransferred = 0;

#if 0
    struct rusage resUsage;

    memset(&resUsage, 0, sizeof(resUsage));

    // TODO: This returns 0 for memory values on linux
    if (getrusage(RUSAGE_SELF, &resUsage) < 0)
    {
        status = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(status);
    }

    LWIO_LOG_ALWAYS(
        "Memory:shared-mem-size(%ld),unshared-data-size(%ld),unshared-stack-size(%ld)",
        resUsage.ru_ixrss,
        resUsage.ru_idrss,
        resUsage.ru_isrss);
#endif

    status = SrvProcessStatistics(
                        (PBYTE)&ioStatBuf,
                        sizeof(ioStatBuf),
                        (PBYTE)&ioStatInfo0,
                        sizeof(ioStatInfo0),
                        &ulBytesTransferred);
    BAIL_ON_NT_STATUS(status);

    LWIO_LOG_ALWAYS(
            "Statistics (srv): connections(%lld),sessions(%lld),"
            "trees(%lld),files(%lld)",
            (long long)ioStatInfo0.llNumConnections,
            (long long)ioStatInfo0.llNumSessions,
            (long long)ioStatInfo0.llNumTreeConnects,
            (long long)ioStatInfo0.llNumOpenFiles);

cleanup:

    return;

error:

    LWIO_LOG_ERROR( "Failed to retrieve resource statistics. [status:0x%x]",
                    status);

    goto cleanup;
}

VOID
SrvMonitorIndicateStop(
    PLWIO_SRV_MONITOR pMonitor
    )
{
    if (pMonitor->pWorker)
    {
        SrvMonitorIndicateStopContext(&pMonitor->context);
    }
}

static
VOID
SrvMonitorIndicateStopContext(
    PLWIO_SRV_MONITOR_CONTEXT pContext
    )
{
    pthread_mutex_lock(&pContext->mutex);

    pContext->bStop = TRUE;

    pthread_cond_signal(&pContext->cond);

    pthread_mutex_unlock(&pContext->mutex);
}

VOID
SrvMonitorFree(
    PLWIO_SRV_MONITOR pMonitor
    )
{
    if (pMonitor->pWorker)
    {
        // Someone must have already called SrvMonitorIndicateStop
        // and stopped the thread

        pthread_join(pMonitor->worker, NULL);

        pMonitor->pWorker = NULL;
    }

    if (pMonitor->context.pCond)
    {
        pthread_cond_destroy(&pMonitor->context.cond);
        pMonitor->context.pCond = NULL;
    }

    if (pMonitor->context.pMutex)
    {
        pthread_mutex_destroy(&pMonitor->context.mutex);
        pMonitor->context.pMutex = NULL;
    }

    SrvFreeMemory(pMonitor);
}
