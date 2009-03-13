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


/********************************************************
 * Top level driver for creating an actual file.  Splits
 * work based on the CreateDisposition.
 *******************************************************/

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

/********************************************************
 *******************************************************/

static NTSTATUS
PvfsCreateFileSupersede(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    IRP_ARGS_CREATE Args = pIrp->Args.Create;
    PIO_CREATE_SECURITY_CONTEXT pSecCtx = Args.SecurityContext;
    PSTR pszFilename = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;
    FILE_CREATE_RESULT CreateResult = 0;
    ACCESS_MASK GrantedAccess = 0;
    PVFS_STAT Stat = {0};
    BOOLEAN bFileExisted = FALSE;
    PSTR pszDirectory = NULL;

    ntError = PvfsCanonicalPathName(&pszFilename, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for file existence.  Remove it if necessary */

    ntError = PvfsSysStat(pszFilename, &Stat);
    bFileExisted = NT_SUCCESS(ntError);

    if (bFileExisted) {
        ntError = PvfsAccessCheckFile(pSecCtx,
                                      pszFilename,
                                      DELETE,
                                      &GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysRemove(pszFilename);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAccessCheckDir(pSecCtx,
                                 pszDirectory,
                                 Args.DesiredAccess,
                                 &GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags, GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpen(&fd, pszFilename, unixFlags, 0600);
    BAIL_ON_NT_STATUS(ntError);

    /* TODO: Set SD on file */

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = Args.CreateOptions;
    pCcb->pszFilename = pszFilename;
    pszFilename = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* File properties */

    if (pSecCtx) {
        ntError = PvfsSysChown(pCcb,
                               pSecCtx->Process.Uid,
                               pSecCtx->Process.Gid);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (Args.FileAttributes != 0) {
        ntError = PvfsSetFileAttributes(pCcb, Args.FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    CreateResult = bFileExisted ? FILE_SUPERSEDED : FILE_CREATED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    return ntError;

error:
    CreateResult = bFileExisted ? FILE_EXISTS : FILE_DOES_NOT_EXIST;

    if (fd != -1)
    {
        close(fd);
    }

    PvfsFreeCCB(pCcb);
    RtlCStringFree(&pszFilename);

    goto cleanup;
}

/********************************************************
 *******************************************************/

static NTSTATUS
PvfsCreateFileCreate(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    IRP_ARGS_CREATE Args = pIrp->Args.Create;
    PIO_CREATE_SECURITY_CONTEXT pSecCtx = Args.SecurityContext;
    PSTR pszFilename = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;
    FILE_CREATE_RESULT CreateResult = 0;
    ACCESS_MASK GrantedAccess = 0;
    PSTR pszParentDir = NULL;

    ntError = PvfsCanonicalPathName(&pszFilename, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Check that we can add files to the parent directory.  If
       we can, then the granted access on the file should be
       ALL_ACCESS. */

    ntError = PvfsAccessCheckDir(pSecCtx,
                                 pszParentDir,
                                 FILE_ADD_FILE,
                                 &GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);
    GrantedAccess = FILE_ALL_ACCESS;

    ntError = MapPosixOpenFlags(&unixFlags, GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    /* Let the open() call fail if the file exists.  No need
       for a prior stat() */

    ntError = PvfsSysOpen(&fd, pszFilename, unixFlags, 0600);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = Args.CreateOptions;
    pCcb->pszFilename = pszFilename;
    pszFilename = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* File properties */

    if (pSecCtx) {
        ntError = PvfsSysChown(pCcb,
                               pSecCtx->Process.Uid,
                               pSecCtx->Process.Gid);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (Args.FileAttributes != 0) {
        ntError = PvfsSetFileAttributes(pCcb, Args.FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    CreateResult = FILE_CREATED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    return ntError;

error:
    CreateResult = (ntError == STATUS_OBJECT_NAME_COLLISION) ?
                   FILE_EXISTS : FILE_DOES_NOT_EXIST;

    if (fd != -1)
    {
        /* TODO: Remove the file */

        close(fd);
    }

    PvfsFreeCCB(pCcb);
    RtlCStringFree(&pszFilename);

    goto cleanup;
}

/********************************************************
 *******************************************************/

NTSTATUS
PvfsCreateFileOpen(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    IRP_ARGS_CREATE Args = pIrp->Args.Create;
    PIO_CREATE_SECURITY_CONTEXT pSecCtx = Args.SecurityContext;
    PSTR pszFilename = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;
    FILE_CREATE_RESULT CreateResult = 0;
    ACCESS_MASK GrantedAccess = 0;

    ntError = PvfsCanonicalPathName(&pszFilename, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckFile(pSecCtx,
                                  pszFilename,
                                  Args.DesiredAccess,
                                  &GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags, GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    /* Let the open() call fail if the file does not exists.  No
       need for a prior stat() */

    ntError = PvfsSysOpen(&fd, pszFilename, unixFlags, 0);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = Args.CreateOptions;
    pCcb->pszFilename = pszFilename;
    pszFilename = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    CreateResult = FILE_OPENED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    return ntError;

error:
    CreateResult = (ntError == STATUS_OBJECT_PATH_NOT_FOUND) ?
                   FILE_DOES_NOT_EXIST : FILE_EXISTS;

    if (fd != -1)
    {
        close(fd);
    }

    PvfsFreeCCB(pCcb);
    RtlCStringFree(&pszFilename);

    goto cleanup;
}


/********************************************************
 *******************************************************/

static NTSTATUS
PvfsCreateFileOpenIf(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    IRP_ARGS_CREATE Args = pIrp->Args.Create;
    PIO_CREATE_SECURITY_CONTEXT pSecCtx = Args.SecurityContext;
    PSTR pszFilename = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;
    FILE_CREATE_RESULT CreateResult = 0;
    ACCESS_MASK GrantedAccess = 0;
    PVFS_STAT Stat = {0};
    BOOLEAN bFileExisted = FALSE;

    ntError = PvfsCanonicalPathName(&pszFilename, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for file existence */

    ntError = PvfsSysStat(pszFilename, &Stat);
    bFileExisted = NT_SUCCESS(ntError);

    /* If the file doesn't exist, we have to rely on the security
       of the parent directory */

    if (!bFileExisted)
    {
        PSTR pszDirectory = NULL;

        ntError = PvfsAccessCheckDir(pSecCtx,
                                     pszDirectory,
                                     Args.DesiredAccess,
                                     &GrantedAccess);
    }
    else
    {
        ntError = PvfsAccessCheckFile(pSecCtx,
                                      pszFilename,
                                      Args.DesiredAccess,
                                      &GrantedAccess);
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags, GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpen(&fd, pszFilename, unixFlags, 0600);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = Args.CreateOptions;
    pCcb->pszFilename = pszFilename;
    pszFilename = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* File properties */

    if (!bFileExisted && pSecCtx)
    {
        ntError = PvfsSysChown(pCcb,
                               pSecCtx->Process.Uid,
                               pSecCtx->Process.Gid);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (!bFileExisted && Args.FileAttributes != 0) {
        ntError = PvfsSetFileAttributes(pCcb, Args.FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    CreateResult = bFileExisted ? FILE_OPENED : FILE_CREATED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    return ntError;

error:
    CreateResult = (ntError == STATUS_OBJECT_NAME_COLLISION) ?
                   FILE_EXISTS : FILE_DOES_NOT_EXIST;

    if (fd != -1)
    {
        close(fd);
    }

    PvfsFreeCCB(pCcb);
    RtlCStringFree(&pszFilename);

    goto cleanup;
}

/********************************************************
 *******************************************************/

static NTSTATUS
PvfsCreateFileOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    IRP_ARGS_CREATE Args = pIrp->Args.Create;
    PIO_CREATE_SECURITY_CONTEXT pSecCtx = Args.SecurityContext;
    PSTR pszFilename = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;
    FILE_CREATE_RESULT CreateResult = 0;
    ACCESS_MASK GrantedAccess = 0;

    ntError = PvfsCanonicalPathName(&pszFilename, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Just check access on the file and allow the open to
       validate existence */

    ntError = PvfsAccessCheckFile(pSecCtx,
                                  pszFilename,
                                  Args.DesiredAccess,
                                  &GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags, GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpen(&fd, pszFilename, unixFlags, 0600);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = Args.CreateOptions;
    pCcb->pszFilename = pszFilename;
    pszFilename = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* File properties */

    if (pSecCtx)
    {
        ntError = PvfsSysChown(pCcb,
                               pSecCtx->Process.Uid,
                               pSecCtx->Process.Gid);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* This should always update the File Attributes even if the
       file previously existed */

    if (Args.FileAttributes != 0) {
        ntError = PvfsSetFileAttributes(pCcb, Args.FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    CreateResult = FILE_OVERWRITTEN;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    return ntError;

error:
    CreateResult = (ntError == STATUS_OBJECT_PATH_NOT_FOUND) ?
        FILE_DOES_NOT_EXIST : FILE_EXISTS;

    if (fd != -1)
    {
        close(fd);
    }

    PvfsFreeCCB(pCcb);
    RtlCStringFree(&pszFilename);

    goto cleanup;
}

/********************************************************
 *******************************************************/

static NTSTATUS
PvfsCreateFileOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    IRP_ARGS_CREATE Args = pIrp->Args.Create;
    PIO_CREATE_SECURITY_CONTEXT pSecCtx = Args.SecurityContext;
    PSTR pszFilename = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;
    FILE_CREATE_RESULT CreateResult = 0;
    ACCESS_MASK GrantedAccess = 0;
    PVFS_STAT Stat = {0};
    BOOLEAN bFileExisted = FALSE;

    ntError = PvfsCanonicalPathName(&pszFilename, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for file existence */

    ntError = PvfsSysStat(pszFilename, &Stat);
    bFileExisted = NT_SUCCESS(ntError);

    /* If the file doesn't exist, we have to rely on the security
       of the parent directory */

    if (!bFileExisted)
    {
        PSTR pszDirectory = NULL;

        ntError = PvfsAccessCheckDir(pSecCtx,
                                     pszDirectory,
                                     Args.DesiredAccess,
                                     &GrantedAccess);
    }
    else
    {
        ntError = PvfsAccessCheckFile(pSecCtx,
                                      pszFilename,
                                      Args.DesiredAccess,
                                      &GrantedAccess);
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags, GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpen(&fd, pszFilename, unixFlags, 0600);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = Args.CreateOptions;
    pCcb->pszFilename = pszFilename;
    pszFilename = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* File properties */

    if (pSecCtx)
    {
        ntError = PvfsSysChown(pCcb,
                               pSecCtx->Process.Uid,
                               pSecCtx->Process.Gid);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* This should always update the File Attributes even if the
       file previously existed */

    if (Args.FileAttributes != 0) {
        ntError = PvfsSetFileAttributes(pCcb, Args.FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    CreateResult = bFileExisted ? FILE_OVERWRITTEN : FILE_CREATED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    return ntError;

error:
    CreateResult = (ntError == STATUS_OBJECT_NAME_COLLISION) ?
                   FILE_EXISTS : FILE_DOES_NOT_EXIST;

    if (fd != -1)
    {
        close(fd);
    }

    PvfsFreeCCB(pCcb);
    RtlCStringFree(&pszFilename);

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
