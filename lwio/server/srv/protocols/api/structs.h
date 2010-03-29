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
 *        structs.h
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Protocols
 *
 *        Structures
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#ifndef __STRUCTS_H__
#define __STRUCTS_H__

typedef struct _LWIO_SRV_PROTOCOL_WORKER_CONTEXT
{
    pthread_mutex_t  mutex;
    pthread_mutex_t* pMutex;

    BOOLEAN bStop;

    ULONG   workerId;

    PSMB_PROD_CONS_QUEUE pWorkQueue;

} LWIO_SRV_PROTOCOL_WORKER_CONTEXT, *PLWIO_SRV_PROTOCOL_WORKER_CONTEXT;

typedef struct _LWIO_SRV_PROTOCOL_WORKER
{
    pthread_t  worker;
    pthread_t* pWorker;

    ULONG      workerId;

    LWIO_SRV_PROTOCOL_WORKER_CONTEXT context;

} LWIO_SRV_PROTOCOL_WORKER, *PLWIO_SRV_PROTOCOL_WORKER;

typedef struct _SRV_PROTOCOL_CONFIG
{
    BOOLEAN bEnableSmb2;
    BOOLEAN bEnableSigning;
    BOOLEAN bRequireSigning;
    ULONG ulZctReadThreshold;
    ULONG ulZctWriteThreshold;
} SRV_PROTOCOL_CONFIG, *PSRV_PROTOCOL_CONFIG;

typedef struct _SRV_PROTOCOL_TRANSPORT_CONTEXT {
    // Initialized on startup
    struct _SRV_PROTOCOL_API_GLOBALS* pGlobals;
    SRV_TRANSPORT_HANDLE hTransport;
    SRV_TRANSPORT_PROTOCOL_DISPATCH Dispatch;
    SRV_CONNECTION_SOCKET_DISPATCH SocketDispatch;
    uuid_t Guid;
    // Initialized on first use
    HANDLE hGssContext;
} SRV_PROTOCOL_TRANSPORT_CONTEXT;

typedef struct _SRV_PROTOCOL_API_GLOBALS
{
    pthread_mutex_t           mutex;

    PSMB_PROD_CONS_QUEUE      pWorkQueue;
    PLWIO_PACKET_ALLOCATOR     hPacketAllocator;
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList;
    SRV_PROTOCOL_CONFIG        Config;
    SRV_PROTOCOL_TRANSPORT_CONTEXT TransportContext;

} SRV_PROTOCOL_API_GLOBALS, *PSRV_PROTOCOL_API_GLOBALS;

typedef struct _SRV_SEND_CONTEXT {
    PSRV_CONNECTION pConnection;
    BOOLEAN bIsZct;
    union {
        PSMB_PACKET pPacket;
        struct {
            PLW_ZCT_VECTOR pZct;
            PFN_SRV_PROTOCOL_SEND_COMPLETE pfnCallback;
            PVOID pCallbackContext;
        };
    };
} SRV_SEND_CONTEXT;

#endif /* __STRUCTS_H__ */
