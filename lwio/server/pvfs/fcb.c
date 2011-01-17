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
        if (pFcb->pParentFcb)
        {
            PvfsReleaseFCB(&pFcb->pParentFcb);
        }

        RtlCStringFree(&pFcb->pszFilename);

        pthread_rwlock_destroy(&pFcb->rwScbLock);

        PvfsListDestroy(&pFcb->pScbList);
        PvfsDestroyCB(&pFcb->BaseControlBlock);

        PVFS_FREE(&pFcb);

        InterlockedDecrement(&gPvfsFcbCount);
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

    InterlockedIncrement(&gPvfsFcbCount);

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

/*******************************************************
 ******************************************************/


static
NTSTATUS
PvfsFlushLastWriteTimeFcb(
    PPVFS_FCB pFcb,
    LONG64 LastWriteTime
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
                           LastWriteTime,
                           LastAccessTime);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;

}

/***********************************************************************
 Make sure to alkways enter this unfunction with thee pScb->ControlMutex
 locked
 **********************************************************************/

static
NTSTATUS
PvfsExecuteDeleteOnCloseFcb(
    PPVFS_FCB pFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    /* Always reset the delete-on-close state to be safe */

    pFcb->bDeleteOnClose = FALSE;

    /* Verify we are deleting the file we think we are */

    ntError = PvfsValidatePathFCB(pFcb, &pFcb->FileId);
    if (ntError == STATUS_SUCCESS)
    {
        ntError = PvfsSysRemove(pFcb->pszFilename);

        /* Reset dev/inode state */

        pFcb->FileId.Device = 0;
        pFcb->FileId.Inode  = 0;

    }
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    switch (ntError)
    {
    case STATUS_OBJECT_NAME_NOT_FOUND:
        break;

    default:
        LWIO_LOG_ERROR(
            "%s: Failed to execute delete-on-close on %s (%d,%d) (%s)\n",
            PVFS_LOG_HEADER,
            pFcb->pszFilename, pFcb->FileId.Device, pFcb->FileId.Inode,
            LwNtStatusToName(ntError));
        break;
    }

    goto cleanup;
}


////////////////////////////////////////////////////////////////////////

