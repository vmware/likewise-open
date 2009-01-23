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
    USHORT            uid,
    PSMB_SRV_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SRV_SESSION pSession = NULL;

    pthread_mutex_init(&pSession->mutex, NULL);
    pSession->pMutex = &pSession->mutex;

    ntStatus = SMBAllocateMemory(
                    sizeof(PSMB_SRV_SESSION),
                    (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->uid = uid;
    pSession->refcount = 1;

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
SrvSessionCreateTree(
    PSMB_SRV_SESSION pSession,
    USHORT           tid,
    PSMB_SRV_TREE*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SRV_TREE pTree = NULL;

    SMB_LOCK_MUTEX(bInLock, &pSession->mutex);

    pTree = SMBRBTreeFind(
                    pSession->pTreeCollection,
                    &tid);
    if (!pTree)
    {
        ntStatus = SrvTreeCreate(
                        tid,
                        &pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBRBTreeAdd(
                        pSession->pTreeCollection,
                        pTree);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    InterlockedIncrement(&pTree->refcount);

    *ppTree = pTree;

cleanup:

    SMB_UNLOCK_MUTEX(bInLock, &pSession->mutex);

error:

    *ppTree = NULL;

    if (pTree)
    {
        SrvTreeRelease(pTree);
    }

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
        pthread_mutex_destroy(&pSession->mutex);
        pSession->pMutex = NULL;
    }

    if (pSession->pTreeCollection)
    {
        SMBRBTreeFree(pSession->pTreeCollection);
    }

    SMBFreeMemory(pSession);
}
