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
 *        cutil_open.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Utility functions for the Unix open() syscall
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
MapPosixOpenDisposition(
    int *unixFlags,
    FILE_CREATE_DISPOSITION Disposition,
    BOOLEAN bIsDir
    );

static NTSTATUS
MapPosixOpenAccess(
    int *unixFlags,
    ACCESS_MASK Access,
    BOOLEAN bIsDir
    );

static NTSTATUS
MapPosixOpenOptions(
    int *unixFlags,
    FILE_CREATE_OPTIONS Options,
    BOOLEAN bIsDir
    );


/* Code */

/********************************************************
 *******************************************************/

NTSTATUS
MapPosixOpenFlags(
    int *unixFlags,
    ACCESS_MASK Access,
    IRP_ARGS_CREATE CreateArgs
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int iUnixMode = 0;
    BOOLEAN bIsDir = FALSE;

    bIsDir = (CreateArgs.CreateOptions & FILE_DIRECTORY_FILE) ?
             TRUE : FALSE;

    ntError = MapPosixOpenDisposition(&iUnixMode,
                                      CreateArgs.CreateDisposition,
                                      bIsDir);
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenAccess(&iUnixMode, Access, bIsDir);
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenOptions(&iUnixMode,
                                  CreateArgs.CreateOptions,
                                  bIsDir);
    BAIL_ON_NT_STATUS(ntError);

    *unixFlags = iUnixMode;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/********************************************************
 *******************************************************/

static NTSTATUS
MapPosixOpenDisposition(
    int *unixFlags,
    FILE_CREATE_DISPOSITION Disposition,
    BOOLEAN bIsDir
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int iUnixMode = 0;

    switch (Disposition)
    {
    case FILE_OVERWRITE_IF:
    case FILE_SUPERSEDE:
        iUnixMode |= O_CREAT | O_TRUNC;
        break;

    case FILE_CREATE:
        iUnixMode =  O_CREAT | O_EXCL;
        break;

    case FILE_OPEN:
        /* No extra flags */
        break;

    case FILE_OPEN_IF:
        iUnixMode =  O_CREAT;
        break;

    case FILE_OVERWRITE:
        iUnixMode |= O_TRUNC;
        break;

    default:
        ntError = STATUS_INVALID_PARAMETER;
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

    *unixFlags |= iUnixMode;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/********************************************************
 *******************************************************/

static NTSTATUS
MapPosixOpenAccess(
    int *unixFlags,
    ACCESS_MASK Access,
    BOOLEAN bIsDir
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    int iUnixMode = 0;
    BOOLEAN bRead = Access & FILE_READ_DATA;
    BOOLEAN bWrite = Access & FILE_WRITE_DATA;

    /* These really only apply when opening a file */
    if (!bIsDir)
    {
        if (bRead && bWrite) {
            iUnixMode = O_RDWR;
        } else if (bRead) {
            iUnixMode = O_RDONLY;
        } else if (bWrite) {
            iUnixMode = O_WRONLY;
        } else {
            ntError = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntError);
        }

#if 0 /* disabled */
        if (Access & FILE_APPEND_DATA) {
            iUnixMode |= O_APPEND;
        }
#endif
    }

    *unixFlags |= iUnixMode;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/********************************************************
 *******************************************************/

static NTSTATUS
MapPosixOpenOptions(
    int *unixFlags,
    FILE_CREATE_OPTIONS Options,
    BOOLEAN bIsDir
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    int iUnixMode = 0;

    /* Ignore most options here at the moment */

    if (!bIsDir)
    {
        if (Options & FILE_WRITE_THROUGH) {
            iUnixMode |= O_SYNC;
        }
    }

    *unixFlags |= iUnixMode;
    ntError = STATUS_SUCCESS;

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
