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
    pCCB->pszFilename = NULL;
    pCCB->pwszShareName = NULL;
    pCCB->pDirContext = NULL;
    pCCB->pUserToken = NULL;
    pCCB->EcpFlags = 0;
    pCCB->ChangeEvent = 0;
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

    LwRtlCStringFree(&pCCB->pszFilename);
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
    BOOLEAN bLocked = FALSE;

    ntError = PvfsSysFstat(pCcb->fd, &Stat);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->FileId.Device = Stat.s_dev;
    pCcb->FileId.Inode  = Stat.s_ino;
    pCcb->FileSize = Stat.s_size;

    LWIO_LOCK_MUTEX(bLocked, &pScb->BaseControlBlock.Mutex);
    if ((pScb->FileId.Device == 0) || (pScb->FileId.Inode == 0))
    {
        pScb->FileId = pCcb->FileId;
    }
    LWIO_UNLOCK_MUTEX(bLocked, &pScb->BaseControlBlock.Mutex);

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
    PSTR NewFileName
    );

static
NTSTATUS
PvfsRenameStream(
    PPVFS_CCB pCcb,
    PSTR NewFileName
    );

NTSTATUS
PvfsRenameCCB(
    PPVFS_CCB pCcb,
    PSTR NewFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PVFS_FILE_NAME srcFileName = { 0 };
    PVFS_FILE_NAME dstFileName = { 0 };

    ntError = PvfsValidatePathSCB(pCcb->pScb, &pCcb->FileId);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsBuildFileNameFromScb(&srcFileName, pCcb->pScb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsBuildFileNameFromCString(&dstFileName, NewFileName);
    BAIL_ON_NT_STATUS(ntError);

    if (LwRtlCStringIsEqual(
            srcFileName.StreamName,
            dstFileName.StreamName,
            FALSE))
    {
        // Both src and dst stream names ar ethe same some only
        // renaming the underlying file object

        ntError = PvfsRenameFile(pCcb, NewFileName);
    }
    else if (LwRtlCStringIsEqual(
                 srcFileName.FileName,
                 dstFileName.FileName,
                 FALSE))
    {
        // Renaming the named stream, file name stays the same

        ntError = PvfsRenameStream(pCcb, NewFileName);
    }
    else
    {
        // Don't allow renaming both the file name and stream name at the
        // same time (yet)

        ntError = STATUS_INVALID_PARAMETER;
    }
    BAIL_ON_NT_STATUS(ntError);

error:
    PvfsDestroyFileName(&srcFileName);
    PvfsDestroyFileName(&dstFileName);

    return ntError;
}

////////////////////////////////////////////////////////////////////////

static
NTSTATUS
PvfsRenameFile(
    PPVFS_CCB pCcb,
    PSTR NewFileName
    )
{
    NTSTATUS ntError = STATUS_SUCCESS;
    PPVFS_CB_TABLE_ENTRY pTargetBucket = NULL;
    PPVFS_CB_TABLE_ENTRY pCurrentBucket = NULL;
    PSTR currentFullStreamName = NULL;
    PSTR newFullStreamName = NULL;
    BOOLEAN renameLock = FALSE;
    BOOLEAN targetBucketLock = FALSE;
    BOOLEAN currentBucketLock = FALSE;
    BOOLEAN scbLock = FALSE;

    ntError = PvfsGetFullStreamname(&currentFullStreamName, pCcb->pScb);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCbTableGetBucket(&pTargetBucket, &gScbTable, NewFileName);
    BAIL_ON_NT_STATUS(ntError);

    pCurrentBucket = pCcb->pScb->BaseControlBlock.pBucket;

    // Locks - gScbTable(Excl)
    //       - pCurrentBucket(Excl)
    //       - pTargetBucket(Excl)

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(renameLock, &gScbTable.rwLock);
    LWIO_LOCK_RWMUTEX_EXCLUSIVE(currentBucketLock, &pCurrentBucket->rwLock);
    if (pCurrentBucket != pTargetBucket)
    {
        // Will be moved to a new bucket in the table
        LWIO_LOCK_RWMUTEX_EXCLUSIVE(targetBucketLock, &pTargetBucket->rwLock);
    }

    ntError = PvfsRenameFCB(pCcb->pScb->pOwnerFcb, pCcb, NewFileName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsGetFullStreamname(&newFullStreamName, pCcb->pScb);
    BAIL_ON_NT_STATUS(ntError);

    LWIO_LOCK_MUTEX(scbLock, &pCcb->pScb->BaseControlBlock.Mutex);

    ntError = PvfsCbTableRemove_inlock(pCurrentBucket, currentFullStreamName);
    BAIL_ON_NT_STATUS(ntError);

    ntError = PvfsCbTableAdd_inlock(
                  pTargetBucket,
                  newFullStreamName,
                  (PPVFS_CONTROL_BLOCK)pCcb->pScb);
    BAIL_ON_NT_STATUS(ntError);

    pCcb->pScb->BaseControlBlock.pBucket = pTargetBucket;

    LWIO_UNLOCK_MUTEX(scbLock, &pCcb->pScb->BaseControlBlock.Mutex);


error:
    LWIO_UNLOCK_RWMUTEX(renameLock, &gScbTable.rwLock);
    LWIO_UNLOCK_RWMUTEX(currentBucketLock, &pCurrentBucket->rwLock);
    LWIO_UNLOCK_RWMUTEX(targetBucketLock, &pTargetBucket->rwLock);

    if (currentFullStreamName)
    {
        LwRtlCStringFree(&currentFullStreamName);
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
    PSTR NewFileName
    )
{
    return STATUS_NOT_IMPLEMENTED;
}
