#include "includes.h"

static
inline
VOID
SRV_SOCKET_LOCK(
    IN PSRV_SOCKET pSocket
    )
{
    LwRtlLockMutex(&pSocket->Mutex);
}

static
inline
VOID
SRV_SOCKET_UNLOCK(
    IN PSRV_SOCKET pSocket
    )
{
    LwRtlUnlockMutex(&pSocket->Mutex);
}

static
inline
VOID
SRV_SOCKET_LOCK_WITH(
    IN OUT PBOOLEAN pbIsLocked,
    IN PSRV_SOCKET pSocket
    )
{
    LWIO_ASSERT(!*pbIsLocked);
    SRV_SOCKET_LOCK(pSocket);
    *pbIsLocked = TRUE;
}

static
inline
VOID
SRV_SOCKET_UNLOCK_WITH(
    IN OUT PBOOLEAN pbIsLocked,
    IN PSRV_SOCKET pSocket
    )
{
    if (*pbIsLocked)
    {
        SRV_SOCKET_UNLOCK(pSocket);
        *pbIsLocked = FALSE;
    }
}

static
inline
PSRV_TRANSPORT_PROTOCOL_DISPATCH
SrvSocketGetDispatch(
    IN PSRV_SOCKET pSocket
    )
{
    return &pSocket->pListener->pTransport->Dispatch;
}

static
NTSTATUS
SrvSocketProcessTaskWrite(
    IN OUT PSRV_SOCKET pSocket
    );

static
VOID
SrvSocketProcessTaskDisconnect(
    IN PSRV_SOCKET pSocket
    );

static
VOID
SrvSocketProcessTask(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    );

// Implementations

PCSTR
SrvSocketAddressToString(
    IN struct sockaddr* pSocketAddress,
    OUT PSTR pszAddress,
    IN ULONG AddressLength
    )
{
    PCSTR pszResult = NULL;
    PVOID pAddressPart = NULL;

    switch (pSocketAddress->sa_family)
    {
        case AF_INET:
            pAddressPart = &((struct sockaddr_in*)pSocketAddress)->sin_addr;
            break;
#ifdef AF_INET6
        case AF_INET6:
            pAddressPart = &((struct sockaddr_in6*)pSocketAddress)->sin6_addr;
            break;
#endif
        default:
            goto error;
    }

    pszResult = inet_ntop(pSocketAddress->sa_family,
                          pAddressPart,
                          pszAddress,
                          AddressLength);

cleanup:

    return pszResult;

error:

    pszResult = NULL;

    // Terminate output buffer
    if (AddressLength > 0)
    {
        pszAddress[0] = 0;
    }

    goto cleanup;
}

