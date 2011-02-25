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
 *        ccb.c
 *
 * Abstract:
 *
 *        Likewise Posix File System Driver (PVFS)
 *
 *        Context Control Block routineus
 *
 * Authors: Gerald Carter <gcarter@likewise.com>
 *          Krishna Ganugapati <krishnag@likewise.com>
 */

#include "pvfs.h"


/***********************************************************
 **********************************************************/

NTSTATUS
PvfsAllocateCCB(
    PPVFS_CCB *ppCCB
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PPVFS_CCB pCCB = NULL;

    *ppCCB = NULL;

    ntError = PvfsAllocateMemory(
                  (PVOID*)&pCCB,
                  sizeof(PVFS_CCB),
                  FALSE);
    BAIL_ON_NT_STATUS(ntError);

    InterlockedIncrement(&gPvfsCcbCount);

    /* Initialize mutexes and refcounts */

    pthread_mutex_init(&pCCB->ControlBlock, NULL);

    PVFS_INIT_LINKS(&pCCB->ScbList);

    pCCB->bPendingDeleteHandle = FALSE;
    pCCB->bCloseInProgress = FALSE;
    pCCB->OplockState = PVFS_OPLOCK_STATE_NONE;

    pCCB->fd = -1;
    PVFS_CLEAR_FILEID(pCCB->FileId);

    pCCB->pScb = NULL;
    pCCB->pwszShareName = NULL;
    pCCB->pDirContext = NULL;
    pCCB->pUserToken = NULL;
    pCCB->EcpFlags = 0;
    pCCB->ChangeEvent = 0;
    pCCB->WriteCount = 0;
    pCCB->bQuotaFile = FALSE;

    LwRtlZeroMemory(&pCCB->LockTable, sizeof(pCCB->LockTable));

    ntError = PvfsListInit(
                  &pCCB->pZctContextList,
                  0,  /* no max size */
                  (PPVFS_LIST_FREE_DATA_FN)PvfsFreeZctContext);
    BAIL_ON_NT_STATUS(ntError);

    /* Add initial ref count */

    pCCB->RefCount = 1;

    *ppCCB = pCCB;

    ntError = STATUS_SUCCESS;

cleanup:
    return ntError;

error:
    if (pCCB)
    {
        PvfsFreeCCB(pCCB);
    }


    goto cleanup;
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsFreeCCB(
    PPVFS_CCB pCCB
    )
{
    if (pCCB->pScb)
    {
        PvfsRemoveCCBFromSCB(pCCB->pScb, pCCB);
        PvfsReleaseSCB(&pCCB->pScb);
    }

    if (pCCB->pDirContext)
    {
        PvfsFreeDirectoryContext(pCCB->pDirContext);
    }

    if (pCCB->pUserToken)
    {
        RtlReleaseAccessToken(&pCCB->pUserToken);
        pCCB->pUserToken = NULL;
    }

    PvfsListDestroy(&pCCB->pZctContextList);

    LwRtlWC16StringFree(&pCCB->pwszShareName);

    PVFS_FREE(&pCCB->LockTable.ExclusiveLocks.pLocks);
    PVFS_FREE(&pCCB->LockTable.SharedLocks.pLocks);

    pthread_mutex_destroy(&pCCB->ControlBlock);

    PVFS_FREE(&pCCB);

    InterlockedDecrement(&gPvfsCcbCount);

    return STATUS_SUCCESS;
}

/*******************************************************
 ******************************************************/

VOID
PvfsReleaseCCB(
    PPVFS_CCB pCCB
    )
{
    if (InterlockedDecrement(&pCCB->RefCount) == 0)
    {
        PvfsFreeCCB(pCCB);
    }

    return;
}

/*******************************************************
 ******************************************************/

PPVFS_CCB
PvfsReferenceCCB(
    PPVFS_CCB pCCB
    )
{
    InterlockedIncrement(&pCCB->RefCount);

    return pCCB;
}


/*******************************************************
 ******************************************************/

static
NTSTATUS
PvfsAcquireCCBInternal(
    IO_FILE_HANDLE FileHandle,
    PPVFS_CCB * ppCCB,
    BOOLEAN bIncRef
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_CCB pCCB = (PPVFS_CCB)IoFileGetContext(FileHandle);

    PVFS_BAIL_ON_INVALID_CCB(pCCB, ntError);

    if (bIncRef) {
        InterlockedIncrement(&pCCB->RefCount);
    }

    *ppCCB = pCCB;

cleanup:
    return ntError;

error:
    *ppCCB = NULL;

    goto cleanup;

}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsAcquireCCB(
    IO_FILE_HANDLE FileHandle,
    PPVFS_CCB * ppCCB
    )
{
    return PvfsAcquireCCBInternal(FileHandle, ppCCB, TRUE);
}

/*******************************************************
 ******************************************************/

NTSTATUS
PvfsAcquireCCBClose(
    IO_FILE_HANDLE FileHandle,
    PPVFS_CCB * ppCCB
    )
{
    return PvfsAcquireCCBInternal(FileHandle, ppCCB, FALSE);
}


/*******************************************************
 ******************************************************/

NTSTATUS
PvfsStoreCCB(
    IO_FILE_HANDLE FileHandle,
    PPVFS_CCB pCCB
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;

    ntError = IoFileSetContext(FileHandle, (PVOID)pCCB);
    BAIL_ON_NT_STATUS(ntError);

cleanup:
    return ntError;

error:
    goto cleanup;
}


/********************************************************
 *******************************************************/

NTSTATUS
PvfsSaveFileDeviceInfo(
    PPVFS_CCB pCcb
    )
{
    NTSTATUS ntError = STATUS_UNSUCCESSFUL;
    PVFS_STAT Stat = {0};
    PPVFS_SCB pScb = pCcb->pScb;
    PPVFS_FCB pFcb = pCcb->pScb->pOwnerFcb;
    BOOLEAN scbLocked = FALSE;
    BOOLEAN fcbLocked = FALSE;

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->FileId.Device = Stat.s_dev;
    pCcb->FileId.Inode  = Stat.s_ino;
    pCcb->FileSize = Stat.s_size;

    LWIO_LOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);
    if ((pScb->FileId.Device == 0) || (pScb->FileId.Inode == 0))
    {
        // Have to set the FileID on the Stream Block
        pScb->FileId = pCcb->FileId;
        if (PvfsIsDefaultStream(pScb))
        {
            // Push the FIleId through to the File Block if unset
            LWIO_LOCK_MUTEX(fcbLocked, &pFcb->BaseControlBlock.Mutex);
            if ((pFcb->FileId.Device == 0) || (pFcb->FileId.Inode == 0))
            {
                pFcb->FileId = pScb->FileId;
            }
            LWIO_UNLOCK_MUTEX(fcbLocked, &pFcb->BaseControlBlock.Mutex);
        }
    }
    LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

cleanup:
    return ntError;

error:
    goto cleanup;
}


