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
PvfsNotifyAddFilter(
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

    ntError = PvfsAccessCheckFileHandle(pCcb,  SYNCHRONIZE);
    BAIL_ON_NT_STATUS(ntError);

    BAIL_ON_INVALID_PTR(Args.Buffer, ntError);
    BAIL_ON_ZERO_LENGTH(Args.Length, ntError);

    ntError = PvfsNotifyAddFilter(
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
PvfsFreeNotifyRecord(
    PPVFS_NOTIFY_FILTER_RECORD *ppNotifyRecord
    )
{
    PPVFS_NOTIFY_FILTER_RECORD pFilter = NULL;

    if (ppNotifyRecord && *ppNotifyRecord)
    {
        pFilter = *ppNotifyRecord;

        if (pFilter->pIrpContext)
        {
            pFilter->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_FILE_CLOSED;

            PvfsAsyncIrpComplete(pFilter->pIrpContext);
            PvfsFreeIrpContext(&pFilter->pIrpContext);
        }

        if (pFilter->pCcb)
        {
            PvfsReleaseCCB(pFilter->pCcb);
        }
    }

    return;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsNotifyAllocateFilter(
    PPVFS_NOTIFY_FILTER_RECORD *ppNotifyRecord,
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    FILE_NOTIFY_CHANGE NotifyFilter,
    BOOLEAN bWatchTree
    );

static
NTSTATUS
PvfsNotifyAddFilter(
    PPVFS_FCB pFcb,
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    FILE_NOTIFY_CHANGE NotifyFilter,
    BOOLEAN bWatchTree
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_NOTIFY_FILTER_RECORD pFilter = NULL;
    BOOLEAN bLocked = FALSE;

    BAIL_ON_INVALID_PTR(pFcb, ntError);

    ntError = PvfsNotifyAllocateFilter(
                  &pFilter,
                  pIrpContext,
                  pCcb,
                  NotifyFilter,
                  bWatchTree);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_MUTEX(bLocked, &pFcb->mutexNotify);
    ntError = PvfsListAddTail(
                  pFcb->pNotifyList,
                  &pFilter->NotifyList);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->mutexNotify);

    return ntError;

error:
    if (pFilter)
    {
        pFilter->pIrpContext = NULL;
        PvfsFreeNotifyRecord(&pFilter);
    }

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsNotifyAllocateFilter(
    PPVFS_NOTIFY_FILTER_RECORD *ppNotifyRecord,
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    FILE_NOTIFY_CHANGE NotifyFilter,
    BOOLEAN bWatchTree
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_NOTIFY_FILTER_RECORD pFilter = NULL;

    ntError = PvfsAllocateMemory((PVOID*)pFilter, sizeof(*pFilter));
    BAIL_ON_NT_STATUS(ntError);

    pFilter->pIrpContext = pIrpContext;
    pFilter->pCcb = PvfsReferenceCCB(pCcb);
    pFilter->NotifyFilter = NotifyFilter;
    pFilter->bWatchTree = bWatchTree;

    *ppNotifyRecord = pFilter;
    pFilter  = NULL;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsNotifyFullReport(
    PVOID pContext
    );

static VOID
PvfsNotifyFullReportCtxFree(
    PPVFS_NOTIFY_REPORT_RECORD *ppContext
    );

VOID
PvfsNotifyScheduleFullReport(
    PPVFS_FCB pFcb,
    FILE_ACTION Action,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;
    PPVFS_NOTIFY_REPORT_RECORD pReport = NULL;

    BAIL_ON_INVALID_PTR(pFcb, ntError);

    ntError = PvfsAllocateMemory((PVOID*)&pReport, sizeof(*pReport));
    BAIL_ON_NT_STATUS(ntError);

    pReport->pFcb = PvfsReferenceFCB(pFcb);
    pReport->Action = Action;

    ntError = LwRtlCStringDuplicate(&pReport->pszFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  FALSE,
                  pReport,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsNotifyFullReport,
                  (PPVFS_WORK_CONTEXT_FREE_CTX)PvfsNotifyFullReportCtxFree);
    BAIL_ON_NT_STATUS(ntError);

    pReport = NULL;

    ntError = PvfsAddWorkItem(gpPvfsInternalWorkQueue, (PVOID)pWorkCtx);
    BAIL_ON_NT_STATUS(ntError);

    pWorkCtx = NULL;

cleanup:
    PvfsNotifyFullReportCtxFree(&pReport);
    PvfsFreeWorkContext(&pWorkCtx);

    return;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsNotifyFullReport(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_NOTIFY_REPORT_RECORD pReport = (PPVFS_NOTIFY_REPORT_RECORD)pContext;

    BAIL_ON_INVALID_PTR(pReport, ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsNotifyFullReportCtxFree(
    PPVFS_NOTIFY_REPORT_RECORD *ppReport
    )
{
    PPVFS_NOTIFY_REPORT_RECORD pReport = NULL;

    if (ppReport && *ppReport)
    {
        pReport = (PPVFS_NOTIFY_REPORT_RECORD)*ppReport;

        if (pReport->pFcb)
        {
            PvfsReleaseFCB(pReport->pFcb);
        }

        LwRtlCStringFree(&pReport->pszFilename);

        PVFS_FREE(ppReport);
    }

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
