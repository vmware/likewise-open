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
    IN FILE_SHARE_FLAGS ShareAccess,
    IN ACCESS_MASK DesiredAccess,
    OUT PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FCB pFcb = NULL;
    BOOLEAN bLockedTable = FALSE;
    PVFS_STAT Stat = {0};

    /* LOCK_MUTEX(gTable) */
    bLockedTable = TRUE;

    /* First we have to find the FCB in our table of open
       files */

    /* FIND_FCB(gTable, pszFilename); */

    /* CHECK_SHARING(pFcb, ShareAccess); */
    /* CHECK_DESIRED_ACCESS(pFcb, DesiredAccess); */

    /* If not found, then add one */

    ntError = PvfsAllocateFCB(&pFcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCStringDuplicate(&pFcb->pszFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysStat(pFcb->pszFilename, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    pFcb->Device = Stat.s_dev;
    pFcb->Inode  = Stat.s_ino;

    pFcb->ShareAccess = ShareAccess;

    /* ADD_FCB(gTable, pFCB); */

    /* UNLOCK_MUTEX(gTable); */

    *ppFcb = pFcb;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    if (pFcb) {
        PvfsReleaseFCB(pFcb);
    }

    if (bLockedTable) {
        /* UNLOCK_MUTEX(gTable) */;
    }

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
