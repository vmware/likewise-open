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

    ntError = LwRtlCStringAllocatePrintf(
                  &pCreateCtx->pszDiskFilename,
                  "%s/%s",
                  pszDiskDirname,
                  pszRelativeFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* check parent here */

    ntError = PvfsAccessCheckDir(
                  pCreateCtx->pCcb->pUserToken,
                  pszDirname,
                  FILE_ADD_SUBDIRECTORY,
                  &pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);
    pCreateCtx->GrantedAccess = Args.DesiredAccess;

    ntError = PvfsCheckReadOnlyDeleteOnClose(Args, NULL);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCheckShareMode(
                  pCreateCtx->pszDiskFilename,
                  Args.ShareAccess,
                  pCreateCtx->GrantedAccess,
                  &pCreateCtx->pFcb);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->bFileExisted = FALSE;
    pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|PVFS_SET_PROP_ATTRIB;

    ntError = PvfsCreateDirDoSysOpen(pCreateCtx);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    LwRtlCStringFree(&pszDirname);
    LwRtlCStringFree(&pszRelativeFilename);
    LwRtlCStringFree(&pszDiskDirname);

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

    if (pCreateCtx->pCcb &&
        pCreateCtx->pCcb->pDirContext &&
        pCreateCtx->pCcb->pDirContext->pDir)
    {
        PvfsSysCloseDir(pCreateCtx->pCcb->pDirContext->pDir);
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
    IRP_ARGS_CREATE Args = pIrpContext->pIrp->Args.Create;
    PPVFS_PENDING_CREATE pCreateCtx = NULL;

    ntError = PvfsAllocateCreateContext(&pCreateCtx, pIrpContext);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsLookupPath(
                  &pCreateCtx->pszDiskFilename,
                  pCreateCtx->pszOriginalFilename,
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckDir(
                  pCreateCtx->pCcb->pUserToken,
                  pCreateCtx->pszDiskFilename,
                  Args.DesiredAccess,
                  &pCreateCtx->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

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

    ntError = PvfsCreateDirDoSysOpen(pCreateCtx);
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

    if (pCreateCtx->pCcb &&
        pCreateCtx->pCcb->pDirContext &&
        pCreateCtx->pCcb->pDirContext->pDir)
    {
        PvfsSysCloseDir(pCreateCtx->pCcb->pDirContext->pDir);
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
    pCreateCtx->bFileExisted = NT_SUCCESS(ntError) ? TRUE : FALSE;

    if (!pCreateCtx->bFileExisted)
    {
        ntError = LwRtlCStringAllocatePrintf(
                      &pCreateCtx->pszDiskFilename,
                      "%s/%s",
                      pszDiskDirname,
                      pszRelativeFilename);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsAccessCheckDir(
                      pCreateCtx->pCcb->pUserToken,
                      pszDirname,
                      FILE_ADD_SUBDIRECTORY,
                      &pCreateCtx->GrantedAccess);
        BAIL_ON_NT_STATUS(ntError);

        pCreateCtx->GrantedAccess = Args.DesiredAccess;
    }
    else
    {
        ntError = PvfsAccessCheckDir(
                      pCreateCtx->pCcb->pUserToken,
                      pCreateCtx->pszDiskFilename,
                      Args.DesiredAccess,
                      &pCreateCtx->GrantedAccess);
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

    if (!pCreateCtx->bFileExisted)
    {
        pCreateCtx->SetPropertyFlags = PVFS_SET_PROP_OWNER|
                                       PVFS_SET_PROP_ATTRIB;
    }

    ntError = PvfsCreateDirDoSysOpen(pCreateCtx);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    LwRtlCStringFree(&pszDirname);
    LwRtlCStringFree(&pszRelativeFilename);
    LwRtlCStringFree(&pszDiskDirname);

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

    if (pCreateCtx->pCcb &&
        pCreateCtx->pCcb->pDirContext &&
        pCreateCtx->pCcb->pDirContext->pDir)
    {
        PvfsSysCloseDir(pCreateCtx->pCcb->pDirContext->pDir);
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
