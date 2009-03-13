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

static NTSTATUS
PerformDeleteOnClose(
    PPVFS_CCB pCcb
    );

static NTSTATUS
SetLastWriteTime(
    PPVFS_CCB pCcb
    );



/* Code */


/******************************************************************
 *****************************************************************/

NTSTATUS
PvfsClose(
    IO_DEVICE_HANDLE DeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    BOOLEAN bValidPath = FALSE;

    /* make sure we have a proper CCB */

    ntError =  PvfsAcquireCCBClose(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsValidatePath(pCcb);
    bValidPath = (ntError == STATUS_SUCCESS);

    /* Deal with delete-on-close */

    if (pCcb->CreateOptions & FILE_DELETE_ON_CLOSE) {
        ntError = PerformDeleteOnClose(pCcb);
        /* Don't fail */
    }

    /* Call closedir() for directions and close() for files */

    if (PVFS_IS_DIR(pCcb)) {
        ntError = PvfsSysCloseDir(pCcb->pDirContext->pDir);
        /* pCcb->fd is invalid now */
    } else {
        ntError = PvfsSysClose(pCcb->fd);
    }
    /* Don't fail */

    /* "sticky" last write times (if we didn't delete it already) */

    if (bValidPath &&
        !(pCcb->CreateOptions & FILE_DELETE_ON_CLOSE) &&
        pCcb->LastWriteTime != 0)
    {
        ntError = SetLastWriteTime(pCcb);
        /* Don't fail */
    }



cleanup:
    /* This is the final Release that will free the memory */

    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    /* We can't really do anything here in the case of failure */

    return STATUS_SUCCESS;

error:
    goto cleanup;
}


/******************************************************************
 *****************************************************************/

static NTSTATUS
PerformDeleteOnClose(
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    /* Check for renames */

    ntError = PvfsValidatePath(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysRemove(pCcb->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    /* Never fail this */
    return STATUS_SUCCESS;

error:
    goto cleanup;
}

/******************************************************************
 Note that the file/directory has been closed prior to this
 so the pCcb->fd is actually invalid.  You only have the
 pszFilename to work with.
 *****************************************************************/

static NTSTATUS
SetLastWriteTime(
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVFS_STAT Stat = {0};
    LONG64 LastAccessTime = 0;

    /* Need the original access time */

    ntError = PvfsSysStat(pCcb->pszFilename, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&LastAccessTime, Stat.s_atime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysUtime(pCcb->pszFilename,
                           pCcb->LastWriteTime,
                           LastAccessTime);
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
