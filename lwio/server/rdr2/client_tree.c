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

#include "rdr.h"

static
NTSTATUS
RdrTransceiveTreeConnect(
    PRDR_OP_CONTEXT pContext,
    PSMB_TREE pTree,
    PCWSTR pwszPath
    );

static
NTSTATUS
SMBSrvClientTreeCreate(
    IN OUT PSMB_SESSION* ppSession,
    IN PCSTR pszPath,
    OUT PSMB_TREE* ppTree
    );

static
NTSTATUS
RdrAcquireNegotiatedSocket(
    PCWSTR pwszHostname,
    OUT PSMB_SOCKET* ppSocket
    );

static
NTSTATUS
RdrAcquireEstablishedSession(
    IN OUT PSMB_SOCKET* ppSocket,
    PIO_CREDS pCreds,
    uid_t uid,
    OUT PSMB_SESSION* ppSession
    );

static
VOID
RdrAcquireSessionWorkItem(
    PVOID _pContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = _pContext;
    PSMB_SOCKET pSocket = NULL;
    PSMB_SESSION pSession = NULL;
    PIO_CREDS pCreds = pContext->State.TreeConnect.pCreds;
    PSTR pszCachePath = NULL;

    status = RdrAcquireNegotiatedSocket(
        pContext->State.TreeConnect.pwszHostname,
        &pSocket);
    BAIL_ON_NT_STATUS(status);

    switch (pCreds->type)
    {
    case IO_CREDS_TYPE_KRB5_TGT:
        status = SMBCredTokenToKrb5CredCache(pCreds, &pszCachePath);
        BAIL_ON_NT_STATUS(status);

        status = SMBKrb5SetDefaultCachePath(pszCachePath, NULL);
        BAIL_ON_NT_STATUS(status);
        break;
    case IO_CREDS_TYPE_PLAIN:
        break;
    default:
        status = STATUS_ACCESS_DENIED;
        BAIL_ON_NT_STATUS(status);
    }

    status = RdrAcquireEstablishedSession(
        &pSocket,
        pCreds,
        pContext->State.TreeConnect.Uid,
        &pSession);
    BAIL_ON_NT_STATUS(status);

cleanup:

    if (pszCachePath)
    {
        SMBKrb5DestroyCache(pszCachePath);
        RTL_FREE(&pszCachePath);
    }

    RdrContinueContext(pContext, status, pSession);

    return;

error:

    if (status != STATUS_PENDING && pSession)
    {
        SMBSessionRelease(pSession);
        pSession = NULL;
    }

    if (status != STATUS_PENDING && pSocket)
    {
        SMBSocketRelease(pSocket);
        pSocket = NULL;
    }

    goto cleanup;
}


static
BOOLEAN
RdrFinishTreeConnect(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_TREE pTree = pContext->State.TreeConnect.pTree;
    PSMB_PACKET pResponsePacket = pParam;
    BOOLEAN bTreeLocked = FALSE;

    LWIO_LOCK_MUTEX(bTreeLocked, &pTree->mutex);

    BAIL_ON_NT_STATUS(status);

    status = pResponsePacket->pSMBHeader->error;
    BAIL_ON_NT_STATUS(status);

    pTree->tid = pResponsePacket->pSMBHeader->tid;
    pTree->state = RDR_TREE_STATE_READY;

cleanup:

    if (pResponsePacket)
    {
        SMBPacketRelease(
            pTree->pSession->pSocket->hPacketAllocator,
            pResponsePacket);
    }

    RdrNotifyContextList(&pTree->StateWaiters, bTreeLocked, &pTree->mutex, status, pTree);

    LWIO_UNLOCK_MUTEX(bTreeLocked, &pTree->mutex);

    RTL_FREE(&pContext->State.TreeConnect.pwszHostname);
    RTL_FREE(&pContext->State.TreeConnect.pszSharename);
    LwIoDeleteCreds(pContext->State.TreeConnect.pCreds);
    RdrFreeContext(pContext);

    return FALSE;

error:

    goto cleanup;
}

