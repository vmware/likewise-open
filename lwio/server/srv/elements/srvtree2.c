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
 *        srvtree2.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Tree Object (Version 2)
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
BOOLEAN
SrvTree2IsInParent_inlock(
    PLWIO_SRV_TREE_2 pTree
    );

static
NTSTATUS
SrvTree2AddFile_inlock(
    PLWIO_SRV_TREE_2 pTree,
    PLWIO_SRV_FILE_2 pFile
    );

static
NTSTATUS
SrvTree2AcquireFileId_inlock(
   PLWIO_SRV_TREE_2 pTree,
   PSMB2_FID        pFid
   );

static
int
SrvTree2FileCompare(
    PVOID pKey1,
    PVOID pKey2
    );

static
VOID
SrvTree2FileRelease(
    PVOID pFile
    );

static
VOID
SrvTree2Free(
    PLWIO_SRV_TREE_2 pTree
    );

static
BOOLEAN
SrvTree2IsRundown_inlock(
    PLWIO_SRV_TREE_2 pTree
    );

static
VOID
SrvTree2SetRundown_inlock(
    PLWIO_SRV_TREE_2 pTree
    );

static
BOOLEAN
SrvTree2GatherRundownFileListCallback(
    PLWIO_SRV_FILE_2 pFile,
    PVOID pContext
    );

static
VOID
SrvTree2RundownFileList(
    PLWIO_SRV_FILE_2 pRundownList
    );


NTSTATUS
SrvTree2Create(
    PLWIO_SRV_SESSION_2 pSession,
    ULONG             ulTid,
    PSRV_SHARE_INFO   pShareInfo,
    PLWIO_SRV_TREE_2* ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_TREE_2 pTree = NULL;

    LWIO_LOG_DEBUG("Creating Tree [tid: %u]", ulTid);

    ntStatus = SrvAllocateMemory(sizeof(LWIO_SRV_TREE_2), (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->refcount = 1;

    pthread_rwlock_init(&pTree->mutex, NULL);
    pTree->pMutex = &pTree->mutex;

    pTree->pSession = pSession;;
    SrvSession2Acquire(pSession);

    pTree->ulTid = ulTid;
    pTree->ullUid = pSession->ullUid;

    pTree->resource.resourceType                  = SRV_RESOURCE_TYPE_TREE;
    pTree->resource.pAttributes                   = &pTree->resourceAttrs;
    pTree->resource.pAttributes->protocolVersion  = SMB_PROTOCOL_VERSION_2;
    pTree->resource.pAttributes->treeId.ulTid     = pTree->ulTid;
    pTree->resource.pAttributes->sessionId.ullUid = pTree->ullUid;
    pTree->resource.pAttributes->ulConnectionResourceId =
                                             pSession->ulConnectionResourceId;

    LWIO_LOG_DEBUG("Associating Tree [object:0x%x][tid:%u]",
                    pTree,
                    ulTid);

    pTree->pShareInfo = pShareInfo;
    InterlockedIncrement(&pShareInfo->refcount);

    pTree->ullNextAvailableFid = 0xFFFFFFFF00000001LL;

    ntStatus = LwRtlRBTreeCreate(
                    &SrvTree2FileCompare,
                    NULL,
                    &SrvTree2FileRelease,
                    &pTree->pFileCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    SRV_ELEMENTS_INCREMENT_TREE_CONNECTS;

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    *ppTree = NULL;

    if (pTree)
    {
        SrvTree2Release(pTree);
    }

    goto cleanup;
}

BOOLEAN
SrvTree2IsInParent(
    PLWIO_SRV_TREE_2 pTree
    )
{
    BOOLEAN bIsInParent = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);
    bIsInParent = SrvTree2IsInParent_inlock(pTree);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return bIsInParent;
}

static
BOOLEAN
SrvTree2IsInParent_inlock(
    PLWIO_SRV_TREE_2 pTree
    )
{
    return IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
}

VOID
SrvTree2SetInParent(
    PLWIO_SRV_TREE_2 pTree
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);
    LWIO_ASSERT(!IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    SetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);
}

VOID
SrvTree2ClearInParent(
    PLWIO_SRV_TREE_2 pTree
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);
    LWIO_ASSERT(IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT));
    ClearFlag(pTree->objectFlags, SRV_OBJECT_FLAG_IN_PARENT);
    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);
}

NTSTATUS
SrvTree2FindFile(
    PLWIO_SRV_TREE_2  pTree,
    PSMB2_FID         pFid,
    PLWIO_SRV_FILE_2* ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_FILE_2 pFile = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->mutex);

    pFile = pTree->lruFile[ pFid->ullVolatileId % SRV_LRU_CAPACITY ];

    if (!pFile ||
        (pFile->fid.ullPersistentId != pFid->ullPersistentId) ||
        (pFile->fid.ullVolatileId != pFid->ullVolatileId))
    {
        ntStatus = LwRtlRBTreeFind(
                        pTree->pFileCollection,
                        pFid,
                        (PVOID*)&pFile);
        BAIL_ON_NT_STATUS(ntStatus);

        pTree->lruFile[pFid->ullVolatileId % SRV_LRU_CAPACITY] = pFile;
    }

    InterlockedIncrement(&pFile->refcount);

    *ppFile = pFile;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:
    if (ntStatus == STATUS_NOT_FOUND)
    {
        ntStatus = STATUS_FILE_CLOSED;
    }

    *ppFile = NULL;

    goto cleanup;
}

