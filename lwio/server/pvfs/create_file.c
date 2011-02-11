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
 *        create_file.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *       Create Dispatch Routine
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

static
NTSTATUS
PvfsCreateFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static
NTSTATUS
PvfsCreateFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext
    );

NTSTATUS
PvfsCreateFileOpenOrOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static
NTSTATUS
PvfsCreateFileOpenOrOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext
    );


/*****************************************************************************
 ****************************************************************************/

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

    case FILE_OVERWRITE:
    case FILE_OPEN:
        ntError = PvfsCreateFileOpenOrOverwrite(pIrpContext);
        break;

    case FILE_OVERWRITE_IF:
    case FILE_OPEN_IF:
        ntError = PvfsCreateFileOpenOrOverwriteIf(pIrpContext);
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

static
NTSTATUS
PvfsCreateFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IRP_ARGS_CREATE Args = pIrpContext->pIrp->Args.Create;
    PPVFS_PENDING_CREATE pCreateCtx = NULL;
    PVFS_STAT statPath = {0};
    PVFS_STAT statFile = {0};
    PPVFS_FILE_NAME directoryName = NULL;
    PPVFS_FILE_NAME resolvedDirName = NULL;
    PPVFS_FILE_NAME relativeName = NULL;

    ntError = PvfsAllocateCreateContext(&pCreateCtx, pIrpContext);
    BAIL_ON_NT_STATUS(ntError);

    /* Deal with the pathname */

    ntError = PvfsSplitFileNamePath(
                  &directoryName,
                  &relativeName,
                  pCreateCtx->OriginalFileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath2(
                  &resolvedDirName,
                  &statPath,
                  directoryName,
                  Args.FileName.IoNameOptions & IO_NAME_OPTION_CASE_SENSITIVE);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for file existence.  Remove it if necessary */

    ntError = PvfsLookupFile2(
                  &pCreateCtx->ResolvedFileName,
                  &statFile,
                  resolvedDirName,
                  relativeName,
                  Args.FileName.IoNameOptions & IO_NAME_OPTION_CASE_SENSITIVE);
    pCreateCtx->bFileExisted = TRUE;
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        pCreateCtx->bFileExisted = FALSE;
        ntError = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntError);

    if (pCreateCtx->bFileExisted)
    {
        if (S_ISDIR(statFile.s_mode))
        {
            ntError = STATUS_FILE_IS_A_DIRECTORY;
            BAIL_ON_NT_STATUS(ntError);
        }

        // The caller must have DELETE permission on the file even if not
        // specifically requested

        ntError = PvfsAccessCheckFile(
                      pCreateCtx->pCcb->pUserToken,
                      pCreateCtx->ResolvedFileName,
                      Args.DesiredAccess | DELETE,
                      &pCreateCtx->GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsCheckDosAttributes(
                      Args,
                      pCreateCtx->ResolvedFileName,
                      pCreateCtx->GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsCheckShareMode(
                      pCreateCtx->ResolvedFileName,
                      Args.ShareAccess,
                      pCreateCtx->GrantedAccess,
                      &pCreateCtx->pScb);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsCreateFileCheckPendingDelete(pCreateCtx->pScb);
        BAIL_ON_NT_STATUS(ntError);

        /* Finally remove the file */

        ntError = PvfsSysRemoveByFileName(pCreateCtx->ResolvedFileName);
        BAIL_ON_NT_STATUS(ntError);

        /* Seems like this should clear the SCB from
           the open table.  Not sure */

        PvfsReleaseSCB(&pCreateCtx->pScb);

        PvfsFreeFileName(pCreateCtx->ResolvedFileName);
        pCreateCtx->ResolvedFileName = NULL;
    }

    ntError = PvfsAppendFileName(
                  &pCreateCtx->ResolvedFileName,
                  resolvedDirName,
                  relativeName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckFile(
                  pCreateCtx->pCcb->pUserToken,
                  resolvedDirName,
                  Args.DesiredAccess,
                  &pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->GrantedAccess = PvfsGetGrantedAccessForNewObject(
                                        Args.DesiredAccess);

    ntError = PvfsCheckDosAttributes(
                  Args,
                  NULL,  /* New File */
                  pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_SECURITY|PVFS_SET_PROP_ATTRIB;

    /* This should get us a new SCB */

    ntError = PvfsCheckShareMode(
                  pCreateCtx->ResolvedFileName,
                  Args.ShareAccess,
                  pCreateCtx->GrantedAccess,
                  &pCreateCtx->pScb);
    if ((ntError != STATUS_SUCCESS) &&
        (ntError != STATUS_SHARING_VIOLATION))
    {
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsCreateFileCheckPendingDelete(pCreateCtx->pScb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = STATUS_SUCCESS;
    if (pCreateCtx->bFileExisted &&
        (pCreateCtx->GrantedAccess &
         ~(FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE)))
    {
        ntError = PvfsOplockBreakIfLocked(
                      pCreateCtx->pIrpContext,
                      pCreateCtx->pCcb,
                      pCreateCtx->pScb);
    }

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pCreateCtx->pScb,
                      pIrpContext,
                      pCreateCtx->pCcb,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_PENDING)
        {
            pCreateCtx = NULL;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pCreateCtx->pScb,
                      pIrpContext,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_PENDING)
        {
            pCreateCtx = NULL;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    if (directoryName)
    {
        PvfsFreeFileName(directoryName);
    }
    if (relativeName)
    {
        PvfsFreeFileName(relativeName);
    }
    if (resolvedDirName)
    {
        PvfsFreeFileName(resolvedDirName);
    }

    return ntError;

error:
    if (ntError != STATUS_PENDING)
    {
        pIrpContext->pIrp->IoStatusBlock.CreateResult =
            PvfsSetCreateResult(
                Args.CreateDisposition,
                pCreateCtx ? pCreateCtx->bFileExisted : FALSE,
                ntError);
    }

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsCreateFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IRP_ARGS_CREATE Args = pIrpContext->pIrp->Args.Create;
    PPVFS_PENDING_CREATE pCreateCtx = NULL;
    PVFS_STAT Stat = {0};
    PPVFS_FILE_NAME directoryName = NULL;
    PPVFS_FILE_NAME relativeName = NULL;
    PPVFS_FILE_NAME resolvedDirName = NULL;

    ntError = PvfsAllocateCreateContext(&pCreateCtx, pIrpContext);
    BAIL_ON_NT_STATUS(ntError);

    /* We expect this call to fail with OBJECT_NAME_NOT_FOUND */

    ntError = PvfsLookupPath2(
                  &pCreateCtx->ResolvedFileName,
                  &Stat,
                  pCreateCtx->OriginalFileName,
                  Args.FileName.IoNameOptions & IO_NAME_OPTION_CASE_SENSITIVE);
    switch (ntError)
    {
        case STATUS_SUCCESS:
            if (S_ISDIR(Stat.s_mode))
            {
                ntError = STATUS_FILE_IS_A_DIRECTORY;
            }
            else
            {
                ntError = STATUS_OBJECT_NAME_COLLISION;
            }
            break;

        case STATUS_OBJECT_NAME_NOT_FOUND:
            ntError = STATUS_SUCCESS;
            break;

        default:
            /* do nothing */
            break;
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSplitFileNamePath(
                  &directoryName,
                  &relativeName,
                  pCreateCtx->OriginalFileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath2(
                  &resolvedDirName,
                  &Stat,
                  directoryName,
                  Args.FileName.IoNameOptions & IO_NAME_OPTION_CASE_SENSITIVE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAppendFileName(
                  &pCreateCtx->ResolvedFileName,
                  resolvedDirName,
                  relativeName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckFile(
                  pCreateCtx->pCcb->pUserToken,
                  resolvedDirName,
                  FILE_ADD_FILE,
                  &pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->GrantedAccess = PvfsGetGrantedAccessForNewObject(
                                        Args.DesiredAccess);

    ntError = PvfsCheckDosAttributes(
                  Args,
                  NULL,  /* New File */
                  pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    /* Need to go ahead and create a share mode entry */

    ntError = PvfsCheckShareMode(
                  pCreateCtx->ResolvedFileName,
                  Args.ShareAccess,
                  pCreateCtx->GrantedAccess,
                  &pCreateCtx->pScb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCreateFileCheckPendingDelete(pCreateCtx->pScb);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_SECURITY|PVFS_SET_PROP_ATTRIB;

    /* Can be no oplock break here since the file does not exist yet */

    ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
    BAIL_ON_NT_STATUS(ntError);


cleanup:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    if (directoryName)
    {
        PvfsFreeFileName(directoryName);
    }
    if (relativeName)
    {
        PvfsFreeFileName(relativeName);
    }
    if (resolvedDirName)
    {
        PvfsFreeFileName(resolvedDirName);
    }

    return ntError;

error:
    if (ntError != STATUS_PENDING)
    {
        pIrpContext->pIrp->IoStatusBlock.CreateResult =
            PvfsSetCreateResult(
                Args.CreateDisposition,
                pCreateCtx ? pCreateCtx->bFileExisted : FALSE,
                ntError);
    }

    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCreateFileOpenOrOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IRP_ARGS_CREATE Args = pIrpContext->pIrp->Args.Create;
    FILE_CREATE_DISPOSITION Disposition = Args.CreateDisposition;;
    PPVFS_PENDING_CREATE pCreateCtx = NULL;
    PVFS_STAT Stat = {0};

    ntError = PvfsAllocateCreateContext(&pCreateCtx, pIrpContext);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath2(
                  &pCreateCtx->ResolvedFileName,
                  &Stat,
                  pCreateCtx->OriginalFileName,
                  Args.FileName.IoNameOptions & IO_NAME_OPTION_CASE_SENSITIVE);
    BAIL_ON_NT_STATUS(ntError);

    if (S_ISDIR(Stat.s_mode))
    {
        ntError = STATUS_FILE_IS_A_DIRECTORY;
        BAIL_ON_NT_STATUS(ntError);
    }

    pCreateCtx->bFileExisted = TRUE;

    if (Disposition == FILE_OVERWRITE)
    {
        pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_ATTRIB;
    }

    ntError = PvfsAccessCheckFile(
                  pCreateCtx->pCcb->pUserToken,
                  pCreateCtx->ResolvedFileName,
                  Args.DesiredAccess,
                  &pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCheckDosAttributes(
                  Args,
                  pCreateCtx->ResolvedFileName,
                  pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCheckShareMode(
                  pCreateCtx->ResolvedFileName,
                  Args.ShareAccess,
                  pCreateCtx->GrantedAccess,
                  &pCreateCtx->pScb);
    if ((ntError != STATUS_SUCCESS) &&
        (ntError != STATUS_SHARING_VIOLATION))
    {
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsCreateFileCheckPendingDelete(pCreateCtx->pScb);
    BAIL_ON_NT_STATUS(ntError);

    /* We can only potentially force an oplock break IFF
       (a) the file already existed,
       (b) and one of the following is true
           i.  Disposition == FILE_OPEN_IF && permissions more than
               (FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE)
           ii. Disposition == FILE_OVERWRITE_IF
    */

    if (pCreateCtx->bFileExisted &&
        (((Args.CreateDisposition == FILE_OPEN) &&
         (pCreateCtx->GrantedAccess &
          ~(FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE))) ||
         (Args.CreateDisposition == FILE_OVERWRITE)))
    {
        ntError = PvfsOplockBreakIfLocked(
                      pCreateCtx->pIrpContext,
                      pCreateCtx->pCcb,
                      pCreateCtx->pScb);
    }

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pCreateCtx->pScb,
                      pIrpContext,
                      pCreateCtx->pCcb,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_PENDING)
        {
            pCreateCtx = NULL;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pCreateCtx->pScb,
                      pIrpContext,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_PENDING)
        {
            pCreateCtx = NULL;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    return ntError;

error:
    if (ntError != STATUS_PENDING)
    {
        pIrpContext->pIrp->IoStatusBlock.CreateResult =
            PvfsSetCreateResult(
                Args.CreateDisposition,
                pCreateCtx ? pCreateCtx->bFileExisted : FALSE,
                ntError);
    }

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsCreateFileOpenOrOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IRP_ARGS_CREATE Args = pIrpContext->pIrp->Args.Create;
    PPVFS_PENDING_CREATE pCreateCtx = NULL;
    PVFS_STAT statPath = {0};
    PVFS_STAT statFile = {0};
    PPVFS_FILE_NAME directoryName = NULL;
    PPVFS_FILE_NAME resolvedDirName = NULL;
    PPVFS_FILE_NAME relativeFileName = NULL;

    ntError = PvfsAllocateCreateContext(&pCreateCtx, pIrpContext);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSplitFileNamePath(
                  &directoryName,
                  &relativeFileName,
                  pCreateCtx->OriginalFileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath2(
                  &resolvedDirName,
                  &statPath,
                  directoryName,
                  Args.FileName.IoNameOptions & IO_NAME_OPTION_CASE_SENSITIVE);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for file existence */

    ntError = PvfsLookupFile2(
                  &pCreateCtx->ResolvedFileName,
                  &statFile,
                  resolvedDirName,
                  relativeFileName,
                  Args.FileName.IoNameOptions & IO_NAME_OPTION_CASE_SENSITIVE);
    pCreateCtx->bFileExisted = TRUE;
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        pCreateCtx->bFileExisted = FALSE;
        ntError = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntError);

    if (pCreateCtx->bFileExisted)
    {
        if (S_ISDIR(statFile.s_mode))
        {
            ntError = STATUS_FILE_IS_A_DIRECTORY;
            BAIL_ON_NT_STATUS(ntError);
        }

        ntError = PvfsAccessCheckFile(
                      pCreateCtx->pCcb->pUserToken,
                      pCreateCtx->ResolvedFileName,
                      Args.DesiredAccess,
                      &pCreateCtx->GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        ntError = PvfsAccessCheckFile(
                      pCreateCtx->pCcb->pUserToken,
                      resolvedDirName,
                      FILE_ADD_FILE,
                      &pCreateCtx->GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);

        pCreateCtx->GrantedAccess = PvfsGetGrantedAccessForNewObject(
                                            Args.DesiredAccess);

        ntError = PvfsAppendFileName(
                      &pCreateCtx->ResolvedFileName,
                      resolvedDirName,
                      relativeFileName);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsCheckDosAttributes(
                  Args,
                  pCreateCtx->bFileExisted ? pCreateCtx->ResolvedFileName : NULL,
                  pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    if (!pCreateCtx->bFileExisted)
    {
        pCreateCtx->SetPropertyFlags = (PVFS_SET_PROP_ATTRIB|
                                        PVFS_SET_PROP_SECURITY);
    }
    else if (Args.CreateDisposition == FILE_OVERWRITE_IF)
    {
        pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_ATTRIB;
    }

    ntError = PvfsCheckShareMode(
                  pCreateCtx->ResolvedFileName,
                  Args.ShareAccess,
                  pCreateCtx->GrantedAccess,
                  &pCreateCtx->pScb);
    if ((ntError != STATUS_SUCCESS) &&
        (ntError != STATUS_SHARING_VIOLATION))
    {
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsCreateFileCheckPendingDelete(pCreateCtx->pScb);
    BAIL_ON_NT_STATUS(ntError);

    /* We can only potentially force an oplock break IFF
       (a) the file already existed,
       (b) and one of the following is true
           i.  Disposition == FILE_OPEN_IF && permissions more than
               (FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE)
           ii. Disposition == FILE_OVERWRITE_IF
    */

    if (pCreateCtx->bFileExisted &&
        (((Args.CreateDisposition == FILE_OPEN_IF) &&
         (pCreateCtx->GrantedAccess &
          ~(FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE))) ||
         (Args.CreateDisposition == FILE_OVERWRITE_IF)))
    {
        ntError = PvfsOplockBreakIfLocked(
                      pCreateCtx->pIrpContext,
                      pCreateCtx->pCcb,
                      pCreateCtx->pScb);
    }

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pCreateCtx->pScb,
                      pIrpContext,
                      pCreateCtx->pCcb,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_PENDING)
        {
            pCreateCtx = NULL;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pCreateCtx->pScb,
                      pIrpContext,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_PENDING)
        {
            pCreateCtx = NULL;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    if (directoryName)
    {
        PvfsFreeFileName(directoryName);
    }
    if (relativeFileName)
    {
        PvfsFreeFileName(relativeFileName);
    }
    if (resolvedDirName)
    {
        PvfsFreeFileName(resolvedDirName);
    }

    return ntError;

error:
    if (ntError != STATUS_PENDING)
    {
        pIrpContext->pIrp->IoStatusBlock.CreateResult =
            PvfsSetCreateResult(
                Args.CreateDisposition,
                pCreateCtx ? pCreateCtx->bFileExisted : FALSE,
                ntError);
    }

    goto cleanup;
}


