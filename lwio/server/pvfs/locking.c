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
 *        locking.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Internal Lock Manager
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
CanLock(
    PPVFS_LOCK_TABLE pLocks,
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    );

static NTSTATUS
AddLock(
    PPVFS_LOCK_TABLE pLocks,
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    );


/* Code */

/**************************************************************
 TODO: Implement blocking locks
 *************************************************************/

NTSTATUS
PvfsLockFile(
    PPVFS_CCB pCcb,
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    PVFS_LOCK_FLAGS Flags
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bFcbLocked = FALSE;
    PPVFS_CCB_LIST_NODE pCursor = NULL;
    PPVFS_FCB pFcb = pCcb->pFcb;
    BOOLEAN bExclusive = FALSE;

    if (Flags & PVFS_LOCK_EXCLUSIVE) {
        bExclusive = TRUE;
    }

    PvfsReaderLockFCB(pCcb->pFcb);
    bFcbLocked = TRUE;

    for (pCursor = PvfsNextCCBFromList(pFcb, pCursor);
         pCursor;
         pCursor = PvfsNextCCBFromList(pFcb, pCursor))
    {
        PPVFS_LOCK_TABLE pLockTable = &pCursor->pCcb->LockTable;

        /* We'll deal with ourselves in AddLock() */

        if (pCcb == pCursor->pCcb) {
            continue;
        }

        ntError = CanLock(pLockTable, pKey, Offset, Length, bExclusive);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = AddLock(&pCcb->LockTable, pKey, Offset, Length, bExclusive);
    BAIL_ON_NT_STATUS(ntError);


cleanup:
    if (bFcbLocked) {
        PvfsReaderUnlockFCB(pCcb->pFcb);
    }

    return ntError;

error:
    goto cleanup;
}

/**************************************************************
 *************************************************************/

NTSTATUS
PvfsUnlockFile(
    PPVFS_CCB pCcb,
    BOOLEAN bUnlockAll,
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/**************************************************************
 *************************************************************/

static NTSTATUS
CanLock(
    PPVFS_LOCK_TABLE pLocks,
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/**************************************************************
 *************************************************************/

static NTSTATUS
AddLock(
    PPVFS_LOCK_TABLE pLocks,
    PULONG pKey,
    LONG64 Offset,
    LONG64 Length,
    BOOLEAN bExclusive
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    BAIL_ON_NT_STATUS(ntError);

cleanup:
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
