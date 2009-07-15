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
 *        fcb.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        File Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"

/* Forward declarations */

/* File Globals */

typedef struct _PVFS_FCB_TABLE
{
    pthread_rwlock_t rwLock;

    PLWRTL_RB_TREE pFcbTree;

} PVFS_FCB_TABLE;

static PVFS_FCB_TABLE gFcbTable;


/* Code */

/*******************************************************
 ******************************************************/

static VOID
PvfsFreeFCB(
    PPVFS_FCB pFcb
    )
{
    PLW_LIST_LINKS pLinks = NULL;
    PPVFS_OPLOCK_RECORD pOplockRec = NULL;
    PIRP pIrp = NULL;

    if (!pFcb) {
        return;
    }

    RtlCStringFree(&pFcb->pszFilename);
    pthread_mutex_destroy(&pFcb->ControlBlock);
    pthread_rwlock_destroy(&pFcb->rwLock);
    pthread_rwlock_destroy(&pFcb->rwBrlLock);

    LwRtlQueueDestroy(&pFcb->pPendingLockQueue);
    LwRtlQueueDestroy(&pFcb->pOplockPendingOpsQueue);

    while (!LwListIsEmpty(&pFcb->OplockList))
    {
        pLinks = LwListRemoveTail(&pFcb->OplockList);
        pOplockRec = LW_STRUCT_FROM_FIELD(pLinks, PVFS_OPLOCK_RECORD, Oplocks);

        pIrp = pOplockRec->pIrpContext->pIrp;

        pIrp->IoStatusBlock.Status = STATUS_FILE_CLOSED;
        IoIrpComplete(pIrp);

        PvfsFreeIrpContext(&pOplockRec->pIrpContext);
        PvfsFreeOplockRecord(&pOplockRec);
    }

    PVFS_FREE(&pFcb);

    return;
}

/***********************************************************
 **********************************************************/

VOID
PvfsFreePendingLock(
    PVOID *ppData
    )
{
    if (!ppData || !*ppData) {
        return;
    }

    PVFS_FREE(ppData);

    return;
}

