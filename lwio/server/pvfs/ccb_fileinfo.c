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
 *        ccb_fileinfo.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Internal File Informatoin handlers
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCcbQueryFileBasicInformation(
    PPVFS_CCB pCcb,
    PFILE_BASIC_INFORMATION pFileInfo
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_STAT Stat = {0};
    PVFS_FILE_NAME baseObjectFileName = { 0 };

    if (PvfsIsDefaultStream(pCcb->pScb))
    {
        ntError = PvfsGetFileAttributes(pCcb, &pFileInfo->FileAttributes);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysFstat(pCcb->fd, &Stat);
        BAIL_ON_NT_STATUS(ntError);
    }
    else
    {
        // Named Streams share meta-data with the owning FCB
        ntError = PvfsBuildFileNameFromCString(
                      &baseObjectFileName,
                      pCcb->pScb->pOwnerFcb->pszFilename);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsGetFilenameAttributes(
                      PvfsGetCStringBaseFileName(&baseObjectFileName),
                      &pFileInfo->FileAttributes);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysStatByFileName(&baseObjectFileName, &Stat);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsUnixToWinTime(&pFileInfo->LastAccessTime, Stat.s_atime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->LastWriteTime, Stat.s_mtime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->ChangeTime, Stat.s_ctime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->CreationTime, Stat.s_crtime);
    BAIL_ON_NT_STATUS(ntError);


error:
    PvfsDestroyFileName(&baseObjectFileName);

    return ntError;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
PvfsCcbSetFileBasicInformation(
    PPVFS_CCB pCcb,
    PFILE_BASIC_INFORMATION pFileInfo
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    LONG64 WriteTime = 0;
    LONG64 AccessTime = 0;
    FILE_NOTIFY_CHANGE NotifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE |
                                      FILE_NOTIFY_CHANGE_LAST_ACCESS;
    PVFS_STAT Stat = {0};
    BOOLEAN fcbControlLocked = FALSE;
    PPVFS_FCB pFcb = PvfsReferenceFCB(pCcb->pScb->pOwnerFcb);

    if (PvfsIsDefaultStream(pCcb->pScb))
    {
        ntError = PvfsValidatePathSCB(pCcb->pScb, &pCcb->FileId);
    }
    else
    {
        // Have to grab the base file object
        LWIO_LOCK_MUTEX(fcbControlLocked, &pFcb->BaseControlBlock.Mutex);

        ntError = PvfsValidatePathFCB(pFcb, &pFcb->FileId);
        BAIL_ON_NT_STATUS(ntError);

        LWIO_UNLOCK_MUTEX(fcbControlLocked, &pFcb->BaseControlBlock.Mutex);
    }

    /* We cant's set the Change Time so ignore it */

    WriteTime  = pFileInfo->LastWriteTime;
    AccessTime = pFileInfo->LastAccessTime;

    /* Ignore 0xffffffff */

    if (WriteTime == (LONG64)-1) {
        WriteTime = 0;
    }

    if (AccessTime == (LONG64)-1) {
        AccessTime = 0;
    }

    /* Save for "sticky" WriteTime sematics */

    if (WriteTime != 0)
    {
        PvfsSetLastWriteTimeFCB(pFcb, WriteTime);
    }

    /* Check if we need to preserve any original timestamps */

    if (WriteTime == 0 || AccessTime == 0)
    {
        if (PvfsIsDefaultStream(pCcb->pScb))
        {
            ntError = PvfsSysFstat(pCcb->fd, &Stat);
        }
        else
        {
            // Have to grab the stat() on the base file object
            ntError = PvfsSysStatByFcb(pFcb, &Stat);
        }
        BAIL_ON_NT_STATUS(ntError);

        if (WriteTime == 0)
        {
            NotifyFilter &= ~FILE_NOTIFY_CHANGE_LAST_WRITE;

            ntError = PvfsUnixToWinTime(&WriteTime, Stat.s_mtime);
            BAIL_ON_NT_STATUS(ntError);
        }

        if (AccessTime == 0)
        {
            NotifyFilter &= ~FILE_NOTIFY_CHANGE_LAST_ACCESS;

            ntError = PvfsUnixToWinTime(&AccessTime, Stat.s_atime);
            BAIL_ON_NT_STATUS(ntError);
        }
    }

    ntError = PvfsSysUtimeByFcb(pFcb, WriteTime, AccessTime);
    BAIL_ON_NT_STATUS(ntError);

    if (pFileInfo->FileAttributes != 0)
    {
        ntError = PvfsSetFileAttributes(pCcb, pFileInfo->FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (NotifyFilter != 0)
    {
        PvfsNotifyScheduleFullReport(
            pFcb,
            NotifyFilter,
            FILE_ACTION_MODIFIED,
            pFcb->pszFilename);
    }

error:
    LWIO_UNLOCK_MUTEX(fcbControlLocked, &pFcb->BaseControlBlock.Mutex);

    if (pFcb)
    {
        PvfsReleaseFCB(&pFcb);
    }

    return ntError;
}

NTSTATUS
PvfsCcbQueryFileStandardInformation(
    PPVFS_CCB pCcb,
    PFILE_STANDARD_INFORMATION pFileInfo
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_STAT Stat = {0};
    BOOLEAN bDeletePending = FALSE;

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    bDeletePending = PvfsScbIsPendingDelete(pCcb->pScb);

    if (PVFS_IS_DIR(pCcb))
    {
        /* NTFS reports the allocation and end-of-file on
           directories as 0.  smbtorture cares about this even
           if no apps that I know of do. */

        pFileInfo->AllocationSize = 0;
        pFileInfo->EndOfFile      = 0;

        pFileInfo->NumberOfLinks  = bDeletePending ? 0 : 1;
    }
    else
    {
        pFileInfo->EndOfFile      = Stat.s_size;
        pFileInfo->AllocationSize = Stat.s_alloc > Stat.s_size ?
                                    Stat.s_alloc : Stat.s_size;

        pFileInfo->NumberOfLinks  = bDeletePending ?
                                    Stat.s_nlink - 1 :
                                    Stat.s_nlink;
    }

    pFileInfo->DeletePending  = bDeletePending;
    pFileInfo->Directory      = S_ISDIR(Stat.s_mode) ? TRUE : FALSE;

cleanup:

    return ntError;

error:

    goto cleanup;

}

NTSTATUS
PvfsCcbQueryFileNetworkOpenInformation(
    PPVFS_CCB                      pCcb,
    PFILE_NETWORK_OPEN_INFORMATION pFileInfo
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_STAT fileStat = { 0 };
    PVFS_STAT streamStat = { 0 };
    PVFS_FILE_NAME baseObjectFileName = { 0 };

    if (PvfsIsDefaultStream(pCcb->pScb))
    {
        ntError = PvfsGetFileAttributes(pCcb, &pFileInfo->FileAttributes);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysFstat(pCcb->fd, &fileStat);
        BAIL_ON_NT_STATUS(ntError);

        streamStat = fileStat;
    }
    else
    {
        // Named Streams share meta-data with the owning FCB
        ntError = PvfsBuildFileNameFromCString(
                      &baseObjectFileName,
                      pCcb->pScb->pOwnerFcb->pszFilename);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsGetFilenameAttributes(
                      PvfsGetCStringBaseFileName(&baseObjectFileName),
                      &pFileInfo->FileAttributes);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysStatByFileName(&baseObjectFileName, &fileStat);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsSysFstat(pCcb->fd, &streamStat);
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsUnixToWinTime(&pFileInfo->LastAccessTime, fileStat.s_atime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->LastWriteTime, fileStat.s_mtime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->ChangeTime, fileStat.s_ctime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->CreationTime, fileStat.s_crtime);
    BAIL_ON_NT_STATUS(ntError);

    pFileInfo->EndOfFile      = streamStat.s_size;
    pFileInfo->AllocationSize = streamStat.s_alloc;

error:
    PvfsDestroyFileName(&baseObjectFileName);

    return ntError;
}

