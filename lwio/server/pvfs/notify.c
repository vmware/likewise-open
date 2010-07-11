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


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsNotifyAddFilter(
    PPVFS_FCB pFcb,
    PPVFS_IRP_CONTEXT pIrpContext,
    PPVFS_CCB pCcb,
    FILE_NOTIFY_CHANGE NotifyFilter,
    BOOLEAN bWatchTree,
    PULONG pMaxBufferSize
    );

static
NTSTATUS
PvfsNotifyReportBufferedChanges(
    PPVFS_CCB pCcb,
    PPVFS_FCB pFcb,
    PPVFS_IRP_CONTEXT pIrpContext
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
    PULONG pMaxBufferSize = Args.MaxBufferSize;

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

    BAIL_ON_INVALID_PTR(Args.Buffer, ntError);
    BAIL_ON_ZERO_LENGTH(Args.Length, ntError);

    /* If we have something in the buffer, return that immediately.  Else
       register a notification filter */

    ntError = PvfsNotifyReportBufferedChanges(
                  pCcb,
                  pCcb->pFcb,
                  pIrpContext);
    if (ntError == STATUS_NOT_FOUND)
    {
        PvfsIrpMarkPending(pIrpContext, PvfsQueueCancelIrp, pIrpContext);

        ntError = PvfsNotifyAddFilter(
                      pCcb->pFcb,
                      pIrpContext,
                      pCcb,
                      Args.NotifyFilter,
                      Args.WatchTree,
                      pMaxBufferSize);
        if (ntError == STATUS_SUCCESS)
        {
            pIrpContext->QueueType = PVFS_QUEUE_TYPE_NOTIFY;

            if (!pIrpContext->pFcb)
            {
                pIrpContext->pFcb = PvfsReferenceFCB(pCcb->pFcb);
            }

            /* Allow the call to be cancelled while in the queue */

            PvfsIrpContextClearFlag(pIrpContext, PVFS_IRP_CTX_FLAG_ACTIVE);

            ntError = STATUS_PENDING;
            goto cleanup;
        }
    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    if (pCcb)
    {
        PvfsReleaseCCB(pCcb);
    }

    return ntError;

error:
    if (PvfsIrpContextCheckFlag(pIrpContext, PVFS_IRP_CTX_FLAG_PENDED))
    {
        pIrpContext->pIrp->IoStatusBlock.Status = ntError;
        PvfsAsyncIrpComplete(pIrpContext);
    }

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsNotifyClearBufferedChanges(
    PPVFS_NOTIFY_FILTER_BUFFER pBuffer
    );


static
NTSTATUS
PvfsNotifyReportBufferedChanges(
    PPVFS_CCB pCcb,
    PPVFS_FCB pFcb,
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    FILE_NOTIFY_CHANGE Filter = pIrpContext->pIrp->Args.ReadDirectoryChange.NotifyFilter;
    PVOID pBuffer = pIrpContext->pIrp->Args.ReadDirectoryChange.Buffer;
    ULONG Length = pIrpContext->pIrp->Args.ReadDirectoryChange.Length;
    PLW_LIST_LINKS pFilterLink = NULL;
    PPVFS_NOTIFY_FILTER_RECORD pFilter = NULL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_MUTEX(bLocked, &pFcb->mutexNotify);

    /* See if we have any changes to report immediately */

    for (pFilterLink = PvfsListTraverse(pFcb->pNotifyListBuffer, NULL);
         pFilterLink;
         pFilterLink = PvfsListTraverse(pFcb->pNotifyListBuffer, pFilterLink))
    {
        pFilter = LW_STRUCT_FROM_FIELD(
                      pFilterLink,
                      PVFS_NOTIFY_FILTER_RECORD,
                      NotifyList);

        /* To return the buffered changes, we have to match the handle
           and the filter */

        if ((pFilter->NotifyFilter != Filter) || (pFilter->pCcb != pCcb))
        {
            continue;
        }

        /* We have a match to check to see if we have anything */

        if ((pFilter->Buffer.Length == 0) ||
            (pFilter->Buffer.Offset == 0))
        {
            ntError = STATUS_NOT_FOUND;
            BAIL_ON_NT_STATUS(ntError);
        }

        /* We have changes....Do we have enough space?  Have we already
           overflowed the buffer? */

        ntError = pFilter->Buffer.Status;
        BAIL_ON_NT_STATUS(ntError);

        if (pFilter->Buffer.Offset > Length)
        {
            PvfsNotifyClearBufferedChanges(&pFilter->Buffer);

            ntError = STATUS_NOTIFY_ENUM_DIR;
            BAIL_ON_NT_STATUS(ntError);
        }

        memcpy(pBuffer, pFilter->Buffer.pData, pFilter->Buffer.Offset);
        pIrpContext->pIrp->IoStatusBlock.BytesTransferred = pFilter->Buffer.Offset;

        PvfsNotifyClearBufferedChanges(&pFilter->Buffer);
    }

    if (pFilterLink == NULL)
    {
        ntError = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }


cleanup:
    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->mutexNotify);

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsNotifyClearBufferedChanges(
    PPVFS_NOTIFY_FILTER_BUFFER pBuffer
    )
{
    memset(pBuffer->pData, 0x0, pBuffer->Length);

    pBuffer->Status = STATUS_SUCCESS;
    pBuffer->Offset = 0;
    pBuffer->pNotify = NULL;
}



/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsNotifyAllocateChangeBuffer(
    PPVFS_NOTIFY_FILTER_BUFFER pBuffer,
    ULONG Length
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    ntError = PvfsAllocateMemory((PVOID*)&pBuffer->pData, Length, TRUE);
    BAIL_ON_NT_STATUS(ntError);

    pBuffer->Length = Length;

    pBuffer->Status = STATUS_SUCCESS;
    pBuffer->Offset = 0;
    pBuffer->pNotify = NULL;

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsNotifyFreeChangeBuffer(
    PPVFS_NOTIFY_FILTER_BUFFER pBuffer
    )
{
    PVFS_FREE(&pBuffer->pData);

    PVFS_ZERO_MEMORY(pBuffer);
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
            PvfsReleaseIrpContext(&pFilter->pIrpContext);
        }

        PvfsNotifyFreeChangeBuffer(&pFilter->Buffer);

        if (pFilter->pCcb)
        {
            PvfsReleaseCCB(pFilter->pCcb);
        }

        PVFS_FREE(ppNotifyRecord);
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
    BOOLEAN bWatchTree,
    PULONG pMaxBufferSize
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

    /* Add a buffer log to this filter if specified.  We'll move
       the record to the buffer list after first processing the Irp */

    if (pMaxBufferSize && (*pMaxBufferSize > 0))
    {
        ntError = PvfsNotifyAllocateChangeBuffer(
                      &pFilter->Buffer,
                      *pMaxBufferSize);
        BAIL_ON_NT_STATUS(ntError);
    }


    LWIO_LOCK_MUTEX(bLocked, &pFcb->mutexNotify);

    ntError = PvfsListAddTail(
                  pFcb->pNotifyListIrp,
                  &pFilter->NotifyList);

    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->mutexNotify);

    BAIL_ON_NT_STATUS(ntError);

cleanup:

    return ntError;

error:
    if (pFilter)
    {
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

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pFilter,
                  sizeof(PVFS_NOTIFY_FILTER_RECORD),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    pFilter->pIrpContext = PvfsReferenceIrpContext(pIrpContext);
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
    FILE_NOTIFY_CHANGE Filter,
    FILE_ACTION Action,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;
    PPVFS_NOTIFY_REPORT_RECORD pReport = NULL;

    BAIL_ON_INVALID_PTR(pFcb, ntError);

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pReport,
                  sizeof(PVFS_NOTIFY_REPORT_RECORD),
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    pReport->pFcb = PvfsReferenceFCB(pFcb);
    pReport->Filter = Filter;
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

cleanup:

    return;

error:
    PvfsNotifyFullReportCtxFree(&pReport);
    PvfsFreeWorkContext(&pWorkCtx);

    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsNotifyFullReportBuffer(
    PPVFS_FCB pFcb,
    PPVFS_NOTIFY_REPORT_RECORD pReport
    );

static
VOID
PvfsNotifyFullReportIrp(
    PPVFS_FCB pFcb,
    PPVFS_NOTIFY_REPORT_RECORD pReport
    );

static
NTSTATUS
PvfsNotifyFullReport(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_NOTIFY_REPORT_RECORD pReport = (PPVFS_NOTIFY_REPORT_RECORD)pContext;
    PPVFS_FCB pParentFcb = NULL;
    BOOLEAN bLocked = FALSE;
    PPVFS_FCB pCursor = NULL;

    BAIL_ON_INVALID_PTR(pReport, ntError);

    /* Simply walk up the ancestory and process the notify filter
       record on top if there is a match */

    pCursor = PvfsReferenceFCB(pReport->pFcb);

    while ((pParentFcb = PvfsGetParentFCB(pCursor)) != NULL)
    {
        LWIO_LOCK_MUTEX(bLocked, &pParentFcb->mutexNotify);

        /* Process buffers before Irp so we don't doublt report
           a change on a pending Irp that has requested buffering a
           change log (which shouldn't start until the existing Irp
           has been completed). */

        PvfsNotifyFullReportBuffer(pParentFcb, pReport);
        PvfsNotifyFullReportIrp(pParentFcb, pReport);

        LWIO_UNLOCK_MUTEX(bLocked, &pParentFcb->mutexNotify);

        PvfsReleaseFCB(&pCursor);

        pCursor = pParentFcb;
    }


cleanup:
    if (pCursor)
    {
        PvfsReleaseFCB(&pCursor);
    }

    return ntError;

error:
    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsNotifyReportBuffer(
    PPVFS_NOTIFY_FILTER_BUFFER pFilterBuffer,
    FILE_ACTION Action,
    PCSTR pszFilename
    );

static
VOID
PvfsNotifyFullReportBuffer(
    PPVFS_FCB pFcb,
    PPVFS_NOTIFY_REPORT_RECORD pReport
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PLW_LIST_LINKS pFilterLink = NULL;
    PPVFS_NOTIFY_FILTER_RECORD pFilter = NULL;
    PPVFS_FCB pReportParentFcb = NULL;

    for (pFilterLink = PvfsListTraverse(pFcb->pNotifyListBuffer, NULL);
         pFilterLink;
         pFilterLink = PvfsListTraverse(pFcb->pNotifyListBuffer, pFilterLink))
    {
        pFilter = LW_STRUCT_FROM_FIELD(
                      pFilterLink,
                      PVFS_NOTIFY_FILTER_RECORD,
                      NotifyList);

        /* Match the filter and depth */

        if (pReportParentFcb)
        {
            PvfsReleaseFCB(&pReportParentFcb);
        }

        pReportParentFcb = PvfsGetParentFCB(pReport->pFcb);

        if ((pFilter->NotifyFilter & pReport->Filter) &&
            ((pFcb == pReportParentFcb) || pFilter->bWatchTree))
        {
            ntError = PvfsNotifyReportBuffer(
                          &pFilter->Buffer,
                          pReport->Action,
                          pReport->pszFilename);
            break;
        }
    }

    if (pReportParentFcb)
    {
        PvfsReleaseFCB(&pReportParentFcb);
    }


    return;
}



/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsNotifyReportIrp(
    PPVFS_IRP_CONTEXT pIrpContext,
    FILE_ACTION Action,
    PCSTR pszFilename
    );

static
VOID
PvfsNotifyFullReportIrp(
    PPVFS_FCB pFcb,
    PPVFS_NOTIFY_REPORT_RECORD pReport
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PLW_LIST_LINKS pFilterLink = NULL;
    PPVFS_NOTIFY_FILTER_RECORD pFilter = NULL;
    BOOLEAN bActive = FALSE;
    PPVFS_FCB pReportParentFcb = NULL;

    for (pFilterLink = PvfsListTraverse(pFcb->pNotifyListIrp, NULL);
         pFilterLink;
         pFilterLink = PvfsListTraverse(pFcb->pNotifyListIrp, pFilterLink))
    {
        pFilter = LW_STRUCT_FROM_FIELD(
                      pFilterLink,
                      PVFS_NOTIFY_FILTER_RECORD,
                      NotifyList);

        /* Continue if we don't match the filter and depth */

        if (pReportParentFcb)
        {
            PvfsReleaseFCB(&pReportParentFcb);
        }

        pReportParentFcb = PvfsGetParentFCB(pReport->pFcb);

        if (!((pFilter->NotifyFilter & pReport->Filter) &&
              ((pFcb == pReportParentFcb) || pFilter->bWatchTree)))
        {
            pFilter = NULL;
            continue;
        }

        PvfsListRemoveItem(pFcb->pNotifyListIrp, pFilterLink);

        PvfsQueueCancelIrpIfRequested(pFilter->pIrpContext);

        bActive = PvfsIrpContextMarkIfNotSetFlag(
                      pFilter->pIrpContext,
                      PVFS_IRP_CTX_FLAG_CANCELLED,
                      PVFS_IRP_CTX_FLAG_ACTIVE);

        if (!bActive)
        {
            PvfsFreeNotifyRecord(&pFilter);
            continue;
        }

        ntError = PvfsNotifyReportIrp(
                      pFilter->pIrpContext,
                      pReport->Action,
                      pReport->pszFilename);
        BAIL_ON_NT_STATUS(ntError);

        /* If we have been asked to buffer changes, move the Fitler Record
           to the buffer list */

        if (pFilter->Buffer.Length > 0)
        {
            ntError = PvfsListAddTail(pFcb->pNotifyListBuffer, pFilterLink);
            BAIL_ON_NT_STATUS(ntError);

            pFilter = NULL;
        }

        /* We only process on matching IRP */
        break;
    }

cleanup:

    if (pReportParentFcb)
    {
        PvfsReleaseFCB(&pReportParentFcb);
    }

    if (pFilter)
    {
        PvfsFreeNotifyRecord(&pFilter);
    }

    return;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsNotifyReportIrp(
    PPVFS_IRP_CONTEXT pIrpContext,
    FILE_ACTION Action,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVOID pBuffer = pIrpContext->pIrp->Args.ReadDirectoryChange.Buffer;
    ULONG Length = pIrpContext->pIrp->Args.ReadDirectoryChange.Length;
    PFILE_NOTIFY_INFORMATION pNotifyInfo = NULL;
    LONG FilenameBytes = 0;
    PWSTR pwszFilename = NULL;
    ULONG BytesNeeded = 0;

    ntError = LwRtlWC16StringAllocateFromCString(&pwszFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    FilenameBytes = (LwRtlWC16StringNumChars(pwszFilename) + 1) * sizeof(WCHAR);

    BytesNeeded = sizeof(*pNotifyInfo) + FilenameBytes;

    if (Length < BytesNeeded)
    {
        memset(pBuffer, 0x0, Length);
        ntError = STATUS_NOTIFY_ENUM_DIR;
        BAIL_ON_NT_STATUS(ntError);
    }

    pNotifyInfo = (PFILE_NOTIFY_INFORMATION)pBuffer;

    pNotifyInfo->NextEntryOffset = 0;
    pNotifyInfo->Action = Action;
    pNotifyInfo->FileNameLength = FilenameBytes;

    memcpy(&pNotifyInfo->FileName, pwszFilename, FilenameBytes);

    pIrpContext->pIrp->IoStatusBlock.BytesTransferred = BytesNeeded;

cleanup:
    pIrpContext->pIrp->IoStatusBlock.Status = ntError;

    PvfsAsyncIrpComplete(pIrpContext);

    LwRtlWC16StringFree(&pwszFilename);

    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

static
NTSTATUS
PvfsNotifyReportBuffer(
    PPVFS_NOTIFY_FILTER_BUFFER pFilterBuffer,
    FILE_ACTION Action,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVOID pBuffer = pFilterBuffer->pData + pFilterBuffer->Offset;
    ULONG Length = pFilterBuffer->Length - pFilterBuffer->Offset;
    PFILE_NOTIFY_INFORMATION pNotifyInfo = NULL;
    LONG FilenameBytes = 0;
    PWSTR pwszFilename = NULL;
    ULONG BytesNeeded = 0;

    /* Don't bother if we have already overflowed the buffer */

    BAIL_ON_NT_STATUS(pFilterBuffer->Status);

    ntError = LwRtlWC16StringAllocateFromCString(&pwszFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    FilenameBytes = (LwRtlWC16StringNumChars(pwszFilename) + 1) * sizeof(WCHAR);
    BytesNeeded = sizeof(*pNotifyInfo) + FilenameBytes;
    PVFS_ALIGN_MEMORY(BytesNeeded, 8);

    if (Length < BytesNeeded)
    {
        ntError = pFilterBuffer->Status = STATUS_NOTIFY_ENUM_DIR;
        BAIL_ON_NT_STATUS(ntError);
    }

    pNotifyInfo = (PFILE_NOTIFY_INFORMATION)pBuffer;

    pNotifyInfo->NextEntryOffset = 0;
    pNotifyInfo->Action = Action;
    pNotifyInfo->FileNameLength = FilenameBytes;

    memcpy(&pNotifyInfo->FileName, pwszFilename, FilenameBytes);

    if (pFilterBuffer->pNotify)
    {
        ULONG NextEntry = PVFS_PTR_DIFF(pFilterBuffer->pNotify, pNotifyInfo);

        pFilterBuffer->pNotify->NextEntryOffset = NextEntry;
    }

    pFilterBuffer->pNotify = pNotifyInfo;
    pFilterBuffer->Offset += BytesNeeded;


cleanup:

    LwRtlWC16StringFree(&pwszFilename);

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
            PvfsReleaseFCB(&pReport->pFcb);
        }

        LwRtlCStringFree(&pReport->pszFilename);

        PVFS_FREE(ppReport);
    }

    return;
}



/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsNotifyCleanIrpList(
    PVOID pContext
    );

static VOID
PvfsNotifyCleanIrpListFree(
    PVOID *ppContext
    );

NTSTATUS
PvfsScheduleCancelNotify(
    PPVFS_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_WORK_CONTEXT pWorkCtx = NULL;
    PPVFS_IRP_CONTEXT pIrpCtx = NULL;

    BAIL_ON_INVALID_PTR(pIrpContext->pFcb, ntError);

    pIrpCtx = PvfsReferenceIrpContext(pIrpContext);

    ntError = PvfsCreateWorkContext(
                  &pWorkCtx,
                  FALSE,
                  pIrpCtx,
                  (PPVFS_WORK_CONTEXT_CALLBACK)PvfsNotifyCleanIrpList,
                  (PPVFS_WORK_CONTEXT_FREE_CTX)PvfsNotifyCleanIrpListFree);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAddWorkItem(gpPvfsInternalWorkQueue, (PVOID)pWorkCtx);
    BAIL_ON_NT_STATUS(ntError);

cleanup:

    return ntError;

error:
    if (pIrpCtx)
    {
        PvfsReleaseIrpContext(&pIrpCtx);
    }

    PvfsFreeWorkContext(&pWorkCtx);

    goto cleanup;
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsNotifyCleanIrpList(
    PVOID pContext
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_IRP_CONTEXT pIrpCtx = (PPVFS_IRP_CONTEXT)pContext;
    PPVFS_FCB pFcb = PvfsReferenceFCB(pIrpCtx->pFcb);
    BOOLEAN bFcbLocked = FALSE;
    PPVFS_NOTIFY_FILTER_RECORD pFilter = NULL;
    PLW_LIST_LINKS pFilterLink = NULL;
    PLW_LIST_LINKS pNextLink = NULL;
    BOOLEAN bFound = FALSE;

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->mutexNotify);

    pFilterLink = PvfsListTraverse(pFcb->pNotifyListIrp, NULL);

    while (pFilterLink)
    {
        pFilter = LW_STRUCT_FROM_FIELD(
                      pFilterLink,
                      PVFS_NOTIFY_FILTER_RECORD,
                      NotifyList);

        pNextLink = PvfsListTraverse(pFcb->pNotifyListIrp, pFilterLink);

        if (pFilter->pIrpContext != pIrpCtx)
        {
            pFilterLink = pNextLink;
            continue;
        }

        bFound = TRUE;

        PvfsListRemoveItem(pFcb->pNotifyListIrp, pFilterLink);
        pFilterLink = NULL;

        pFilter->pIrpContext->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsAsyncIrpComplete(pFilter->pIrpContext);

        PvfsFreeNotifyRecord(&pFilter);

        /* Can only be one IrpContext match so we are done */
    }

    if (!bFound)
    {
        pIrpCtx->pIrp->IoStatusBlock.Status = STATUS_CANCELLED;

        PvfsAsyncIrpComplete(pIrpCtx);
    }

    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->mutexNotify);

    if (pFcb)
    {
        PvfsReleaseFCB(&pFcb);
    }

    if (pIrpCtx)
    {
        PvfsReleaseIrpContext(&pIrpCtx);
    }

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

static VOID
PvfsNotifyCleanIrpListFree(
    PVOID *ppContext
    )
{
    /* No op -- context released in PvfsNotifyCleanIrpList */
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