static
BOOLEAN
RdrTreeConnectSessionSetup(
    PRDR_OP_CONTEXT pContext,
    NTSTATUS status,
    PVOID pParam
    )
{
    PSMB_SESSION pSession = pParam;
    PSMB_TREE pTree = NULL;
    BOOLEAN bTreeLocked = FALSE;
    BOOLEAN bFreeContext = FALSE;
    PWSTR pwszPath = NULL;

    BAIL_ON_NT_STATUS(status);

    status = SMBSrvClientTreeCreate(
        &pSession,
        pContext->State.TreeConnect.pszSharename,
        &pTree);
    BAIL_ON_NT_STATUS(status);

    pContext->State.TreeConnect.pTree = pTree;

    LWIO_LOCK_MUTEX(bTreeLocked, &pTree->mutex);

    switch (pTree->state)
    {
    case RDR_TREE_STATE_NOT_READY:
        LwListInsertTail(&pTree->StateWaiters, &pContext->State.TreeConnect.pContinue->Link);
        pTree->state = RDR_TREE_STATE_INITIALIZING;

        status = SMBMbsToWc16s(pTree->pszPath, &pwszPath);
        BAIL_ON_NT_STATUS(status);

        pContext->Continue = RdrFinishTreeConnect;

        status = RdrTransceiveTreeConnect(pContext, pTree, pwszPath);
        BAIL_ON_NT_STATUS(status);
        break;
    case RDR_TREE_STATE_INITIALIZING:
        LwListInsertTail(&pTree->StateWaiters, &pContext->State.TreeConnect.pContinue->Link);
        bFreeContext = TRUE;
        status = STATUS_PENDING;
        break;
    case RDR_TREE_STATE_READY:
        bFreeContext = TRUE;
        break;
    case RDR_TREE_STATE_ERROR:
        status = pTree->error;
        BAIL_ON_NT_STATUS(status);
        break;
    }

cleanup:

    LWIO_UNLOCK_MUTEX(bTreeLocked, &pTree->mutex);

    if (status != STATUS_PENDING)
    {
        RdrContinueContext(pContext->State.TreeConnect.pContinue, status, pTree);
        bFreeContext = TRUE;
    }

    if (bFreeContext)
    {
        RTL_FREE(&pContext->State.TreeConnect.pwszHostname);
        RTL_FREE(&pContext->State.TreeConnect.pszSharename);
        LwIoDeleteCreds(pContext->State.TreeConnect.pCreds);
        RdrFreeContext(pContext);
    }

    RTL_FREE(&pwszPath);

    return FALSE;

error:

    goto cleanup;
}


