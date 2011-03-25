/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
 *        close.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Close Dispatch Function
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

static
NTSTATUS
PvfsExecuteDeleteOnCloseSCB(
    IN PPVFS_SCB pScb
    );

static
NTSTATUS
PvfsExecuteDeleteOnCloseFCB(
    IN PPVFS_FCB pFcb
    );

static
NTSTATUS
PvfsFlushLastWriteTimeFcb(
    IN PPVFS_FCB pFcb,
    IN LONG64 LastWriteTime
    );

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsClose(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PPVFS_SCB pScb = NULL;

    ntError =  PvfsAcquireCCBClose(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ///
    /// State transitions
    ///

    // Mark the handle as closed to prevent a future sharing violations

    SetFlag(pCcb->Flags, PVFS_CCB_FLAG_CLOSE_IN_PROGRESS);

    if (IsSetFlag(pCcb->Flags, PVFS_CCB_FLAG_PENDING_DELETE) &&
        IsSetFlag(pCcb->Flags, PVFS_CCB_FLAG_CREATE_COMPLETE))
    {
        PvfsScbSetPendingDelete(pCcb->pScb, TRUE);
    }

    ///
    /// FileHandle resource cleanup
    ///


    if (PVFS_IS_DIR(pCcb))
    {
        if (pCcb->pDirContext && pCcb->pDirContext->pDir)
        {
            ntError = PvfsSysCloseDir(pCcb->pDirContext->pDir);
            /* pCcb->fd is invalid now */
        }
    }
    else
    {
        /* Release all byte range locks to ensure proper
           processing of pending locks */

        ntError = PvfsUnlockFile(pCcb, TRUE, 0, 0, 0);

        /* Deal with any pended operations awaiting an oplock break response */

        switch (pCcb->OplockState)
        {
        case PVFS_OPLOCK_STATE_NONE:
            break;

        case PVFS_OPLOCK_STATE_GRANTED:
            /* The IoMgr will cancel all pending IRPs on a handle prior to close
               so this state is not possible unless there is a bug in the IoMgr */
            break;

        case PVFS_OPLOCK_STATE_BREAK_IN_PROGRESS:
            /* This is our Ack */
            ntError = PvfsOplockMarkPendedOpsReady(pCcb->pScb);
            break;
        }
    }

    /* Explicitly remove the CCB from the SCB list to force
       rundown that it triggered by closing the last open handle.
       Events like an async IRP cancellation could be running
       in the background maintaining a valid reference to the CCB
       that would other prevent the final PvfsFreeCCB() call */

    pScb = pCcb->pScb;
    pCcb->pScb = NULL;

    PvfsRemoveCCBFromSCB(pScb, pCcb);

    /* Close the fd */

    if (pCcb->fd != -1)
    {
        ntError = PvfsSysClose(pCcb->fd);
    }

    /* Technically, it would be more proper to do this in the utility
       functions in PvfsFreeSCB, but we will end up with memory corruption
       since the SCB is already well on it's way to be free'd.  Can't
       schedule a work item using a free'd SCB */

    if (pCcb->ChangeEvent != 0)
    {
        PvfsNotifyScheduleFullReport(
                pScb->pOwnerFcb,
                pCcb->ChangeEvent,
                FILE_ACTION_MODIFIED,
                pScb->pOwnerFcb->pszFilename);
    }

    PvfsZctCloseCcb(pCcb);

error:

    /* This is the final Release that will free the memory */

    if (pScb)
    {
        PvfsReleaseSCB(&pScb);
    }

    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    /* We can't really do anything here in the case of failure */

    return STATUS_SUCCESS;
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsCloseHandleCleanup(
    IN PPVFS_SCB pScb
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSTR fullStreamName = NULL;
    PPVFS_FILE_NAME streamName = NULL;
    BOOLEAN scbLocked = FALSE;
    BOOLEAN fcbLocked = FALSE;
    BOOLEAN objectDeleted = FALSE;
    PVFS_STAT statBuf = { 0 };

    if (PVFS_IS_DEVICE_HANDLE(pScb))
    {
        status = STATUS_SUCCESS;
        goto error;
    }

    if (!PvfsIsDefaultStream(pScb))
    {
        if (pScb->OpenHandleCount == 0)
        {
            LWIO_LOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

            if (pScb->bDeleteOnClose)
            {
                status = PvfsAllocateFileNameFromScb(&streamName, pScb);
                BAIL_ON_NT_STATUS(status);

                status = PvfsSysStatByFileName(streamName, &statBuf);
                BAIL_ON_NT_STATUS(status);

                // The locking heirarchy requires that we drop the SCB control
                // block mutex before trying to pick up the ScbTable exclusive
                // lock

                if (!pScb->BaseControlBlock.Removed)
                {
                    PPVFS_CB_TABLE_ENTRY pBucket = pScb->BaseControlBlock.pBucket;

                    LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

                    status = PvfsCbTableRemove(pBucket, fullStreamName);
                    LWIO_ASSERT(status == STATUS_SUCCESS);

                    LWIO_LOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

                    pScb->BaseControlBlock.Removed = TRUE;
                    pScb->BaseControlBlock.pBucket = NULL;

                    LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);
                }

                status = PvfsExecuteDeleteOnCloseSCB(pScb);
                // Ignore errors here

                LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

                PvfsPathCacheRemove(streamName);

                objectDeleted = TRUE;
            }

            LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);
        }
    }
    else
    {
        if (pScb->pOwnerFcb->OpenHandleCount == 0)
        {
            LWIO_LOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);
            LWIO_LOCK_MUTEX(fcbLocked, &pScb->pOwnerFcb->BaseControlBlock.Mutex);

            if (pScb->pOwnerFcb->bDeleteOnClose)
            {
                status = PvfsAllocateFileNameFromScb(&streamName, pScb);
                BAIL_ON_NT_STATUS(status);

                status = PvfsSysStatByFileName(streamName, &statBuf);
                BAIL_ON_NT_STATUS(status);

                // The locking heirarchy requires that we drop the FCB control
                // block mutex before trying to pick up the ScbTable exclusive
                // lock

                if (!pScb->pOwnerFcb->BaseControlBlock.Removed)
                {
                    PPVFS_CB_TABLE_ENTRY pBucket = pScb->pOwnerFcb->BaseControlBlock.pBucket;

                    LWIO_UNLOCK_MUTEX(fcbLocked, &pScb->pOwnerFcb->BaseControlBlock.Mutex);

                    status = PvfsCbTableRemove(pBucket, pScb->pOwnerFcb->pszFilename);
                    LWIO_ASSERT(status == STATUS_SUCCESS);

                    LWIO_LOCK_MUTEX(fcbLocked, &pScb->pOwnerFcb->BaseControlBlock.Mutex);

                    pScb->pOwnerFcb->BaseControlBlock.Removed = TRUE;
                    pScb->pOwnerFcb->BaseControlBlock.pBucket = NULL;

                    LWIO_UNLOCK_MUTEX(fcbLocked, &pScb->pOwnerFcb->BaseControlBlock.Mutex);

                }

                pScb->bDeleteOnClose = FALSE;
                status = PvfsExecuteDeleteOnCloseFCB(pScb->pOwnerFcb);
                // Ignore errors here

                LWIO_UNLOCK_MUTEX(fcbLocked, &pScb->pOwnerFcb->BaseControlBlock.Mutex);
                LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

                PvfsPathCacheRemove(streamName);

                objectDeleted = TRUE;
            }
            else
            {
                // Since we aren't deleting the file, see if we have a cached timestamp to set
                LONG64 lastWriteTime = PvfsClearLastWriteTimeFCB(pScb->pOwnerFcb);

                if (lastWriteTime != 0)
                {
                    status = PvfsFlushLastWriteTimeFcb(pScb->pOwnerFcb, lastWriteTime);

                    if (status == STATUS_SUCCESS)
                    {
                        PvfsNotifyScheduleFullReport(
                            pScb->pOwnerFcb,
                            FILE_NOTIFY_CHANGE_LAST_WRITE,
                            FILE_ACTION_MODIFIED,
                            pScb->pOwnerFcb->pszFilename);
                    }
                }
            }


            LWIO_UNLOCK_MUTEX(fcbLocked, &pScb->pOwnerFcb->BaseControlBlock.Mutex);
            LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);
        }
    }


    if (objectDeleted)
    {
        status = PvfsAllocateCStringFromFileName(&fullStreamName, streamName);
        BAIL_ON_NT_STATUS(status);

        PvfsNotifyScheduleFullReport(
            pScb->pOwnerFcb,
            S_ISDIR(statBuf.s_mode) ?
            FILE_NOTIFY_CHANGE_DIR_NAME :
            FILE_NOTIFY_CHANGE_FILE_NAME,
            FILE_ACTION_REMOVED,
            fullStreamName);
    }


