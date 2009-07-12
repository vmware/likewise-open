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

static NTSTATUS
PvfsCreateFileDoSysOpen(
    IN PPVFS_PENDING_CREATE pCreateContext
    );

static NTSTATUS
PvfsCheckReadOnlyDeleteOnClose(
    IN IRP_ARGS_CREATE CreateArgs,
    IN PSTR pszFilename
    );

static FILE_CREATE_RESULT
PvfsSetCreateResult(
    IN FILE_CREATE_DISPOSITION Disposition,
    IN BOOLEAN bFileExisted,
    IN NTSTATUS ntStatus
    );


/* Code */


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
                      Args.DesiredAccess,
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

    pCreateCtx->GrantedAccess = FILE_ALL_ACCESS;

    ntError = PvfsCheckReadOnlyDeleteOnClose(Args, NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* This should get us a new FCB */

    ntError = PvfsCheckShareMode(
                  pCreateCtx->pszDiskFilename,
                  Args.ShareAccess,
                  Args.DesiredAccess,
                  &pCreateCtx->pFcb);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|PVFS_SET_PROP_ATTRIB;

    ntError = PvfsOplockBreakIfLocked(pCreateCtx->pFcb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
        BAIL_ON_NT_STATUS(ntError);

        PvfsFreeCreateContext(&pCreateCtx);
        break;

    case STATUS_PENDING:
        /* If we have a pending break, then add the CreateContext
           to the pending open queue on the FCB */
        break;

    default:
        BAIL_ON_NT_STATUS(ntError);
        break;
    }

cleanup:
    RtlCStringFree(&pszDirname);
    RtlCStringFree(&pszRelativeFilename);
    RtlCStringFree(&pszDiskDirname);

    return ntError;

error:
    pIrpContext->pIrp->IoStatusBlock.CreateResult =
        PvfsSetCreateResult(
            Args.CreateDisposition,
            pCreateCtx->bFileExisted,
            ntError);

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

    pCreateCtx->GrantedAccess = FILE_ALL_ACCESS;

    ntError = PvfsCheckReadOnlyDeleteOnClose(Args, NULL);
    BAIL_ON_NT_STATUS(ntError);

    /* Need to go ahead and create a share mode entry */

    ntError = PvfsCheckShareMode(
                  pCreateCtx->pszDiskFilename,
                  Args.ShareAccess,
                  Args.DesiredAccess,
                  &pCreateCtx->pFcb);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|PVFS_SET_PROP_ATTRIB;

    /* Can be no oplock break here since the file does not exist yet */

    ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
    BAIL_ON_NT_STATUS(ntError);

    PvfsFreeCreateContext(&pCreateCtx);

cleanup:
    RtlCStringFree(&pszDirname);
    RtlCStringFree(&pszRelativeFilename);
    RtlCStringFree(&pszDiskDirname);

    return ntError;

error:
    pIrpContext->pIrp->IoStatusBlock.CreateResult =
        PvfsSetCreateResult(
            Args.CreateDisposition,
            pCreateCtx->bFileExisted,
            ntError);

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
                  Args.DesiredAccess,
                  &pCreateCtx->pFcb);
    BAIL_ON_NT_STATUS(ntError);

    if (Disposition == FILE_OVERWRITE) {
        pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|
                                       PVFS_SET_PROP_ATTRIB;
    }

    ntError = PvfsOplockBreakIfLocked(pCreateCtx->pFcb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
        BAIL_ON_NT_STATUS(ntError);

        PvfsFreeCreateContext(&pCreateCtx);
        break;

    case STATUS_PENDING:
        /* If we have a pending break, then add the CreateContext
           to the pending open queue on the FCB */
        break;

    default:
        BAIL_ON_NT_STATUS(ntError);
        break;
    }

cleanup:
    return ntError;

