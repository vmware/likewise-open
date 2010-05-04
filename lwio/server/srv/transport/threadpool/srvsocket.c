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
VOID
SrvSocketAcquire(
    IN PSRV_SOCKET pSocket
    );

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
inline
VOID
SrvSocketSetDoneStatusIf(
    IN PSRV_SOCKET pSocket,
    IN NTSTATUS Status
    )
{
    LWIO_ASSERT(Status);

    if (!pSocket->DoneStatus)
    {
        pSocket->DoneStatus = Status;
    }
}

static
VOID
SrvSocketFree(
    IN OUT PSRV_SOCKET pSocket
    );

static
NTSTATUS
SrvSocketProcessTaskReadZct(
    IN OUT PSRV_SOCKET pSocket,
    IN BOOLEAN bIsAsync
    );

static
NTSTATUS
SrvSocketProcessTaskWrite(
    IN OUT PSRV_SOCKET pSocket,
    IN OPTIONAL PSRV_SEND_ITEM pNoNotifySendItem
    );

static
VOID
SrvSocketProcessTaskDisconnect(
    IN PSRV_SOCKET pSocket,
    IN NTSTATUS Status
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

    if (ClientAddressLength > sizeof(pSocket->clientAddress))
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

    pSocket->RefCount = 1;
    LwListInit(&pSocket->SendHead);

    ntStatus = LwRtlInitializeMutex(&pSocket->Mutex, TRUE);
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket->pListener = pListener;
    pSocket->fd = fd;

    memcpy(&pSocket->clientAddress, pClientAddress, ClientAddressLength);
    pSocket->ClientAddressLength = ClientAddressLength;

    ntStatus = SrvSocketAddressToString(
                            &pSocket->clientAddress,
                            pSocket->AddressStringBuffer,
                            sizeof(pSocket->AddressStringBuffer));
    BAIL_ON_NT_STATUS(ntStatus);

    // TODO-Use separate listener and socket task groups
    // in case of shutdown as we are adding a new socket task.
    ntStatus = LwRtlCreateTask(
                    pListener->pPool,
                    &pSocket->pTask,
                    pListener->pTaskGroup,
                    SrvSocketProcessTask,
                    pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    // Referenced by task
    SrvSocketAcquire(pSocket);

    LwRtlWakeTask(pSocket->pTask);

cleanup:

    *ppSocket = pSocket;

    return ntStatus;

error:

    if (pSocket)
    {
        if (pSocket->pTask)
        {
            pSocket->DoneStatus = ntStatus;
            LwRtlWakeTask(pSocket->pTask);
        }

        SrvSocketRelease(pSocket);
        pSocket = NULL;
    }

    goto cleanup;
}

static
VOID
SrvSocketAcquire(
    IN PSRV_SOCKET pSocket
    )
{
    LONG count = InterlockedIncrement(&pSocket->RefCount);
    LWIO_ASSERT(count > 1);
}

VOID
SrvSocketRelease(
    IN OUT PSRV_SOCKET pSocket
    )
{
    if (pSocket)
    {
        LONG count = InterlockedDecrement(&pSocket->RefCount);
        LWIO_ASSERT(count >= 0);
        if (0 == count)
        {
            SrvSocketFree(pSocket);
        }
    }
}

static
VOID
SrvSocketFree(
    IN OUT PSRV_SOCKET pSocket
    )
{
    LWIO_ASSERT(!pSocket->pConnection);
    LWIO_ASSERT(LwListIsEmpty(&pSocket->SendHead));
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

VOID
SrvSocketGetAddress(
    IN PSRV_SOCKET              pSocket,
    OUT const struct sockaddr** ppAddress,
    OUT SOCKLEN_T*              pAddressLength
    )
{
    // immutable, so lock not needed.
    *ppAddress = &pSocket->clientAddress;
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
            LWIO_ASSERT(!pSocket->pZct);
            // Notify task that there is a buffer
            LwRtlWakeTask(pSocket->pTask);
        }
        SRV_SOCKET_UNLOCK(pSocket);
    }

    return ntStatus;
}

