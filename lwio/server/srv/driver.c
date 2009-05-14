/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
 *        createnp.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (SMB)
 *
 *        CreateNamedPipe API
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */

#include "includes.h"

static
NTSTATUS
SrvDriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    );

static
VOID
SrvDriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    );

static
NTSTATUS
SrvInitialize(
    VOID
    );

static
NTSTATUS
SrvShutdown(
    VOID
    );

NTSTATUS
DriverEntry(
    IN IO_DRIVER_HANDLE hDriver,
    IN ULONG ulInterfaceVersion
    )
{
    NTSTATUS ntStatus = 0;
    IO_DEVICE_HANDLE hDevice = NULL;

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != ulInterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoDriverInitialize(
                    hDriver,
                    NULL,
                    SrvDriverShutdown,
                    SrvDriverDispatch);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = IoDeviceCreate(
                    &hDevice,
                    hDriver,
                    "srv",
                    NULL
                    );
    BAIL_ON_NT_STATUS(ntStatus);


    ntStatus = SrvInitialize();

error:

    return ntStatus;
}

static
NTSTATUS
SrvDriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    )
{
    NTSTATUS ntStatus = 0;

    switch (pIrp->Type)
    {
        case IRP_TYPE_CREATE:

            ntStatus = SrvDeviceCreate(
                            hDevice,
                            pIrp);
            break;

        case IRP_TYPE_CLOSE:

            ntStatus = SrvDeviceClose(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_READ:

            ntStatus = SrvDeviceRead(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_WRITE:

            ntStatus = SrvDeviceWrite(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_DEVICE_IO_CONTROL:

            ntStatus = SrvDeviceControlIo(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FS_CONTROL:

            ntStatus = SrvDeviceControlFS(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_FLUSH_BUFFERS:

            ntStatus = SrvDeviceFlush(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_QUERY_INFORMATION:

            ntStatus = SrvDeviceQueryInfo(
                            hDevice,
                            pIrp);

            break;

        case IRP_TYPE_SET_INFORMATION:

            ntStatus = SrvDeviceSetInfo(
                            hDevice,
                            pIrp);

            break;

        default:

            ntStatus = STATUS_UNSUCCESSFUL;
            BAIL_ON_NT_STATUS(ntStatus);
    }

error:

    return ntStatus;
}

static
VOID
SrvDriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    )
{
    NTSTATUS ntStatus = SrvShutdown();

    if (ntStatus)
    {
        LWIO_LOG_ERROR("[srv] driver failed to stop. [code: %d]", ntStatus);
    }
}

static
NTSTATUS
SrvInitialize(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
    INT      iReader = 0;
    INT      iWorker = 0;

    memset(&gSMBSrvGlobals, 0, sizeof(gSMBSrvGlobals));

    pthread_mutex_init(&gSMBSrvGlobals.mutex, NULL);
    gSMBSrvGlobals.pMutex = &gSMBSrvGlobals.mutex;

    ntStatus = SrvShareInitContextContents(&gSMBSrvGlobals.shareList);
    BAIL_ON_NT_STATUS(ntStatus);

    gSMBSrvGlobals.config.ulNumReaders = LWIO_SRV_DEFAULT_NUM_READERS;
    gSMBSrvGlobals.config.ulNumWorkers = LWIO_SRV_DEFAULT_NUM_WORKERS;
    gSMBSrvGlobals.config.ulMaxNumWorkItemsInQueue = LWIO_SRV_DEFAULT_NUM_MAX_QUEUE_ITEMS;
    gSMBSrvGlobals.config.ulMaxNumPackets = LWIO_SRV_DEFAULT_NUM_MAX_PACKETS;

    ntStatus = SMBPacketCreateAllocator(
                    gSMBSrvGlobals.config.ulMaxNumPackets,
                    &gSMBSrvGlobals.hPacketAllocator);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvProdConsInitContents(
                    &gSMBSrvGlobals.workQueue,
                    gSMBSrvGlobals.config.ulMaxNumWorkItemsInQueue,
                    &SrvContextFree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LW_RTL_ALLOCATE(
                    &gSMBSrvGlobals.pReaderArray,
                    LWIO_SRV_SOCKET_READER,
                    gSMBSrvGlobals.config.ulNumReaders * sizeof(LWIO_SRV_SOCKET_READER));
    BAIL_ON_NT_STATUS(ntStatus);

    gSMBSrvGlobals.ulNumReaders = gSMBSrvGlobals.config.ulNumReaders;

    for (; iReader < gSMBSrvGlobals.config.ulNumReaders; iReader++)
    {
        PLWIO_SRV_SOCKET_READER pReader = NULL;

        pReader = &gSMBSrvGlobals.pReaderArray[iReader];

        pReader->readerId = iReader + 1;

        ntStatus = SrvSocketReaderInit(
                        &gSMBSrvGlobals.workQueue,
                        pReader);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LW_RTL_ALLOCATE(
                    &gSMBSrvGlobals.pWorkerArray,
                    LWIO_SRV_WORKER,
                    gSMBSrvGlobals.config.ulNumWorkers * sizeof(LWIO_SRV_WORKER));
    BAIL_ON_NT_STATUS(ntStatus);

    gSMBSrvGlobals.ulNumWorkers = gSMBSrvGlobals.config.ulNumWorkers;

    for (; iWorker < gSMBSrvGlobals.config.ulNumWorkers; iWorker++)
    {
        PLWIO_SRV_WORKER pWorker = &gSMBSrvGlobals.pWorkerArray[iWorker];

        pWorker->workerId = iWorker + 1;

        ntStatus = SrvWorkerInit(
                        &gSMBSrvGlobals.workQueue,
                        pWorker);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvListenerInit(
                    gSMBSrvGlobals.hPacketAllocator,
                    &gSMBSrvGlobals.shareList,
                    gSMBSrvGlobals.pReaderArray,
                    gSMBSrvGlobals.ulNumReaders,
                    &gSMBSrvGlobals.listener);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
SrvShutdown(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_CONTEXT pContext = NULL;

    if (gSMBSrvGlobals.pMutex)
    {
        pthread_mutex_lock(gSMBSrvGlobals.pMutex);

        ntStatus = SrvListenerShutdown(
                        &gSMBSrvGlobals.listener);
        BAIL_ON_NT_STATUS(ntStatus);

        if (gSMBSrvGlobals.pReaderArray)
        {
            if (gSMBSrvGlobals.ulNumReaders)
            {
                INT      iReader = 0;

                for (; iReader < gSMBSrvGlobals.ulNumReaders; iReader++)
                {
                    ntStatus = SrvSocketReaderFreeContents(
                                    &gSMBSrvGlobals.pReaderArray[iReader]);
                    BAIL_ON_NT_STATUS(ntStatus);
                }
            }

            LwRtlMemoryFree(gSMBSrvGlobals.pReaderArray);
        }

        if (gSMBSrvGlobals.pWorkerArray)
        {
            if (gSMBSrvGlobals.ulNumWorkers)
            {
                INT iWorker = 0;
                INT iItem = 0;

                for (; iWorker < gSMBSrvGlobals.ulNumWorkers; iWorker++)
                {
                    PLWIO_SRV_WORKER pWorker = &gSMBSrvGlobals.pWorkerArray[iWorker];

                    SrvWorkerIndicateStop(pWorker);
                }

                // Interrupt the workers by inserting dummy items in the work queue
                for (; iItem < gSMBSrvGlobals.ulNumWorkers * 2; iItem++)
                {
                    ntStatus = LW_RTL_ALLOCATE(
                                    &pContext,
                                    LWIO_SRV_CONTEXT,
                                    sizeof(LWIO_SRV_CONTEXT));
                    BAIL_ON_NT_STATUS(ntStatus);

                    ntStatus = SrvProdConsEnqueue(&gSMBSrvGlobals.workQueue, pContext);
                    BAIL_ON_NT_STATUS(ntStatus);

                    pContext = NULL;
                }

                for (iWorker = 0; iWorker < gSMBSrvGlobals.ulNumWorkers; iWorker++)
                {
                    PLWIO_SRV_WORKER pWorker = &gSMBSrvGlobals.pWorkerArray[iWorker];

                    SrvWorkerFreeContents(pWorker);
                }
            }
        }

        SrvProdConsFreeContents(&gSMBSrvGlobals.workQueue);

        SrvShareFreeContextContents(&gSMBSrvGlobals.shareList);

        if (gSMBSrvGlobals.hPacketAllocator)
        {
            SMBPacketFreeAllocator(gSMBSrvGlobals.hPacketAllocator);
            gSMBSrvGlobals.hPacketAllocator = NULL;
        }

        while (gSMBSrvGlobals.pCCBList)
        {
            PSRV_CCB pCCB = gSMBSrvGlobals.pCCBList;

            gSMBSrvGlobals.pCCBList = pCCB->pNext;

            SrvCCBRelease(pCCB);
        }
    }

cleanup:

    if (gSMBSrvGlobals.pMutex)
    {
        pthread_mutex_unlock(gSMBSrvGlobals.pMutex);
        gSMBSrvGlobals.pMutex = NULL;
    }

    if (pContext)
    {
        SrvContextFree(pContext);
    }

    return ntStatus;

error:

    goto cleanup;
}



