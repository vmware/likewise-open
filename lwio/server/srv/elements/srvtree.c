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
 *        srvtree.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Tree Object
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
BOOLEAN
SrvTreeIsInParent_inlock(
    PLWIO_SRV_TREE pTree
    );

static
NTSTATUS
SrvTreeAddFile_inlock(
    PLWIO_SRV_TREE pTree,
    PLWIO_SRV_FILE pFile
    );

static
NTSTATUS
SrvTreeAcquireFileId_inlock(
   PLWIO_SRV_TREE pTree,
   PUSHORT       pFid
   );

static
int
SrvTreeFileCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvTreeFileRelease(
    PVOID pFile
    );

static
VOID
SrvTreeFree(
    PLWIO_SRV_TREE pTree
    );

static
int
SrvTreeAsyncStateCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
BOOLEAN
SrvTreeRundownAsyncStateCallback(
    PLWIO_ASYNC_STATE pAsyncState,
    PVOID pContext
    );

static
VOID
SrvTreeAsyncStateRelease(
    PVOID pAsyncState
    );

static
BOOLEAN
SrvTreeIsRundown_inlock(
    PLWIO_SRV_TREE pTree
    );

static
VOID
SrvTreeSetRundown_inlock(
    PLWIO_SRV_TREE pTree
    );

static
BOOLEAN
SrvTreeGatherRundownFileListCallback(
    PLWIO_SRV_FILE pFile,
    PVOID pContext
    );

static
VOID
SrvTreeRundownFileList(
    PLWIO_SRV_FILE pRundownList
    );


NTSTATUS
SrvTreeCreate(
    PLWIO_SRV_SESSION pSession,
    USHORT            tid,
    PSRV_SHARE_INFO   pShareInfo,
    PLWIO_SRV_TREE*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_TREE pTree = NULL;

    LWIO_LOG_DEBUG("Creating Tree [tid: %u]", tid);

    ntStatus = SrvAllocateMemory(sizeof(LWIO_SRV_TREE), (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->refcount = 1;

    pthread_rwlock_init(&pTree->mutex, NULL);
    pTree->pMutex = &pTree->mutex;

    pTree->pSession = pSession;;
    SrvSessionAcquire(pSession);

    pTree->tid = tid;
    pTree->uid = pSession->uid;
    pTree->ulConnectionResourceId = pSession->ulConnectionResourceId;

    LWIO_LOG_DEBUG("Associating Tree [object:0x%x][tid:%u]",
                    pTree,
                    tid);

    pTree->pShareInfo = SrvShareAcquireInfo(pShareInfo);

    ntStatus = LwRtlRBTreeCreate(
                    &SrvTreeFileCompare,
                    NULL,
                    &SrvTreeFileRelease,
                    &pTree->pFileCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeCreate(
                    &SrvTreeAsyncStateCompare,
                    NULL,
                    &SrvTreeAsyncStateRelease,
                    &pTree->pAsyncStateCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    SRV_ELEMENTS_INCREMENT_TREE_CONNECTS;

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    *ppTree = NULL;

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

    goto cleanup;
}

BOOLEAN
SrvTreeIsInParent(
    PLWIO_SRV_TREE pTree
    )
{
    BOOLEAN bIsInParent = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);
    bIsInParent = SrvTreeIsInParent_inlock(pTree);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return bIsInParent;
}

static
BOOLEAN
SrvTreeIsInParent_inlock(
    PLWIO_SRV_TREE pTree
    )
{
    return IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
}

VOID
SrvTreeSetInParent(
    PLWIO_SRV_TREE pTree
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);
    LWIO_ASSERT(!IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    SetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);
}

VOID
SrvTreeClearInParent(
    PLWIO_SRV_TREE pTree
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);
    LWIO_ASSERT(IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    ClearFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);
}

NTSTATUS
SrvTreeFindFile(
    PLWIO_SRV_TREE  pTree,
    USHORT         fid,
    PLWIO_SRV_FILE* ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_FILE pFile = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->mutex);

    pFile = pTree->lruFile[fid % SRV_LRU_CAPACITY];

    if (!pFile || (pFile->fid != fid))
    {
        ntStatus = LwRtlRBTreeFind(
                        pTree->pFileCollection,
                        &fid,
                        (PVOID*)&pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        pTree->lruFile[fid % SRV_LRU_CAPACITY] = pFile;
    }

    *ppFile = SrvFileAcquire(pFile);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_INVALID_HANDLE;
    }

    *ppFile = NULL;

    goto cleanup;
}

