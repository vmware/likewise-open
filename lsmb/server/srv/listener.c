/* Editor Settings: expandtabs and use 4 spaces for indentation
 * ex: set softtabstop=4 tabstop=8 expandtab shiftwidth=4: *
 * -*- mode: c, c-basic-offset: 4 -*- */

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
SMBSrvListenerMain(
    PVOID pData
    );

static
PVOID
SMBSrvHandleConnection(
    PVOID pData
    );

static
VOID
SMBSrvFreeConnection(
    PSMB_CONNECTION pConnection
    );

static
DWORD
SMBSrvFakeClientConnection(
    VOID
    );

static
DWORD
SMBSrvGetLocalIPAddress(
    PSTR* ppszIpAddress
    );

DWORD
SMBSrvListenerStart(
    VOID
    )
{
    DWORD dwError = 0;

    dwError = pthread_create(
                    &gListenerThread,
                    NULL,
                    &SMBSrvListenerMain,
                    NULL);
    BAIL_ON_SMB_ERROR(dwError);

    gpListenerThread = &gListenerThread;

cleanup:

    return dwError;

error:

    goto cleanup;
}

DWORD
SMBSrvListenerStop(
    VOID
    )
{
    DWORD dwError = 0;

    if (gpListenerThread)
    {
        SMBSrvListenerIndicateMustStop();

        SMBSrvFakeClientConnection();

        pthread_join(gListenerThread, NULL);

        gpListenerThread = NULL;
    }

    return dwError;
}

static
PVOID
SMBSrvListenerMain(
    PVOID pData
    )
{
    DWORD dwError = 0;
    int sockFd = -1;
    int connFd = -1;
    struct sockaddr_in servaddr;
    pthread_t handlerThrId;
    PSMB_CONNECTION pConnection = NULL;

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr  = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SMB_SERVER_PORT);

    if (bind(sockFd, (const struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

    if (listen(sockFd, SMB_LISTEN_Q) < 0)
    {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

    while (!SMBSrvListenerShouldStop())
    {
        struct sockaddr_in cliaddr;
        SOCKLEN_T clilen;

        memset(&cliaddr, 0, sizeof(cliaddr));

        connFd = accept(sockFd, (struct sockaddr*)&cliaddr, &clilen);
        if (SMBSrvListenerShouldStop())
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
                if (SMBSrvListenerShouldStop())
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
                dwError = errno;
                BAIL_ON_SMB_ERROR(dwError);
            }
        }

        dwError = SMBAllocateMemory(
                    sizeof(SMB_CONNECTION),
                    (PVOID*)&pConnection);
        BAIL_ON_SMB_ERROR(dwError);

        pConnection->fd = connFd;
        connFd = -1;
        memcpy(&pConnection->cliaddr, &cliaddr, sizeof(cliaddr));

        dwError = pthread_create(
                    &handlerThrId,
                    NULL,
                    &SMBSrvHandleConnection,
                    pConnection);
        BAIL_ON_SMB_ERROR(dwError);

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
        SMBSrvFreeConnection(pConnection);
    }

    return NULL;

error:

    goto cleanup;
}

BOOLEAN
SMBSrvListenerShouldStop(
    VOID
    )
{
    BOOLEAN bStopListener = FALSE;

    SMB_LOCK_LISTENER;

    bStopListener = gbStopListener;

    SMB_UNLOCK_LISTENER;

    return bStopListener;
}

VOID
SMBSrvListenerIndicateMustStop(
    VOID
    )
{
    SMB_LOCK_LISTENER;

    gbStopListener = TRUE;

    SMB_UNLOCK_LISTENER;
}

static
PVOID
SMBSrvHandleConnection(
    PVOID pData
    )
{
    DWORD dwError = 0;
    PSMB_CONNECTION pConnection = (PSMB_CONNECTION)pData;
    PSMB_SOCKET pSocket = NULL;

    pthread_detach(pthread_self());

    BAIL_ON_INVALID_POINTER(pConnection);

    SMB_LOG_INFO("Handling client from [%s]",
                 SMB_SAFE_LOG_STRING(inet_ntoa(pConnection->cliaddr.sin_addr)));

    dwError = SMBSrvSocketCreate(
                    pConnection->fd,
                    pConnection->cliaddr,
                    &pSocket);
    BAIL_ON_SMB_ERROR(dwError);

    while(!SMBSrvListenerShouldStop())
    {
        SMB_CONNECTION request_packet;
        SMB_CONNECTION response_packet;

        // TODO: Use a timeout
        dwError = SMBPacketReceiveAndUnmarshall(
                    pSocket,
                    &request_packet);
        BAIL_ON_SMB_ERROR(dwError);

        dwError = SMBSrvProcessRequest_V1(
                    &request_packet,
                    &response_packet);
        BAIL_ON_SMB_ERROR(dwError);

        dwError = SMBPacketSend(
                    pSocket,
                    &response_packet);
        BAIL_ON_SMB_ERROR(dwError);
    }

cleanup:

    SMB_LOG_INFO("Closing connection from client [%s] [fd:%d]",
                 SMB_SAFE_LOG_STRING(inet_ntoa(pConnection->cliaddr.sin_addr)),
                    pConnection->fd);

    if (pSocket)
    {
        SMBSocketRelease(pSocket);
    }

    SMBSrvFreeConnection(pConnection);

    return NULL;

error:

    SMB_LOG_ERROR("Error when handling SMB Connection [code:%d]", dwError);

    goto cleanup;
}

static
VOID
SMBSrvFreeConnection(
    PSMB_CONNECTION pConnection
    )
{
    if (pConnection->fd >= 0)
    {
        close(pConnection->fd);
    }
    SMBFreeMemory(pConnection);
}


static
DWORD
SMBSrvFakeClientConnection(
    VOID
    )
{
    DWORD dwError = 0;
    int sockFd = -1;
    struct sockaddr_in servaddr;
    PSTR pszLocalIPAddress = NULL;

    dwError = SMBSrvGetLocalIPAddress(&pszLocalIPAddress);
    BAIL_ON_SMB_ERROR(dwError);

    sockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockFd < 0)
    {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SMB_SERVER_PORT);
    if (inet_pton(AF_INET, pszLocalIPAddress, &servaddr.sin_addr) < 0)
    {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

    if (connect(sockFd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0)
    {
        dwError = errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

cleanup:

    if (sockFd >= 0)
    {
        close(sockFd);
    }

    SMB_SAFE_FREE_STRING(pszLocalIPAddress);

    return dwError;

error:

    goto cleanup;
}

static
DWORD
SMBSrvGetLocalIPAddress(
    PSTR* ppszIpAddress
    )
{
    DWORD dwError = 0;
    struct hostent* pHost = NULL;
    PSTR pszIpAddress = NULL;

    pHost = gethostbyname("localhost");
    if (!pHost)
    {
        dwError = h_errno;
        BAIL_ON_SMB_ERROR(dwError);
    }

    dwError = SMBAllocateString(
                inet_ntoa(*(struct in_addr*)(pHost->h_addr_list[0])),
                &pszIpAddress);
    BAIL_ON_SMB_ERROR(dwError);

    *ppszIpAddress = pszIpAddress;

cleanup:

    return dwError;

error:

    SMB_SAFE_FREE_STRING(pszIpAddress);

    *ppszIpAddress = NULL;

    goto cleanup;
}