NTSTATUS
SrvSocketCreate(
    IN PSRV_TRANSPORT_LISTENER pListener,
    IN int fd,
    IN struct sockaddr* pClientAddress,
    IN SOCKLEN_T ClientAddressLength,
    OUT PSRV_SOCKET* ppSocket
    )
{
    NTSTATUS ntStatus = 0;
    PSRV_SOCKET pSocket = NULL;
    PCSTR pszAddress = NULL;

    if (ClientAddressLength > sizeof(pSocket->ClientAddress))
    {
        LWIO_LOG_ERROR("Client address is too long at %d bytes",
                       ClientAddressLength);
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvAllocateMemory(sizeof(*pSocket), OUT_PPVOID(&pSocket));
    BAIL_ON_NT_STATUS(ntStatus);

    LwListInit(&pSocket->SendHead);

    ntStatus = LwRtlInitializeMutex(&pSocket->Mutex, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket->pListener = pListener;
    pSocket->fd = fd;

    memcpy(&pSocket->ClientAddress, pClientAddress, ClientAddressLength);
    pSocket->ClientAddressLength = ClientAddressLength;

    pszAddress = SrvSocketAddressToString(
                            &pSocket->ClientAddress.Generic,
                            pSocket->AddressStringBuffer,
                            sizeof(pSocket->AddressStringBuffer));
    if (!pszAddress)
    {
        // This should never happen.
        LWIO_LOG_ERROR("Cannot fetch address string for fd = %d", fd);
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = LwRtlCreateTask(
                    pListener->pPool,
                    &pSocket->pTask,
                    pListener->pTaskGroup,
                    SrvSocketProcessTask,
                    pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO - Perhaps grab mutex on socket first in case
    // protocol driver tries to close socket right after returning
    // (from another thread) to make sure that pConnection
    // is set.
    ntStatus = SrvSocketGetDispatch(pSocket)->pfnConnectionNew(
                    &pSocket->pConnection,
                    pListener->pTransport->pContext,
                    pSocket);
    if (STATUS_ACCESS_DENIED == ntStatus)
    {
        LWIO_LOG_ERROR("Connection denied by protocol for fd = %d, address = '%s'",
                       pSocket->fd,
                       pSocket->AddressStringBuffer);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = STATUS_SUCCESS;
    LwRtlWakeTask(pSocket->pTask);

cleanup:

    *ppSocket = pSocket;

    return ntStatus;

error:

    if (pSocket)
    {
        if (pSocket->pConnection)
        {
            pSocket->DoneStatus = ntStatus;
            LwRtlWakeTask(pSocket->pTask);
            pSocket = NULL;
        }
        else if (pSocket->pTask)
        {
            pSocket->DoneStatus = ntStatus;
            LwRtlCancelTask(pSocket->pTask);
            pSocket = NULL;
        }
        else
        {
            SrvSocketFree(pSocket);
            pSocket = NULL;
        }
    }

    goto cleanup;
}

VOID
SrvSocketFree(
    IN OUT PSRV_SOCKET pSocket
    )
{
    if (pSocket)
    {
        SrvSocketProcessTaskDisconnect(pSocket);
        if (pSocket->pTask)
        {
            LwRtlReleaseTask(&pSocket->pTask);
        }
        if (pSocket->fd >= 0)
        {
            close(pSocket->fd);
        }
        LwRtlCleanupMutex(&pSocket->Mutex);
        SrvFreeMemory(pSocket);
    }
}

VOID
SrvSocketGetAddress(
    IN PSRV_SOCKET pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT size_t* pAddressLength
    )
{
    // immutable, so lock not needed.
    *ppAddress = &pSocket->ClientAddress.Generic;
    *pAddressLength = pSocket->ClientAddressLength;
}

PCSTR
SrvSocketGetAddressString(
    IN PSRV_SOCKET pSocket
    )
{
    // immutable, so lock not needed.
    return pSocket->AddressStringBuffer;
}

int
SrvSocketGetFileDescriptor(
    IN PSRV_SOCKET pSocket
    )
{
    // immutable, so lock not needed.
    return pSocket->fd;
}


NTSTATUS
SrvSocketSetBuffer(
    IN PSRV_SOCKET pSocket,
    IN PVOID pBuffer,
    IN ULONG Size,
    IN ULONG Minimum
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (Minimum > Size)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }
    else if ((Size > 0) && !pBuffer)
    {
        ntStatus = STATUS_INVALID_PARAMETER;
    }

    if (ntStatus)
    {
        // This can never happen!
        LWIO_ASSERT(FALSE);

        SRV_SOCKET_LOCK(pSocket);
        pSocket->pBuffer = NULL;
        pSocket->Size = 0;
        pSocket->Minimum = 0;
        pSocket->Offset = 0;
        SRV_SOCKET_UNLOCK(pSocket);
    }
    else
    {
        SRV_SOCKET_LOCK(pSocket);
        pSocket->pBuffer = pBuffer;
        pSocket->Size = Size;
        pSocket->Minimum = Minimum;
        pSocket->Offset = 0;
        if (Size > 0)
        {
            // Notify task that there is a buffer
            LwRtlWakeTask(pSocket->pTask);
        }
        SRV_SOCKET_UNLOCK(pSocket);
    }

    return ntStatus;
}

static
NTSTATUS
SrvSocketSendReplyCommon(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN OPTIONAL PLW_ZCT_VECTOR pZct,
    IN OPTIONAL PVOID pBuffer,
    IN ULONG Size
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SEND_ITEM pSendItem = NULL;
    BOOLEAN bIsLocked = FALSE;

    LWIO_ASSERT(!IS_BOTH_OR_NEITHER(pZct, pBuffer));
    LWIO_ASSERT(!pZct || (0 == Size));
    LWIO_ASSERT(!pBuffer || (Size > 0));

    ntStatus = SrvAllocateMemory(sizeof(*pSendItem), OUT_PPVOID(&pSendItem));
    BAIL_ON_NT_STATUS(ntStatus);

    pSendItem->pSendContext = pSendContext;
    if (pZct)
    {
        pSendItem->pZct = pZct;
    }
    else
    {
        pSendItem->pBuffer = pBuffer;
        pSendItem->Length = Size;
    }

    SRV_SOCKET_LOCK_WITH(&bIsLocked, pSocket);

    ntStatus = SrvSocketGetDispatch(pSocket)->pfnSendPrepare(pSendContext);
    BAIL_ON_NT_STATUS(ntStatus);

    // Cannot fail after this.

    LwListInsertTail(&pSocket->SendHead, &pSendItem->SendLinks);

    if (IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_WRITABLE))
    {
        // Try to write inline.
        ntStatus = SrvSocketProcessTaskWrite(pSocket);
    }

    // Wake task to process list or send error
    if (!LwListIsEmpty(&pSocket->SendHead) || ntStatus)
    {
        LwRtlWakeTask(pSocket->pTask);
    }

    ntStatus = STATUS_SUCCESS;

cleanup:

    SRV_SOCKET_UNLOCK_WITH(&bIsLocked, pSocket);

    return ntStatus;

error:

    if (pSendItem)
    {
        SrvFreeMemory(pSendItem);
    }

    goto cleanup;
}

NTSTATUS
SrvSocketSendReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PVOID pBuffer,
    IN ULONG Size
    )
{
    return SrvSocketSendReplyCommon(
                pSocket,
                pSendContext,
                NULL,
                pBuffer,
                Size);
}

NTSTATUS
SrvSocketSendZctReply(
    IN PSRV_SOCKET pSocket,
    IN PSRV_SEND_CONTEXT pSendContext,
    IN PLW_ZCT_VECTOR pZct
    )
{
    return SrvSocketSendReplyCommon(
                pSocket,
                pSendContext,
                pZct,
                NULL,
                0);
}

VOID
SrvSocketClose(
    IN OUT PSRV_SOCKET pSocket
    )
{
    SRV_SOCKET_LOCK(pSocket);

    // Check that caller is not doing something wrong.
    LWIO_ASSERT(!IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_EXTERNAL_CLOSE));
    SetFlag(pSocket->StateMask, SRV_SOCKET_STATE_EXTERNAL_CLOSE);

    LwRtlCancelTask(pSocket->pTask);

    SRV_SOCKET_UNLOCK(pSocket);
}

static
NTSTATUS
SrvSocketProcessTaskInit(
    IN PSRV_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    /* Register fd with thread pool */
    ntStatus = LwRtlSetTaskFd(
                    pSocket->pTask,
                    pSocket->fd,
                    LW_TASK_EVENT_FD_READABLE | LW_TASK_EVENT_FD_WRITABLE);
    BAIL_ON_NT_STATUS(ntStatus);

error:

    return ntStatus;
}

static
NTSTATUS
SrvSocketRead(
    IN int FileDescriptor,
    OUT PVOID pBuffer,
    IN ULONG Length,
    OUT PULONG pBytesTransferred
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t bytesTransferred = 0;

    for (;;)
    {
        bytesTransferred = read(FileDescriptor, pBuffer, Length);
        if (bytesTransferred >= 0)
        {
            break;
        }
        if (EINTR == errno)
        {
            continue;
        }
        else if (EAGAIN == errno)
        {
            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            ntStatus = LwErrnoToNtStatus(errno);
            LWIO_ASSERT(ntStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

cleanup:

    *pBytesTransferred = bytesTransferred;

    return ntStatus;

error:

    bytesTransferred = 0;

    goto cleanup;
}

static
NTSTATUS
SrvSocketWrite(
    IN int FileDescriptor,
    OUT PVOID pBuffer,
    IN ULONG Length,
    OUT PULONG pBytesTransferred
    )
{
    NTSTATUS ntStatus = 0;
    ssize_t bytesTransferred = 0;

    for (;;)
    {
        bytesTransferred = write(FileDescriptor, pBuffer, Length);
        if (bytesTransferred >= 0)
        {
            break;
        }
        if (EINTR == errno)
        {
            continue;
        }
        else if (EAGAIN == errno)
        {
            ntStatus = STATUS_MORE_PROCESSING_REQUIRED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            ntStatus = LwErrnoToNtStatus(errno);
            LWIO_ASSERT(ntStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

cleanup:

    *pBytesTransferred = bytesTransferred;

    return ntStatus;

error:

    bytesTransferred = 0;

    goto cleanup;
}

static
NTSTATUS
SrvSocketProcessTaskRead(
    IN OUT PSRV_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG bytesRemaining = 0;
    ULONG initialOffset = pSocket->Offset;

    if (pSocket->DoneStatus)
    {
        ntStatus = pSocket->DoneStatus;
        goto cleanup;
    }

    if (!IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_READABLE))
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    LWIO_ASSERT(pSocket->Offset <= pSocket->Size);
    bytesRemaining = pSocket->Size - pSocket->Offset;
    while (bytesRemaining > 0)
    {
        PVOID pCurrent = LwRtlOffsetToPointer(pSocket->pBuffer, pSocket->Offset);
        ULONG bytesTransferred = 0;

        ntStatus = SrvSocketRead(pSocket->fd, pCurrent, bytesRemaining, &bytesTransferred);
        if (STATUS_MORE_PROCESSING_REQUIRED == ntStatus)
        {
            ClearFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_READABLE);
            ntStatus = STATUS_SUCCESS;
            break;
        }
        else if (STATUS_SUCCESS != ntStatus)
        {
            BAIL_ON_NT_STATUS(ntStatus);
        }

        if (0 == bytesTransferred)
        {
            ntStatus = STATUS_CONNECTION_RESET;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        pSocket->Offset += bytesTransferred;

        LWIO_ASSERT(pSocket->Offset <= pSocket->Size);
        bytesRemaining = pSocket->Size - pSocket->Offset;
    }

    // Call protocol transport driver, if needed.
    if ((pSocket->Offset > initialOffset) &&
        (pSocket->Offset >= pSocket->Minimum))
    {
        ntStatus = SrvSocketGetDispatch(pSocket)->pfnConnectionData(
                        pSocket->pConnection,
                        pSocket->Offset);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    pSocket->DoneStatus = ntStatus;

    goto cleanup;
}

static
NTSTATUS
SrvSocketProcessTaskWrite(
    IN OUT PSRV_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (pSocket->DoneStatus)
    {
        ntStatus = pSocket->DoneStatus;
        goto cleanup;
    }

    if (!IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_WRITABLE))
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    while (!LwListIsEmpty(&pSocket->SendHead))
    {
        PLW_LIST_LINKS pLinks = pSocket->SendHead.Next;
        PSRV_SEND_ITEM pSendItem = LW_STRUCT_FROM_FIELD(pLinks, SRV_SEND_ITEM, SendLinks);
        ULONG bytesRemaining = 0;
        ULONG bytesTransferred = 0;

        if (pSendItem->pZct)
        {
            ntStatus = LwZctWriteSocketIo(
                            pSendItem->pZct,
                            pSocket->fd,
                            &bytesTransferred,
                            &bytesRemaining);
        }
        else
        {
            PVOID pCurrent = LwRtlOffsetToPointer(pSendItem->pBuffer, pSendItem->Offset);

            LWIO_ASSERT(pSendItem->Length > pSendItem->Offset);

            bytesRemaining = pSendItem->Length - pSendItem->Offset;

            ntStatus = SrvSocketWrite(pSocket->fd, pCurrent, bytesRemaining, &bytesTransferred);
            if (STATUS_SUCCESS == ntStatus)
            {
                pSendItem->Offset += bytesTransferred;

                LWIO_ASSERT(pSendItem->Offset <= pSendItem->Length);
                bytesRemaining = pSendItem->Length - pSendItem->Offset;
            }
        }

        if (STATUS_MORE_PROCESSING_REQUIRED == ntStatus)
        {
            ClearFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_WRITABLE);
            ntStatus = STATUS_SUCCESS;
            break;
        }

        LWIO_ASSERT(NT_SUCCESS_OR_NOT(ntStatus));
        BAIL_ON_NT_STATUS(ntStatus);

        if (!bytesRemaining)
        {
            SrvSocketGetDispatch(pSocket)->pfnSendDone(
                    pSendItem->pSendContext,
                    STATUS_SUCCESS);

            LwListRemove(&pSendItem->SendLinks);
            SrvFreeMemory(pSendItem);
        }
    }

cleanup:

    return ntStatus;

error:

    pSocket->DoneStatus = ntStatus;

    goto cleanup;
}

static
VOID
SrvSocketProcessTaskDisconnect(
    IN PSRV_SOCKET pSocket
    )
{
    if (pSocket->pConnection)
    {
        NTSTATUS sendStatus = pSocket->DoneStatus;

        if (!sendStatus)
        {
            sendStatus = STATUS_CONNECTION_DISCONNECTED;
        }

        while (!LwListIsEmpty(&pSocket->SendHead))
        {
            PLW_LIST_LINKS pLinks = pSocket->SendHead.Next;
            PSRV_SEND_ITEM pSendItem = LW_STRUCT_FROM_FIELD(pLinks, SRV_SEND_ITEM, SendLinks);

            SrvSocketGetDispatch(pSocket)->pfnSendDone(
                            pSendItem->pSendContext,
                            sendStatus);

            LwListRemove(&pSendItem->SendLinks);
            SrvFreeMemory(pSendItem);
        }

        SrvSocketGetDispatch(pSocket)->pfnConnectionDone(
                pSocket->pConnection,
                pSocket->DoneStatus);

        pSocket->pConnection = NULL;
    }
}

static
VOID
SrvSocketProcessTask(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_SOCKET pSocket = (PSRV_SOCKET) pDataContext;
    BOOLEAN bIsLocked = FALSE;
    LW_TASK_EVENT_MASK waitMask = 0;

    // Handle EVENT_CANCEL - this only happens when the socket is closed
    // by the protocol transport driver.

    // TODO-what about listener shutdown?!  -- NEED TO HANDLE THAT CASE,
    // probably by setting a boolean on the listener, waking the group,
    // and checking for that to do a disconnect, which will cause the
    // protocol to close the socket.

    if (IsSetFlag(WakeMask, LW_TASK_EVENT_CANCEL))
    {
        SrvSocketFree(pSocket);
        pSocket = NULL;
        waitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    SRV_SOCKET_LOCK_WITH(&bIsLocked, pSocket);

    // Process any error conditions.
    if (pSocket->DoneStatus)
    {
        ntStatus = pSocket->DoneStatus;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // Handle EVENT_INIT:
    if (IsSetFlag(WakeMask, LW_TASK_EVENT_INIT))
    {
        ntStatus = SrvSocketProcessTaskInit(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    //
    // Handle EVENT_FD_{WRITABLE,READABLE}
    //

    // Save FD state (in case we cannot fully act on it now).
    if (IsSetFlag(WakeMask, LW_TASK_EVENT_FD_WRITABLE))
    {
        SetFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_WRITABLE);
    }
    if (IsSetFlag(WakeMask, LW_TASK_EVENT_FD_READABLE))
    {
        SetFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_READABLE);
    }

    // Process FD state, handling write first so we can free up
    // memory.

    ntStatus = SrvSocketProcessTaskWrite(pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSocketProcessTaskRead(pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    // Determine what wait mask is needed.
    if ((pSocket->Size - pSocket->Offset) > 0)
    {
        SetFlag(waitMask, LW_TASK_EVENT_FD_READABLE);
        if (IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_READABLE))
        {
            SetFlag(waitMask, LW_TASK_EVENT_YIELD);
        }
    }
    if (!LwListIsEmpty(&pSocket->SendHead))
    {
        SetFlag(waitMask, LW_TASK_EVENT_FD_WRITABLE);
        if (IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_WRITABLE))
        {
            SetFlag(waitMask, LW_TASK_EVENT_YIELD);
        }
    }
    if (!waitMask)
    {
        waitMask = LW_TASK_EVENT_EXPLICIT;
    }

cleanup:

    SRV_SOCKET_UNLOCK_WITH(&bIsLocked, pSocket);

    // waitMask can only be 0 (aka COMPLETE) for EVENT_CANCEL.
    LWIO_ASSERT(waitMask ||
                ((LW_TASK_EVENT_COMPLETE == waitMask) &&
                 IsSetFlag(WakeMask, LW_TASK_EVENT_CANCEL)));

    *pWaitMask = waitMask;

    return;

error:

    if (!pSocket->DoneStatus)
    {
        pSocket->DoneStatus = ntStatus;
    }

    SrvSocketProcessTaskDisconnect(pSocket);

    // Connection needs to close the socket (i.e., cancel the task).
    waitMask = LW_TASK_EVENT_EXPLICIT;

    goto cleanup;
}

