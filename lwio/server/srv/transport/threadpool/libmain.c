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
 *        libmain.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Transport API
 *
 *        Library Main
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

NTSTATUS
SrvThreadpoolTransportInit(
    PLWIO_PACKET_ALLOCATOR         hPacketAllocator,
    PLWIO_SRV_SHARE_ENTRY_LIST     pShareList,
    PSMB_PROD_CONS_QUEUE           pWorkQueue,
    PSRV_TRANSPORT_FUNCTION_TABLE* ppFnTable
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SrvThreadpoolTransportInitConfig(&gSrvThreadpoolTransport.config);
    BAIL_ON_NT_STATUS(status);

    status = SrvThreadpoolTransportReadConfig(&gSrvThreadpoolTransport.config);
    BAIL_ON_NT_STATUS(status);

    gSrvThreadpoolTransport.hPacketAllocator = hPacketAllocator;
    gSrvThreadpoolTransport.pShareList = pShareList;
    gSrvThreadpoolTransport.pWorkQueue = pWorkQueue;

    status = SrvListenerInit(
                    gSrvThreadpoolTransport.hPacketAllocator,
                    gSrvThreadpoolTransport.pShareList,
                    &gSrvThreadpoolTransport.listener,
                    gSrvThreadpoolTransport.config.bEnableSigning,
                    gSrvThreadpoolTransport.config.bRequireSigning);
    BAIL_ON_NT_STATUS(status);

    *ppFnTable = &gSrvThreadpoolTransport.fnTable;

cleanup:

    return status;

error:

    *ppFnTable = NULL;

    goto cleanup;
}

NTSTATUS
SrvThreadpoolTransportGetRequest(
    IN  struct timespec*   pTimespec,
    OUT PSRV_EXEC_CONTEXT* ppContext
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
SrvThreadpoolTransportSendResponse(
    IN          PLWIO_SRV_CONNECTION pConnection,
    IN          PSMB_PACKET          pResponse
    )
{
    return SrvConnectionWriteMessage(pConnection, pResponse);
}

NTSTATUS
SrvThreadpoolTransportShutdown(
    PSRV_TRANSPORT_FUNCTION_TABLE pFnTable
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SrvListenerShutdown(&gSrvThreadpoolTransport.listener);
    BAIL_ON_NT_STATUS(status);

    gSrvThreadpoolTransport.pWorkQueue = NULL;

    SrvThreadpoolTransportFreeConfigContents(&gSrvThreadpoolTransport.config);

error:

    return status;
}

