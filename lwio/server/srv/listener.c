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
    PSMB_SRV_LISTENER_CONTEXT pContext
    );

static
PSMB_SRV_READER
SrvFindLeastBusyReader(
    PSMB_SRV_READER pReaderArray,
    ULONG           ulNumReaders
    );

static
DWORD
SrvFakeClientConnection(
    VOID
    );

static
DWORD
SMBSrvGetLocalIPAddress(
    PSTR* ppszIpAddress
    );

NTSTATUS
SrvListenerInit(
    PSMB_SRV_LISTENER pListener
    )
{
    NTSTATUS ntStatus = 0;

    memset(&pListener->context, 0, sizeof(pListener->context));

    ntStatus = pthread_create(
                    pListener->listener,
                    NULL,
                    &SrvListenerMain,
                    &pListener->context);
    BAIL_ON_NT_STATUS;

    pListener->pListener = &pListener->listener;

error:

    return ntStatus;
}

NTSTATUS
SrvListenerStop(
    PSMB_SRV_LISTENER pListener
    )
{
    NTSTATUS ntStatus = 0;

    if (pListener->pListener)
    {
        pthread_mutex_lock(&pListener->context.mutex);

        pListener->context.bStop = TRUE;

        pthread_mutex_unlock(&pListener->context.mutex);

        SrvFakeClientConnection();

        pthread_join(pListener->listener, NULL);

        pListener->pListener = NULL;
    }

    return ntStatus;
}

static
PVOID
SrvListenerMain(
    PVOID pData
    )
{
    DWORD dwError = 0;
    int sockFd = -1;
    int connFd = -1;
    struct sockaddr_in servaddr;
    PSMB_SRV_SOCKET pSocket = NULL;
    PSMB_SRV_LISTENER_CONTEXT pContext = (PSMB_SRV_LISTENER_CONTEXT)pData;
    PSMB_SRV_READER pReader = NULL;

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

    while (!SrvListenerMustStop(pContext))
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
                dwError = errno;
                BAIL_ON_SMB_ERROR(dwError);
            }
        }

        SMB_LOG_INFO("Handling client from [%s]",
                     SMB_SAFE_LOG_STRING(inet_ntoa(cliaddr.sin_addr)));

        dwError = SrvSocketCreate(
                        connFd,
                        &cliaddr,
                        &pSocket);
        BAIL_ON_SMB_ERROR(dwError);

        connFd = -1;

        dwError = SrvConnectionCreate(
                        pSocket,
                        &pConnection);
        BAIL_ON_SMB_ERROR(dwError);

        pSocket = NULL;

        pReader = SrvFindLeastBusyReader(
                        pContext->pReaderArray,
                        pContext->ulNumReaders);

        assert(pReader != NULL);

        dwError = SrvReaderEnqueueConnection(
                        pReader,
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
        SrvConnectionRelease(pConnection);
    }

    return NULL;

error:

    goto cleanup;
}

static
BOOLEAN
SrvListenerMustStop(
    PSMB_SRV_LISTENER_CONTEXT pContext
    )
{
    BOOLEAN bStop = FALSE;

    pthread_mutex_lock(&pContext->mutex);

    bStop = pContext->bStop;

    pthread_mutex_unlock(&pContext->mutex);

    return bStop;
}

static
PSMB_SRV_READER
SrvFindLeastBusyReader(
    PSMB_SRV_READER pReaderArray,
    ULONG           ulNumReaders
    )
{
    PSMB_SRV_READER pReaderMin = NULL;
    ULONG           ulNumSocketsMin = 0;
    ULONG           iReader = 0;

    for (; iReader < ulNumReaders; iReader++)
    {
        PSMB_SRV_READER pReader = &pReaderArray[iReader];

        if (!iReader)
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
DWORD
SrvFakeClientConnection(
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
