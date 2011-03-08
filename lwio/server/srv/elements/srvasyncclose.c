/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

/*
 * Copyright (c) Likewise Software.  All rights reserved.
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
 * license@likewise.com
 */

/*
 * Copyright (C) Likewise Software. All rights reserved.
 *
 * Module Name:
 *
 *        srvasyncclose.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Async Close File Support
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "includes.h"


//
// Inlines and Static Prototypes - Tracker
//

static
inline
VOID
SrvAsyncCloseFileTrackerSignal(
    IN PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker
    )
{
    BOOLEAN bInLock = FALSE;

    // The lock here ensures that SrvAsyncCloseFileTrackerWaitPending
    // will not miss the signal.

    LWIO_LOCK_MUTEX(bInLock, pTracker->pMutex);
    pthread_cond_signal(pTracker->pCondition);
    LWIO_UNLOCK_MUTEX(bInLock, pTracker->pMutex);
}

static
VOID
SrvAsyncCloseFileTrackerAddPending(
    IN PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker
    );

static
VOID
SrvAsyncCloseFileTrackerRemovePending(
    IN PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker
    );

//
// Inlines and Static Prototypes - State
//

static
NTSTATUS
SrvAsyncCloseFileStateCreate(
    OUT PSRV_ASYNC_CLOSE_FILE_STATE* ppState,
    IN PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker
    );

static
VOID
SrvAsyncCloseFileStateCallback(
    IN PVOID pContext
    );

//
// Function Definitions - Global State
//

NTSTATUS
SrvElementsCreateAsyncCloseFileState(
    OUT PSRV_ASYNC_CLOSE_FILE_STATE* ppState
    )
{
    return SrvAsyncCloseFileStateCreate(ppState, gSrvElements.pAsyncCloseFileTracker);
}

//
// Function Definitions - Tracker
//

NTSTATUS
SrvAsyncCloseFileTrackerCreate(
    OUT PSRV_ASYNC_CLOSE_FILE_TRACKER* ppTracker
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker = NULL;

    ntStatus = SrvAllocateMemory(sizeof(*pTracker), OUT_PPVOID(&pTracker));
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwErrnoToNtStatus(pthread_mutex_init(&pTracker->Mutex, NULL));
    BAIL_ON_NT_STATUS(ntStatus);
    pTracker->pMutex = &pTracker->Mutex;

    ntStatus = LwErrnoToNtStatus(pthread_cond_init(&pTracker->Condition, NULL));
    BAIL_ON_NT_STATUS(ntStatus);
    pTracker->pCondition = &pTracker->Condition;

error:
    if (ntStatus)
    {
        SrvAsyncCloseFileTrackerFree(pTracker);
        pTracker = NULL;
    }

    *ppTracker = pTracker;

    return ntStatus;
}

VOID
SrvAsyncCloseFileTrackerFree(
    IN PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker
    )
{
    if (pTracker)
    {
        LWIO_ASSERT(!pTracker->PendingCount);

        if (pTracker->pCondition)
        {
            pthread_cond_destroy(pTracker->pCondition);
        }
        if (pTracker->pMutex)
        {
            pthread_mutex_destroy(pTracker->pMutex);
        }

        SrvFreeMemory(pTracker);
    }
}

VOID
SrvAsyncCloseFileTrackerWaitPending(
    IN PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_ASSERT(pTracker);

    LWIO_LOCK_MUTEX(bInLock, pTracker->pMutex);
    while (LwInterlockedRead(&pTracker->PendingCount))
    {
        int error = pthread_cond_wait(pTracker->pCondition, pTracker->pMutex);
        LWIO_ASSERT(!error || (error == EINTR));
    }
    LWIO_UNLOCK_MUTEX(bInLock, pTracker->pMutex);
}

//
// Static Function Definitions - Tracker
//

static
VOID
SrvAsyncCloseFileTrackerAddPending(
    IN PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker
    )
{
    LONG count = LwInterlockedIncrement(&pTracker->PendingCount);
    LWIO_ASSERT(count > 0);
}

static
VOID
SrvAsyncCloseFileTrackerRemovePending(
    IN PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker
    )
{
    LONG count = LwInterlockedDecrement(&pTracker->PendingCount);
    LWIO_ASSERT(count >= 0);
    if (count == 0)
    {
        SrvAsyncCloseFileTrackerSignal(pTracker);
    }
}

//
// Function Definitions - State
//

static
NTSTATUS
SrvAsyncCloseFileStateCreate(
    OUT PSRV_ASYNC_CLOSE_FILE_STATE* ppState,
    IN PSRV_ASYNC_CLOSE_FILE_TRACKER pTracker
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_ASYNC_CLOSE_FILE_STATE pState = NULL;

    LWIO_ASSERT(pTracker);

    ntStatus = SrvAllocateMemory(sizeof(*pState), OUT_PPVOID(&pState));
    BAIL_ON_NT_STATUS(ntStatus);

    pState->pTracker = pTracker;
    pState->AsyncControlBlock.Callback = SrvAsyncCloseFileStateCallback;
    pState->AsyncControlBlock.CallbackContext = pState;

error:
    if (ntStatus)
    {
        SrvAsyncCloseFileStateFree(pState);
        pState = NULL;
    }

    *ppState = pState;

    return ntStatus;
}

VOID
SrvAsyncCloseFileStateFree(
    IN PSRV_ASYNC_CLOSE_FILE_STATE pState
    )
{
    if (pState)
    {
        SrvFreeMemory(pState);
    }
}

VOID
SrvAsyncCloseFileStateExecute(
    IN PSRV_ASYNC_CLOSE_FILE_STATE pState,
    IN IO_FILE_HANDLE FileHandle
    )
{
    NTSTATUS ntStatus = 0;

    LWIO_ASSERT(FileHandle);

    SrvAsyncCloseFileTrackerAddPending(pState->pTracker);
    ntStatus = IoAsyncCloseFile(
                    FileHandle,
                    &pState->AsyncControlBlock,
                    &pState->IoStatusBlock);
    if (ntStatus != STATUS_PENDING)
    {
        // TODO: Log something if (ntStatus != STATUS_SUCCESS)
        SrvAsyncCloseFileTrackerRemovePending(pState->pTracker);
        SrvAsyncCloseFileStateFree(pState);
    }
}

//
// Static Function Definitions - State
//

static
VOID
SrvAsyncCloseFileStateCallback(
    IN PVOID pContext
    )
{
    PSRV_ASYNC_CLOSE_FILE_STATE pState = (PSRV_ASYNC_CLOSE_FILE_STATE) pContext;

    // TODO: Log something if (pState->IoStatusBlock.Status != STATUS_SUCCESS)

    SrvAsyncCloseFileTrackerRemovePending(pState->pTracker);
    SrvAsyncCloseFileStateFree(pState);
}