error:

    LWIO_UNLOCK_MUTEX(fcbLocked, &pScb->pOwnerFcb->BaseControlBlock.Mutex);
    LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

    if (streamName)
    {
        PvfsFreeFileName(streamName);
    }
    if (fullStreamName)
    {
        LwRtlCStringFree(&fullStreamName);
    }


    return status;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsFlushLastWriteTimeFcb(
    IN PPVFS_FCB pFcb,
    IN LONG64 LastWriteTime
    )
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    PVFS_STAT Stat = {0};
    LONG64 LastAccessTime = 0;

    /* Need the original access time */

    status = PvfsSysStat(pFcb->pszFilename, &Stat);
    BAIL_ON_NT_STATUS(status);

    status = PvfsUnixToWinTime(&LastAccessTime, Stat.s_atime);
    BAIL_ON_NT_STATUS(status);

    status = PvfsSysUtimeByFcb(pFcb, LastWriteTime, LastAccessTime);
    BAIL_ON_NT_STATUS(status);

error:
    return status;
}

////////////////////////////////////////////////////////////////////////
// Requires that SCB->BaseControlBlock.Mutex is locked

static
NTSTATUS
PvfsExecuteDeleteOnCloseSCB(
    IN PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_FILE_NAME streamName = {0};

    // Always reset the delete-pending state to be safe

    pScb->bDeleteOnClose = FALSE;

    // Verify we are deleting the file we think we are

    ntError = PvfsValidatePathSCB(pScb, &pScb->FileId);
    if (ntError == STATUS_SUCCESS)
    {
        ntError = PvfsBuildFileNameFromScb(&streamName, pScb);
        // Don't BAIL_ON_NT_STATUS() so the FileId is always reset
        if (ntError == STATUS_SUCCESS)
        {
            ntError = PvfsSysRemoveByFileName(&streamName);
        }

        /* Reset dev/inode state */

        pScb->FileId.Device = 0;
        pScb->FileId.Inode  = 0;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsDestroyFileName(&streamName);

    return ntError;

error:
    switch (ntError)
    {
    case STATUS_OBJECT_NAME_NOT_FOUND:
        break;

    default:
        LWIO_LOG_ERROR(
            "%s: (SCB) Failed to execute delete-on-close on \"%s%s%s\" (%d,%d) (%s)\n",
            PVFS_LOG_HEADER,
            pScb->pOwnerFcb->pszFilename,
            pScb->pszStreamname ? ":" : "",
            pScb->pszStreamname ? pScb->pszStreamname : "",
            pScb->FileId.Device,
            pScb->FileId.Inode,
            LwNtStatusToName(ntError));
        break;
    }

    goto cleanup;
}

////////////////////////////////////////////////////////////////////////
// Requires that FCB->BaseControlBlock.Mutex is locked

static
NTSTATUS
PvfsExecuteDeleteOnCloseFCB(
    IN PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_FILE_NAME fileName = {0};

    // Always reset the delete-pending state to be safe

    pFcb->bDeleteOnClose = FALSE;

    // Verify we are deleting the file we think we are

    ntError = PvfsValidatePathFCB(pFcb, &pFcb->FileId);
    if (ntError == STATUS_SUCCESS)
    {
        ntError = PvfsBuildFileNameFromCString(&fileName, pFcb->pszFilename, 0);
        // Don't BAIL_ON_NT_STATUS() so the FileId is always reset
        if (ntError == STATUS_SUCCESS)
        {
            ntError = PvfsSysRemoveByFileName(&fileName);
        }

        /* Reset dev/inode state */

        pFcb->FileId.Device = 0;
        pFcb->FileId.Inode  = 0;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsDestroyFileName(&fileName);

    return ntError;

error:
    switch (ntError)
    {
    case STATUS_OBJECT_NAME_NOT_FOUND:
        break;

    default:
        LWIO_LOG_ERROR(
            "%s: (FCB) Failed to execute delete-on-close on \"%s\" (%d,%d) (%s)\n",
            PVFS_LOG_HEADER,
            pFcb->pszFilename,
            pFcb->FileId.Device,
            pFcb->FileId.Inode,
            LwNtStatusToName(ntError));
        break;
    }

    goto cleanup;
}
