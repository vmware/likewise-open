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
VOID
SMBSessionFree(
    PSMB_SESSION pSession
    );

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

static
NTSTATUS
RdrTransceiveLogoff(
    PRDR_OP_CONTEXT pContext,
    PSMB_SESSION pSession
    );

NTSTATUS
SMBSessionCreate(
    PSMB_SESSION* ppSession
    )
{
    NTSTATUS status = 0;
    SMB_SESSION *pSession = NULL;
    BOOLEAN bDestroySetupCondition = FALSE;
    BOOLEAN bDestroyMutex = FALSE;

    status = SMBAllocateMemory(
                sizeof(SMB_SESSION),
                (PVOID*)&pSession);
    BAIL_ON_NT_STATUS(status);

    LwListInit(&pSession->StateWaiters);

    pthread_mutex_init(&pSession->mutex, NULL);
    bDestroyMutex = TRUE;

    pSession->refCount = 1;

    status = SMBHashCreate(
                19,
                SMBHashCaselessStringCompare,
                SMBHashCaselessString,
                NULL,
                &pSession->pTreeHashByPath);
    BAIL_ON_NT_STATUS(status);

    status = SMBHashCreate(
                19,
                &SMBSessionHashTreeCompareByTID,
                &SMBSessionHashTreeByTID,
                NULL,
                &pSession->pTreeHashByTID);
    BAIL_ON_NT_STATUS(status);

    bDestroySetupCondition = TRUE;

    /* Pre-allocate resources to send a logoff */
    status = RdrCreateContext(NULL, &pSession->pLogoffContext);
    BAIL_ON_NT_STATUS(status);

    status = RdrAllocateContextPacket(pSession->pLogoffContext, 64 * 1024);
    BAIL_ON_NT_STATUS(status);

    *ppSession = pSession;

cleanup:

    return status;

error:

    if (pSession)
    {
        SMBHashSafeFree(&pSession->pTreeHashByTID);

        SMBHashSafeFree(&pSession->pTreeHashByPath);

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

static
BOOLEAN
RdrLogoffComplete(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_PACKET pPacket = pParam;
    PSMB_SESSION pSession = pContext->State.TreeConnect.pSession;

    if (pPacket)
    {
        SMBPacketRelease(gRdrRuntime.hPacketAllocator, pPacket);
    }

    SMBSessionFree(pSession);

    /* We don't explicitly free pContext because SMBSessionFree() does it */
    return FALSE;
}

static
VOID
RdrSessionTimeout(
    PLW_TASK pTask,
    LW_PVOID _pSession,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSMB_SESSION pSession = _pSession;
    BOOLEAN bLocked = FALSE;
    PRDR_OP_CONTEXT pContext = NULL;

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
        LWIO_LOCK_MUTEX(bLocked, &pSession->pSocket->mutex);

        if (pSession->refCount == 0)
        {
            RdrSessionUnlink(pSession);

            pContext = pSession->pLogoffContext;
            pContext->Continue = RdrLogoffComplete;
            pContext->State.TreeConnect.pSession = pSession;

            LWIO_UNLOCK_MUTEX(bLocked, &pSession->pSocket->mutex);

            status = RdrTransceiveLogoff(pContext, pSession);
            if (status != STATUS_PENDING)
            {
                /* Give up and free the session now */
                SMBSessionFree(pSession);
            }

            *pWaitMask = LW_TASK_EVENT_COMPLETE;
        }
        else
        {
            *pWaitMask = LW_TASK_EVENT_TIME;
            *pllTime = RDR_IDLE_TIMEOUT * 1000000000ll;
        }
    }

    LWIO_UNLOCK_MUTEX(bLocked, &pSession->pSocket->mutex);

    return;
}

VOID
SMBSessionRelease(
    PSMB_SESSION pSession
    )
{
    BOOLEAN bInLock = FALSE;
    LW_TASK_EVENT_MASK dummy = 0;
    LONG64 llDummy = 0;

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

            LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);

            if (LwRtlCreateTask(
                    gRdrRuntime.pThreadPool,
                    &pSession->pTimeout,
                    gRdrRuntime.pReaderTaskGroup,
                    RdrSessionTimeout,
                    pSession) == STATUS_SUCCESS)
            {
                LwRtlWakeTask(pSession->pTimeout);
            }
            else
            {
                LWIO_LOG_ERROR("Could not create timer for session %p; logging off immediately");
                RdrSessionTimeout(NULL, pSession, LW_TASK_EVENT_TIME, &dummy, &llDummy);
            }
        }
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &pSession->pSocket->mutex);
    }
}

