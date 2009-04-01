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
 *        fileRenameInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileRenameInformation Handler
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static NTSTATUS
PvfsSetFileRenameInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/* File Globals */



/* Code */


/****************************************************************
 ***************************************************************/

NTSTATUS
PvfsFileRenameInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = PvfsSetFileRenameInfo(pIrpContext);
        break;

    case PVFS_QUERY:
        ntError =  STATUS_NOT_SUPPORTED;
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

/****************************************************************
 ***************************************************************/

static NTSTATUS
PvfsSetFileRenameInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PPVFS_CCB pRootDirCcb = NULL;
    PFILE_RENAME_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetInformation;
    PSTR pszNewFilename = NULL;
    PSTR pszNewPathname = NULL;
    ACCESS_MASK DirGranted = 0;
    ACCESS_MASK DirDesired = 0;
    PSTR pszNewFileDirname = NULL;
    PSTR pszNewFileBasename = NULL;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    pFileInfo = (PFILE_RENAME_INFORMATION)Args.FileInformation;

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (pFileInfo->RootDirectory)
    {
        ntError =  PvfsAcquireCCB(pFileInfo->RootDirectory, &pRootDirCcb);
        BAIL_ON_NT_STATUS(ntError);

        if (!PVFS_IS_DIR(pRootDirCcb)) {
            ntError = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    /* Make sure we have DELETE permissions on the source file */

    ntError = PvfsAccessCheckFileHandle(pCcb, DELETE);
    BAIL_ON_NT_STATUS(ntError);

    /* Make sure we can add the new object to the parent directory
       (not necessarily the same as the RootDirectory handle) */

    ntError = PvfsWC16CanonicalPathName(&pszNewFilename, pFileInfo->FileName);
    BAIL_ON_NT_STATUS(ntError);

    pszNewPathname = pszNewFilename;
    if (pRootDirCcb)
    {
        pszNewPathname = NULL;
        ntError = RtlCStringAllocatePrintf(&pszNewPathname,
                                           "%s/%s",
                                           pRootDirCcb->pszFilename,
                                           pszNewFilename);
        PVFS_SAFE_FREE_MEMORY(pszNewFilename);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsFileSplitPath(&pszNewFileDirname, &pszNewFileBasename, pszNewPathname);
    BAIL_ON_NT_STATUS(ntError);

    if (PVFS_IS_DIR(pCcb)) {
        DirDesired = FILE_ADD_SUBDIRECTORY;
    } else {
        DirDesired = FILE_ADD_FILE;
    }
    ntError = PvfsAccessCheckFile(pCcb->pUserToken,
                                  pszNewFileDirname,
                                  DirDesired,
                                  &DirGranted);
    BAIL_ON_NT_STATUS(ntError);

    /* Real work starts here */

    /* Check for an existing file if not asked to overwrite */

    if (pFileInfo->ReplaceIfExists == FALSE)
    {
        PVFS_STAT Stat = {0};

        ntError = PvfsSysStat(pszNewPathname, &Stat);
        if (ntError == STATUS_SUCCESS) {
            ntError = STATUS_OBJECT_NAME_COLLISION;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    ntError = PvfsValidatePath(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysRename(pCcb->pszFilename, pszNewPathname);
    BAIL_ON_NT_STATUS(ntError);

    PVFS_SAFE_FREE_MEMORY(pCcb->pszFilename);
    pCcb->pszFilename = pszNewPathname;
    pszNewPathname = NULL;

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

cleanup:
    PVFS_SAFE_FREE_MEMORY(pszNewPathname);

    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }
    if (pRootDirCcb) {
        PvfsReleaseCCB(pRootDirCcb);
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

