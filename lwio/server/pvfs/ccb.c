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

    InterlockedIncrement(&gPvfsDriverState.Counters.Ccb);

    /* Initialize mutexes and refcounts */

    pthread_mutex_init(&pCCB->ControlBlock, NULL);

    PVFS_INIT_LINKS(&pCCB->ScbList);

    pCCB->OplockState = PVFS_OPLOCK_STATE_NONE;

    pCCB->fd = -1;
    PVFS_CLEAR_FILEID(pCCB->FileId);

    pCCB->pScb = NULL;
    pCCB->pwszShareName = NULL;
    pCCB->pDirContext = NULL;
    pCCB->pUserToken = NULL;
    pCCB->ChangeEvent = 0;
    pCCB->WriteCount = 0;
    pCCB->AccessGranted = 0;
    pCCB->Flags = PVFS_CCB_FLAG_NONE;

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
    LWIO_ASSERT(pCCB->RefCount == 0);

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

    InterlockedDecrement(&gPvfsDriverState.Counters.Ccb);

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

    // Have to set the FileID on the Stream Block
    pScb->FileId = pCcb->FileId;

    if (PvfsIsDefaultStream(pScb))
    {
        // Push the FIleId through to the File Block if unset
        LWIO_LOCK_MUTEX(fcbLocked, &pFcb->BaseControlBlock.Mutex);

        pFcb->FileId = pScb->FileId;

        LWIO_UNLOCK_MUTEX(fcbLocked, &pFcb->BaseControlBlock.Mutex);
    }

    LWIO_UNLOCK_MUTEX(scbLocked, &pScb->BaseControlBlock.Mutex);

error:

    return ntError;
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
    PPVFS_CB_TABLE scbTable = &gPvfsDriverState.ScbTable;
    PPVFS_CB_TABLE_ENTRY pTargetBucket = NULL;
    PPVFS_CB_TABLE_ENTRY pCurrentBucket = NULL;
    BOOLEAN renameLock = FALSE;
    BOOLEAN targetBucketLock = FALSE;
    BOOLEAN currentBucketLock = FALSE;
    PPVFS_FCB pFcb = NULL;
    PLW_LIST_LINKS scbCursorLink = NULL;
    PPVFS_SCB scbCursor = NULL;
    BOOLEAN scbMutexLock = FALSE;
    BOOLEAN fcbListLocked = FALSE;
    PPVFS_FILE_NAME origTargetFileName = NULL;
    PPVFS_FILE_NAME scbCursorFileName = NULL;
    PSTR origFullStreamName = NULL;
    PSTR newFullStreamName = NULL;
    PPVFS_SCB* scbList = NULL;
    LONG scbCount = 0;
    LONG scbIndex = 0;

    // The CCB holds out reference down the chain so no need to take a new one
    pFcb = pCcb->pScb->pOwnerFcb;

    ntError = PvfsAllocateFileNameFromScb(&origTargetFileName, pCcb->pScb);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(renameLock, &scbTable->rwLock);

    ntError = PvfsRenameFCB(pFcb, pCcb, pNewFileName);
    BAIL_ON_NT_STATUS(ntError);

    // Grab a reference to all SCBs in the list so we can drop the list lock
    LWIO_LOCK_RWMUTEX_SHARED(fcbListLocked, &pFcb->rwScbLock);

    scbCount = PvfsListLength(pFcb->pScbList);
    if (scbCount == 0)
    {
        ntError = STATUS_SUCCESS;
        goto cleanup;
    }

    ntError = PvfsAllocateMemory(
                  (PVOID*)&scbList,
                  sizeof(*scbList)*scbCount,
                  TRUE);
    BAIL_ON_NT_STATUS(ntError);

    for (scbIndex = 0, scbCursorLink = PvfsListTraverse(pFcb->pScbList, NULL);
         scbIndex<scbCount && scbCursorLink;
         scbIndex++, scbCursorLink = PvfsListTraverse(pFcb->pScbList, scbCursorLink))
    {
        scbCursor = LW_STRUCT_FROM_FIELD(
                          scbCursorLink,
                          PVFS_SCB,
                          FcbList);
        scbList[scbIndex] = PvfsReferenceSCB(scbCursor);
        LWIO_ASSERT(scbList[scbIndex] != NULL);
    }

    LWIO_UNLOCK_RWMUTEX(fcbListLocked, &pFcb->rwScbLock);


    // The CB Table key is based on the complete file name (stream and base file)
    // Remove and Re-add the stream objects under the new key
    for (scbIndex=0; scbIndex<scbCount; scbIndex++)
    {
        ntError = PvfsAllocateFileNameFromScb(&scbCursorFileName, scbCursor);
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

        pCurrentBucket = scbCursor->BaseControlBlock.pBucket;

        ntError = PvfsCbTableGetBucket(&pTargetBucket, scbTable, newFullStreamName);
        BAIL_ON_NT_STATUS(ntError);

        LWIO_LOCK_MUTEX(scbMutexLock, &scbCursor->BaseControlBlock.Mutex);
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(currentBucketLock, &pCurrentBucket->rwLock);
        if (pCurrentBucket != pTargetBucket)
        {
            // Will be moved to a new bucket in the table
            LWIO_LOCK_RWMUTEX_EXCLUSIVE(targetBucketLock, &pTargetBucket->rwLock);
        }

        ntError = PvfsCbTableRemove_inlock(
                      (PPVFS_CONTROL_BLOCK)scbCursor,
                      origFullStreamName);
        LWIO_ASSERT(STATUS_SUCCESS == ntError);

        ntError = PvfsCbTableAdd_inlock(
                      pTargetBucket,
                      newFullStreamName,
                     (PPVFS_CONTROL_BLOCK)scbCursor);
        if (STATUS_SUCCESS != ntError)
        {
            LWIO_LOG_ERROR(
                "Failed to rename stream \"%s\" (%s)\n",
                newFullStreamName,
                LwNtStatusToName(ntError));
        }

        LWIO_UNLOCK_RWMUTEX(targetBucketLock, &pTargetBucket->rwLock);
        LWIO_UNLOCK_RWMUTEX(currentBucketLock, &pCurrentBucket->rwLock);
        LWIO_UNLOCK_MUTEX(scbMutexLock, &scbCursor->BaseControlBlock.Mutex);

        if (origFullStreamName)
        {
            LwRtlCStringFree(&origFullStreamName);
        }
        if (newFullStreamName)
        {
            LwRtlCStringFree(&newFullStreamName);
        }
    }

cleanup:
error:
    LWIO_UNLOCK_MUTEX(scbMutexLock, &scbCursor->BaseControlBlock.Mutex);
    LWIO_UNLOCK_RWMUTEX(targetBucketLock, &pTargetBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(currentBucketLock, &pCurrentBucket->rwLock);

    LWIO_UNLOCK_RWMUTEX(fcbListLocked, &pFcb->rwScbLock);
    LWIO_UNLOCK_RWMUTEX(renameLock, &scbTable->rwLock);

    for(scbIndex=0; scbIndex<scbCount; scbIndex++)
    {
        PvfsReleaseSCB(&scbList[scbIndex]);
    }

    PVFS_FREE(&scbList);

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
