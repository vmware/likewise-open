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
 *        session.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Common Session Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

static
int
SMBSessionHashTreeCompareByTID(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
SMBSessionHashTreeByTID(
    PCVOID vp
    );

NTSTATUS
SMBSessionCreate(
    PSMB_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    SMB_SESSION *pSession = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroySetupCondition = FALSE;
    BOOLEAN bDestroyMutex = FALSE;

    ntStatus = SMBAllocateMemory(
                sizeof(SMB_SESSION),
                (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pthread_mutex_init(&pSession->mutex, NULL);
    bDestroyMutex = TRUE;

    ntStatus = pthread_cond_init(&pSession->event, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    bDestroyCondition = TRUE;

    pSession->refCount = 1;

    ntStatus = SMBHashCreate(
                19,
                SMBHashCaselessStringCompare,
                SMBHashCaselessString,
                NULL,
                &pSession->pTreeHashByPath);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashCreate(
                19,
                &SMBSessionHashTreeCompareByTID,
                &SMBSessionHashTreeByTID,
                NULL,
                &pSession->pTreeHashByTID);
    BAIL_ON_NT_STATUS(ntStatus);

    bDestroySetupCondition = TRUE;

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:

    if (pSession)
    {
        SMBHashSafeFree(&pSession->pTreeHashByTID);

        SMBHashSafeFree(&pSession->pTreeHashByPath);

        if (bDestroyCondition)
        {
            pthread_cond_destroy(&pSession->event);
        }

        if (bDestroyMutex)
        {
            pthread_mutex_destroy(&pSession->mutex);
        }

        SMBFreeMemory(pSession);
    }

    *ppSession = NULL;

    goto cleanup;
}

static
int
SMBSessionHashTreeCompareByTID(
    PCVOID vp1,
    PCVOID vp2
    )
{
    uint16_t tid1 = *((uint16_t *) vp1);
    uint16_t tid2 = *((uint16_t *) vp2);

    if (tid1 == tid2)
    {
        return 0;
    }
    else if (tid1 > tid2)
    {
        return 1;
    }

    return -1;
}

static
size_t
SMBSessionHashTreeByTID(
    PCVOID vp
    )
{
    return *((uint16_t *) vp);
}

static
VOID
RdrSessionUnlink(
    PSMB_SESSION pSession
    )
{
    if (pSession->bParentLink)
    {
        SMBHashRemoveKey(
            pSession->pSocket->pSessionHashByPrincipal,
            &pSession->key);
        SMBHashRemoveKey(
            pSession->pSocket->pSessionHashByUID,
            &pSession->uid);
        pSession->bParentLink = FALSE;
    }
}

VOID
RdrSessionRevive(
    PSMB_SESSION pSession
    )
{
    if (pSession->pTimeout)
    {
        LwRtlCancelTask(pSession->pTimeout);
        LwRtlReleaseTask(&pSession->pTimeout);
    }
}

VOID
SMBSessionAddReference(
    PSMB_SESSION pSession
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->pSocket->mutex);

    pSession->refCount++;
    RdrSessionRevive(pSession);

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);
}

static
VOID
RdrSessionReap(
    PVOID pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    BOOLEAN bLocked = FALSE;
    PSMB_SESSION pSession = pContext;
    PSMB_SOCKET pSocket = pSession->pSocket;

    LWIO_LOCK_MUTEX(bLocked, &pSocket->mutex);

    if (pSession->refCount == 0)
    {
        RdrSessionUnlink(pSession);
        LWIO_UNLOCK_MUTEX(bLocked, &pSocket->mutex);
        status = Logoff(pSession);
        SMBSessionFree(pSession);
        BAIL_ON_NT_STATUS(status);
    }

error:

    LWIO_UNLOCK_MUTEX(bLocked, &pSocket->mutex);
}

static
VOID
RdrSessionTimeout(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    PSMB_SESSION pSession = pContext;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }
    else if (WakeMask & LW_TASK_EVENT_INIT)
    {
        *pWaitMask = LW_TASK_EVENT_TIME;
        *pllTime = RDR_IDLE_TIMEOUT * 1000000000ll;
    }
    else if (WakeMask & LW_TASK_EVENT_TIME)
    {
        if (LwRtlQueueWorkItem(gRdrRuntime.pThreadPool, RdrSessionReap, pSession, 0) == STATUS_SUCCESS)
        {
            *pWaitMask = LW_TASK_EVENT_COMPLETE;
        }
        else
        {
            *pWaitMask = LW_TASK_EVENT_TIME;
            *pllTime = RDR_IDLE_TIMEOUT * 1000000000ll;
        }
    }
}

VOID
SMBSessionRelease(
    PSMB_SESSION pSession
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->pSocket->mutex);

    assert(pSession->refCount > 0);

    if (--pSession->refCount == 0)
    {
        if (pSession->state != RDR_SESSION_STATE_READY)
        {
            RdrSessionUnlink(pSession);
            LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);
            SMBSessionFree(pSession);
        }
        else
        {
            LWIO_LOG_VERBOSE("Session %p is eligible for reaping", pSession);
            if (LwRtlCreateTask(
                    gRdrRuntime.pThreadPool,
                    &pSession->pTimeout,
                    gRdrRuntime.pReaderTaskGroup,
                    RdrSessionTimeout,
                    pSession) == STATUS_SUCCESS)
            {
                LwRtlWakeTask(pSession->pTimeout);
                LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);
            }
            else
            {
                RdrSessionUnlink(pSession);
                LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);
                SMBSessionFree(pSession);
            }
        }
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);
    }
}

