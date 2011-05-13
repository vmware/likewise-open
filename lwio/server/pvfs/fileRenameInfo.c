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

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsSetFileRenameInfoWithContext(
    PVOID pContext
    );

static
NTSTATUS
PvfsCreateSetFileRenameInfoContext(
    OUT PPVFS_PENDING_SET_FILE_RENAME *ppSetRenameInfoContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_FILE_NAME newResolvedFileName
    );

static
VOID
PvfsFreeSetFileRenameInfoContext(
    IN OUT PVOID *ppContext
    );


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
    IRP_ARGS_QUERY_SET_INFORMATION Args = {0};
    PFILE_RENAME_INFORMATION pFileInfo = NULL;
    PPVFS_PENDING_SET_FILE_RENAME pSetRenameInfoCtx = NULL;
    BOOLEAN bIsDefaultStream = TRUE;
    PPVFS_CCB pRootDirCcb = NULL;
    ACCESS_MASK DirGranted = 0;
    ACCESS_MASK DirDesired = 0;
    PVFS_STAT stat = { 0 };
    PSTR pszNewFilename = NULL;
    PPVFS_FILE_NAME inputNewFilename = NULL;
    PCSTR pszFilename = NULL;
    PSTR pszNewPathname = NULL;

    PPVFS_FILE_NAME newFileName = NULL;
    PPVFS_FILE_NAME newDirectoryName = NULL;
    PPVFS_FILE_NAME newResolvedDirName = NULL;
    PPVFS_FILE_NAME newRelativeFileName = NULL;
    PPVFS_FILE_NAME newResolvedFileName = NULL;
    PPVFS_FILE_NAME existingResolvedFileName = NULL;
    PPVFS_FILE_NAME currentFileName = NULL;

    Args = pIrpContext->pIrp->Args.QuerySetInformation;

    /* Sanity checks */

    BAIL_ON_INVALID_PTR(Args.FileInformation, ntError);
    pFileInfo = (PFILE_RENAME_INFORMATION)Args.FileInformation;

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    bIsDefaultStream = PvfsIsDefaultStream(pCcb->pScb);

    ntError = PvfsWC16CanonicalPathName(
                    &pszNewFilename,
                    pFileInfo->FileName,
                    pFileInfo->FileNameLength);
    BAIL_ON_NT_STATUS(ntError);

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

    ntError = PvfsAllocateFileNameFromCString(&inputNewFilename, pszNewFilename, 0);
    BAIL_ON_NT_STATUS(ntError);
    pszFilename = PvfsGetCStringBaseFileName(inputNewFilename);

    /* Make sure we can add the new object to the parent directory
       (not necessarily the same as the RootDirectory handle) */

    if (bIsDefaultStream ||
        (!bIsDefaultStream && pszFilename && *pszFilename))
    {
        if (pRootDirCcb)
        {
            pszNewPathname = NULL;

            /* Check if we need to separate the root path from
            the relative path with a '/' */

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
    }
    else
    {
        // make up newstream full path using pOwnerFcb and the streamname info
        // pszNewFilanem has a leading ':'
        ntError = RtlCStringAllocatePrintf(
                      &pszNewPathname,
                      "%s%s",
                      pCcb->pScb->pOwnerFcb->pszFilename,
                      pszNewFilename);
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

    ntError = PvfsCreateSetFileRenameInfoContext(
                  &pSetRenameInfoCtx,
                  pIrpContext,
                  pCcb,
                  newResolvedFileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsOplockBreakIfLocked(pIrpContext, pCcb, pCcb->pScb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsSetFileRenameInfoWithContext(pSetRenameInfoCtx);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pSetRenameInfoCtx->pCcb->pScb,
                      pIrpContext,
                      pSetRenameInfoCtx->pCcb,
                      PvfsSetFileRenameInfoWithContext,
                      PvfsFreeSetFileRenameInfoContext,
                      (PVOID)pSetRenameInfoCtx);
        if (ntError == STATUS_PENDING)
        {
            pSetRenameInfoCtx = NULL;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pSetRenameInfoCtx->pCcb->pScb,
                      pIrpContext,
                      PvfsSetFileRenameInfoWithContext,
                      PvfsFreeSetFileRenameInfoContext,
                      (PVOID)pSetRenameInfoCtx);
        if (ntError == STATUS_PENDING)
        {
            pSetRenameInfoCtx = NULL;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeSetFileRenameInfoContext(OUT_PPVOID(&pSetRenameInfoCtx));

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
    if (inputNewFilename)
    {
        PvfsFreeFileName(inputNewFilename);
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

    // CCB
    if (pRootDirCcb)
    {
        PvfsReleaseCCB(pRootDirCcb);
    }

    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;

}

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsSetFileRenameInfoWithContext(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_SET_FILE_RENAME pSetFileRenameCtx = (PPVFS_PENDING_SET_FILE_RENAME)pContext;
    PIRP pIrp = pSetFileRenameCtx->pIrpContext->pIrp;
    PPVFS_CCB pCcb = pSetFileRenameCtx->pCcb;
    PPVFS_FILE_NAME newResolvedFileName = pSetFileRenameCtx->newResolvedFileName;
    IRP_ARGS_QUERY_SET_INFORMATION Args = {0};
    PFILE_RENAME_INFORMATION pFileInfo = NULL;
    PSTR streamName = NULL;


    Args = pSetFileRenameCtx->pIrpContext->pIrp->Args.QuerySetInformation;
    pFileInfo = (PFILE_RENAME_INFORMATION)Args.FileInformation;

    if (Args.Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
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
    if (streamName)
    {
        LwRtlCStringFree(&streamName);
    }

    return ntError;
}

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsCreateSetFileRenameInfoContext(
    OUT PPVFS_PENDING_SET_FILE_RENAME *ppSetRenameInfoContext,
    IN  PPVFS_IRP_CONTEXT pIrpContext,
    IN  PPVFS_CCB pCcb,
    IN  PPVFS_FILE_NAME newResolvedFileName
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_SET_FILE_RENAME pSetRenameInfoCtx = NULL;

    ntError = PvfsAllocateMemory(
		      OUT_PPVOID(&pSetRenameInfoCtx),
                  sizeof(PVFS_PENDING_SET_FILE_RENAME),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->SetFileType = PVFS_SET_FILE_RENAME;
    pSetRenameInfoCtx->pIrpContext = PvfsReferenceIrpContext(pIrpContext);
    pSetRenameInfoCtx->pCcb = PvfsReferenceCCB(pCcb);
    pSetRenameInfoCtx->newResolvedFileName = newResolvedFileName;

    *ppSetRenameInfoContext = pSetRenameInfoCtx;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsFreeSetFileRenameInfoContext(
    IN OUT PVOID *ppContext
    )
{
    PPVFS_PENDING_SET_FILE_RENAME pRenameInfoCtx = NULL;

    if (ppContext && *ppContext)
    {
        pRenameInfoCtx = (PPVFS_PENDING_SET_FILE_RENAME)(*ppContext);

        if (pRenameInfoCtx->pIrpContext)
        {
            PvfsReleaseIrpContext(&pRenameInfoCtx->pIrpContext);
        }

        if (pRenameInfoCtx->pCcb)
        {
            PvfsReleaseCCB(pRenameInfoCtx->pCcb);
        }

        PVFS_FREE(ppContext);
    }

    return;
}
