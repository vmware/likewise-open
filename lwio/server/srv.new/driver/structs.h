/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 */

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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _LWIO_SRV_CONFIG
{
    ULONG ulNumReaders;
    ULONG ulNumWorkers;
    ULONG ulMaxNumWorkItemsInQueue;
    ULONG ulMaxNumPackets;

} LWIO_SRV_CONFIG, *PLWIO_SRV_CONFIG;

typedef struct _SRV_CCB
{
    LONG                    refCount;

    CCB_TYPE                CcbType;
    UNICODE_STRING          AbsolutePathName;
    ACCESS_MASK             DesiredAccess;
    LONG64                  AllocationSize;
    FILE_ATTRIBUTES         FileAttributes;
    FILE_SHARE_FLAGS        ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS     CreateOptions;

    struct _SRV_CCB *       pNext;

} SRV_CCB, *PSRV_CCB;

typedef struct _SRV_IRP_CONTEXT
{
    PIRP             pIrp;
    IO_DEVICE_HANDLE targetDeviceHandle;
    UNICODE_STRING   rootPathName;
    UNICODE_STRING   relativePathName;
    UNICODE_STRING   absolutePathName;

} SRV_IRP_CONTEXT, *PSRV_IRP_CONTEXT;

typedef struct _LWIO_SRV_WORKER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    BOOLEAN bStop;

    ULONG   workerId;

    // Invariant
    // not owned
    PSMB_PROD_CONS_QUEUE pWorkQueue;

} LWIO_SRV_WORKER_CONTEXT, *PLWIO_SRV_WORKER_CONTEXT;

typedef struct _LWIO_SRV_WORKER
{
    pthread_t  worker;
    pthread_t* pWorker;

    ULONG      workerId;

    LWIO_SRV_WORKER_CONTEXT context;

} LWIO_SRV_WORKER, *PLWIO_SRV_WORKER;

typedef struct _LWIO_SRV_RUNTIME_GLOBALS
{
    pthread_mutex_t          mutex;
    pthread_mutex_t*         pMutex;

    LWIO_SRV_CONFIG          config;

    LWIO_SRV_SHARE_ENTRY_LIST shareList;

#if 0
    SMB_PROD_CONS_QUEUE      workQueue;

    PLWIO_SRV_SOCKET_READER  pReaderArray;
    ULONG                    ulNumReaders;

    PLWIO_SRV_WORKER         pWorkerArray;
    ULONG                    ulNumWorkers;

    LWIO_SRV_LISTENER        listener;

    PLWIO_PACKET_ALLOCATOR   hPacketAllocator;
#endif

    PSRV_CCB                 pCCBList;

} LWIO_SRV_RUNTIME_GLOBALS, *PLWIO_SRV_RUNTIME_GLOBALS;


#endif /* __STRUCTS_H__ */
