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

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->LastAccessTime, Stat.s_atime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->LastWriteTime, Stat.s_mtime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->ChangeTime, Stat.s_ctime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&pFileInfo->CreationTime, Stat.s_crtime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsGetFileAttributes(pCcb, &pFileInfo->FileAttributes);
    BAIL_ON_NT_STATUS(ntError);


cleanup:
    return ntError;

error:
    goto cleanup;

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

    ntError = PvfsValidatePath(pCcb->pFcb, &pCcb->FileId);
    BAIL_ON_NT_STATUS(ntError);

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
        PvfsSetLastWriteTimeFCB(pCcb->pFcb, WriteTime);
    }

    /* Check if we need to preserve any original timestamps */

    if (WriteTime == 0 || AccessTime == 0)
    {
        ntError = PvfsSysFstat(pCcb->fd, &Stat);
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

    ntError = PvfsSysUtime(pCcb->pszFilename, WriteTime, AccessTime);
    BAIL_ON_NT_STATUS(ntError);

    if (pFileInfo->FileAttributes != 0)
    {
        ntError = PvfsSetFileAttributes(pCcb, pFileInfo->FileAttributes);
        BAIL_ON_NT_STATUS(ntError);
    }

    if (NotifyFilter != 0)
    {
        PvfsNotifyScheduleFullReport(
            pCcb->pFcb,
            NotifyFilter,
            FILE_ACTION_MODIFIED,
            pCcb->pFcb->pszFilename);
    }

cleanup:
    return ntError;

error:
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
