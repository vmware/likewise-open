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
 *        srvstructs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO)
 *
 *        Server structures
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _SMB_SRV_CONFIG
{
    ULONG ulNumReaders;
    ULONG ulNumWorkers;
    ULONG ulMaxNumWorkItemsInQueue;
    ULONG ulMaxNumPackets;

} SMB_SRV_CONFIG, *PSMB_SRV_CONFIG;

typedef struct _SHARE_INFO
{
    PSTR pszName;
    PSTR pszPath;
    PSTR pszComment;
    PSTR pszSID;
} SHARE_INFO, *PSHARE_INFO;

typedef struct _SMB_SRV_SHARE_DB_CONTEXT
{
    pthread_rwlock_t  dbLock;
    pthread_rwlock_t* pDbLock;

} SMB_SRV_SHARE_DB_CONTEXT, *PSMB_SRV_SHARE_DB_CONTEXT;

typedef struct _SRV_CCB
{
    //LIST_ENTRY NextCCB;
    UNICODE_STRING AbsolutePathName;
    ACCESS_MASK DesiredAccess;
    LONG64 AllocationSize;
    FILE_ATTRIBUTES FileAttributes;
    FILE_SHARE_FLAGS ShareAccess;
    FILE_CREATE_DISPOSITION CreateDisposition;
    FILE_CREATE_OPTIONS CreateOptions;
    PIO_EA_BUFFER pEaBuffer;

    int fd;
    char *path;
    int oflags;
    mode_t mode;

} SRV_CCB, *PSRV_CCB;

typedef struct _SRV_IRP_CONTEXT
{
    PIRP             pIrp;
    IO_DEVICE_HANDLE targetDeviceHandle;
    UNICODE_STRING   rootPathName;
    UNICODE_STRING   relativePathName;
    UNICODE_STRING   absolutePathName;

} SRV_IRP_CONTEXT, *PSRV_IRP_CONTEXT;


typedef VOID (*PFN_PROD_CONS_QUEUE_FREE_ITEM)(PVOID pItem);

typedef struct _SMB_PROD_CONS_QUEUE
{
    pthread_mutex_t mutex;

    SMB_QUEUE       queue;

    ULONG           ulNumMaxItems;
    ULONG           ulNumItems;

    PFN_PROD_CONS_QUEUE_FREE_ITEM pfnFreeItem;

    pthread_cond_t  event;
    pthread_cond_t* pEvent;

} SMB_PROD_CONS_QUEUE, *PSMB_PROD_CONS_QUEUE;

typedef struct _SMB_SRV_SOCKET
{
    pthread_mutex_t mutex;

    int fd;

    struct sockaddr_in cliaddr;

} SMB_SRV_SOCKET, *PSMB_SRV_SOCKET;

typedef struct _SMB_SRV_CONNECTION
{
    pthread_mutex_t     mutex;

    LONG                refCount;

    SMB_SRV_CONN_STATE  state;

    PSMB_SRV_SOCKET     pSocket;

    // Invariant
    // Not owned
    HANDLE              hPacketAllocator;

} SMB_SRV_CONNECTION, *PSMB_SRV_CONNECTION;

typedef struct _LWIO_SRV_TASK
{
    PSMB_SRV_CONNECTION pConnection;

    PSMB_PACKET         pRequest;

} LWIO_SRV_TASK, *PLWIO_SRV_TASK;

typedef struct _SMB_SRV_SOCKET_READER_CONTEXT
{
    pthread_mutex_t mutex;

    BOOLEAN        bStop;

    PSMB_RB_TREE   pConnections;
    ULONG          ulNumSockets;

    PSMB_PROD_CONS_QUEUE pWorkQueue;

    // pipe used to interrupt the reader
    int fd[2];

} SMB_SRV_SOCKET_READER_CONTEXT, *PSMB_SRV_SOCKET_READER_CONTEXT;

typedef struct _SMB_SRV_SOCKET_READER
{
    pthread_t  reader;
    pthread_t* pReader;

    SMB_SRV_SOCKET_READER_CONTEXT context;

} SMB_SRV_SOCKET_READER, *PSMB_SRV_SOCKET_READER;

typedef struct _SMB_SRV_WORKER_CONTEXT
{
    pthread_mutex_t mutex;

    BOOLEAN bStop;

} SMB_SRV_WORKER_CONTEXT, *PSMB_SRV_WORKER_CONTEXT;

typedef struct _SMB_SRV_WORKER
{
    pthread_t  worker;
    pthread_t* pWorker;

    SMB_SRV_WORKER_CONTEXT context;

} SMB_SRV_WORKER, *PSMB_SRV_WORKER;

typedef struct _SMB_SRV_LISTENER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    BOOLEAN bStop;

    // Invariant
    // Not owned
    HANDLE  hPacketAllocator;

    // Invariant
    // Not owned
    PSMB_SRV_SOCKET_READER pReaderArray;
    ULONG                  ulNumReaders;

} SMB_SRV_LISTENER_CONTEXT, *PSMB_SRV_LISTENER_CONTEXT;

typedef struct _SMB_SRV_LISTENER
{
    pthread_t  listener;
    pthread_t* pListener;

    SMB_SRV_LISTENER_CONTEXT context;

} SMB_SRV_LISTENER, *PSMB_SRV_LISTENER;

typedef struct _SMB_SRV_RUNTIME_GLOBALS
{
    pthread_mutex_t          mutex;
    pthread_mutex_t*         pMutex;

    SMB_SRV_CONFIG           config;

    SMB_SRV_SHARE_DB_CONTEXT shareDBContext;

    SMB_PROD_CONS_QUEUE      workQueue;

    PSMB_SRV_SOCKET_READER   pReaderArray;
    ULONG                    ulNumReaders;

    PSMB_SRV_WORKER          pWorkerArray;
    ULONG                    ulNumWorkers;

    SMB_SRV_LISTENER         listener;

    HANDLE                   hPacketAllocator;

} SMB_SRV_RUNTIME_GLOBALS, *PSMB_SRV_RUNTIME_GLOBALS;

#endif /* __STRUCTS_H__ */
