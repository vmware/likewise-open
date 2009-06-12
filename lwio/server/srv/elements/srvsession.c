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
 *        srvsession.c
 *
 * Abstract:
 *
 *        Likewise IO (LWIO) - SRV
 *
 *        Elements
 *
 *        Session Object
 *
 * Authors: Sriram Nambakam (snambakam@likewise.com)
 */

#include "includes.h"

static
NTSTATUS
SrvSessionAcquireTreeId_inlock(
   PLWIO_SRV_SESSION pSession,
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
    PLWIO_SRV_SESSION pSession
    );

NTSTATUS
SrvSessionCreate(
    USHORT            uid,
    PLWIO_SRV_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_SESSION pSession = NULL;

    LWIO_LOG_DEBUG("Creating session [uid:%u]", uid);

    ntStatus = SrvAllocateMemory(
                    sizeof(LWIO_SRV_SESSION),
                    (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->refcount = 1;

    pthread_rwlock_init(&pSession->mutex, NULL);
    pSession->pMutex = &pSession->mutex;

    pSession->uid = uid;

    LWIO_LOG_DEBUG("Associating session [object:0x%x][uid:%u]", pSession, uid);

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
    PLWIO_SRV_SESSION pSession,
    USHORT           tid,
    PLWIO_SRV_TREE*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PLWIO_SRV_TREE pTree = NULL;

    LWIO_LOCK_RWMUTEX_SHARED(bInLock, &pSession->mutex);

    ntStatus = LwRtlRBTreeFind(
                    pSession->pTreeCollection,
                    &tid,
                    (PVOID*)&pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    InterlockedIncrement(&pTree->refcount);

    *ppTree = pTree;

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *ppTree = NULL;

    goto cleanup;
}

NTSTATUS
SrvSessionRemoveTree(
    PLWIO_SRV_SESSION pSession,
    USHORT           tid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

    ntStatus = LwRtlRBTreeRemove(
                    pSession->pTreeCollection,
                    &tid);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SrvSessionCreateTree(
    PLWIO_SRV_SESSION pSession,
    PSRV_SHARE_INFO   pShareInfo,
    PLWIO_SRV_TREE*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    PLWIO_SRV_TREE pTree = NULL;
    BOOLEAN bInLock = FALSE;
    USHORT  tid = 0;

    LWIO_LOCK_RWMUTEX_EXCLUSIVE(bInLock, &pSession->mutex);

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

    LWIO_UNLOCK_RWMUTEX(bInLock, &pSession->mutex);

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
    IN     PLWIO_SRV_SESSION pSession,
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
    PLWIO_SRV_SESSION pSession
    )
{
    LWIO_LOG_DEBUG("Releasing session [uid:%u]", pSession->uid);

    if (InterlockedDecrement(&pSession->refcount) == 0)
    {
        SrvSessionFree(pSession);
    }
}

static
NTSTATUS
SrvSessionAcquireTreeId_inlock(
   PLWIO_SRV_SESSION pSession,
   PUSHORT          pTid
   )
{
    NTSTATUS ntStatus = 0;
    USHORT   candidateTid = pSession->nextAvailableTid;
    BOOLEAN  bFound = FALSE;

    do
    {
        PLWIO_SRV_TREE pTree = NULL;

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
    SrvTreeRelease((PLWIO_SRV_TREE)pTree);
}

static
VOID
SrvSessionFree(
    PLWIO_SRV_SESSION pSession
    )
{
    LWIO_LOG_DEBUG("Freeing session [object:0x%x][uid:%u]",
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
        IoSecurityDereferenceSecurityContext(&pSession->pIoSecurityContext);
    }

    SrvFreeMemory(pSession);
}


/*
local variables:
mode: c
c-basic-offset: 4
indent-tabs-mode: nil
tab-width: 4
end:
*/
