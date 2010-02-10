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
 *        prototypes.h
 *
 * Abstract:
 *
 *        Likewise I/O Subsystem
 *
 *        SRV Threadpool Transport
 *
 *        Prototypes
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

// config.c

NTSTATUS
SrvThreadpoolTransportInitConfig(
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pConfig
    );

NTSTATUS
SrvThreadpoolTransportReadConfig(
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pConfig
    );

VOID
SrvThreadpoolTransportFreeConfigContents(
    PLWIO_SRV_THREADPOOL_TRANSPORT_CONFIG pConfig
    );

// libmain.c

NTSTATUS
SrvThreadpoolTransportGetRequest(
    struct timespec*   pTimespec,
    PSRV_EXEC_CONTEXT* ppContext
    );

NTSTATUS
SrvThreadpoolTransportSendResponse(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET          pResponse
    );

// listener.c

NTSTATUS
SrvListenerInit(
    HANDLE                     hPacketAllocator,
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    PLWIO_SRV_LISTENER         pListener,
    BOOLEAN                    bEnableSecuritySignatures,
    BOOLEAN                    bRequireSecuritySignatures
    );

NTSTATUS
SrvListenerShutdown(
    PLWIO_SRV_LISTENER pListener
    );

// reader.c

VOID
SrvSocketReaderProcess(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    );

// srvconnection.c

int
SrvConnectionGetFd(
    PLWIO_SRV_CONNECTION pConnection
    );

NTSTATUS
SrvConnectionGetNextSequence(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pSmbRequest,
    PULONG              pulRequestSequence
    );

NTSTATUS
SrvConnectionReadPacket(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET* ppPacket
    );

NTSTATUS
SrvConnectionWriteMessage(
    PLWIO_SRV_CONNECTION pConnection,
    PSMB_PACKET         pPacket
    );

NTSTATUS
SrvConnectionGetNamedPipeClientAddress(
    PLWIO_SRV_CONNECTION pConnection,
    PIO_ECP_LIST        pEcpList
    );

// srvsocket.c

NTSTATUS
SrvSocketCreate(
    int fd,
    struct sockaddr_in* pClientAddr,
    PLWIO_SRV_SOCKET* ppSocket
    );

VOID
SrvSocketFree(
    HANDLE hSocket
    );

