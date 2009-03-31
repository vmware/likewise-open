#include "includes.h"

static
NTSTATUS
SrvSessionAcquireTreeId_inlock(
   PSMB_SRV_SESSION pSession,
   PUSHORT          pTid
   );

static
int
SrvSessionTreeCompare(
    PVOID pKey1,
    PVOID pKey2
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

    SMB_LOG_DEBUG("Creating session [uid:%u]", uid);

    ntStatus = LW_RTL_ALLOCATE(
                    &pSession,
                    SMB_SRV_SESSION,
                    sizeof(SMB_SRV_SESSION));
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->refcount = 1;

    pthread_rwlock_init(&pSession->mutex, NULL);
    pSession->pMutex = &pSession->mutex;

    pSession->uid = uid;

    SMB_LOG_DEBUG("Associating session [object:0x%x][uid:%u]", pSession, uid);

    ntStatus = LwRtlRBTreeCreate(
                    &SrvSessionTreeCompare,
                    NULL,
                    &SrvSessionTreeRelease,
                    &pSession->pTreeCollection);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvFinderCreateRepository(
                    &pSession->hFinderRepository);
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

    SMB_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pSession->pTreeCollection,
                    &tid,
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

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pSession->pTreeCollection,
                    &tid);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

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
    USHORT  tid = 0;

    SMB_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = SrvSessionAcquireTreeId_inlock(
                    pSession,
                    &tid);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvTreeCreate(
                    tid,
                    pShareInfo,
                    &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlRBTreeAdd(
                    pSession->pTreeCollection,
                    &pTree->tid,
                    pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pTree->refcount);

    *ppTree = pTree;

cleanup:

    SMB_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

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
SrvSessionGetNamedPipeClientPrincipal(
    IN     PSMB_SRV_SESSION pSession,
    IN OUT PIO_ECP_LIST     pEcpList
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSTR pszClientPrincipalName = pSession->pszClientPrincipalName;
    ULONG ulEcpLength = strlen(pszClientPrincipalName) + 1;

    ntStatus = IoRtlEcpListInsert(pEcpList,
                                  IO_ECP_TYPE_PEER_PRINCIPAL,
                                  pszClientPrincipalName,
                                  ulEcpLength,
                                  NULL);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

VOID
SrvSessionRelease(
    PSMB_SRV_SESSION pSession
    )
{
    SMB_LOG_DEBUG("Releasing session [uid:%u]", pSession->uid);

    if (InterlockedDecrement(&pSession->refcount) == 0)
    {
        SrvSessionFree(pSession);
    }
}

static
NTSTATUS
SrvSessionAcquireTreeId_inlock(
   PSMB_SRV_SESSION pSession,
   PUSHORT          pTid
   )
{
    NTSTATUS ntStatus = 0;
    USHORT   candidateTid = pSession->nextAvailableTid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PSMB_SRV_TREE pTree = NULL;

        /* 0 is never a valid tid */

        if ((candidateTid == 0) || (candidateTid == UINT16_MAX))
        {
            candidateTid = 1;
        }

        ntStatus = LwRtlRBTreeFind(
                        pSession->pTreeCollection,
                        &candidateTid,
                        (PVOID*)&pTree);
        if (ntStatus == STATUS_NOT_FOUND)
        {
            ntStatus = STATUS_SUCCESS;
            bFound = TRUE;
        }
	else
	{
            candidateTid++;
	}
        BAIL_ON_NT_STATUS(ntStatus);

    } while ((candidateTid != pSession->nextAvailableTid) && !bFound);

    if (!bFound)
    {
        ntStatus = STATUS_TOO_MANY_LINKS;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pTid = candidateTid;

    /* Increment by 1 by make sure tyo deal with wraparound */

    candidateTid++;
    pSession->nextAvailableTid = candidateTid ? candidateTid : 1;

cleanup:

    return ntStatus;

error:

    *pTid = 0;

    goto cleanup;
}

static
int
SrvSessionTreeCompare(
    PVOID pKey1,
    PVOID pKey2
    )
{
    PUSHORT pTid1 = (PUSHORT)pKey1;
    PUSHORT pTid2 = (PUSHORT)pKey2;

    assert (pTid1 != NULL);
    assert (pTid2 != NULL);

    if (*pTid1 > *pTid2)
    {
        return 1;
    }
    else if (*pTid1 < *pTid2)
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
    SMB_LOG_DEBUG("Freeing session [object:0x%x][uid:%u]",
                    pSession,
                    pSession->uid);

    if (pSession->pMutex)
    {
        pthread_rwlock_destroy(&pSession->mutex);
        pSession->pMutex = NULL;
    }

    if (pSession->pTreeCollection)
    {
        LwRtlRBTreeFree(pSession->pTreeCollection);
    }

    if (pSession->hFinderRepository)
    {
        SrvFinderCloseRepository(pSession->hFinderRepository);
    }

    IO_SAFE_FREE_MEMORY(pSession->pszClientPrincipalName);

    if (pSession->pIoSecurityContext) {
        IoSecurityFreeSecurityContext(&pSession->pIoSecurityContext);
    }

    LwRtlMemoryFree(pSession);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
