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
 *        listener.c
 *
 * Abstract:
 *
 *        Likewise SMB Subsystem
 *
 *        SMB Service Listener
 *
 * Authors: Krishna Ganugapati (krishnag@likewisesoftware.com)
 *          Sriram Nambakam (snambakam@likewisesoftware.com)
 */
#include "includes.h"

static
VOID
SrvListenerProcessTask(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    );

static
NTSTATUS
SrvListenerInitSocket(
    PSRV_TRANSPORT_LISTENER pListener,
    BOOLEAN bInet6
    )
{
    NTSTATUS ntStatus = 0;
    int on = 1;
    long opts = 0;

    if (bInet6)
    {
#ifdef AF_INET6
        pListener->Addr.Addr6.sin6_family = AF_INET6;
        /* Leave address zero */
        pListener->Addr.Addr6.sin6_port = htons(SMB_SERVER_PORT);
        pListener->AddrLen = sizeof(pListener->Addr.Addr6);
#else
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
#endif
    }
    else
    {
        pListener->Addr.Addr4.sin_family = AF_INET;
        pListener->Addr.Addr4.sin_addr.s_addr  = htonl(INADDR_ANY);
        pListener->Addr.Addr4.sin_port = htons(SMB_SERVER_PORT);
        pListener->AddrLen = sizeof(pListener->Addr.Addr4);
    }

    pListener->ListenFd = socket(
        ((const struct sockaddr*)&pListener->Addr)->sa_family,
        SOCK_STREAM,
        0);

    if (pListener->ListenFd < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (setsockopt(pListener->ListenFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

#ifdef TCP_NODELAY
    if (setsockopt(pListener->ListenFd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }
#endif

    /* Put socket in nonblock mode */
    opts = fcntl(pListener->ListenFd, F_GETFL, 0);
    if (opts < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    opts |= O_NONBLOCK;
    if (fcntl(pListener->ListenFd, F_SETFL, opts) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (bind(pListener->ListenFd, (const struct sockaddr*)&pListener->Addr, pListener->AddrLen) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (listen(pListener->ListenFd, SMB_LISTEN_QUEUE_LENGTH) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    return ntStatus;

error:

    if (pListener->ListenFd >= 0)
    {
        close(pListener->ListenFd);
        pListener->ListenFd = -1;
    }

    goto cleanup;
}

NTSTATUS
SrvListenerInit(
    OUT PSRV_TRANSPORT_LISTENER pListener,
    IN SRV_TRANSPORT_HANDLE pTransport,
    IN BOOLEAN bInet6
    )
{
    NTSTATUS ntStatus = 0;

    RtlZeroMemory(pListener, sizeof(*pListener));

    pListener->pTransport = pTransport;

    ntStatus = SrvListenerInitSocket(pListener, bInet6);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTaskGroup(pTransport->pPool, &pListener->pTaskGroup);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTask(
                    pTransport->pPool,
                    &pListener->pTask,
                    pListener->pTaskGroup,
                    SrvListenerProcessTask,
                    pListener);
    BAIL_ON_NT_STATUS(ntStatus);

    LwRtlWakeTask(pListener->pTask);

cleanup:

    return ntStatus;

error:

    SrvListenerShutdown(pListener);

    goto cleanup;
}

VOID
SrvListenerShutdown(
    IN OUT PSRV_TRANSPORT_LISTENER pListener
    )
{
    if (pListener->pTaskGroup)
    {
        LwRtlCancelTaskGroup(pListener->pTaskGroup);
        LwRtlWaitTaskGroup(pListener->pTaskGroup);
        LwRtlFreeTaskGroup(&pListener->pTaskGroup);
    }

    LwRtlReleaseTask(&pListener->pTask);

    RtlZeroMemory(pListener, sizeof(*pListener));
}

static
NTSTATUS
SrvListenerProcessTaskInit(
    PSRV_TRANSPORT_LISTENER pListener
    )
{
    NTSTATUS ntStatus = 0;

    /* Register fd with thread pool */
    ntStatus = LwRtlSetTaskFd(
        pListener->pTask,
        pListener->ListenFd,
        LW_TASK_EVENT_FD_READABLE);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    if (pListener->ListenFd >= 0)
    {
        close(pListener->ListenFd);
        pListener->ListenFd = -1;
    }

    goto cleanup;
}

static
VOID
SrvListenerProcessTask(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    )
{
    NTSTATUS ntStatus = STATUS_SUCCESS;
    PSRV_TRANSPORT_LISTENER pListener = (PSRV_TRANSPORT_LISTENER) pDataContext;
    int connFd = -1;
    PSRV_SOCKET pSocket = NULL;
    union
    {
        struct sockaddr_in Addr4;
#ifdef AF_INET6
        struct sockaddr_in6 Addr6;
#endif
    } clientAddress;

    SOCKLEN_T clientAddressLength = sizeof(clientAddress);
    CHAR clientAddressStringBuffer[SRV_SOCKET_ADDRESS_STRING_MAX_SIZE];
    LW_TASK_EVENT_MASK waitMask = 0;

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        LWIO_LOG_DEBUG("Srv listener starting");

        ntStatus = SrvListenerProcessTaskInit(pListener);
        // Note that task will terminate on error.
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        LWIO_LOG_DEBUG("Srv listener stopping");

        if (pListener->ListenFd >= 0)
        {
            close(pListener->ListenFd);
            pListener->ListenFd = -1;
        }

        waitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    connFd = accept(pListener->ListenFd,
                    (struct sockaddr*) &clientAddress,
                    &clientAddressLength);
    if (connFd < 0)
    {
        if (errno == EPROTO || errno == ECONNABORTED || errno == EINTR)
        {
            waitMask = LW_TASK_EVENT_YIELD;
            goto cleanup;
        }
        else if (errno == EMFILE)
        {
            LWIO_LOG_ERROR("Failed to accept connection due to too many open files");
            waitMask = LW_TASK_EVENT_YIELD;
            goto cleanup;
        }
        else if (errno == EAGAIN)
        {
            waitMask = LW_TASK_EVENT_FD_READABLE;
            goto cleanup;
        }
        else
        {
            // Note that task will terminate.
            ntStatus = LwErrnoToNtStatus(errno);
            LWIO_LOG_ERROR("Failed to accept connection (errno = %d, status = 0x%08x)", errno, ntStatus);
            LWIO_ASSERT(ntStatus);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    // TODO - getpeername should not be necessary after accept.
    if (getpeername(connFd, (struct sockaddr*) &clientAddress, &clientAddressLength) < 0)
    {
        // Note that task will terminate.
        ntStatus = LwErrnoToNtStatus(errno);
        LWIO_LOG_ERROR("Failed to find the remote socket address for fd = %d (errno = %d, status = 0x%08x)", connFd, errno, ntStatus);
        LWIO_ASSERT(ntStatus);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SrvSocketAddressToString(
        (struct sockaddr*) &clientAddress,
        clientAddressStringBuffer,
        sizeof(clientAddressStringBuffer));
    BAIL_ON_NT_STATUS(ntStatus);

    LWIO_LOG_INFO("Handling client from '%s' on fd = %d",
                  clientAddressStringBuffer,
                  connFd);

    ntStatus = SrvSocketCreate(
                    pListener,
                    connFd,
                    (struct sockaddr*) &clientAddress,
                    clientAddressLength,
                    &pSocket);
    if (ntStatus)
    {
        // Do not terminate on this error.
        LWIO_LOG_ERROR("Failed to create transport socket for fd = %d, address = '%s' (status = 0x%08x)",
                       connFd,
                       clientAddressStringBuffer,
                       ntStatus);
        ntStatus = STATUS_SUCCESS;
        waitMask = LW_TASK_EVENT_YIELD;
        goto cleanup;
    }

    connFd = -1;

    waitMask = LW_TASK_EVENT_YIELD;

cleanup:

    SrvSocketRelease(pSocket);

    if (connFd >= 0)
    {
        close(connFd);
    }

    // waitMask can only be 0 (aka COMPLETE) for EVENT_CANCEL or error.
    LWIO_ASSERT(waitMask ||
                ((LW_TASK_EVENT_COMPLETE == waitMask) &&
                 (IsSetFlag(WakeMask, LW_TASK_EVENT_CANCEL) || ntStatus)));

    *pWaitMask = waitMask;

    return;

error:

    waitMask = LW_TASK_EVENT_COMPLETE;

    goto cleanup;
}
