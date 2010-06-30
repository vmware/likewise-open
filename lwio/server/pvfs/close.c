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

/* Forward declarations */


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsClose(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;

    /* make sure we have a proper CCB */

    ntError =  PvfsAcquireCCBClose(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Mark the handle as closed in an effort to prevent
       a potential sharing violation */

    pCcb->bCloseInProgress = TRUE;

    if (pCcb->bPendingDeleteHandle)
    {
        /* delete-on-close-handle becomes delete-on-close-file */

        PvfsFcbSetPendingDelete(pCcb->pFcb, TRUE);
    }

    /* Explicitly remove the CCB from the FCB list to force
       rundown that it triggered by closing the last open handle.
       Events like an async IRP cancellation could be running
       in the background maintaining a valid reference to the CCB
       that would other prevent the final PvfsFreeCCB() call */

    PvfsRemoveCCBFromFCB(pCcb->pFcb, pCcb);

    if (PVFS_IS_DIR(pCcb))
    {
        if (pCcb->pDirContext->pDir)
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
            ntError = PvfsOplockMarkPendedOpsReady(pCcb->pFcb);
            break;
        }
    }

    /* Close the fd */

    ntError = PvfsSysClose(pCcb->fd);


    /* Technically, it would be more proper to do this in the utility
       functions in PvfsFreeFCB, but we will end up with memory corruption
       since the FCB is already well on it's way to be free'd.  Can't
       schedule a work item using a free'd FCB */

    if (pCcb->ChangeEvent != 0)
    {
        PvfsNotifyScheduleFullReport(
            pCcb->pFcb,
            pCcb->ChangeEvent,
            FILE_ACTION_MODIFIED,
            pCcb->pszFilename);
    }

    PvfsZctCloseCcb(pCcb);

cleanup:
    /* This is the final Release that will free the memory */


    if (pCcb)
    {
        if (pCcb->pFcb)
        {
            PvfsReleaseFCB(&pCcb->pFcb);
        }

        PvfsReleaseCCB(pCcb);
    }

    /* We can't really do anything here in the case of failure */

    return STATUS_SUCCESS;

error:
    goto cleanup;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