NTSTATUS
SrvTreeCreateFile(
    PLWIO_SRV_TREE          pTree,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_SRV_FILE*         ppFile
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_FILE pFile = NULL;
    USHORT  fid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    if (SrvTreeIsRundown_inlock(pTree))
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvTreeAcquireFileId_inlock(
                    pTree,
                    &fid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFileCreate(
                    pTree,
                    fid,
                    pwszFilename,
                    phFile,
                    ppFilename,
                    desiredAccess,
                    allocationSize,
                    fileAttributes,
                    shareAccess,
                    createDisposition,
                    createOptions,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeAddFile_inlock(pTree, pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppFile = pFile;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    *ppFile = NULL;

    if (pFile)
    {
        SrvFileRundown(pFile);
        SrvFileRelease(pFile);
    }

    goto cleanup;
}

static
NTSTATUS
SrvTreeAddFile_inlock(
    PLWIO_SRV_TREE pTree,
    PLWIO_SRV_FILE pFile
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = LwRtlRBTreeAdd(
                    pTree->pFileCollection,
                    &pFile->fid,
                    pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    // Reference from parent
    SrvFileAcquire(pFile);
    SrvFileSetInParent(pFile);

    pTree->lruFile[pFile->fid % SRV_LRU_CAPACITY] = pFile;
    pTree->ulNumOpenFiles++;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvTreeRemoveFile(
    PLWIO_SRV_TREE pTree,
    PLWIO_SRV_FILE pFile
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_FILE pCachedFile = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    if (SrvFileIsInParent(pFile))
    {
        pCachedFile = pTree->lruFile[ pFile->fid % SRV_LRU_CAPACITY ];
        if (pCachedFile && (pCachedFile->fid == pFile->fid))
        {
            pTree->lruFile[ pFile->fid % SRV_LRU_CAPACITY ] = NULL;
        }

        // removal automatically releases reference
        ntStatus = LwRtlRBTreeRemove(
                        pTree->pFileCollection,
                        &pFile->fid);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvFileClearInParent(pFile);

        pTree->ulNumOpenFiles--;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvTreeAddAsyncState(
    PLWIO_SRV_TREE    pTree,
    PLWIO_ASYNC_STATE pAsyncState
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;
    PLWIO_ASYNC_STATE pAsyncState1 = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    if (SrvTreeIsRundown_inlock(pTree))
    {
        ntStatus = STATUS_INVALID_HANDLE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LwRtlRBTreeFind(
                    pTree->pAsyncStateCollection,
                    &pAsyncState->ullAsyncId,
                    (PVOID*)&pAsyncState1);
    switch (ntStatus)
    {
        case STATUS_NOT_FOUND:

            ntStatus = LwRtlRBTreeAdd(
                            pTree->pAsyncStateCollection,
                            &pAsyncState->ullAsyncId,
                            pAsyncState);
            BAIL_ON_NT_STATUS(ntStatus);

            SrvAsyncStateAcquire(pAsyncState);

            break;

        case STATUS_SUCCESS:

            ntStatus = STATUS_DUPLICATE_OBJECTID;

            break;

        default:

            ;
    }
    BAIL_ON_NT_STATUS(ntStatus);

error:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;
}

NTSTATUS
SrvTreeFindAsyncState(
    PLWIO_SRV_TREE     pTree,
    ULONG64            ullAsyncId,
    PLWIO_ASYNC_STATE* ppAsyncState
    )
{
    NTSTATUS          ntStatus = STATUS_SUCCESS;
    PLWIO_ASYNC_STATE pAsyncState = NULL;
    BOOLEAN           bInLock     = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pTree->pAsyncStateCollection,
                    &ullAsyncId,
                    (PVOID*)&pAsyncState);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppAsyncState = SrvAsyncStateAcquire(pAsyncState);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    *ppAsyncState = NULL;

    goto cleanup;
}

VOID
SrvTreeRemoveAsyncState(
    PLWIO_SRV_TREE pTree,
    ULONG64        ullAsyncId
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN  bInLock  = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pTree->pAsyncStateCollection,
                    &ullAsyncId);
    if ((ntStatus != STATUS_SUCCESS) && (ntStatus != STATUS_NOT_FOUND))
    {
        LWIO_LOG_ERROR("Unexpected error removing async state, "
                       "status = 0x%08X (%s)",
                       ntStatus, LwNtStatusToName(ntStatus));
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);
}

BOOLEAN
SrvTreeIsNamedPipe(
    PLWIO_SRV_TREE pTree
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    bResult = (pTree->pShareInfo->service == SHARE_SERVICE_NAMED_PIPE);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    return bResult;
}

NTSTATUS
SrvGetTreeRelativePath(
    PWSTR  pwszOriginalPath,
    PWSTR* ppwszSpecificPath
    )
{
    NTSTATUS ntStatus        = STATUS_SUCCESS;
    wchar16_t wszBackSlash[] = { '\\', 0 };
    wchar16_t wszFwdSlash[]  = { '/',  0 };

    if ((*pwszOriginalPath != wszBackSlash[0]) &&
         (*pwszOriginalPath != wszFwdSlash[0]))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }
    pwszOriginalPath++;

    // Skip the device name
    while (!IsNullOrEmptyString(pwszOriginalPath) &&
           (*pwszOriginalPath != wszBackSlash[0]) &&
           (*pwszOriginalPath != wszFwdSlash[0]))
    {
        pwszOriginalPath++;
    }

    if (IsNullOrEmptyString(pwszOriginalPath) ||
        ((*pwszOriginalPath != wszBackSlash[0]) &&
         (*pwszOriginalPath != wszFwdSlash[0])))
    {
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *ppwszSpecificPath = pwszOriginalPath;

cleanup:

    return ntStatus;

error:

    *ppwszSpecificPath = NULL;

    goto cleanup;
}

PLWIO_SRV_TREE
SrvTreeAcquire(
    PLWIO_SRV_TREE pTree
    )
{
    LWIO_LOG_DEBUG("Acquiring tree [tid:%u]", pTree->tid);

    InterlockedIncrement(&pTree->refcount);

    return pTree;
}

VOID
SrvTreeRelease(
    PLWIO_SRV_TREE pTree
    )
{
    LWIO_LOG_DEBUG("Releasing tree [tid:%u]", pTree->tid);

    if (InterlockedDecrement(&pTree->refcount) == 0)
    {
        SRV_ELEMENTS_DECREMENT_TREE_CONNECTS;

        SrvTreeFree(pTree);
    }
}

VOID
SrvTreeRundown(
    PLWIO_SRV_TREE pTree
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bDoRundown = FALSE;
    BOOLEAN bIsInParent = FALSE;
    PLWIO_SRV_FILE pRundownFileList = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    if (!SrvTreeIsRundown_inlock(pTree))
    {
        SrvTreeSetRundown_inlock(pTree);

        bDoRundown = TRUE;
        bIsInParent = SrvTreeIsInParent_inlock(pTree);

        SrvEnumAsyncStateCollection(
                pTree->pAsyncStateCollection,
                SrvTreeRundownAsyncStateCallback,
                NULL);

        LwRtlRBTreeRemoveAll(pTree->pAsyncStateCollection);

        SrvEnumFileCollection(
                pTree->pFileCollection,
                SrvTreeGatherRundownFileListCallback,
                &pRundownFileList);

    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    if (bIsInParent)
    {
        SrvSessionRemoveTree(pTree->pSession, pTree);
    }

    if (bDoRundown)
    {
        // Cannot rundown with lock held as they self-remove
        SrvTreeRundownFileList(pRundownFileList);
    }
}

static
NTSTATUS
SrvTreeAcquireFileId_inlock(
   PLWIO_SRV_TREE pTree,
   PUSHORT       pFid
   )
{
    NTSTATUS ntStatus = 0;
    USHORT   candidateFid = pTree->nextAvailableFid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_SRV_FILE pFile = NULL;

        /* 0 is never a valid fid */

        if ((candidateFid == 0) || (candidateFid == UINT16_MAX))
        {
            candidateFid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pTree->pFileCollection,
                        &candidateFid,
                        (PVOID*)&pFile);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
        else
        {
            candidateFid++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateFid != pTree->nextAvailableFid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_OPENED_FILES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pFid = candidateFid;

    /* Increment by 1 by make sure tyo deal with wraparound */

    candidateFid++;
    pTree->nextAvailableFid = candidateFid ? candidateFid : 1;

cleanup:

    return ntStatus;

error:

    *pFid = 0;

    goto cleanup;
}

static
int
SrvTreeFileCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PUSHORT pFid1 = (PUSHORT)pKey1;
    PUSHORT pFid2 = (PUSHORT)pKey2;

    if (*pFid1 > *pFid2)
    {
        return 1;
    }
    else if (*pFid1 < *pFid2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
VOID
SrvTreeFileRelease(
    PVOID pFile
    )
{
    SrvFileRelease((PLWIO_SRV_FILE)pFile);
}

static
VOID
SrvTreeFree(
    PLWIO_SRV_TREE pTree
    )
{
    LWIO_LOG_DEBUG("Freeing tree [object:0x%x][tid:%u]",
                    pTree,
                    pTree->tid);

    // Cannot be in the parent since parent would have a reference.
    LWIO_ASSERT(!SrvTreeIsInParent_inlock(pTree));

    if (pTree->pMutex)
    {
        pthread_rwlock_destroy(&pTree->mutex);
        pTree->pMutex = NULL;
    }

    if (pTree->pFileCollection)
    {
        LwRtlRBTreeFree(pTree->pFileCollection);
    }

    if (pTree->pAsyncStateCollection)
    {
        LwRtlRBTreeFree(pTree->pAsyncStateCollection);
    }

    if (pTree->hFile)
    {
        IoCloseFile(pTree->hFile);
    }

    if (pTree->pShareInfo)
    {
        SrvShareReleaseInfo(pTree->pShareInfo);
    }

    // Release parent at the end
    if (pTree->pSession)
    {
        SrvSessionRelease(pTree->pSession);
    }

    SrvFreeMemory(pTree);
}

static
int
SrvTreeAsyncStateCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PULONG64 pAsyncId1 = (PULONG64)pKey1;
    PULONG64 pAsyncId2 = (PULONG64)pKey2;

    if (*pAsyncId1 > *pAsyncId2)
    {
        return 1;
    }
    else if (*pAsyncId1 < *pAsyncId2)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

static
BOOLEAN
SrvTreeRundownAsyncStateCallback(
    PLWIO_ASYNC_STATE pAsyncState,
    PVOID pContext
    )
{
    SrvAsyncStateCancel(pAsyncState);

    return TRUE;
}

static
VOID
SrvTreeAsyncStateRelease(
    PVOID pAsyncState
    )
{
    SrvAsyncStateRelease((PLWIO_ASYNC_STATE)pAsyncState);
}

static
BOOLEAN
SrvTreeIsRundown_inlock(
    PLWIO_SRV_TREE pTree
    )
{
    return IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}

static
VOID
SrvTreeSetRundown_inlock(
    PLWIO_SRV_TREE pTree
    )
{
    LWIO_ASSERT(!IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_RUNDOWN));
    SetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}

static
BOOLEAN
SrvTreeGatherRundownFileListCallback(
    PLWIO_SRV_FILE pFile,
    PVOID pContext
    )
{
    PLWIO_SRV_FILE* ppRundownList = (PLWIO_SRV_FILE*) pContext;

    LWIO_ASSERT(!pFile->pRundownNext);
    pFile->pRundownNext = *ppRundownList;
    *ppRundownList = SrvFileAcquire(pFile);

    return TRUE;
}

static
VOID
SrvTreeRundownFileList(
    PLWIO_SRV_FILE pRundownList
    )
{
    while (pRundownList)
    {
        PLWIO_SRV_FILE pFile = pRundownList;

        pRundownList = pFile->pRundownNext;
        SrvFileRundown(pFile);
        SrvFileRelease(pFile);
    }
}

/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
