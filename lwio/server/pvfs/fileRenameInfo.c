/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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

static
NTSTATUS
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

static
NTSTATUS
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
    ACCESS_MASK DirGranted = 0;
    ACCESS_MASK DirDesired = 0;
    PVFS_STAT stat = { 0 };
    PSTR pszNewFilename = NULL;
    PSTR pszNewPathname = NULL;
    PSTR streamName = NULL;
    PPVFS_FILE_NAME newFileName = NULL;
    PPVFS_FILE_NAME newDirectoryName = NULL;
    PPVFS_FILE_NAME newResolvedDirName = NULL;
    PPVFS_FILE_NAME newRelativeFileName = NULL;
    PPVFS_FILE_NAME newResolvedFileName = NULL;
    PPVFS_FILE_NAME existingResolvedFileName = NULL;
    PPVFS_FILE_NAME currentFileName = NULL;

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

    ntError = PvfsWC16CanonicalPathName(
                    &pszNewFilename,
                    pFileInfo->FileName,
                    pFileInfo->FileNameLength);
    BAIL_ON_NT_STATUS(ntError);

    if (pRootDirCcb)
    {
        pszNewPathname = NULL;

        /* Check if we need to separate the root path from
           the relative path witha a '/' */

        if (*pszNewFilename == '/')
        {
            ntError = RtlCStringAllocatePrintf(
                          &pszNewPathname,
                          "%s%s",
                          pRootDirCcb->pScb->pOwnerFcb->pszFilename,
                          pszNewFilename);
        }
        else
        {
            ntError = RtlCStringAllocatePrintf(
                          &pszNewPathname,
                          "%s/%s",
                          pRootDirCcb->pScb->pOwnerFcb->pszFilename,
                          pszNewFilename);
        }
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        ntError = RtlCStringDuplicate(&pszNewPathname, pszNewFilename);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAllocateFileNameFromCString(&newFileName, pszNewPathname, 0);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSplitFileNamePath(
                  &newDirectoryName,
                  &newRelativeFileName,
                  newFileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath2(
                  &newResolvedDirName,
                  &stat,
                  newDirectoryName,
                  FALSE);
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ntError = STATUS_OBJECT_PATH_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntError);

    if (PVFS_IS_DIR(pCcb))
    {
        DirDesired = FILE_ADD_SUBDIRECTORY;
    }
    else
    {
        DirDesired = FILE_ADD_FILE;
    }
    ntError = PvfsAccessCheckFile(
                  pCcb->pUserToken,
                  newResolvedDirName,
                  DirDesired,
                  &DirGranted);
    BAIL_ON_NT_STATUS(ntError);

    /* Real work starts here */

    ntError = PvfsAppendFileName(
                  &newResolvedFileName,
                  newResolvedDirName,
                  newRelativeFileName);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for an existing file */

    ntError = PvfsLookupPath2(
                  &existingResolvedFileName,
                  &stat,
                  newResolvedFileName,
                  FALSE);
    if (ntError == STATUS_SUCCESS)
    {
        /* Several cases to consider
           (a) Rename in case only (test => Test)
           (b) Rename to exist file
           (c) Rename to new file
        */

        ntError = PvfsAllocateFileNameFromScb(&currentFileName, pCcb->pScb);
        BAIL_ON_NT_STATUS(ntError);

        if (PvfsFileNameCompare(currentFileName, newResolvedFileName, FALSE) != 0)
        {
            if (pFileInfo->ReplaceIfExists)
            {
                PvfsFreeFileName(newResolvedFileName);
                newResolvedFileName = existingResolvedFileName;
                existingResolvedFileName = NULL;
            }
            else
            {
                ntError = STATUS_OBJECT_NAME_COLLISION;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
    }

    ntError = PvfsRenameCCB(pCcb, newResolvedFileName);
    BAIL_ON_NT_STATUS(ntError);

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

    ntError = PvfsAllocateCStringFromFileName(&streamName, newResolvedFileName);
    if (ntError == STATUS_SUCCESS)
    {
        PvfsNotifyScheduleFullReport(
            pCcb->pScb->pOwnerFcb,
            PVFS_IS_DIR(pCcb) ? FILE_NOTIFY_CHANGE_DIR_NAME : FILE_NOTIFY_CHANGE_FILE_NAME,
            FILE_ACTION_RENAMED_NEW_NAME,
            streamName);
    }

error:
    // PVFS_FILE_NAME
    if (newFileName)
    {
        PvfsFreeFileName(newFileName);
    }
    if (newDirectoryName)
    {
        PvfsFreeFileName(newDirectoryName);
    }
    if (newResolvedDirName)
    {
        PvfsFreeFileName(newResolvedDirName);
    }
    if (newRelativeFileName)
    {
        PvfsFreeFileName(newRelativeFileName);
    }
    if (newResolvedFileName)
    {
        PvfsFreeFileName(newResolvedFileName);
    }
    if (existingResolvedFileName)
    {
        PvfsFreeFileName(existingResolvedFileName);
    }
    if (currentFileName)
    {
        PvfsFreeFileName(currentFileName);
    }

    // PSTR
    if (pszNewFilename)
    {
        LwRtlCStringFree(&pszNewFilename);
    }
    if (pszNewPathname)
    {
        LwRtlCStringFree(&pszNewPathname);
    }
    if (streamName)
    {
        LwRtlCStringFree(&streamName);
    }

    // CCB
    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    if (pRootDirCcb)
    {
        PvfsReleaseCCB(pRootDirCcb);
    }

    return ntError;
}