static
NTSTATUS
RdrAcquireNegotiatedSocket(
    IN PCWSTR pwszHostname,
    OUT PSMB_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_SOCKET pSocket = NULL;
    BOOLEAN bInSocketLock = FALSE;

    ntStatus = SMBSrvClientSocketCreate(
        pwszHostname,
        &pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bInSocketLock, &pSocket->mutex);

    if (pSocket->state == RDR_SOCKET_STATE_NOT_READY)
    {
        /* We're the first thread to use this socket.
           Go through the connect/negotiate procedure */
        pSocket->state = RDR_SOCKET_STATE_CONNECTING;
        LWIO_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);

        ntStatus = SMBSocketConnect(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = Negotiate(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    else
    {
        ntStatus = SMBSocketWaitReady(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);
    }

    *ppSocket = pSocket;

cleanup:

    return ntStatus;

error:

    *ppSocket = NULL;

    LWIO_UNLOCK_MUTEX(bInSocketLock, &pSocket->mutex);

    if (pSocket)
    {
        SMBSocketRelease(pSocket);
    }

    goto cleanup;
}

static
NTSTATUS
RdrAcquireEstablishedSession(
    PSMB_SOCKET* ppSocket,
    PIO_CREDS pCreds,
    uid_t uid,
    OUT PSMB_SESSION* ppSession
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSMB_SESSION pSession = NULL;
    BOOLEAN bInSessionLock = FALSE;
    BOOLEAN bInSocketLock = FALSE;
    BOOLEAN bSessionSetupInProgress = FALSE;

    ntStatus = SMBSrvClientSessionCreate(
        ppSocket,
        pCreds,
        uid,
        &pSession);
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOCK_MUTEX(bInSessionLock, &pSession->mutex);

    if (pSession->state == RDR_SESSION_STATE_NOT_READY)
    {
        /* Begin initializing session */
        pSession->state = RDR_SESSION_STATE_INITIALIZING;
        LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);

        /* Exclude other session setups on this socket */
        LWIO_LOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
        ntStatus = SMBSocketWaitSessionSetup(pSession->pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
        pSession->pSocket->bSessionSetupInProgress = bSessionSetupInProgress = TRUE;
        LWIO_UNLOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);

        ntStatus = SessionSetup(
                    pSession->pSocket,
                    pCreds,
                    &pSession->uid,
                    &pSession->pSessionKey,
                    &pSession->dwSessionKeyLength);
        BAIL_ON_NT_STATUS(ntStatus);

        if (!pSession->pSocket->pSessionKey && pSession->pSessionKey)
        {
            ntStatus = SMBAllocateMemory(
                pSession->dwSessionKeyLength,
                (PVOID*)&pSession->pSocket->pSessionKey);
            BAIL_ON_NT_STATUS(ntStatus);

            memcpy(pSession->pSocket->pSessionKey, pSession->pSessionKey, pSession->dwSessionKeyLength);

            pSession->pSocket->dwSessionKeyLength = pSession->dwSessionKeyLength;
        }

        ntStatus = SMBSrvClientSocketAddSessionByUID(
            pSession->pSocket,
            pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        /* Wake up anyone waiting for session to be ready */
        LWIO_LOCK_MUTEX(bInSessionLock, &pSession->mutex);
        pSession->state = RDR_SESSION_STATE_READY;
        pthread_cond_broadcast(&pSession->event);
        LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);

        /* Wake up anyone waiting for session setup exclusion */
        LWIO_LOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
        pSession->pSocket->bSessionSetupInProgress = bSessionSetupInProgress = FALSE;
        pthread_cond_broadcast(&pSession->pSocket->event);
        LWIO_UNLOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
    }
    else
    {
        /* Wait for session to be ready */
        ntStatus = SMBSessionWaitReady(pSession);
        BAIL_ON_NT_STATUS(ntStatus);

        LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);
    }

    *ppSession = pSession;

cleanup:

    return ntStatus;

error:

    *ppSession = NULL;

    if (bSessionSetupInProgress)
    {
        LWIO_LOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
        pSession->pSocket->bSessionSetupInProgress = FALSE;
        pthread_cond_broadcast(&pSession->pSocket->event);
    }

    LWIO_UNLOCK_MUTEX(bInSocketLock, &pSession->pSocket->mutex);
    LWIO_UNLOCK_MUTEX(bInSessionLock, &pSession->mutex);

    if (pSession)
    {
        SMBSessionRelease(pSession);
    }

    goto cleanup;
}

