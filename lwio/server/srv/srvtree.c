#include "includes.h"

static
int
SrvTreeFileCompare(
    PVOID pFile1,
    PVOID pFile2
    );

static
VOID
SrvTreeFileRelease(
    PVOID pFile
    );

static
VOID
SrvTreeFree(
    PSMB_SRV_TREE pTree
    );

NTSTATUS
SrvTreeCreate(
    PSRV_ID_ALLOCATOR pIdAllocator,
    PSMB_SRV_TREE*    ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_TREE pTree = NULL;
    USHORT tid  = 0;

    pthread_rwlock_init(&pTree->mutex, NULL);
    pTree->pMutex = &pTree->mutex;

    ntStatus = SMBAllocateMemory(
                    sizeof(PSMB_SRV_TREE),
                    (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->refcount = 1;

    ntStatus = SrvIdAllocatorAcquireId(
                    pIdAllocator,
                    &tid);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->tid = tid;

    pTree->pTreeIdAllocator = pIdAllocator;
    InterlockedIncrement(&pIdAllocator->refcount);

    ntStatus = SrvIdAllocatorCreate(
                    UINT16_MAX,
                    &pTree->pFileIdAllocator);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBRBTreeCreate(
                    &SrvTreeFileCompare,
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
    PSMB_SRV_TREE  pTree,
    USHORT         fid,
    PSMB_SRV_FILE* ppFile
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_FILE pFile = NULL;
    BOOLEAN bInLock = FALSE;
    SMB_SRV_FILE finder;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pTree->mutex);

    memset(&finder, 0, sizeof(finder));
    finder.fid = fid;

    ntStatus = SMBRBTreeFind(
                    pTree->pFileCollection,
                    &finder,
                    (PVOID*)&pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pFile->refcount);

    *ppFile = pFile;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

    return ntStatus;

error:

    *ppFile = NULL;

    goto cleanup;
}

NTSTATUS
SrvTreeCreateFile(
    PSMB_SRV_TREE  pTree,
    PSMB_SRV_FILE* ppFile
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SRV_FILE pFile = NULL;

    ntStatus = SrvFileCreate(
                    pTree->pFileIdAllocator,
                    &pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pTree->mutex);

    ntStatus = SMBRBTreeAdd(
                    pTree->pFileCollection,
                    pFile);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pFile->refcount);

    *ppFile = pFile;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pTree->mutex);

error:

    *ppFile = NULL;

    if (pFile)
    {
        SrvFileRelease(pFile);
    }

    goto cleanup;
}

VOID
SrvTreeRelease(
    PSMB_SRV_TREE pTree
    )
{
    if (InterlockedDecrement(&pTree->refcount) == 0)
    {
        SrvTreeFree(pTree);
    }
}

static
int
SrvTreeFileCompare(
    PVOID pFile1,
    PVOID pFile2
    )
{
    PSMB_SRV_FILE pFile1_casted = (PSMB_SRV_FILE)pFile1;
    PSMB_SRV_FILE pFile2_casted = (PSMB_SRV_FILE)pFile2;

    // tids will not change after the tree is created
    // so, no need to lock the tree objects when fetching tids
    if (pFile1_casted->fid > pFile2_casted->fid)
    {
        return 1;
    }
    else if (pFile1_casted->fid < pFile2_casted->fid)
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
    PVOID pTree
    )
{
    SrvFileRelease((PSMB_SRV_FILE)pTree);
}

static
VOID
SrvTreeFree(
    PSMB_SRV_TREE pTree
    )
{
    if (pTree->pMutex)
    {
        pthread_rwlock_destroy(&pTree->mutex);
        pTree->pMutex = NULL;
    }

    if (pTree->pTreeIdAllocator)
    {
        SrvIdAllocatorReleaseId(
                pTree->pTreeIdAllocator,
                pTree->tid);

        SrvIdAllocatorRelease(
                pTree->pTreeIdAllocator);
    }

    if (pTree->pFileCollection)
    {
        SMBRBTreeFree(pTree->pFileCollection);
    }

    if (pTree->pFileIdAllocator)
    {
        SrvIdAllocatorRelease(
                pTree->pFileIdAllocator);
    }

    SMBFreeMemory(pTree);
}

