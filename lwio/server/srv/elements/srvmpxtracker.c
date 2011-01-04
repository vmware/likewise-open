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
 *        srvmpxtracker.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        MPX (PID/MID) Tracker (for SMB1)
 *
 * Authors: Danilo Almeida (dalmeida@likewise.com)
 *
 */

#include "includes.h"


//
// Inlines and Static Prototypes
//

static
NTSTATUS
SrvMpxTrackerAdd(
    PSRV_MPX_TRACKER pTracker,
    ULONG ulPid,
    USHORT usMid,
    USHORT usUid,
    USHORT usTid,
    PSRV_MPX_TRACKING_STATE pState
    );

static
PSRV_MPX_TRACKING_STATE
SrvMpxTrackerFind_inlock(
    PSRV_MPX_TRACKER pTracker,
    ULONG ulPid,
    USHORT usMid
    );

//
// Function Definitions
//

BOOLEAN
SrvMpxTrackerIsNtCancelPacket(
    PSMB_PACKET pPacket
    )
{
    return ((pPacket->protocolVer == SMB_PROTOCOL_VERSION_1) &&
            (pPacket->pSMBHeader->command == COM_NT_CANCEL));
}

NTSTATUS
SrvMpxTrackerAddExecContext_inlock(
    PLWIO_SRV_CONNECTION pConnection,
    PSRV_EXEC_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_PACKET pPacket = pContext->pSmbRequest;

    if (pPacket->protocolVer == SMB_PROTOCOL_VERSION_1)
    {
        PSMB_HEADER pHeader = pPacket->pSMBHeader;

        // It is a bug for the caller to try to track a cancel.
        LWIO_ASSERT(!SrvMpxTrackerIsNtCancelPacket(pPacket));

        // 0xFFFF is a special MID used for oplock breaks.

        if (0xFFFF != pHeader->mid)
        {
            ntStatus = SrvConnectionEnsureMpxTracker_inlock(pConnection);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SrvMpxTrackerAdd(
                            pConnection->pMpxTracker,
                            SMB_V1_GET_PROCESS_ID(pHeader),
                            pHeader->mid,
                            pHeader->uid,
                            pHeader->tid,
                            &pContext->trackingState);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

VOID
SrvMpxTrackerSwapExecContext(
    PSRV_EXEC_CONTEXT pContext,
    PSRV_EXEC_CONTEXT pNewContext
    )
{
    BOOLEAN bInLock = FALSE;
    PSRV_MPX_TRACKER pTracker = pContext->pConnection->pMpxTracker;
    PSRV_MPX_TRACKING_STATE pState = &pContext->trackingState;
    PSRV_MPX_TRACKING_STATE pNewState = &pNewContext->trackingState;

    LWIO_ASSERT(pContext->pConnection == pNewContext->pConnection);

    LWIO_LOCK_MUTEX(bInLock, &pTracker->mutex);

    LWIO_ASSERT(IsSetFlag(pState->flags, SRV_MPX_TRACKING_FLAG_IN_TRACKER));
    LWIO_ASSERT(!IsSetFlag(pNewState->flags, SRV_MPX_TRACKING_FLAG_IN_TRACKER));

    pNewState->flags = pState->flags;
    pNewState->ulPid = pState->ulPid;
    pNewState->usMid = pState->usMid;
    pNewState->usUid = pState->usUid;
    pNewState->usTid = pState->usTid;

    LwListRemove(&pState->mpxTrackerListEntry);
    pState->flags = 0;

    LwListInsertHead(&pTracker->list, &pNewState->mpxTrackerListEntry);

    LWIO_UNLOCK_MUTEX(bInLock, &pTracker->mutex);
}

VOID
SrvMpxTrackerRemoveExecContext(
    PSRV_EXEC_CONTEXT pContext
    )
{
    if (pContext->pConnection && pContext->pConnection->pMpxTracker)
    {
        PSRV_MPX_TRACKER pTracker = pContext->pConnection->pMpxTracker;
        PSRV_MPX_TRACKING_STATE pState = &pContext->trackingState;
        BOOLEAN bInLock = FALSE;

        LWIO_LOCK_MUTEX(bInLock, &pTracker->mutex);

        LWIO_ASSERT(!IsSetFlag(pState->flags, SRV_MPX_TRACKING_FLAG_IN_TRACKER) ||
                    (pState->mpxTrackerListEntry.Next &&
                     pState->mpxTrackerListEntry.Prev &&
                     !LwListIsEmpty(&pState->mpxTrackerListEntry)));

        if (IsSetFlag(pState->flags, SRV_MPX_TRACKING_FLAG_IN_TRACKER))
        {
            LWIO_ASSERT(pTracker->ulCount > 0);

            LwListRemove(&pState->mpxTrackerListEntry);
            pState->flags = 0;

            pTracker->ulCount--;
        }

        LWIO_UNLOCK_MUTEX(bInLock, &pTracker->mutex);
    }
}

VOID
SrvMpxTrackerSetExecutingExecContext(
    PSRV_EXEC_CONTEXT pContext
    )
{
    PSRV_MPX_TRACKER pTracker = pContext->pConnection->pMpxTracker;

    if (pTracker)
    {
        PSRV_MPX_TRACKING_STATE pState = &pContext->trackingState;
        BOOLEAN bInLock = FALSE;

        LWIO_LOCK_MUTEX(bInLock, &pTracker->mutex);
        SetFlag(pState->flags, SRV_MPX_TRACKING_FLAG_EXECUTING);
        LWIO_UNLOCK_MUTEX(bInLock, &pTracker->mutex);
    }
}

BOOLEAN
SrvMpxTrackerIsCancelledExecContext(
    PSRV_EXEC_CONTEXT pContext
    )
{
    BOOLEAN bIsCancelled = FALSE;
    PSRV_MPX_TRACKER pTracker = pContext->pConnection->pMpxTracker;

    if (pTracker)
    {
        PSRV_MPX_TRACKING_STATE pState = &pContext->trackingState;
        BOOLEAN bInLock = FALSE;

        LWIO_LOCK_MUTEX(bInLock, &pTracker->mutex);
        bIsCancelled = IsSetFlag(pState->flags, SRV_MPX_TRACKING_FLAG_CANCELLED);
        LWIO_UNLOCK_MUTEX(bInLock, &pTracker->mutex);
    }

    return bIsCancelled;
}

VOID
SrvMpxTrackerCancelExecContextById(
    PSRV_MPX_TRACKER pTracker,
    ULONG ulPid,
    USHORT usMid,
    USHORT usUid,
    USHORT usTid
    )
{
    BOOLEAN bInLock = FALSE;
    PSRV_MPX_TRACKING_STATE pState = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pTracker->mutex);

    pState = SrvMpxTrackerFind_inlock(pTracker, ulPid, usMid);
    if (pState)
    {
        if ((usUid == pState->usUid) && (usTid == pState->usTid))
        {
            SetFlag(pState->flags, SRV_MPX_TRACKING_FLAG_CANCELLED);
        }
        else
        {
            LWIO_LOG_VERBOSE("UID/TID mismatch for PID/MID = (%d, %d)",
                             ulPid, usMid);
        }
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pTracker->mutex);
}

NTSTATUS
SrvMpxTrackerCreate(
    PSRV_MPX_TRACKER* ppTracker,
    ULONG ulLimit
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_MPX_TRACKER pTracker = NULL;

    ntStatus = SrvAllocateMemory(sizeof(*pTracker), OUT_PPVOID(&pTracker));
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pTracker->list);
    pTracker->ulLimit = ulLimit;
    pTracker->ulCount = 0;

    ntStatus = LwErrnoToNtStatus(pthread_mutex_init(&pTracker->mutex, NULL));
    BAIL_ON_NT_STATUS(ntStatus);
    pTracker->pMutex = &pTracker->mutex;

    *ppTracker = pTracker;

cleanup:

    return ntStatus;

error:

    *ppTracker = NULL;

    if (pTracker)
    {
        SrvMpxTrackerDestroy(pTracker);
    }

    goto cleanup;
}

VOID
SrvMpxTrackerDestroy(
    PSRV_MPX_TRACKER pTracker
    )
{
    LWIO_ASSERT(0 == pTracker->ulCount);

    if (pTracker->pMutex)
    {
        pthread_mutex_destroy(&pTracker->mutex);
    }

    SrvFreeMemory(pTracker);
}


//
// Static Function Definitions
//

static
NTSTATUS
SrvMpxTrackerAdd(
    PSRV_MPX_TRACKER pTracker,
    ULONG ulPid,
    USHORT usMid,
    USHORT usUid,
    USHORT usTid,
    PSRV_MPX_TRACKING_STATE pState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bInLock = FALSE;
    PSRV_MPX_TRACKING_STATE pFoundState = NULL;

    if (0xFFFF == usMid)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_MUTEX(bInLock, &pTracker->mutex);

    pFoundState = SrvMpxTrackerFind_inlock(pTracker, ulPid, usMid);
    if (pFoundState)
    {
        // Context already exists.  Error code will go back
        // to client.

        LWIO_LOG_ERROR("Attempt to add duplicate PID/MID = (%d, %d)",
                         ulPid, usMid);

        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Reached MPX limit
    if (pTracker->ulCount >= pTracker->ulLimit)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LwListInsertHead(&pTracker->list, &pState->mpxTrackerListEntry);
    pState->ulPid = ulPid;
    pState->usMid = usMid;
    pState->usUid = usUid;
    pState->usTid = usTid;
    pState->flags = SRV_MPX_TRACKING_FLAG_IN_TRACKER;

    pTracker->ulCount++;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pTracker->mutex);

    return ntStatus;

error:

    goto cleanup;
}


static
PSRV_MPX_TRACKING_STATE
SrvMpxTrackerFind_inlock(
    PSRV_MPX_TRACKER pTracker,
    ULONG ulPid,
    USHORT usMid
    )
{
    PSRV_MPX_TRACKING_STATE pFoundState = NULL;
    PLW_LIST_LINKS pEntry = NULL;

    if (usMid == 0xFFFF)
    {
        goto cleanup;
    }

    for (pEntry = pTracker->list.Next;
         pEntry != &pTracker->list;
         pEntry = pEntry->Next)
    {
        PSRV_MPX_TRACKING_STATE pState = LW_STRUCT_FROM_FIELD(pEntry, SRV_MPX_TRACKING_STATE, mpxTrackerListEntry);

        if ((ulPid == pState->ulPid) && (usMid == pState->usMid))
        {
            pFoundState = pState;
            break;
        }
    }

cleanup:

    return pFoundState;
}