VOID
PvfsReleaseFCB(
    PPVFS_FCB *ppFcb
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    BOOLEAN bBucketLocked = FALSE;
    BOOLEAN bFcbLocked = FALSE;
    BOOLEAN bFcbControlLocked = FALSE;
    PVFS_STAT Stat = {0};
    PPVFS_FCB pFcb = NULL;
    PPVFS_CB_TABLE_ENTRY pBucket = NULL;

    /* Do housekeeping such as setting the last write time or honoring
       DeletOnClose when the CCB handle count reaches 0.  Not necessarily
       when the RefCount reaches 0.  We may have a non-handle open in the
       SCB table for a path component (see PvfsFindParentSCB()). */

    if (ppFcb == NULL || *ppFcb == NULL)
    {
        goto cleanup;
    }

    pFcb = *ppFcb;

    LWIO_LOCK_RWMUTEX_SHARED(bFcbLocked, &pFcb->rwScbLock);

    if (pFcb->OpenHandleCount == 0)
    {
        ntError = PvfsSysStat(pFcb->pszFilename, &Stat);
        if (ntError == STATUS_SUCCESS)
        {
            LONG64 LastWriteTime = PvfsClearLastWriteTimeFCB(pFcb);

            if (LastWriteTime != 0)
            {

                ntError = PvfsFlushLastWriteTimeFcb(pFcb, LastWriteTime);
#if 0
                if (ntError == STATUS_SUCCESS)
                {
                    PvfsNotifyScheduleFullReport(
                        pFcb,
                        FILE_NOTIFY_CHANGE_LAST_WRITE,
                        FILE_ACTION_MODIFIED,
                        pFcb->pszFilename);
                }
#endif
            }

            LWIO_LOCK_MUTEX(bFcbControlLocked, &pFcb->BaseControlBlock.Mutex);

            if (pFcb->bDeleteOnClose)
            {
                /* Clear the cache entry and remove the file but ignore any errors */

                ntError = PvfsExecuteDeleteOnCloseFcb(pFcb);

                /* The locking heirarchy requires that we drop the FCP control
                   block mutex before trying to pick up the ScbTable exclusive
                   lock */

                if (!pFcb->BaseControlBlock.Removed)
                {
                    PPVFS_CB_TABLE_ENTRY pBucket = pFcb->BaseControlBlock.pBucket;

                    LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->BaseControlBlock.Mutex);

                    /* Remove the SCB from the Bucket before setting pScb->pBucket
                       to NULL */

                    PvfsCbTableRemove(pBucket, pFcb->pszFilename);

                    LWIO_LOCK_MUTEX(bFcbControlLocked, &pFcb->BaseControlBlock.Mutex);
                    pFcb->BaseControlBlock.Removed = TRUE;
                    pFcb->BaseControlBlock.pBucket = NULL;
                    LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->BaseControlBlock.Mutex);
                }
                LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->BaseControlBlock.Mutex);

                PvfsPathCacheRemove(pFcb->pszFilename);

#if 0
                if (ntError == STATUS_SUCCESS)
                {
                    PvfsNotifyScheduleFullReport(
                        pFcb,
                        S_ISDIR(Stat.s_mode) ?
                            FILE_NOTIFY_CHANGE_DIR_NAME :
                            FILE_NOTIFY_CHANGE_FILE_NAME,
                        FILE_ACTION_REMOVED,
                        pFcb->pszFilename);
                }
#endif
            }

            LWIO_UNLOCK_MUTEX(bFcbControlLocked, &pFcb->BaseControlBlock.Mutex);
        }
    }

    LWIO_UNLOCK_RWMUTEX(bFcbLocked, &pFcb->rwScbLock);


        /* It is important to lock the ScbTable here so that there is no window
           between the refcount check and the remove. Otherwise another open request
           could search and locate the SCB in the tree and return free()'d memory.
           However, if the SCB has no bucket pointer, it has already been removed
           from the ScbTable so locking is unnecessary. */

    pBucket = pFcb->BaseControlBlock.pBucket;

    if (pBucket)
    {
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(bBucketLocked, &pBucket->rwLock);
    }

    if (InterlockedDecrement(&pFcb->BaseControlBlock.RefCount) == 0)
    {

        if (!pFcb->BaseControlBlock.Removed)
        {
            PvfsCbTableRemove_inlock(pBucket, pFcb->pszFilename);

            pFcb->BaseControlBlock.Removed = TRUE;
            pFcb->BaseControlBlock.pBucket = NULL;
        }

        if (pBucket)
        {
            LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);
        }

        PvfsFreeFCB(pFcb);
    }

    if (pBucket)
    {
        LWIO_UNLOCK_RWMUTEX(bBucketLocked, &pBucket->rwLock);
    }

    *ppFcb = NULL;

cleanup:
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
    IN PSTR pszFilename,
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
    BOOLEAN bFcbLocked = FALSE;

    ntError = PvfsFindParentFCB(&pParentFcb, pszFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCbTableGetBucket(&pBucket, &gFcbTable, pszFilename);
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

#if 0
        if (bCheckShareAccess)
        {
            ntError = PvfsEnforceShareMode(
                          pFcb,
                          SharedAccess,
                          DesiredAccess);
        }
#endif

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

    /* Add to the file handle table */

    LWIO_LOCK_MUTEX(bFcbLocked, &pFcb->BaseControlBlock.Mutex);
    ntError = PvfsCbTableAdd_inlock(
                  pBucket,
                  pFcb->pszFilename,
                  (PPVFS_CONTROL_BLOCK)pFcb);
    if (ntError == STATUS_SUCCESS)
    {
        pFcb->BaseControlBlock.pBucket = pBucket;
    }
    LWIO_UNLOCK_MUTEX(bFcbLocked, &pFcb->BaseControlBlock.Mutex);
    BAIL_ON_NT_STATUS(ntError);

    /* Return a reference to the FCB */

    *ppFcb = PvfsReferenceFCB(pFcb);
    ntError = STATUS_SUCCESS;

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

    ntError = PvfsCbTableGetBucket(&pBucket, &gFcbTable, pszDirname);
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
PvfsAddSCBToFCB(
    PPVFS_FCB pFcb,
    PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bFcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwScbLock);

    ntError = PvfsListAddTail(pFcb->pScbList, &pScb->FcbList);
    BAIL_ON_NT_STATUS(ntError);

    pScb->pOwnerFcb = PvfsReferenceFCB(pFcb);

    ntError = STATUS_SUCCESS;

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwScbLock);

    return ntError;

