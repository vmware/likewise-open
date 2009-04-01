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

/* Code */

/**************************************************************
 *************************************************************/

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
    case FILE_OVERWRITE:
    case FILE_OVERWRITE_IF:
        /* These are all invalid create dispositions for directories */
        ntError = STATUS_INVALID_DISPOSITION;
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


/**************************************************************
 *************************************************************/

static NTSTATUS
PvfsCreateDirCreate(
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
    PSTR pszDirname = NULL;
    PSTR pszRelativeFilename = NULL;
    PSTR pszDiskFilename = NULL;
    PSTR pszDiskDirname = NULL;

    ntError = PvfsCanonicalPathName(&pszFilename, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFileSplitPath(&pszDirname, &pszRelativeFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath(&pszDiskDirname, pszDirname, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupFile(&pszDiskFilename,
                             pszDiskDirname,
                             pszRelativeFilename,
                             FALSE);
    if (ntError == STATUS_SUCCESS) {
        ntError = STATUS_OBJECT_NAME_COLLISION;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = RtlCStringAllocatePrintf(&pszDiskFilename,
                                       "%s/%s",
                                       pszDiskDirname,
                                       pszRelativeFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAcquireAccessToken(pCcb, pSecCtx);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory((PVOID)&pCcb->pDirContext,
                                 sizeof(PVFS_DIRECTORY_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);

    /* Should check parent here */

    ntError = PvfsAccessCheckDir(pCcb->pUserToken,
                                 pszDirname,
                                 FILE_ADD_SUBDIRECTORY,
                                 &GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    GrantedAccess = FILE_ALL_ACCESS;

    ntError = MapPosixOpenFlags(&unixFlags, GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the DIR* and then open a fd based on that */

    ntError = PvfsSysMkDir(pszDiskFilename, 0700);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpenDir(pszDiskFilename, &pCcb->pDirContext->pDir);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysDirFd(pCcb, &fd);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = Args.CreateOptions;
    pCcb->pszFilename = pszDiskFilename;
    pszDiskFilename = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Directory Properties */

    if (pSecCtx) {
        PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcess = IoSecurityGetProcessInfo(pSecCtx);

        ntError = PvfsSysChown(pCcb,
                               pProcess->Uid,
                               pProcess->Gid);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (Args.FileAttributes != 0) {
        ntError = PvfsSetFileAttributes(pCcb, Args.FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    CreateResult = FILE_CREATED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    RtlCStringFree(&pszFilename);
    RtlCStringFree(&pszDirname);
    RtlCStringFree(&pszRelativeFilename);
    RtlCStringFree(&pszDiskDirname);
    RtlCStringFree(&pszDiskFilename);

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

    goto cleanup;
}

/**************************************************************
 *************************************************************/

static NTSTATUS
PvfsCreateDirOpen(
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
    PSTR pszDiskFilename = NULL;

    ntError = PvfsCanonicalPathName(&pszFilename, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath(&pszDiskFilename, pszFilename, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAcquireAccessToken(pCcb, pSecCtx);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory((PVOID)&pCcb->pDirContext,
                                 sizeof(PVFS_DIRECTORY_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);


    ntError = PvfsAccessCheckDir(pCcb->pUserToken,
                                 pszDiskFilename,
                                 Args.DesiredAccess,
                                 &GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags, GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    /* Open the DIR* and then open a fd based on that */

    ntError = PvfsSysOpenDir(pszDiskFilename, &pCcb->pDirContext->pDir);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysDirFd(pCcb, &fd);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = Args.CreateOptions;
    pCcb->pszFilename = pszDiskFilename;
    pszDiskFilename = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    CreateResult = FILE_OPENED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    RtlCStringFree(&pszFilename);
    RtlCStringFree(&pszDiskFilename);

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


    goto cleanup;
}

/**************************************************************
 *************************************************************/

static NTSTATUS
PvfsCreateDirOpenIf(
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
    BOOLEAN bFileExisted = FALSE;
    BOOLEAN bDirCreated = FALSE;
    PSTR pszDirname = NULL;
    PSTR pszRelativeFilename = NULL;
    PSTR pszDiskFilename = NULL;
    PSTR pszDiskDirname = NULL;

    ntError = PvfsCanonicalPathName(&pszFilename, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsFileSplitPath(&pszDirname, &pszRelativeFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath(&pszDiskDirname, pszDirname, FALSE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAcquireAccessToken(pCcb, pSecCtx);
    BAIL_ON_NT_STATUS(ntError);

    /* Check for file existence */

    ntError = PvfsLookupFile(&pszDiskFilename,
                             pszDiskDirname,
                             pszRelativeFilename,
                             FALSE);
    bFileExisted = NT_SUCCESS(ntError);

    if (!bFileExisted)
    {
        ntError = RtlCStringAllocatePrintf(&pszDiskFilename,
                                           "%s/%s",
                                           pszDiskDirname,
                                           pszRelativeFilename);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsAccessCheckDir(pCcb->pUserToken,
                                     pszDirname,
                                     FILE_ADD_SUBDIRECTORY,
                                     &GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        ntError = PvfsAccessCheckDir(pCcb->pUserToken,
                                     pszDiskFilename,
                                     Args.DesiredAccess,
                                     &GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (!bFileExisted) {
        ntError = PvfsSysMkDir(pszDiskFilename, 0700);
        bDirCreated = TRUE;
    }
    BAIL_ON_NT_STATUS(ntError);

    ntError = MapPosixOpenFlags(&unixFlags, GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory((PVOID)&pCcb->pDirContext,
                                 sizeof(PVFS_DIRECTORY_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysOpenDir(pszDiskFilename, &pCcb->pDirContext->pDir);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysDirFd(pCcb, &fd);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCcb->fd = fd;
    pCcb->AccessGranted = GrantedAccess;
    pCcb->CreateOptions = Args.CreateOptions;
    pCcb->pszFilename = pszDiskFilename;
    pszDiskFilename = NULL;

    ntError = PvfsSaveFileDeviceInfo(pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Directory Properties */

    if (bDirCreated)
    {
        if (pSecCtx) {
            PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcess = IoSecurityGetProcessInfo(pSecCtx);

            ntError = PvfsSysChown(pCcb,
                                   pProcess->Uid,
                                   pProcess->Gid);
            BAIL_ON_NT_STATUS(ntError);
        }

        if (Args.FileAttributes != 0) {
            ntError = PvfsSetFileAttributes(pCcb, Args.FileAttributes);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    CreateResult = bDirCreated ? FILE_CREATED : FILE_OPENED;

cleanup:
    pIrp->IoStatusBlock.CreateResult = CreateResult;

    RtlCStringFree(&pszFilename);
    RtlCStringFree(&pszDirname);
    RtlCStringFree(&pszRelativeFilename);
    RtlCStringFree(&pszDiskDirname);
    RtlCStringFree(&pszDiskFilename);

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
