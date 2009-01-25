#include "includes.h"

static
int
SrvSessionTreeCompare(
    PVOID pTree1,
    PVOID pTree2
    );

static
VOID
SrvSessionTreeRelease(
    PVOID pTree
    );

static
VOID
SrvSessionFree(
    PSMB_SRV_SESSION pSession
    );

NTSTATUS
SrvSessionCreate(
    PSRV_ID_ALLOCATOR pIdAllocator,
    PSMB_SRV_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SESSION pSession = NULL;
    USHORT uid = 0;

    pthread_rwlock_init(&pSession->mutex, NULL);
    pSession->pMutex = &pSession->mutex;

    ntStatus = SMBAllocateMemory(
                    sizeof(PSMB_SRV_SESSION),
                    (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->refcount = 1;

    ntStatus = SrvIdAllocatorAcquireId(
                    pIdAllocator,
                    &uid);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->uid = uid;

    pSession->pSessionIdAllocator = pIdAllocator;
    InterlockedIncrement(&pIdAllocator->refcount);

    ntStatus = SrvIdAllocatorCreate(
                    UINT16_MAX,
                    &pSession->pTreeIdAllocator);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBRBTreeCreate(
                    &SrvSessionTreeCompare,
                    &SrvSessionTreeRelease,
                    &pSession->pTreeCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:

    *ppSession = NULL;

    if (pSession)
    {
        SrvSessionRelease(pSession);
    }

    goto cleanup;
}

NTSTATUS
SrvSessionFindTree(
    PSMB_SRV_SESSION pSession,
    USHORT           tid,
    PSMB_SRV_TREE*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SRV_TREE pTree = NULL;
    SMB_SRV_TREE finder;

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    memset(&finder, 0, sizeof(finder));
    finder.tid = tid;

    ntStatus = SMBRBTreeFind(
                    pSession->pTreeCollection,
                    &finder,
                    (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pTree->refcount);

    *ppTree = pTree;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *ppTree = NULL;

    goto cleanup;
}

NTSTATUS
SrvSessionRemoveTree(
    PSMB_SRV_SESSION pSession,
    USHORT           tid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    SMB_SRV_TREE finder;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    memset(&finder, 0, sizeof(finder));
    finder.tid = tid;

    ntStatus = SMBRBTreeRemove(
                    pSession->pTreeCollection,
                    &finder);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

error:

    goto cleanup;
}

NTSTATUS
SrvSessionCreateTree(
    PSMB_SRV_SESSION pSession,
    PSHARE_DB_INFO   pShareInfo,
    PSMB_SRV_TREE*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_TREE pTree = NULL;
    BOOLEAN bInLock = FALSE;

    ntStatus = SrvTreeCreate(
                    pSession->pTreeIdAllocator,
                    pShareInfo,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = SMBRBTreeAdd(
                    pSession->pTreeCollection,
                    pTree);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

VOID
SrvSessionRelease(
    PSMB_SRV_SESSION pSession
    )
{
    if (InterlockedDecrement(&pSession->refcount) == 0)
    {
        SrvSessionFree(pSession);
    }
}

static
int
SrvSessionTreeCompare(
    PVOID pTree1,
    PVOID pTree2
    )
{
    PSMB_SRV_TREE pTree1_casted = (PSMB_SRV_TREE)pTree1;
    PSMB_SRV_TREE pTree2_casted = (PSMB_SRV_TREE)pTree2;

    // tids will not change after the tree is created
    // so, no need to lock the tree objects when fetching tids
    if (pTree1_casted->tid > pTree2_casted->tid)
    {
        return 1;
    }
    else if (pTree1_casted->tid < pTree2_casted->tid)
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
SrvSessionTreeRelease(
    PVOID pTree
    )
{
    SrvTreeRelease((PSMB_SRV_TREE)pTree);
}

static
VOID
SrvSessionFree(
    PSMB_SRV_SESSION pSession
    )
{
    if (pSession->pMutex)
    {
        pthread_rwlock_destroy(&pSession->mutex);
        pSession->pMutex = NULL;
    }

    if (pSession->pSessionIdAllocator)
    {
        SrvIdAllocatorReleaseId(
                pSession->pSessionIdAllocator,
                pSession->uid);

        SrvIdAllocatorRelease(pSession->pSessionIdAllocator);
    }

    if (pSession->pTreeCollection)
    {
        SMBRBTreeFree(pSession->pTreeCollection);
    }

    if (pSession->pTreeIdAllocator)
    {
        SrvIdAllocatorRelease(pSession->pTreeIdAllocator);
    }

    SMBFreeMemory(pSession);
}
