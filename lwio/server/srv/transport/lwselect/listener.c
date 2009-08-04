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
PVOID
SrvListenerMain(
    PVOID pData
    );

static
BOOLEAN
SrvListenerMustStop(
    PLWIO_SRV_LISTENER_CONTEXT pContext
    );

static
PLWIO_SRV_SOCKET_READER
SrvFindLeastBusyReader(
    PLWIO_SRV_SOCKET_READER pReaderArray,
    ULONG           ulNumReaders
    );

static
NTSTATUS
SrvFakeClientConnection(
    VOID
    );

static
NTSTATUS
SMBSrvGetLocalIPAddress(
    PSTR* ppszIpAddress
    );

NTSTATUS
SrvListenerInit(
    HANDLE                    hPacketAllocator,
    PLWIO_SRV_SHARE_ENTRY_LIST pShareList,
    PLWIO_SRV_SOCKET_READER    pReaderArray,
    ULONG                     ulNumReaders,
    PLWIO_SRV_LISTENER         pListener
    )
{
    NTSTATUS ntStatus = 0;

    memset(&pListener->context, 0, sizeof(pListener->context));

    pthread_mutex_init(&pListener->context.mutex, NULL);
    pListener->context.pMutex = &pListener->context.mutex;

    pListener->context.hPacketAllocator = hPacketAllocator;
    pListener->context.pReaderArray = pReaderArray;
    pListener->context.ulNumReaders = ulNumReaders;
    pListener->context.bStop = FALSE;

    uuid_generate(pListener->context.serverProperties.GUID);

    pListener->context.serverProperties.preferredSecurityMode = SMB_SECURITY_MODE_USER;
    pListener->context.serverProperties.bEnableSecuritySignatures = TRUE;
    pListener->context.serverProperties.bRequireSecuritySignatures = TRUE;
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
    // pListener->context.serverProperties.Capabilities |= CAP_LEVEL_II_OPLOCKS;
    pListener->context.serverProperties.Capabilities |= CAP_LARGE_READX;
    pListener->context.serverProperties.Capabilities |= CAP_LARGE_WRITEX;
    pListener->context.serverProperties.Capabilities |= CAP_EXTENDED_SECURITY;

    pListener->context.pShareList = pShareList;

    ntStatus = pthread_create(
                    &pListener->listener,
                    NULL,
                    &SrvListenerMain,
                    &pListener->context);
    BAIL_ON_NT_STATUS(ntStatus);

    pListener->pListener = &pListener->listener;

error:

    return ntStatus;
}