error:
    pIrpContext->pIrp->IoStatusBlock.CreateResult =
        PvfsSetCreateResult(
            Args.CreateDisposition,
            pCreateCtx->bFileExisted,
            ntError);

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

        pCreateCtx->GrantedAccess = FILE_ALL_ACCESS;

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
                  Args.DesiredAccess,
                  &pCreateCtx->pFcb);
    BAIL_ON_NT_STATUS(ntError);

    if (!pCreateCtx->bFileExisted ||
        (Args.CreateDisposition == FILE_OVERWRITE_IF))
    {
        pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|
                                       PVFS_SET_PROP_ATTRIB;
    }

    ntError = PvfsOplockBreakIfLocked(pCreateCtx->pFcb);

    switch (ntError)
    {
    case STATUS_SUCCESS:
        ntError = PvfsCreateFileDoSysOpen(pCreateCtx);
        BAIL_ON_NT_STATUS(ntError);

        PvfsFreeCreateContext(&pCreateCtx);
        break;

    case STATUS_PENDING:
        /* If we have a pending break, then add the CreateContext
           to the pending open queue on the FCB */
        break;

    default:
        BAIL_ON_NT_STATUS(ntError);
        break;
    }

cleanup:
    RtlCStringFree(&pszDirname);
    RtlCStringFree(&pszRelativeFilename);
    RtlCStringFree(&pszDiskDirname);

    return ntError;

