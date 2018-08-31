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

static
NTSTATUS
PvfsSetMaximalAccessMask(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    );

static
NTSTATUS
PvfsGetEcpShareName(
    PIO_ECP_LIST pEcpList,
    PWSTR* ppwszShareName
    );

static
NTSTATUS
PvfsSetEcpFileInfo(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    );

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
    PVFS_STAT Stat = {0};
    FILE_CREATE_OPTIONS FileDirCombo = (FILE_DIRECTORY_FILE|
                                        FILE_NON_DIRECTORY_FILE);
    PPVFS_FILE_NAME createFileName = NULL;
    PPVFS_FILE_NAME resolvedFileName = NULL;

    /* Check to see if this is a Device Create (i.e. NULL RootFileHandle
       and empty Filename) */

    if (!pIrp->Args.Create.FileName.RootFileHandle &&
        !pIrp->Args.Create.FileName.Name.Length)
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

        ntError = PvfsCanonicalPathName2(
                      &createFileName,
                      pIrp->Args.Create.FileName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsLookupPath2(
                      &resolvedFileName,
                      &Stat,
                      createFileName,
                      pIrp->Args.Create.FileName.IoNameOptions &
                                    IO_NAME_OPTION_CASE_SENSITIVE);

        if (NT_SUCCESS(ntError) &&
            S_ISDIR(Stat.s_mode) &&
            IsSetFlag(
                resolvedFileName->NameOptions,
                PVFS_FILE_NAME_OPTION_DEFINED_STREAM_TYPE))
        {
            ntError = STATUS_FILE_IS_A_DIRECTORY;
            BAIL_ON_NT_STATUS(ntError);
        }

        PvfsFreeFileName(createFileName);
        createFileName = NULL;

        PvfsFreeFileName(resolvedFileName);
        resolvedFileName = NULL;

        /* The path lookup may fail which is ok.  We'll catch whether
           or not this is a real error later on */

        if (ntError == STATUS_SUCCESS)
        {
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
    if (createFileName)
    {
        PvfsFreeFileName(createFileName);
    }

    if (resolvedFileName)
    {
        PvfsFreeFileName(resolvedFileName);
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
    PPVFS_FILE_NAME pPath,
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
    BOOLEAN bCreateOwnerfile = FALSE;

    BAIL_ON_INVALID_PTR(pSecCtx, ntError);

    pProcess = IoSecurityGetProcessInfo(pSecCtx);

    /* Fail any create that requires setting the security but doesn't
       have the Unix uid/gid information */

    if ((pCreateContext->SetPropertyFlags & PVFS_SET_PROP_SECURITY) &&
        (pProcess == NULL))
    {
        ntError = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntError);
    }

    if ((pCreateContext->Status == STATUS_SHARING_VIOLATION) &&
        !(IsSetFlag(Args.CreateOptions, FILE_COMPLETE_IF_OPLOCKED) &&
          pCreateContext->OplockBroken))
    {
        // We rety the share mode check we broke an oplock and
        // have already dealt with the ACK

        ntError = PvfsEnforceShareMode(
                       pCreateContext->pScb,
                       Args.ShareAccess,
                       pCreateContext->GrantedAccess);
        pCreateContext->Status = ntError;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Do the open() */

    ntError = MapPosixOpenFlags(&unixFlags, pCreateContext->GrantedAccess, Args);
    BAIL_ON_NT_STATUS(ntError);

    do
    {
        ntError = PvfsSysOpenByFileName(
                      &fd,
                      &bCreateOwnerfile,
                      pCreateContext->ResolvedFileName,
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
            PvfsSetScbAllocationSize(
                pCreateContext->pCcb->pScb,
                Args.AllocationSize);
        }
    }

    /* Save our state */

    pCreateContext->pCcb->fd = fd;
    pCreateContext->pCcb->ShareFlags = Args.ShareAccess;
    pCreateContext->pCcb->AccessGranted = pCreateContext->GrantedAccess;
    pCreateContext->pCcb->CreateOptions = Args.CreateOptions;

    ntError = PvfsSaveFileDeviceInfo(pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    /* CCB is now complete */

    if (pCreateContext->SetPropertyFlags & PVFS_SET_PROP_SECURITY)
    {
        uid_t ownerId = pProcess->Uid;
        gid_t groupId = pProcess->Gid;

        /* Unix Security */

        if (gPvfsDriverConfig.VirtualUid != (uid_t)-1)
        {
            ownerId = gPvfsDriverConfig.VirtualUid;
            groupId = 0;
            if (gPvfsDriverConfig.VirtualGid != (gid_t)-1)
            {
                groupId = gPvfsDriverConfig.VirtualGid;
            }
        }

        ntError = PvfsSysChownByFileName(
                      pCreateContext->ResolvedFileName,
                      ownerId,
                      groupId);
        BAIL_ON_NT_STATUS(ntError);

        /* Security Descriptor */

        if (PvfsIsDefaultStream(pCreateContext->pCcb->pScb) || bCreateOwnerfile)
        {
            ntError = PvfsCreateFileSecurity(
                          pCreateContext->pCcb->pUserToken,
                          pCreateContext->pCcb,
                          Args.SecurityDescriptor,
                          FALSE);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    if (pCreateContext->SetPropertyFlags & PVFS_SET_PROP_ATTRIB)
    {
        // Overwrite should remove pre-existing streams
        if (PvfsIsDefaultStream(pCreateContext->pCcb->pScb))
        {
            ntError = PvfsRemoveStreams(pCreateContext->pCcb);
            BAIL_ON_NT_STATUS(ntError);
        }

        if (Args.FileAttributes != 0)
        {
            ntError = PvfsSetFileAttributes(
                          pCreateContext->pCcb,
                          Args.FileAttributes);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    /* Save the delete-on-close flag to the SCB */

    if (Args.CreateOptions & FILE_DELETE_ON_CLOSE)
    {
        SetFlag(pCreateContext->pCcb->Flags, PVFS_CCB_FLAG_PENDING_DELETE);
    }

    ntError = PvfsSetMaximalAccessMask(
                  pCreateContext->pIrpContext,
                  pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSetEcpFileInfo(
                  pCreateContext->pIrpContext,
                  pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    PvfsInitializeZctSupport(pCreateContext->pCcb, pIrp->FileHandle);

    CreateResult = PvfsSetCreateResult(
                       Args.CreateDisposition,
                       pCreateContext->bFileExisted,
                       STATUS_SUCCESS);

    // We are done unless the we broke an oplock and
    // FILE_COMPLETE_IF_OPLOCKED was specified

    if (IsSetFlag(Args.CreateOptions, FILE_COMPLETE_IF_OPLOCKED))
    {
        BOOLEAN scbLocked = FALSE;
        PPVFS_SCB pScb = pCreateContext->pCcb->pScb;

        // TODO: This lock may be unnecessary since we are only testing
        // the OplockBreakInProgress boolean.  but I can't quite convince
        // myself.  --gcarter@likewise.com

        LWIO_LOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);
#if 0
        if (pScb->bOplockBreakInProgress)
        {
            // Make sure this error doesn't get squashed later
            ntError = STATUS_OPLOCK_BREAK_IN_PROGRESS;
        }
        else
        {
            // FILE_COMPLETE_IF_OPLOCKED was already specified but the
            // oplock break ACK has already occurred so the FileHandle
            // is ready for use
            SetFlag(pCreateContext->pCcb->Flags, PVFS_CCB_FLAG_CREATE_COMPLETE);
        }
#else
        if (pCreateContext->OplockBroken)
        {
            ntError = STATUS_OPLOCK_BREAK_IN_PROGRESS;
        }
        else
        {
            SetFlag(pCreateContext->pCcb->Flags, PVFS_CCB_FLAG_CREATE_COMPLETE);
        }
#endif
        LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);
    }
    else
    {
        // FILE_COMPLETE_IF_OPLOCKED was not specified so the only way to
        // get here was a normal CreateFile path
        SetFlag(pCreateContext->pCcb->Flags, PVFS_CCB_FLAG_CREATE_COMPLETE);
    }

    if (CreateResult == FILE_CREATED)
    {
        // TODO: May need to defer the norify event if the FileHandle is
        // not ready for use.  --gcarter@likewise.com
        PvfsNotifyScheduleFullReport(
            pCreateContext->pCcb->pScb->pOwnerFcb,
            FILE_NOTIFY_CHANGE_FILE_NAME,
            FILE_ACTION_ADDED,
            pCreateContext->pCcb->pScb->pOwnerFcb->pszFilename);
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

    if (fd != -1 && pCreateContext->ResolvedFileName)
    {
        /* Pick up where we started the pathname */
        PvfsCleanupFailedCreate(
            fd,
            pCreateContext->ResolvedFileName,
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
    BOOLEAN bCreateOwnerFile = FALSE;

    BAIL_ON_INVALID_PTR(pSecCtx, ntError);

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
        ntError = PvfsSysMkDirByFileName(
                      pCreateContext->ResolvedFileName,
                      (mode_t)gPvfsDriverConfig.CreateDirectoryMode);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Open the DIR* and then open a fd based on that */

    ntError = PvfsAllocateMemory(
                  (PVOID)&pCreateContext->pCcb->pDirContext,
                  sizeof(PVFS_DIRECTORY_CONTEXT),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

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
            SetFlag(pCreateContext->pCcb->Flags, PVFS_CCB_FLAG_ENABLE_ABE);
        }
    }

    ntError = PvfsGetEcpShareName(
                    pIrp->Args.Create.EcpList,
                    &pCreateContext->pCcb->pwszShareName);
    BAIL_ON_NT_STATUS(ntError);

    do
    {
        ntError = PvfsSysOpenByFileName(
                      &fd,
                      &bCreateOwnerFile,
                      pCreateContext->ResolvedFileName,
                      0,
                      0);

    } while (ntError == STATUS_MORE_PROCESSING_REQUIRED);
    BAIL_ON_NT_STATUS(ntError);

    /* Save our state */

    pCreateContext->pCcb->fd = fd;
    pCreateContext->pCcb->ShareFlags = Args.ShareAccess;
    pCreateContext->pCcb->AccessGranted = pCreateContext->GrantedAccess;
    pCreateContext->pCcb->CreateOptions = Args.CreateOptions;

    ntError = PvfsSaveFileDeviceInfo(pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (pCreateContext->SetPropertyFlags & PVFS_SET_PROP_SECURITY)
    {
        uid_t ownerId = pProcess->Uid;
        gid_t groupId = pProcess->Gid;

        /* Unix Security */

        if (gPvfsDriverConfig.VirtualUid != (uid_t)-1)
        {
            ownerId = gPvfsDriverConfig.VirtualUid;
            groupId = 0;
            if (gPvfsDriverConfig.VirtualGid != (gid_t)-1)
            {
                groupId = gPvfsDriverConfig.VirtualGid;
            }
        }

        ntError = PvfsSysChownByFileName(
                      pCreateContext->ResolvedFileName,
                      ownerId,
                      groupId);
        BAIL_ON_NT_STATUS(ntError);

        /* Security Descriptor */

        if (PvfsIsDefaultStream(pCreateContext->pCcb->pScb) || bCreateOwnerFile)
        {
            // Only need to set security on the File object (and not the stream)
            ntError = PvfsCreateFileSecurity(
                          pCreateContext->pCcb->pUserToken,
                          pCreateContext->pCcb,
                          Args.SecurityDescriptor,
                          TRUE);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    if ((pCreateContext->SetPropertyFlags & PVFS_SET_PROP_ATTRIB) &&
        (Args.FileAttributes != 0))
    {
        ntError = PvfsSetFileAttributes(
                      pCreateContext->pCcb,
                      Args.FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Save the delete-on-close flag to the SCB */

    if (Args.CreateOptions & FILE_DELETE_ON_CLOSE)
    {
        LwRtlUnicodeStringInit(&FileSpec.Pattern, wszPattern);

        ntError = PvfsEnumerateDirectory(pCreateContext->pCcb, &FileSpec, 1, FALSE);
        if (ntError == STATUS_SUCCESS)
        {
            ntError = STATUS_DIRECTORY_NOT_EMPTY;
            BAIL_ON_NT_STATUS(ntError);
        }

        SetFlag(pCreateContext->pCcb->Flags, PVFS_CCB_FLAG_PENDING_DELETE);
    }

    ntError = PvfsSetMaximalAccessMask(
                  pCreateContext->pIrpContext,
                  pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSetEcpFileInfo(
                  pCreateContext->pIrpContext,
                  pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsStoreCCB(pIrp->FileHandle, pCreateContext->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    PvfsInitializeZctSupport(pCreateContext->pCcb, pIrp->FileHandle);

    CreateResult = PvfsSetCreateResult(
                       Args.CreateDisposition,
                       pCreateContext->bFileExisted,
                       STATUS_SUCCESS);

    // We are done (no directory oplocks to deal with)

    SetFlag(pCreateContext->pCcb->Flags, PVFS_CCB_FLAG_CREATE_COMPLETE);

    if (CreateResult == FILE_CREATED)
    {
        PvfsNotifyScheduleFullReport(
            pCreateContext->pCcb->pScb->pOwnerFcb,
            FILE_NOTIFY_CHANGE_FILE_NAME,
            FILE_ACTION_ADDED,
            pCreateContext->pCcb->pScb->pOwnerFcb->pszFilename);
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
        if (pCreateContext->ResolvedFileName)
        {
            PvfsCleanupFailedCreate(
                fd,
                pCreateContext->ResolvedFileName,
                !pCreateContext->bFileExisted);
        }
        else if (pCreateContext->ResolvedFileName)
        {
            PvfsCleanupFailedCreate(
                    fd,
                    pCreateContext->ResolvedFileName,
                    !pCreateContext->bFileExisted);
        }
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

static
NTSTATUS
PvfsCheckDeleteOnClose(
    IN IRP_ARGS_CREATE CreateArgs,
    IN BOOLEAN bFileExists,
    IN ACCESS_MASK GrantedAccess,
    IN FILE_ATTRIBUTES Attributes
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    if (!(CreateArgs.CreateOptions & FILE_DELETE_ON_CLOSE))
    {
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

    switch (CreateArgs.CreateDisposition)
    {
    case FILE_OPEN:
    case FILE_OPEN_IF:
    case FILE_SUPERSEDE:
    case FILE_CREATE:
        if (Attributes & FILE_ATTRIBUTE_READONLY)
        {
            ntError = STATUS_CANNOT_DELETE;
            BAIL_ON_NT_STATUS(ntError);
        }
        break;

    case FILE_OVERWRITE:
    case FILE_OVERWRITE_IF:
        if (bFileExists && (CreateArgs.FileAttributes == 0))
        {
            if (Attributes & FILE_ATTRIBUTE_READONLY)
            {
                ntError = STATUS_CANNOT_DELETE;
                BAIL_ON_NT_STATUS(ntError);
            }
        }
        else
        {
            if (Attributes & FILE_ATTRIBUTE_READONLY)
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

static
NTSTATUS
PvfsCheckReadOnly(
    IN IRP_ARGS_CREATE CreateArgs,
    IN BOOLEAN bFileExists,
    IN ACCESS_MASK GrantedAccess,
    IN FILE_ATTRIBUTES Attributes
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    /* ReadOnly is largely ignored by the file system on directories.
       It is used by the Windows explorer.exe to mark folders as "special" */

    if (!(CreateArgs.CreateOptions & FILE_DIRECTORY_FILE))
    {
        if (Attributes & FILE_ATTRIBUTE_READONLY)
        {
            ntError = STATUS_ACCESS_DENIED;
            BAIL_ON_NT_STATUS(ntError);
        }
    }


cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsCheckDosAttributes(
    IN IRP_ARGS_CREATE CreateArgs,
    IN OPTIONAL PPVFS_FILE_NAME FileName,
    IN ACCESS_MASK GrantedAccess
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    FILE_ATTRIBUTES Attributes = CreateArgs.FileAttributes;
    BOOLEAN bFileExists = (FileName != NULL) ? TRUE : FALSE;

    if (!((CreateArgs.CreateOptions & FILE_DELETE_ON_CLOSE) ||
          (GrantedAccess & FILE_GENERIC_WRITE)))
    {
        goto error;
    }

    if (FileName)
    {
        ntError = PvfsGetFilenameAttributes(
                      PvfsGetCStringBaseFileName(FileName),
                      &Attributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (CreateArgs.CreateOptions & FILE_DELETE_ON_CLOSE)
    {
        ntError = PvfsCheckDeleteOnClose(
                      CreateArgs,
                      bFileExists,
                      GrantedAccess,
                      Attributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    // Enforce ReadOnly on pre-existing files

    if (bFileExists && (GrantedAccess & FILE_WRITE_DATA))
    {
        ntError = PvfsCheckReadOnly(
                      CreateArgs,
                      bFileExists,
                      GrantedAccess,
                      Attributes);
        BAIL_ON_NT_STATUS(ntError);
    }


error:
    return ntError;
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

        if (pCreateCtx->pScb)
        {
            PvfsReleaseSCB(&pCreateCtx->pScb);
        }

        if (pCreateCtx->OriginalFileName)
        {
            PvfsFreeFileName(pCreateCtx->OriginalFileName);
        }

        if (pCreateCtx->ResolvedFileName)
        {
            PvfsFreeFileName(pCreateCtx->ResolvedFileName);
        }

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
    PPVFS_FILE_NAME originalFileName = NULL;

    ntError = PvfsCanonicalPathName2(&originalFileName, Args.FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateMemory((PVOID*)&pCreateCtx, sizeof(*pCreateCtx), TRUE);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateCCB(&pCreateCtx->pCcb);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->OriginalFileName = originalFileName;
    originalFileName = NULL;

    ntError = PvfsAcquireAccessToken(pCreateCtx->pCcb, pSecCtx);
    BAIL_ON_NT_STATUS(ntError);

    pCreateCtx->pIrpContext = PvfsReferenceIrpContext(pIrpContext);

    pCreateCtx->Status = STATUS_SUCCESS;
    pCreateCtx->OplockBroken = FALSE;

    *ppCreate = pCreateCtx;
    pCreateCtx = NULL;

error:
    if (!NT_SUCCESS(ntError))
    {
        if (pCreateCtx)
        {
            PvfsFreeCreateContext((PVOID*)&pCreateCtx);
        }

        if (originalFileName)
        {
            PvfsFreeFileName(originalFileName);
        }
    }

    return ntError;
}



////////////////////////////////////////////////////////////////////////
//
// Check that neither (a) the current stream, nor (b) the containing
// parent directory have the delete-pending state set

NTSTATUS
PvfsCreateFileCheckPendingDelete(
    PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FCB pOwnerParentFcb = NULL;

    // (a) Is the delete-pending state set on this stream

    if (PvfsScbIsPendingDelete(pScb))
    {
        ntError = STATUS_DELETE_PENDING;
        BAIL_ON_NT_STATUS(ntError);
    }

    // (b) Is the delete-pending state enabled for the parent directory

    pOwnerParentFcb = PvfsGetParentFCB(pScb->pOwnerFcb);
    if (pOwnerParentFcb && PvfsFcbIsPendingDelete(pOwnerParentFcb))
    {
        ntError = STATUS_DELETE_PENDING;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    if (pOwnerParentFcb)
    {
        PvfsReleaseFCB(&pOwnerParentFcb);
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
    PPVFS_FILE_NAME pPath,
    BOOLEAN bWasCreated
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_STAT Stat1 = {0};
    PVFS_STAT Stat2 = {0};

    /* Remove the file if it was created and the Dev/Inode matches our fd */

    if (bWasCreated && (pPath != NULL))
    {
        ntError = PvfsSysFstat(fd, &Stat1);
        if (ntError == STATUS_SUCCESS)
        {
            ntError = PvfsSysStatByFileName(pPath, &Stat2);
            if (ntError == STATUS_SUCCESS)
            {
                if ((Stat1.s_dev == Stat2.s_dev) &&
                    (Stat1.s_ino == Stat2.s_ino))
                {
                    ntError = PvfsSysRemoveByFileName(pPath);
                }
            }
        }
    }

    ntError = PvfsSysClose(fd);

    return;
}

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsSetMaximalAccessMask(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PIRP pIrp = pIrpContext->pIrp;
    PACCESS_MASK pulMaxAccessMask = NULL;
    ULONG ulEcpSize = 0;
    PPVFS_FILE_NAME fileName = NULL;

    ntError = IoRtlEcpListFind(
                  pIrp->Args.Create.EcpList,
                  SRV_ECP_TYPE_MAX_ACCESS,
                  OUT_PPVOID(&pulMaxAccessMask),
                  &ulEcpSize);
    if (ntError == STATUS_NOT_FOUND)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }
    BAIL_ON_NT_STATUS(ntError);

    if (ulEcpSize != sizeof(ACCESS_MASK))
    {
        ntError = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAllocateFileNameFromScb(&fileName, pCcb->pScb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAccessCheckFile(
                  pCcb->pUserToken,
                  fileName,
                  MAXIMUM_ALLOWED,
                  pulMaxAccessMask);
    if (ntError != STATUS_SUCCESS)
    {
        *pulMaxAccessMask = 0;
        ntError = STATUS_SUCCESS;
    }

cleanup:
    if (fileName)
    {
        PvfsFreeFileName(fileName);
    }

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsGetEcpShareName(    PIO_ECP_LIST pEcpList,
    PWSTR* ppwszShareName)
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PUNICODE_STRING pShareName = NULL;
    ULONG ulEcpSize = 0;

    ntError = IoRtlEcpListFind(
                  pEcpList,
                  SRV_ECP_TYPE_SHARE_NAME,
                  OUT_PPVOID(&pShareName),
                  &ulEcpSize);
    if (ntError != STATUS_NOT_FOUND)
    {
        BAIL_ON_NT_STATUS(ntError);

        if (ulEcpSize != sizeof(*pShareName))
        {
            ntError = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntError);
        }

        ntError = RtlWC16StringDuplicate(
                        ppwszShareName,
                        pShareName->Buffer);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsSetEcpFileInfo(
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PIRP pIrp = pIrpContext->pIrp;
    PFILE_NETWORK_OPEN_INFORMATION pNetworkOpenInfo = NULL;
    ULONG ulEcpSize = 0;

    ntError = IoRtlEcpListFind(
                    pIrp->Args.Create.EcpList,
                    SRV_ECP_TYPE_NET_OPEN_INFO,
                    OUT_PPVOID(&pNetworkOpenInfo),
                    &ulEcpSize);
    if (ntError == STATUS_SUCCESS)
    {
        if (ulEcpSize != sizeof(FILE_NETWORK_OPEN_INFORMATION))
        {
            ntError = STATUS_INVALID_PARAMETER;
            BAIL_ON_NT_STATUS(ntError);
        }

        ntError = PvfsCcbQueryFileNetworkOpenInformation(
                        pCcb,
                        pNetworkOpenInfo);
        BAIL_ON_NT_STATUS(ntError);

        ntError = IoRtlEcpListAcknowledge(
                        pIrp->Args.Create.EcpList,
                        SRV_ECP_TYPE_NET_OPEN_INFO);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}