static
NTSTATUS
SMBSrvClientTreeCreate(
    IN OUT PSMB_SESSION* ppSession,
    IN PCSTR pszPath,
    OUT PSMB_TREE* ppTree
    )
{
    DWORD     ntStatus = 0;
    PSMB_TREE pTree = NULL;
    BOOLEAN   bInLock = FALSE;
    PSMB_SESSION pSession = *ppSession;

    LWIO_LOCK_MUTEX(bInLock, &pSession->mutex);

    ntStatus = SMBHashGetValue(
                pSession->pTreeHashByPath,
                pszPath,
                (PVOID *) &pTree);

    if (!ntStatus)
    {
        pTree->refCount++;
        RdrTreeRevive(pTree);
        SMBSessionRelease(pSession);
        *ppSession = NULL;
    }
    else
    {
        ntStatus = SMBTreeCreate(&pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        pTree->pSession = pSession;

        ntStatus = SMBStrndup(
            pszPath,
            strlen(pszPath) + 1,
            &pTree->pszPath);
        BAIL_ON_NT_STATUS(ntStatus);

        ntStatus = SMBHashSetValue(
            pSession->pTreeHashByPath,
            pTree->pszPath,
            pTree);
        BAIL_ON_NT_STATUS(ntStatus);

        pTree->bParentLink = TRUE;

        *ppSession = NULL;
    }

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    *ppTree = pTree;

cleanup:

    return ntStatus;

error:

    LWIO_UNLOCK_MUTEX(bInLock, &pSession->mutex);

    *ppTree = NULL;

    if (pTree)
    {
        SMBTreeRelease(pTree);
    }

    goto cleanup;
}

static
NTSTATUS
RdrTransceiveTreeConnect(
    PRDR_OP_CONTEXT pContext,
    PSMB_TREE pTree,
    PCWSTR pwszPath
    )
{
    NTSTATUS status = 0;
    uint32_t packetByteCount = 0;
    TREE_CONNECT_REQUEST_HEADER *pHeader = NULL;

    status = RdrAllocateContextPacket(
        pContext,
        1024*64);
    BAIL_ON_NT_STATUS(status);

    status = SMBPacketMarshallHeader(
                    pContext->Packet.pRawBuffer,
                    pContext->Packet.bufferLen,
                    COM_TREE_CONNECT_ANDX,
                    0,
                    0,
                    0,
                    gRdrRuntime.SysPid,
                    pTree->pSession->uid,
                    0,
                    TRUE,
                    &pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    pContext->Packet.pData = pContext->Packet.pParams + sizeof(TREE_CONNECT_REQUEST_HEADER);
    pContext->Packet.bufferUsed += sizeof(TREE_CONNECT_REQUEST_HEADER);
    /* If most commands have word counts which are easy to compute, this
       should be folded into a parameter to SMBPacketMarshallHeader() */
    pContext->Packet.pSMBHeader->wordCount = 4;

    pHeader = (TREE_CONNECT_REQUEST_HEADER *) pContext->Packet.pParams;

    pHeader->flags = 0;
    pHeader->passwordLength = 1;    /* strlen("") + terminating NULL */

    status = MarshallTreeConnectRequestData(
                    pContext->Packet.pData,
                    pContext->Packet.bufferLen - pContext->Packet.bufferUsed,
                    (pContext->Packet.pData - (uint8_t *) pContext->Packet.pSMBHeader) % 2,
                    &packetByteCount,
                    pwszPath,
                    "?????");
    BAIL_ON_NT_STATUS(status);

    assert(packetByteCount <= UINT16_MAX);
    pHeader->byteCount = (uint16_t) packetByteCount;
    pContext->Packet.bufferUsed += packetByteCount;

    // byte order conversions
    SMB_HTOL16_INPLACE(pHeader->flags);
    SMB_HTOL16_INPLACE(pHeader->passwordLength);
    SMB_HTOL16_INPLACE(pHeader->byteCount);

    status = SMBPacketMarshallFooter(&pContext->Packet);
    BAIL_ON_NT_STATUS(status);

    status = RdrSocketTransceive(pTree->pSession->pSocket, pContext);
    BAIL_ON_NT_STATUS(status);

cleanup:

    return status;

error:

    goto cleanup;
}

NTSTATUS
RdrTreeConnect(
    PCWSTR pwszHostname,
    PCSTR pszSharename,
    PIO_CREDS pCreds,
    uid_t Uid,
    PRDR_OP_CONTEXT pContinue
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PRDR_OP_CONTEXT pContext = NULL;

    status = RdrCreateContext(pContinue->pIrp, &pContext);
    BAIL_ON_NT_STATUS(status);

    /* These parameters might be temporary allocations,
       so we need to copy them */
    status = LwRtlWC16StringDuplicate(
        &pContext->State.TreeConnect.pwszHostname,
        pwszHostname);
    BAIL_ON_NT_STATUS(status);

    status = LwRtlCStringDuplicate(
        &pContext->State.TreeConnect.pszSharename,
        pszSharename);
    BAIL_ON_NT_STATUS(status);

    status = LwIoCopyCreds(
        pCreds,
        &pContext->State.TreeConnect.pCreds);
    BAIL_ON_NT_STATUS(status);

    pContext->State.TreeConnect.Uid = Uid;
    pContext->State.TreeConnect.pContinue = pContinue;
    pContext->Continue = RdrTreeConnectSessionSetup;

    status = LwRtlQueueWorkItem(
        gRdrRuntime.pThreadPool,
        RdrAcquireSessionWorkItem,
        pContext,
        0);
    BAIL_ON_NT_STATUS(status);

    status = STATUS_PENDING;

cleanup:

    return status;

error:

    goto cleanup;
}