error:
    pIrpContext->pIrp->IoStatusBlock.CreateResult =
        PvfsSetCreateResult(
            Args.CreateDisposition,
            pCreateCtx->bFileExisted,
            ntError);

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsCreateFileDoSysOpen(
    IN PPVFS_PENDING_CREATE pCreateContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pCreateContext->pIrpContext->pIrp;
    IRP_ARGS_CREATE Args = pIrp->Args.Create;
    int fd = -1;
    int unixFlags = 0;
    PIO_CREATE_SECURITY_CONTEXT pSecCtx = Args.SecurityContext;
    FILE_CREATE_RESULT CreateResult = 0;

    /* Do the open() */

    ntError = MapPosixOpenFlags(&unixFlags, pCreateContext->GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpen(&fd, pCreateContext->pszDiskFilename, unixFlags, 0600);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCreateContext->pCcb->fd = fd;
    pCreateContext->pCcb->ShareFlags = Args.ShareAccess;
    pCreateContext->pCcb->AccessGranted = pCreateContext->GrantedAccess;
    pCreateContext->pCcb->CreateOptions = Args.CreateOptions;

    pCreateContext->pCcb->pszFilename = pCreateContext->pszDiskFilename;
    pCreateContext->pszDiskFilename = NULL;

    ntError = PvfsAddCCBToFCB(pCreateContext->pFcb, pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);
    pCreateContext->pFcb = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if ((pCreateContext->SetPropertyFlags & PVFS_SET_PROP_OWNER) && pSecCtx)
    {
        PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcess = NULL;

        pProcess = IoSecurityGetProcessInfo(pSecCtx);

        ntError = PvfsSysChown(
                      pCreateContext->pCcb,
                      pProcess->Uid,
                      pProcess->Gid);
        BAIL_ON_NT_STATUS(ntError);
    }

    if ((pCreateContext->SetPropertyFlags & PVFS_SET_PROP_ATTRIB) &&
        (Args.FileAttributes != 0))
    {
        ntError = PvfsSetFileAttributes(
                      pCreateContext->pCcb,
                      Args.FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);
    pCreateContext->pCcb = NULL;

    CreateResult = PvfsSetCreateResult(
                       Args.CreateDisposition,
                       pCreateContext->bFileExisted,
                       STATUS_SUCCESS);

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    return ntError;

error:
    CreateResult = PvfsSetCreateResult(
                       Args.CreateDisposition,
                       pCreateContext->bFileExisted,
                       ntError);

    if (fd != -1)
    {
        close(fd);
    }


    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static FILE_CREATE_RESULT
PvfsSetCreateResult(
    IN FILE_CREATE_DISPOSITION Disposition,
    IN BOOLEAN bFileExisted,
    IN NTSTATUS ntError
    )
{
    FILE_CREATE_RESULT CreateResult = 0;

    /* Handle the error case in the "error" label */
    BAIL_ON_NT_STATUS(ntError);

    switch (Disposition)
    {
    case FILE_CREATE:
    case FILE_OPEN:
    case FILE_OPEN_IF:
        CreateResult = bFileExisted ? FILE_OPENED: FILE_CREATED;
        break;
    case FILE_OVERWRITE:
    case FILE_OVERWRITE_IF:
        CreateResult = bFileExisted ? FILE_OVERWRITTEN: FILE_CREATED;
        break;
    case FILE_SUPERSEDE:
        CreateResult = bFileExisted ? FILE_SUPERSEDED: FILE_CREATED;
        break;
    }

cleanup:
    return CreateResult;

error:
    switch (Disposition)
    {
    case FILE_CREATE:
        CreateResult = (ntError == STATUS_OBJECT_NAME_COLLISION) ?
                       FILE_EXISTS : FILE_DOES_NOT_EXIST;
        break;
    case FILE_OPEN:
    case FILE_OVERWRITE:
        CreateResult = (ntError == STATUS_OBJECT_PATH_NOT_FOUND) ?
                       FILE_DOES_NOT_EXIST : FILE_EXISTS;
        break;
    case FILE_OPEN_IF:
    case FILE_OVERWRITE_IF:
        CreateResult = (ntError == STATUS_OBJECT_NAME_COLLISION) ?
                       FILE_EXISTS : FILE_DOES_NOT_EXIST;
        break;
    case FILE_SUPERSEDE:
        CreateResult = bFileExisted ? FILE_EXISTS : FILE_DOES_NOT_EXIST;
        break;
    }

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsCheckReadOnlyDeleteOnClose(
    IN IRP_ARGS_CREATE CreateArgs,
    IN PSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    FILE_ATTRIBUTES Attributes = 0;

    if (!(CreateArgs.CreateOptions & FILE_DELETE_ON_CLOSE)) {
        goto cleanup;
    }

    if (pszFilename) {
        ntError = PvfsGetFilenameAttributes(
                      pszFilename,
                      &Attributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    switch (CreateArgs.CreateDisposition)
    {
    case FILE_OPEN:
    case FILE_OPEN_IF:
    case FILE_SUPERSEDE:
    case FILE_CREATE:
        if (pszFilename)
        {
            if (Attributes & FILE_ATTRIBUTE_READONLY)
            {
                ntError = STATUS_CANNOT_DELETE;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        else
        {
            if (CreateArgs.FileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                ntError = STATUS_CANNOT_DELETE;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        break;

    case FILE_OVERWRITE:
    case FILE_OVERWRITE_IF:
        if (pszFilename && (CreateArgs.FileAttributes == 0))
        {
            if (Attributes & FILE_ATTRIBUTE_READONLY)
            {
                ntError = STATUS_CANNOT_DELETE;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        else
        {
            if (CreateArgs.FileAttributes & FILE_ATTRIBUTE_READONLY)
            {
                ntError = STATUS_CANNOT_DELETE;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        break;
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreeCreateContext(
    IN OUT PPVFS_PENDING_CREATE *ppCreate
    )
{
    PPVFS_PENDING_CREATE pCreateCtx = NULL;

    if (!ppCreate || !*ppCreate) {
        return;
    }

    pCreateCtx = (PPVFS_PENDING_CREATE)*ppCreate;


    RtlCStringFree(&pCreateCtx->pszDiskFilename);
    RtlCStringFree(&pCreateCtx->pszOriginalFilename);

    if (pCreateCtx->pCcb) {
        PvfsReleaseCCB(pCreateCtx->pCcb);
    }

    if (pCreateCtx->pFcb) {
        PvfsReleaseFCB(pCreateCtx->pFcb);
    }

    /* The pIrpContext will be freed after somewhere else */

    PVFS_FREE(&pCreateCtx);

    return;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsAllocateCreateContext(
    OUT PPVFS_PENDING_CREATE *ppCreate,
    IN  PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_CREATE pCreateCtx = NULL;
    IRP_ARGS_CREATE Args = pIrpContext->pIrp->Args.Create;
    PIO_CREATE_SECURITY_CONTEXT pSecCtx = Args.SecurityContext;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pCreateCtx,
                  sizeof(PVFS_PENDING_CREATE));
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCanonicalPathName(
                  &pCreateCtx->pszOriginalFilename,
                  Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCreateCtx->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAcquireAccessToken(pCreateCtx->pCcb, pSecCtx);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->pIrpContext = pIrpContext;

    *ppCreate = pCreateCtx;
    pCreateCtx = NULL;


cleanup:

    return ntError;

error:
    PvfsFreeCreateContext(&pCreateCtx);

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
