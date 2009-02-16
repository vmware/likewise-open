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
 *        create.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *       Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* Code */

/* Main entry to the Create() routine for driver.  Splits work based
   on the CreateOptions */

NTSTATUS
PvfsCreate(
    IO_DEVICE_HANDLE IoDeviceHandle,
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    BOOLEAN bIsDirectory = FALSE;
    PIRP pIrp = pIrpContext->pIrp;
    PSTR pszFilename = NULL;
    PVFS_STAT Stat = {0};

    CreateOptions = pIrp->Args.Create.CreateOptions;

    if (CreateOptions & FILE_DIRECTORY_FILE)
    {
        bIsDirectory = TRUE;
    }
    else if (CreateOptions & FILE_NON_DIRECTORY_FILE)
    {
        bIsDirectory = FALSE;
    }
    else
    {
        /* stat() the path and find out if this is a file or directory */

        ntError = PvfsCanonicalPathName(&pszFilename,
                                        pIrp->Args.Create.FileName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysStat(pszFilename, &Stat);
        BAIL_ON_NT_STATUS(ntError);

        bIsDirectory = S_ISDIR(Stat.s_mode);
    }


    if (bIsDirectory)
    {
        pIrp->Args.Create.CreateOptions |= FILE_DIRECTORY_FILE;
        ntError = PvfsCreateDirectory(pIrpContext);
    }
    /* File branch */
    else
    {
        pIrp->Args.Create.CreateOptions |= FILE_NON_DIRECTORY_FILE;
        ntError = PvfsCreateFile(pIrpContext);
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PVFS_SAFE_FREE_MEMORY(pszFilename);

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