error:
    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsRemoveSCBFromFCB(
    PPVFS_FCB pFcb,
    PPVFS_SCB pScb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    BOOLEAN bFcbWriteLocked = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbWriteLocked, &pFcb->rwScbLock);

    ntError = PvfsListRemoveItem(pFcb->pScbList, &pScb->FcbList);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    LWIO_UNLOCK_RWMUTEX(bFcbWriteLocked, &pFcb->rwScbLock);

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
    PLW_LIST_LINKS pScbCursor = NULL;
    PPVFS_SCB pCurrentScb = NULL;
    BOOLEAN bFcbReadLocked = FALSE;

    PLW_LIST_LINKS pCcbCursor = NULL;
    PPVFS_CCB pCurrentCcb = NULL;
    BOOLEAN bScbReadLocked = FALSE;

    BOOLEAN bNonSelfOpen = FALSE;


    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pFcb->rwScbLock);

    while((pScbCursor = PvfsListTraverse(pFcb->pScbList, pScbCursor)) != NULL)
    {
        pCurrentScb = LW_STRUCT_FROM_FIELD(
                          pScbCursor,
                          PVFS_SCB,
                          FcbList);

        LWIO_LOCK_RWMUTEX_SHARED(bScbReadLocked, &pCurrentScb->rwCcbLock);

        while((pCcbCursor = PvfsListTraverse(pCurrentScb->pCcbList, pCcbCursor)) != NULL)
        {
            pCurrentCcb = LW_STRUCT_FROM_FIELD(
                              pCcbCursor,
                              PVFS_CCB,
                              ScbList);

            if (pCcb != pCurrentCcb)
            {
                bNonSelfOpen = TRUE;
                break;
            }
        }

        LWIO_UNLOCK_RWMUTEX(bScbReadLocked, &pCurrentScb->rwCcbLock);
    }

    LWIO_UNLOCK_RWMUTEX(bFcbReadLocked, &pFcb->rwScbLock);

    return bNonSelfOpen;
}


/*****************************************************************************
 ****************************************************************************/

