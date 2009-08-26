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
 * Authors: Gerald Carter <gcarter@likewise.com>
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
PvfsCreateFileOpenOrOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
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

static NTSTATUS
PvfsCreateFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IRP_ARGS_CREATE Args = pIrpContext->pIrp->Args.Create;
    PSTR pszDirectory = NULL;
    PSTR pszDirname = NULL;
    PSTR pszRelativeFilename = NULL;
    PSTR pszDiskDirname = NULL;
    PPVFS_PENDING_CREATE pCreateCtx = NULL;

    /* Caller had to have asked for DELETE access */

    if (!(Args.DesiredAccess & DELETE)) {
        ntError = STATUS_CANNOT_DELETE;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAllocateCreateContext(&pCreateCtx, pIrpContext);
    BAIL_ON_NT_STATUS(ntError);

    /* Deal with the pathname */

    ntError = PvfsFileSplitPath(
                  &pszDirname,
                  &pszRelativeFilename,
                  pCreateCtx->pszOriginalFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath(&pszDiskDirname, pszDirname, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for file existence.  Remove it if necessary */

    ntError = PvfsLookupFile(
                  &pCreateCtx->pszDiskFilename,
                  pszDiskDirname,
                  pszRelativeFilename,
                  FALSE);
    pCreateCtx->bFileExisted = TRUE;
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        pCreateCtx->bFileExisted = FALSE;
        ntError = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntError);

    if (pCreateCtx->bFileExisted)
    {
        ntError = PvfsAccessCheckFile(
                      pCreateCtx->pCcb->pUserToken,
                      pCreateCtx->pszDiskFilename,
                      Args.DesiredAccess,
                      &pCreateCtx->GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsCheckReadOnlyDeleteOnClose(
                      Args,
                      pCreateCtx->pszDiskFilename);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsCheckShareMode(
                      pCreateCtx->pszDiskFilename,
                      Args.ShareAccess,
                      pCreateCtx->GrantedAccess,
                      &pCreateCtx->pFcb);
        BAIL_ON_NT_STATUS(ntError);

        /* Finally remove the file */

        ntError = PvfsSysRemove(pCreateCtx->pszDiskFilename);
        BAIL_ON_NT_STATUS(ntError);

        /* Seems like this should clear the FCB from
           the open table.  Not sure */

        PvfsReleaseFCB(pCreateCtx->pFcb);
        pCreateCtx->pFcb = NULL;

        RtlCStringFree(&pCreateCtx->pszDiskFilename);
    }

    ntError = RtlCStringAllocatePrintf(
                  &pCreateCtx->pszDiskFilename,
                  "%s/%s",
                  pszDiskDirname,
                  pszRelativeFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckDir(
                  pCreateCtx->pCcb->pUserToken,
                  pszDirectory,
                  Args.DesiredAccess,
                  &pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->GrantedAccess = Args.DesiredAccess;
    RtlMapGenericMask(&pCreateCtx->GrantedAccess, &gPvfsFileGenericMapping);

    ntError = PvfsCheckReadOnlyDeleteOnClose(Args, NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* This should get us a new FCB */

    ntError = PvfsCheckShareMode(
                  pCreateCtx->pszDiskFilename,
                  Args.ShareAccess,
                  pCreateCtx->GrantedAccess,
                  &pCreateCtx->pFcb);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|PVFS_SET_PROP_ATTRIB;

    ntError = STATUS_SUCCESS;
    if (pCreateCtx->bFileExisted &&
        (pCreateCtx->GrantedAccess &
         ~(FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE)))
    {
        ntError = PvfsOplockBreakIfLocked(
                      pCreateCtx->pIrpContext,
                      pCreateCtx->pCcb,
                      pCreateCtx->pFcb);
    }

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pCreateCtx->pFcb,
                      pIrpContext,
                      pCreateCtx->pCcb,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_SUCCESS) {
            pCreateCtx = NULL;
            ntError = STATUS_PENDING;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pCreateCtx->pFcb,
                      pIrpContext,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_SUCCESS) {
            pCreateCtx = NULL;
            ntError = STATUS_PENDING;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    RtlCStringFree(&pszDirname);
    RtlCStringFree(&pszRelativeFilename);
    RtlCStringFree(&pszDiskDirname);

    return ntError;

error:
    if (ntError != STATUS_PENDING)
    {
        pIrpContext->pIrp->IoStatusBlock.CreateResult =
            PvfsSetCreateResult(
                Args.CreateDisposition,
                pCreateCtx->bFileExisted,
                ntError);
    }

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsCreateFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IRP_ARGS_CREATE Args = pIrpContext->pIrp->Args.Create;
    PSTR pszDirname = NULL;
    PSTR pszRelativeFilename = NULL;
    PSTR pszDiskDirname = NULL;
    PPVFS_PENDING_CREATE pCreateCtx = NULL;

    ntError = PvfsAllocateCreateContext(&pCreateCtx, pIrpContext);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFileSplitPath(
                  &pszDirname,
                  &pszRelativeFilename,
                  pCreateCtx->pszOriginalFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath(&pszDiskDirname, pszDirname, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupFile(
                  &pCreateCtx->pszDiskFilename,
                  pszDiskDirname,
                  pszRelativeFilename,
                  FALSE);
    if (ntError == STATUS_SUCCESS) {
        ntError = STATUS_OBJECT_NAME_COLLISION;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = RtlCStringAllocatePrintf(
                  &pCreateCtx->pszDiskFilename,
                  "%s/%s",
                  pszDiskDirname,
                  pszRelativeFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckDir(
                  pCreateCtx->pCcb->pUserToken,
                  pszDiskDirname,
                  FILE_ADD_FILE,
                  &pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->GrantedAccess = Args.DesiredAccess;
    RtlMapGenericMask(&pCreateCtx->GrantedAccess, &gPvfsFileGenericMapping);

    ntError = PvfsCheckReadOnlyDeleteOnClose(Args, NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* Need to go ahead and create a share mode entry */

    ntError = PvfsCheckShareMode(
                  pCreateCtx->pszDiskFilename,
                  Args.ShareAccess,
                  pCreateCtx->GrantedAccess,
                  &pCreateCtx->pFcb);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|PVFS_SET_PROP_ATTRIB;

    /* Can be no oplock break here since the file does not exist yet */

    ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
    BAIL_ON_NT_STATUS(ntError);


cleanup:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    RtlCStringFree(&pszDirname);
    RtlCStringFree(&pszRelativeFilename);
    RtlCStringFree(&pszDiskDirname);

    return ntError;

error:
    if (ntError != STATUS_PENDING)
    {
        pIrpContext->pIrp->IoStatusBlock.CreateResult =
            PvfsSetCreateResult(
                Args.CreateDisposition,
                pCreateCtx->bFileExisted,
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

    ntError = PvfsAllocateCreateContext(&pCreateCtx, pIrpContext);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath(
                  &pCreateCtx->pszDiskFilename,
                  pCreateCtx->pszOriginalFilename,
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckFile(
                  pCreateCtx->pCcb->pUserToken,
                  pCreateCtx->pszDiskFilename,
                  Args.DesiredAccess,
                  &pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCheckReadOnlyDeleteOnClose(
                  Args,
                  pCreateCtx->pszDiskFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCheckShareMode(
                  pCreateCtx->pszDiskFilename,
                  Args.ShareAccess,
                  pCreateCtx->GrantedAccess,
                  &pCreateCtx->pFcb);
    BAIL_ON_NT_STATUS(ntError);

    if (Disposition == FILE_OVERWRITE) {
        pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|
                                       PVFS_SET_PROP_ATTRIB;
    }

    ntError = STATUS_SUCCESS;
    if (pCreateCtx->GrantedAccess &
        ~(FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE))
    {
        ntError = PvfsOplockBreakIfLocked(
                      pCreateCtx->pIrpContext,
                      pCreateCtx->pCcb,
                      pCreateCtx->pFcb);
    }

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pCreateCtx->pFcb,
                      pIrpContext,
                      pCreateCtx->pCcb,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_SUCCESS) {
            pCreateCtx = NULL;
            ntError = STATUS_PENDING;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pCreateCtx->pFcb,
                      pIrpContext,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_SUCCESS) {
            pCreateCtx = NULL;
            ntError = STATUS_PENDING;
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
                pCreateCtx->bFileExisted,
                ntError);
    }

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsCreateFileOpenOrOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    IRP_ARGS_CREATE Args = pIrpContext->pIrp->Args.Create;
    PSTR pszDirname = NULL;
    PSTR pszRelativeFilename = NULL;
    PSTR pszDiskDirname = NULL;
    PPVFS_PENDING_CREATE pCreateCtx = NULL;

    ntError = PvfsAllocateCreateContext(&pCreateCtx, pIrpContext);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFileSplitPath(
                  &pszDirname,
                  &pszRelativeFilename,
                  pCreateCtx->pszOriginalFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath(&pszDiskDirname, pszDirname, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for file existence */

    ntError = PvfsLookupFile(
                  &pCreateCtx->pszDiskFilename,
                  pszDiskDirname,
                  pszRelativeFilename,
                  FALSE);
    pCreateCtx->bFileExisted = TRUE;
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        pCreateCtx->bFileExisted = FALSE;
        ntError = STATUS_SUCCESS;
    }
    BAIL_ON_NT_STATUS(ntError);

    if (pCreateCtx->bFileExisted)
    {
        ntError = PvfsAccessCheckFile(
                      pCreateCtx->pCcb->pUserToken,
                      pCreateCtx->pszDiskFilename,
                      Args.DesiredAccess,
                      &pCreateCtx->GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        ntError = PvfsAccessCheckDir(
                      pCreateCtx->pCcb->pUserToken,
                      pszDiskDirname,
                      FILE_ADD_FILE,
                      &pCreateCtx->GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);

        pCreateCtx->GrantedAccess = Args.DesiredAccess;
        RtlMapGenericMask(
            &pCreateCtx->GrantedAccess,
            &gPvfsFileGenericMapping);


        ntError = RtlCStringAllocatePrintf(
                      &pCreateCtx->pszDiskFilename,
                      "%s/%s",
                      pszDiskDirname,
                      pszRelativeFilename);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsCheckReadOnlyDeleteOnClose(
                  Args,
                  pCreateCtx->bFileExisted ? pCreateCtx->pszDiskFilename : NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCheckShareMode(
                  pCreateCtx->pszDiskFilename,
                  Args.ShareAccess,
                  pCreateCtx->GrantedAccess,
                  &pCreateCtx->pFcb);
    BAIL_ON_NT_STATUS(ntError);

    if (!pCreateCtx->bFileExisted ||
        (Args.CreateDisposition == FILE_OVERWRITE_IF))
    {
        pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|
                                       PVFS_SET_PROP_ATTRIB;
    }

    ntError = STATUS_SUCCESS;
    if (pCreateCtx->bFileExisted &&
        (pCreateCtx->GrantedAccess &
         ~(FILE_READ_ATTRIBUTES|FILE_WRITE_ATTRIBUTES|SYNCHRONIZE)))
    {
        ntError = PvfsOplockBreakIfLocked(
                      pCreateCtx->pIrpContext,
                      pCreateCtx->pCcb,
                      pCreateCtx->pFcb);
    }

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
        break;

    case STATUS_OPLOCK_BREAK_IN_PROGRESS:
        ntError = PvfsPendOplockBreakTest(
                      pCreateCtx->pFcb,
                      pIrpContext,
                      pCreateCtx->pCcb,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_SUCCESS) {
            pCreateCtx = NULL;
            ntError = STATUS_PENDING;
        }
        break;

    case STATUS_PENDING:
        ntError = PvfsAddItemPendingOplockBreakAck(
                      pCreateCtx->pFcb,
                      pIrpContext,
                      PvfsCreateFileDoSysOpen,
                      PvfsFreeCreateContext,
                      (PVOID)pCreateCtx);
        if (ntError == STATUS_SUCCESS) {
            pCreateCtx = NULL;
            ntError = STATUS_PENDING;
        }
        break;
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    RtlCStringFree(&pszDirname);
    RtlCStringFree(&pszRelativeFilename);
    RtlCStringFree(&pszDiskDirname);

    return ntError;

error:
    if (ntError != STATUS_PENDING)
    {
        pIrpContext->pIrp->IoStatusBlock.CreateResult =
            PvfsSetCreateResult(
                Args.CreateDisposition,
                pCreateCtx->bFileExisted,
                ntError);
    }

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
