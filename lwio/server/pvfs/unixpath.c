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
 *        unixpath.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        POSIX filename routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* Code */


/********************************************************
 *******************************************************/

NTSTATUS
PvfsCanonicalPathName(
    PSTR *ppszPath,
    IO_FILE_NAME IoPath
    )
{
    return PvfsWC16CanonicalPathName(ppszPath, IoPath.FileName);
}


/********************************************************
 *******************************************************/

NTSTATUS
PvfsWC16CanonicalPathName(
    PSTR *ppszPath,
    PWSTR pwszPathname
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszPath = NULL;
    PSTR pszCursor = NULL;
    size_t Length = 0;
    int i = 0;

    ntError = RtlCStringAllocateFromWC16String(&pszPath,
                                               pwszPathname);
    BAIL_ON_NT_STATUS(ntError);

    pszCursor = pszPath;
    while (pszCursor && *pszCursor)
    {
        if (*pszCursor == '\\') {
            *pszCursor = '/';
        }
        pszCursor++;
    }

    /* Strip trailing slashes */

    Length = RtlCStringNumChars(pszPath);
    for (i=Length-1; i>=0; i--)
    {
        /* break out at first non slash */
        if (pszPath[i] != '/') {
            break;
        }

        pszPath[i] = '\0';
    }

    *ppszPath = pszPath;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsValidatePath(
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError= STATUS_UNSUCCESSFUL;
    PVFS_STAT Stat = {0};

    /* Verify that the dev/inode pair is the same on the pathname
       and the fd */

    ntError = PvfsSysStat(pCcb->pszFilename, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    if ((pCcb->device != Stat.s_dev) || (pCcb->inode != Stat.s_ino))
    {
        ntError = STATUS_FILE_RENAMED;
        BAIL_ON_NT_STATUS(ntError);
    }

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsFileBasename(
    PSTR *ppszFilename,
    PCSTR pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszCursor = NULL;

    if ((pszCursor = strrchr(pszPath, '/')) != NULL)
    {
        /* Assume there is never a trailing '/' since that should
           be handled by PvfsCanonicalPathName() */

        pszCursor++;
    }

    if (pszCursor != NULL) {
        ntError = RtlCStringDuplicate(ppszFilename, pszCursor);
    } else {
        ntError = RtlCStringDuplicate(ppszFilename, pszPath);
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsFileDirname(
    PSTR *ppszDirname,
    PCSTR pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszCursor = NULL;
    PSTR pszNewString = NULL;

    /* Case #1: No '/' so just return '.' */
    if (((pszCursor = strchr(pszPath, '/')) == NULL))
    {
        ntError = RtlCStringDuplicate(ppszDirname, ".");
        goto cleanup;
    }

    /* Case #2: only one '/' (at beginning of path) */

    if (pszCursor == pszPath) {
        ntError = RtlCStringDuplicate(ppszDirname, "/");
        goto cleanup;
    }

    /* Case #3: Real dirname and file name components */

    ntError = RTL_ALLOCATE(&pszNewString, CHAR,
                           PVFS_PTR_DIFF(pszPath,pszCursor) + 1);
    BAIL_ON_NT_STATUS(ntError);

    RtlCopyMemory(pszNewString, pszPath, PVFS_PTR_DIFF(pszPath,pszCursor));

    *ppszDirname = pszNewString;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsFileSplitPath(
    PSTR *ppszDirname,
    PSTR *ppszBasename,
    PCSTR pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    ntError = PvfsFileDirname(ppszDirname, pszPath);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFileBasename(ppszBasename, pszPath);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;

}


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsLookupPath(
    PSTR *ppszDiskPath,
    PCSTR pszPath
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVFS_STAT Stat = {0};
    PSTR pszDiskPath = NULL;

    ntError = RtlCStringDuplicate(&pszDiskPath, pszPath);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysStat(pszDiskPath, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    *ppszDiskPath = pszDiskPath;
    pszDiskPath = NULL;

    ntError = STATUS_SUCCESS;

cleanup:

    RtlCStringFree(&pszDiskPath);

    return ntError;

error:
    goto cleanup;
}

/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsLookupFile(
    PSTR *ppszDiskPath,
    PCSTR pszDiskDirname,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PSTR pszFullPath = NULL;
    PVFS_STAT Stat = {0};

    ntError = RtlCStringAllocatePrintf(&pszFullPath,
                                       "%s/%s",
                                       pszDiskDirname,
                                       pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysStat(pszFullPath, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    *ppszDiskPath = pszFullPath;
    pszFullPath = NULL;

    ntError = STATUS_SUCCESS;

cleanup:
    RtlCStringFree(&pszFullPath);

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
