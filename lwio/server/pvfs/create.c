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
 *        create.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Create Dispatch Routine
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/*****************************************************************************
 Main entry to the Create() routine for driver.  Splits work based on the
 CreateOptions.
 ****************************************************************************/

NTSTATUS
PvfsCreate(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    FILE_CREATE_OPTIONS CreateOptions = 0;
    BOOLEAN bIsDirectory = FALSE;
    PIRP pIrp = pIrpContext->pIrp;
    PSTR pszFilename = NULL;
    PSTR pszDiskFilename = NULL;
    PVFS_STAT Stat = {0};

    CreateOptions = pIrp->Args.Create.CreateOptions;

    if (CreateOptions & FILE_DIRECTORY_FILE)
    {
        bIsDirectory = TRUE;
    }
    else if (CreateOptions & FILE_NON_DIRECTORY_FILE)
    {
        bIsDirectory = FALSE;
    }
    else
    {
        /* stat() the path and find out if this is a file or directory */

        ntError = PvfsCanonicalPathName(&pszFilename,
                                        pIrp->Args.Create.FileName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsLookupPath(&pszDiskFilename, pszFilename, FALSE);

        /* The path lookup may fail which is ok.  We'll catch whether
           or not this is a real error later on */

        if (ntError == STATUS_SUCCESS)
        {
            ntError = PvfsSysStat(pszDiskFilename, &Stat);

            bIsDirectory = (ntError == STATUS_SUCCESS) ?
                           S_ISDIR(Stat.s_mode) : FALSE;
        }
    }


    if (bIsDirectory)
    {
        pIrp->Args.Create.CreateOptions |= FILE_DIRECTORY_FILE;
        ntError = PvfsCreateDirectory(pIrpContext);
    }
    /* File branch */
    else
    {
        pIrp->Args.Create.CreateOptions |= FILE_NON_DIRECTORY_FILE;
        ntError = PvfsCreateFile(pIrpContext);
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    RtlCStringFree(&pszFilename);
    RtlCStringFree(&pszDiskFilename);

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCreateFileDoSysOpen(
    IN PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_CREATE pCreateContext = (PPVFS_PENDING_CREATE)pContext;
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

NTSTATUS
PvfsCreateDirDoSysOpen(
    IN PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_PENDING_CREATE pCreateContext = (PPVFS_PENDING_CREATE)pContext;
    PIRP pIrp = pCreateContext->pIrpContext->pIrp;
    IRP_ARGS_CREATE Args = pIrp->Args.Create;
    int fd = -1;
    int unixFlags = 0;
    PIO_CREATE_SECURITY_CONTEXT pSecCtx = Args.SecurityContext;
    FILE_CREATE_RESULT CreateResult = 0;

    /* Do the open() */

    ntError = MapPosixOpenFlags(
                  &unixFlags,
                  pCreateContext->GrantedAccess,
                  Args);
    BAIL_ON_NT_STATUS(ntError);

    if (!pCreateContext->bFileExisted)
    {
        ntError = PvfsSysMkDir(pCreateContext->pszDiskFilename, 0700);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Open the DIR* and then open a fd based on that */

    ntError = PvfsAllocateMemory(
                  (PVOID)&pCreateContext->pCcb->pDirContext,
                  sizeof(PVFS_DIRECTORY_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpenDir(
                  pCreateContext->pszDiskFilename,
                  &pCreateContext->pCcb->pDirContext->pDir);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysDirFd(pCreateContext->pCcb, &fd);
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

FILE_CREATE_RESULT
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

NTSTATUS
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
    IN OUT PVOID *ppContext
    )
{
    PPVFS_PENDING_CREATE pCreateCtx = NULL;

    if (!ppContext || !*ppContext) {
        return;
    }

    pCreateCtx = (PPVFS_PENDING_CREATE)*ppContext;


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
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

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
