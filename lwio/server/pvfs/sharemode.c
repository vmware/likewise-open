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
 *        sharemode.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Support for Windows File Share Modes
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* Code */

/***********************************************************
 **********************************************************/

NTSTATUS
PvfsCheckShareMode(
    IN PSTR pszFilename,
    IN FILE_SHARE_FLAGS SharedAccess,
    IN ACCESS_MASK DesiredAccess,
    OUT PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FCB pFcb = NULL;

    ntError = PvfsFindFCB(&pFcb, pszFilename);
    if (ntError == STATUS_SUCCESS) {

        ntError = PvfsEnforceShareMode(pFcb,
                                       SharedAccess,
                                       DesiredAccess);
        BAIL_ON_NT_STATUS(ntError);

        *ppFcb = pFcb;

        goto cleanup;
    }

    if (ntError != STATUS_OBJECT_NAME_NOT_FOUND) {
        BAIL_ON_NT_STATUS(ntError);
    }

    /* If not found, then add one */

    ntError = PvfsCreateFCB(&pFcb, pszFilename,
                            SharedAccess, DesiredAccess);
    BAIL_ON_NT_STATUS(ntError);

    *ppFcb = pFcb;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    if (pFcb) {
        PvfsReleaseFCB(pFcb);
    }

    goto cleanup;
}


/***********************************************************
 **********************************************************/

NTSTATUS
PvfsEnforceShareMode(
    IN PPVFS_FCB pFcb,
    IN FILE_SHARE_FLAGS ShareAccess,
    IN ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_CCB_LIST_NODE pCursor = NULL;
    BOOLEAN bLocked = FALSE;

    /* If the caller wants exclusive access, fail if we have any
       open instances already */

    if (ShareAccess == 0) {
        ntError = pFcb->pCcbList ? STATUS_SUCCESS : STATUS_SHARING_VIOLATION;
        goto cleanup;
    }

    RtlMapGenericMask(&DesiredAccess, &gPvfsFileGenericMapping);

    PvfsReaderLockFCB(pFcb);
    bLocked = TRUE;

    for (pCursor = PvfsNextCCBFromList(pFcb, pCursor);
         pCursor;
         pCursor = PvfsNextCCBFromList(pFcb, pCursor))
    {
        ACCESS_MASK Mask = pCursor->pCcb->AccessGranted;
        FILE_SHARE_FLAGS Flags = pCursor->pCcb->ShareFlags;

        /* File was previously opened for exclusive access */

        if (Flags == 0) {
            ntError = STATUS_SHARING_VIOLATION;
            break;
        }

        /* Check incoming Desired Access */

        if ((DesiredAccess & FILE_READ_DATA) && !(Flags & FILE_SHARE_READ))
        {
            ntError = STATUS_SHARING_VIOLATION;
            break;
        }

        if ((DesiredAccess & FILE_WRITE_DATA) && !(Flags & FILE_SHARE_WRITE))
        {
            ntError = STATUS_SHARING_VIOLATION;
            break;
        }

        if ((DesiredAccess & DELETE) && !(Flags & FILE_SHARE_DELETE))
        {
            ntError = STATUS_SHARING_VIOLATION;
            break;
        }

        /* Check incoming File ShareAccess */

        if (ShareAccess & FILE_SHARE_WRITE)
        {
            if ((Mask & FILE_READ_DATA) && !(ShareAccess & FILE_SHARE_READ))
            {
                ntError = STATUS_SHARING_VIOLATION;
                break;
            }
        }

        if (ShareAccess & FILE_SHARE_READ)
        {
            if ((Mask & FILE_WRITE_DATA) && !(ShareAccess & FILE_SHARE_WRITE))
            {
                ntError = STATUS_SHARING_VIOLATION;
                break;
            }
        }

        if ((ShareAccess & FILE_SHARE_DELETE) &&
            (Mask & (DELETE|FILE_READ_DATA|FILE_WRITE_DATA)))
        {
            ntError = STATUS_SHARING_VIOLATION;
            break;
        }
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    if (bLocked) {
        PvfsReaderUnlockFCB(pFcb);
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
