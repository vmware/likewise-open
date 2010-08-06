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
VOID
RdrSocketInvalidate_InLock(
    PRDR_SOCKET pSocket,
    NTSTATUS status
    );

static
NTSTATUS
RdrSocketReceiveAndUnmarshall(
    IN PRDR_SOCKET pSocket,
    OUT PSMB_PACKET pPacket
    );

static
NTSTATUS
RdrSocketRead(
    PRDR_SOCKET pSocket,
    uint8_t    *buffer,
    uint32_t    dwLen,
    uint32_t   *actualLen
    );

static
int
RdrSocketHashSessionCompareByUID(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
RdrSocketHashSessionByUID(
    PCVOID vp
    );

static
int
RdrSocketHashSessionCompareByKey(
    PCVOID vp1,
    PCVOID vp2
    );

static
size_t
RdrSocketHashSessionByKey(
    PCVOID vp
    );

static
NTSTATUS
RdrSocketSendData(
    IN PRDR_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    );

static
VOID
RdrSocketTask(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    );

static
NTSTATUS
RdrSocketFindAndSignalResponse(
    PRDR_SOCKET pSocket,
    PSMB_PACKET pPacket
    );

static
NTSTATUS
RdrEaiToNtStatus(
    int eai
    );

static
VOID
RdrSocketFreeContents(
    PRDR_SOCKET pSocket
    );

static
VOID
RdrSocketFree(
    PRDR_SOCKET pSocket
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
RdrSocketCreate(
    IN PCWSTR pwszHostname,
    IN BOOLEAN bUseSignedMessagesIfSupported,
    OUT PRDR_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;
    RDR_SOCKET *pSocket = NULL;
    BOOLEAN bDestroyCondition = FALSE;
    BOOLEAN bDestroyMutex = FALSE;
    PWSTR pwszCanonicalName = NULL;
    PWSTR pwszCursor = NULL;

    ntStatus = SMBAllocateMemory(
                sizeof(RDR_SOCKET),
                (PVOID*)&pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pSocket->PendingSend);
    LwListInit(&pSocket->StateWaiters);

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
                    RdrSocketHashSessionCompareByKey,
                    RdrSocketHashSessionByKey,
                    NULL,
                    &pSocket->pSessionHashByPrincipal);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SMBHashCreate(
                    19,
                    RdrSocketHashSessionCompareByUID,
                    RdrSocketHashSessionByUID,
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
        RdrSocketTask,
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
RdrSocketHashSessionCompareByUID(
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
RdrSocketHashSessionByUID(
    PCVOID vp
    )
{
   return *((uint16_t *) vp);
}

static
int
RdrSocketHashSessionCompareByKey(
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
RdrSocketHashSessionByKey(
    PCVOID vp
    )
{
    const struct _RDR_SESSION_KEY* pKey = (struct _RDR_SESSION_KEY*) vp;

    return SMBHashCaselessString(pKey->pszPrincipal) ^ (pKey->uid ^ (pKey->uid << 16));
}

static
BOOLEAN
RdrSocketIsSignatureRequired(
    IN PRDR_SOCKET pSocket
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
RdrSocketGetNextSequence_inlock(
    PRDR_SOCKET pSocket
    )
{
    DWORD dwSequence = 0;

    dwSequence = pSocket->dwSequence;
    // Next for response
    // Next for next message
    pSocket->dwSequence += 2;

    return dwSequence;
}

VOID
RdrSocketBeginSequence(
    PRDR_SOCKET pSocket
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->dwSequence = 2;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

static
NTSTATUS
RdrSocketSendData(
    IN PRDR_SOCKET pSocket,
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
RdrSocketPrepareSend(
    IN PRDR_SOCKET pSocket,
    IN PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    BOOLEAN bIsSignatureRequired = FALSE;

    if (pPacket->allowSignature)
    {
        bIsSignatureRequired = RdrSocketIsSignatureRequired(pSocket);
    }

    if (bIsSignatureRequired)
    {
        pPacket->pSMBHeader->flags2 |= FLAG2_SECURITY_SIG;
    }

    SMBPacketHTOLSmbHeader(pPacket->pSMBHeader);

    pPacket->haveSignature = bIsSignatureRequired;

    if (pPacket->pSMBHeader->command != COM_NEGOTIATE)
    {
        pPacket->sequence = RdrSocketGetNextSequence_inlock(pSocket);
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
RdrSocketSend(
    IN PRDR_SOCKET pSocket,
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

    ntStatus = RdrSocketPrepareSend(pSocket, pPacket);
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

static
NTSTATUS
RdrSocketQueue(
    IN PRDR_SOCKET pSocket,
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
    IN PRDR_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_RESPONSE pResponse = NULL;
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

    status = RdrSocketQueue(pSocket, pContext);
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
RdrSocketReceiveAndUnmarshall(
    IN PRDR_SOCKET pSocket,
    OUT PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    uint32_t readLen = 0;

    if (pPacket->bufferUsed < sizeof(NETBIOS_HEADER))
    {
        while (pPacket->bufferUsed < sizeof(NETBIOS_HEADER))
        {
            ntStatus = RdrSocketRead(
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
        ntStatus = RdrSocketRead(
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
RdrSocketTask(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    NTSTATUS ntStatus = 0;
    PRDR_SOCKET pSocket = (PRDR_SOCKET) pContext;
    BOOLEAN bGlobalLock = FALSE;
    BOOLEAN bInLock = FALSE;
    int err = 0;
    SOCKLEN_T len = sizeof(err);
    static const LONG64 llConnectTimeout = 10 * 1000000000ll; // 10 sec
    PLW_LIST_LINKS pLink = NULL;
    PRDR_OP_CONTEXT pIrpContext = NULL;

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        /* If we are being explicitly cancelled,
           we are charged with cleaning up the socket */
        LWIO_LOCK_MUTEX(bGlobalLock, &gRdrRuntime.socketHashLock);
        RdrSocketFreeContents(pSocket);
        LWIO_UNLOCK_MUTEX(bGlobalLock, &gRdrRuntime.socketHashLock);

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
        ntStatus = RdrSocketPrepareSend(pSocket, &pIrpContext->Packet);
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

        ntStatus = RdrSocketReceiveAndUnmarshall(pSocket, pSocket->pPacket);
        switch(ntStatus)
        {
        case STATUS_SUCCESS:
            /* This function should free packet and socket memory on error */
            ntStatus = RdrSocketFindAndSignalResponse(pSocket, pSocket->pPacket);
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
        ntStatus = RdrSocketSendData(pSocket, pSocket->pOutgoing);
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
        RdrSocketInvalidate_InLock(pSocket, ntStatus);
        *pWaitMask = LW_TASK_EVENT_COMPLETE;
    }

    goto cleanup;
}

static
NTSTATUS
RdrSocketFindResponseByMid(
    PRDR_SOCKET pSocket,
    USHORT usMid,
    PRDR_RESPONSE* ppResponse
    )
{
    NTSTATUS ntStatus = 0;
    PRDR_RESPONSE pResponse = NULL;

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
RdrSocketFindAndSignalResponse(
    PRDR_SOCKET pSocket,
    PSMB_PACKET pPacket
    )
{
    NTSTATUS ntStatus = 0;
    PRDR_RESPONSE pResponse = NULL;
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
    IN PRDR_SOCKET pSocket,
    IN PRDR_OP_CONTEXT pContext
    )
{
    /* Currently a no-op */
}

VOID
RdrSocketRevive(
    PRDR_SOCKET pSocket
    )
{
    if (pSocket->pTimeout)
    {
        LwRtlCancelTask(pSocket->pTimeout);
        LwRtlReleaseTask(&pSocket->pTimeout);
    }
}

static
VOID
RdrSocketUnlink(
    PRDR_SOCKET pSocket
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
RdrSocketTimeout(
    PLW_TASK pTask,
    LW_PVOID pContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    LW_LONG64* pllTime
    )
{
    PRDR_SOCKET pSocket = pContext;
    BOOLEAN bInLock = FALSE;

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
        LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);

        if (pSocket->refCount == 0)
        {
            RdrSocketUnlink(pSocket);
            RdrSocketFree(pSocket);
            *pWaitMask = LW_TASK_EVENT_COMPLETE;
        }
        else
        {
            *pWaitMask = LW_TASK_EVENT_TIME;
            *pllTime = RDR_IDLE_TIMEOUT * 1000000000ll;
        }

        LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
    }
}

NTSTATUS
RdrSocketConnect(
    PRDR_SOCKET pSocket
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

    RdrSocketInvalidate(pSocket, ntStatus);

    goto cleanup;
}

static
NTSTATUS
RdrSocketRead(
    PRDR_SOCKET pSocket,
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
        RdrSocketInvalidate(pSocket, ntStatus);
    }

    goto cleanup;
}

VOID
RdrSocketInvalidate(
    PRDR_SOCKET pSocket,
    NTSTATUS ntStatus
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    RdrSocketInvalidate_InLock(pSocket, ntStatus);

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
}

static
VOID
RdrSocketInvalidate_InLock(
    PRDR_SOCKET pSocket,
    NTSTATUS status
    )
{
    BOOLEAN bInGlobalLock = FALSE;
    PLW_LIST_LINKS pLink = NULL;
    PRDR_OP_CONTEXT pContext = NULL;
    SMB_HASH_ITERATOR iter = {0};
    SMB_HASH_ENTRY* pEntry = NULL;
    PRDR_RESPONSE pResponse = NULL;
    BOOLEAN bLocked = TRUE;

    pSocket->state = RDR_SOCKET_STATE_ERROR;
    pSocket->error = status;

    LWIO_LOCK_MUTEX(bInGlobalLock, &gRdrRuntime.socketHashLock);
    RdrSocketUnlink(pSocket);
    LWIO_UNLOCK_MUTEX(bInGlobalLock, &gRdrRuntime.socketHashLock);

    pthread_cond_broadcast(&pSocket->event);

    while ((pLink = LwListTraverse(&pSocket->PendingSend, pLink)))
    {
        pContext = LW_STRUCT_FROM_FIELD(pLink, RDR_OP_CONTEXT, Link);

        if (RdrSocketFindResponseByMid(pSocket, pContext->usMid, &pResponse) == STATUS_SUCCESS)
        {
            SMBHashRemoveKey(pSocket->pResponseHash, &pContext->usMid);

            pResponse->pSocket = NULL;
            SMBResponseFree(pResponse);
        }
    }

    RdrNotifyContextList(
        &pSocket->PendingSend,
        TRUE,
        &pSocket->mutex,
        status,
        NULL);

    RdrNotifyContextList(
        &pSocket->StateWaiters,
        TRUE,
        &pSocket->mutex,
        status,
        pSocket);


    if (SMBHashGetIterator(pSocket->pResponseHash, &iter))
    {
        abort();
    }

    LWIO_UNLOCK_MUTEX(bLocked, &pSocket->mutex);
    while ((pEntry = SMBHashNext(&iter)))
    {
        pResponse = pEntry->pValue;

        RdrContinueContext(pResponse->pContext, status, NULL);

        pResponse->pSocket = NULL;
        SMBResponseFree(pResponse);
    }
    LWIO_LOCK_MUTEX(bLocked, &pSocket->mutex);
}

VOID
RdrSocketRelease(
    PRDR_SOCKET pSocket
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
            RdrSocketFree(pSocket);
            LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
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
                LWIO_LOG_VERBOSE("Could not start timer for socket %p; closing immediately", pSocket);
                RdrSocketUnlink(pSocket);
                RdrSocketFree(pSocket);
                LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
            }
        }
    }
    else
    {
        LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);
    }
}

NTSTATUS
RdrSocketWaitReady(
    PRDR_SOCKET pSocket
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

static
VOID
RdrSocketFree(
    PRDR_SOCKET pSocket
    )
{
    /*
     * If the task for the socket is alive, cancel it and let
     * it finish freeing the socket after it has cleaned up.
     * Otherwise, free the contents immediately.
     */
    if (pSocket->pTask)
    {
        LwRtlCancelTask(pSocket->pTask);
        LwRtlReleaseTask(&pSocket->pTask);
    }
    else
    {
        RdrSocketFreeContents(pSocket);
    }
}

static
VOID
RdrSocketFreeContents(
    PRDR_SOCKET pSocket
    )
{
    assert(!pSocket->refCount);

    if ((pSocket->fd >= 0) && (close(pSocket->fd) < 0))
    {
        LWIO_LOG_ERROR("Failed to close socket [fd:%d]", pSocket->fd);
    }

    pthread_cond_destroy(&pSocket->event);

    LWIO_SAFE_FREE_MEMORY(pSocket->pwszHostname);
    LWIO_SAFE_FREE_MEMORY(pSocket->pwszCanonicalName);
    LWIO_SAFE_FREE_MEMORY(pSocket->pSecurityBlob);

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

    SMBFreeMemory(pSocket);
}

VOID
RdrSocketSetIgnoreServerSignatures(
    PRDR_SOCKET pSocket,
    BOOLEAN bValue
    )
{
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    pSocket->bIgnoreServerSignatures = bValue;

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);
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
    PRDR_SOCKET pSocket,
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
    PRDR_SOCKET pSocket,
    PRDR_RESPONSE pResponse
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
    PRDR_SOCKET pSocket,
    PRDR_RESPONSE pResponse
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
    IN PRDR_SOCKET pSocket,
    IN BOOLEAN bVerifySignature,
    IN DWORD dwExpectedSequence,
    IN PRDR_RESPONSE pResponse,
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
            RdrSocketInvalidate(pSocket, ntStatus);
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

static
NTSTATUS
_FindOrCreateSocket(
    IN PCWSTR pwszHostname,
    OUT PRDR_SOCKET* ppSocket
    );

NTSTATUS
RdrSocketInit(
    VOID
    )
{
    NTSTATUS ntStatus = 0;

    assert(!gRdrRuntime.pSocketHashByName);

    ntStatus = SMBHashCreate(
                    19,
                    SMBHashCaselessWc16StringCompare,
                    SMBHashCaselessWc16String,
                    NULL,
                    &gRdrRuntime.pSocketHashByName);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

NTSTATUS
SMBSrvClientSocketCreate(
    IN PCWSTR pwszHostname,
    OUT PRDR_SOCKET* ppSocket
    )
{
    return _FindOrCreateSocket(pwszHostname, ppSocket);
}

static
NTSTATUS
_FindOrCreateSocket(
    IN PCWSTR pwszHostname,
    OUT PRDR_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;

    PRDR_SOCKET pSocket = NULL;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);

    ntStatus = SMBHashGetValue(
        gRdrRuntime.pSocketHashByName,
        pwszHostname,
        (PVOID *) &pSocket);

    if (!ntStatus)
    {
        pSocket->refCount++;
        RdrSocketRevive(pSocket);
    }
    else
    {
        ntStatus = RdrSocketCreate(
            pwszHostname,
            gRdrRuntime.config.bSignMessagesIfSupported,
            &pSocket);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBHashSetValue(
            gRdrRuntime.pSocketHashByName,
            pSocket->pwszHostname,
            pSocket);
        BAIL_ON_NT_STATUS(ntStatus);

        pSocket->bParentLink = TRUE;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);

    *ppSocket = pSocket;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &gRdrRuntime.socketHashLock);

    return ntStatus;

error:

    *ppSocket = NULL;

    goto cleanup;
}

NTSTATUS
SMBSrvClientSocketAddSessionByUID(
    PRDR_SOCKET  pSocket,
    PRDR_SESSION pSession
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN bInLock = FALSE;

    LWIO_LOCK_MUTEX(bInLock, &pSocket->mutex);

    ntStatus = SMBHashSetValue(
                    pSocket->pSessionHashByUID,
                    &pSession->uid,
                    pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    pSession->bParentLink = TRUE;

cleanup:

    LWIO_UNLOCK_MUTEX(bInLock, &pSocket->mutex);

    return ntStatus;

error:

    goto cleanup;
}

NTSTATUS
RdrSocketShutdown(
    VOID
    )
{
    SMBHashSafeFree(&gRdrRuntime.pSocketHashByName);

    return 0;
}

NTSTATUS
SMBResponseCreate(
    uint16_t       wMid,
    RDR_RESPONSE **ppResponse
    )
{
    NTSTATUS ntStatus = 0;
    PRDR_RESPONSE pResponse = NULL;
    BOOLEAN bDestroyCondition = FALSE;

    ntStatus = SMBAllocateMemory(
                    sizeof(RDR_RESPONSE),
                    (PVOID*)&pResponse);
    BAIL_ON_NT_STATUS(ntStatus);

    pResponse->state = SMB_RESOURCE_STATE_INITIALIZING;

    ntStatus = pthread_cond_init(&pResponse->event, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    bDestroyCondition = TRUE;

    pResponse->mid = wMid;
    pResponse->pPacket = NULL;

    *ppResponse = pResponse;

cleanup:

    return ntStatus;

error:

    if (bDestroyCondition)
    {
        pthread_cond_destroy(&pResponse->event);
    }

    LWIO_SAFE_FREE_MEMORY(pResponse);

    *ppResponse = NULL;

    goto cleanup;
}

VOID
SMBResponseFree(
    PRDR_RESPONSE pResponse
    )
{
    if (pResponse->pSocket)
    {
        RdrSocketRemoveResponse(pResponse->pSocket, pResponse);
    }

    pthread_cond_destroy(&pResponse->event);

    SMBFreeMemory(pResponse);
}

VOID
SMBResponseInvalidate_InLock(
    PRDR_RESPONSE pResponse,
    NTSTATUS ntStatus
    )
{
    pResponse->state = SMB_RESOURCE_STATE_INVALID;
    pResponse->error = ntStatus;

    if (pResponse->pContext)
    {
        RdrContinueContext(pResponse->pContext, ntStatus, NULL);
    }

    pthread_cond_broadcast(&pResponse->event);
}
