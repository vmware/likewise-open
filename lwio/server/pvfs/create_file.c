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
 *        create_file.c
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

static NTSTATUS
PvfsCreateFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsCreateFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsCreateFileOpen(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsCreateFileOpenIf(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsCreateFileOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsCreateFileOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext
	);


/* Code */

/**
 * Top level driver for creating an actual file.  Splits
 * work based on the CreateDisposition.
 */

NTSTATUS
PvfsCreateFile(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;

    CreateDisposition = pIrpContext->pIrp->Args.Create.CreateDisposition;

    switch (CreateDisposition)
    {
    case FILE_SUPERSEDE:
        ntError = PvfsCreateFileSupersede(pIrpContext);
        break;

    case FILE_CREATE:
        ntError = PvfsCreateFileCreate(pIrpContext);
        break;

    case FILE_OPEN:
        ntError = PvfsCreateFileOpen(pIrpContext);
        break;

    case FILE_OPEN_IF:
        ntError = PvfsCreateFileOpenIf(pIrpContext);
        break;

    case FILE_OVERWRITE:
        ntError = PvfsCreateFileOverwrite(pIrpContext);
        break;

    case FILE_OVERWRITE_IF:
        ntError = PvfsCreateFileOverwriteIf(pIrpContext);
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);


cleanup:
    return ntError;

error:
    goto cleanup;
}

/**
 *
 *
 */

static NTSTATUS
MapPosixOpenFlags(
    int *unixFlags,
    ACCESS_MASK desiredAccess,
    FILE_CREATE_OPTIONS createOptions
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    /* This should derive the unixFlags based on the
       access_mask and the request create_options.
       Fake it for the moment. */

    *unixFlags = O_RDWR;
    ntError = STATUS_SUCCESS;

    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/**
 *
 *
 */

static NTSTATUS
PvfsCreateFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    return STATUS_NOT_IMPLEMENTED;

}

/**
 *
 *
 */

static NTSTATUS
PvfsCreateFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    IO_FILE_NAME fileName = pIrp->Args.Create.FileName;
    PSTR pszFilename = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;

    ntError = RtlCStringAllocateFromWC16String(&pszFilename,
                                               fileName.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

#if 0
    /* Check whether the user can create a new file */

    ntError = PvfsCheckParentSecurity(pIrp->Args.Create.SecurityContext,
                                      pszFilename,
                                      FILE_WRITE_DATA);
    BAIL_ON_NT_STATUS(ntError);
#endif

    ntError = MapPosixOpenFlags(&unixFlags,
                                pIrp->Args.Create.DesiredAccess,
                                pIrp->Args.Create.CreateOptions);
    BAIL_ON_NT_STATUS(ntError);

    /* Make sure we are dealing with file creation */

    unixFlags |= O_CREAT | O_EXCL;

    if ((fd = open(pszFilename, unixFlags, 0600)) == -1)
    {
        int err = errno;

        ntError = PvfsMapUnixErrnoToNtStatus(err);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = MAXIMUM_ALLOWED;
    pCcb->CreateOptions = pIrp->Args.Create.CreateOptions;
    pCcb->pszFilename = pszFilename;

    ntError = IoFileSetContext(pIrp->FileHandle, (PVOID)pCcb);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    if (fd != -1)
    {
        // Need to remove the file as well
        close(fd);
    }

    PVFS_SAFE_FREE_MEMORY(pCcb);
    PVFS_SAFE_FREE_MEMORY(pszFilename);

    goto cleanup;
}

/**
 *
 *
 */
NTSTATUS
PvfsCreateFileOpen(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/**
 *
 *
 */

static NTSTATUS
PvfsCreateFileOpenIf(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/**
 *
 *
 */

static NTSTATUS
PvfsCreateFileOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

/**
 *
 *
 */

static NTSTATUS
PvfsCreateFileOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    return STATUS_NOT_IMPLEMENTED;
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
