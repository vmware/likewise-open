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
 *        context.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Execution Context
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 *
 */

#include "includes.h"

static
VOID
SrvFreeExecContext(
   IN PSRV_EXEC_CONTEXT pContext
   );

NTSTATUS
SrvBuildExecContext(
   IN  PLWIO_SRV_CONNECTION pConnection,
   IN  PSMB_PACKET          pSmbRequest,
   IN  BOOLEAN              bInternal,
   OUT PSRV_EXEC_CONTEXT*   ppContext
   )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pContext = NULL;

    ntStatus = SrvBuildEmptyExecContext(&pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvLogContextUpdateFilter(
                    pContext->pLogContext,
                    &pConnection->clientAddress,
                    pConnection->clientAddrLen);
    BAIL_ON_NT_STATUS(ntStatus);

    pContext->pConnection = SrvConnectionAcquire(pConnection);

    pContext->pSmbRequest = pSmbRequest;
    InterlockedIncrement(&pSmbRequest->refCount);

    pContext->bInternal = bInternal;

    *ppContext = pContext;

cleanup:

    return ntStatus;

error:

    *ppContext = NULL;

    goto cleanup;
}

NTSTATUS
SrvBuildEmptyExecContext(
   OUT PSRV_EXEC_CONTEXT* ppContext
   )
{
    NTSTATUS          ntStatus = STATUS_SUCCESS;
    PSRV_EXEC_CONTEXT pContext = NULL;

    ntStatus = SrvAllocateMemory(sizeof(SRV_EXEC_CONTEXT), (PVOID*)&pContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pContext->execMutex, NULL);
    pContext->pExecMutex = &pContext->execMutex;

    ntStatus = SrvLogContextCreate(&pContext->pLogContext);
    BAIL_ON_NT_STATUS(ntStatus);

    pContext->refCount = 1;

    pContext->bInternal = TRUE;

    *ppContext = pContext;

cleanup:

    return ntStatus;

error:

    *ppContext = NULL;

    if (pContext)
    {
        SrvReleaseExecContext(pContext);
    }

    goto cleanup;
}

BOOLEAN
SrvIsValidExecContext(
   IN PSRV_EXEC_CONTEXT pExecContext
   )
{
    return (pExecContext &&
            pExecContext->pConnection &&
            pExecContext->pSmbRequest);
}

VOID
SrvReleaseExecContextHandle(
   IN HANDLE hExecContext
   )
{
    SrvReleaseExecContext((PSRV_EXEC_CONTEXT)hExecContext);
}

PSRV_EXEC_CONTEXT
SrvAcquireExecContext(
   PSRV_EXEC_CONTEXT pContext
   )
{
    InterlockedIncrement(&pContext->refCount);
    return pContext;
}

VOID
SrvReleaseExecContext(
   IN PSRV_EXEC_CONTEXT pContext
   )
{
    if (InterlockedDecrement(&pContext->refCount) == 0)
    {
        SrvFreeExecContext(pContext);
    }
}

static
VOID
SrvFreeExecContext(
   IN PSRV_EXEC_CONTEXT pContext
   )
{
    if (pContext->pProtocolContext)
    {
        pContext->pfnFreeContext(pContext->pProtocolContext);
    }

    if (pContext->pSmbRequest)
    {
        SMBPacketRelease(
            pContext->pConnection->hPacketAllocator,
            pContext->pSmbRequest);
    }

    if (pContext->pSmbResponse)
    {
        SMBPacketRelease(
            pContext->pConnection->hPacketAllocator,
            pContext->pSmbResponse);
    }

    if (pContext->pInterimResponse)
    {
        SMBPacketRelease(
            pContext->pConnection->hPacketAllocator,
            pContext->pInterimResponse);
    }

    if (pContext->pConnection)
    {
        SrvConnectionRelease(pContext->pConnection);
    }

    if (pContext->pStatInfo)
    {
        SrvStatisticsRelease(pContext->pStatInfo);
    }

    if (pContext->pLogContext)
    {
        SrvLogContextFree(pContext->pLogContext);
    }

    if (pContext->pExecMutex)
    {
        pthread_mutex_destroy(&pContext->execMutex);
    }

    SrvFreeMemory(pContext);
}
