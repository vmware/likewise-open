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
 *        srvasyncstate.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Asynchronous State
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
VOID
SrvAsyncStateFree(
    PLWIO_ASYNC_STATE pAsyncState
    );

NTSTATUS
SrvAsyncBuildUniqueId(
    PSRV_EXEC_CONTEXT pContext,
    PULONG64          pullAsyncId
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG64  ullAsyncId = 0LL;

    if (!RAND_bytes((PBYTE)&ullAsyncId, sizeof(ullAsyncId)))
    {
        uuid_t uuid;
        CHAR   szUUID[37] = "";
        UCHAR  ucDigest[EVP_MAX_MD_SIZE];
        ULONG  ulDigest = 0;

        memset(&szUUID, 0, sizeof(szUUID));

        uuid_generate(uuid);
        uuid_unparse(uuid, szUUID);

        HMAC(EVP_sha512(),
             &szUUID[0],
             sizeof(szUUID),
             pContext->pSmbRequest->pRawBuffer,
             pContext->pSmbRequest->bufferUsed,
             &ucDigest[0],
             &ulDigest);

        assert (ulDigest == sizeof(ULONG64));

        memcpy((PBYTE)&ullAsyncId, &ucDigest[0], sizeof(ullAsyncId));
    }

    *pullAsyncId = ullAsyncId;

    return ntStatus;
}

NTSTATUS
SrvAsyncStateCreate(
    ULONG64                       ullAsyncId,
    USHORT                        usCommand,
    HANDLE                        hAsyncState,
    PFN_LWIO_SRV_FREE_ASYNC_STATE pfnFreeAsyncState,
    PLWIO_ASYNC_STATE*            ppAsyncState
    )
{
    NTSTATUS          ntStatus    = STATUS_SUCCESS;
    PLWIO_ASYNC_STATE pAsyncState = NULL;

    ntStatus = SrvAllocateMemory(
                    sizeof(LWIO_ASYNC_STATE),
                    (PVOID*)&pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    pAsyncState->refcount = 1;

    pthread_rwlock_init(&pAsyncState->mutex, NULL);
    pAsyncState->pMutex = &pAsyncState->mutex;

    pAsyncState->ullAsyncId        = ullAsyncId;
    pAsyncState->usCommand         = usCommand;
    pAsyncState->hAsyncState       = hAsyncState;
    pAsyncState->pfnFreeAsyncState = pfnFreeAsyncState;

    *ppAsyncState = pAsyncState;

cleanup:

    return ntStatus;

error:

    *ppAsyncState = NULL;

    goto cleanup;
}

PLWIO_ASYNC_STATE
SrvAsyncStateAcquire(
    PLWIO_ASYNC_STATE pAsyncState
    )
{
    InterlockedIncrement(&pAsyncState->refcount);

    return pAsyncState;
}

VOID
SrvAsyncStateRelease(
    PLWIO_ASYNC_STATE pAsyncState
    )
{
    if (InterlockedDecrement(&pAsyncState->refcount) == 0)
    {
        SrvAsyncStateFree(pAsyncState);
    }
}

static
VOID
SrvAsyncStateFree(
    PLWIO_ASYNC_STATE pAsyncState
    )
{
    if (pAsyncState->hAsyncState && pAsyncState->pfnFreeAsyncState)
    {
        pAsyncState->pfnFreeAsyncState(pAsyncState->hAsyncState);
    }

    if (pAsyncState->pMutex)
    {
        pthread_rwlock_destroy(&pAsyncState->mutex);
    }

    SrvFreeMemory(pAsyncState);
}
