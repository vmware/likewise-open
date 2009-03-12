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
 *       Create Dispatch Routine
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */


static NTSTATUS
PvfsCreateDirSupersede(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsCreateDirCreate(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsCreateDirOpen(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsCreateDirOpenIf(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsCreateDirOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static NTSTATUS
PvfsCreateDirOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext
	);



/* Code */

NTSTATUS
PvfsCreateDirectory(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    FILE_CREATE_DISPOSITION CreateDisposition = 0;

    CreateDisposition = pIrpContext->pIrp->Args.Create.CreateDisposition;

    switch (CreateDisposition)
    {
    case FILE_SUPERSEDE:
        ntError = PvfsCreateDirSupersede(pIrpContext);
        break;

    case FILE_CREATE:
        ntError = PvfsCreateDirCreate(pIrpContext);
        break;

    case FILE_OPEN:
        ntError = PvfsCreateDirOpen(pIrpContext);
        break;

    case FILE_OPEN_IF:
        ntError = PvfsCreateDirOpenIf(pIrpContext);
        break;

    case FILE_OVERWRITE:
        ntError = PvfsCreateDirOverwrite(pIrpContext);
        break;

    case FILE_OVERWRITE_IF:
        ntError = PvfsCreateDirOverwriteIf(pIrpContext);
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


static NTSTATUS
PvfsCreateDirSupersede(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    return STATUS_NOT_IMPLEMENTED;

}

static NTSTATUS
PvfsCreateDirCreate(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PSTR pszPathname = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;
    FILE_CREATE_RESULT CreateResult = 0;
    ACCESS_MASK GrantedAccess = 0;

    ntError = PvfsCanonicalPathName(&pszPathname,
                                    pIrp->Args.Create.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory((PVOID)&pCcb->pDirContext,
                                 sizeof(PVFS_DIRECTORY_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);

    /* Should check parent here */

    ntError = PvfsAccessCheckDir(pIrp->Args.Create.SecurityContext,
                                 pszPathname,
                                 pIrp->Args.Create.DesiredAccess,
                                 &GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags,
                                GrantedAccess,
                                pIrp->Args.Create);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the DIR* and then open a fd based on that */

    ntError = PvfsSysMkDir(pszPathname, 0700);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpenDir(pszPathname, &pCcb->pDirContext->pDir);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->pszFilename = pszPathname;
    pszPathname = NULL;

    ntError = PvfsSysDirFd(pCcb, &fd);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = pIrp->Args.Create.CreateOptions;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    CreateResult = FILE_CREATED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    return ntError;

error:
    CreateResult = (ntError == STATUS_OBJECT_PATH_NOT_FOUND) ?
                   FILE_DOES_NOT_EXIST : FILE_EXISTS;

    if (pCcb && pCcb->pDirContext && pCcb->pDirContext->pDir) {
        PvfsSysCloseDir(pCcb->pDirContext->pDir);
    }

    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    PVFS_SAFE_FREE_MEMORY(pszPathname);

    goto cleanup;
}

/******************************************************
 ******************************************************/

static NTSTATUS
PvfsCreateDirOpen(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PSTR pszPathname = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;
    FILE_CREATE_RESULT CreateResult = 0;
    ACCESS_MASK GrantedAccess = 0;

    ntError = PvfsCanonicalPathName(&pszPathname,
                                    pIrp->Args.Create.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory((PVOID)&pCcb->pDirContext,
                                 sizeof(PVFS_DIRECTORY_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);


    ntError = PvfsAccessCheckDir(pIrp->Args.Create.SecurityContext,
                                 pszPathname,
                                 pIrp->Args.Create.DesiredAccess,
                                 &GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags,
                                GrantedAccess,
                                pIrp->Args.Create);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the DIR* and then open a fd based on that */

    ntError = PvfsSysOpenDir(pszPathname, &pCcb->pDirContext->pDir);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->pszFilename = pszPathname;
    pszPathname = NULL;

    ntError = PvfsSysDirFd(pCcb, &fd);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = pIrp->Args.Create.CreateOptions;

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

    if (pCcb && pCcb->pDirContext && pCcb->pDirContext->pDir) {
        PvfsSysCloseDir(pCcb->pDirContext->pDir);
    }

    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    PVFS_SAFE_FREE_MEMORY(pszPathname);

    goto cleanup;
}

static NTSTATUS
PvfsCreateDirOpenIf(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PSTR pszPathname = NULL;
    int fd = -1;
    int unixFlags = 0;
    PPVFS_CCB pCcb = NULL;
    FILE_CREATE_RESULT CreateResult = 0;
    ACCESS_MASK GrantedAccess = 0;
    PVFS_STAT Stat = {0};
    BOOLEAN bDirCreated = FALSE;

    ntError = PvfsCanonicalPathName(&pszPathname,
                                    pIrp->Args.Create.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory((PVOID)&pCcb->pDirContext,
                                 sizeof(PVFS_DIRECTORY_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);

    /* Should check parent here */

    ntError = PvfsAccessCheckDir(pIrp->Args.Create.SecurityContext,
                                 pszPathname,
                                 pIrp->Args.Create.DesiredAccess,
                                 &GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags,
                                GrantedAccess,
                                pIrp->Args.Create);
    BAIL_ON_NT_STATUS(ntError);

    /* Check if the directory exists, else create it */

    ntError = PvfsSysStat(pszPathname, &Stat);
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND) {
        ntError = PvfsSysMkDir(pszPathname, 0700);
        bDirCreated = TRUE;
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpenDir(pszPathname, &pCcb->pDirContext->pDir);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->pszFilename = pszPathname;
    pszPathname = NULL;

    ntError = PvfsSysDirFd(pCcb, &fd);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = pIrp->Args.Create.CreateOptions;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    CreateResult = bDirCreated ? FILE_CREATED : FILE_OPENED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    return ntError;

error:
    CreateResult = (ntError == STATUS_OBJECT_PATH_NOT_FOUND) ?
                   FILE_DOES_NOT_EXIST : FILE_EXISTS;

    if (pCcb && pCcb->pDirContext && pCcb->pDirContext->pDir) {
        PvfsSysCloseDir(pCcb->pDirContext->pDir);
    }

    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    PVFS_SAFE_FREE_MEMORY(pszPathname);

    goto cleanup;
}

static NTSTATUS
PvfsCreateDirOverwrite(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

static NTSTATUS
PvfsCreateDirOverwriteIf(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    return STATUS_NOT_IMPLEMENTED;
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
