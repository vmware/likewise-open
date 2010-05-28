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
    FILE_ATTRIBUTES     fileAttributes = 0;
    BOOLEAN bIsDirectory = FALSE;
    PIRP pIrp = pIrpContext->pIrp;
    PSTR pszFilename = NULL;
    PSTR pszDiskFilename = NULL;
    PVFS_STAT Stat = {0};
    FILE_CREATE_OPTIONS FileDirCombo = (FILE_DIRECTORY_FILE|
                                        FILE_NON_DIRECTORY_FILE);


    /* Check to see if this is a Device Create (i.e. NULL RootFileHandle
       and empty Filename) */

    if (!pIrp->Args.Create.FileName.RootFileHandle &&
        pIrp->Args.Create.FileName.FileName &&
        *pIrp->Args.Create.FileName.FileName == '\0')
    {
        ntError = PvfsCreateDevice(pIrpContext);
        BAIL_ON_NT_STATUS(ntError);

        goto cleanup;
    }

    /* Regular File/Directory Create() */

    CreateOptions = pIrp->Args.Create.CreateOptions;

    if (((CreateOptions & ~FILE_CREATE_OPTIONS_VALID) != 0))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    fileAttributes = pIrp->Args.Create.FileAttributes;

    if ((fileAttributes & ~FILE_ATTRIBUTE_VALID_SET_FLAGS) != 0)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    if ((CreateOptions & FileDirCombo) == FileDirCombo)
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

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

        ntError = PvfsCanonicalPathName(
                      &pszFilename,
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

static
VOID
PvfsCleanupFailedCreate(
    int fd,
    PCSTR pszPath,
    BOOLEAN bWasCreated
    );

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
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcess = NULL;

    pProcess = IoSecurityGetProcessInfo(pSecCtx);

    /* Fail any create that requires setting the security but doesn't
       have the Unix uid/gid information */

    if ((pCreateContext->SetPropertyFlags & PVFS_SET_PROP_SECURITY) &&
        (pProcess == NULL))
    {
        ntError = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsEnforceShareMode(
                   pCreateContext->pFcb,
                   Args.ShareAccess,
                   pCreateContext->GrantedAccess);
    BAIL_ON_NT_STATUS(ntError);

    /* Do the open() */

    ntError = MapPosixOpenFlags(&unixFlags, pCreateContext->GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    do
    {
        ntError = PvfsSysOpen(
                      &fd,
                      pCreateContext->pszDiskFilename,
                      unixFlags,
                      (mode_t)gPvfsDriverConfig.CreateFileMode);

    } while (ntError == STATUS_MORE_PROCESSING_REQUIRED);
    BAIL_ON_NT_STATUS(ntError);

    /* Perform preallocation is requested */

    if (Args.AllocationSize > 0)
    {
        BOOLEAN bAllocate = FALSE;

        switch (Args.CreateDisposition)
        {
        case FILE_SUPERSEDE:
        case FILE_CREATE:
        case FILE_OVERWRITE:
        case FILE_OVERWRITE_IF:
            bAllocate = TRUE;
            break;

        case FILE_OPEN_IF:
            if (!pCreateContext->bFileExisted)
            {
                bAllocate = TRUE;
            }
            break;
        }

        if (bAllocate)
        {
            ntError = PvfsSysFtruncate(fd, (off_t)Args.AllocationSize);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    /* Save our state */

    pCreateContext->pCcb->fd = fd;
    pCreateContext->pCcb->ShareFlags = Args.ShareAccess;
    pCreateContext->pCcb->AccessGranted = pCreateContext->GrantedAccess;
    pCreateContext->pCcb->CreateOptions = Args.CreateOptions;

    pCreateContext->pCcb->pszFilename = pCreateContext->pszDiskFilename;
    pCreateContext->pszDiskFilename = NULL;

    ntError = PvfsAddCCBToFCB(pCreateContext->pFcb, pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSaveFileDeviceInfo(pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* CCB is now complete */

    if ((pCreateContext->SetPropertyFlags & PVFS_SET_PROP_SECURITY) && pSecCtx)
    {
        /* Unix Security */

        ntError = PvfsSysChown(
                      pCreateContext->pCcb,
                      pProcess->Uid,
                      pProcess->Gid);
        BAIL_ON_NT_STATUS(ntError);

        /* Security Descriptor */

        ntError = PvfsCreateFileSecurity(
                      pCreateContext->pCcb->pUserToken,
                      pCreateContext->pCcb,
                      Args.SecurityDescriptor,
                      FALSE);
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

    /* Save the delete-on-close flag to the FCB */

    if (Args.CreateOptions & FILE_DELETE_ON_CLOSE)
    {
        pCreateContext->pCcb->bPendingDeleteHandle = TRUE;
    }

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    PvfsInitializeZctSupport(pCreateContext->pCcb, pIrp->FileHandle);

    CreateResult = PvfsSetCreateResult(
                       Args.CreateDisposition,
                       pCreateContext->bFileExisted,
                       STATUS_SUCCESS);

    if (CreateResult == FILE_CREATED)
    {
        PvfsNotifyScheduleFullReport(
            pCreateContext->pCcb->pFcb,
            FILE_NOTIFY_CHANGE_FILE_NAME,
            FILE_ACTION_ADDED,
            pCreateContext->pCcb->pszFilename);
    }

    /* The CCB has been handled off to the FileHandle so make sure
       we don't think we still own it */

    pCreateContext->pCcb = NULL;

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
        PSTR pszRemovePath = NULL;

        /* Pick up where we started the pathname */

        pszRemovePath = pCreateContext->pszDiskFilename ?
                        pCreateContext->pszDiskFilename :
                        pCreateContext->pCcb->pszFilename;

        PvfsCleanupFailedCreate(
            fd,
            pszRemovePath,
            !pCreateContext->bFileExisted);
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
    IO_MATCH_FILE_SPEC FileSpec = {0};
    WCHAR wszPattern[2] = {L'*', 0x0 };
    PIO_SECURITY_CONTEXT_PROCESS_INFORMATION pProcess = NULL;
    PBOOLEAN pbEnableAbe = NULL;
    ULONG ulEcpSize = 0;

    pProcess = IoSecurityGetProcessInfo(pSecCtx);

    /* Fail any create that requires setting the security but doesn't
       have the Unix uid/gid information */

    if ((pCreateContext->SetPropertyFlags & PVFS_SET_PROP_SECURITY) &&
        (pProcess == NULL))
    {
        ntError = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Do the open() */

    ntError = MapPosixOpenFlags(
                  &unixFlags,
                  pCreateContext->GrantedAccess,
                  Args);
    BAIL_ON_NT_STATUS(ntError);

    if (!pCreateContext->bFileExisted)
    {
        ntError = PvfsSysMkDir(
                      pCreateContext->pszDiskFilename,
                      (mode_t)gPvfsDriverConfig.CreateDirectoryMode);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Open the DIR* and then open a fd based on that */

    ntError = PvfsAllocateMemory(
                  (PVOID)&pCreateContext->pCcb->pDirContext,
                  sizeof(PVFS_DIRECTORY_CONTEXT));
    BAIL_ON_NT_STATUS(ntError);

    pCreateContext->pCcb->pszFilename = pCreateContext->pszDiskFilename;
    pCreateContext->pszDiskFilename = NULL;

    ntError = IoRtlEcpListFind(
                  pIrp->Args.Create.EcpList,
                  SRV_ECP_TYPE_ABE,
                  OUT_PPVOID(&pbEnableAbe),
                  &ulEcpSize);
    if (ntError != STATUS_NOT_FOUND)
    {
        BAIL_ON_NT_STATUS(ntError);

        if (ulEcpSize != sizeof(BOOLEAN))
        {
            ntError = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntError);
        }

        if (*pbEnableAbe)
        {
            pCreateContext->pCcb->EcpFlags |= PVFS_ECP_ENABLE_ABE;
        }
    }

    do
    {
        ntError = PvfsSysOpen(
                      &fd,
                      pCreateContext->pCcb->pszFilename,
                      0,
                      0);

    } while (ntError == STATUS_MORE_PROCESSING_REQUIRED);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCreateContext->pCcb->fd = fd;
    pCreateContext->pCcb->ShareFlags = Args.ShareAccess;
    pCreateContext->pCcb->AccessGranted = pCreateContext->GrantedAccess;
    pCreateContext->pCcb->CreateOptions = Args.CreateOptions;

    ntError = PvfsAddCCBToFCB(pCreateContext->pFcb, pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSaveFileDeviceInfo(pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if ((pCreateContext->SetPropertyFlags & PVFS_SET_PROP_SECURITY) && pSecCtx)
    {
        /* Unix Security */

        ntError = PvfsSysChown(
                      pCreateContext->pCcb,
                      pProcess->Uid,
                      pProcess->Gid);
        BAIL_ON_NT_STATUS(ntError);

        /* Security Descriptor */

        ntError = PvfsCreateFileSecurity(
                      pCreateContext->pCcb->pUserToken,
                      pCreateContext->pCcb,
                      Args.SecurityDescriptor,
                      TRUE);
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

    /* Save the delete-on-close flag to the FCB */

    if (Args.CreateOptions & FILE_DELETE_ON_CLOSE)
    {
        LwRtlUnicodeStringInit(&FileSpec.Pattern, wszPattern);

        ntError = PvfsEnumerateDirectory(pCreateContext->pCcb, &FileSpec, 1, FALSE);
        if (ntError == STATUS_SUCCESS)
        {
            ntError = STATUS_DIRECTORY_NOT_EMPTY;
            BAIL_ON_NT_STATUS(ntError);
        }

        pCreateContext->pCcb->bPendingDeleteHandle = TRUE;
    }

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    PvfsInitializeZctSupport(pCreateContext->pCcb, pIrp->FileHandle);

    CreateResult = PvfsSetCreateResult(
                       Args.CreateDisposition,
                       pCreateContext->bFileExisted,
                       STATUS_SUCCESS);

    if (CreateResult == FILE_CREATED)
    {
        PvfsNotifyScheduleFullReport(
            pCreateContext->pCcb->pFcb,
            FILE_NOTIFY_CHANGE_FILE_NAME,
            FILE_ACTION_ADDED,
            pCreateContext->pCcb->pszFilename);
    }

    /* The CCB has been handled off to the FileHandle so make sure
       we don't think we still own it */

    pCreateContext->pCcb = NULL;

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
        PSTR pszRemovePath = NULL;

        /* Pick up where we started the pathname */

        pszRemovePath = pCreateContext->pszDiskFilename ?
                        pCreateContext->pszDiskFilename :
                        pCreateContext->pCcb->pszFilename;

        PvfsCleanupFailedCreate(
            fd,
            pszRemovePath,
            !pCreateContext->bFileExisted);
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
PvfsCheckDeleteOnClose(
    IN IRP_ARGS_CREATE CreateArgs,
    IN PSTR pszFilename,
    IN ACCESS_MASK GrantedAccess
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    FILE_ATTRIBUTES Attributes = 0;

    if (!(CreateArgs.CreateOptions & FILE_DELETE_ON_CLOSE)) {
        goto cleanup;
    }

    if (!(GrantedAccess & DELETE))
    {
        ntError = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* ReadOnly is largely ignored by the file system on directories.
       It is used by the Windows explorer.exe to mark folders as "special" */

    if (CreateArgs.CreateOptions & FILE_DIRECTORY_FILE)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    /* Dealing with files from here down */

    if (pszFilename)
    {
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

    if (ppContext && *ppContext)
    {
        pCreateCtx = (PPVFS_PENDING_CREATE)*ppContext;

        if (pCreateCtx->pIrpContext)
        {
            PvfsReleaseIrpContext(&pCreateCtx->pIrpContext);
        }

        if (pCreateCtx->pCcb)
        {
            PvfsReleaseCCB(pCreateCtx->pCcb);
        }

        if (pCreateCtx->pFcb)
        {
            PvfsReleaseFCB(&pCreateCtx->pFcb);
        }

        RtlCStringFree(&pCreateCtx->pszDiskFilename);
        RtlCStringFree(&pCreateCtx->pszOriginalFilename);

        PVFS_FREE(ppContext);
    }

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

    pCreateCtx->pIrpContext = PvfsReferenceIrpContext(pIrpContext);

    *ppCreate = pCreateCtx;
    pCreateCtx = NULL;


cleanup:
    return ntError;

error:
    PvfsFreeCreateContext((PVOID*)&pCreateCtx);

    goto cleanup;
}



/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCreateFileCheckPendingDelete(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FCB pParentFcb = NULL;

    if (PvfsFcbIsPendingDelete(pFcb))
    {
        ntError = STATUS_DELETE_PENDING;
        BAIL_ON_NT_STATUS(ntError);
    }

    pParentFcb = PvfsGetParentFCB(pFcb);
    if (pParentFcb && PvfsFcbIsPendingDelete(pParentFcb))
    {
        ntError = STATUS_DELETE_PENDING;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    if (pParentFcb)
    {
        PvfsReleaseFCB(&pParentFcb);
    }


    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsCleanupFailedCreate(
    int fd,
    PCSTR pszPath,
    BOOLEAN bWasCreated
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_STAT Stat1 = {0};
    PVFS_STAT Stat2 = {0};

    /* Remove the file if it was created and the Dev/Inode matches our fd */

    if (bWasCreated && (pszPath != NULL))
    {
        ntError = PvfsSysFstat(fd, &Stat1);
        if (ntError == STATUS_SUCCESS)
        {
            ntError = PvfsSysStat(pszPath, &Stat2);
            if (ntError == STATUS_SUCCESS)
            {
                if ((Stat1.s_dev == Stat1.s_dev) &&
                    (Stat2.s_ino == Stat2.s_ino))
                {
                    ntError = PvfsSysRemove(pszPath);
                }
            }
        }
    }

    ntError = PvfsSysClose(fd);

    return;
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