NTSTATUS
PvfsRenameFCB(
    PPVFS_FCB pFcb,
    PPVFS_CCB pCcb,
    PSTR pszNewFilename
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_FCB pNewParentFcb = NULL;
    PPVFS_FCB pOldParentFcb = NULL;
    PPVFS_FCB pTargetFcb = NULL;
    PPVFS_CB_TABLE_ENTRY pTargetBucket = NULL;
    PPVFS_CB_TABLE_ENTRY pCurrentBucket = NULL;
    BOOLEAN bCurrentFcbControl = FALSE;
    BOOLEAN bTargetFcbControl = FALSE;
    BOOLEAN bTargetBucketLocked = FALSE;
    BOOLEAN bCurrentBucketLocked = FALSE;
    BOOLEAN bFcbRwLocked = FALSE;
    BOOLEAN bCcbLocked = FALSE;
    BOOLEAN bRenameLock = FALSE;

    /* If the target has an existing SCB, remove it from the Table and let
       the existing ref counters play out (e.g. pending change notifies. */

    ntError = PvfsFindParentFCB(&pNewParentFcb, pszNewFilename);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCbTableGetBucket(&pTargetBucket, &gFcbTable, pszNewFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Locks - gFcbTable(Excl) */
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bRenameLock, &gFcbTable.rwLock);

    /* Locks - gFcbTable(Excl),
               pTargetBucket(Excl) */

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bTargetBucketLocked, &pTargetBucket->rwLock);

    ntError = PvfsCbTableLookup_inlock(
                  (PPVFS_CONTROL_BLOCK*)&pTargetFcb,
                  pTargetBucket,
                  pszNewFilename);
    if (ntError == STATUS_SUCCESS)
    {
        /* Make sure we have a different SCB */

        if (pTargetFcb != pFcb)
        {
            LWIO_LOCK_MUTEX(bTargetFcbControl, &pTargetFcb->BaseControlBlock.Mutex);
            if (!pTargetFcb->BaseControlBlock.Removed)
            {
                pTargetFcb->BaseControlBlock.Removed = TRUE;
                pTargetFcb->BaseControlBlock.pBucket = NULL;

                LWIO_UNLOCK_MUTEX(bTargetFcbControl, &pTargetFcb->BaseControlBlock.Mutex);

                PvfsCbTableRemove_inlock(pTargetBucket, pTargetFcb->pszFilename);
            }
            LWIO_UNLOCK_MUTEX(bTargetFcbControl, &pTargetFcb->BaseControlBlock.Mutex);
        }
    }

    /* Locks - gFcbTable(Excl),
               pTargetBucket(Excl),
               pFcb->BaseControlBlock.RwLock(Excl) */

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bFcbRwLocked, &pFcb->BaseControlBlock.RwLock);

    ntError = PvfsSysRename(pCcb->pScb->pOwnerFcb->pszFilename, pszNewFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Clear the cache entry but ignore any errors */

    ntError = PvfsPathCacheRemove(pFcb->pszFilename);

    /* Remove the SCB from the table, update the lookup key, and then re-add.
       Otherwise you will get memory corruption as a freed pointer gets left
       in the Table because if cannot be located using the current (updated)
       filename. Another reason to use the dev/inode pair instead if we could
       solve the "Create New File" issue. */

    pCurrentBucket = pFcb->BaseControlBlock.pBucket;

    LWIO_LOCK_MUTEX(bCurrentFcbControl, &pFcb->BaseControlBlock.Mutex);
    pFcb->BaseControlBlock.Removed = TRUE;
    pFcb->BaseControlBlock.pBucket = NULL;
    LWIO_UNLOCK_MUTEX(bCurrentFcbControl, &pFcb->BaseControlBlock.Mutex);

    /* Locks - gFcbTable(Excl)
               pTargetBucket(Excl)
               pFcb->BaseControlBlock.RwLock(Excl),
               pCurrentBucket(Excl) */

    if (pCurrentBucket != pTargetBucket)
    {
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(bCurrentBucketLocked, &pCurrentBucket->rwLock);
    }

    ntError = PvfsCbTableRemove_inlock(pCurrentBucket, pFcb->pszFilename);
    LWIO_UNLOCK_RWMUTEX(bCurrentBucketLocked, &pCurrentBucket->rwLock);
    BAIL_ON_NT_STATUS(ntError);

    PVFS_FREE(&pFcb->pszFilename);

    ntError = LwRtlCStringDuplicate(&pFcb->pszFilename, pszNewFilename);
    BAIL_ON_NT_STATUS(ntError);

    /* Have to update the parent links as well */

    if (pNewParentFcb != pFcb->pParentFcb)
    {
        pOldParentFcb = pFcb->pParentFcb;
        pFcb->pParentFcb = pNewParentFcb;
        pNewParentFcb = NULL;
    }

    /* Locks - gFcbTable(Excl),
               pTargetBucket(Excl),
               pFcb->BaseControlBlock.RwLock(Excl),
               pFcb->BaseControlBlock.Mutex */

    LWIO_LOCK_MUTEX(bCurrentFcbControl, &pFcb->BaseControlBlock.Mutex);
    ntError = PvfsCbTableAdd_inlock(
                  pTargetBucket,
                  pFcb->pszFilename,
                  (PPVFS_CONTROL_BLOCK)pFcb);
    if (ntError == STATUS_SUCCESS)
    {
        pFcb->BaseControlBlock.Removed = FALSE;
        pFcb->BaseControlBlock.pBucket = pTargetBucket;
    }
    LWIO_UNLOCK_MUTEX(bCurrentFcbControl, &pFcb->BaseControlBlock.Mutex);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_UNLOCK_RWMUTEX(bFcbRwLocked, &pFcb->BaseControlBlock.RwLock);
    LWIO_UNLOCK_RWMUTEX(bTargetBucketLocked, &pTargetBucket->rwLock);


    LWIO_LOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

        PVFS_FREE(&pCcb->pszFilename);
        ntError = LwRtlCStringDuplicate(&pCcb->pszFilename, pszNewFilename);
        BAIL_ON_NT_STATUS(ntError);

    LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

cleanup:
    LWIO_UNLOCK_RWMUTEX(bTargetBucketLocked, &pTargetBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(bCurrentBucketLocked, &pCurrentBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(bRenameLock, &gFcbTable.rwLock);
    LWIO_UNLOCK_RWMUTEX(bFcbRwLocked, &pFcb->BaseControlBlock.RwLock);
    LWIO_UNLOCK_MUTEX(bCurrentFcbControl, &pFcb->BaseControlBlock.Mutex);
    LWIO_UNLOCK_MUTEX(bCcbLocked, &pCcb->ControlBlock);

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

    return ntError;

error:
    goto cleanup;
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