////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsRenameFile(
    PPVFS_CCB pCcb,
    PPVFS_FILE_NAME pNewFileName
    );

static
NTSTATUS
PvfsRenameStream(
    PPVFS_CCB pCcb,
    PPVFS_FILE_NAME pNewStreamName
    );

NTSTATUS
PvfsRenameCCB(
    IN PPVFS_CCB pCcb,
    IN PPVFS_FILE_NAME pDestFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_FILE_NAME srcFileName = { 0 };

    ntError = PvfsValidatePathSCB(pCcb->pScb, &pCcb->FileId);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsBuildFileNameFromScb(&srcFileName, pCcb->pScb);
    BAIL_ON_NT_STATUS(ntError);

    if (!PvfsIsDefaultStreamName(&srcFileName) &&
        !PvfsIsDefaultStreamName(pDestFileName))
    {
        // Two named streams
        if (LwRtlCStringIsEqual(
                PvfsGetCStringBaseStreamName(&srcFileName),
                PvfsGetCStringBaseStreamName(pDestFileName),
                FALSE))
        {
            // Both src and dst stream names are the same
            // renaming the underlying file object

            ntError = PvfsRenameFile(pCcb, pDestFileName);
        }
        else if (LwRtlCStringIsEqual(
                     PvfsGetCStringBaseFileName(&srcFileName),
                     PvfsGetCStringBaseFileName(pDestFileName),
                     FALSE))
        {
            // Renaming the named stream, file name stays the same

            ntError = PvfsRenameStream(pCcb, pDestFileName);
        }
        else
        {
            // Don't allow renaming both the file name and stream name at the
            // same time (yet)

            ntError = STATUS_OBJECT_NAME_INVALID;
        }
    }
    else if (PvfsIsDefaultStreamName(&srcFileName) &&
             PvfsIsDefaultStreamName(pDestFileName))
    {
        // Two default streams rename object itself
        ntError = PvfsRenameFile(pCcb, pDestFileName);
    }
    else if (!PvfsIsDefaultStreamName(&srcFileName) &&
             PvfsIsDefaultStreamName(pDestFileName))
    {
        // rename name stream -> default stream
        // A stream on a directory cannot be renamed to the default data stream
        if (!LwRtlCStringIsEqual(
                         PvfsGetCStringBaseFileName(&srcFileName),
                         PvfsGetCStringBaseFileName(pDestFileName),
                         FALSE) || PVFS_IS_DIR(pCcb))
        {
            ntError = STATUS_OBJECT_NAME_INVALID;
        }
        else
        {
            ntError = PvfsRenameStream(pCcb, pDestFileName);
        }
    }
    else
    {
        // disallow rename object->stream as what smbtorture expects
        // TODO:
        // we may want to allow "Renaming" the default data stream
        // it is not a true rename, and it leaves behind a zero-length default data streams
        ntError = STATUS_OBJECT_NAME_INVALID;
    }
    BAIL_ON_NT_STATUS(ntError);

error:
    PvfsDestroyFileName(&srcFileName);

    return ntError;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsRenameFile(
    PPVFS_CCB pCcb,
    PPVFS_FILE_NAME pNewFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_CB_TABLE_ENTRY pTargetBucket = NULL;
    PPVFS_CB_TABLE_ENTRY pCurrentBucket = NULL;
    BOOLEAN renameLock = FALSE;
    BOOLEAN targetBucketLock = FALSE;
    BOOLEAN currentBucketLock = FALSE;
    PPVFS_FCB pOwnerFcb = NULL;
    PLW_LIST_LINKS pScbCursor = NULL;
    PPVFS_SCB pCurrentScb = NULL;
    BOOLEAN scbMutexLock = FALSE;
    BOOLEAN bFcbReadLocked = FALSE;
    BOOLEAN bMatchScb = FALSE;
    PPVFS_FILE_NAME origTargetFileName = NULL;
    PPVFS_FILE_NAME scbCursorFileName = NULL;
    PSTR origFullStreamName = NULL;
    PSTR newFullStreamName = NULL;

    pOwnerFcb = PvfsReferenceFCB(pCcb->pScb->pOwnerFcb);

    ntError = PvfsAllocateFileNameFromScb(&origTargetFileName, pCcb->pScb);
    BAIL_ON_NT_STATUS(ntError);

    // Locks - gScbTable(Excl)

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(renameLock, &gScbTable.rwLock);

    ntError = PvfsRenameFCB(pOwnerFcb, pCcb, pNewFileName);
    BAIL_ON_NT_STATUS(ntError);

    // Locks - gScbTable(Excl)
    //       - pOwnerFcb->ScbLock(Shared)

    LWIO_LOCK_RWMUTEX_SHARED(bFcbReadLocked, &pOwnerFcb->rwScbLock);

    // The CB Table key is based on the complete file name (stream and base file)
    // Remove and Re-add the stream objects under the new key

    while((pScbCursor = PvfsListTraverse(pOwnerFcb->pScbList, pScbCursor)) != NULL)
    {
        pCurrentScb = LW_STRUCT_FROM_FIELD(
                          pScbCursor,
                          PVFS_SCB,
                          FcbList);

        if (!bMatchScb && pCurrentScb == pCcb->pScb)
        {
            bMatchScb = TRUE;
        }

        BAIL_ON_INVALID_PTR(pCurrentScb, ntError);

        ntError = PvfsAllocateFileNameFromScb(&scbCursorFileName, pCurrentScb);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsAllocateCStringFromFileName(
                      &newFullStreamName,
                      scbCursorFileName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsRenameBaseFileName(
                      scbCursorFileName,
                      PvfsGetCStringBaseFileName(origTargetFileName));
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsAllocateCStringFromFileName(
                      &origFullStreamName,
                      scbCursorFileName);
        BAIL_ON_NT_STATUS(ntError);

        pCurrentBucket = pCurrentScb->BaseControlBlock.pBucket;

        ntError = PvfsCbTableGetBucket(&pTargetBucket, &gScbTable, newFullStreamName);
        BAIL_ON_NT_STATUS(ntError);

        LWIO_LOCK_RWMUTEX_EXCLUSIVE(currentBucketLock, &pCurrentBucket->rwLock);
        if (pCurrentBucket != pTargetBucket)
        {
            // Will be moved to a new bucket in the table
            LWIO_LOCK_RWMUTEX_EXCLUSIVE(targetBucketLock, &pTargetBucket->rwLock);
        }

        LWIO_LOCK_MUTEX(scbMutexLock, &pCurrentScb->BaseControlBlock.Mutex);

        ntError = PvfsCbTableRemove_inlock(pCurrentBucket, origFullStreamName);
        BAIL_ON_NT_STATUS(ntError);

        ntError = PvfsCbTableAdd_inlock(
                      pTargetBucket,
                      newFullStreamName,
                     (PPVFS_CONTROL_BLOCK)pCcb->pScb);
        BAIL_ON_NT_STATUS(ntError);

        pCurrentScb->BaseControlBlock.pBucket = pTargetBucket;

        LWIO_UNLOCK_MUTEX(scbMutexLock, &pCurrentScb->BaseControlBlock.Mutex);

        LWIO_UNLOCK_RWMUTEX(targetBucketLock, &pTargetBucket->rwLock);
        LWIO_UNLOCK_RWMUTEX(currentBucketLock, &pCurrentBucket->rwLock);
    }

    // Sanity check
    // Make sure pCcb->pScb is accessed by going through pOwnerFcb's pScbList
    LWIO_ASSERT(bMatchScb);

error:
    LWIO_UNLOCK_MUTEX(scbMutexLock, &pCurrentScb->BaseControlBlock.Mutex);
    LWIO_UNLOCK_RWMUTEX(targetBucketLock, &pTargetBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(currentBucketLock, &pCurrentBucket->rwLock);

    LWIO_UNLOCK_RWMUTEX(bFcbReadLocked, &pOwnerFcb->rwScbLock);
    LWIO_UNLOCK_RWMUTEX(renameLock, &gScbTable.rwLock);

    if (pOwnerFcb)
    {
        PvfsReleaseFCB(&pOwnerFcb);
    }

    if (origTargetFileName)
    {
        PvfsFreeFileName(origTargetFileName);
    }
    if (origFullStreamName)
    {
        LwRtlCStringFree(&origFullStreamName);
    }
    if (newFullStreamName)
    {
        LwRtlCStringFree(&newFullStreamName);
    }

    return ntError;
}


////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsRenameStream(
    PPVFS_CCB pCcb,
    PPVFS_FILE_NAME pNewStreamName
    )
{
    return PvfsRenameSCB(pCcb->pScb, pCcb, pNewStreamName);
}
