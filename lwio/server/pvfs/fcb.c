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
 *        fcb.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        File Stream Control Block routines
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 */

#include "pvfs.h"


/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsFreeFCB(
    PPVFS_FCB pFcb
    )
{
    if (pFcb)
    {
        LWIO_ASSERT(pFcb->BaseControlBlock.pBucket == NULL);
        LWIO_ASSERT(pFcb->BaseControlBlock.RefCount == 0);

        if (pFcb->pParentFcb)
        {
            PvfsReleaseFCB(&pFcb->pParentFcb);
        }

        RtlCStringFree(&pFcb->pszFilename);

        pthread_rwlock_destroy(&pFcb->rwScbLock);

        PvfsListDestroy(&pFcb->pScbList);
        PvfsListDestroy(&pFcb->pNotifyListIrp);
        PvfsListDestroy(&pFcb->pNotifyListBuffer);

        PvfsDestroyCB(&pFcb->BaseControlBlock);

        PVFS_FREE(&pFcb);

        InterlockedDecrement(&gPvfsDriverState.Counters.Fcb);
    }

    return;
}

/*****************************************************************************
 ****************************************************************************/

static
VOID
PvfsFCBFreeSCB(
    PVOID *ppData
    )
{
    /* This should never be called.  The SCB count has to be 0 when
       we call PvfsFreeFCB() and hence destroy the FCB->pScbList */

    return;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsAllocateFCB(
    PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;

    *ppFcb = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pFcb,
                  sizeof(*pFcb),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    PvfsInitializeCB(&pFcb->BaseControlBlock);

    /* Initialize mutexes and refcounts */

    pthread_rwlock_init(&pFcb->rwScbLock, NULL);

    /* List of Notify requests */

    ntError = PvfsListInit(
                  &pFcb->pNotifyListIrp,
                  PVFS_SCB_MAX_PENDING_NOTIFY,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreeNotifyRecord);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsListInit(
                  &pFcb->pNotifyListBuffer,
                  PVFS_SCB_MAX_PENDING_NOTIFY,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreeNotifyRecord);
    BAIL_ON_NT_STATUS(ntError);

    /* List of SCBs */

    ntError = PvfsListInit(
                  &pFcb->pScbList,
                  0,
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFCBFreeSCB);
    BAIL_ON_NT_STATUS(ntError);


    /* Miscellaneous */

    PVFS_CLEAR_FILEID(pFcb->FileId);

    pFcb->LastWriteTime = 0;
    pFcb->OpenHandleCount = 0;
    pFcb->bDeleteOnClose = FALSE;
    pFcb->pParentFcb = NULL;
    pFcb->pszFilename = NULL;

    *ppFcb = pFcb;

    InterlockedIncrement(&gPvfsDriverState.Counters.Fcb);

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    if (pFcb)
    {
        PvfsFreeFCB(pFcb);
    }

    goto cleanup;
}

/*******************************************************
 ******************************************************/

PPVFS_FCB
PvfsReferenceFCB(
    IN PPVFS_FCB pFcb
    )
{
    InterlockedIncrement(&pFcb->BaseControlBlock.RefCount);

    return pFcb;
}

////////////////////////////////////////////////////////////////////////

VOID
PvfsReleaseFCB(
    PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bucketLocked = FALSE;
    BOOLEAN fcbLocked = FALSE;
    PPVFS_FCB pFcb = NULL;
    PPVFS_CB_TABLE_ENTRY pBucket = NULL;
    LONG refCount = 0;

    LWIO_ASSERT((ppFcb != NULL) && (*ppFcb != NULL));

    pFcb = *ppFcb;

    // It is important to lock the ScbTable here so that there is no window
    // between the refcount check and the remove. Otherwise another open request
    // could search and locate the SCB in the tree and return free()'d memory.
    // However, if the SCB has no bucket pointer, it has already been removed
    // from the ScbTable so locking is unnecessary. */

    LWIO_LOCK_MUTEX(fcbLocked, &pFcb->BaseControlBlock.Mutex);

    pBucket = pFcb->BaseControlBlock.pBucket;

    if (pBucket)
    {
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(bucketLocked, &pBucket->rwLock);
    }

    refCount = InterlockedDecrement(&pFcb->BaseControlBlock.RefCount);
    LWIO_ASSERT(refCount >= 0);

    if (refCount == 0)
    {
        if (pBucket)
        {
            ntError = PvfsCbTableRemove_inlock(
                          (PPVFS_CONTROL_BLOCK)pFcb,
                          pFcb->pszFilename);
            LWIO_ASSERT(ntError == STATUS_SUCCESS);

            LWIO_UNLOCK_RWMUTEX(bucketLocked, &pBucket->rwLock);
        }

        LWIO_UNLOCK_MUTEX(fcbLocked, &pFcb->BaseControlBlock.Mutex);

        PvfsFreeFCB(pFcb);
    }

    LWIO_UNLOCK_RWMUTEX(bucketLocked, &pBucket->rwLock);
    LWIO_UNLOCK_MUTEX(fcbLocked, &pFcb->BaseControlBlock.Mutex);

    *ppFcb = NULL;

    return;
}


/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsFindParentFCB(
    PPVFS_FCB *ppParentFcb,
    PCSTR pszFilename
    );

NTSTATUS
PvfsCreateFCB(
    OUT PPVFS_FCB *ppFcb,
    IN PCSTR pszFilename,
    IN BOOLEAN bCheckShareAccess,
    IN FILE_SHARE_FLAGS SharedAccess,
    IN ACCESS_MASK DesiredAccess
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    BOOLEAN bBucketLocked = FALSE;
    PPVFS_CB_TABLE_ENTRY pBucket = NULL;
    PPVFS_FCB pParentFcb = NULL;

    ntError = PvfsFindParentFCB(&pParentFcb, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCbTableGetBucket(&pBucket, &gPvfsDriverState.FcbTable, (PVOID)pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Protect against adding a duplicate */

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBucketLocked, &pBucket->rwLock);

    ntError = PvfsCbTableLookup_inlock(
                  (PPVFS_CONTROL_BLOCK*)&pFcb,
                  pBucket,
                  pszFilename);
    if (ntError == STATUS_SUCCESS)
    {
        LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);

        /* If we have success, then we are good.  If we have a sharing
           violation, give the caller a chance to break the oplock and
           we'll try again when the create is resumed. */

        if (ntError == STATUS_SUCCESS ||
            ntError == STATUS_SHARING_VIOLATION)
        {
            *ppFcb = PvfsReferenceFCB(pFcb);
        }

        goto cleanup;
    }

    ntError = PvfsAllocateFCB(&pFcb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = RtlCStringDuplicate(&pFcb->pszFilename, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    pFcb->pParentFcb = pParentFcb ? PvfsReferenceFCB(pParentFcb) : NULL;

    // New FCB so BaseControlBlock.Mutext locking is not necessary

    ntError = PvfsCbTableAdd_inlock(
                  pBucket,
                  pFcb->pszFilename,
                  (PPVFS_CONTROL_BLOCK)pFcb);
    BAIL_ON_NT_STATUS(ntError);

    /* Return a reference to the FCB */

    *ppFcb = PvfsReferenceFCB(pFcb);

cleanup:
    LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);

    if (pParentFcb)
    {
        PvfsReleaseFCB(&pParentFcb);
    }

    if (pFcb)
    {
        PvfsReleaseFCB(&pFcb);
    }

    return ntError;

error:
    goto cleanup;
}

/***********************************************************************
 **********************************************************************/

static
NTSTATUS
PvfsFindParentFCB(
    PPVFS_FCB *ppParentFcb,
    PCSTR pszFilename
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_FCB pFcb = NULL;
    PSTR pszDirname = NULL;
    PPVFS_CB_TABLE_ENTRY pBucket = NULL;

    if (LwRtlCStringIsEqual(pszFilename, "/", TRUE))
    {
        ntError = STATUS_SUCCESS;
        *ppParentFcb = NULL;

        goto cleanup;
    }

    ntError = PvfsFileDirname(&pszDirname, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCbTableGetBucket(&pBucket, &gPvfsDriverState.FcbTable, pszDirname);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCbTableLookup((PPVFS_CONTROL_BLOCK*)&pFcb, pBucket, pszDirname);
    if (ntError == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        ntError = PvfsCreateFCB(
                      &pFcb,
                      pszDirname,
                      FALSE,
                      0,
                      0);
    }
    BAIL_ON_NT_STATUS(ntError);

    *ppParentFcb = PvfsReferenceFCB(pFcb);

cleanup:
    if (pFcb)
    {
        PvfsReleaseFCB(&pFcb);
    }

    if (pszDirname)
    {
        LwRtlCStringFree(&pszDirname);
    }

    return ntError;

error:

    goto cleanup;
}


/*******************************************************
 ******************************************************/

NTSTATUS
PvfsAddSCBToFCB_inlock(
    IN OUT PPVFS_FCB pFcb,
    IN PPVFS_SCB pScb
    )
{
    return PvfsListAddTail(pFcb->pScbList, &pScb->FcbList);
}

////////////////////////////////////////////////////////////////////////

NTSTATUS
PvfsRemoveSCBFromFCB_inlock(
    IN OUT PPVFS_FCB pFcb,
    IN PPVFS_SCB pScb
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    if ((pScb->FcbList.Next != NULL) && (pScb->FcbList.Prev != NULL))
    {
        status = PvfsListRemoveItem(pFcb->pScbList, &pScb->FcbList);
    }

    return status;
}

/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsRenameFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb,
    PPVFS_FILE_NAME pNewFilename
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_CB_TABLE fcbTable = &gPvfsDriverState.FcbTable;
    PPVFS_FCB pNewParentFcb = NULL;
    PPVFS_FCB pOldParentFcb = NULL;
    PPVFS_FCB pTargetFcb = NULL;
    PPVFS_CB_TABLE_ENTRY pTargetBucket = NULL;
    PPVFS_CB_TABLE_ENTRY pCurrentBucket = NULL;
    BOOLEAN currentFcbControl = FALSE;
    BOOLEAN targetFcbListLocked = FALSE;
    BOOLEAN targetBucketLocked = FALSE;
    BOOLEAN currentBucketLocked = FALSE;
    BOOLEAN fcbRwLocked = FALSE;
    BOOLEAN renameLock = FALSE;
    PPVFS_FILE_NAME currentFileName = NULL;

    /* If the target has an existing SCB, remove it from the Table and let
       the existing ref counters play out (e.g. pending change notifies. */

    BAIL_ON_INVALID_PTR(pNewFilename, ntError);

    ntError = PvfsFindParentFCB(&pNewParentFcb, pNewFilename->FileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsAllocateFileNameFromScb(&currentFileName, pCcb->pScb);
    BAIL_ON_NT_STATUS(ntError);

    // Obtain all locks for the rename
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(renameLock, &fcbTable->rwLock);
    LWIO_LOCK_MUTEX(currentFcbControl, &pFcb->BaseControlBlock.Mutex);

    pCurrentBucket = pFcb->BaseControlBlock.pBucket;

    ntError = PvfsCbTableGetBucket(
                  &pTargetBucket,
                  fcbTable,
                  pNewFilename->FileName);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(currentBucketLocked, &pCurrentBucket->rwLock);
    if (pCurrentBucket != pTargetBucket)
    {
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(targetBucketLocked, &pTargetBucket->rwLock);
    }

    // Do the rename work now
    ntError = PvfsCbTableLookup_inlock(
                  (PPVFS_CONTROL_BLOCK*)&pTargetFcb,
                  pTargetBucket,
                  pNewFilename->FileName);
    if (ntError == STATUS_SUCCESS)
    {
        if (pTargetFcb != pFcb)
        {
            // Remove an existing FCB for the target file/stream (if one exists)
            // But make sure it is a different FCB

            LWIO_LOCK_RWMUTEX_SHARED(targetFcbListLocked, &pTargetFcb->rwScbLock);

            if (pTargetFcb->OpenHandleCount > 0 )
            {
                // if TargetScb has open handles cannot rename
                // This except in the batch-oplock case
                ntError = STATUS_INVALID_PARAMETER;
                BAIL_ON_NT_STATUS(ntError);
            }

            LWIO_UNLOCK_RWMUTEX(targetFcbListLocked, &pTargetFcb->rwScbLock);

            // TODO - How to get the Control Mutex without violating the
            // lock heirarchy?  Does it matter at all ?
            ntError = PvfsCbTableRemove_inlock(
                          (PPVFS_CONTROL_BLOCK)pTargetFcb,
                          pTargetFcb->pszFilename);
            LWIO_ASSERT(STATUS_SUCCESS == ntError);
        }
    }

    ntError = PvfsSysRenameByFileName(currentFileName, pNewFilename);
    // Ignore the error here and continue

    ntError = PvfsPathCacheRemove(currentFileName);
    // Ignore the error here and continue

    /* Remove the SCB from the table, update the lookup key, and then re-add.
       Otherwise you will get memory corruption as a freed pointer gets left
       in the Table because if cannot be located using the current (updated)
       filename. Another reason to use the dev/inode pair instead if we could
       solve the "Create New File" issue. */

    // Remove FCB
    ntError = PvfsCbTableRemove_inlock(
                  (PPVFS_CONTROL_BLOCK)pFcb,
                  pFcb->pszFilename);
    LWIO_ASSERT(STATUS_SUCCESS == ntError);

    // Rename FCB & Update parent links
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(fcbRwLocked, &pFcb->BaseControlBlock.RwLock);
    LwRtlCStringFree(&pFcb->pszFilename);

    ntError = LwRtlCStringDuplicate(&pFcb->pszFilename, pNewFilename->FileName);
    BAIL_ON_NT_STATUS(ntError);

    if (pNewParentFcb != pFcb->pParentFcb)
    {
        pOldParentFcb = pFcb->pParentFcb;
        pFcb->pParentFcb = pNewParentFcb;
        pNewParentFcb = NULL;
    }
    LWIO_UNLOCK_RWMUTEX(fcbRwLocked, &pFcb->BaseControlBlock.RwLock);

    // Re-Add SCB
    ntError = PvfsCbTableAdd_inlock(
                  pTargetBucket,
                  pFcb->pszFilename,
                  (PPVFS_CONTROL_BLOCK)pFcb);
    BAIL_ON_NT_STATUS(ntError);

error:

    // Release all locks .. Whew!
    LWIO_UNLOCK_RWMUTEX(targetFcbListLocked, &pTargetFcb->rwScbLock);
    LWIO_UNLOCK_RWMUTEX(targetBucketLocked, &pTargetBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(currentBucketLocked, &pCurrentBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(fcbRwLocked, &pFcb->BaseControlBlock.RwLock);
    LWIO_UNLOCK_MUTEX(currentFcbControl, &pFcb->BaseControlBlock.Mutex);
    LWIO_UNLOCK_RWMUTEX(renameLock, &fcbTable->rwLock);

    if (pNewParentFcb)
    {
        PvfsReleaseFCB(&pNewParentFcb);
    }
    if (pOldParentFcb)
    {
        PvfsReleaseFCB(&pOldParentFcb);
    }
    if (pTargetFcb)
    {
        PvfsReleaseFCB(&pTargetFcb);
    }
    if (currentFileName)
    {
        PvfsFreeFileName(currentFileName);
    }

    return ntError;
}


/*****************************************************************************
 ****************************************************************************/

BOOLEAN
PvfsFcbIsPendingDelete(
    PPVFS_FCB pFcb
    )
{
    BOOLEAN bPendingDelete = FALSE;
    BOOLEAN bIsLocked = FALSE;

    LWIO_LOCK_MUTEX(bIsLocked, &pFcb->BaseControlBlock.Mutex);
    bPendingDelete = pFcb->bDeleteOnClose;
    LWIO_UNLOCK_MUTEX(bIsLocked, &pFcb->BaseControlBlock.Mutex);

    return bPendingDelete;
}

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsFcbSetPendingDelete(
    PPVFS_FCB pFcb,
    BOOLEAN bPendingDelete
    )
{
    BOOLEAN bIsLocked = FALSE;

    LWIO_LOCK_MUTEX(bIsLocked, &pFcb->BaseControlBlock.Mutex);
    pFcb->bDeleteOnClose = bPendingDelete;
    LWIO_UNLOCK_MUTEX(bIsLocked, &pFcb->BaseControlBlock.Mutex);
}

/*****************************************************************************
 ****************************************************************************/

PPVFS_FCB
PvfsGetParentFCB(
    PPVFS_FCB pFcb
    )
{
    PPVFS_FCB pParent = NULL;
    BOOLEAN bLocked = FALSE;

    if (pFcb)
    {
        LWIO_LOCK_RWMUTEX_SHARED(bLocked, &pFcb->BaseControlBlock.RwLock);
        if (pFcb->pParentFcb)
        {
            pParent = PvfsReferenceFCB(pFcb->pParentFcb);
        }
        LWIO_UNLOCK_RWMUTEX(bLocked, &pFcb->BaseControlBlock.RwLock);
    }

    return pParent;
}

/*****************************************************************************
 ****************************************************************************/

LONG64
PvfsClearLastWriteTimeFCB(
    PPVFS_FCB pFcb
    )
{
    BOOLEAN bLocked = FALSE;
    LONG64 LastWriteTime = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pFcb->BaseControlBlock.RwLock);
    LastWriteTime = pFcb->LastWriteTime;
    pFcb->LastWriteTime = 0;
    LWIO_UNLOCK_RWMUTEX(bLocked, &pFcb->BaseControlBlock.RwLock);

    return LastWriteTime;
}

/*****************************************************************************
 ****************************************************************************/

VOID
PvfsSetLastWriteTimeFCB(
    PPVFS_FCB pFcb,
    LONG64 LastWriteTime
    )
{
    BOOLEAN bLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bLocked, &pFcb->BaseControlBlock.RwLock);
    pFcb->LastWriteTime = LastWriteTime;
    LWIO_UNLOCK_RWMUTEX(bLocked, &pFcb->BaseControlBlock.RwLock);
}