NTSTATUS
SrvTree2CreateFile(
    PLWIO_SRV_TREE_2        pTree,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_SRV_FILE_2*       ppFile
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_FILE_2 pFile = NULL;
    SMB2_FID  fid = {0};

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    ntStatus = SrvTree2AcquireFileId_inlock(
                    pTree,
                    &fid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFile2Create(
                    pTree,
                    &fid,
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

    ntStatus = SrvTree2AddFile_inlock(pTree, pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    SrvFile2BlockIdleTimeout(pFile);

    *ppFile = pFile;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    *ppFile = NULL;

    if (pFile)
    {
        SrvFile2Rundown(pFile);
        SrvFile2Release(pFile);
    }

    goto cleanup;
}

static
NTSTATUS
SrvTree2AddFile_inlock(
    PLWIO_SRV_TREE_2 pTree,
    PLWIO_SRV_FILE_2 pFile
    )
{
    NTSTATUS ntStatus = 0;

    ntStatus = LwRtlRBTreeAdd(
                    pTree->pFileCollection,
                    &pFile->fid,
                    pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    // Reference from parent
    SrvFile2Acquire(pFile);
    SrvFile2SetInParent(pFile);

    pTree->lruFile[pFile->fid.ullVolatileId % SRV_LRU_CAPACITY] = pFile;
    pTree->ulNumOpenFiles++;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvTree2RemoveFile(
    PLWIO_SRV_TREE_2 pTree,
    PLWIO_SRV_FILE_2 pFile
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_FILE_2 pCachedFile = NULL;
    PSMB2_FID pFid = &pFile->fid;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    if (SrvFile2IsInParent(pFile))
    {
        pCachedFile = pTree->lruFile[ pFid->ullVolatileId % SRV_LRU_CAPACITY ];
        if (pCachedFile &&
            (pCachedFile->fid.ullPersistentId == pFid->ullPersistentId) &&
            (pCachedFile->fid.ullVolatileId == pFid->ullVolatileId))
        {
            pTree->lruFile[ pFid->ullVolatileId % SRV_LRU_CAPACITY ] = NULL;
        }

        // removal automatically releases reference
        ntStatus = LwRtlRBTreeRemove(pTree->pFileCollection, pFid);
        BAIL_ON_NT_STATUS(ntStatus);

        SrvFile2ClearInParent(pFile);

        pTree->ulNumOpenFiles--;
    }

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    goto cleanup;
}

BOOLEAN
SrvTree2IsNamedPipe(
    PLWIO_SRV_TREE_2 pTree
    )
{
    BOOLEAN bResult = FALSE;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pTree->pShareInfo->mutex);

    bResult = (pTree->pShareInfo->service == SHARE_SERVICE_NAMED_PIPE);

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->pShareInfo->mutex);

    return bResult;
}

PLWIO_SRV_TREE_2
SrvTree2Acquire(
    PLWIO_SRV_TREE_2 pTree
    )
{
    LWIO_LOG_DEBUG("Acquring tree [tid:%u]", pTree->ulTid);

    InterlockedIncrement(&pTree->refcount);

    return pTree;
}

VOID
SrvTree2Release(
    PLWIO_SRV_TREE_2 pTree
    )
{
    LWIO_LOG_DEBUG("Releasing tree [tid:%u]", pTree->ulTid);

    if (InterlockedDecrement(&pTree->refcount) == 0)
    {
        SRV_ELEMENTS_DECREMENT_TREE_CONNECTS;

        SrvTree2Free(pTree);
    }
}

VOID
SrvTree2Rundown(
    PLWIO_SRV_TREE_2 pTree
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bDoRundown = FALSE;
    BOOLEAN bIsInParent = FALSE;
    PLWIO_SRV_FILE_2 pRundownFileList = NULL;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    if (!SrvTree2IsRundown_inlock(pTree))
    {
        SrvTree2SetRundown_inlock(pTree);

        bDoRundown = TRUE;
        bIsInParent = SrvTree2IsInParent_inlock(pTree);

        SrvEnumFile2Collection(
                pTree->pFileCollection,
                SrvTree2GatherRundownFileListCallback,
                &pRundownFileList);
    }

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    if (bIsInParent)
    {
        SrvSession2RemoveTree(pTree->pSession, pTree);
    }

    if (bDoRundown)
    {
        if (pTree->resource.ulResourceId)
        {
            PSRV_RESOURCE pResource = NULL;

            SrvElementsUnregisterResource(pTree->resource.ulResourceId,
                                          &pResource);
            pTree->resource.ulResourceId = 0;
        }

        // Cannot rundown with lock held as they self-remove
        SrvTree2RundownFileList(pRundownFileList);
    }
}

static
NTSTATUS
SrvTree2AcquireFileId_inlock(
   PLWIO_SRV_TREE_2 pTree,
   PSMB2_FID        pFid
   )
{
    NTSTATUS ntStatus = 0;
    SMB2_FID candidateFid = {   .ullPersistentId = 0xFFFFFFFFFFFFFFFFLL,
                                .ullVolatileId = pTree->ullNextAvailableFid
                            };
    BOOLEAN  bFound = FALSE;
    BOOLEAN  bInLock = FALSE;
    union {
        ULONG64 ullFileId;
        struct
        {
            ULONG ulFileId1;
            ULONG ulFileId2;
        } fileIdParts;
    } fileId;

    do
    {
        PLWIO_SRV_FILE_2 pFile = NULL;

        /* 0 is never a valid fid */

        if ((candidateFid.ullVolatileId == 0) ||
            (candidateFid.ullVolatileId == UINT64_MAX))
        {
            candidateFid.ullVolatileId = 1;
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
            candidateFid.ullVolatileId++;
        }
        BAIL_ON_NT_STATUS(ntStatus);

    } while (   (candidateFid.ullVolatileId != pTree->ullNextAvailableFid) &&
                !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_OPENED_FILES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_MUTEX(bInLock, gSrvElements.pMutex);

    fileId.fileIdParts.ulFileId1 = mt_genrand_int32(&gSrvElements.randGen);
    fileId.fileIdParts.ulFileId2 = mt_genrand_int32(&gSrvElements.randGen);

    candidateFid.ullPersistentId = fileId.ullFileId;

    LWIO_UNLOCK_MUTEX(bInLock, gSrvElements.pMutex);

    *pFid = candidateFid;

    /* Increment by 1 by make sure to deal with wraparound */

    candidateFid.ullVolatileId++;
    pTree->ullNextAvailableFid =
                candidateFid.ullVolatileId ? candidateFid.ullVolatileId : 1;

cleanup:

    return ntStatus;

error:

    pFid->ullPersistentId = 0LL;
    pFid->ullVolatileId = 0LL;

    goto cleanup;
}

static
int
SrvTree2FileCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PSMB2_FID pFid1 = (PSMB2_FID)pKey1;
    PSMB2_FID pFid2 = (PSMB2_FID)pKey2;

    return memcmp((PBYTE)pFid1, (PBYTE)pFid2, sizeof(SMB2_FID));
}

static
VOID
SrvTree2FileRelease(
    PVOID pFile
    )
{
    SrvFile2Release((PLWIO_SRV_FILE_2)pFile);
}

static
VOID
SrvTree2Free(
    PLWIO_SRV_TREE_2 pTree
    )
{
    LWIO_LOG_DEBUG("Freeing tree [object:0x%x][tid:%u]",
                    pTree,
                    pTree->ulTid);

    // Cannot be in the parent since parent would have a reference.
    LWIO_ASSERT(!SrvTree2IsInParent_inlock(pTree));

    if (pTree->pMutex)
    {
        pthread_rwlock_destroy(&pTree->mutex);
        pTree->pMutex = NULL;
    }

    if (pTree->pFileCollection)
    {
        LwRtlRBTreeFree(pTree->pFileCollection);
    }

    if (pTree->hFile)
    {
        IoCloseFile(pTree->hFile);
    }

    if (pTree->pShareInfo)
    {
        SrvShareReleaseInfo(pTree->pShareInfo);
    }

    if (pTree->resource.ulResourceId)
    {
        PSRV_RESOURCE pResource = NULL;

        SrvElementsUnregisterResource(pTree->resource.ulResourceId,
                                      &pResource);
        pTree->resource.ulResourceId = 0;
    }

    // Release parent at the end
    if (pTree->pSession)
    {
        SrvSession2Release(pTree->pSession);
    }

    SrvFreeMemory(pTree);
}

static
BOOLEAN
SrvTree2IsRundown_inlock(
    PLWIO_SRV_TREE_2 pTree
    )
{
    return IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}

static
VOID
SrvTree2SetRundown_inlock(
    PLWIO_SRV_TREE_2 pTree
    )
{
    LWIO_ASSERT(!IsSetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_RUNDOWN));
    SetFlag(pTree->objectFlags, SRV_OBJECT_FLAG_RUNDOWN);
}

static
BOOLEAN
SrvTree2GatherRundownFileListCallback(
    PLWIO_SRV_FILE_2 pFile,
    PVOID pContext
    )
{
    PLWIO_SRV_FILE_2* ppRundownList = (PLWIO_SRV_FILE_2*) pContext;

    LWIO_ASSERT(!pFile->pRundownNext);
    pFile->pRundownNext = *ppRundownList;
    *ppRundownList = SrvFile2Acquire(pFile);

    return TRUE;
}

static
VOID
SrvTree2RundownFileList(
    PLWIO_SRV_FILE_2 pRundownList
    )
{
    while (pRundownList)
    {
        PLWIO_SRV_FILE_2 pFile = pRundownList;

        pRundownList = pFile->pRundownNext;
        SrvFile2Rundown(pFile);
        SrvFile2Release(pFile);
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