NTSTATUS
SrvSocketReceiveZct(
    IN PSRV_SOCKET pSocket,
    IN PLW_ZCT_VECTOR pZct
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG zctSize = 0;

    SRV_SOCKET_LOCK(pSocket);

    if (!pZct)
    {
        // Caller behavior is underfined.
        LWIO_ASSERT(FALSE);
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    zctSize = LwZctGetRemaining(pZct);
    if (!zctSize)
    {
        // Caller behavior is underfined.
        LWIO_ASSERT(FALSE);
        ntStatus = STATUS_INVALID_PARAMETER;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pSocket->Size)
    {
        // Internal state is inconsistent.
        LWIO_ASSERT(FALSE);
        ntStatus = STATUS_ASSERTION_FAILURE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (pSocket->pZct)
    {
        // Internal state is inconsistent.
        LWIO_ASSERT(FALSE);
        ntStatus = STATUS_ASSERTION_FAILURE;
        BAIL_ON_NT_STATUS(ntStatus);
    }

    pSocket->pZct = pZct;
    pSocket->ZctSize = zctSize;

    // Try to receive inline
    ntStatus = SrvSocketProcessTaskReadZct(pSocket, FALSE);
    BAIL_ON_NT_STATUS(ntStatus);

    zctSize = LwZctGetRemaining(pSocket->pZct);
    if (zctSize > 0)
    {
        // Notify task that it needs to read into ZCT
        LwRtlWakeTask(pSocket->pTask);
        ntStatus = STATUS_PENDING;
    }
    else
    {
        pSocket->pZct = NULL;
        pSocket->ZctSize = 0;

        ntStatus = STATUS_SUCCESS;
    }

cleanup:

    SRV_SOCKET_UNLOCK(pSocket);

    return ntStatus;

error:

    LWIO_ASSERT(ntStatus != STATUS_PENDING);

    pSocket->pZct = NULL;
    pSocket->ZctSize = 0;

    goto cleanup;
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

    ntStatus = pSocket->DoneStatus;
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSocketGetDispatch(pSocket)->pfnSendPrepare(pSendContext);
    BAIL_ON_NT_STATUS(ntStatus);

    // Cannot fail after this.

    LwListInsertTail(&pSocket->SendHead, &pSendItem->SendLinks);

    if (IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_WRITABLE))
    {
        // Try to write inline.
        ntStatus = SrvSocketProcessTaskWrite(pSocket, pSendItem);
    }

    // Wake task to process list or send error
    if (!LwListIsEmpty(&pSocket->SendHead) || ntStatus)
    {
        ntStatus = STATUS_PENDING;
        LwRtlWakeTask(pSocket->pTask);
    }
    else
    {
        ntStatus = STATUS_SUCCESS;
    }

cleanup:

    SRV_SOCKET_UNLOCK_WITH(&bIsLocked, pSocket);

    return ntStatus;

error:

    LWIO_ASSERT(ntStatus != STATUS_PENDING);

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
SrvSocketDisconnect(
    IN PSRV_SOCKET pSocket
    )
{
    SRV_SOCKET_LOCK(pSocket);
    SrvSocketSetDoneStatusIf(pSocket, STATUS_CONNECTION_DISCONNECTED);
    LwRtlWakeTask(pSocket->pTask);
    SRV_SOCKET_UNLOCK(pSocket);
}

VOID
SrvSocketClose(
    IN OUT PSRV_SOCKET pSocket
    )
{
    SRV_SOCKET_LOCK(pSocket);

    // Check that caller is not doing something wrong.
    LWIO_ASSERT(!IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_CLOSED));
    SetFlag(pSocket->StateMask, SRV_SOCKET_STATE_CLOSED);

    // No connection any more.  This disables notifications to the PTD.
    pSocket->pConnection = NULL;

    SrvSocketSetDoneStatusIf(pSocket, STATUS_CONNECTION_DISCONNECTED);
    LwRtlWakeTask(pSocket->pTask);

    SRV_SOCKET_UNLOCK(pSocket);

    // Release PTD's reference.
    SrvSocketRelease(pSocket);
}

static
NTSTATUS
SrvSocketProcessTaskInit(
    IN PSRV_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    // Notify PTD about new connection.
    ntStatus = SrvSocketGetDispatch(pSocket)->pfnConnectionNew(
                    &pSocket->pConnection,
                    pSocket->pListener->pTransport->pContext,
                    pSocket);
    if (ntStatus)
    {
        LWIO_LOG_ERROR("New connection failed in protocol for fd = %d, address = '%s' with status = 0x%08x",
                       pSocket->fd,
                       pSocket->AddressStringBuffer,
                       ntStatus);
    }
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_ASSERT(pSocket->pConnection);

    // Take a reference for the PTD's new connection object.
    SrvSocketAcquire(pSocket);

    // Register file descriptor with thread pool for read/write.
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
SrvSocketProcessTaskReadBuffer(
    IN OUT PSRV_SOCKET pSocket
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG bytesRemaining = 0;
    ULONG initialOffset = pSocket->Offset;

    ntStatus = pSocket->DoneStatus;
    BAIL_ON_NT_STATUS(ntStatus);

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
SrvSocketProcessTaskReadZct(
    IN OUT PSRV_SOCKET pSocket,
    IN BOOLEAN bIsAsync
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG bytesRemaining = 0;

    ntStatus = pSocket->DoneStatus;
    BAIL_ON_NT_STATUS(ntStatus);

    if (!IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_FD_READABLE) && bIsAsync)
    {
        ntStatus = STATUS_SUCCESS;
        goto cleanup;
    }

    bytesRemaining = LwZctGetRemaining(pSocket->pZct);
    while (bytesRemaining > 0)
    {
        ULONG bytesTransferred = 0;

        ntStatus = LwZctReadSocketIo(pSocket->pZct, pSocket->fd, &bytesTransferred, &bytesRemaining);
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
    }

    // Call protocol transport driver, if needed.
    if (!bytesRemaining && bIsAsync)
    {
        ULONG size = pSocket->ZctSize;

        pSocket->pZct = NULL;
        pSocket->ZctSize = 0;

        ntStatus = SrvSocketGetDispatch(pSocket)->pfnConnectionData(
                        pSocket->pConnection,
                        size);
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
SrvSocketProcessTaskRead(
    IN OUT PSRV_SOCKET pSocket
    )
{
    if (pSocket->ZctSize > 0)
    {
        return SrvSocketProcessTaskReadZct(pSocket, TRUE);
    }
    else
    {
        return SrvSocketProcessTaskReadBuffer(pSocket);
    }
}

static
NTSTATUS
SrvSocketProcessTaskWrite(
    IN OUT PSRV_SOCKET pSocket,
    IN OPTIONAL PSRV_SEND_ITEM pNoNotifySendItem
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = pSocket->DoneStatus;
    BAIL_ON_NT_STATUS(ntStatus);

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
            if (pSendItem != pNoNotifySendItem)
            {
                SrvSocketGetDispatch(pSocket)->pfnSendDone(
                        pSendItem->pSendContext,
                        STATUS_SUCCESS);
            }

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
    IN PSRV_SOCKET pSocket,
    IN NTSTATUS Status
    )
{
    SrvSocketSetDoneStatusIf(pSocket, Status);

    LWIO_ASSERT(pSocket->DoneStatus);

    // For debugging only - the state machine should call in here just once
    // because this function will cancel the task.

    LWIO_ASSERT(!IsSetFlag(pSocket->StateMask, SRV_SOCKET_STATE_DISCONNECTED));
    SetFlag(pSocket->StateMask, SRV_SOCKET_STATE_DISCONNECTED);

    if (pSocket->pConnection)
    {
        while (!LwListIsEmpty(&pSocket->SendHead))
        {
            PLW_LIST_LINKS pLinks = pSocket->SendHead.Next;
            PSRV_SEND_ITEM pSendItem = LW_STRUCT_FROM_FIELD(pLinks, SRV_SEND_ITEM, SendLinks);

            SrvSocketGetDispatch(pSocket)->pfnSendDone(
                            pSendItem->pSendContext,
                            pSocket->DoneStatus);

            LwListRemove(&pSendItem->SendLinks);
            SrvFreeMemory(pSendItem);
        }

        SrvSocketGetDispatch(pSocket)->pfnConnectionDone(
                pSocket->pConnection,
                pSocket->DoneStatus);

        pSocket->pConnection = NULL;
    }

    LWIO_ASSERT(LwListIsEmpty(&pSocket->SendHead));

    if (pSocket->fd >= 0)
    {
        NTSTATUS ntStatus = LwRtlSetTaskFd(pSocket->pTask, pSocket->fd, 0);
        if (ntStatus)
        {
            LWIO_LOG_ERROR("Unexpected set task FD error for client '%s', "
                           "fd = %d0x%08x, status = 0x%08x",
                           pSocket->AddressStringBuffer,
                           pSocket->fd,
                           ntStatus);
        }
        close(pSocket->fd);
        pSocket->fd = -1;
    }

    LwRtlCancelTask(pSocket->pTask);
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

    SRV_SOCKET_LOCK_WITH(&bIsLocked, pSocket);

    if (IsSetFlag(WakeMask, LW_TASK_EVENT_CANCEL))
    {
        if (pSocket->pConnection)
        {
            ntStatus = STATUS_CANCELLED;
            BAIL_ON_NT_STATUS(ntStatus);
        }

        SRV_SOCKET_UNLOCK_WITH(&bIsLocked, pSocket);

        SrvSocketRelease(pSocket);
        pSocket = NULL;

        waitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    // Process any error conditions.
    ntStatus = pSocket->DoneStatus;
    BAIL_ON_NT_STATUS(ntStatus);

    // Handle EVENT_INIT:
    if (IsSetFlag(WakeMask, LW_TASK_EVENT_INIT))
    {
        ntStatus = SrvSocketProcessTaskInit(pSocket);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    // If we have made it this far, there must be an active connection.
    LWIO_ASSERT(pSocket->pConnection);

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

    ntStatus = SrvSocketProcessTaskWrite(pSocket, NULL);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = SrvSocketProcessTaskRead(pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    // Determine what wait mask is needed.
    if (((pSocket->Size - pSocket->Offset) > 0) || (pSocket->ZctSize > 0))
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

    SrvSocketProcessTaskDisconnect(pSocket, ntStatus);

    // Connection needs to close the socket (i.e., cancel the task).
    waitMask = LW_TASK_EVENT_EXPLICIT;

    goto cleanup;
}

