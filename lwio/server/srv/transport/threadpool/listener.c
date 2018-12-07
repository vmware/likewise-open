/* -*- mode: c; c-basic-offset: 4; indent-tabs-mode: nil; tab-width: 4 -*-
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * Editor Settings: expandtabs and use 4 spaces for indentation */

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
    BOOLEAN bInet6,
    BOOLEAN bNetbios
    )
{
    NTSTATUS ntStatus = 0;
    int on = 1;
    long opts = 0;

    if (bInet6)
    {
#ifdef AF_INET6
        if (bNetbios)
        {
            ntStatus = STATUS_NOT_SUPPORTED;
            BAIL_ON_NT_STATUS(ntStatus);
        }
        else
        {
            pListener->Addr.Addr6.sin6_family = AF_INET6;
            /* Leave address zero */
            pListener->Addr.Addr6.sin6_port = htons(SMB_SERVER_PORT);
            pListener->AddrLen = sizeof(pListener->Addr.Addr6);
        }
#else
        ntStatus = STATUS_NOT_SUPPORTED;
        BAIL_ON_NT_STATUS(ntStatus);
#endif
    }
    else
    {
        pListener->Addr.Addr4.sin_family = AF_INET;
        pListener->Addr.Addr4.sin_addr.s_addr  = htonl(INADDR_ANY);
        if (bNetbios)
        {
            pListener->Addr.Addr4.sin_port = htons(NETBIOS_SERVER_PORT);
        }
        else
        {
            pListener->Addr.Addr4.sin_port = htons(SMB_SERVER_PORT);
        }
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

    if (bInet6)
    {
        if (setsockopt(pListener->ListenFd, SOL_IPV6, IPV6_V6ONLY, (const char *)(&on), sizeof(on)))
        {
            ntStatus = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (setsockopt(pListener->ListenFd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) < 0)
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
        LWIO_LOG_ERROR("SrvListenerInitSocket: errno=%d", errno);
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
    IN BOOLEAN bInet6,
    IN BOOLEAN bNetbios
    )
{
    NTSTATUS ntStatus = 0;

    RtlZeroMemory(pListener, sizeof(*pListener));

    pListener->pTransport = pTransport;

    ntStatus = SrvListenerInitSocket(pListener, bInet6, bNetbios);
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
    SOCKLEN_TYPE clientAddressLength = sizeof(clientAddress);
    CHAR clientAddressStringBuffer[SRV_SOCKET_ADDRESS_STRING_MAX_SIZE] = {0};

    union
    {
        struct sockaddr_in Addr4;
#ifdef AF_INET6
        struct sockaddr_in6 Addr6;
#endif
    } serverAddress;
    SOCKLEN_TYPE serverAddressLength = sizeof(serverAddress);
    CHAR serverAddressStringBuffer[SRV_SOCKET_ADDRESS_STRING_MAX_SIZE] = {0};
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
        switch (errno)
        {
            case EPROTO:
            case ECONNABORTED:
            case EINTR:
                waitMask = LW_TASK_EVENT_YIELD;
                goto cleanup;

            case EMFILE:
            case ENFILE:
                LWIO_LOG_ERROR("Failed to accept connection due to too many open files");
                waitMask = LW_TASK_EVENT_YIELD;
                goto cleanup;

            case EAGAIN:
                waitMask = LW_TASK_EVENT_FD_READABLE;
                goto cleanup;

            default:
                // Note that task will terminate.
                ntStatus = LwErrnoToNtStatus(errno);
                LWIO_LOG_ERROR(
                    "Failed to accept connection (errno = %d, status = 0x%08x)",
                    errno,
                    ntStatus);
                LWIO_ASSERT(ntStatus);
                BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    // TODO - getpeername should not be necessary after accept.
    if (getpeername(connFd, (struct sockaddr*) &clientAddress, &clientAddressLength) < 0)
    {
        // Do not terminate on this error.
        ntStatus = LwErrnoToNtStatus(errno);
        LWIO_LOG_ERROR(
            "Failed to find the remote socket address for fd = %d (errno = %d, status = 0x%08x)",
            connFd,
            errno,
            ntStatus);
        ntStatus = STATUS_SUCCESS;
        waitMask = LW_TASK_EVENT_YIELD;
        goto cleanup;
    }

    if (getsockname(connFd, (struct sockaddr*) &serverAddress, &serverAddressLength) < 0)
    {
        // Do not terminate on this error.
        ntStatus = LwErrnoToNtStatus(errno);
        LWIO_LOG_ERROR(
            "Failed to find the local socket address for fd = %d (errno = %d, status = 0x%08x)",
            connFd,
            errno,
            ntStatus);
        ntStatus = STATUS_SUCCESS;
        waitMask = LW_TASK_EVENT_YIELD;
        goto cleanup;
    }

    ntStatus = SrvSocketAddressToString(
        (struct sockaddr*) &clientAddress,
        clientAddressStringBuffer,
        sizeof(clientAddressStringBuffer));
    if (ntStatus)
    {
        // Do not terminate on this error.
        LWIO_LOG_ERROR("Failed to allocate client address string for fd = %d (status = 0x%08x)",
                       connFd, ntStatus);
        ntStatus = STATUS_SUCCESS;
        waitMask = LW_TASK_EVENT_YIELD;
        goto cleanup;
    }

    ntStatus = SrvSocketAddressToString(
        (struct sockaddr*) &serverAddress,
        serverAddressStringBuffer,
        sizeof(serverAddressStringBuffer));
    if (ntStatus)
    {
        // Do not terminate on this error.
        LWIO_LOG_ERROR("Failed to allocate server address string for fd = %d (status = 0x%08x)",
                       connFd, ntStatus);
        ntStatus = STATUS_SUCCESS;
        waitMask = LW_TASK_EVENT_YIELD;
        goto cleanup;
    }

    LWIO_LOG_INFO("Handling client from '%s' [server address: %s] on fd = %d",
                  clientAddressStringBuffer,
                  serverAddressStringBuffer,
                  connFd);

    ntStatus = SrvSocketCreate(
                    pListener,
                    connFd,
                    (struct sockaddr*) &clientAddress,
                    clientAddressLength,
                    (struct sockaddr*) &serverAddress,
                    serverAddressLength,
                    &pSocket);
    if (ntStatus)
    {
        // Do not terminate on this error.
        LWIO_LOG_ERROR(
            "Failed to create transport socket for fd = %d, address = '%s' (status = 0x%08x)",
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

    //
    // waitMask can be one of:
    //
    // LW_TASK_EVENT_COMPLETE - for LW_TASK_EVENT_CANCEL or error
    // LW_TASK_EVENT_YIELD or LW_TASK_FD_READABLE - for !LW_TASK_EVENT_CANCEL
    //   and no error
    //

    LWIO_ASSERT(((LW_TASK_EVENT_COMPLETE == waitMask) &&
                 (IsSetFlag(WakeMask, LW_TASK_EVENT_CANCEL) || ntStatus)) ||
                (((LW_TASK_EVENT_YIELD == waitMask) ||
                  (LW_TASK_EVENT_FD_READABLE == waitMask)) &&
                 (!IsSetFlag(WakeMask, LW_TASK_EVENT_CANCEL) && !ntStatus)));

    *pWaitMask = waitMask;

    return;

error:

    waitMask = LW_TASK_EVENT_COMPLETE;
    LWIO_ASSERT(ntStatus);

    goto cleanup;
}