static
VOID
SMBSessionFree(
    PSMB_SESSION pSession
    )
{
    assert(!pSession->refCount);

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

    if (pSession->pLogoffContext)
    {
        RdrFreeContext(pSession->pLogoffContext);
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
    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);
}

NTSTATUS
SMBSrvClientSessionCreate(
    IN OUT PSMB_SOCKET* ppSocket,
    IN PIO_CREDS pCreds,
    uid_t uid,
    OUT PSMB_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SESSION pSession = NULL;
    BOOLEAN bInLock = FALSE;
    PSMB_SOCKET pSocket = *ppSocket;
    struct _RDR_SESSION_KEY key = {0};

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    switch (pCreds->type)
    {
    case IO_CREDS_TYPE_KRB5_TGT:
        ntStatus = LwRtlCStringAllocateFromWC16String(
            &key.pszPrincipal,
            pCreds->payload.krb5Tgt.pwszClientPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    case IO_CREDS_TYPE_PLAIN:
        ntStatus = LwRtlCStringAllocateFromWC16String(
            &key.pszPrincipal,
            pCreds->payload.krb5Tgt.pwszClientPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    default:
        ntStatus = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(ntStatus);
        break;
    }

    key.uid = uid;

    ntStatus = SMBHashGetValue(
        pSocket->pSessionHashByPrincipal,
        &key,
        OUT_PPVOID(&pSession));

    if (!ntStatus)
    {
        pSession->refCount++;
        RdrSessionRevive(pSession);
        SMBSocketRelease(pSocket);
        *ppSocket = NULL;
    }
    else
    {
        ntStatus = SMBSessionCreate(&pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        pSession->pSocket = pSocket;

        ntStatus = SMBStrndup(
            key.pszPrincipal,
            strlen(key.pszPrincipal) + 1,
            &pSession->key.pszPrincipal);
        BAIL_ON_NT_STATUS(ntStatus);

        pSession->key.uid = key.uid;

        ntStatus = SMBHashSetValue(
            pSocket->pSessionHashByPrincipal,
            &pSession->key,
            pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        pSession->bParentLink = TRUE;

        *ppSocket = NULL;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    *ppSession = pSession;

cleanup:

    LWIO_SAFE_FREE_STRING(key.pszPrincipal);

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    if (pSession)
    {
        SMBSessionRelease(pSession);
    }

    *ppSession = NULL;

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveLogoff(
    PRDR_OP_CONTEXT pContext,
    PSMB_SESSION pSession
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    status = SMBPacketMarshallHeader(
                pContext->Packet.pRawBuffer,
                pContext->Packet.bufferLen,
                COM_LOGOFF_ANDX,
                0,
                0,
                0,
                gRdrRuntime.SysPid,
                pSession->uid,
                0,
                TRUE,
                &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pContext->Packet.pSMBHeader->wordCount = 2;

    pContext->Packet.pData = pContext->Packet.pParams;
    pContext->Packet.bufferUsed += sizeof(uint16_t); /* ByteCount */
    memset(pContext->Packet.pData, 0, sizeof(uint16_t));

    // no byte order conversions necessary (due to zeros)

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:
    goto cleanup;
}
