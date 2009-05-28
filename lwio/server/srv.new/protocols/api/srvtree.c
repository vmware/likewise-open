#include "includes.h"

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

NTSTATUS
SrvTreeCreate(
    USHORT            tid,
    PSHARE_DB_INFO    pShareInfo,
    PLWIO_SRV_TREE*    ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_TREE pTree = NULL;

    LWIO_LOG_DEBUG("Creating Tree [tid: %u]", tid);

    ntStatus = LW_RTL_ALLOCATE(&pTree, LWIO_SRV_TREE, sizeof(LWIO_SRV_TREE));
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->refcount = 1;

    pthread_rwlock_init(&pTree->mutex, NULL);
    pTree->pMutex = &pTree->mutex;

    pTree->tid = tid;

    LWIO_LOG_DEBUG("Associating Tree [object:0x%x][tid:%u]",
                    pTree,
                    tid);

    pTree->pShareInfo = pShareInfo;
    InterlockedIncrement(&pShareInfo->refcount);

    ntStatus = LwRtlRBTreeCreate(
                    &SrvTreeFileCompare,
                    NULL,
                    &SrvTreeFileRelease,
                    &pTree->pFileCollection);
    BAIL_ON_NT_STATUS(ntStatus);

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

    ntStatus = LwRtlRBTreeFind(
                    pTree->pFileCollection,
                    &fid,
                    (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pFile->refcount);

    *ppFile = pFile;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    *ppFile = NULL;

    goto cleanup;
}

NTSTATUS
SrvTreeCreateFile(
    PLWIO_SRV_TREE           pTree,
    PWSTR                   pwszFilename,
    PIO_FILE_HANDLE         phFile,
    PIO_FILE_NAME*          ppFilename,
    ACCESS_MASK             desiredAccess,
    LONG64                  allocationSize,
    FILE_ATTRIBUTES         fileAttributes,
    FILE_SHARE_FLAGS        shareAccess,
    FILE_CREATE_DISPOSITION createDisposition,
    FILE_CREATE_OPTIONS     createOptions,
    PLWIO_SRV_FILE*          ppFile
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_FILE pFile = NULL;
    USHORT  fid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    ntStatus = SrvTreeAcquireFileId_inlock(
                    pTree,
                    &fid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFileCreate(
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

    ntStatus = LwRtlRBTreeAdd(
                    pTree->pFileCollection,
                    &pFile->fid,
                    pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pFile->refcount);

    *ppFile = pFile;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    *ppFile = NULL;

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    goto cleanup;
}

NTSTATUS
SrvTreeRemoveFile(
    PLWIO_SRV_TREE pTree,
    USHORT        fid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pTree->pFileCollection,
                    &fid);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    goto cleanup;
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

VOID
SrvTreeRelease(
    PLWIO_SRV_TREE pTree
    )
{
    LWIO_LOG_DEBUG("Releasing tree [tid:%u]", pTree->tid);

    if (InterlockedDecrement(&pTree->refcount) == 0)
    {
        SrvTreeFree(pTree);
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

    if (pTree->pMutex)
    {
        pthread_rwlock_destroy(&pTree->mutex);
        pTree->pMutex = NULL;
    }

    if (pTree->pFileCollection)
    {
        LwRtlRBTreeFree(pTree->pFileCollection);
    }

    if (pTree->pShareInfo)
    {
        SrvShareDbReleaseInfo(pTree->pShareInfo);
    }

    LwRtlMemoryFree(pTree);
}



/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