NTSTATUS
SrvListenerShutdown(
    PLWIO_SRV_LISTENER pListener
    )
{
    NTSTATUS ntStatus = 0;
    BOOLEAN  bInLock = FALSE;

    if (pListener->pListener)
    {
        if (pListener->context.pMutex)
        {
            LWIO_LOCK_MUTEX(bInLock, &pListener->context.mutex);
        }

        pListener->context.bStop = TRUE;

        if (pListener->context.pMutex)
        {
            LWIO_UNLOCK_MUTEX(bInLock, &pListener->context.mutex);
        }

        SrvFakeClientConnection();

        pthread_join(pListener->listener, NULL);

        pListener->pListener = NULL;
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
PVOID
SrvListenerMain(
    PVOID pData
    )
{
    NTSTATUS ntStatus = 0;
    int sockFd = -1;
    int connFd = -1;
    struct sockaddr_in servaddr;
    PLWIO_SRV_SOCKET pSocket = NULL;
    PLWIO_SRV_LISTENER_CONTEXT pContext = (PLWIO_SRV_LISTENER_CONTEXT)pData;
    PLWIO_SRV_SOCKET_READER pReader = NULL;
    PLWIO_SRV_CONNECTION pConnection = NULL;
    PSRV_HOST_INFO pHostinfo = NULL;
    int on = 1;

    LWIO_LOG_DEBUG("Srv listener starting");

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (setsockopt(sockFd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

#ifdef TCP_NODELAY
    if (setsockopt(sockFd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }
#endif

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SMB_SERVER_PORT);

    if (bind(sockFd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (listen(sockFd, SMB_LISTEN_Q) < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    while (!SrvListenerMustStop(pContext))
    {
        CHAR   remoteIpAddr[256];
        struct sockaddr_in cliaddr;
        SOCKLEN_T clilen = sizeof(cliaddr);

        memset(&cliaddr, 0, sizeof(cliaddr));

        connFd = accept(sockFd, (struct sockaddr*)&cliaddr, &clilen);
        if (SrvListenerMustStop(pContext))
        {
            goto cleanup;
        }

        if (connFd < 0)
        {
            if (errno == EPROTO || errno == ECONNABORTED)
            {
                continue;
            }
            else if (errno == EINTR)
            {
                if (SrvListenerMustStop(pContext))
                {
                    goto cleanup;
                }
                else
                {
                    continue;
                }
            }
            else
            {
                ntStatus = LwUnixErrnoToNtStatus(errno);
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

                continue;
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

                continue;
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

        pReader = SrvFindLeastBusyReader(
                        pContext->pReaderArray,
                        pContext->ulNumReaders);

        assert(pReader != NULL);

        ntStatus = SrvSocketReaderEnqueueConnection(
                        pReader,
                        pConnection);
        BAIL_ON_NT_STATUS(ntStatus);

        pConnection = NULL;
    }

cleanup:

    if (sockFd >= 0)
    {
        close(sockFd);
    }

    if (connFd >= 0)
    {
        close(connFd);
    }

    if (pConnection)
    {
        SrvConnectionRelease(pConnection);
    }

    LWIO_LOG_DEBUG("Srv listener stopping");

    return NULL;

error:

    goto cleanup;
}

static
BOOLEAN
SrvListenerMustStop(
    PLWIO_SRV_LISTENER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pContext->mutex);

    bStop = pContext->bStop;

    pthread_mutex_unlock(&pContext->mutex);

    return bStop;
}

static
PLWIO_SRV_SOCKET_READER
SrvFindLeastBusyReader(
    PLWIO_SRV_SOCKET_READER pReaderArray,
    ULONG                  ulNumReaders
    )
{
    PLWIO_SRV_SOCKET_READER pReaderMin = NULL;
    ULONG                  ulNumSocketsMin = 0;
    ULONG                  iReader = 0;

    for (; iReader < ulNumReaders; iReader++)
    {
        PLWIO_SRV_SOCKET_READER pReader = &pReaderArray[iReader];

        if (!SrvSocketReaderIsActive(pReader))
        {
            LWIO_LOG_DEBUG("Reader [id:%u] is in-active", pReader->readerId);

            continue;
        }

        if (!pReaderMin)
        {
            pReaderMin = pReader;
            ulNumSocketsMin = SrvSocketReaderGetCount(pReader);
        }
        else
        {
            ULONG ulNumSockets = SrvSocketReaderGetCount(pReader);

            if (ulNumSockets < ulNumSocketsMin)
            {
                ulNumSocketsMin = ulNumSockets;
                pReaderMin = pReader;
            }
        }
    }

    return pReaderMin;
}

static
NTSTATUS
SrvFakeClientConnection(
    VOID
    )
{
    NTSTATUS ntStatus = 0;
    int sockFd = -1;
    struct sockaddr_in servaddr;
    PSTR pszLocalIPAddress = NULL;

    ntStatus = SMBSrvGetLocalIPAddress(&pszLocalIPAddress);
    BAIL_ON_NT_STATUS(ntStatus);

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SMB_SERVER_PORT);
    if (inet_pton(AF_INET, pszLocalIPAddress, &servaddr.sin_addr) < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    if (connect(sockFd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        ntStatus = LwUnixErrnoToNtStatus(errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

cleanup:

    if (sockFd >= 0)
    {
        close(sockFd);
    }

    LWIO_SAFE_FREE_STRING(pszLocalIPAddress);

    return ntStatus;

error:

    goto cleanup;
}

static
NTSTATUS
SMBSrvGetLocalIPAddress(
    PSTR* ppszIpAddress
    )
{
    NTSTATUS ntStatus = 0;
    CHAR  szHostIpAddress[256];
    struct hostent* pHost = NULL;
    PSTR pszIpAddress = NULL;

    pHost = gethostbyname("localhost");
    if (!pHost)
    {
        ntStatus = LwUnixErrnoToNtStatus(h_errno);
        BAIL_ON_NT_STATUS(ntStatus);
    }

    ntStatus = SMBAllocateString(
                inet_ntop(
                    AF_INET,
                    pHost->h_addr_list[0],
                    szHostIpAddress,
                    sizeof(szHostIpAddress)),
                &pszIpAddress);
    BAIL_ON_NT_STATUS(ntStatus);

    *ppszIpAddress = pszIpAddress;

cleanup:

    return ntStatus;

error:

    LWIO_SAFE_FREE_STRING(pszIpAddress);

    *ppszIpAddress = NULL;

    goto cleanup;
}
