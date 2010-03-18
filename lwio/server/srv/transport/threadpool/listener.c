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
SrvListenerMain(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    );

NTSTATUS
SrvListenerInit(
    HANDLE                     hPacketAllocator,
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    PLWIO_SRV_LISTENER         pListener,
    BOOLEAN                    bEnableSecuritySignatures,
    BOOLEAN                    bRequireSecuritySignatures
    )
{
    NTSTATUS ntStatus = 0;
    PLW_THREAD_POOL_ATTRIBUTES pAttrs = NULL;

    memset(&pListener->context, 0, sizeof(pListener->context));

    pthread_mutex_init(&pListener->context.mutex, NULL);
    pListener->context.pMutex = &pListener->context.mutex;

    pListener->context.hPacketAllocator = hPacketAllocator;

    uuid_generate(pListener->context.serverProperties.GUID);

    pListener->context.serverProperties.preferredSecurityMode = SMB_SECURITY_MODE_USER;
    pListener->context.serverProperties.bEnableSecuritySignatures =
                                                    bEnableSecuritySignatures;
    pListener->context.serverProperties.bRequireSecuritySignatures =
                                                    bRequireSecuritySignatures;
    pListener->context.serverProperties.bEncryptPasswords = TRUE;
    pListener->context.serverProperties.MaxRawSize = 64 * 1024;
    pListener->context.serverProperties.MaxMpxCount = 50;
    pListener->context.serverProperties.MaxNumberVCs = 1;
    pListener->context.serverProperties.MaxBufferSize = 16644;
    pListener->context.serverProperties.Capabilities = 0;
    pListener->context.serverProperties.Capabilities |= CAP_UNICODE;
    pListener->context.serverProperties.Capabilities |= CAP_LARGE_FILES;
    pListener->context.serverProperties.Capabilities |= CAP_NT_SMBS;
    pListener->context.serverProperties.Capabilities |= CAP_RPC_REMOTE_APIS;
    pListener->context.serverProperties.Capabilities |= CAP_STATUS32;
    pListener->context.serverProperties.Capabilities |= CAP_LEVEL_II_OPLOCKS;
    pListener->context.serverProperties.Capabilities |= CAP_LARGE_READX;
    pListener->context.serverProperties.Capabilities |= CAP_LARGE_WRITEX;
    pListener->context.serverProperties.Capabilities |= CAP_EXTENDED_SECURITY;

    pListener->context.pShareList = pShareList;

    ntStatus = LwRtlCreateThreadPoolAttributes(&pAttrs);
    BAIL_ON_NT_STATUS(ntStatus);

    /* Our tasks sometimes make blocking IPC calls, so use a private set of task threads */
    LwRtlSetThreadPoolAttribute(pAttrs, LW_THREAD_POOL_OPTION_DELEGATE_TASKS, FALSE);
    /* Create one task thread per CPU */
    LwRtlSetThreadPoolAttribute(pAttrs, LW_THREAD_POOL_OPTION_TASK_THREADS, -1);
    /* We don't presently use work threads, so turn them off */
    LwRtlSetThreadPoolAttribute(pAttrs, LW_THREAD_POOL_OPTION_WORK_THREADS, 0);

    ntStatus = LwRtlCreateThreadPool(&pListener->context.pPool, pAttrs);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTaskGroup(pListener->context.pPool, &pListener->context.pTaskGroup);
    BAIL_ON_NT_STATUS(ntStatus);

    ntStatus = LwRtlCreateTask(
        pListener->context.pPool,
        &pListener->context.pTask,
        pListener->context.pTaskGroup,
        SrvListenerMain,
        &pListener->context);
    BAIL_ON_NT_STATUS(ntStatus);

    LwRtlWakeTask(pListener->context.pTask);

error:

    LwRtlFreeThreadPoolAttributes(&pAttrs);

    return ntStatus;
}

NTSTATUS
SrvListenerShutdown(
    PLWIO_SRV_LISTENER pListener
    )
{
    NTSTATUS ntStatus = 0;

    if (pListener->context.pTaskGroup)
    {
        LwRtlCancelTaskGroup(pListener->context.pTaskGroup);
        LwRtlWaitTaskGroup(pListener->context.pTaskGroup);
        LwRtlFreeTaskGroup(&pListener->context.pTaskGroup);
    }

    LwRtlReleaseTask(&pListener->context.pTask);

    if (pListener->context.pPool)
    {
        LwRtlFreeThreadPool(&pListener->context.pPool);
    }

    if (pListener->context.hGssContext)
    {
        SrvGssReleaseContext(pListener->context.hGssContext);
    }

    if (pListener->context.pMutex)
    {
        pthread_mutex_destroy(&pListener->context.mutex);
        pListener->context.pMutex = NULL;
    }

    return ntStatus;
}

