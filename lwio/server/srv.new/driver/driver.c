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
 *        driver.c
 *
 * Abstract:
 *
 *        Likewise I/O (LWIO) - SRV
 *
 *        Driver
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
SrvShareBootstrap(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
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
    PCSTR    pszName = "srv";
    PVOID    pDeviceContext = NULL;

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
                    pszName,
                    pDeviceContext);
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
#if 0
    INT      iReader = 0;
    INT      iWorker = 0;
#endif

    memset(&gSMBSrvGlobals, 0, sizeof(gSMBSrvGlobals));

    pthread_mutex_init(&gSMBSrvGlobals.mutex, NULL);
    gSMBSrvGlobals.pMutex = &gSMBSrvGlobals.mutex;

    ntStatus = SrvShareInit();
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareInitList(&gSMBSrvGlobals.shareList);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareBootstrap(&gSMBSrvGlobals.shareList);
    BAIL_ON_NT_STATUS(ntStatus);

#if 0
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

    ntStatus = SrvAllocateMemory(
					gSMBSrvGlobals.config.ulNumReaders * sizeof(LWIO_SRV_SOCKET_READER),
                    (PVOID*)&gSMBSrvGlobals.pReaderArray);
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

    ntStatus = SrvAllocateMemory(
					gSMBSrvGlobals.config.ulNumWorkers * sizeof(LWIO_SRV_WORKER),
                    (PVOID*)&gSMBSrvGlobals.pWorkerArray);
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
#endif

error:

    return ntStatus;
}

static
NTSTATUS
SrvShareBootstrap(
    IN OUT PLWIO_SRV_SHARE_ENTRY_LIST pShareList
    )
{
	NTSTATUS ntStatus = STATUS_SUCCESS;
	wchar16_t wszPipeRootName[] = {'I','P','C','$',0};
	wchar16_t wszFileRootName[] = {'C','$',0};
    PSTR  pszFileSystemRoot = NULL;
    PWSTR pwszFileSystemRoot = NULL;
    PSRV_SHARE_INFO pShareInfo = NULL;

    ntStatus = SrvShareFindByName(
					pShareList,
					&wszPipeRootName[0],
					&pShareInfo);
    if (ntStatus == STATUS_NOT_FOUND)
    {
	wchar16_t wszPipeSystemRoot[] = LWIO_SRV_PIPE_SYSTEM_ROOT_W;
	wchar16_t wszServiceType[] = LWIO_SRV_SHARE_STRING_ID_IPC_W;
	wchar16_t wszDesc[] = {'R','e','m','o','t','e',' ','I','P','C',0};

	ntStatus = SrvShareAdd(
							pShareList,
	                    &wszPipeRootName[0],
	                    &wszPipeSystemRoot[0],
	                    &wszDesc[0],
	                    NULL,
	                    0,
	                    &wszServiceType[0]);
    }
    else
    {
	SrvShareReleaseInfo(pShareInfo);
	pShareInfo = NULL;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvShareFindByName(
						pShareList,
						&wszFileRootName[0],
						&pShareInfo);
    if (ntStatus == STATUS_NOT_FOUND)
    {
	wchar16_t wszDesc[] =
						{'D','e','f','a','u','l','t',' ','S','h','a','r','e',0};
	wchar16_t wszServiceType[] = LWIO_SRV_SHARE_STRING_ID_DISK_W;
	CHAR szTmpFSRoot[] = LWIO_SRV_FILE_SYSTEM_ROOT_A;
	CHAR szDefaultSharePath[] = LWIO_SRV_DEFAULT_SHARE_PATH_A;

	ntStatus = SrvAllocateStringPrintf(
	                    &pszFileSystemRoot,
	                    "%s%s%s",
	                    &szTmpFSRoot[0],
	                    (((szTmpFSRoot[strlen(&szTmpFSRoot[0])-1] == '/') ||
	                      (szTmpFSRoot[strlen(&szTmpFSRoot[0])-1] == '\\')) ? "" : "\\"),
	                    &szDefaultSharePath[0]);
	BAIL_ON_NT_STATUS(ntStatus);

	ntStatus = SrvMbsToWc16s(pszFileSystemRoot, &pwszFileSystemRoot);
	BAIL_ON_NT_STATUS(ntStatus);

		ntStatus = SrvShareAdd(
						pShareList,
						&wszFileRootName[0],
						pwszFileSystemRoot,
						&wszDesc[0],
						NULL,
						0,
						&wszServiceType[0]);
    }
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SRV_SAFE_FREE_MEMORY(pszFileSystemRoot);
    SRV_SAFE_FREE_MEMORY(pwszFileSystemRoot);

	return ntStatus;

error:

	LWIO_LOG_ERROR("Failed to bootstrap default shares. [error code: %d]",
			       ntStatus);

	goto cleanup;
}

static
NTSTATUS
SrvShutdown(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
#if 0
    PLWIO_SRV_CONTEXT pContext = NULL;
#endif

    if (gSMBSrvGlobals.pMutex)
    {
        pthread_mutex_lock(gSMBSrvGlobals.pMutex);

#if 0
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

            SrvFreeMemory(gSMBSrvGlobals.pReaderArray);
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
                    ntStatus = SrvAllocateMemory(
									sizeof(LWIO_SRV_CONTEXT),
                                    (PVOID*)&pContext);
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
#endif

        SrvShareFreeListContents(&gSMBSrvGlobals.shareList);

        SrvShareShutdown();

#if 0
        if (gSMBSrvGlobals.hPacketAllocator)
        {
            SMBPacketFreeAllocator(gSMBSrvGlobals.hPacketAllocator);
            gSMBSrvGlobals.hPacketAllocator = NULL;
        }
#endif

        while (gSMBSrvGlobals.pCCBList)
        {
            PSRV_CCB pCCB = gSMBSrvGlobals.pCCBList;

            gSMBSrvGlobals.pCCBList = pCCB->pNext;

            SrvCCBRelease(pCCB);
        }
    }

// cleanup:

    if (gSMBSrvGlobals.pMutex)
    {
        pthread_mutex_unlock(gSMBSrvGlobals.pMutex);
        gSMBSrvGlobals.pMutex = NULL;
    }

#if 0
    if (pContext)
    {
        SrvContextFree(pContext);
    }
#endif

    return ntStatus;

#if 0
error:

    goto cleanup;
#endif
}



