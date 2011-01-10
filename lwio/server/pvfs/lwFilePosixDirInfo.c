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
 *        lwFilePosixDirInfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        LwFilePosixInformation Handler - directory enumeration case
 *
 * Authors: Evgeny Popovich <epopovich@likewise.com>
 */

#include "pvfs.h"


static
NTSTATUS
PvfsQueryLwFilePosixDirInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    );

static
NTSTATUS
FillLwFilePosixDirInfoBuffer(
    PVOID pBuffer,
    ULONG ulBufLen,
    PSTR pszParent,
    PPVFS_DIRECTORY_ENTRY pEntry,
    PULONG pulConsumed
    );


NTSTATUS
PvfsLwFilePosixDirInfo(
    PVFS_INFO_TYPE Type,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    switch(Type)
    {
    case PVFS_SET:
        ntError = STATUS_NOT_SUPPORTED;
        break;

    case PVFS_QUERY:
        ntError = PvfsQueryLwFilePosixDirInfo(pIrpContext);
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


static
NTSTATUS
PvfsQueryLwFilePosixDirInfo(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    PPVFS_CCB pCcb = NULL;
    PLW_FILE_POSIX_DIR_INFORMATION pFileInfo = NULL;
    PLW_FILE_POSIX_DIR_INFORMATION pPrevFileInfo = NULL;
    IRP_ARGS_QUERY_DIRECTORY* pArgs = &pIrpContext->pIrp->Args.QueryDirectory;
    PVOID pBuffer = NULL;
    ULONG ulBufLen = 0;
    ULONG ulOffset = 0;
    ULONG ulConsumed = 0;
    BOOLEAN bLocked = FALSE;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (!PVFS_IS_DIR(pCcb))
    {
        ntError = STATUS_NOT_A_DIRECTORY;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAccessCheckFileHandle(pCcb, FILE_LIST_DIRECTORY);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(pArgs->FileInformation, ntError);

    if (pArgs->Length < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo = (PLW_FILE_POSIX_DIR_INFORMATION)pArgs->FileInformation;

    /* Scan the first time through */

    ntError = STATUS_SUCCESS;

    if (pArgs->RestartScan)
    {
        /* Critical region to prevent inteleaving directory
           enumeration */
        LWIO_LOCK_MUTEX(bLocked, &pCcb->ControlBlock);

        ntError = PvfsEnumerateDirectory(
                        pCcb,
                        pArgs->FileSpec,
                        -1,
                        TRUE);

        LWIO_UNLOCK_MUTEX(bLocked, &pCcb->ControlBlock);
    }

    BAIL_ON_NT_STATUS(ntError);

    /* Check for ending condition */

    if (pCcb->pDirContext->dwIndex == pCcb->pDirContext->dwNumEntries)
    {
        ntError = STATUS_NO_MORE_MATCHES;
        BAIL_ON_NT_STATUS(ntError);
    }


    /* Fill in the buffer */

    pBuffer = pArgs->FileInformation;
    ulBufLen = pArgs->Length;
    ulOffset = 0;
    pFileInfo = NULL;
    pPrevFileInfo = NULL;

    do
    {
        PPVFS_DIRECTORY_ENTRY pEntry = NULL;
        ULONG ulIndex = 0;

        pFileInfo = (PLW_FILE_POSIX_DIR_INFORMATION)(pBuffer + ulOffset);
        pFileInfo->NextEntryOffset = 0;

        ulIndex = pCcb->pDirContext->dwIndex;
        pEntry  = &pCcb->pDirContext->pDirEntries[ulIndex];
        ntError = FillLwFilePosixDirInfoBuffer(
                      pFileInfo,
                      ulBufLen - ulOffset,
                      pCcb->pszFilename,
                      pEntry,
                      &ulConsumed);

        /* If we ran out of buffer space, reset pointer to previous
           entry and break out of loop */

        if (ntError == STATUS_BUFFER_TOO_SMALL) {
            pFileInfo = pPrevFileInfo;
            break;
        }

        /* OBJECT_NAME_NOT_FOUND - This deals with a possible race
           where the directory contents was read but the file was
           removed before we could stat() it.

           INSUFFICIENT_RESOURCES - Invalid UTF-8 name.

           ACCESS_DENIED - Special cases like $HOME/.gvfs that can't
           be read by root.  Possibly just an Ubuntu bug but don't
           fail on it.
           https://bugs.launchpad.net/ubuntu/+source/gvfs/+bug/227724

           Just skip the file and move on. */

        if (ntError == STATUS_OBJECT_NAME_NOT_FOUND ||
            ntError == STATUS_INSUFFICIENT_RESOURCES ||
            ntError == STATUS_ACCESS_DENIED)
        {
            pFileInfo = pPrevFileInfo;
            pCcb->pDirContext->dwIndex++;
            continue;
        }

        /* Catch any other errors and bail */

        BAIL_ON_NT_STATUS(ntError);

        pFileInfo->NextEntryOffset = ulConsumed;

        ulOffset += ulConsumed;
        pCcb->pDirContext->dwIndex++;

        pPrevFileInfo = pFileInfo;

        if (pArgs->ReturnSingleEntry) {
            break;
        }
    }
    /* Exit loop when we are out of buffer or out of entries.  The
       filling function can also break us out of the loop. */
    while (((ulBufLen - ulOffset) > sizeof(LW_FILE_POSIX_DIR_INFORMATION)) &&
             (pCcb->pDirContext->dwIndex < pCcb->pDirContext->dwNumEntries));

    /* Update final offset */

    if (pFileInfo) {
        pFileInfo->NextEntryOffset = 0;
    }

    pIrp->IoStatusBlock.BytesTransferred = ulOffset;
    ntError = STATUS_SUCCESS;

cleanup:
    if (pCcb) {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}

/**
 * Returns:
 *   STATUS_BUFFER_TOO_SMALL (not enough space)
 *   STATUS_OBJECT_NAME_NOT_FOUND (stat() failed)
 **/

static
NTSTATUS
FillLwFilePosixDirInfoBuffer(
    PVOID pBuffer,
    ULONG ulBufLen,
    PSTR pszParent,
    PPVFS_DIRECTORY_ENTRY pEntry,
    PULONG pulConsumed
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PLW_FILE_POSIX_DIR_INFORMATION pFileInfo = (PLW_FILE_POSIX_DIR_INFORMATION)pBuffer;
    PSTR pszFullPath = NULL;
    ULONG ulNeeded = 0;
    ULONG ulFilenameLen = 0;

    /* Check for enough space for static members */

    if (ulBufLen < sizeof(*pFileInfo))
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    /* Build the absolute path and stat() it */

    ntError = RtlCStringAllocatePrintf(
                  &pszFullPath,
                  "%s/%s",
                  pszParent,
                  pEntry->pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysStat(pszFullPath, &pEntry->Stat);
    BAIL_ON_NT_STATUS(ntError);

    pFileInfo->PosixInformation.UnixAccessTime = pEntry->Stat.s_atime;
    pFileInfo->PosixInformation.UnixModificationTime = pEntry->Stat.s_mtime;
    pFileInfo->PosixInformation.UnixChangeTime = pEntry->Stat.s_ctime;
    pFileInfo->PosixInformation.EndOfFile = pEntry->Stat.s_size;
    pFileInfo->PosixInformation.AllocationSize = pEntry->Stat.s_alloc;
    pFileInfo->PosixInformation.UnixNumberOfLinks = pEntry->Stat.s_nlink;
    pFileInfo->PosixInformation.UnixMode = pEntry->Stat.s_mode;
    pFileInfo->PosixInformation.Uid = pEntry->Stat.s_uid;
    pFileInfo->PosixInformation.Gid = pEntry->Stat.s_gid;

    // TODO - fill in the correct volume id
    // pFileInfo->PosixInformation.VolumeId = pEntry->Stat.s_dev;
    pFileInfo->PosixInformation.VolumeId = 1;

    // TODO - is this the file id we want?  Also get the generation number.
    // pFileInfo->PosixInformation.InodeNumber = pEntry->Stat.s_ino;
    // pFileInfo->PosixInformation.GenerationNumber = 0;
    // Hack now - encode path instead of inode/generation numbers
    pFileInfo->PosixInformation.GenerationNumber = 0;
    strcpy((char*)&pFileInfo->PosixInformation.InodeNumber, "/pvfs");
    strncpy(((char*)&pFileInfo->PosixInformation.InodeNumber) + 5, pszFullPath, 10);

    ntError = PvfsGetFilenameAttributes(
                  pszFullPath,
                  &pFileInfo->PosixInformation.FileAttributes);
    BAIL_ON_NT_STATUS(ntError);

    /* Take care of filename */

    ulFilenameLen = LwRtlCStringNumChars(pEntry->pszFilename);
    ulNeeded = sizeof(*pFileInfo) + ulFilenameLen;

    /* alignment on 8 byte boundary */

    if (ulNeeded % 8) {
        ulNeeded += 8 - (ulNeeded % 8);
    }

    if (ulNeeded > ulBufLen)
    {
        ntError = STATUS_BUFFER_TOO_SMALL;
        BAIL_ON_NT_STATUS(ntError);
    }

    pFileInfo->FileNameLength = ulFilenameLen;
    memcpy(pFileInfo->FileName, pEntry->pszFilename, ulFilenameLen);


    *pulConsumed = ulNeeded;
    ntError = STATUS_SUCCESS;

cleanup:
    RtlCStringFree(&pszFullPath);

    return ntError;

error:
    goto cleanup;
}
