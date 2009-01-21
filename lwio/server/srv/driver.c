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

NTSTATUS
DriverDispatch(
    IN IO_DEVICE_HANDLE hDevice,
    IN PIRP pIrp
    );

VOID
DriverShutdown(
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

    if (IO_DRIVER_ENTRY_INTERFACE_VERSION != ulInterfaceVersion)
    {
        ntStatus = STATUS_UNSUCCESSFUL;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = IoDriverInitialize(
                    hDriver,
                    NULL,
                    DriverShutdown,
                    DriverDispatch);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvInitialize();

error:

    return ntStatus;
}

NTSTATUS
DriverDispatch(
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

            ntStatus = SrvDeviceControlIO(
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

VOID
DriverShutdown(
    IN IO_DRIVER_HANDLE hDriver
    )
{
    NTSTATUS ntStatus = SrvShutdown();

    if (ntStatus)
    {
        SMB_LOG_ERROR("[srv] driver failed to stop. [code: %d]", ntStatus);
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
    CHAR     szHostname[256];

    memset(&gSMBSrvGlobals, 0, sizeof(gSMBSrvGlobals));

    pthread_mutex_init(&gSMBSrvGlobals.mutex, NULL);
    gSMBSrvGlobals.pMutex = &gSMBSrvGlobals.mutex;

    if (gethostname(szHostname, sizeof(szHostname)) != 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvShareDbInit(&gSMBSrvGlobals.shareDBContext);
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
                    &SrvTaskFree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBAllocateMemory(
                    gSMBSrvGlobals.config.ulNumReaders * sizeof(SMB_SRV_SOCKET_READER),
                    (PVOID*)&gSMBSrvGlobals.pReaderArray);
    BAIL_ON_NT_STATUS(ntStatus);

    gSMBSrvGlobals.ulNumReaders = gSMBSrvGlobals.config.ulNumReaders;

    for (; iReader < gSMBSrvGlobals.config.ulNumReaders; iReader++)
    {
        ntStatus = SrvSocketReaderInit(
                        &gSMBSrvGlobals.workQueue,
                        &gSMBSrvGlobals.pReaderArray[iReader]);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateMemory(
                    gSMBSrvGlobals.config.ulNumWorkers * sizeof(SMB_SRV_WORKER),
                    (PVOID*)&gSMBSrvGlobals.pWorkerArray);
    BAIL_ON_NT_STATUS(ntStatus);

    gSMBSrvGlobals.ulNumWorkers = gSMBSrvGlobals.config.ulNumWorkers;

    for (; iWorker < gSMBSrvGlobals.config.ulNumWorkers; iWorker++)
    {
        ntStatus = SrvWorkerInit(
                        &gSMBSrvGlobals.workQueue,
                        &gSMBSrvGlobals.pWorkerArray[iWorker]);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvListenerInit(
                    gSMBSrvGlobals.hPacketAllocator,
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

            SMBFreeMemory(gSMBSrvGlobals.pReaderArray);
        }

        if (gSMBSrvGlobals.pWorkerArray)
        {
            if (gSMBSrvGlobals.ulNumWorkers)
            {
                INT iWorker = 0;

                for (; iWorker < gSMBSrvGlobals.ulNumWorkers; iWorker++)
                {
                    ntStatus = SrvWorkerFreeContents(
                                    &gSMBSrvGlobals.pWorkerArray[iWorker]);
                    BAIL_ON_NT_STATUS(ntStatus);
                }
            }
        }

        SrvProdConsFreeContents(&gSMBSrvGlobals.workQueue);

        SrvShareDbShutdown(&gSMBSrvGlobals.shareDBContext);

        if (gSMBSrvGlobals.hPacketAllocator)
        {
            SMBPacketFreeAllocator(gSMBSrvGlobals.hPacketAllocator);
            gSMBSrvGlobals.hPacketAllocator = NULL;
        }
    }

cleanup:

    if (gSMBSrvGlobals.pMutex)
    {
        pthread_mutex_unlock(gSMBSrvGlobals.pMutex);
        gSMBSrvGlobals.pMutex = NULL;
    }

    return ntStatus;

error:

    goto cleanup;
}



