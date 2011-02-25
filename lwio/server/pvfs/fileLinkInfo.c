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
 *        fileLinkInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        FileLinkInformation Handler
 *
 * Authors: Evgeny Popovich <epopovich@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


/* File Globals */



/* Code */

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsSetFileLinkInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsFileLinkInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = PvfsSetFileLinkInfo(pIrpContext);
        break;

    case PVFS_QUERY:
        ntError = STATUS_NOT_SUPPORTED;
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


/*****************************************************************************
 ****************************************************************************/

/*
 * NOTICE: To support links properly so that things like oplocks work properly,
 * we need a LINK_CONTROL_BLOCK layer in between the CCB and SCB.
 */
static
NTSTATUS
PvfsSetFileLinkInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PPVFS_CCB pRootDirCcb = NULL;
    PFILE_LINK_INFORMATION pFileInfo = NULL;
    IRP_ARGS_QUERY_SET_INFORMATION Args = pIrpContext->pIrp->Args.QuerySetInformation;
    PSTR pszNewFilename = NULL;
    PSTR pszNewPathname = NULL;
    PSTR pszStreamName = NULL;
    PPVFS_FILE_NAME newFilename = NULL;
    PPVFS_FILE_NAME newFileDirname = NULL;
    PPVFS_FILE_NAME newFileBasename = NULL;
    PPVFS_FILE_NAME newFileDiskDirname = NULL;
    PPVFS_FILE_NAME newDiskPathname = NULL;
    PPVFS_FILE_NAME canonicalNewPathname = NULL;
    PPVFS_FILE_NAME targetFilename = NULL;
    ACCESS_MASK dirGranted = 0;
    PVFS_STAT stat = { 0 };

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);

    pFileInfo = (PFILE_LINK_INFORMATION)Args.FileInformation;

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (PVFS_IS_DIR(pCcb)) {
        ntError = STATUS_FILE_IS_A_DIRECTORY;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (!PvfsIsDefaultStream(pCcb->pScb))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (pFileInfo->RootDirectory)
    {
        ntError = PvfsAcquireCCB(pFileInfo->RootDirectory, &pRootDirCcb);
        BAIL_ON_NT_STATUS(ntError);

        if (!PVFS_IS_DIR(pRootDirCcb)) {
            ntError = STATUS_NOT_A_DIRECTORY;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

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

    ntError = PvfsAllocateFileNameFromCString(&newFilename, pszNewPathname, 0);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSplitFileNamePath(&newFileDirname,
                                    &newFileBasename,
                                    newFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath2(&newFileDiskDirname, &stat, newFileDirname, FALSE);
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ntError = STATUS_OBJECT_PATH_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckFile(
                  pCcb->pUserToken,
                  newFileDiskDirname,
                  FILE_ADD_FILE,
                  &dirGranted);
    BAIL_ON_NT_STATUS(ntError);

    /* Real work starts here */

    ntError = PvfsAppendFileName(
                  &newDiskPathname,
                  newFileDiskDirname,
                  newFileBasename);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for an existing file */
    if (pFileInfo->ReplaceIfExists)
    {
        ntError = PvfsLookupPath2(
                      &canonicalNewPathname,
                      &stat,
                      newDiskPathname,
                      FALSE);
        if (ntError == STATUS_SUCCESS)
        {
            // TODO - finish the implementation
            ntError = STATUS_NOT_IMPLEMENTED;
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    ntError = PvfsAllocateFileNameFromCString(
                    &targetFilename,
                    pCcb->pScb->pOwnerFcb->pszFilename,
                    0);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysLinkByFileName(targetFilename, newDiskPathname);
    BAIL_ON_NT_STATUS(ntError);

    pIrp->IoStatusBlock.BytesTransferred = sizeof(*pFileInfo);
    ntError = STATUS_SUCCESS;

    // TODO - how to get the destination directory ccb (pDstCcb)? or scb?
//    ntError = PvfsGetBasicStreamname(&streamName, pDstCcb->pScb);
//    if (ntError == STATUS_SUCCESS)
//    {
//        PvfsNotifyScheduleFullReport(
//            pDstCcb->pScb->pOwnerFcb,
//            FILE_NOTIFY_CHANGE_FILE_NAME,
//            FILE_ACTION_ADDED,
//            streamName);
//    }

error:
    // PVFS_FILE_NAME
    if (newFilename)
    {
        PvfsFreeFileName(newFilename);
    }

    if (newFileDirname)
    {
        PvfsFreeFileName(newFileDirname);
    }

    if (newFileBasename)
    {
        PvfsFreeFileName(newFileBasename);
    }

    if (newFileDiskDirname)
    {
        PvfsFreeFileName(newFileDiskDirname);
    }

    if (newDiskPathname)
    {
        PvfsFreeFileName(newDiskPathname);
    }

    if (canonicalNewPathname)
    {
        PvfsFreeFileName(canonicalNewPathname);
    }

    if (targetFilename)
    {
        PvfsFreeFileName(targetFilename);
    }

    LwRtlCStringFree(&pszNewFilename);
    LwRtlCStringFree(&pszNewPathname);
    LwRtlCStringFree(&pszStreamName);

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