static
NTSTATUS
SrvListenerMainInit(
    PLWIO_SRV_LISTENER_CONTEXT pContext
    )
{
    NTSTATUS ntStatus = 0;
    struct sockaddr_in servaddr;
    int on = 1;
    long opts = 0;

    pContext->listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if (pContext->listenFd < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (setsockopt(pContext->listenFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

#ifdef TCP_NODELAY
    if (setsockopt(pContext->listenFd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }
#endif

    /* Put socket in nonblock mode */
    opts = fcntl(pContext->listenFd, F_GETFL, 0);
    if (opts < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }
    opts |= O_NONBLOCK;
    if (fcntl(pContext->listenFd, F_SETFL, opts) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SMB_SERVER_PORT);

    if (bind(pContext->listenFd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (listen(pContext->listenFd, SMB_LISTEN_Q) < 0)
    {
        ntStatus = LwErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    /* Register fd with thread pool */
    ntStatus = LwRtlSetTaskFd(
        pContext->pTask,
        pContext->listenFd,
        LW_TASK_EVENT_FD_READABLE);
    BAIL_ON_NT_STATUS(ntStatus);

cleanup:

    return ntStatus;

error:

    if (pContext->listenFd >= 0)
    {
        close(pContext->listenFd);
        pContext->listenFd = -1;
    }

    goto cleanup;
}

static
VOID
SrvListenerMain(
    PLW_TASK pTask,
    PVOID pDataContext,
    LW_TASK_EVENT_MASK WakeMask,
    LW_TASK_EVENT_MASK* pWaitMask,
    PLONG64 pllTime
    )
{
    NTSTATUS ntStatus = 0;
    int connFd = -1;
    PLWIO_SRV_SOCKET pSocket = NULL;
    PLWIO_SRV_LISTENER_CONTEXT pContext = (PLWIO_SRV_LISTENER_CONTEXT) pDataContext;
    PLWIO_SRV_CONNECTION pConnection = NULL;
    PSRV_HOST_INFO pHostinfo = NULL;
    CHAR   remoteIpAddr[256];
    struct sockaddr_in cliaddr;
    SOCKLEN_T clilen = 0;
    PLW_TASK pConnTask = NULL;

    if (WakeMask & LW_TASK_EVENT_INIT)
    {
        LWIO_LOG_DEBUG("Srv listener starting");

        ntStatus = SrvListenerMainInit(pContext);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (WakeMask & LW_TASK_EVENT_CANCEL)
    {
        LWIO_LOG_DEBUG("Srv listener stopping");

        if (pContext->listenFd >= 0)
        {
            close(pContext->listenFd);
            pContext->listenFd = -1;
        }

        *pWaitMask = LW_TASK_EVENT_COMPLETE;
        goto cleanup;
    }

    clilen = sizeof(cliaddr);
    memset(&cliaddr, 0, sizeof(cliaddr));

    connFd = accept(pContext->listenFd, (struct sockaddr*)&cliaddr, &clilen);

    if (connFd < 0)
    {
        if (errno == EPROTO || errno == ECONNABORTED || errno == EINTR)
        {
            *pWaitMask = LW_TASK_EVENT_YIELD;
            goto cleanup;
        }
        else if (errno == EAGAIN)
        {
            *pWaitMask = LW_TASK_EVENT_FD_READABLE;
            goto cleanup;
        }
        else
        {
            ntStatus = LwErrnoToNtStatus(errno);
            BAIL_ON_NT_STATUS(ntStatus);
        }
    }

    if (getpeername(connFd, (struct sockaddr*)&cliaddr, &clilen) < 0)
    {
        LWIO_LOG_WARNING("Failed to find the remote socket address for [fd:%d][code:%d]", connFd, errno);
    }

    LWIO_LOG_INFO("Handling client from [%s]",
                  SMB_SAFE_LOG_STRING(inet_ntop(
                                          AF_INET,
                                          &cliaddr.sin_addr,
                                          remoteIpAddr,
                                          sizeof(remoteIpAddr))));

    if (!pHostinfo)
    {
        NTSTATUS ntStatus1 = SrvAcquireHostInfo(
            NULL,
            &pHostinfo);
        if (ntStatus1)
        {
            LWIO_LOG_ERROR("Failed to acquire current host information [code:%d]", ntStatus1);
            close(connFd);
            connFd = -1;

            *pWaitMask = LW_TASK_EVENT_YIELD;
            goto cleanup;
        }
    }

    if (!pContext->hGssContext)
    {
        NTSTATUS ntStatus2 = 0;

        ntStatus2 = SrvGssAcquireContext(
            pHostinfo,
            NULL,
            &pContext->hGssContext);
        if (ntStatus2)
        {
            LWIO_LOG_ERROR("Failed to initialize GSS Handle [code:%d]", ntStatus2);


            close(connFd);
            connFd = -1;

            *pWaitMask = LW_TASK_EVENT_YIELD;
            goto cleanup;
        }
    }

    ntStatus = SrvSocketCreate(
        connFd,
        &cliaddr,
        &pSocket);
    BAIL_ON_NT_STATUS(ntStatus);

    connFd = -1;

    ntStatus = SrvConnectionCreate(
        pSocket,
        pContext->hPacketAllocator,
        pContext->hGssContext,
        pContext->pShareList,
        &pContext->serverProperties,
        pHostinfo,
        &SrvSocketFree,
        &pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    pSocket = NULL;

    ntStatus = LwRtlCreateTask(
        pContext->pPool,
        &pConnTask,
        pContext->pTaskGroup,
        SrvSocketReaderProcess,
        pConnection);
    BAIL_ON_NT_STATUS(ntStatus);

    pConnection = NULL;

    LwRtlWakeTask(pConnTask);

    *pWaitMask = LW_TASK_EVENT_YIELD;

cleanup:

    if (connFd >= 0)
    {
        close(connFd);
    }

    if (pConnection)
    {
        SrvConnectionRelease(pConnection);
    }

    if (pConnTask)
    {
        LwRtlReleaseTask(&pConnTask);
    }

    if (pHostinfo)
    {
        SrvReleaseHostInfo(pHostinfo);
    }

    return;

error:

    if (pConnTask)
    {
        LwRtlCancelTask(pConnTask);
    }

    goto cleanup;
}
