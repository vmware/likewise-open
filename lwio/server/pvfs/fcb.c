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
    if (!pFcb) {
        return;
    }

    RtlCStringFree(&pFcb->pszFilename);
    pthread_mutex_destroy(&pFcb->ControlBlock);
    pthread_rwlock_destroy(&pFcb->rwLock);
    pthread_rwlock_destroy(&pFcb->rwBrlLock);

    LwRtlQueueDestroy(&pFcb->pPendingLockQueue);

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
    if (ppData && *ppData) {
        PVFS_FREE(ppData);
    }

    return;
}

/***********************************************************
 **********************************************************/

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


    ENTER_WRITER_RW_LOCK(&gFcbTable.rwLock);
    ntError = LwRtlRBTreeRemove(gFcbTable.pFcbTree,
                               (PVOID)pFcb->pszFilename);
    LEAVE_WRITER_RW_LOCK(&gFcbTable.rwLock);

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

    ENTER_READER_RW_LOCK(&gFcbTable.rwLock);
    ntError = _PvfsFindFCB(ppFcb, pszFilename);
    LEAVE_READER_RW_LOCK(&gFcbTable.rwLock);

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

    ENTER_WRITER_RW_LOCK(&gFcbTable.rwLock);

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
    LEAVE_WRITER_RW_LOCK(&gFcbTable.rwLock);

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

    ntError = PvfsAllocateMemory((PVOID*)&pCcbNode,
                                 sizeof(PVFS_CCB_LIST_NODE));
    BAIL_ON_NT_STATUS(ntError);

    ENTER_WRITER_RW_LOCK(&pFcb->rwLock);

    /* Add to the front of the list */

    pCcbNode->pCcb  = pCcb;
    pCcbNode->pNext = pFcb->pCcbList;
    if (pFcb->pCcbList) {
        pFcb->pCcbList->pPrevious = pCcbNode;
    }

    pFcb->pCcbList  = pCcbNode;

    LEAVE_WRITER_RW_LOCK(&pFcb->rwLock);

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

    ENTER_WRITER_RW_LOCK(&pFcb->rwLock);

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

    PVFS_FREE(&pCursor);

    ntError = STATUS_SUCCESS;

cleanup:
    LEAVE_WRITER_RW_LOCK(&pFcb->rwLock);

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

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
