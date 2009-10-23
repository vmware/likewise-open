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
 *       notify.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Directory Change Notify Package
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/* Code */

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsDirAddWatchRecord(
    PPVFS_FCB pFcb,
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    FILE_NOTIFY_CHANGE NotifyFilter,
    BOOLEAN bWatchTree
    );

NTSTATUS
PvfsReadDirectoryChange(
    PPVFS_IRP_CONTEXT  pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PIRP pIrp = pIrpContext->pIrp;
    IRP_ARGS_READ_DIRECTORY_CHANGE Args = pIrp->Args.ReadDirectoryChange;
    PPVFS_CCB pCcb = NULL;

    /* Sanity checks */

    ntError =  PvfsAcquireCCB(pIrp->FileHandle, &pCcb);
    BAIL_ON_NT_STATUS(ntError);

    if (!PVFS_IS_DIR(pCcb))
    {
        ntError = STATUS_NOT_A_DIRECTORY;
        BAIL_ON_NT_STATUS(ntError);
    }

    ntError = PvfsAccessCheckFileHandle(pCcb,  FILE_LIST_DIRECTORY);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.Buffer, ntError);
    BAIL_ON_ZERO_LENGTH(Args.Length, ntError);

    ntError = PvfsDirAddWatchRecord(
                  pCcb->pFcb,
                  pIrpContext,
                  pCcb,
                  Args.NotifyFilter,
                  Args.WatchTree);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreeWatchDirRecord(
    PPVFS_DIR_WATCH_RECORD *ppWatchRecord
    )
{
    PPVFS_DIR_WATCH_RECORD pWatch = NULL;

    if (ppWatchRecord && *ppWatchRecord)
    {
        pWatch = *ppWatchRecord;

        if (pWatch->pIrpContext)
        {
            pWatch->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_FILE_CLOSED;

            PvfsAsyncIrpComplete(pWatch->pIrpContext);
            PvfsFreeIrpContext(&pWatch->pIrpContext);
        }

        if (pWatch->pCcb)
        {
            PvfsReleaseCCB(pWatch->pCcb);
        }
    }

    return;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsAllocateDirWatchRecord(
    PPVFS_DIR_WATCH_RECORD *ppWatchRecord,
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    FILE_NOTIFY_CHANGE NotifyFilter,
    BOOLEAN bWatchTree
    );

static
NTSTATUS
PvfsDirAddWatchRecord(
    PPVFS_FCB pFcb,
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    FILE_NOTIFY_CHANGE NotifyFilter,
    BOOLEAN bWatchTree
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_DIR_WATCH_RECORD pWatch = NULL;
    BOOLEAN bLocked = FALSE;

    BAIL_ON_INVALID_PTR(pFcb, ntError);

    ntError = PvfsAllocateDirWatchRecord(
                  &pWatch,
                  pIrpContext,
                  pCcb,
                  NotifyFilter,
                  bWatchTree);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_MUTEX(bLocked, &pFcb->mutexNotify);
    ntError = PvfsListAddTail(
                  pFcb->pNotifyList,
                  &pWatch->NotifyList);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->mutexNotify);

    return ntError;

error:
    if (pWatch)
    {
        pWatch->pIrpContext = NULL;
        PvfsFreeWatchDirRecord(&pWatch);
    }

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsAllocateDirWatchRecord(
    PPVFS_DIR_WATCH_RECORD *ppWatchRecord,
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    FILE_NOTIFY_CHANGE NotifyFilter,
    BOOLEAN bWatchTree
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_DIR_WATCH_RECORD pWatch = NULL;

    ntError = PvfsAllocateMemory((PVOID*)pWatch, sizeof(*pWatch));
    BAIL_ON_NT_STATUS(ntError);

    pWatch->pIrpContext = pIrpContext;
    pWatch->pCcb = PvfsReferenceCCB(pCcb);
    pWatch->NotifyFilter = NotifyFilter;
    pWatch->bWatchTree = bWatchTree;

    *ppWatchRecord = pWatch;
    pWatch  = NULL;

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