VOID
SMBSessionFree(
    PSMB_SESSION pSession
    )
{
    assert(!pSession->refCount);

    pthread_cond_destroy(&pSession->event);

    SMBHashSafeFree(&pSession->pTreeHashByPath);
    SMBHashSafeFree(&pSession->pTreeHashByTID);

    pthread_mutex_destroy(&pSession->mutex);

    LWIO_SAFE_FREE_MEMORY(pSession->pSessionKey);
    LWIO_SAFE_FREE_MEMORY(pSession->key.pszPrincipal);

    if (pSession->pTimeout)
    {
        LwRtlCancelTask(pSession->pTimeout);
        LwRtlReleaseTask(&pSession->pTimeout);
    }

    if (pSession->pSocket)
    {
        SMBSocketRelease(pSession->pSocket);
    }

    /* @todo: use allocator */
    SMBFreeMemory(pSession);
}

VOID
SMBSessionInvalidate(
    PSMB_SESSION   pSession,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bInSocketLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    pSession->state = RDR_SESSION_STATE_ERROR;
    pSession->error = ntStatus;

    LWIO_LOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
    if (pSession->bParentLink)
    {
        SMBHashRemoveKey(
            pSession->pSocket->pSessionHashByPrincipal,
            &pSession->key);
        SMBHashRemoveKey(
            pSession->pSocket->pSessionHashByUID,
            &pSession->uid);
        pSession->bParentLink = FALSE;
    }
    LWIO_UNLOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);

    pthread_cond_broadcast(&pSession->event);

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);
}

VOID
SMBSessionSetState(
    PSMB_SESSION pSession,
    SMB_RESOURCE_STATE state
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    pSession->state = state;

    pthread_cond_broadcast(&pSession->event);

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);
}

NTSTATUS
SMBSessionFindTreeByPath(
    IN PSMB_SESSION pSession,
    IN PCSTR pszPath,
    OUT PSMB_TREE* ppTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_TREE pTree = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    ntStatus = SMBHashGetValue(
                pSession->pTreeHashByPath,
                pszPath,
                (PVOID *) &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBTreeAddReference(pTree);

    *ppTree = pTree;

cleanup:

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *ppTree = NULL;

    goto cleanup;
}

NTSTATUS
SMBSessionFindTreeById(
    PSMB_SESSION pSession,
    uint16_t     tid,
    PSMB_TREE*   ppTree
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_TREE pTree = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    ntStatus = SMBHashGetValue(
                    pSession->pTreeHashByTID,
                    &tid,
                    (PVOID *) &pTree);
    BAIL_ON_NT_STATUS(ntStatus);

    pTree->refCount++;

    *ppTree = pTree;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    return ntStatus;

error:

    *ppTree = NULL;

    goto cleanup;
}

NTSTATUS
SMBSessionWaitReady(
    PSMB_SESSION pSession
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    while (pSession->state < RDR_SESSION_STATE_READY)
    {
        if (pSession->state == RDR_SESSION_STATE_ERROR)
        {
            ntStatus = pSession->error;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pthread_cond_wait(&pSession->event, &pSession->mutex);
    }

error:

    return ntStatus;
}
