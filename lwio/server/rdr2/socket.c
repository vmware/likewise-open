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
 *        socket.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem (LWIO)
 *
 *        Common Socket Code
 *
 * Author: Kaya Bekiroglu (kaya@likewisesoftware.com)
 *
 * @todo: add error logging code
 * @todo: switch to NT error codes where appropriate
 */

#include "rdr.h"

static
int
SMBSocketHashSessionCompareByUID(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
SMBSocketHashSessionByUID(
    PCVOID vp
    );

static
int
SMBSocketHashSessionCompareByKey(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
SMBSocketHashSessionByKey(
    PCVOID vp
    );

static
NTSTATUS
SMBSocketSendData(
    IN PSMB_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    );

static
VOID
SMBSocketTask(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    );

static
NTSTATUS
SMBSocketFindAndSignalResponse(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    );

static
NTSTATUS
RdrEaiToNtStatus(
    int eai
    );

static
int
RdrSocketHashResponseCompare(
    PCVOID vp1,
    PCVOID vp2)
{
    uint16_t mid1 = *((uint16_t *) vp1);
    uint16_t mid2 = *((uint16_t *) vp2);

    if (mid1 == mid2)
    {
        return 0;
    }
    else if (mid1 > mid2)
    {
        return 1;
    }

    return -1;
}

static
size_t
RdrSocketHashResponse(
    PCVOID vp)
{
    return *((uint16_t *) vp);
}

NTSTATUS
SMBSocketCreate(
    IN PCWSTR pwszHostname,
    IN BOOLEAN bUseSignedMessagesIfSupported,
    OUT PSMB_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;
    SMB_SOCKET *pSocket = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroyMutex = FALSE;
    PWSTR pwszCanonicalName = NULL;
    PWSTR pwszCursor = NULL;

    ntStatus = SMBAllocateMemory(
                sizeof(SMB_SOCKET),
                (PVOID*)&pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pSocket->PendingSend);

    pSocket->bUseSignedMessagesIfSupported = bUseSignedMessagesIfSupported;

    pthread_mutex_init(&pSocket->mutex, NULL);
    bDestroyMutex = TRUE;

    ntStatus = pthread_cond_init(&pSocket->event, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    bDestroyCondition = TRUE;

    pSocket->refCount = 1;
    pSocket->fd = -1;

    /* Hostname is trusted */
    ntStatus = LwRtlWC16StringDuplicate(&pSocket->pwszHostname, pwszHostname);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Construct canonical name by removing channel specifier */
    ntStatus = LwRtlWC16StringDuplicate(&pwszCanonicalName, pwszHostname);
    BAIL_ON_NT_STATUS(ntStatus);

    for (pwszCursor = pwszCanonicalName; *pwszCursor; pwszCursor++)
    {
        if (*pwszCursor == '@')
        {
            *pwszCursor = '\0';
            break;
        }
    }

    pSocket->pwszCanonicalName = pwszCanonicalName;

    pSocket->maxBufferSize = 0;
    pSocket->maxRawSize = 0;
    pSocket->sessionKey = 0;
    pSocket->capabilities = 0;
    pSocket->pSecurityBlob = NULL;
    pSocket->securityBlobLen = 0;

    pSocket->hPacketAllocator = gRdrRuntime.hPacketAllocator;

    ntStatus = SMBHashCreate(
                    19,
                    SMBSocketHashSessionCompareByKey,
                    SMBSocketHashSessionByKey,
                    NULL,
                    &pSocket->pSessionHashByPrincipal);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashCreate(
                    19,
                    SMBSocketHashSessionCompareByUID,
                    SMBSocketHashSessionByUID,
                    NULL,
                    &pSocket->pSessionHashByUID);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashCreate(
                19,
                &RdrSocketHashResponseCompare,
                &RdrSocketHashResponse,
                NULL,
                &pSocket->pResponseHash);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTask(
        gRdrRuntime.pThreadPool,
        &pSocket->pTask,
        gRdrRuntime.pReaderTaskGroup,
        SMBSocketTask,
        pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppSocket = pSocket;

cleanup:

    return ntStatus;

error:

    if (pSocket)
    {
        SMBHashSafeFree(&pSocket->pSessionHashByUID);
        SMBHashSafeFree(&pSocket->pSessionHashByPrincipal);
        SMBHashSafeFree(&pSocket->pResponseHash);

        LWIO_SAFE_FREE_MEMORY(pSocket->pwszHostname);
        LWIO_SAFE_FREE_MEMORY(pSocket->pwszCanonicalName);

        if (bDestroyCondition)
        {
            pthread_cond_destroy(&pSocket->event);
        }

        if (bDestroyMutex)
        {
            pthread_mutex_destroy(&pSocket->mutex);
        }

        SMBFreeMemory(pSocket);
    }

    *ppSocket = NULL;

    goto cleanup;
}

static
int
SMBSocketHashSessionCompareByUID(
    PCVOID vp1,
    PCVOID vp2
    )
{
    uint16_t uid1 = *((uint16_t *) vp1);
    uint16_t uid2 = *((uint16_t *) vp2);

    if (uid1 == uid2)
    {
        return 0;
    }
    else if (uid1 > uid2)
    {
        return 1;
    }

    return -1;
}

static
size_t
SMBSocketHashSessionByUID(
    PCVOID vp
    )
{
   return *((uint16_t *) vp);
}

static
int
SMBSocketHashSessionCompareByKey(
    PCVOID vp1,
    PCVOID vp2
    )
{
    const struct _RDR_SESSION_KEY* pKey1 = (struct _RDR_SESSION_KEY*) vp1;
    const struct _RDR_SESSION_KEY* pKey2 = (struct _RDR_SESSION_KEY*) vp2;

    return !(pKey1->uid == pKey2->uid &&
             !strcmp(pKey1->pszPrincipal, pKey2->pszPrincipal));
}

static
size_t
SMBSocketHashSessionByKey(
    PCVOID vp
    )
{
    const struct _RDR_SESSION_KEY* pKey = (struct _RDR_SESSION_KEY*) vp;

    return SMBHashCaselessString(pKey->pszPrincipal) ^ (pKey->uid ^ (pKey->uid << 16));
}

static
BOOLEAN
SMBSocketIsSignatureRequired(
    IN PSMB_SOCKET pSocket
    )
{
    BOOLEAN bIsRequired = FALSE;

    /* We need to sign outgoing packets if we negotiated it and the
       socket is in the ready state -- that is, we have completed the
       negotiate stage.  This causes the initial session setup packet to
       be signed, even though we do not yet have a session key.  This results
       in signing with a zero-length key, which matches Windows behavior.
       Note that no known SMB server actually verifies this signature since it
       is meaningless and probably the result of an implementation quirk. */
    if (pSocket->state == RDR_SOCKET_STATE_READY &&
        (pSocket->bSignedMessagesRequired ||
         (pSocket->bSignedMessagesSupported && pSocket->bUseSignedMessagesIfSupported)))
    {
        bIsRequired = TRUE;
    }

    return bIsRequired;
}

static
ULONG
SMBSocketGetNextSequence_inlock(
    PSMB_SOCKET pSocket
    )
{
    DWORD dwSequence = 0;

    dwSequence = pSocket->dwSequence;
    // Next for response
    // Next for next message
    pSocket->dwSequence += 2;

    return dwSequence;
}

ULONG
SMBSocketGetNextSequence(
    PSMB_SOCKET pSocket
    )
{
    DWORD dwSequence = 0;

    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    dwSequence = SMBSocketGetNextSequence_inlock(pSocket);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return dwSequence;
}

VOID
SMBSocketBeginSequence(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->dwSequence = 2;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

static
NTSTATUS
SMBSocketSendData(
    IN PSMB_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t  writtenLen = 0;

    do
    {
        writtenLen = write(
            pSocket->fd,
            pPacket->pRawBuffer + pSocket->OutgoingWritten,
            pPacket->bufferUsed - pSocket->OutgoingWritten);
        if (writtenLen >= 0)
        {
            pSocket->OutgoingWritten += writtenLen;
        }
    } while (pSocket->OutgoingWritten < pPacket->bufferUsed &&
             (writtenLen >= 0 || (writtenLen < 0 && errno == EINTR)));

    if (writtenLen < 0)
    {
        switch (errno)
        {
        case EAGAIN:
            ntStatus = STATUS_PENDING;
            BAIL_ON_NT_STATUS(ntStatus);
        default:
            ntStatus = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

error:

    return ntStatus;
}

static
NTSTATUS
SMBSocketPrepareSend(
    IN PSMB_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bIsSignatureRequired = FALSE;

    if (pPacket->allowSignature)
    {
        bIsSignatureRequired = SMBSocketIsSignatureRequired(pSocket);
    }

    if (bIsSignatureRequired)
    {
        pPacket->pSMBHeader->flags2 |= FLAG2_SECURITY_SIG;
    }

    SMBPacketHTOLSmbHeader(pPacket->pSMBHeader);

    pPacket->haveSignature = bIsSignatureRequired;

    if (pPacket->pSMBHeader->command != COM_NEGOTIATE)
    {
        pPacket->sequence = SMBSocketGetNextSequence_inlock(pSocket);
    }

    if (bIsSignatureRequired)
    {
        ntStatus = SMBPacketSign(
                        pPacket,
                        pPacket->sequence,
                        pSocket->pSessionKey,
                        pSocket->dwSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSocket->pOutgoing = pPacket;

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSocketSend(
    IN PSMB_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* Wait until packet queue is empty */
    while (pSocket->pOutgoing || !LwListIsEmpty(&pSocket->PendingSend))
    {
        if (pSocket->error)
        {
            ntStatus = pSocket->error;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pthread_cond_wait(&pSocket->event, &pSocket->mutex);
    }

    ntStatus = SMBSocketPrepareSend(pSocket, pPacket);
    BAIL_ON_NT_STATUS(ntStatus);

    LwRtlWakeTask(pSocket->pTask);

   /* Wait until our packet is fully sent */
    while (pSocket->pOutgoing == pPacket)
    {
        if (pSocket->error)
        {
            ntStatus = pSocket->error;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pthread_cond_wait(&pSocket->event, &pSocket->mutex);
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
SMBSocketQueue(
    IN PSMB_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    LwListInsertBefore(&pSocket->PendingSend, &pContext->Link);
    LwRtlWakeTask(pSocket->pTask);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;
}

NTSTATUS
RdrSocketTransceive(
    IN PSMB_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PSMB_RESPONSE pResponse = NULL;
    PSMB_PACKET pPacket = &pContext->Packet;
    USHORT usMid = 0;

    status = RdrSocketAcquireMid(pSocket, &usMid);
    BAIL_ON_NT_STATUS(status);

    pPacket->pSMBHeader->mid = usMid;

    status = SMBResponseCreate(usMid, &pResponse);
    BAIL_ON_NT_STATUS(status);

    pResponse->pContext = pContext;

    status = RdrSocketAddResponse(pSocket, pResponse);
    BAIL_ON_NT_STATUS(status);

    status = SMBSocketQueue(pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

    status = STATUS_PENDING;

cleanup:

    return status;

error:

    if (pResponse)
    {
        SMBResponseFree(pResponse);
    }

    goto cleanup;
}

NTSTATUS
SMBSocketReceiveAndUnmarshall(
    IN PSMB_SOCKET pSocket,
    OUT PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    uint32_t readLen = 0;

    if (pPacket->bufferUsed < sizeof(NETBIOS_HEADER))
    {
        while (pPacket->bufferUsed < sizeof(NETBIOS_HEADER))
        {
            ntStatus = SMBSocketRead(
                pSocket,
                pPacket->pRawBuffer + pPacket->bufferUsed,
                sizeof(NETBIOS_HEADER) - pPacket->bufferUsed,
                &readLen);
            BAIL_ON_NT_STATUS(ntStatus);

            if (readLen == 0)
            {
                ntStatus = STATUS_END_OF_FILE;
                BAIL_ON_NT_STATUS(ntStatus);
            }

            pPacket->bufferUsed += readLen;
        }

        pPacket->pNetBIOSHeader = (NETBIOS_HEADER *) pPacket->pRawBuffer;
        pPacket->pNetBIOSHeader->len = htonl(pPacket->pNetBIOSHeader->len);

        if ((uint64_t) pPacket->pNetBIOSHeader->len + sizeof(NETBIOS_HEADER) > (uint64_t) pPacket->bufferLen)
        {
            ntStatus = STATUS_INVALID_NETWORK_RESPONSE;
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    while (pPacket->bufferUsed < pPacket->pNetBIOSHeader->len + sizeof(NETBIOS_HEADER))
    {
        ntStatus = SMBSocketRead(
            pSocket,
            pPacket->pRawBuffer + pPacket->bufferUsed,
            pPacket->pNetBIOSHeader->len + sizeof(NETBIOS_HEADER) - pPacket->bufferUsed,
            &readLen);
        BAIL_ON_NT_STATUS(ntStatus);

        if (readLen == 0)
        {
            ntStatus = STATUS_END_OF_FILE;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pPacket->bufferUsed += readLen;
    }

    pPacket->pSMBHeader = (SMB_HEADER *) (pPacket->pRawBuffer + sizeof(NETBIOS_HEADER));

    if (SMBIsAndXCommand(SMB_LTOH8(pPacket->pSMBHeader->command)))
    {
        pPacket->pAndXHeader = (ANDX_HEADER *)
            (pPacket->pRawBuffer + sizeof(SMB_HEADER) + sizeof(NETBIOS_HEADER));
    }

    pPacket->pParams = pPacket->pAndXHeader ?
        (PBYTE) pPacket->pAndXHeader + sizeof(ANDX_HEADER) :
        (PBYTE) pPacket->pSMBHeader + sizeof(SMB_HEADER);
    pPacket->pData = NULL;

error:

    return ntStatus;
}

static
VOID
SMBSocketTask(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_SOCKET pSocket = (PSMB_SOCKET) pContext;
    BOOLEAN bInLock = FALSE;
    int err = 0;
    SOCKLEN_T len = sizeof(err);
    static const LONG64 llConnectTimeout = 10 * 1000000000ll; // 10 sec
    PLW_LIST_LINKS pLink = NULL;
    PRDR_OP_CONTEXT pIrpContext = NULL;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        LwRtlSetTaskFd(pTask, pSocket->fd, LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_FD_WRITABLE);
    }

    if (pSocket->state < RDR_SOCKET_STATE_NEGOTIATING)
    {
        /* If the socket is writable, we can safely call getsockopt() */
        if (WakeMask & LW_TASK_EVENT_FD_WRITABLE)
        {
            /* Get result of connect() */
            if (getsockopt(pSocket->fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0)
            {
                ntStatus = LwErrnoToNtStatus(errno);
                BAIL_ON_NT_STATUS(ntStatus);
            }
        }
        else
        {
            /* The connect() is still in progress */
            err = EINPROGRESS;
        }

        switch (err)
        {
        case 0:
            /* Notify thread initiating connect that it can now move on to negotiate */
            pSocket->state = RDR_SOCKET_STATE_NEGOTIATING;
            pthread_cond_broadcast(&pSocket->event);
            break;
        case EINPROGRESS:
            if (WakeMask & LW_TASK_EVENT_TIME)
            {
                /* We timed out, give up */
                ntStatus = STATUS_TIMEOUT;
                BAIL_ON_NT_STATUS(ntStatus);
            }
            else
            {
                *pWaitMask = LW_TASK_EVENT_FD_WRITABLE | LW_TASK_EVENT_TIME;
                *pllTime = llConnectTimeout;
                goto cleanup;
            }
        default:
            ntStatus = LwErrnoToNtStatus(err);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (WakeMask & LW_TASK_EVENT_FD_READABLE)
    {
        pSocket->bReadBlocked = FALSE;
    }

    if (WakeMask & LW_TASK_EVENT_FD_WRITABLE)
    {
        pSocket->bWriteBlocked = FALSE;
    }

    /* FIXME: max mpx count */
    if (!pSocket->pOutgoing && !LwListIsEmpty(&pSocket->PendingSend))
    {
        pLink = LwListRemoveHead(&pSocket->PendingSend);
        pIrpContext = LW_STRUCT_FROM_FIELD(pLink, RDR_OP_CONTEXT, Link);
        ntStatus = SMBSocketPrepareSend(pSocket, &pIrpContext->Packet);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    *pWaitMask = LW_TASK_EVENT_EXPLICIT;

    if (!pSocket->bReadBlocked)
    {
        if (!pSocket->pPacket)
        {
            ntStatus = SMBPacketAllocate(pSocket->hPacketAllocator, &pSocket->pPacket);
            BAIL_ON_NT_STATUS(ntStatus);

            ntStatus = SMBPacketBufferAllocate(
                pSocket->hPacketAllocator,
                1024*64,
                &pSocket->pPacket->pRawBuffer,
                &pSocket->pPacket->bufferLen);
            BAIL_ON_NT_STATUS(ntStatus);
        }

        ntStatus = SMBSocketReceiveAndUnmarshall(pSocket, pSocket->pPacket);
        switch(ntStatus)
        {
        case STATUS_SUCCESS:
            /* This function should free packet and socket memory on error */
            ntStatus = SMBSocketFindAndSignalResponse(pSocket, pSocket->pPacket);
            BAIL_ON_NT_STATUS(ntStatus);

            pSocket->pPacket = NULL;

            *pWaitMask |= LW_TASK_EVENT_YIELD;
            break;
        case STATUS_PENDING:
            pSocket->bReadBlocked = TRUE;
            break;
        default:
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (!pSocket->bWriteBlocked && pSocket->pOutgoing)
    {
        ntStatus = SMBSocketSendData(pSocket, pSocket->pOutgoing);
        switch (ntStatus)
        {
        case STATUS_SUCCESS:
            pSocket->OutgoingWritten = 0;
            pSocket->pOutgoing = NULL;
            pthread_cond_broadcast(&pSocket->event);
            *pWaitMask |= LW_TASK_EVENT_YIELD;
            break;
        case STATUS_PENDING:
            pSocket->bWriteBlocked = TRUE;
            break;
        default:
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    /* Determine next wake mask */
    if (pSocket->bReadBlocked)
    {
        *pWaitMask |= LW_TASK_EVENT_FD_READABLE;
    }

    if (pSocket->bWriteBlocked && pSocket->pOutgoing)
    {
        *pWaitMask |= LW_TASK_EVENT_FD_WRITABLE;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return;

error:

    if (ntStatus != STATUS_PENDING)
    {
        LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);
        SMBSocketInvalidate_InLock(pSocket, ntStatus);
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }

    goto cleanup;
}

static
NTSTATUS
RdrSocketFindResponseByMid(
    PSMB_SOCKET pSocket,
    USHORT usMid,
    PSMB_RESPONSE* ppResponse
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RESPONSE pResponse = NULL;

    ntStatus = SMBHashGetValue(
        pSocket->pResponseHash,
        &usMid,
        (PVOID *) &pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppResponse = pResponse;

cleanup:

    return ntStatus;

error:

    *ppResponse = NULL;

    goto cleanup;
}

static
NTSTATUS
SMBSocketFindAndSignalResponse(
    PSMB_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    PSMB_RESPONSE pResponse = NULL;
    USHORT usMid = SMB_LTOH16(pPacket->pSMBHeader->mid);
    BOOLEAN bLocked = TRUE;
    BOOLEAN bKeep = FALSE;

    ntStatus = RdrSocketFindResponseByMid(
                    pSocket,
                    usMid,
                    &pResponse);
    switch(ntStatus)
    {
    case STATUS_SUCCESS:
        break;
    case STATUS_NOT_FOUND:
        SMBPacketRelease(pSocket->hPacketAllocator, pPacket);
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    default:
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pResponse->pPacket = pPacket;
    pResponse->state = SMB_RESOURCE_STATE_VALID;

    if (pResponse->pContext)
    {
        ntStatus = SMBPacketDecodeHeader(
            pPacket,
            (pResponse->pContext->Packet.haveSignature &&
             !pSocket->bIgnoreServerSignatures &&
             pSocket->pSessionKey != NULL),
            pResponse->pContext->Packet.sequence + 1,
            pSocket->pSessionKey,
            pSocket->dwSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_MUTEX(bLocked, &pSocket->mutex);
        bKeep = RdrContinueContext(pResponse->pContext, STATUS_SUCCESS, pPacket);
        LWIO_LOCK_MUTEX(bLocked, &pSocket->mutex);

        if (!bKeep)
        {
            ntStatus = SMBHashRemoveKey(
                pSocket->pResponseHash,
                &pResponse->mid);
            BAIL_ON_NT_STATUS(ntStatus);

            pResponse->pSocket = NULL;
            SMBResponseFree(pResponse);
        }
    }
    else
    {
        pthread_cond_signal(&pResponse->event);
    }

cleanup:

    return ntStatus;

error:

    goto cleanup;
}

VOID
RdrSocketCancel(
    IN PSMB_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    )
{
    /* Currently a no-op */
}

VOID
RdrSocketRevive(
    PSMB_SOCKET pSocket
    )
{
    if (pSocket->pTimeout)
    {
        LwRtlCancelTask(pSocket->pTimeout);
        LwRtlReleaseTask(&pSocket->pTimeout);
    }
}

VOID
SMBSocketAddReference(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);

    pSocket->refCount++;
    RdrSocketRevive(pSocket);

    LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
}

static
VOID
RdrSocketUnlink(
    PSMB_SOCKET pSocket
    )
{
    if (pSocket->bParentLink)
    {
        SMBHashRemoveKey(gRdrRuntime.pSocketHashByName,
                         pSocket->pwszHostname);
        pSocket->bParentLink = FALSE;
    }
}

static
VOID
RdrSocketReap(
    PVOID pContext
    )
{
    BOOLEAN bLocked = FALSE;
    PSMB_SOCKET pSocket = pContext;

    LWIO_LOCK_MUTEX(bLocked, &gRdrRuntime.socketHashLock);

    if (pSocket->refCount == 0)
    {
        RdrSocketUnlink(pSocket);
        LWIO_UNLOCK_MUTEX(bLocked, &gRdrRuntime.socketHashLock);
        SMBSocketFree(pSocket);
    }

    LWIO_UNLOCK_MUTEX(bLocked, &gRdrRuntime.socketHashLock);
}

static
VOID
RdrSocketTimeout(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    PSMB_SOCKET pSocket = pContext;

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
        if (LwRtlQueueWorkItem(gRdrRuntime.pThreadPool, RdrSocketReap, pSocket, 0) == STATUS_SUCCESS)
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

NTSTATUS
SMBSocketConnect(
    PSMB_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = 0;
    int fd = -1;
    BOOLEAN bInLock = FALSE;
    struct addrinfo *ai = NULL;
    struct addrinfo *pCursor = NULL;
    struct addrinfo hints;
    PSTR pszHostname = NULL;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    ntStatus = LwRtlCStringAllocateFromWC16String(&pszHostname, pSocket->pwszCanonicalName);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = RdrEaiToNtStatus(
        getaddrinfo(pszHostname, "445", &hints, &ai));
    BAIL_ON_NT_STATUS(ntStatus);

    for (pCursor = ai; pCursor; pCursor = pCursor->ai_next)
    {
        fd = socket(pCursor->ai_family, pCursor->ai_socktype, pCursor->ai_protocol);

        if (fd < 0)
        {
#ifdef EPROTONOSUPPORT
            if (errno == EPROTONOSUPPORT)
            {
                continue;
            }
#endif
            ntStatus = ErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            break;
        }
    }

    if (fd < 0)
    {
        ntStatus = STATUS_BAD_NETWORK_NAME;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        ntStatus = ErrnoToNtStatus(errno);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    if (connect(fd, ai->ai_addr, ai->ai_addrlen) && errno != EINPROGRESS)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->fd = fd;
    fd = -1;
    memcpy(&pSocket->address, &ai->ai_addr, ai->ai_addrlen);

    /* Let the task wait for the connect() to complete before proceeding */
    LwRtlWakeTask(pSocket->pTask);

    while (pSocket->state < RDR_SOCKET_STATE_NEGOTIATING)
    {
        pthread_cond_wait(&pSocket->event, &pSocket->mutex);
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

cleanup:

    if (ai)
    {
        freeaddrinfo(ai);
    }

    LWIO_SAFE_FREE_MEMORY(pszHostname);

    return ntStatus;

error:

    if (fd >= 0)
    {
        close(fd);
    }

    SMBSocketInvalidate(pSocket, ntStatus);

    goto cleanup;
}

NTSTATUS
SMBSocketRead(
    PSMB_SOCKET pSocket,
    uint8_t *buffer,
    uint32_t len,
    uint32_t *actualLen
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t totalRead = 0;
    ssize_t nRead = 0;

    while (totalRead < len)
    {
        nRead = read(pSocket->fd, buffer + totalRead, len - totalRead);
        if(nRead < 0)
        {
            switch (errno)
            {
            case EINTR:
                continue;
            case EAGAIN:
                if (totalRead == 0)
                {
                    ntStatus = STATUS_PENDING;
                }
                else
                {
                    /* Return a successful partial read */
                    goto cleanup;
                }
                break;
            default:
                ntStatus = ErrnoToNtStatus(errno);
            }

            BAIL_ON_NT_STATUS(ntStatus);
        }
        else if (nRead == 0)
        {
            /* EOF */
            goto cleanup;
        }

        totalRead += nRead;
    }

cleanup:

    *actualLen = totalRead;

    return ntStatus;

error:

    if (ntStatus != STATUS_PENDING)
    {
        SMBSocketInvalidate(pSocket, ntStatus);
    }

    goto cleanup;
}

VOID
SMBSocketInvalidate(
    PSMB_SOCKET pSocket,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    SMBSocketInvalidate_InLock(pSocket, ntStatus);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

VOID
SMBSocketInvalidate_InLock(
    PSMB_SOCKET pSocket,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInGlobalLock = FALSE;
    pSocket->state = RDR_SOCKET_STATE_ERROR;
    pSocket->error = ntStatus;

    LWIO_LOCK_MUTEX(bInGlobalLock, &gRdrRuntime.socketHashLock);
    RdrSocketUnlink(pSocket);
    LWIO_UNLOCK_MUTEX(bInGlobalLock, &gRdrRuntime.socketHashLock);

    pthread_cond_broadcast(&pSocket->event);
}

VOID
SMBSocketSetState(
    PSMB_SOCKET        pSocket,
    RDR_SOCKET_STATE   state
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->state = state;

    pthread_cond_broadcast(&pSocket->event);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

RDR_SOCKET_STATE
SMBSocketGetState(
    PSMB_SOCKET        pSocket
    )
{
    BOOLEAN bInLock = FALSE;
    RDR_SOCKET_STATE socketState = RDR_SOCKET_STATE_ERROR;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    socketState = pSocket->state;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return socketState;
}

NTSTATUS
SMBSocketFindSessionByPrincipal(
    IN PSMB_SOCKET pSocket,
    IN PCSTR pszPrincipal,
    OUT PSMB_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    PSMB_SESSION pSession = NULL;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    ntStatus = SMBHashGetValue(
                    pSocket->pSessionHashByPrincipal,
                    pszPrincipal,
                    (PVOID *) &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    SMBSessionAddReference(pSession);

    *ppSession = pSession;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

VOID
SMBSocketRelease(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);

    assert(pSocket->refCount > 0);

    /* If the socket is no longer referenced and
       it is not usable, free it immediately.
       Otherwise, allow the reaper to collect it
       asynchronously */
    if (--pSocket->refCount == 0)
    {
        if (pSocket->state != RDR_SOCKET_STATE_READY)
        {
            RdrSocketUnlink(pSocket);
            LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
            SMBSocketFree(pSocket);
        }
        else
        {
            LWIO_LOG_VERBOSE("Socket %p is eligible for reaping", pSocket);
            if (LwRtlCreateTask(
                    gRdrRuntime.pThreadPool,
                    &pSocket->pTimeout,
                    gRdrRuntime.pReaderTaskGroup,
                    RdrSocketTimeout,
                    pSocket) == STATUS_SUCCESS)
            {
                LwRtlWakeTask(pSocket->pTimeout);
                LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
            }
            else
            {
                RdrSocketUnlink(pSocket);
                LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
                SMBSocketFree(pSocket);
            }
        }
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
    }

}

NTSTATUS
SMBSocketWaitReady(
    PSMB_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    while (pSocket->state != RDR_SOCKET_STATE_READY)
    {
        if (pSocket->state == RDR_SOCKET_STATE_ERROR)
        {
            ntStatus = pSocket->error;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pthread_cond_wait(&pSocket->event, &pSocket->mutex);
    }

error:

    return ntStatus;
}

NTSTATUS
SMBSocketWaitSessionSetup(
    PSMB_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    while (pSocket->bSessionSetupInProgress)
    {
        if (pSocket->state == RDR_SOCKET_STATE_ERROR)
        {
            ntStatus = pSocket->error;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pthread_cond_wait(&pSocket->event, &pSocket->mutex);
    }

error:

    return ntStatus;
}

VOID
SMBSocketFree(
    PSMB_SOCKET pSocket
    )
{
    assert(!pSocket->refCount);

    LwRtlCancelTask(pSocket->pTask);
    LwRtlWaitTask(pSocket->pTask);
    LwRtlReleaseTask(&pSocket->pTask);

    if ((pSocket->fd >= 0) && (close(pSocket->fd) < 0))
    {
        LWIO_LOG_ERROR("Failed to close socket [fd:%d]", pSocket->fd);
    }

    pthread_cond_destroy(&pSocket->event);

    LWIO_SAFE_FREE_MEMORY(pSocket->pwszHostname);
    LWIO_SAFE_FREE_MEMORY(pSocket->pwszCanonicalName);
    LWIO_SAFE_FREE_MEMORY(pSocket->pSecurityBlob);

    /* @todo: assert that the session hashes are empty */
    SMBHashSafeFree(&pSocket->pSessionHashByPrincipal);
    SMBHashSafeFree(&pSocket->pSessionHashByUID);

    if (pSocket->pPacket)
    {
        SMBPacketRelease(pSocket->hPacketAllocator, pSocket->pPacket);
    }

    pthread_mutex_destroy(&pSocket->mutex);

    LWIO_SAFE_FREE_MEMORY(pSocket->pSessionKey);

    if (pSocket->pTimeout)
    {
        LwRtlCancelTask(pSocket->pTimeout);
        LwRtlReleaseTask(&pSocket->pTimeout);
    }

    /* @todo: use allocator */
    SMBFreeMemory(pSocket);
}

VOID
RdrSocketSetIgnoreServerSignatures(
    PSMB_SOCKET pSocket,
    BOOLEAN bValue
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->bIgnoreServerSignatures = bValue;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

BOOLEAN
RdrSocketGetIgnoreServerSignatures(
    PSMB_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;
    BOOLEAN bValue = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    bValue = pSocket->bIgnoreServerSignatures;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return bValue;
}


static
NTSTATUS
RdrEaiToNtStatus(
    int eai
    )
{
    switch (eai)
    {
    case 0:
        return STATUS_SUCCESS;
    case EAI_NONAME:
#ifdef EAI_NODATA
    case EAI_NODATA:
#endif
        return STATUS_BAD_NETWORK_NAME;
    case EAI_MEMORY:
        return STATUS_INSUFFICIENT_RESOURCES;
    default:
        return STATUS_UNSUCCESSFUL;
    }
}

NTSTATUS
RdrSocketAcquireMid(
    PSMB_SOCKET pSocket,
    USHORT* pusMid
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    *pusMid = pSocket->usNextMid++;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;
}



NTSTATUS
RdrSocketAddResponse(
    PSMB_SOCKET pSocket,
    PSMB_RESPONSE pResponse
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* @todo: if we allocate the MID outside of this function, we need to
       check for a conflict here */
    ntStatus = SMBHashSetValue(
                    pSocket->pResponseHash,
                    &pResponse->mid,
                    pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponse->pSocket = pSocket;

    if (pResponse->pContext)
    {
        pResponse->pContext->usMid = pResponse->mid;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
RdrSocketRemoveResponse(
    PSMB_SOCKET pSocket,
    PSMB_RESPONSE pResponse
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    /* @todo: if we allocate the MID outside of this function, we need to
       check for a conflict here */
    ntStatus = SMBHashRemoveKey(
                    pSocket->pResponseHash,
                    &pResponse->mid);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponse->pSocket = NULL;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
RdrSocketReceiveResponse(
    IN PSMB_SOCKET pSocket,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    IN PSMB_RESPONSE pResponse,
    OUT PSMB_PACKET* ppResponsePacket
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;
    struct timespec ts = {0, 0};
    int err = 0;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    while (!(pResponse->state == SMB_RESOURCE_STATE_VALID))
    {
        ts.tv_sec = time(NULL) + 30;
        ts.tv_nsec = 0;

retry_wait:

        /* @todo: always verify non-error state after acquiring mutex */
        err = pthread_cond_timedwait(
            &pResponse->event,
            &pSocket->mutex,
            &ts);
        if (err == ETIMEDOUT)
        {
            if (time(NULL) < ts.tv_sec)
            {
                err = 0;
                goto retry_wait;
            }

            ntStatus = STATUS_IO_TIMEOUT;
            SMBSocketInvalidate(pSocket, ntStatus);
            SMBResponseInvalidate_InLock(pResponse, ntStatus);
        }
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBHashRemoveKey(
        pSocket->pResponseHash,
        &pResponse->mid);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponse->pSocket = NULL;

    ntStatus = SMBPacketDecodeHeader(
                    pResponse->pPacket,
                    bVerifySignature && !pSocket->bIgnoreServerSignatures,
                    dwExpectedSequence,
                    pSocket->pSessionKey,
                    pSocket->dwSessionKeyLength);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Could be NULL on error */
    *ppResponsePacket = pResponse->pPacket;
    pResponse->pPacket = NULL;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    *ppResponsePacket = NULL;

    goto cleanup;
}

