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
 *        lock.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Lock/Unlock Dispatch Routine
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */



/* Code */

/**************************************************************
 *************************************************************/

NTSTATUS
PvfsDispatchLockControl(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    /* If the request is marked as FailImmediately, then
       it must be synchronous */

    if (pIrpContext->pIrp->Args.LockControl.FailImmediately) {
        return PvfsLockControl(pIrpContext);
    }

    return PvfsAsyncLockControl(pIrpContext);
}

/**************************************************************
 *************************************************************/

NTSTATUS
PvfsLockControl(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    IRP_ARGS_LOCK_CONTROL Args = pIrp->Args.LockControl;
    LONG64 Offset = Args.ByteOffset;
    LONG64 Length = Args.Length;
    ULONG Key = Args.Key;
    PVFS_LOCK_FLAGS Flags = 0;
    PPVFS_CCB pCcb = NULL;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (PVFS_IS_DIR(pCcb)) {
        ntError = STATUS_FILE_IS_A_DIRECTORY;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Either READ or WRITE access is ok */

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_READ_DATA);
    if (ntError == STATUS_ACCESS_DENIED) {
        ntError = PvfsAccessCheckFileHandle(pCcb, FILE_WRITE_DATA);
    }
    BAIL_ON_NT_STATUS(ntError);

    if (Args.ExclusiveLock) {
        Flags |= PVFS_LOCK_EXCLUSIVE;
    }
    if (!Args.FailImmediately) {
        Flags |= PVFS_LOCK_BLOCK;
    }

    switch(Args.LockControl) {
    case IO_LOCK_CONTROL_LOCK:
        ntError = PvfsLockFile(pIrpContext, pCcb, &Key, Offset, Length, Flags);
        break;

    case IO_LOCK_CONTROL_UNLOCK:
        ntError = PvfsUnlockFile(pCcb, FALSE, &Key, Offset, Length);
        break;

    case IO_LOCK_CONTROL_UNLOCK_ALL_BY_KEY:
        ntError = PvfsUnlockFile(pCcb, TRUE, &Key, Offset, Length);
        break;

    case IO_LOCK_CONTROL_UNLOCK_ALL:
        ntError = PvfsUnlockFile(pCcb, TRUE, NULL, Offset, Length);
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;

    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    /* Release the CCB unless this is a pending lock */

    if (ntError != STATUS_PENDING && pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

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
