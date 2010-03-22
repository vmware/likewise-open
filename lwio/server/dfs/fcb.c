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
 *        Likewise  Distributed File System Driver (DFS)
 *
 *        File Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "dfs.h"

/***********************************************************************
 **********************************************************************/

static
int
DfsFcbTablePathnameCompare(
    PVOID a,
    PVOID b
    );

NTSTATUS
DfsInitializeFCBTable(
    VOID
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    pthread_rwlock_init(&gDfsFcbTable.rwLock, NULL);

    ntStatus = LwRtlRBTreeCreate(
                  &DfsFcbTablePathnameCompare,
                  NULL,
                  NULL,
                  &gDfsFcbTable.pFcbTree);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

static
int
DfsFcbTablePathnameCompare(
    PVOID a,
    PVOID b
    )
{
    int Result = 0;

    PSTR pszPathname1 = (PSTR)a;
    PSTR pszPathname2 = (PSTR)b;

    Result = RtlCStringCompare(pszPathname1, pszPathname2, TRUE);

    return Result;
}

/***********************************************************************
 **********************************************************************/

VOID
DfsDestroyFCBTable(
    VOID
    )
{
    LwRtlRBTreeFree(gDfsFcbTable.pFcbTree);
    pthread_rwlock_destroy(&gDfsFcbTable.rwLock);

    LwRtlZeroMemory(&gDfsFcbTable, sizeof(gDfsFcbTable));

    return;
}


/***********************************************************************
 **********************************************************************/

static
VOID
DfsFreeFCB(
    PDFS_FCB pFcb
    )
{
    if (pFcb)
    {
        RtlCStringFree(&pFcb->pszPathname);

        pthread_mutex_destroy(&pFcb->ControlBlock);
        pthread_rwlock_destroy(&pFcb->rwCcbLock);

        DfsListDestroy(&pFcb->pCcbList);

        DfsFreeMemory((PVOID*)&pFcb);

        InterlockedDecrement(&gDfsObjectCounter.FcbCount);
    }

    return;
}


/***********************************************************************
 **********************************************************************/

static
VOID
DfsFCBFreeCCB(
    PVOID *ppData
    )
{
    /* This should never be called.  The CCB count has to be 0 when
       we call DfsFreeFCB() and hence destroy the FCB->pCcbList */

    return;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsAllocateFCB(
    PDFS_FCB *ppFcb
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDFS_FCB pFcb = NULL;

    *ppFcb = NULL;

    ntStatus = DfsAllocateMemory((PVOID*)&pFcb, sizeof(DFS_FCB));
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pFcb->ControlBlock, NULL);
    pthread_rwlock_init(&pFcb->rwCcbLock, NULL);

    pFcb->RefCount = 1;

    /* List of CCBs */

    ntStatus = DfsListInit(
                  &pFcb->pCcbList,
                  0,
                  (PDFS_LIST_FREE_DATA_FN)DfsFCBFreeCCB);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppFcb = pFcb;

    InterlockedIncrement(&gDfsObjectCounter.FcbCount);

    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    if (pFcb)
    {
        DfsFreeFCB(pFcb);
    }

    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsAddReferralFCB(
    PDFS_FCB pFcb,
    PSTR pszReferral
    )
{
    return STATUS_SUCCESS;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
DfsRemoveFCB(
    PDFS_FCB pFcb
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    /* We must have the mutex locked exclusively coming into this */

    ntStatus = LwRtlRBTreeRemove(
                  gDfsFcbTable.pFcbTree,
                  (PVOID)pFcb->pszPathname);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

PDFS_FCB
DfsReferenceFCB(
    IN PDFS_FCB pFcb
    )
{
    InterlockedIncrement(&pFcb->RefCount);

    return pFcb;
}


/***********************************************************************
 **********************************************************************/

VOID
DfsReleaseFCB(
    PDFS_FCB *ppFcb
    )
{
    BOOLEAN bTableLocked = FALSE;
    PDFS_FCB pFcb = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bTableLocked, &gDfsFcbTable.rwLock);

    if (InterlockedDecrement(&pFcb->RefCount) == 0)
    {
        DfsRemoveFCB(pFcb);
        DfsFreeFCB(pFcb);
    }

    LWIO_UNLOCK_RWMUTEX(bTableLocked, &gDfsFcbTable.rwLock);

    *ppFcb = (PDFS_FCB)NULL;

    return;
}


/***********************************************************************
 **********************************************************************/

static NTSTATUS
_DfsFindFCB(
    PDFS_FCB *ppFcb,
    PCSTR pszPathname
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDFS_FCB pFcb = NULL;

    ntStatus = LwRtlRBTreeFind(
                  gDfsFcbTable.pFcbTree,
                  (PVOID)pszPathname,
                  (PVOID*)&pFcb);
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_NO_SUCH_DEVICE;
    }
    BAIL_ON_NT_STATUS(ntStatus);

    *ppFcb = DfsReferenceFCB(pFcb);
    ntStatus = STATUS_SUCCESS;

cleanup:
    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsFindFCB(
    PDFS_FCB *ppFcb,
    PSTR pszPathname
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bLocked, &gDfsFcbTable.rwLock);

    ntStatus = _DfsFindFCB(ppFcb, pszPathname);

    LWIO_UNLOCK_RWMUTEX(bLocked, &gDfsFcbTable.rwLock);

    return ntStatus;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
DfsAddFCB(
    PDFS_FCB pFcb
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;

    ntStatus = LwRtlRBTreeAdd(
                  gDfsFcbTable.pFcbTree,
                  (PVOID)pFcb->pszPathname,
                  (PVOID)pFcb);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    return ntStatus;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsCreateFCB(
    OUT PDFS_FCB *ppFcb,
    IN  PSTR pszPathname
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    PDFS_FCB pFcb = NULL;
    BOOLEAN bFcbTableLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbTableLocked, &gDfsFcbTable.rwLock);

    /* Protect against adding a duplicate */

    ntStatus = _DfsFindFCB(&pFcb, pszPathname);
    if (ntStatus != STATUS_SUCCESS)
    {
        ntStatus = DfsAllocateFCB(&pFcb);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = RtlCStringDuplicate(&pFcb->pszPathname, pszPathname);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = DfsAddFCB(pFcb);
        BAIL_ON_NT_STATUS(ntStatus);

        *ppFcb = DfsReferenceFCB(pFcb);
    }

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbTableLocked, &gDfsFcbTable.rwLock);

    if (pFcb)
    {
        DfsReleaseFCB(&pFcb);
    }

    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsAddCCBToFCB(
    PDFS_FCB pFcb,
    PDFS_CCB pCcb
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN bFcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwCcbLock);

    ntStatus = DfsListAddTail(pFcb->pCcbList, &pCcb->FcbList);
    BAIL_ON_NT_STATUS(ntStatus);

    pCcb->pFcb = DfsReferenceFCB(pFcb);

    ntStatus = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwCcbLock);

    return ntStatus;

error:
    goto cleanup;
}


/***********************************************************************
 **********************************************************************/

NTSTATUS
DfsRemoveCCBFromFCB(
    PDFS_FCB pFcb,
    PDFS_CCB pCcb
    )
{
    NTSTATUS ntStatus = STATUS_UNSUCCESSFUL;
    BOOLEAN bFcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwCcbLock);

    ntStatus = DfsListRemoveItem(pFcb->pCcbList, &pCcb->FcbList);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwCcbLock);

    return ntStatus;

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