NTSTATUS
PvfsAllocateFCB(
    PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    LONG NewRefCount = 0;

    *ppFcb = NULL;

    ntError = PvfsAllocateMemory((PVOID*)&pFcb, sizeof(PVFS_FCB));
    BAIL_ON_NT_STATUS(ntError);

    /* Setup pendlock byte-range lock queue */

    ntError = LwRtlQueueInit(&pFcb->pPendingLockQueue,
                             PVFS_FCB_MAX_PENDING_LOCKS,
                             PvfsFreePendingLock);
    BAIL_ON_NT_STATUS(ntError);

    /* Pending create queue */

    ntError = LwRtlQueueInit(&pFcb->pOplockPendingOpsQueue,
                             PVFS_FCB_MAX_PENDING_OPERATIONS,
                             PvfsFreeCreateContext);
    BAIL_ON_NT_STATUS(ntError);

    LwListInit(&pFcb->OplockList);

    /* Initialize mutexes and refcounts */

    pthread_mutex_init(&pFcb->ControlBlock, NULL);
    pthread_rwlock_init(&pFcb->rwLock, NULL);
    pthread_rwlock_init(&pFcb->rwBrlLock, NULL);

    /* Add initial ref count */

    pFcb->RefCount = 0;
    NewRefCount = InterlockedIncrement(&pFcb->RefCount);

    *ppFcb = pFcb;
    pFcb = NULL;

    ntError = STATUS_SUCCESS;

cleanup:
    PvfsFreeFCB(pFcb);

    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

static NTSTATUS
PvfsRemoveFCB(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &gFcbTable.rwLock);

    ntError = LwRtlRBTreeRemove(gFcbTable.pFcbTree,
                               (PVOID)pFcb->pszFilename);

    LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/


static NTSTATUS
SetLastWriteTime(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVFS_STAT Stat = {0};
    LONG64 LastAccessTime = 0;

    /* Need the original access time */

    ntError = PvfsSysStat(pFcb->pszFilename, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsUnixToWinTime(&LastAccessTime, Stat.s_atime);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsSysUtime(pFcb->pszFilename,
                           pFcb->LastWriteTime,
                           LastAccessTime);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;

}

VOID
PvfsReleaseFCB(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    if (InterlockedDecrement(&pFcb->RefCount) == 0)
    {
        PvfsRemoveFCB(pFcb);

        /* sticky write times */

        if (pFcb->LastWriteTime != 0) {
            ntError = SetLastWriteTime(pFcb);
            /* Don't fail */
        }

        PvfsFreeFCB(pFcb);
    }

    return;
}

/*******************************************************
 ******************************************************/

static int
FcbTableFilenameCompare(
    PVOID a,
    PVOID b
    )
{
    int iReturn = 0;

    PSTR pszFilename1 = (PSTR)a;
    PSTR pszFilename2 = (PSTR)b;

    iReturn = RtlCStringCompare(pszFilename1, pszFilename2, TRUE);

    return iReturn;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsInitializeFCBTable(
    VOID
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    pthread_rwlock_init(&gFcbTable.rwLock, NULL);

    ntError = LwRtlRBTreeCreate(&FcbTableFilenameCompare,
                                NULL,
                                NULL,
                                &gFcbTable.pFcbTree);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsDestroyFCBTable(
    VOID
    )
{
    /* Need to add tree traversal here and remove
       data */

    LwRtlRBTreeFree(gFcbTable.pFcbTree);
    pthread_rwlock_destroy(&gFcbTable.rwLock);

    PVFS_ZERO_MEMORY(&gFcbTable);

    return STATUS_SUCCESS;
}

/*******************************************************
 ******************************************************/

static NTSTATUS
_PvfsFindFCB(
    PPVFS_FCB *ppFcb,
    PSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    LONG NewRefCount = 0;

    ntError = LwRtlRBTreeFind(gFcbTable.pFcbTree,
                              (PVOID)pszFilename,
                              (PVOID*)&pFcb);
    if (ntError == STATUS_NOT_FOUND) {
        ntError = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    BAIL_ON_NT_STATUS(ntError);

    NewRefCount = InterlockedIncrement(&pFcb->RefCount);

    *ppFcb = pFcb;
    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsFindFCB(
    PPVFS_FCB *ppFcb,
    PSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bLocked, &gFcbTable.rwLock);

    ntError = _PvfsFindFCB(ppFcb, pszFilename);

    LWIO_UNLOCK_RWMUTEX(bLocked, &gFcbTable.rwLock);

    return ntError;
}


/*******************************************************
 ******************************************************/

static NTSTATUS
PvfsAddFCB(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;

    ntError = LwRtlRBTreeAdd(gFcbTable.pFcbTree,
                             (PVOID)pFcb->pszFilename,
                             (PVOID)pFcb);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsCreateFCB(
    OUT PPVFS_FCB *ppFcb,
    IN  PSTR pszFilename,
    IN  FILE_SHARE_FLAGS SharedAccess,
    IN  ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    BOOLEAN bFcbTableLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbTableLocked, &gFcbTable.rwLock);

    /* Protect against adding a duplicate */

    ntError = _PvfsFindFCB(&pFcb, pszFilename);
    if (ntError == STATUS_SUCCESS) {

        ntError = PvfsEnforceShareMode(pFcb,
                                       SharedAccess,
                                       DesiredAccess);
        BAIL_ON_NT_STATUS(ntError);

        *ppFcb = pFcb;

        goto cleanup;
    }

    ntError = PvfsAllocateFCB(&pFcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCStringDuplicate(&pFcb->pszFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Add to the file handle table */

    ntError = PvfsAddFCB(pFcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Return a reference to the FCB */

    *ppFcb = pFcb;
    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbTableLocked, &gFcbTable.rwLock);

    return ntError;

error:
    if (pFcb) {
        PvfsReleaseFCB(pFcb);
    }
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsAddCCBToFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB_LIST_NODE pCcbNode = NULL;
    BOOLEAN bFcbWriteLocked = FALSE;

    ntError = PvfsAllocateMemory((PVOID*)&pCcbNode,
                                 sizeof(PVFS_CCB_LIST_NODE));
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwLock);

    /* Add to the front of the list */

    pCcbNode->pCcb  = pCcb;
    pCcbNode->pNext = pFcb->pCcbList;
    if (pFcb->pCcbList) {
        pFcb->pCcbList->pPrevious = pCcbNode;
    }

    pFcb->pCcbList  = pCcbNode;
    pFcb->CcbCount++;

    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwLock);

    pCcb->pFcb = pFcb;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsRemoveCCBFromFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB_LIST_NODE pCursor = NULL;
    PPVFS_CCB_LIST_NODE pTmp = NULL;
    BOOLEAN bFcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwLock);

    for (pCursor=pFcb->pCcbList; pCursor; pCursor = pCursor->pNext)
    {
        if (pCursor->pCcb == pCcb) {
            break;
        }
        pTmp = pCursor;
    }

    if (!pCursor) {
        ntError = STATUS_NOT_FOUND;
        BAIL_ON_NT_STATUS(ntError);
    }

    if (pCursor == pFcb->pCcbList) {
        pFcb->pCcbList = pCursor->pNext;
        if (pFcb->pCcbList) {
            pFcb->pCcbList->pPrevious = NULL;
        }
    } else {
        pTmp->pNext = pCursor->pNext;
        if (pCursor->pNext) {
            pCursor->pNext->pPrevious = pTmp;
        }

    }

    pFcb->CcbCount--;
    PVFS_FREE(&pCursor);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwLock);

    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

PPVFS_CCB_LIST_NODE
PvfsNextCCBFromList(
    PPVFS_FCB pFcb,
    PPVFS_CCB_LIST_NODE pCurrent
    )
{
    if (pCurrent == NULL) {
        return pFcb->pCcbList;
    }

    return pCurrent->pNext;
}

/*******************************************************
 ******************************************************/

PPVFS_CCB_LIST_NODE
PvfsPreviousCCBFromList(
    PPVFS_FCB pFcb,
    PPVFS_CCB_LIST_NODE pCurrent
    )
{
    if (pCurrent == NULL) {
        return NULL;
    }

    return pCurrent->pPrevious;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsAddItemPendingOplockBreakAck(
    IN OUT PPVFS_FCB pFcb,
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PPVFS_OPLOCK_PENDING_COMPLETION_CALLBACK pfnCompletion,
    IN     PPVFS_OPLOCK_PENDING_COMPLETION_FREE pfnFree,
    IN     PVOID pCompletionContext
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;
    PPVFS_OPLOCK_PENDING_OPERATION pPendingOp = NULL;

    BAIL_ON_INVALID_PTR(pFcb, ntError);
    BAIL_ON_INVALID_PTR(pfnCompletion, ntError);

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pPendingOp,
                  sizeof(PVFS_OPLOCK_PENDING_OPERATION));
    BAIL_ON_NT_STATUS(ntError);

    pPendingOp->pIrpContext = pIrpContext;
    pPendingOp->pfnCompletion = pfnCompletion;
    pPendingOp->pfnFree = pfnFree;
    pPendingOp->pCompletionContext = pCompletionContext;

    LWIO_LOCK_MUTEX(bLocked, &pFcb->ControlBlock);

    ntError = LwRtlQueueAddItem(pFcb->pOplockPendingOpsQueue,
                                (PVOID)pPendingOp);

    LWIO_UNLOCK_MUTEX(bLocked, &pFcb->ControlBlock);

    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFileHasOtherOpens(
    IN PPVFS_FCB pFcb,
    IN PPVFS_CCB pCcb
    )
{
    PPVFS_CCB_LIST_NODE pCursor = NULL;
    BOOLEAN bNonSelfOpen = FALSE;
    BOOLEAN bFcbReadLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pFcb->rwLock);

    for (pCursor = PvfsNextCCBFromList(pFcb, pCursor);
         pCursor;
         pCursor = PvfsNextCCBFromList(pFcb, pCursor))
    {
        /* We'll deal with ourselves in AddLock() */

        if (pCcb != pCursor->pCcb) {
            bNonSelfOpen = TRUE;
            break;
        }
    }

    LWIO_UNLOCK_RWMUTEX(bFcbReadLocked, &pFcb->rwLock);

    return bNonSelfOpen;
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFileIsOplocked(
    IN PPVFS_FCB pFcb
    )
{
    return !LwListIsEmpty(&pFcb->OplockList);
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFileIsOplockedExclusive(
    IN PPVFS_FCB pFcb
    )
{
    BOOLEAN bExclusiveOplock = FALSE;
    PPVFS_OPLOCK_RECORD pOplockRec = NULL;

    if (LwListIsEmpty(&pFcb->OplockList)) {
        return FALSE;
    }

    /* We only need to check the list head since the list itself must
       be consistent and non-conflicting */

    pOplockRec = LW_STRUCT_FROM_FIELD(
                     &pFcb->OplockList,
                     PVFS_OPLOCK_RECORD,
                     Oplocks);

    if ((pOplockRec->OplockType == IO_OPLOCK_REQUEST_OPLOCK_BATCH) ||
        (pOplockRec->OplockType == IO_OPLOCK_REQUEST_OPLOCK_LEVEL_1))
    {
        bExclusiveOplock = TRUE;
    }

    return bExclusiveOplock;
}

/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFileIsOplockedShared(
    IN PPVFS_FCB pFcb
    )
{
    if (LwListIsEmpty(&pFcb->OplockList)) {
        return FALSE;
    }

    return !PvfsFileIsOplockedExclusive(pFcb);
}

/*****************************************************************************
 ****************************************************************************/

static NTSTATUS
PvfsStoreOplockRecord(
    IN OUT PPVFS_FCB pFcb,
    IN     PPVFS_OPLOCK_RECORD pOplock
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    LwListInsertTail(&pFcb->OplockList, &pOplock->Oplocks);

    return ntError;
}

NTSTATUS
PvfsAddOplockRecord(
    IN OUT PPVFS_FCB pFcb,
    IN     PPVFS_IRP_CONTEXT pIrpContext,
    IN     PPVFS_CCB pCcb,
    IN     ULONG OplockType
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_OPLOCK_RECORD pOplock = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pOplock,
                  sizeof(PVFS_OPLOCK_RECORD));
    BAIL_ON_NT_STATUS(ntError);

    pOplock->OplockType = OplockType;
    pOplock->pCcb = pCcb;
    pOplock->pIrpContext = pIrpContext;

    ntError = PvfsStoreOplockRecord(pFcb, pOplock);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}



/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFreeOplockRecord(
    PPVFS_OPLOCK_RECORD *ppOplockRec
    )
{
    if (!ppOplockRec || !*ppOplockRec) {
        return;
    }

    PvfsReleaseCCB((*ppOplockRec)->pCcb);

    PVFS_FREE(ppOplockRec);

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
